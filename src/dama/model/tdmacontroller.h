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
#ifndef TDMACONTROLLER_H
#define TDMACONTROLLER_H

#include "damacontroller.h"
#include "ns3/nstime.h"
#include "ns3/event-id.h"
#include "ns3/data-rate.h"
#include "ns3/timer.h"

namespace ns3 {

class TdmaController : public DamaController
{
public:
    static TypeId GetTypeId (void);

    TdmaController();

    void SetDataRate (DataRate bps);
    DataRate GetDataRate (void) const;

    void SetSlotTime (Time slotTime);
    Time GetSlotTime (void) const;

    void SetGaurdTime (Time gaurdTime);
    Time GetGaurdTime (void) const;

    void SetInterFrameTimeInterval (Time interFrameTime);
    Time GetInterFrameTimeInterval (void) const;

    virtual void Start (uint32_t nodeId);

    virtual void NotifyRx (void) = 0;
    virtual void NotifyPromiscRx (void) = 0;

protected:
    virtual void ScheduleTdmaSessions (void) = 0;
    void StartTransmission ();


    static Time GetDefaultSlotTime (void);
    static Time GetDefaultGaurdTime (void);
    static DataRate GetDefaultDataRate (void);

    DataRate m_bps;
    uint32_t m_slotTime;
    uint32_t m_gaurdTime;
    uint32_t m_interFrameTime;
    uint32_t m_slotNumber;

private:
    bool m_isActive;
};

} // namespace ns3

#endif // TDMACONTROLLER_H
