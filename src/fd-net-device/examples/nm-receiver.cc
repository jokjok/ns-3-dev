/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 University of Modena and Reggio Emilia
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
 * Author: Natale Patriciello <natale.patriciello@gmail.com>
 *
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/netmap-net-device.h"
#include "ns3/netmap-net-device-helper.h"
#include "ns3/applications-module.h"
#include <unistd.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("NetmapNetDeviceReceiver");


bool printPacket(Ptr<NetDevice> net,Ptr<const Packet> pkt, uint16_t protocol,
                 const Address & addr)
{
  return true;
}

int
main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

  Ptr<NetmapNetDevice> d = Create<NetmapNetDevice>();
  d->SetIfName ("vale0:1");

  d->SetReceiveCallback(MakeCallback(&printPacket));

  usleep(10000000);
}

