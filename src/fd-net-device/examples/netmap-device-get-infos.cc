/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"

#include "../model/netmap-priv-impl.h"

#include "iostream"
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("NetmapDeviceGetInfos");

int
main (int argc, char *argv[])
{
  std::string deviceName ("eth1");
  bool checkSystemNetmap = true;
  bool checkNetmapCap = true;
  bool getTxRingNumber = true;
  bool getRxRingNumber = true;
  bool getSlotsInTx = true;
  bool getSlotsInRx = true;
  bool getMmapMemSize = true;

  CommandLine cmd;
  cmd.AddValue ("deviceName", "device name", deviceName);
  cmd.AddValue ("checkNetmapCap", "check netmap capabilities for interface", checkNetmapCap);
  cmd.AddValue ("checkSystemNetmap", "check netmap capabilities for the system", checkSystemNetmap);

  cmd.Parse (argc, argv);

  NetmapPrivImpl dev(NULL);
  dev.SetIfName (deviceName);

  if (checkSystemNetmap)
    {
      if (NetmapPrivImpl::IsSystemNetmapCapable ())
        std::cout << "system has Netmap capabilities" << std::endl;
      else
        std::cout << "system hasn't Netmap capabilities" << std::endl;
    }

  if (checkNetmapCap)
    {
      if (NetmapPrivImpl::IsDeviceNetmapCapable(deviceName))
        std::cout << deviceName << " has Netmap capabilities" << std::endl;
      else
        {
          std::cout << deviceName << " hasn't Netmap capabilities. No more results" << std::endl;
          return 1;
        }
    }

  if (getTxRingNumber)
    {
      std::cout << "TX rings: " << dev.GetTxRingsNumber () << std::endl;
    }

  if (getRxRingNumber)
    {
      std::cout << "RX rings: " << dev.GetRxRingsNumber () << std::endl;
    }

  if (getSlotsInTx)
    {
      std::cout << "Slots in TX rings: " << dev.GetSlotsInTxRing () << std::endl;
    }

  if (getSlotsInRx)
    {
      std::cout << "Slots in RX rings: " << dev.GetSlotsInRxRing () << std::endl;
    }

  if (getMmapMemSize)
    {
      std::cout << "Mmap size: " << dev.GetMmapMemSize () << std::endl;
    }

}


