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
  TcpNoordwijk();

  virtual int Send (Ptr<Packet> p, uint32_t flags);  // Call by app to send data to network

protected:
  virtual void NewAck (SequenceNumber32 const& seq); // Update buffers w.r.t. ACK
  virtual void DupAck (const TcpHeader& t, uint32_t count);
  virtual void Retransmit (void); // Retransmit timeout

private:
  void RateTracking();
  void RateAdjustment(const Time& delta, const Time& deltaRtt);

  uint8_t m_burstSize;
  uint8_t m_retrBurstMod; // Retransmission modifier of the burst size. Retrans 5 packet, next burst is
                          // (m_burstSize - 5) packets
  Time m_firstAck;
  uint8_t m_ackCount;
  uint8_t m_trainReceived;

  Time m_minRtt;
};

}; // namespace ns3

#endif // TCPNOORDWIJK_H
