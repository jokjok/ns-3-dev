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
#include "damacontroller.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("DamaController");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (DamaController);

TypeId
DamaController::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DamaController")
    .SetParent<Object> ();
  return tid;
}

DamaController::DamaController()
{
  NS_LOG_FUNCTION (this);
}

void
DamaController::AddMac(Ptr<DamaMac> m)
{
  NS_LOG_FUNCTION(this);

  m_mac.push_front(m);
}

} // namespace ns-3
