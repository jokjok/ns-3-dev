/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 Natale Patriciello <natale.patriciello@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#include "tcp-noordwijk.h"
#include "ns3/abort.h"
#include "ns3/node.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/log.h"
#include "ns3/ipv4.h"
#include "ns3/ipv6.h"
#include "ns3/ipv4-interface-address.h"
#include "ns3/ipv4-route.h"
#include "ns3/ipv6-route.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv6-routing-protocol.h"
#include "ns3/simulation-singleton.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/trace-source-accessor.h"
#include "tcp-l4-protocol.h"
#include "ipv4-end-point.h"
#include "ipv6-end-point.h"
#include "tcp-header.h"

NS_LOG_COMPONENT_DEFINE ("TcpNoordwijk");

using namespace ns3;

#define STAB_FACTOR 2
#define BETA Time::FromInteger(2, Time::S)
#define DEFAULT_BURST_SIZE 20

TcpNoordwijk::TcpNoordwijk ()
  : m_burstSize(DEFAULT_BURST_SIZE),
    m_retrBurstMod(0),
    m_ackCount(0),
    m_trainReceived(0)
{
  m_minRtt = Time::Max ();
  SetTcpNoDelay(true);
}

int TcpNoordwijk::Send (Ptr<Packet> p, uint32_t flags)
{
  NS_LOG_FUNCTION (this << p);
  NS_ABORT_MSG_IF (flags, "use of flags is not supported in TcpNoordwijk::Send()");
  if (m_state == ESTABLISHED || m_state == SYN_SENT || m_state == CLOSE_WAIT)
    {
      // Store the packet into Tx buffer
      if (!m_txBuffer.Add (p))
        { // TxBuffer overflow, send failed
          m_errno = ERROR_MSGSIZE;
          return -1;
        }
      if (m_shutdownSend)
        {
          m_errno = ERROR_SHUTDOWN;
          return -1;
        }

      // Transmit if burst size is reached
      NS_LOG_LOGIC ("txBufSize=" << m_txBuffer.Size () << " state " << TcpStateName[m_state]);

      if (m_txBuffer.Size() >= (uint8_t)(m_burstSize-m_retrBurstMod))
        {
          if (m_state == ESTABLISHED || m_state == CLOSE_WAIT)
            { // Try to send the data out
              SendPendingData (m_connected);
              m_retrBurstMod = 0;
            }
        }

      return p->GetSize ();
    }
  else
    { // Connection not established yet
      m_errno = ERROR_NOTCONN;
      return -1; // Send failure
    }
}

void
TcpNoordwijk::RateAdjustment(const Time& deltaGrande, const Time& deltaRtt)
{
  int64_t denominatore = 1 + (deltaRtt.GetMilliSeconds() / deltaGrande.GetMilliSeconds());
  m_burstSize = m_burstSize / denominatore;
}

void
TcpNoordwijk::RateTracking()
{
  m_burstSize = m_burstSize + ((DEFAULT_BURST_SIZE - m_burstSize) / 2);
}

// Called by the ReceivedAck() when new ACK received and by ProcessSynRcvd()
// when the three-way handshake completed. This cancels retransmission timer
// and advances Tx window
void
TcpNoordwijk::NewAck (SequenceNumber32 const& ack)
{
  NS_LOG_FUNCTION (this << ack);

  if (m_minRtt > m_lastRtt)
    {
      m_minRtt = m_lastRtt;
    }

  ++m_ackCount;

  if (m_ackCount == 1)
    {
      m_firstAck = Simulator::Now ();
    }
  else if (m_ackCount == m_burstSize)
    {
      Time deltaGrande = (Simulator::Now () - m_firstAck);
      Time deltaPiccolo = Time::FromInteger(deltaGrande.GetMilliSeconds()/m_burstSize, Time::MS);

      m_ackCount = 0;
      ++m_trainReceived;

      // todo: STAB deve essere parametrizzato
      // todo: BETA deve essere parametrizzato
      if (m_trainReceived == STAB_FACTOR)
        {
          if (m_lastRtt-m_minRtt > BETA)
            {
              RateAdjustment(deltaGrande, m_lastRtt-m_minRtt);
            }
          else
            {
              RateTracking();
            }

          m_trainReceived = 0;
        }
    }

  if (m_state != SYN_RCVD)
    { // Set RTO unless the ACK is received in SYN_RCVD state
      NS_LOG_LOGIC (this << " Cancelled ReTxTimeout event which was set to expire at " <<
                    (Simulator::Now () + Simulator::GetDelayLeft (m_retxEvent)).GetSeconds ());
      m_retxEvent.Cancel ();
      // On recieving a "New" ack we restart retransmission timer .. RFC 2988
      m_rto = m_rtt->RetransmitTimeout ();
      NS_LOG_LOGIC (this << " Schedule ReTxTimeout at time " <<
                    Simulator::Now ().GetSeconds () << " to expire at time " <<
                    (Simulator::Now () + m_rto.Get ()).GetSeconds ());
      m_retxEvent = Simulator::Schedule (m_rto, &TcpNoordwijk::ReTxTimeout, this);
    }
  if (m_rWnd.Get () == 0 && m_persistEvent.IsExpired ())
    { // Zero window: Enter persist state to send 1 byte to probe
      NS_LOG_LOGIC (this << "Enter zerowindow persist state");
      NS_LOG_LOGIC (this << "Cancelled ReTxTimeout event which was set to expire at " <<
                    (Simulator::Now () + Simulator::GetDelayLeft (m_retxEvent)).GetSeconds ());
      m_retxEvent.Cancel ();
      NS_LOG_LOGIC ("Schedule persist timeout at time " <<
                    Simulator::Now ().GetSeconds () << " to expire at time " <<
                    (Simulator::Now () + m_persistTimeout).GetSeconds ());
      m_persistEvent = Simulator::Schedule (m_persistTimeout, &TcpNoordwijk::PersistTimeout, this);
      NS_ASSERT (m_persistTimeout == Simulator::GetDelayLeft (m_persistEvent));
    }
  // Note the highest ACK and tell app to send more
  NS_LOG_LOGIC ("TCP " << this << " NewAck " << ack <<
                " numberAck " << (ack - m_txBuffer.HeadSequence ())); // Number bytes ack'ed
  m_txBuffer.DiscardUpTo (ack);
  if (GetTxAvailable () > 0)
    {
      NotifySend (GetTxAvailable ());
    }
  if (ack > m_nextTxSequence)
    {
      m_nextTxSequence = ack; // If advanced
    }
  if (m_txBuffer.Size () == 0 && m_state != FIN_WAIT_1 && m_state != CLOSING)
    { // No retransmit timer if no data to retransmit
      NS_LOG_LOGIC (this << " Cancelled ReTxTimeout event which was set to expire at " <<
                    (Simulator::Now () + Simulator::GetDelayLeft (m_retxEvent)).GetSeconds ());
      m_retxEvent.Cancel ();
    }

  // tcp-socket-base here will try to send more data; we should wait fill of burst.
}

void
TcpNoordwijk::DupAck (const TcpHeader& t, uint32_t count)
{
  NS_LOG_FUNCTION (this << "t " << count);

  // Retransmit SYN packet
  if (m_state == SYN_SENT)
    {
      if (m_cnCount > 0)
        {
          SendEmptyPacket (TcpHeader::SYN);
        }
      else
        {
          NotifyConnectionFailed ();
        }
      return;
    }
  // Retransmit non-data packet: Only if in FIN_WAIT_1 or CLOSING state
  if (m_txBuffer.Size () == 0)
    {
      if (m_state == FIN_WAIT_1 || m_state == CLOSING)
        { // Must have lost FIN, re-send
          SendEmptyPacket (TcpHeader::FIN);
        }
      return;
    }

  uint32_t packetsRetransmitted = m_txBuffer.Size();

  // Retransmit all data packet at once

  if (SendPendingData (m_connected))
    {
      m_retrBurstMod = (uint8_t) packetsRetransmitted;
    }

}

// Retransmit timeout
void
TcpNoordwijk::Retransmit (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC (this << " ReTxTimeout Expired at time " << Simulator::Now ().GetSeconds ());

  // If erroneous timeout in closed/timed-wait state, just return
  if (m_state == CLOSED || m_state == TIME_WAIT)
    return;

  // If all data are received (non-closing socket and nothing to send), just return
  if (m_state <= ESTABLISHED && m_txBuffer.HeadSequence () >= m_highTxMark)
    return;

  m_nextTxSequence = m_txBuffer.HeadSequence (); // Restart from highest Ack
  m_dupAckCount = 0;

  m_rtt->IncreaseMultiplier ();             // Double the next RTO
  DoRetransmit ();                          // Retransmit only one packet (the head)
}

void
TcpNoordwijk::SetSegSize (uint32_t size)
{
  (void) size;
  NS_ABORT_MSG ("TcpNoordwijik::SetSegSize() not available");
}

void
TcpNoordwijk::SetSSThresh (uint32_t threshold)
{
  (void) threshold;
  NS_ABORT_MSG ("TcpNoordwijik::SetSSThresh() not available");
}

uint32_t
TcpNoordwijk::GetSSThresh (void) const
{
  return 0;
}

void
TcpNoordwijk::SetInitialCwnd (uint32_t cwnd)
{
  (void) cwnd;
  NS_ABORT_MSG ("TcpNoordwijik::SetInitialCwnd() not available");
}

uint32_t
TcpNoordwijk::GetInitialCwnd (void) const
{
  return 0;
}

Ptr<TcpSocketBase>
TcpNoordwijk::Fork (void)
{
  return CopyObject<TcpNoordwijk> (this);
}
