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
#ifndef DAMANETDEVICE_H
#define DAMANETDEVICE_H

#include "ns3/network-module.h"

namespace ns3 {

class DamaController;
class DamaChannel;
class DamaMac;

/**
 * \defgroup dama DAMA Network Device
 *
 * Demand Assigned Multiple Access (DAMA) is a technology used to assign a
 * channel to clients that don't need to use it constantly. DAMA systems
 * assign communication channels based on requests issued from user terminals
 * to a central or a distribuited network control system. When the circuit
 * is no longer in use, the channels are then reassigned to other users.
 *
 * Channels are typically a pair of carrier frequencies (one for transmit
 * and one for receive), but can be other fixed bandwidth resources such
 * as timeslots in a TDMA burst plan or even physical party line channels.
 * Once a channel is allocated to a given pair of nodes, it is not available
 * to other users in the network until their session is finished.
 *
 * DAMA is related only to channel/resource allocation
 * and should not be confused with the Multiple access/multiplexing methods
 * (such as FDMA frequencies, TDMA slots, CDMA codes, or others) intended to
 * divide a single communication channel into multiple virtual channels.
 * These systems typically use resource allocation protocols that allow
 * a more rapid (although often less deterministic, consider CDMA collisions)
 * near-real-time allocation of bandwidth based on demand and data priority.
 * However, in sparsely allocated multiple-access channels, DAMA can be used
 * to allocate the individual virtual channel resources provided by
 * the multiple-access channel. This is most common in environments
 * that are sufficiently sparsely utilized that there is no need to add
 * complexity just to recover "conversation gap" idle periods.
 *
 * DAMA is widely used in satellite communications, especially in VSAT systems.
 * It is very effective in environments comprising multiple users each having
 * a low to moderate usage profile.
 *
 * ## Design rationale
 *
 * There are three macro concept here: a DamaNetDevice, a DamaMac and a
 * DamaController.
 *
 * A DamaNetDevice is responsible to provide to upper layers a compatible API
 * with NetDevice, to send and receive packets. To do this, it uses a DamaMac
 * instance, which talks to DamaMac installed in other nodes through a
 * DamaChannel.
 *
 * DamaMac and DamaChannel are strictly correlated, as a particular
 * DamaMac should know how to talk over a specified DamaChannel. For example,
 * we provide a SimpleBroadcastChannel and a SimpleBroadcastMac, which uses the
 * broadcast channel (that deliver frames to all nodes after a specified
 * propagation time) to communicate with other nodes.
 *
 * Transmission logic is done into a DamaController, which implements a network
 * controller, and so is responsible for managing transmission over the
 * shared channel. It could depend over predefined events, such as receiving a
 * packet, which are notified to the controller directly from the MAC layer.
 * An example implementation is for the Reservation Aloha distribuited protocol.
 *
 * Please refer to classes documentation if you want hints for subclassing them.
 */

/**
 * \ingroup dama
 * \class DamaNetDevice
 * \brief A Device for a DAMA Network Link.
 *
 * The class holds instances of ns3::DamaMac and a ns3::DamaController, and
 * works by delegating all physical operation to the Mac class, controlled and
 * managed by the Controller class: all the operations are done over a
 * ns3::DamaChannel.
 *
 * \see ns3::DamaChannel
 * \see ns3::DamaMac
 * \see ns3::DamaController
 */
class DamaNetDevice : public NetDevice
{
public:
  static TypeId GetTypeId (void);

  DamaNetDevice ();
  virtual ~DamaNetDevice ();

  /**
   * \brief Set a channel for this device
   *
   * \param channel The channel this device is attached to
   */
  void SetChannel (Ptr<DamaChannel> channel);

  /**
   * \brief Set a controller for this device
   *
   * \param controller The controller this device is attached to
   */
  void SetController (Ptr<DamaController> controller);

  /**
   * \brief Return the controller associated for this device
   *
   * \return Controller of this device
   */
  Ptr<DamaController> GetController() const;

  /**
   * \brief Set the Mac layer to use
   *
   * \param mac The mac layer to use.
   */
  void SetMac (Ptr<DamaMac> mac);

  /**
   * \returns the mac we are currently using.
   */
  Ptr<DamaMac> GetMac (void) const;

  // inherited from NetDevice base class.
  virtual void SetIfIndex (const uint32_t index);
  virtual uint32_t GetIfIndex (void) const;
  virtual Ptr<Channel> GetChannel (void) const;
  virtual void SetAddress (Address address);
  virtual Address GetAddress (void) const;
  virtual bool SetMtu (const uint16_t mtu);
  virtual uint16_t GetMtu (void) const;
  virtual bool IsLinkUp (void) const;
  virtual void AddLinkChangeCallback (Callback<void> callback);
  virtual bool IsBroadcast (void) const;
  virtual Address GetBroadcast (void) const;
  virtual bool IsMulticast (void) const;
  virtual Address GetMulticast (Ipv4Address multicastGroup) const;
  virtual bool IsPointToPoint (void) const;
  virtual bool IsBridge (void) const;
  virtual bool Send (Ptr<Packet> packet, const Address& dest,
                     uint16_t protocolNumber);
  virtual Ptr<Node> GetNode (void) const;
  virtual void SetNode (Ptr<Node> node);
  virtual bool NeedsArp (void) const;
  virtual void SetReceiveCallback (NetDevice::ReceiveCallback cb);

  virtual Address GetMulticast (Ipv6Address addr) const;

  virtual bool SendFrom (Ptr<Packet> packet, const Address& source,
                         const Address& dest, uint16_t protocolNumber);
  virtual void SetPromiscReceiveCallback (PromiscReceiveCallback cb);
  virtual bool SupportsSendFrom (void) const;

private:
  // This value conforms to the 802.11 specification
  static const uint16_t MAX_MSDU_SIZE = 2304;

  virtual void DoDispose (void);

  void ForwardUp (Ptr<Packet> packet, Mac48Address from, Mac48Address to);
  void LinkUp (void);
  void LinkDown (void);
  void Setup (void);

  Ptr<Node> m_node;
  Ptr<DamaController> m_controller;
  Ptr<DamaChannel> m_channel;
  Ptr<DamaMac> m_mac;

  NetDevice::ReceiveCallback m_forwardUp;
  NetDevice::PromiscReceiveCallback m_promiscRx;

  TracedCallback<Ptr<const Packet>, Mac48Address> m_rxLogger;
  TracedCallback<Ptr<const Packet>, Mac48Address> m_txLogger;

  uint32_t m_ifIndex;
  bool m_linkUp;
  TracedCallback<> m_linkChanges;
  mutable uint16_t m_mtu;

  /**
   * The trace source fired when packets come into the "top" of the device
   * at the L3/L2 transition, before being queued for transmission.
   *
   * \see class CallBackTraceSource
   */
  TracedCallback<Ptr<const Packet> > m_macTxTrace;

  /**
   * The trace source fired when packets coming into the "top" of the device
   * are dropped at the MAC layer during transmission.
   *
   * \see class CallBackTraceSource
   */
  TracedCallback<Ptr<const Packet> > m_macTxDropTrace;

  /**
   * The trace source fired for packets successfully received by the device
   * immediately before being forwarded up to higher layers (at the L2/L3
   * transition).  This is a promiscuous trace.
   *
   * \see class CallBackTraceSource
   */
  TracedCallback<Ptr<const Packet> > m_macPromiscRxTrace;

  /**
   * The trace source fired for packets successfully received by the device
   * immediately before being forwarded up to higher layers (at the L2/L3
   * transition).  This is a non- promiscuous trace.
   *
   * \see class CallBackTraceSource
   */
  TracedCallback<Ptr<const Packet> > m_macRxTrace;

  /**
   * The trace source fired when packets coming into the "top" of the device
   * are dropped at the MAC layer during reception.
   *
   * \see class CallBackTraceSource
   */
  TracedCallback<Ptr<const Packet> > m_macRxDropTrace;

  /**
   * The trace source fired when a packet begins the transmission process on
   * the medium.
   *
   * \see class CallBackTraceSource
   */
  TracedCallback<Ptr<const Packet> > m_phyTxBeginTrace;

  /**
   * The trace source fired when a packet ends the transmission process on
   * the medium.
   *
   * \see class CallBackTraceSource
   */
  TracedCallback<Ptr<const Packet> > m_phyTxEndTrace;

  /**
   * The trace source fired when the phy layer drops a packet as it tries
   * to transmit it.
   *
   * \see class CallBackTraceSource
   */
  TracedCallback<Ptr<const Packet> > m_phyTxDropTrace;

  /**
   * The trace source fired when a packet begins the reception process from
   * the medium.
   *
   * \see class CallBackTraceSource
   */
  TracedCallback<Ptr<const Packet> > m_phyRxBeginTrace;

  /**
   * The trace source fired when a packet ends the reception process from
   * the medium.
   *
   * \see class CallBackTraceSource
   */
  TracedCallback<Ptr<const Packet> > m_phyRxEndTrace;

  /**
   * The trace source fired when the phy layer drops a packet it has received.
   *
   * \see class CallBackTraceSource
   */
  TracedCallback<Ptr<const Packet> > m_phyRxDropTrace;

  /**
   * A trace source that emulates a non-promiscuous protocol sniffer connected
   * to the device.  Unlike your average everyday sniffer, this trace source
   * will not fire on PACKET_OTHERHOST events.
   *
   * On the transmit size, this trace hook will fire after a packet is dequeued
   * from the device queue for transmission.  In Linux, for example, this would
   * correspond to the point just before a device hard_start_xmit where
   * dev_queue_xmit_nit is called to dispatch the packet to the PF_PACKET
   * ETH_P_ALL handlers.
   *
   * On the receive side, this trace hook will fire when a packet is received,
   * just before the receive callback is executed.  In Linux, for example,
   * this would correspond to the point at which the packet is dispatched to
   * packet sniffers in netif_receive_skb.
   *
   * \see class CallBackTraceSource
   */
  TracedCallback<Ptr<const Packet> > m_snifferTrace;

  /**
   * A trace source that emulates a promiscuous mode protocol sniffer connected
   * to the device.  This trace source fire on packets destined for any host
   * just like your average everyday packet sniffer.
   *
   * On the transmit size, this trace hook will fire after a packet is dequeued
   * from the device queue for transmission.  In Linux, for example, this would
   * correspond to the point just before a device hard_start_xmit where
   * dev_queue_xmit_nit is called to dispatch the packet to the PF_PACKET
   * ETH_P_ALL handlers.
   *
   * On the receive side, this trace hook will fire when a packet is received,
   * just before the receive callback is executed.  In Linux, for example,
   * this would correspond to the point at which the packet is dispatched to
   * packet sniffers in netif_receive_skb.
   *
   * \see class CallBackTraceSource
   */
  TracedCallback<Ptr<const Packet> > m_promiscSnifferTrace;
};

} // namespace ns3

#endif /* DAMANETDEVICE_H */

