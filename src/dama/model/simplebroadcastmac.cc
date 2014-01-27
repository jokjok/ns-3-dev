/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 Natale Patriciello, UNIMORE, <natale.patriciello@gmail.com>
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
#include "simplebroadcastmac.h"

#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("SimpleBroadcastMac");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (SimpleBroadcastMac);

TypeId
SimpleBroadcastMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SimpleBroadcastMac")
    .SetParent<DamaMac> ()
    .AddConstructor<SimpleBroadcastMac> ()
    .AddAttribute ("MaxPacketNumber", "If a packet arrives when there are "
                   "already this number of packets, it is dropped.",
                   UintegerValue (400),
                   MakeUintegerAccessor (&SimpleBroadcastMac::m_maxSize),
                   MakeUintegerChecker<uint32_t> ())
    ;
  return tid;
}

SimpleBroadcastMac::Item::Item (Ptr<const Packet> packet,
                       Mac48Address to,
                       Time tstamp)
  : packet (packet),
    to (to),
    tstamp (tstamp)
{
}

SimpleBroadcastMac::SimpleBroadcastMac() :
    m_maxSize(0)
{
  NS_LOG_FUNCTION_NOARGS ();
}

bool
SimpleBroadcastMac::Enqueue (Ptr<const Packet> packet, Mac48Address to,
                             Mac48Address from)
{
  NS_LOG_FUNCTION(this);
  NS_LOG_WARN("Enqueue with from address; not supported. Calling Enqueue without");

  (void) from;
  return Enqueue(packet, to);
}

bool
SimpleBroadcastMac::Enqueue (Ptr<const Packet> packet, Mac48Address to)
{
  NS_LOG_FUNCTION(this);

  NS_LOG_DEBUG ("Queue Size: " << m_queue.size () << " Max Size: " << m_maxSize);

  if (m_queue.size () == m_maxSize)
    {
      m_macTxDropTrace (packet);
      return false;
    }

  Time now = Simulator::Now ();
  m_queue.push_back (Item (packet, to, now));

  m_macTxTrace (packet);

  NS_LOG_DEBUG ("Inserted packet of size: " << packet->GetSize ()
                                            << " uid: " << packet->GetUid ());

  return true;
}

bool
SimpleBroadcastMac::SupportsSendFrom (void) const
{
  return true;
}

bool
SimpleBroadcastMac::HasDataToSend () const
{
  return (! m_queue.empty ());
}

bool
SimpleBroadcastMac::SendQueueHead ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (! m_queue.empty ());

  struct Item item = m_queue.front ();
  m_queue.pop_front ();

  Ptr<Packet> p = item.packet->Copy ();
  EthernetHeader header;
  header.SetDestination (item.to);
  header.SetSource (GetAddress ());

  p->AddHeader (header);

  Ptr<DamaChannel> channel = DynamicCast<DamaChannel> (GetDevice ()->GetChannel());

  if (channel == 0)
    {
      NS_LOG_INFO ("SimpleBroadcastMac::SendQueueHead(): channel " <<
                   GetDevice ()->GetChannel() << " not of type ns3::DamaChannel");
      return false;
    }

  m_promiscSnifferTrace (p);
  m_snifferTrace (p);

  channel->Send (p, this);

  return true;
}

void
SimpleBroadcastMac::Receive (Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << packet);

  EthernetHeader header;
  Ptr<Packet> p = packet->Copy ();

  p->RemoveHeader (header);

  m_upCallback(p, header.GetSource (), header.GetDestination ());
}

} // namespace ns3
