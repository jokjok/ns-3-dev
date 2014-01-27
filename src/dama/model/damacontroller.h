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
#ifndef DAMACONTROLLER_H
#define DAMACONTROLLER_H

#include "ns3/object.h"
#include "damachannel.h"

namespace ns3 {

/**
 * \brief A controller for a DamaNetDevice
 *
 * The controller, which could be a distribuited (where each receiver acts for
 * himself) or a centralized one (there is a special device which coordinates
 * all others) is responsible to correctly follow a protocol for the communication
 * over the shared channel. The protocol should define when a device could
 * transmit, and what are the action taken in case of collision (if channel can
 * lead to collisions) or when packet are received.
 *
 * The controller starts with a call (from DamaNetDevice) to the Start method.
 * Subclasses should be aware that it is called only one time, and so they
 * need to schedule their survival cycle or wait a call to a Notify* method.
 *
 * \see Start
 * \see NotifyCollision
 * \see NotifyRx
 */
class DamaController : public Object
{
public:
    static TypeId GetTypeId (void);

    /**
     * \brief Construct a new DamaController
     *
     * This is the default constructor
     */
    DamaController();

    /**
     * \brief Set the DamaMac for this controller
     *
     * \param c DamaMac to set
     */
    void AddMac (Ptr<DamaMac> m);

    /**
     * \brief Start the controller
     *
     * This method is called only one time by a DamaNetDevice.
     */
    virtual void Start (uint32_t nodeId) = 0;

    /**
     * \brief Someone is notifying the controlled that a collision occourred
     *
     * Often called by a DamaMac implementation.
     */
    virtual void NotifyCollision (void) = 0;

    /**
     * \brief Someone is notifying the controlled that a (promisc) packet is received
     *
     * Often called by a DamaMac implementation
     */
    virtual void NotifyPromiscRx (void) = 0;

    /**
     * \brief Someone is notifying the controlled that a packet is received
     *
     * Often called by a DamaMac implementation
     */
    virtual void NotifyRx (void) = 0;

protected:
    DamaMacList m_mac;
};

} // namespace ns3

#endif // DAMACONTROLLER_H
