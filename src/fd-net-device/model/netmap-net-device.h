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
 * This section documents the API of the ns-3 netmap-net-device module.
 * For a generic functional description, please refer to the ns-3 manual.
 */

/**
 * \ingroup netmap-net-device
 *
 * \brief a NetDevice to read/write network traffic from/into real ethernet device, using
 *        the netmap API.
 *
 * TODO
 */
class NetmapNetDevice : public FdNetDevice
{
public:

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
   * @return Interface name opened in netmap mode
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
  virtual uint32_t SendMany     (Ptr<PacketBurst> packets, const Address& dest,
                              uint16_t protocolNumber);
  virtual uint32_t SendManyFrom (Ptr<PacketBurst> packets, const Address& source,
                               const Address& dest, uint16_t protocolNumber);

protected:
  void AddHeader (Ptr<Packet> p,   const Address &source,
                 const Address &dest,  uint16_t protocolNumber);


private:
  // private copy constructor as suggested in:
  // http://www.nsnam.org/wiki/index.php/NS-3_Python_Bindings#.22invalid_use_of_incomplete_type.22
  NetmapNetDevice (NetmapNetDevice const &);

  void DropTrace(Ptr<PacketBurst> packets);
  void Trace(Ptr<PacketBurst> packets);

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

  std::string m_ifName;

  NetmapPrivImpl *m_d;

};

}

#endif /* NETMAP_NET_DEVICE_H */

