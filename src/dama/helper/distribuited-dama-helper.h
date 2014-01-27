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
#ifndef DISTRIBUITED_DAMA_HELPER_H
#define DISTRIBUITED_DAMA_HELPER_H

#include "ns3/damanetdevice.h"
#include "ns3/damachannel.h"
#include "ns3/damacontroller.h"
#include "ns3/damamac.h"

namespace ns3
{

class Packet;

/**
 * \brief build a set of DamaNetDevice objects
 *
 * Normally we eschew multiple inheritance, however, the classes
 * PcapUserHelperForDevice and AsciiTraceUserHelperForDevice are
 * treated as "mixins".  A mixin is a self-contained class that
 * encapsulates a general attribute or a set of functionality that
 * may be of interest to many other classes.
 */
class DistribuitedDamaHelper : public PcapHelperForDevice,
                               public AsciiTraceHelperForDevice
{
public:
  /**
   * Construct a DamaHelper.
   */
  DistribuitedDamaHelper (std::string channelType, std::string controllerType,
                          std::string macType);
  virtual ~DistribuitedDamaHelper () {}

  /**
   * \param n1 the name of the attribute to set
   * \param v1 the value of the attribute to set
   *
   * Set these attributes on each ns3::DamaNetDevice created
   * by DamaHelper::Install
   */
  void SetDeviceAttribute (std::string n1, const AttributeValue &v1);

  /**
   * \param n1 the name of the attribute to set
   * \param v1 the value of the attribute to set
   *
   * Set these attributes on each ns3::DamaController created
   * by DamaHelper::Install
   */
  void SetControllerAttribute (std::string n1, const AttributeValue &v1);

  /**
   * \param n1 the name of the attribute to set
   * \param v1 the value of the attribute to set
   *
   * Set these attributes on each ns3::DamaMac created
   * by DamaHelper::Install
   */
  void SetMacAttribute (std::string n1, const AttributeValue &v1);

  /**
   * \param n1 the name of the attribute to set
   * \param v1 the value of the attribute to set
   *
   * Set these attributes on each ns3::DamaChannel created
   * by DamaHelper::Install
   */
  void SetChannelAttribute (std::string n1, const AttributeValue &v1);

  /**
   * This method creates an ns3::DamaChannel with the attributes configured by
   * DamaHelper::SetChannelAttribute, an ns3::DamaNetDevice with the attributes
   * configured by DamaHelper::SetDeviceAttribute and then adds the device
   * to the node and attaches the channel to the device.
   *
   * \param node The node to install the device in
   * \returns A container holding the added net device.
   */
  NetDeviceContainer Install (Ptr<Node> node);

  /**
   * This method creates an ns3::DamaNetDevice with the attributes configured by
   * DamaHelper::SetDeviceAttribute and then adds the device to the node and
   * attaches the provided channel to the device.
   *
   * \param node The node to install the device in
   * \param channel The channel to attach to the device.
   * \returns A container holding the added net device.
   */
  NetDeviceContainer Install (Ptr<Node> node, Ptr<DamaChannel> channel) const;

  /**
   * This method creates an ns3::DamaChannel with the attributes configured by
   * DamaHelper::SetChannelAttribute.  For each Ptr<node> in the provided
   * container: it creates an ns3::DamaNetDevice (with the attributes
   * configured by DamaHelper::SetDeviceAttribute); adds the device to the
   * node; and attaches the channel to the device.
   *
   * \param c The NodeContainer holding the nodes to be changed.
   * \returns A container holding the added net devices.
   */
  NetDeviceContainer Install (const NodeContainer &c);

  /**
   * For each Ptr<node> in the provided container, this method creates an
   * ns3::DamaNetDevice (with the attributes configured by
   * DamaHelper::SetDeviceAttribute); adds the device to the node; and attaches
   * the provided channel to the device.
   *
   * \param c The NodeContainer holding the nodes to be changed.
   * \param channel The channel to attach to the devices.
   * \returns A container holding the added net devices.
   */
  NetDeviceContainer Install (const NodeContainer &c, Ptr<DamaChannel> channel) const;

private:
  /*
   * \internal
   */
  Ptr<NetDevice> InstallPriv (Ptr<Node> node, Ptr<DamaChannel> channel) const;

  /**
   * \brief Enable pcap output on the indicated net device.
   * \internal
   *
   * NetDevice-specific implementation mechanism for hooking the trace and
   * writing to the trace file.
   *
   * \param prefix Filename prefix to use for pcap files.
   * \param nd Net device for which you want to enable tracing.
   * \param promiscuous If true capture all possible packets available at the device.
   * \param explicitFilename Treat the prefix as an explicit filename if true
   */
  virtual void EnablePcapInternal (std::string prefix, Ptr<NetDevice> nd, bool promiscuous, bool explicitFilename);

  /**
   * \brief Enable ascii trace output on the indicated net device.
   * \internal
   *
   * NetDevice-specific implementation mechanism for hooking the trace and
   * writing to the trace file.
   *
   * \param stream The output stream object to use when logging ascii traces.
   * \param prefix Filename prefix to use for ascii trace files.
   * \param nd Net device for which you want to enable tracing.
   * \param explicitFilename Treat the prefix as an explicit filename if true
   */
  virtual void EnableAsciiInternal (Ptr<OutputStreamWrapper> stream,
                                    std::string prefix,
                                    Ptr<NetDevice> nd,
                                    bool explicitFilename);

  ObjectFactory m_deviceFactory;
  ObjectFactory m_channelFactory;
  ObjectFactory m_controllerFactory;
  ObjectFactory m_macFactory;

  Ptr<DamaChannel> m_channel;

};

} // namespace ns3

#endif /* DISTRIBUITED_DAMA_HELPER_H */

