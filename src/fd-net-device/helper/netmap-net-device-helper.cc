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
#include "netmap-net-device-helper.h"
#include "ns3/netmap-net-device.h"

#include "ns3/log.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("NetmapNetDeviceHelper");

NetmapNetDeviceHelper::NetmapNetDeviceHelper()
{
  m_deviceFactory.SetTypeId ("ns3::NetmapNetDevice");
}

Ptr<NetDevice>
NetmapNetDeviceHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<NetmapNetDevice> device = m_deviceFactory.Create<NetmapNetDevice> ();
  device->SetAddress (Mac48Address::Allocate ());
  node->AddDevice (device);
  return device;
}