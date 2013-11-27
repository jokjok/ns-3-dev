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

#ifndef TCPNOORDWIJK_H
#define TCPNOORDWIJK_H

#include "tcp-socket-base.h"

namespace ns3 {

class TcpNoordwijk : public TcpSocketBase
{
public:
  static TypeId GetTypeId (void);

  TcpNoordwijk();

  // From TcpSocketBase
  virtual int Connect (const Address &address);
  virtual int Listen (void);

protected:
  virtual void NewAck (SequenceNumber32 const& seq); // Update buffers w.r.t. ACK
  virtual Ptr<TcpSocketBase> Fork (void); // Call CopyObject<TcpReno> to clone me
  virtual void DupAck (const TcpHeader& t, uint32_t count);
  virtual void Retransmit (void); // Retransmit timeout
  virtual bool SendPendingData (bool withAck = false); // Send as much as the window allows

  // Implementing ns3::TcpSocket -- Attribute get/set
  virtual void     SetSSThresh (uint32_t threshold);
  virtual uint32_t GetSSThresh (void) const;
  virtual void     SetInitialCwnd (uint32_t cwnd);
  virtual uint32_t GetInitialCwnd (void) const;

private:
  void ExitPrioMode();
  void RateTracking();
  void RateAdjustment(const Time& delta, const Time& deltaRtt);

  uint32_t PrioritySendData(SequenceNumber32 const& from, SequenceNumber32 const& to,
                            bool withAck);

  uint32_t m_stabFactor;
  uint32_t m_defBurstSize;
  int32_t m_congThresold;

  TracedValue<uint32_t> m_burstSize;
  uint32_t m_burstUsed;

  Time m_firstAck;
  uint32_t m_ackCount;
  uint32_t m_trainReceived;

  Time m_minRtt;
  Time m_medRtt;

  SequenceNumber32 m_lastByteOfLastBurst;

  uint32_t m_packetsRetransmitted;
  bool m_prioMode;

  EventId m_prioModeEvent;
  Time m_prioIgnoreDupAckTime;
};

}; // namespace ns3

#endif // TCPNOORDWIJK_H
