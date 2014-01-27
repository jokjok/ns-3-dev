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
#include "damamac.h"
#include "damanetdevice.h"
#include "damachannel.h"

#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("DamaMac");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (DamaMac);

TypeId
DamaMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DamaMac")
    .SetParent<Object> ()
    ;
  return tid;
}

DamaMac::DamaMac()
{
}

void
DamaMac::SetDevice (Ptr<DamaNetDevice> device)
{
  NS_LOG_FUNCTION (this);
  m_device = device;
}

Ptr<DamaNetDevice>
DamaMac::GetDevice () const
{
  return m_device;
}

void
DamaMac::SetForwardUpCallback (Callback<void, Ptr<Packet>, Mac48Address, Mac48Address> upCallback)
{
  NS_LOG_FUNCTION (this);
  m_upCallback = upCallback;
}

void
DamaMac::SetLinkUpCallback (Callback<void> linkUp)
{
  NS_LOG_FUNCTION (this);
  linkUp ();
}

void
DamaMac::SetLinkDownCallback (Callback<void> linkDown)
{
  NS_LOG_FUNCTION (this);
  (void) linkDown;
}

void
DamaMac::SetAddress (Mac48Address address)
{
  NS_LOG_FUNCTION (this);
  m_address = address;
}

Mac48Address
DamaMac::GetAddress (void) const
{
  NS_LOG_FUNCTION (this);
  return m_address;
}

void
DamaMac::SetMacTxTrace (TracedCallback<Ptr<const Packet> > cb)
{
  NS_LOG_FUNCTION (this);
  m_macTxTrace = cb;
}

void
DamaMac::SetMacTxDropTrace (TracedCallback<Ptr<const Packet> > cb)
{
  NS_LOG_FUNCTION (this);
  m_macTxDropTrace = cb;
}

void
DamaMac::SetPromiscSnifferTrace (TracedCallback<Ptr<const Packet> > cb)
{
  NS_LOG_FUNCTION (this);
  m_promiscSnifferTrace = cb;
}

void
DamaMac::SetSnifferTrace (TracedCallback<Ptr<const Packet> > cb)
{
  NS_LOG_FUNCTION (this);
  m_snifferTrace = cb;
}

void
DamaMac::SetMacRxDropTrace (TracedCallback<Ptr<const Packet> > cb)
{
  NS_LOG_FUNCTION (this);
  m_macRxDropTrace = cb;
}


} // namespace ns3
