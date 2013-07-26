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
#ifndef NETMAP_NET_DEVICE_H
#define NETMAP_NET_DEVICE_H

#include <string.h>

#include "fd-net-device.h"
#include "ns3/packet-burst.h"

namespace ns3 {

class NetmapPrivImpl;

/**
 * \defgroup netmap-net-device NetmapNetDevice
 *
 * This section documents the API and a provide a generic description of the
 * ns-3 netmap-net-device module on the point of view of a NS-3 user.
 *
 * To read a more detailed explanation on how is coded the Netmap backend,
 * please refer to the documentation of the NetmapPrivImpl class.
 */

/**
 * \ingroup netmap-net-device
 *
 * \brief a NetDevice to read/write network traffic from/into real ethernet devices, using
 *        the netmap API.
 *
 * A detailed description on how Netmap works, and what are its principles and its API could
 * be found at http://info.iet.unipi.it/~luigi/papers/20120503-netmap-atc12.pdf .
 *
 * \section Sender side
 *
 * To exploit Netmap strength, you can create a burst of packets, then send it
 * using SendMany and SendManyFrom.
 *
 * \section Receiver side
 *
 * Actually receiver side is constructed over the FdNetDevice ReceiveCallback, so the
 * private implementation will call, for each packet, the callback.
 */
class NetmapNetDevice : public FdNetDevice
{
public:
  friend class NetmapPrivImpl;

  static TypeId GetTypeId();

  /**
   * Constructor for the NetmapNetDevice.
   */
  NetmapNetDevice ();

  /**
   * Destructor for the NetmapNetDevice.
   */
  virtual ~NetmapNetDevice ();

  /**
   * Interface to open in netmap mode
   *
   * \param ifName Interface name
   */
  void SetIfName (const std::string& ifName);

  /**
   * Get the interface name associated to this NetmapNedDevice
   *
   * \return Interface name opened in netmap mode
   */
  std::string GetIfName (void) const;

  // From FdNetDevice
  virtual void Start (Time tStart);
  virtual void Stop (Time tStop);

  // From NetDevice
  virtual bool Send         (Ptr<Packet> packet, const Address& destination,
                             uint16_t protocolNumber);
  virtual bool SendFrom     (Ptr<Packet> packet, const Address& source,
                             const Address& dest, uint16_t protocolNumber);

  // PacketBurst version
  /** \brief Send burst of packets
   *
   * To exploit Netmap capabilities to send burst of packets, use this function.
   *
   * \param packets Burst of packets
   * \param dest Destination address
   * \param protocolNumber protocol
   */
  virtual uint32_t SendMany     (Ptr<PacketBurst> packets, const Address& dest,
                                 uint16_t protocolNumber);

  /** \brief Send burst of packets from a specified address
   *
   * To exploit Netmap capabilities to send burst of packets, use this function.
   *
   * \param packets Burst of packets
   * \param source Source address
   * \param dest Destination address
   * \param protocolNumber protocol
   */
  virtual uint32_t SendManyFrom (Ptr<PacketBurst> packets, const Address& source,
                                 const Address& dest, uint16_t protocolNumber);

protected:
  /** \internal
   *
   * Add a ethernet header to the packet
   *
   * \param p Packet
   * \param source Source address
   * \param dest Destination address
   * \param protocolNumber Protocol
   */
  void AddHeader (Ptr<Packet> p,        const Address &source,
                  const Address &dest,  uint16_t protocolNumber);


private:
  // private copy constructor as suggested in:
  // http://www.nsnam.org/wiki/index.php/NS-3_Python_Bindings#.22invalid_use_of_incomplete_type.22
  NetmapNetDevice (NetmapNetDevice const &);

  /**
   * \internal
   * Drop packets with a call to the drop trace function
   *
   * \param packets Packets to drop
   */
  void DropTrace (Ptr<PacketBurst> packets);

  /**
   * \internal
   * Trace packets with a call to the trace function
   *
   * \param packets Packets to trace
   */
  void Trace (Ptr<PacketBurst> packets);

  /**
   * \internal
   *
   * Spin up the device
   */
  void StartDevice (void);

  /**
   * \internal
   *
   * Tear down the device
   */
  void StopDevice (void);

  /**
   * \internal
   *
   * Name of the device
   */
  std::string m_ifName;

  /**
   * \internal
   *
   * Implementation private pointer
   */
  NetmapPrivImpl *m_d;

};

}

#endif /* NETMAP_NET_DEVICE_H */

