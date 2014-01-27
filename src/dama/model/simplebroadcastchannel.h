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
#ifndef SIMPLEBROADCASTCHANNEL_H
#define SIMPLEBROADCASTCHANNEL_H

#include "damachannel.h"

namespace ns3 {

class SimpleBroadcastChannel : public DamaChannel
{
public:
    static TypeId GetTypeId (void);

    SimpleBroadcastChannel();

    virtual void Send (Ptr<const Packet> p, Ptr<DamaMac> sender);

    void SetPropagationTimeMs(uint32_t p);

protected:
    void FreeChannel ();
    void NotifyTrasmitToOtherNodes (Ptr<const Packet> p);

private:
    bool m_isInUse;
    Time m_propagationTime;
    Ptr<DamaMac> m_activeSender;

    std::vector<EventId> m_events;
};

}

#endif // SIMPLEBROADCASTCHANNEL_H
