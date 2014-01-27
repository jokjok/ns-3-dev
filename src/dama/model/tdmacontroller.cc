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
#include "tdmacontroller.h"

#include "ns3/log.h"
#include "ns3/uinteger.h"

NS_LOG_COMPONENT_DEFINE ("TdmaController");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TdmaController);

TypeId
TdmaController::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TdmaController")
    .SetParent<DamaController> ()
    .AddAttribute ("DataRate",
                   "The default data rate for point to point links",
                   DataRateValue (GetDefaultDataRate ()),
                   MakeDataRateAccessor (&TdmaController::SetDataRate,
                                         &TdmaController::GetDataRate),
                   MakeDataRateChecker ())
     .AddAttribute ("SlotTime", "The duration of a Slot in microseconds.",
                    TimeValue (GetDefaultSlotTime ()),
                    MakeTimeAccessor (&TdmaController::SetSlotTime,
                                      &TdmaController::GetSlotTime),
                    MakeTimeChecker ())
     .AddAttribute ("GaurdTime", "GaurdTime between TDMA slots in microseconds.",
                    TimeValue (GetDefaultGaurdTime ()),
                    MakeTimeAccessor (&TdmaController::SetGaurdTime,
                                      &TdmaController::GetGaurdTime),
                    MakeTimeChecker ())
     .AddAttribute ("InterFrameTime", "The wait time between consecutive tdma frames.",
                    TimeValue (MicroSeconds (0)),
                    MakeTimeAccessor (&TdmaController::SetInterFrameTimeInterval,
                                      &TdmaController::GetInterFrameTimeInterval),
                    MakeTimeChecker ())
     .AddAttribute ("SlotNumber","Number of slot in a TDMA frame",
                    UintegerValue (4),
                    MakeUintegerAccessor (&TdmaController::m_slotNumber),
                    MakeUintegerChecker<uint32_t> (1))
    ;
  return tid;
}

Time
TdmaController::GetDefaultSlotTime (void)
{
  return MilliSeconds (500);
}

Time
TdmaController::GetDefaultGaurdTime (void)
{
  return MicroSeconds (100);
}

DataRate
TdmaController::GetDefaultDataRate (void)
{
  NS_LOG_DEBUG ("Setting default");
  return DataRate ("11000000b/s");
}

TdmaController::TdmaController() :
    DamaController(),
    m_isActive(false)
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
TdmaController::SetSlotTime (Time slotTime)
{
  NS_LOG_FUNCTION (this << slotTime);
  m_slotTime = slotTime.GetMicroSeconds ();
}

Time
TdmaController::GetSlotTime (void) const
{
  return MicroSeconds (m_slotTime);
}

void
TdmaController::SetDataRate (DataRate bps)
{
  NS_LOG_FUNCTION (this << bps);
  m_bps = bps;
}

DataRate
TdmaController::GetDataRate (void) const
{
  return m_bps;
}

void
TdmaController::SetGaurdTime (Time gaurdTime)
{
  NS_LOG_FUNCTION (this << gaurdTime);
  m_gaurdTime = gaurdTime.GetMicroSeconds ();
}

Time
TdmaController::GetGaurdTime (void) const
{
  return MicroSeconds (m_gaurdTime);
}

void
TdmaController::SetInterFrameTimeInterval (Time interFrameTime)
{
  NS_LOG_FUNCTION (interFrameTime);
  m_interFrameTime = interFrameTime.GetMicroSeconds ();
}

Time
TdmaController::GetInterFrameTimeInterval (void) const
{
  return MicroSeconds (m_interFrameTime);
}

void
TdmaController::Start(uint32_t nodeId)
{
  (void) nodeId;
  NS_LOG_FUNCTION (this);
  if (!m_isActive)
    {
      m_isActive = true;
      Simulator::Schedule (NanoSeconds (10), &TdmaController::ScheduleTdmaSessions, this);
    }
}

} // namespace ns3
