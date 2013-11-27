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

#define NS_LOG_APPEND_CONTEXT \
  if (m_node) { std::clog << Simulator::Now ().GetSeconds () << " [node " << m_node->GetId () << "] "; }

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

NS_OBJECT_ENSURE_REGISTERED (TcpNoordwijk);

TypeId
TcpNoordwijk::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpNoordwijk")
    .SetParent<TcpSocketBase> ()
    .AddConstructor<TcpNoordwijk> ()
    .AddAttribute ("BurstSize", "Default burst size",
                    UintegerValue (20),
                    MakeUintegerAccessor (&TcpNoordwijk::m_defBurstSize),
                    MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("B", "Congestion thresold",
                    IntegerValue (200),
                    MakeIntegerAccessor (&TcpNoordwijk::m_congThresold),
                    MakeIntegerChecker<int32_t> ())
    .AddAttribute ("S", "Stability factor",
                    UintegerValue (2),
                    MakeUintegerAccessor (&TcpNoordwijk::m_stabFactor),
                    MakeUintegerChecker<uint32_t> ())
    .AddTraceSource ("BurstSize",
                     "The TCP connection's burst size",
                     MakeTraceSourceAccessor (&TcpNoordwijk::m_burstSize))
  ;
  return tid;
}

TcpNoordwijk::TcpNoordwijk ()
  : m_burstUsed(0),
    m_ackCount(0),
    m_trainReceived(0),
    m_minRtt(Time::Max ()),
    m_lastByteOfLastBurst(0),
    m_packetsRetransmitted(0),
    m_prioMode(false),
    m_prioIgnoreDupAckTime(0)
{
  SetTcpNoDelay(true);
}

int
TcpNoordwijk::Listen (void)
{
  NS_LOG_FUNCTION (this);
  m_burstSize = m_defBurstSize;

  return TcpSocketBase::Listen ();
}

int
TcpNoordwijk::Connect (const Address & address)
{
  NS_LOG_FUNCTION (this << address);
  m_burstSize = m_defBurstSize;

  return TcpSocketBase::Connect (address);
}

bool
TcpNoordwijk::SendPendingData (bool withAck)
{
  NS_LOG_FUNCTION (this << withAck);
  if (m_txBuffer.Size () == 0)
    {
      return false;                           // Nothing to send
    }
  if (m_endPoint == 0 && m_endPoint6 == 0)
    {
      NS_LOG_INFO (this << " No endpoint; m_shutdownSend=" << m_shutdownSend);
      return false; // Is this the right way to handle this condition?
    }

  // Stop sending if we don't have enough data to send
  // TODO: Disabilitare con un timer
  if (m_txBuffer.SizeFromSequence (m_nextTxSequence) < m_segmentSize*m_burstSize)
    {
      return true; // No error. We choose to don't send anything, for now
    }

  while (m_txBuffer.SizeFromSequence (m_nextTxSequence) && m_burstUsed < m_burstSize)
    {
      NS_LOG_LOGIC (this << " usedburst=" << m_burstUsed <<
                   " next_tx=" << m_nextTxSequence << " so I expect ack=" <<
                   m_nextTxSequence+m_segmentSize);

      // Stop sending if we need to wait for a larger Tx window (prevent silly window syndrome)
      if (AvailableWindow() < m_segmentSize && m_txBuffer.SizeFromSequence (m_nextTxSequence) > AvailableWindow())
        {
          break; // No more
        }

      uint32_t s = std::min (AvailableWindow (), m_segmentSize);  // Send no more than window
      uint32_t sz = SendDataPacket (m_nextTxSequence, s, withAck);

      ++m_burstUsed;                              // Count sent
      m_nextTxSequence += sz;                     // Advance next tx sequence
    }

  m_lastByteOfLastBurst = m_nextTxSequence;

  NS_LOG_LOGIC (this << " sent " << m_burstUsed << " packets");

  return (m_burstUsed > 0);
}

//TODO: English?
void
TcpNoordwijk::RateAdjustment(const Time& deltaGrande, const Time& deltaRtt)
{
  NS_LOG_FUNCTION(this << deltaGrande << deltaRtt);
  NS_LOG_LOGIC (this << " burst=" << m_burstSize << " dGrande=" << deltaGrande.GetMilliSeconds() <<
               " ms dRtt=" << deltaRtt.GetMilliSeconds());

  int64_t denominatore = 1 + (deltaRtt.GetMilliSeconds() / deltaGrande.GetMilliSeconds());

  m_burstSize = m_burstSize / denominatore;

  NS_LOG_LOGIC (this << " dopo burst=" << m_burstSize << " con denominatore=" << denominatore);
}

void
TcpNoordwijk::RateTracking()
{
  NS_LOG_FUNCTION(this);

  NS_LOG_LOGIC (this << " burst=" << m_burstSize);
  m_burstSize = m_burstSize + ((m_defBurstSize - m_burstSize) / 2);
  NS_LOG_LOGIC (this << " dopo burst=" << m_burstSize);
}

uint32_t
TcpNoordwijk::PrioritySendData(SequenceNumber32 const& from, SequenceNumber32 const& to,
                               bool withAck = false)
{
  NS_LOG_FUNCTION (this << from << to << withAck);

  if (m_txBuffer.Size () == 0)
    {
      return false;                           // Nothing to send
    }
  if (m_endPoint == 0 && m_endPoint6 == 0)
    {
      NS_LOG_INFO (this << " No endpoint; m_shutdownSend=" << m_shutdownSend);
      return false; // Is this the right way to handle this condition?
    }

  NS_LOG_LOGIC (this << " Devo spedire da " << from << " a " << to);

  SequenceNumber32 it = from;

  uint32_t nPacketsSent = 0;

  while (it < to)
    {
      uint32_t s = std::min (AvailableWindow (), m_segmentSize);  // Send no more than window
      s = std::min (s, (uint32_t)(to-it));

      uint32_t sz = SendDataPacket (it, s, withAck);

      it += sz;

      ++nPacketsSent;
    }

  return nPacketsSent;
}

// Called by the ReceivedAck() when new ACK received and by ProcessSynRcvd()
// when the three-way handshake completed. This cancels retransmission timer
// and advances Tx window
void
TcpNoordwijk::NewAck (SequenceNumber32 const& ack)
{
  NS_LOG_FUNCTION (this << ack << m_lastRtt.Get().GetMilliSeconds());

  uint32_t pktsAcked = 0;

  if (m_state != SYN_RCVD)
    { // Set RTO unless the ACK is received in SYN_RCVD state
      NS_LOG_LOGIC (this << " Cancelled ReTxTimeout event which was set to expire at " <<
                    (Simulator::Now () + Simulator::GetDelayLeft (m_retxEvent)).GetSeconds ());
      m_retxEvent.Cancel ();
      // On recieving a "New" ack we restart retransmission timer .. RFC 2988
      m_rto = m_rtt->RetransmitTimeout ();
      NS_LOG_LOGIC(this << " Schedule ReTxTimeout at time " <<
                    Simulator::Now ().GetSeconds () << " to expire at time " <<
                    (Simulator::Now () + m_rto.Get ()).GetSeconds ());
      m_retxEvent = Simulator::Schedule (m_rto, &TcpNoordwijk::ReTxTimeout, this);

      if (m_minRtt > m_lastRtt)
        {
          NS_LOG_LOGIC(this << " Min. RTT found, value=" << m_lastRtt.Get().GetMilliSeconds() << " ms");
          m_minRtt = m_lastRtt;
        }

      pktsAcked = (ack - m_txBuffer.HeadSequence ()) / m_segmentSize;
      m_ackCount += pktsAcked;

      if (m_medRtt == 0)
        {
          m_medRtt = m_lastRtt;
        }
      else
        {
          m_medRtt = (m_medRtt + m_lastRtt) / 2;
        }

      NS_LOG_LOGIC(this << " seg=" << ack.GetValue() << " cumulative ACK of " <<
                   pktsAcked << " packets. lastRtt=" << m_lastRtt.Get().GetMilliSeconds() <<
                   " medRtt=" << m_medRtt.GetMilliSeconds() << " ms");

      if (m_ackCount == 1)
        {
          m_firstAck = Simulator::Now ();
          NS_LOG_LOGIC(this << " Train first ACK, value=" << m_firstAck);
        }
      else if (m_ackCount >= m_burstSize)
        {
          // todo deltagrande e deltapiccolo ?!?! English plz
          Time deltaGrande = (Simulator::Now () - m_firstAck);
          Time deltaPiccolo = Time::FromInteger(deltaGrande.GetMilliSeconds()/m_burstSize, Time::MS);

          NS_LOG_LOGIC(this << " ACK train completed. DeltaP=" << deltaPiccolo << " deltaG=" << deltaGrande);

          if (m_prioMode)
            {
              NS_LOG_LOGIC(this << " I was in prio mode. Reduce burst by " << m_packetsRetransmitted << " pkts.");
              m_burstSize -= m_packetsRetransmitted;

              if (m_burstSize == 0)
                ++m_burstSize;

              m_packetsRetransmitted = 0;
            }

          ++m_trainReceived;

          if (m_trainReceived == m_stabFactor)
            {
              NS_LOG_LOGIC(this << " medRtt=" << m_medRtt.GetMilliSeconds() <<
                           " min=" << m_minRtt.GetMilliSeconds());
              if (m_medRtt-m_minRtt > m_congThresold)
                {
                  RateAdjustment(deltaGrande, m_medRtt-m_minRtt);
                }
              else
                {
                  RateTracking();
                }
              m_trainReceived = 0;
            }

          m_ackCount = 0;
          m_burstUsed = 0;
        }
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

  // Try to send more data
  if (m_burstSize-m_burstUsed > 0)
    {
      SendPendingData (m_connected);
    }
}

void
TcpNoordwijk::DupAck (const TcpHeader& t, uint32_t count)
{
  NS_LOG_FUNCTION (this << "t " << count);

  NS_LOG_LOGIC(this << " Dupack for segment " << t.GetAckNumber() << " count=" << count);

  // Wait the third dupack
  if (m_prioMode || count != 3)
    return;

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

  uint32_t packetsRetransmitted = PrioritySendData(t.GetAckNumber(), m_lastByteOfLastBurst, true);
  m_prioMode = true;
  m_packetsRetransmitted = packetsRetransmitted;

  m_prioIgnoreDupAckTime = m_rtt->GetCurrentEstimate();
  m_prioModeEvent = Simulator::Schedule (m_prioIgnoreDupAckTime, &TcpNoordwijk::ExitPrioMode, this);

  NS_LOG_LOGIC(this << " Retransmitted " << packetsRetransmitted << " packets due to DupAck ");
}

void
TcpNoordwijk::ExitPrioMode()
{
  NS_LOG_FUNCTION(this);

  m_prioMode = false;
}

// Retransmit timeout
void
TcpNoordwijk::Retransmit (void)
{
  NS_LOG_FUNCTION (this);

  // If erroneous timeout in closed/timed-wait state, just return
  if (m_state == CLOSED || m_state == TIME_WAIT)
    return;

  // If all data are received (non-closing socket and nothing to send), just return
  if (m_state <= ESTABLISHED && m_txBuffer.HeadSequence () >= m_highTxMark)
    return;

  NS_LOG_LOGIC(this << " ReTxTimeout Expired at time " << Simulator::Now ().GetSeconds () <<
               " recovering m_burstSize to default");

  m_nextTxSequence = m_txBuffer.HeadSequence (); // Restart from highest Ack
  m_dupAckCount = 0;
  m_burstSize = m_defBurstSize;

  m_rtt->IncreaseMultiplier ();             // Double the next RTO
  DoRetransmit ();                          // Retransmit only one packet (the head)
}

void
TcpNoordwijk::SetSSThresh (uint32_t threshold)
{
  (void) threshold;
  NS_LOG_WARN ("TCPNoordwijk does not perform slow start");
}

uint32_t
TcpNoordwijk::GetSSThresh (void) const
{
  NS_LOG_WARN ("TCPNoordwijk does not perform slow start");
  return 0;
}

void
TcpNoordwijk::SetInitialCwnd (uint32_t cwnd)
{
  (void) cwnd;
  NS_LOG_WARN ("TCPNoordwijk does not not have congestion window");
}

uint32_t
TcpNoordwijk::GetInitialCwnd (void) const
{
  NS_LOG_WARN ("TCPNoordwijk does not not have congestion window");
  return 0;
}

Ptr<TcpSocketBase>
TcpNoordwijk::Fork (void)
{
  return CopyObject<TcpNoordwijk> (this);
}
