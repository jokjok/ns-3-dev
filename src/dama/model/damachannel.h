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
#ifndef DAMACHANNEL_H
#define DAMACHANNEL_H

#include "ns3/channel.h"
#include "damamac.h"

#include <list>

namespace ns3 {

class Packet;

/**
 * \brief Typedef for the list of a DamaMacs sharing this channel
 */
typedef std::list< Ptr<DamaMac> > DamaMacList;

/**
 * \ingroup dama
 * \brief Class for a Channel used by DamaMac to transmit and receive packets
 *
 * The Mac layer will call the Send method to transmit a packet. Subclasses
 * should take care about delivering of such packets to right Mac listeners.
 */
class DamaChannel : public Channel
{
public:
    static TypeId GetTypeId (void);

    /**
     * \brief Construct a new DamaChannel
     *
     * This is the default constructor
     */
    DamaChannel();

    /**
     * \brief Send a packet over the channel
     *
     * Conceptually, it could be done by scheduling in the right context
     * a DamaMac::Receive call.
     *
     * \param p Packet to send
     * \param sender The sender MAC
     */
    virtual void Send (Ptr<const Packet> p, Ptr<DamaMac> sender) = 0;

    /**
     * \brief Add a device to the channel
     *
     * \param device Device to add
     */
    void Add (Ptr<DamaMac> mac);

    // inherited from ns3::Channel
    virtual uint32_t GetNDevices (void) const;
    virtual Ptr<NetDevice> GetDevice (uint32_t i) const;

protected:
     DamaMacList m_damaMacList;
};

} // namespace ns3

#endif // DAMACHANNEL_H
