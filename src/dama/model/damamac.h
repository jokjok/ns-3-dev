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
#ifndef DAMAMAC_H
#define DAMAMAC_H

#include "ns3/core-module.h"
#include "ns3/traced-callback.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"
#include "ns3/mac48-address.h"

#include "damanetdevice.h"
#include "damachannel.h"

namespace ns3 {

/**
 * \brief A Mac layer for a DamaNetDevice
 *
 * The Mac layer is responsible to mantain a queue of packets, to send over
 * the channel when the controller allow to do so. A DamaNetDevice will call
 * the Enqueue method, and then a controller will call the SendQueueHead method.
 *
 * A subclass must provide a way to enqueue packets and send them over a channel,
 * and moreover it should be able to receive packets, implementing the Receive
 * method, which will be called by a subclass of a DamaChannel.
 */
class DamaMac : public Object
{
public:
    static TypeId GetTypeId (void);

    /**
     * \brief Construct a new DamaMac
     *
     * This is the default constructor
     */
    DamaMac();

    virtual void SetDevice (Ptr<DamaNetDevice> device);
    Ptr<DamaNetDevice> GetDevice () const;

    /**
     * \param upCallback the callback to invoke when a packet must be forwarded up the stack.
     */
    void SetForwardUpCallback (Callback<void, Ptr<Packet>, Mac48Address, Mac48Address> upCallback);

    /**
     * \param linkUp the callback to invoke when the link becomes up.
     */
    void SetLinkUpCallback (Callback<void> linkUp);

    /**
     * \param linkDown the callback to invoke when the link becomes down.
     */
    void SetLinkDownCallback (Callback<void> linkDown);

    void SetMacTxTrace (TracedCallback<Ptr<const Packet> > cb);
    void SetMacTxDropTrace (TracedCallback<Ptr<const Packet> > cb);
    void SetMacRxDropTrace (TracedCallback<Ptr<const Packet> > cb);
    void SetPromiscSnifferTrace (TracedCallback<Ptr<const Packet> > cb);
    void SetSnifferTrace (TracedCallback<Ptr<const Packet> > cb);

    void SetAddress (Mac48Address address);

    Mac48Address GetAddress (void) const;

    /**
     * \param packet the packet to send.
     * \param to the address to which the packet should be sent.
     * \param from the address from which the packet should be sent.
     *
     * The packet should be enqueued in a tx queue, and should be
     * dequeued as soon as the DCF function determines that
     * access it granted to this MAC.  The extra parameter "from" allows
     * this device to operate in a bridged mode, forwarding received
     * frames without altering the source address.
     */
    virtual bool Enqueue (Ptr<const Packet> packet, Mac48Address to,
                          Mac48Address from) = 0;

    virtual bool Enqueue (Ptr<const Packet> packet, Mac48Address to) = 0;

    virtual bool SupportsSendFrom (void) const = 0;

    virtual bool HasDataToSend () const = 0;

    virtual bool SendQueueHead () = 0;

    virtual void Receive (Ptr<const Packet> packet) = 0;

protected:
    Callback<void, Ptr<Packet>,Mac48Address, Mac48Address> m_upCallback;
    TracedCallback<Ptr<const Packet> > m_macTxTrace;
    TracedCallback<Ptr<const Packet> > m_macTxDropTrace;
    TracedCallback<Ptr<const Packet> > m_macRxDropTrace;
    TracedCallback<Ptr<const Packet> > m_snifferTrace;
    TracedCallback<Ptr<const Packet> > m_promiscSnifferTrace;

    Ptr<DamaNetDevice> m_device;
    Mac48Address m_address;
};

} // namespace ns3

#endif // DAMAMAC_H
