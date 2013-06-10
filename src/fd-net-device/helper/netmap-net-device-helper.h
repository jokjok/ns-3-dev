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
#ifndef NETMAPNETDEVICEHELPER_H
#define NETMAPNETDEVICEHELPER_H

#include "fd-net-device-helper.h"

namespace ns3 {

class NetmapNetDeviceHelper : public FdNetDeviceHelper
{
public:
  NetmapNetDeviceHelper();

protected:
  /*
   * \internal
   */
  virtual Ptr<NetDevice> InstallPriv (Ptr<Node> node) const;
};

}
#endif // NETMAPNETDEVICEHELPER_H
