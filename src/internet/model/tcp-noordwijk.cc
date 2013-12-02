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
    .AddAttribute ("TxTimer", "Default transmission timer",
                    TimeValue (MilliSeconds (500)),
                    MakeTimeAccessor (&TcpNoordwijk::m_defTxTime),
                    MakeTimeChecker ())
    .AddAttribute ("B", "Congestion thresold",
                    TimeValue (MilliSeconds (200)),
                    MakeTimeAccessor (&TcpNoordwijk::m_congThresold),
                    MakeTimeChecker ())
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
  : m_firstAck(Time::FromInteger(0, Time::MS)),
    m_ackCount(0),
    m_trainReceived(0),
    m_minRtt(Time::Max ()),
    m_packetsRetransmitted(0),
    m_lastAckedSegmentInRTO(0),
    m_txTimer(Timer::CANCEL_ON_DESTROY),
    m_restore(false),
    m_lambda(2),
    m_burstMin(3)
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
  m_txTime = m_defTxTime;

  return TcpSocketBase::Connect (address);
}

void
TcpNoordwijk::ConnectionSucceeded (void)
{
  NS_LOG_FUNCTION(this);

  TcpSocketBase::ConnectionSucceeded();
}

void
TcpNoordwijk::StartTxTimer()
{
  NS_LOG_FUNCTION(this);

  m_txTimer.SetFunction(&TcpNoordwijk::SendPendingData, this);
  m_txTimer.SetArguments(true);
  m_txTimer.SetDelay(m_txTime);
  m_txTimer.Schedule();
}

/**
 * \brief Send as much pending data as possible according to the Tx window and the burst size
 *
 * In TCP Noordwijk, sender is limited, as it can't send more packets than the burst size. When
 * called, this method checks if we have enough data to send (burst size) or the timer is expired
 * (tx_timer), and if the latter holds, start send one burst of packets, without exceeding
 * its dimension.
 *
 * When the burst size and tx_timer are updated? When ACKs are received, in TcpNoordwijk::NewAck.
 *
 * If a retransmission timer expires, perform a recovery actions (see TcpNoordwijk::Retransmit),
 * and one of these is recover initial values of burst size and tx_timer. We do it after the last ack of
 * the burst is received, and then performs Rate adjustment or tracking.
 *
 * \see NewAck
 * \param withAck true if senders want an ack back
 * \return true if some data was sended
 */
bool
TcpNoordwijk::SendPendingData (bool withAck)
{
  NS_LOG_FUNCTION (this << withAck);

  uint32_t packetSent = 0;

  if (m_txBuffer.Size () == 0)
    {
      return false;                           // Nothing to send
    }

  if (m_endPoint == 0 && m_endPoint6 == 0)
    {
      NS_LOG_INFO (this << " No endpoint; m_shutdownSend=" << m_shutdownSend);
      return false; // Is this the right way to handle this condition?
    }

  // Wait buffer filling
  if (m_txBuffer.SizeFromSequence (m_nextTxSequence) < std::min(m_segmentSize, AvailableWindow()) * m_burstSize)
    {
      return true;
    }

  if (m_txTimer.IsRunning())
    {
      NS_LOG_LOGIC(this << " Tx Timer actually running, remaining value=" <<
                   m_txTimer.GetDelayLeft().GetMilliSeconds() << " ms");
      return true; // Actually we don't want to send, so no error..
    }

  while (m_txBuffer.SizeFromSequence (m_nextTxSequence) && packetSent < m_burstSize)
    {
      NS_LOG_LOGIC (this << " usedburst=" << packetSent <<
                   " next_tx=" << m_nextTxSequence << " so I expect ack=" <<
                   m_nextTxSequence+m_segmentSize);

      // Stop sending if we need to wait for a larger Tx window (prevent silly window syndrome)
      if (AvailableWindow() < m_segmentSize && m_txBuffer.SizeFromSequence (m_nextTxSequence) > AvailableWindow())
        {
          break; // No more
        }

      uint32_t s = std::min (AvailableWindow (), m_segmentSize);  // Send no more than window
      uint32_t sz = SendDataPacket (m_nextTxSequence, s, withAck);

      ++packetSent;                               // Count sent
      m_nextTxSequence += sz;                     // Advance next tx sequence
    }

  NS_LOG_LOGIC (this << " sent " << packetSent << " packets");

  StartTxTimer();

  return (packetSent > 0);
}

/**
 * \brief Rate Adjustment algorithm (there is congestion)
 *
 * Algorithm invoked when there are some symptom of congestion. Definining terms as:
 *
 * - \f$B_{i+1}\f$ as the next burst size
 * - \f$B_{i}\f$ as the actual burst size
 * (opportunely decreased by the number of retransmitted packets)
 * - \f$\Delta_{i}\f$ as the ack train dispersion (in other words, the arrival time
 * of first ack minus the arrival time of last ack)
 * - \f${\Delta}RTT_{i}\f$ as the difference between last RTT and the minimun RTT
 * - \f$\delta_{i} = \frac{Time_{LastAck} - Time_{FirstAck}}{B_{i}}\f$ as the ack dispersion of
 *   the \f$i\f$-th burst
 *
 * We could define next burst size as:
 * \f$B_{i+1} = \frac{\Delta_{i}}{\Delta_{i}+{\Delta}RTT_{i}} \cdot B_{i} = \frac{B_{i}}{1+\frac{{\Delta}RTT_{i}}{\Delta_{i}}}\f$
 *
 * We also compute a new TX_TIMER:
 *
 * \f$TX_TIMER_{i+1} = \lambda * B_{0} * \delta_{i}\f$
 *
 * Where \f$\lambda = 1 \f$ if \f$B_{i+1}\f$ is greater than a fixed value (3 packets), \f$\lambda=2\f$ otherwise.
 *
 * \param ackTrainDispersion arrival time of first ack minus the arrival time of last ack
 * \param deltaRtt difference between last RTT and the minimun RTT
 */
void
TcpNoordwijk::RateAdjustment(const Time& ackTrainDispersion, const Time& deltaRtt)
{
  NS_LOG_FUNCTION(this << ackTrainDispersion << deltaRtt);
  NS_LOG_LOGIC (this << " before: burst=" << m_burstSize << " tx_timer=" << m_txTimer.GetDelay()
                << " atraindisp=" << ackTrainDispersion.GetMilliSeconds()
                << " ms dRtt=" << deltaRtt.GetMilliSeconds() << " ms");

  int64_t denominator = 1 + (deltaRtt.GetMilliSeconds() / ackTrainDispersion.GetMilliSeconds());

  m_burstSize = m_burstSize / denominator;

  if (m_burstSize > m_burstMin)
    {
      m_txTimer.SetDelay(MilliSeconds(m_defBurstSize * m_ackDispersion.GetMilliSeconds()));
    }
  else
    {
      m_txTimer.SetDelay(MilliSeconds(m_lambda * m_defBurstSize * m_ackDispersion.GetMilliSeconds()));
    }

  NS_LOG_LOGIC (this << " after: burst=" << m_burstSize << " tx_timer=" << m_txTimer.GetDelay());
}

/**
 * \brief Rate tracking algorithm (no congestion)
 *
 * This algorithm aims at adapting transmission rate to the maximum allowed rate through the following
 * steps: gradually increase burst size (logarithmic grow) up to the initial burst size, and tx_timer
 * is fixed to the optimal value for default-sized bursts.
 *
 * With
 *
 * - \f$B_{0}\f$ as the default burst size
 * - \f$B_{i+1}\f$ as the next burst size
 * - \f$B_{i}\f$ as the actual burst size
 * - \f$\delta_{i} = \frac{Time_{LastAck} - Time_{FirstAck}}{B_{i}}\f$ as the ack dispersion of
 *   the \f$i\f$-th burst
 *
 * We could define next burst size as:
 *
 * \f$B_{i+1} = B_{i} + \frac{B_{0}-B_{i}}{2}\f$
 *
 * We update also the tx_timer here, with the following function:
 *
 * \f$TX_TIMER_{i+1} = B_{0} * \delta_{i}\f$
 */
void
TcpNoordwijk::RateTracking()
{
  NS_LOG_FUNCTION(this);

  NS_LOG_LOGIC (this << " before: burst=" << m_burstSize << " tx_timer=" << m_txTimer.GetDelay());
  m_burstSize = m_burstSize + ((m_defBurstSize - m_burstSize) / 2);
  m_txTimer.SetDelay(MilliSeconds(m_defBurstSize * m_ackDispersion));
  NS_LOG_LOGIC (this << " after: burst=" << m_burstSize << " tx_timer=" << m_txTimer.GetDelay());
}

/** \brief Received an ACK for a previously unacked segment
 *
 * Called by the ReceivedAck() when new ACK received and by ProcessSynRcvd()
 * when the three-way handshake completed. This cancels retransmission timer
 * and advances Tx window.
 *
 * Noordwijk adjust burst size and tx_timer after the ack of the last packet in
 * the burst is arrived; so, we need to take the arrived ack count. These adjustment
 * are made after a fixed number of arrived bursts (called stability factor, an attribute).
 *
 * When the last ack of the last burst is arrived, we compare the difference between
 * the <i>lastest RTT</i> and the minimun RTT. If it is greater
 * than a fixed value, called congestion thresold (an attribute for TCP Noordwijk) it enters
 * the Rate Adjustment algorithm (control the congestion), implemented in the
 * TcpNoordwijk::RateAdjustment method, otherwise it enters into
 * Rate Tracking algorithm, implemented in TcpNoordwijk::RateTracking method.
 *
 * After that, or when we are receiving the j-th burst (with j < stab_factor) we should
 * compute final burst dimension taking care of packets loss or burst re-inizialization.
 *
 * Packet losses are detected (and recovered) by the TcpNoordwijk::DupAck method, or when
 * a timeout expires by TcpNoordwijk::Retransmit method.
 *
 * \see RateAdjustment
 * \see RateTracking
 * \see DupAck
 * \see Retransmit
 *
 * \param ack Ack number
 */
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

      /* Noordwijk operations START */

      if (m_minRtt > m_lastRtt)
        {
          NS_LOG_LOGIC(this << " Min. RTT found, value=" << m_lastRtt.Get().GetMilliSeconds() << " ms");
          m_minRtt = m_lastRtt;
        }

      pktsAcked = (ack - m_txBuffer.HeadSequence ()) / m_segmentSize;
      m_ackCount += pktsAcked;

      NS_LOG_LOGIC(this << " seg=" << ack.GetValue() << " cumulative ACK of " <<
                   pktsAcked << " packets. lastRtt=" << m_lastRtt.Get().GetMilliSeconds());

      // Keep controlled the ack count.
      if (m_firstAck.IsZero())
        {
          m_firstAck = Simulator::Now ();
          NS_LOG_LOGIC(this << " Train first ACK, arrived at=" <<
                       m_firstAck.GetMilliSeconds() << " ms");
        }
      else if (m_ackCount >= m_burstSize)
        {
          Time ackTrainDispersion = (Simulator::Now () - m_firstAck);
          Time ackDispersion      = MilliSeconds(ackTrainDispersion.GetMilliSeconds() / m_burstSize);

          if (m_burstSize == m_defBurstSize)
            {
              m_ackDispersion = ackDispersion;
            }

          NS_LOG_LOGIC(this << " ACK train completed. aTrainDisp="
                       << ackTrainDispersion.GetMilliSeconds() << " ms, ackDisp=" << ackDispersion
                       << " but I use " << m_ackDispersion << " ms.");

          // Train and stability factor check
          ++m_trainReceived;

          if (m_trainReceived == m_stabFactor)
            {
              NS_LOG_LOGIC(this << " lastRtt=" << m_lastRtt.Get().GetMilliSeconds() <<
                           " min=" << m_minRtt.GetMilliSeconds());
              if (m_lastRtt-m_minRtt > m_congThresold)
                {
                  RateAdjustment(ackTrainDispersion, m_lastRtt-m_minRtt);
                }
              else
                {
                  RateTracking();
                }

              m_trainReceived = 0;
            }

          // Are there some retransmitted packet? If not, it is ininfluent.
          m_burstSize -= m_packetsRetransmitted;

          if (m_burstSize == 0)
            ++m_burstSize;

          m_packetsRetransmitted = 0;

          if (m_restore)
            {
              m_burstSize = m_defBurstSize;
              m_txTime = m_defTxTime;

              m_restore = false;
            }

          m_minRtt = Time::Max();
          m_ackCount = 0;
          m_firstAck = Time::FromInteger(0, Time::MS);

          // Try to send more data
          SendPendingData (m_connected);
        }
    }

  /* Noordwijk operations END */

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
}

/**
 * \brief Received a DupAck for a segment size
 *
 * There is (probably) a loss when we detect a triple dupack, so resend
 * the next segment of duplicate ack. Increase also the retransmitted packet count.
 *
 * \param t TcpHeader (unused)
 * \param count Dupack count
 */
void
TcpNoordwijk::DupAck (const TcpHeader& t, uint32_t count)
{
  NS_LOG_FUNCTION (this << "t " << count);

  NS_LOG_LOGIC(this << " Dupack for segment " << t.GetAckNumber() << " count=" << count);

  // Wait the third dupack
  if (count % 3 != 0)
    return;

  DoRetransmit();
  ++m_packetsRetransmitted;

  NS_LOG_LOGIC(this << " This burst I retransmitted " << m_packetsRetransmitted << " packets due to DupAck ");
}

/**
 * \brief Retransmit timeout
 *
 * Retransmit first non ack packet, increase the retransmitted packet count,
 * and set recover flag for the initial values of burst size and tx_timer
 *
 * In case of consecutive RTO expiration, retransmission is repeated while the default
 * tx_timer is doubled.
 */
void
TcpNoordwijk::Retransmit (void)
{
  NS_LOG_FUNCTION (this);

  if (m_txBuffer.HeadSequence () == m_lastAckedSegmentInRTO)
    {
      m_defTxTime = m_defTxTime + m_defTxTime;
    }

  m_lastAckedSegmentInRTO = m_txBuffer.HeadSequence ();

  m_restore = true;

  TcpSocketBase::Retransmit();
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
