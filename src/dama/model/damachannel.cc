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
#include "damachannel.h"
#include "damanetdevice.h"
#include "damamac.h"

#include "ns3/log.h"
#include "ns3/packet.h"


NS_LOG_COMPONENT_DEFINE ("DamaChannel");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (DamaChannel);

TypeId
DamaChannel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DamaChannel")
    .SetParent<Object> ();
  return tid;
}

DamaChannel::DamaChannel()
{
}

void
DamaChannel::Add (Ptr<DamaMac> mac)
{
  NS_LOG_DEBUG (this << " " << mac);
  m_damaMacList.push_back (mac);
  NS_LOG_DEBUG ("current m_damaMacList size: " << m_damaMacList.size ());
}

uint32_t
DamaChannel::GetNDevices (void) const
{
  NS_LOG_FUNCTION (this);
  return m_damaMacList.size ();
}

Ptr<NetDevice>
DamaChannel::GetDevice (uint32_t i) const
{
  NS_LOG_FUNCTION (this << i);

  DamaMacList::const_iterator it = m_damaMacList.begin ();
  uint32_t j = 0;

  while (it != m_damaMacList.end ())
    {
      if (i == j)
        {
          return (*it)->GetDevice ();
        }
      ++j;
      ++it;
    }

  return 0;
}


} // namespace ns3
