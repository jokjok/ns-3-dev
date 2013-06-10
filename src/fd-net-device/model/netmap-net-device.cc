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
#include "netmap-net-device.h"
#include "netmap-priv-impl.h"

#include "ns3/simulator.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("NetmapNetDevice");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (NetmapNetDevice);

TypeId
NetmapNetDevice::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NetmapNetDevice")
    .SetParent<FdNetDevice> ()
    .AddConstructor<NetmapNetDevice> ();

  return tid;
}

NetmapNetDevice::NetmapNetDevice ()
  : FdNetDevice (),
    m_ifName (""),
    m_d (new NetmapPrivImpl (this))
{
}

NetmapNetDevice::NetmapNetDevice (NetmapNetDevice const &)
{
}

NetmapNetDevice::~NetmapNetDevice ()
{
}

void
NetmapNetDevice::Start (Time tStart)
{
  NS_LOG_FUNCTION (tStart);
  std::cout << "START";
  Simulator::Cancel (m_startEvent);
  m_startEvent = Simulator::Schedule (tStart, &NetmapNetDevice::StartDevice, this);
}

void
NetmapNetDevice::Stop (Time tStop)
{
  NS_LOG_FUNCTION (tStop);
  Simulator::Cancel (m_stopEvent);
  m_startEvent = Simulator::Schedule (tStop, &NetmapNetDevice::StopDevice, this);
}

void
NetmapNetDevice::StartDevice (void)
{
  NS_LOG_FUNCTION (this);
  std::cout << "START";

  if (m_ifName.empty())
    {
      NS_LOG_DEBUG("Failure, invalid ifname.");
    }
  else
    {
      if (m_d->IsDeviceNetmapCapable(m_ifName))
        {
          m_d->SetIfName(m_ifName);
          m_d->OpenFd();
          if (m_d->StartNmMode())
            m_linkUp = true;
          else
            return;

          // Register first ring. Write the possibility to use more rings
          m_d->RegisterHwRingId(0);
        }
      else
        {
          NS_LOG_DEBUG("Failure, device isn't netmap compatible");
        }
    }
}

void
NetmapNetDevice::StopDevice (void)
{
  NS_LOG_FUNCTION (this);

  m_d->StopNmMode();
  m_d->CloseFd();
}

void
NetmapNetDevice::SetIfName (const std::string& ifName)
{
  m_ifName = ifName;
}

std::string
NetmapNetDevice::GetIfName( void) const
{
  return m_ifName;
}

bool
NetmapNetDevice::Send (Ptr<Packet> packet, const Address& destination,
                      uint16_t protocolNumber)
{
  std::cout << "SenD";
  NS_LOG_FUNCTION (this << packet << destination << protocolNumber);
  return SendFrom (packet, GetAddress(), destination, protocolNumber);
}

bool
NetmapNetDevice::SendMany (Ptr<PacketBurst> packets, const Address& dest,
                          uint16_t protocolNumber)
{
  std::cout << "SenDManya";
  return SendManyFrom (packets, GetAddress(), dest, protocolNumber);
}

bool
NetmapNetDevice::SendFrom (Ptr<Packet> packet, const Address& src,
                          const Address& dest, uint16_t protocolNumber)
{
  std::cout << "SenDFrom";
  NS_LOG_FUNCTION (this << packet << dest << src << protocolNumber);

  PacketBurst b;
  b.AddPacket (packet);

  return SendManyFrom (Ptr<PacketBurst> (&b), src, dest, protocolNumber);
}

bool
NetmapNetDevice::SendManyFrom (Ptr<PacketBurst> packets, const Address& src,
                              const Address& dest, uint16_t protocolNumber)
{
  std::cout << "SenDManyFrom";
  NS_LOG_FUNCTION (this << packets << src << dest << protocolNumber);

  if (IsLinkUp () == false)
    {
      std::list< Ptr<Packet> >::const_iterator iterator;

      for (iterator = packets->Begin(); iterator != packets->End(); ++iterator) {
        Ptr<Packet> pkt = *iterator;
        m_macTxDropTrace (pkt);
      }

      return false;
    }

  EthernetHeader header (false);
  Mac48Address destination = Mac48Address::ConvertFrom (dest);
  Mac48Address source = Mac48Address::ConvertFrom (src);

  NS_LOG_LOGIC ("Transmit packets from " << source);
  NS_LOG_LOGIC ("Transmit packets to " << destination);

  header.SetSource (source);
  header.SetDestination (destination);

  header.SetLengthType (protocolNumber);

  //
  // there's not much meaning associated with the different layers in this
  // device, so don't be surprised when they're all stacked together in
  // essentially one place.  We do this for trace consistency across devices.
  //
  //m_macTxTrace (packet);
  //m_promiscSnifferTrace (packet);
  //m_snifferTrace (packet);

  NS_LOG_LOGIC ("Calling Netmap API");

  if ( ! m_d->Send (packets, header) )
    {
      //m_macTxDropTrace (packet);
      return false;
    }

  return true;
}

}

