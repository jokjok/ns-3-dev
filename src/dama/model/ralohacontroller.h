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
#ifndef RALOHACONTROLLER_H
#define RALOHACONTROLLER_H

#include "tdmacontroller.h"
#include "ns3/random-variable-stream.h"
#include <vector>
#include <bitset>

namespace ns3 {

class RAlohaController : public TdmaController
{
public:
    static TypeId GetTypeId (void);

    RAlohaController();

    virtual void NotifyCollision (void);
    virtual void NotifyRx (void);
    virtual void NotifyPromiscRx (void);

protected:
    virtual void DoInitialize (void);

    virtual void ScheduleTdmaSessions (void);
    virtual void StartTransmission (void);

    bool IsCurrentSlotAllowed (void);
    bool IsCurrentSlotForbidden (void);
    void SetCurrentSlotAllowed (void);
    void SetCurrentSlotForbidden (void);
    void ResetCurrentSlotAllowed (void);
    void ResetCurrentSlotForbidden (void);

private:
    virtual void DoDispose (void);

    std::map<uint32_t, std::bitset<2> > m_slotStatus;
    uint32_t m_currentSlot;
    EventId m_nextScheduledTransmission;

    uint32_t m_waitingSlot;
};

}
#endif // RALOHACONTROLLER_H
