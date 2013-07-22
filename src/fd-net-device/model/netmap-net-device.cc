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

#include "ns3/llc-snap-header.h"
#include "ns3/ethernet-trailer.h"
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
  m_freeBufferInRCallback = false;
}

NetmapNetDevice::NetmapNetDevice (NetmapNetDevice const &)
{
}

NetmapNetDevice::~NetmapNetDevice ()
{
  delete m_d;
}

void
NetmapNetDevice::Start (Time tStart)
{
  NS_LOG_FUNCTION (tStart);
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

  if (!m_d->IsSystemNetmapCapable ())
    {
      NS_FATAL_ERROR ("Failure, system isn't netmap-capable");
      return;
    }

  if (m_ifName.empty ())
    {
      NS_FATAL_ERROR ("Invalid ifname");
    }
  else
    {
      if (m_d->IsDeviceNetmapCapable (m_ifName))
        {
          m_d->SetIfName (m_ifName);
          m_d->OpenFd ();
          if (m_d->StartNmMode ())
            m_linkUp = true;
          else
            {
              NS_LOG_WARN("Can't open NM mode on device.");
              return;
            }

          // Register first ring. Write the possibility to use more rings
          m_d->RegisterHwRingId (0);
        }
      else
        {
          NS_FATAL_ERROR("Failure, device isn't netmap compatible");
        }
    }
}

void
NetmapNetDevice::StopDevice (void)
{
  NS_LOG_FUNCTION (this);

  m_d->StopNmMode ();
  m_d->CloseFd ();
}

void
NetmapNetDevice::SetIfName (const std::string& ifName)
{
  m_ifName = ifName;
  StartDevice ();
}

std::string
NetmapNetDevice::GetIfName (void) const
{
  return m_ifName;
}

bool
NetmapNetDevice::Send (Ptr<Packet> packet, const Address& destination,
                      uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << destination << protocolNumber);
  return SendFrom (packet, GetAddress (), destination, protocolNumber);
}

uint32_t
NetmapNetDevice::SendMany (Ptr<PacketBurst> packets, const Address& dest,
                          uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packets << dest << protocolNumber);
  return SendManyFrom (packets, GetAddress (), dest, protocolNumber);
}

bool
NetmapNetDevice::SendFrom (Ptr<Packet> packet, const Address& src,
                          const Address& dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << dest << src << protocolNumber);

  PacketBurst b;
  b.SetPacketsOwner (PacketBurst::USER);
  b.AddPacket (packet);

  if (SendManyFrom (Ptr<PacketBurst> (&b), src, dest, protocolNumber) == 1)
    return true;

  return false;
}

void
NetmapNetDevice::DropTrace (Ptr<PacketBurst> packets)
{
  std::list< Ptr<Packet> >::const_iterator iterator;

  for (iterator = packets->Begin (); iterator != packets->End (); ++iterator)
    {
      Ptr<Packet> pkt = *iterator;
      m_macTxDropTrace (pkt);
    }
}

void
NetmapNetDevice::Trace (Ptr<PacketBurst> packets)
{
  std::list< Ptr<Packet> >::const_iterator iterator;

  for (iterator = packets->Begin (); iterator != packets->End (); ++iterator)
    {
      Ptr<Packet> pkt = *iterator;

      m_macTxTrace (pkt);
      m_promiscSnifferTrace (pkt);
      m_snifferTrace (pkt);
      NS_LOG_LOGIC(pkt);
    }
}

uint32_t
NetmapNetDevice::SendManyFrom (Ptr<PacketBurst> packets, const Address& src,
                              const Address& dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << src << dest << protocolNumber);
  std::list< Ptr<Packet> >::const_iterator iterator;
  uint32_t sent = 0;

  if (IsLinkUp () == false)
    {
      DropTrace (packets);
      NS_LOG_LOGIC ("Link down");
      return false;
    }

  NS_LOG_LOGIC ("Transmit packets from " << src);
  NS_LOG_LOGIC ("Transmit packets to " << dest);

  for (iterator = packets->Begin (); iterator != packets->End (); ++iterator)
    {
      Ptr<Packet> pkt = *iterator;
      AddHeader (pkt, src, dest, protocolNumber);
    }

  Trace (packets);

  NS_LOG_LOGIC ("Calling Netmap API for these packets");

  sent = m_d->Send (packets);

  iterator = packets->Begin ();

  for (uint32_t i = 0; i < sent; ++i)
    {
      Ptr<Packet> pkt = *iterator;

      m_macTxTrace (pkt);
      m_promiscSnifferTrace (pkt);
      m_snifferTrace (pkt);

      ++iterator;
    }

  for (; iterator != packets->End (); ++iterator)
    {
      Ptr<Packet> pkt = *iterator;
      m_macTxDropTrace (pkt);

      ++iterator;
    }

  return sent;
}

void
NetmapNetDevice::AddHeader (Ptr<Packet> p,   const Address &src,
                            const Address &dest,  uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (p << src << dest << protocolNumber);

  Mac48Address destination = Mac48Address::ConvertFrom (dest);
  Mac48Address source = Mac48Address::ConvertFrom (src);

  EthernetHeader header (false);
  header.SetSource (source);
  header.SetDestination (destination);

  EthernetTrailer trailer;

  NS_LOG_LOGIC ("p->GetSize () = " << p->GetSize ());
  NS_LOG_LOGIC ("m_encapMode = " << m_encapMode);
  NS_LOG_LOGIC ("m_mtu = " << m_mtu);

  uint16_t lengthType = 0;
  switch (m_encapMode)
    {
    case DIX:
      NS_LOG_LOGIC ("Encapsulating packet as DIX (type interpretation)");
      //
      // This corresponds to the type interpretation of the lengthType field as
      // in the old Ethernet Blue Book.
      //
      lengthType = protocolNumber;

      //
      // All Ethernet frames must carry a minimum payload of 46 bytes.  We need
      // to pad out if we don't have enough bytes.  These must be real bytes
      // since they will be written to pcap files and compared in regression
      // trace files.
      //
      if (p->GetSize () < 46)
        {
          uint8_t buffer[46];
          memset (buffer, 0, 46);
          Ptr<Packet> padd = Create<Packet> (buffer, 46 - p->GetSize ());
          p->AddAtEnd (padd);
        }
      break;
    case LLC:
      {
        NS_LOG_LOGIC ("Encapsulating packet as LLC (length interpretation)");

        LlcSnapHeader llc;
        llc.SetType (protocolNumber);
        p->AddHeader (llc);

        //
        // This corresponds to the length interpretation of the lengthType
        // field but with an LLC/SNAP header added to the payload as in
        // IEEE 802.2
        //
        lengthType = p->GetSize ();

        //
        // All Ethernet frames must carry a minimum payload of 46 bytes.  The
        // LLC SNAP header counts as part of this payload.  We need to padd out
        // if we don't have enough bytes.  These must be real bytes since they
        // will be written to pcap files and compared in regression trace files.
        //
        if (p->GetSize () < 46)
          {
            uint8_t buffer[46];
            memset (buffer, 0, 46);
            Ptr<Packet> padd = Create<Packet> (buffer, 46 - p->GetSize ());
            p->AddAtEnd (padd);
          }

        NS_ASSERT_MSG (p->GetSize () <= GetMtu (),
                       "CsmaNetDevice::AddHeader(): 802.3 Length/Type field with LLC/SNAP: "
                       "length interpretation must not exceed device frame size minus overhead");
      }
      break;
    default:
      NS_FATAL_ERROR ("NetmapNetDevice::AddHeader(): Unknown packet encapsulation mode");
      break;
    }

  NS_LOG_LOGIC ("header.SetLengthType (" << lengthType << ")");
  header.SetLengthType (lengthType);
  p->AddHeader (header);

  if (Node::ChecksumEnabled ())
    {
      trailer.EnableFcs (true);
    }
  trailer.CalcFcs (p);
  p->AddTrailer (trailer);
}

} // namespace ns3

