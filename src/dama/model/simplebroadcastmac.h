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
#ifndef SIMPLEBROADCASTMAC_H
#define SIMPLEBROADCASTMAC_H

#include "damamac.h"
#include "ns3/header.h"

namespace ns3 {

class SimpleBroadcastMac : public DamaMac
{
public:
    static TypeId GetTypeId (void);

    SimpleBroadcastMac();

    virtual bool Enqueue (Ptr<const Packet> packet, Mac48Address to, Mac48Address from);
    virtual bool Enqueue (Ptr<const Packet> packet, Mac48Address to);

    virtual bool SupportsSendFrom (void) const;

    virtual bool SendQueueHead ();

    virtual bool HasDataToSend () const;

    virtual void Receive (Ptr<const Packet> packet);

private:
    struct Item;

    typedef std::list<struct Item> PacketQueue;
    typedef std::list<struct Item>::reverse_iterator PacketQueueRI;
    typedef std::list<struct Item>::iterator PacketQueueI;

    struct Item
    {
      Item (Ptr<const Packet> packet,
            Mac48Address to,
            Time tstamp);
      Ptr<const Packet> packet;
      Mac48Address to;
      Time tstamp;
    };

    PacketQueue m_queue;
    std::size_t m_maxSize;

};

} // namespace ns3

#endif // SIMPLEBROADCASTMAC_H
