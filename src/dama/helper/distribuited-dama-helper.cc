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
#include "distribuited-dama-helper.h"

#include "ns3/abort.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/object-factory.h"
#include "ns3/queue.h"
#include "ns3/csma-net-device.h"
#include "ns3/csma-channel.h"
#include "ns3/config.h"
#include "ns3/packet.h"
#include "ns3/names.h"

#include "ns3/trace-helper.h"

#include <string>

NS_LOG_COMPONENT_DEFINE ("DistribuitedDamaHelper");

namespace ns3 {

DistribuitedDamaHelper::DistribuitedDamaHelper (std::string channelType,
                                                std::string controllerType,
                                                std::string macType) :
    m_channel(0)
{
  m_deviceFactory.SetTypeId ("ns3::DamaNetDevice");
  m_controllerFactory.SetTypeId (controllerType);
  m_macFactory.SetTypeId (macType);
  m_channelFactory.SetTypeId (channelType);
}

void
DistribuitedDamaHelper::SetDeviceAttribute (std::string n1, const AttributeValue &v1)
{
  m_deviceFactory.Set (n1, v1);
}

void
DistribuitedDamaHelper::SetControllerAttribute (std::string n1, const AttributeValue &v1)
{
  m_controllerFactory.Set (n1, v1);
}

void
DistribuitedDamaHelper::SetChannelAttribute (std::string n1, const AttributeValue &v1)
{
  m_channelFactory.Set (n1, v1);
}

void
DistribuitedDamaHelper::SetMacAttribute (std::string n1, const AttributeValue &v1)
{
  m_macFactory.Set (n1, v1);
}

void
DistribuitedDamaHelper::EnablePcapInternal (std::string prefix, Ptr<NetDevice> nd,
                                bool promiscuous, bool explicitFilename)
{
  //
  // All of the Pcap enable functions vector through here including the ones
  // that are wandering through all of devices on perhaps all of the nodes in
  // the system.  We can only deal with devices of type DamaNetDevice.
  //
  Ptr<DamaNetDevice> device = nd->GetObject<DamaNetDevice> ();
  if (device == 0)
    {
      NS_LOG_INFO ("DistribuitedDamaHelper::EnablePcapInternal(): Device " << device <<
                   " not of type ns3::DamaNetDevice");
      return;
    }

  PcapHelper pcapHelper;

  std::string filename;
  if (explicitFilename)
    {
      filename = prefix;
    }
  else
    {
      filename = pcapHelper.GetFilenameFromDevice (prefix, device);
    }

  Ptr<PcapFileWrapper> file = pcapHelper.CreateFile (filename, std::ios::out,
                                                     PcapHelper::DLT_EN10MB);
  if (promiscuous)
    {
      pcapHelper.HookDefaultSink<DamaNetDevice> (device, "PromiscSniffer", file);
    }
  else
    {
      pcapHelper.HookDefaultSink<DamaNetDevice> (device, "Sniffer", file);
    }
}

void
DistribuitedDamaHelper::EnableAsciiInternal (Ptr<OutputStreamWrapper> stream,
                                 std::string prefix, Ptr<NetDevice> nd,
                                 bool explicitFilename)
{
  //
  // All of the ascii enable functions vector through here including the ones
  // that are wandering through all of devices on perhaps all of the nodes in
  // the system.  We can only deal with devices of type DamaNetDevice.
  //
  Ptr<DamaNetDevice> device = nd->GetObject<DamaNetDevice> ();
  if (device == 0)
    {
      NS_LOG_INFO ("DistribuitedDamaHelper::EnableAsciiInternal(): Device " << device <<
                   " not of type ns3::DamaNetDevice");
      return;
    }

  //
  // Our default trace sinks are going to use packet printing, so we have to
  // make sure that is turned on.
  //
  Packet::EnablePrinting ();

  //
  // If we are not provided an OutputStreamWrapper, we are expected to create
  // one using the usual trace filename conventions and do a Hook*WithoutContext
  // since there will be one file per context and therefore the context would
  // be redundant.
  //
  if (stream == 0)
    {
      //
      // Set up an output stream object to deal with private ofstream copy
      // constructor and lifetime issues.  Let the helper decide the actual
      // name of the file given the prefix.
      //
      AsciiTraceHelper asciiTraceHelper;

      std::string filename;
      if (explicitFilename)
        {
          filename = prefix;
        }
      else
        {
          filename = asciiTraceHelper.GetFilenameFromDevice (prefix, device);
        }

      Ptr<OutputStreamWrapper> theStream = asciiTraceHelper.CreateFileStream (filename);

      //
      // The MacRx trace source provides our "r" event.
      //
      asciiTraceHelper.HookDefaultReceiveSinkWithoutContext<DamaNetDevice> (device, "MacRx", theStream);

      //
      // The "+", '-', and 'd' events are driven by trace sources actually in the
      // transmit queue.
      //
      //Ptr<Queue> queue = device->GetQueue ();
      //asciiTraceHelper.HookDefaultEnqueueSinkWithoutContext<Queue> (queue, "Enqueue", theStream);
      //asciiTraceHelper.HookDefaultDropSinkWithoutContext<Queue> (queue, "Drop", theStream);
      //asciiTraceHelper.HookDefaultDequeueSinkWithoutContext<Queue> (queue, "Dequeue", theStream);

      return;
    }

  //
  // If we are provided an OutputStreamWrapper, we are expected to use it, and
  // to providd a context.  We are free to come up with our own context if we
  // want, and use the AsciiTraceHelper Hook*WithContext functions, but for
  // compatibility and simplicity, we just use Config::Connect and let it deal
  // with the context.
  //
  // Note that we are going to use the default trace sinks provided by the
  // ascii trace helper.  There is actually no AsciiTraceHelper in sight here,
  // but the default trace sinks are actually publicly available static
  // functions that are always there waiting for just such a case.
  //
  uint32_t nodeid = nd->GetNode ()->GetId ();
  uint32_t deviceid = nd->GetIfIndex ();
  std::ostringstream oss;

  oss << "/NodeList/" << nodeid <<
         "/DeviceList/" << deviceid << "/$ns3::DamaNetDevice/MacRx";

  Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultReceiveSinkWithContext, stream));

  /*oss.str ("");
  oss << "/NodeList/" << nodeid << "/DeviceList/" <<
         deviceid << "/$ns3::DamaNetDevice/TxQueue/Enqueue";
  Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultEnqueueSinkWithContext, stream));

  oss.str ("");
  oss << "/NodeList/" << nodeid << "/DeviceList/" <<
         deviceid << "/$ns3::DamaNetDevice/TxQueue/Dequeue";
  Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultDequeueSinkWithContext, stream));

  oss.str ("");
  oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::DamaNetDevice/TxQueue/Drop";
  Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultDropSinkWithContext, stream));
  */
}

NetDeviceContainer
DistribuitedDamaHelper::Install (Ptr<Node> node)
{
  if (m_channel == 0)
    {
      m_channel = m_channelFactory.Create ()->GetObject<DamaChannel> ();
    }
  return Install (node, m_channel);
}

NetDeviceContainer
DistribuitedDamaHelper::Install (Ptr<Node> node, Ptr<DamaChannel> channel) const
{
  return NetDeviceContainer (InstallPriv (node, channel));
}


NetDeviceContainer
DistribuitedDamaHelper::Install (const NodeContainer &c)
{
  if (m_channel == 0)
    {
      m_channel = m_channelFactory.Create ()->GetObject<DamaChannel> ();
    }
  return Install (c, m_channel);
}

NetDeviceContainer
DistribuitedDamaHelper::Install (const NodeContainer &c, Ptr<DamaChannel> channel) const
{
  NetDeviceContainer devs;

  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); i++)
    {
      devs.Add (InstallPriv (*i, channel));
    }

  return devs;
}

Ptr<NetDevice>
DistribuitedDamaHelper::InstallPriv (Ptr<Node> node, Ptr<DamaChannel> channel) const
{
  Ptr<DamaNetDevice> device = m_deviceFactory.Create<DamaNetDevice> ();
  Ptr<DamaMac> mac = m_macFactory.Create ()->GetObject<DamaMac> ();
  Ptr<DamaController> controller = m_controllerFactory.Create ()->GetObject<DamaController> ();

  channel->Add (mac);
  node->AddDevice (device);

  controller->AddMac (mac);

  device->SetChannel (channel);
  device->SetMac (mac);
  device->SetController (controller);
  device->SetAddress (Mac48Address::Allocate ());

  return device;
}

} // namespace ns3


