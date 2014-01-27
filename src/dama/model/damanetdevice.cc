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

#include "damanetdevice.h"
#include "damachannel.h"
#include "damacontroller.h"
#include "damamac.h"

#include "ns3/log.h"
#include "ns3/llc-snap-header.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/pointer.h"
#include "ns3/node.h"
#include "ns3/trace-source-accessor.h"

NS_LOG_COMPONENT_DEFINE ("DamaNetDevice");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (DamaNetDevice);

TypeId
DamaNetDevice::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DamaNetDevice")
    .SetParent<NetDevice> ()
    .AddConstructor<DamaNetDevice> ()
    .AddAttribute ("Mtu", "The MAC-level Maximum Transmission Unit",
                   UintegerValue (MAX_MSDU_SIZE - LLC_SNAP_HEADER_LENGTH),
                   MakeUintegerAccessor (&DamaNetDevice::SetMtu,
                                         &DamaNetDevice::GetMtu),
                   MakeUintegerChecker<uint16_t> (1, MAX_MSDU_SIZE - LLC_SNAP_HEADER_LENGTH))
    .AddAttribute ("DamaChannel", "The channel attached to this device",
                   PointerValue (),
                   MakePointerAccessor (&DamaNetDevice::GetChannel,
                                        &DamaNetDevice::SetChannel),
                   MakePointerChecker<DamaChannel> ())
    .AddAttribute ("DamaMac", "The MAC layer attached to this device.",
                   PointerValue (),
                   MakePointerAccessor (&DamaNetDevice::GetMac,
                                        &DamaNetDevice::SetMac),
                   MakePointerChecker<DamaMac> ())
    .AddAttribute ("DamaController", "The Dama controller attached to this device",
                   PointerValue (),
                   MakePointerAccessor (&DamaNetDevice::GetController,
                                        &DamaNetDevice::SetController),
                   MakePointerChecker<DamaController> ())
    //
    // Trace sources at the "top" of the net device, where packets transition
    // to/from higher layers.
    //
    .AddTraceSource ("MacTx",
                     "A packet has been received from higher layers and is "
                     "being processed in preparation for queueing for transmission.",
                     MakeTraceSourceAccessor (&DamaNetDevice::m_macTxTrace))
    .AddTraceSource ("MacTxDrop",
                     "A packet has been dropped in the MAC layer before being "
                     "queued for transmission.",
                     MakeTraceSourceAccessor (&DamaNetDevice::m_macTxDropTrace))
    .AddTraceSource ("MacPromiscRx",
                     "A packet has been received by this device, has been passed "
                     "up from the physical layer and is being forwarded up the "
                     "local protocol stack.  This is a promiscuous trace,",
                     MakeTraceSourceAccessor (&DamaNetDevice::m_macPromiscRxTrace))
    .AddTraceSource ("MacRx",
                     "A packet has been received by this device, has been "
                     "passed up from the physical layer and is being forwarded "
                     "up the local protocol stack.  This is a non-promiscuous trace,",
                     MakeTraceSourceAccessor (&DamaNetDevice::m_macRxTrace))
    .AddTraceSource ("MacRxDrop",
                     "A packet has been dropped in the MAC layer after it "
                     "has been passed up from the physical layer.",
                     MakeTraceSourceAccessor (&DamaNetDevice::m_macRxDropTrace))
#if 0 // Not implemented (for now)
     //
     // Trace souces at the "bottom" of the net device, where packets transition
     // to/from the channel.
     //
     .AddTraceSource ("PhyTxBegin",
                      "Trace source indicating a packet has begun transmitting over the channel",
                      MakeTraceSourceAccessor (&DamaNetDevice::m_phyTxBeginTrace))
     .AddTraceSource ("PhyTxEnd",
                      "Trace source indicating a packet has been completely transmitted over the channel",
                      MakeTraceSourceAccessor (&DamaNetDevice::m_phyTxEndTrace))
     .AddTraceSource ("PhyTxDrop",
                      "Trace source indicating a packet has been dropped by the device during transmission",
                      MakeTraceSourceAccessor (&DamaNetDevice::m_phyTxDropTrace))
     .AddTraceSource ("PhyRxEnd",
                      "Trace source indicating a packet has been completely received by the device",
                      MakeTraceSourceAccessor (&DamaNetDevice::m_phyRxEndTrace))
     .AddTraceSource ("PhyRxDrop",
                      "Trace source indicating a packet has been dropped by the device during reception",
                      MakeTraceSourceAccessor (&DamaNetDevice::m_phyRxDropTrace))
#endif // if 0
     //
     // Trace sources designed to simulate a packet sniffer facility (tcpdump).
     //
     .AddTraceSource ("Sniffer",
                      "Trace source simulating a non-promiscuous packet sniffer attached to the device",
                      MakeTraceSourceAccessor (&DamaNetDevice::m_snifferTrace))
     .AddTraceSource ("PromiscSniffer",
                      "Trace source simulating a promiscuous packet sniffer attached to the device",
                      MakeTraceSourceAccessor (&DamaNetDevice::m_promiscSnifferTrace))
    ;
  return tid;
}

DamaNetDevice::DamaNetDevice ()
{
  NS_LOG_FUNCTION_NOARGS ();
}
DamaNetDevice::~DamaNetDevice ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
DamaNetDevice::DoDispose (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  m_node       = 0;
  m_controller = 0;
  m_mac        = 0;

  NetDevice::DoDispose ();
}

void
DamaNetDevice::SetMac (Ptr<DamaMac> mac)
{
  NS_ASSERT(mac != 0);
  NS_LOG_FUNCTION (this << mac);

  m_mac = mac;
  m_mac->SetDevice (this);

  m_mac->SetForwardUpCallback (MakeCallback (&DamaNetDevice::ForwardUp, this));
  m_mac->SetLinkUpCallback    (MakeCallback (&DamaNetDevice::LinkUp,    this));
  m_mac->SetLinkDownCallback  (MakeCallback (&DamaNetDevice::LinkDown,  this));

  m_mac->SetMacRxDropTrace    (m_macRxDropTrace);
  m_mac->SetMacTxDropTrace    (m_macTxDropTrace);
  m_mac->SetMacTxTrace        (m_macTxTrace);
}

Ptr<DamaMac>
DamaNetDevice::GetMac (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_mac;
}

void
DamaNetDevice::SetController (Ptr<DamaController> controller)
{
  NS_LOG_FUNCTION (this << controller);
  m_controller = controller;

  m_controller->Start (GetNode ()->GetId ());
}

Ptr<DamaController>
DamaNetDevice::GetController (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_controller;
}

Ptr<Node>
DamaNetDevice::GetNode (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_node;
}

void
DamaNetDevice::SetNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION (this << node);
  m_node = node;
}

void
DamaNetDevice::SetChannel (Ptr<DamaChannel> channel)
{
  NS_LOG_FUNCTION (this << channel);

  if (channel != 0)
    {
      m_channel = channel;
    }
}

Ptr<Channel>
DamaNetDevice::GetChannel (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_channel;
}

void
DamaNetDevice::SetIfIndex (const uint32_t index)
{
  m_ifIndex = index;
}

uint32_t
DamaNetDevice::GetIfIndex (void) const
{
  return m_ifIndex;
}

void
DamaNetDevice::SetAddress (Address address)
{
  NS_ASSERT(m_mac != 0);
  m_mac->SetAddress (Mac48Address::ConvertFrom (address));
}

Address
DamaNetDevice::GetAddress (void) const
{
  NS_ASSERT(m_mac != 0);
  return m_mac->GetAddress ();
}

bool
DamaNetDevice::SetMtu (const uint16_t mtu)
{
  if (mtu > MAX_MSDU_SIZE - LLC_SNAP_HEADER_LENGTH)
    {
      return false;
    }

  m_mtu = mtu;
  return true;
}

uint16_t
DamaNetDevice::GetMtu (void) const
{
  return m_mtu;
}

bool
DamaNetDevice::IsLinkUp (void) const
{
  return m_linkUp;
}

void
DamaNetDevice::AddLinkChangeCallback (Callback<void> callback)
{
  m_linkChanges.ConnectWithoutContext (callback);
}

bool
DamaNetDevice::IsBroadcast (void) const
{
  return true;
}

Address
DamaNetDevice::GetBroadcast (void) const
{
  return Mac48Address::GetBroadcast ();
}

bool
DamaNetDevice::IsMulticast (void) const
{
  return true;
}

Address
DamaNetDevice::GetMulticast (Ipv4Address multicastGroup) const
{
  return Mac48Address::GetMulticast (multicastGroup);
}

Address DamaNetDevice::GetMulticast (Ipv6Address addr) const
{
  return Mac48Address::GetMulticast (addr);
}

bool
DamaNetDevice::IsPointToPoint (void) const
{
  return false;
}

bool
DamaNetDevice::IsBridge (void) const
{
  return false;
}

bool
DamaNetDevice::Send (Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (*packet << " Dest:" << dest << " ProtocolNo:" << protocolNumber);
  NS_ASSERT (Mac48Address::IsMatchingType (dest));

  Mac48Address realTo = Mac48Address::ConvertFrom (dest);
  Mac48Address realFrom = Mac48Address::ConvertFrom (GetAddress ());

  LlcSnapHeader llc;
  llc.SetType (protocolNumber);

  packet->AddHeader (llc);

  return m_mac->Enqueue (packet, realTo, realFrom);
}

bool
DamaNetDevice::NeedsArp (void) const
{
  return true;
}

void
DamaNetDevice::SetReceiveCallback (NetDevice::ReceiveCallback cb)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_forwardUp = cb;
}

void
DamaNetDevice::ForwardUp (Ptr<Packet> packet, Mac48Address from, Mac48Address to)
{
  NS_LOG_FUNCTION (*packet << from << to);

  enum NetDevice::PacketType type;

  LlcSnapHeader llc;
  packet->RemoveHeader (llc);

  if (to.IsBroadcast ())
    {
      NS_LOG_DEBUG ("NetDevice::PACKET_BROADCAST");
      type = NetDevice::PACKET_BROADCAST;
    }
  else if (to.IsGroup ())
    {
      NS_LOG_DEBUG ("NetDevice::PACKET_MULTICAST");
      type = NetDevice::PACKET_MULTICAST;
    }
  else if (to == m_mac->GetAddress ())
    {
      NS_LOG_DEBUG ("NetDevice::PACKET_HOST");
      type = NetDevice::PACKET_HOST;
    }
  else
    {
      NS_LOG_DEBUG ("NetDevice::PACKET_OTHERHOST");
      type = NetDevice::PACKET_OTHERHOST;
    }

  if (type != NetDevice::PACKET_OTHERHOST)
    {
      m_controller->NotifyRx ();
      m_macRxTrace (packet);
      m_snifferTrace(packet);
      m_promiscSnifferTrace(packet);
      m_forwardUp (this, packet, llc.GetType (), from);
    }

  if (!m_promiscRx.IsNull ())
    {
      m_controller->NotifyPromiscRx ();
      m_macPromiscRxTrace (packet);
      m_promiscSnifferTrace (packet);
      m_promiscRx (this, packet, llc.GetType (), from, to, type);
    }
}

void
DamaNetDevice::LinkUp (void)
{
  m_linkUp = true;
  m_linkChanges ();
}

void
DamaNetDevice::LinkDown (void)
{
  m_linkUp = false;
  m_linkChanges ();
}

bool
DamaNetDevice::SendFrom (Ptr<Packet> packet, const Address& source,
                         const Address& dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (Mac48Address::IsMatchingType (dest));
  NS_ASSERT (Mac48Address::IsMatchingType (source));

  Mac48Address realTo = Mac48Address::ConvertFrom (dest);
  Mac48Address realFrom = Mac48Address::ConvertFrom (source);

  LlcSnapHeader llc;
  llc.SetType (protocolNumber);

  packet->AddHeader (llc);
  return m_mac->Enqueue (packet, realTo, realFrom);
}

void
DamaNetDevice::SetPromiscReceiveCallback (PromiscReceiveCallback cb)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_promiscRx = cb;
}

bool
DamaNetDevice::SupportsSendFrom (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_mac->SupportsSendFrom ();
}

} // namespace ns3

