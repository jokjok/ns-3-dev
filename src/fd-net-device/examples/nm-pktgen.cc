/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 University of Modena and Reggio Emilia
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
 * Author: Natale Patriciello <natale.patriciello@gmail.com>
 *
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/netmap-net-device.h"
#include "ns3/netmap-net-device-helper.h"
#include "ns3/applications-module.h"

#include <cerrno>
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("NetmapNetDevicePktGen");

int
main (int argc, char *argv[])
{
  uint32_t pktcount = 1024;
  int mode = 0;
  char bufNm[] = "nmpktgen is a we";
  char bufVeth[] = "nmpktgen is a well done packet generator.";

  CommandLine cmd;
  cmd.AddValue ("pkt", "packet count", pktcount);
  cmd.AddValue ("mode", "mode", mode);
  cmd.Parse (argc, argv);

  std::list< Ptr<Packet> > pkts;
  std::list< Ptr<Packet> >::iterator it;

  for (uint32_t i=0; i<pktcount; i++)
    {
      Ptr<Packet> pkt;

      if (mode == 0)
        pkt = Create<Packet> (reinterpret_cast<const uint8_t *> (bufNm), sizeof(bufNm));
      else
        pkt = Create<Packet> (reinterpret_cast<const uint8_t *> (bufVeth), sizeof(bufVeth));
      pkts.push_back(pkt);
    }

  PacketBurst burst;
  burst.SetPacketsOwner (PacketBurst::USER);

  for (it = pkts.begin(); it != pkts.end(); ++it)
    {
      burst.AddPacket(*it);
    }

  if (mode == 0)
    {
      Ptr<NetmapNetDevice> d = Create<NetmapNetDevice>();
      d->SetIfName ("vale0:1");
      d->SetEncapsulationMode(FdNetDevice::LLC);

      d->SendManyFrom(&burst, Mac48Address("00:00:00:00:00:01"),
                      Mac48Address("ff:ff:ff:ff:ff:ff"), 0);
    }
  else
    {
      Ptr<FdNetDevice> d = Create<FdNetDevice>();

      int sk;
      struct ifreq *if_idx = (struct ifreq*) malloc(sizeof(struct ifreq));

      struct ifreq if_mac;
      struct sockaddr_ll sll;

      if ((sk = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1)
        {
          std::cout << " No socket" << std::endl;
          exit(1);
        }

      memset(if_idx, 0, sizeof(struct ifreq));
      strncpy(if_idx->ifr_name, "veth1", IFNAMSIZ-1);

      if (ioctl(sk, SIOCGIFINDEX, if_idx) < 0)
        {
          std::cout << " No SIOCGIF" << std::endl;
          exit(1);
        }

      memset(&sll, 0, sizeof(sll));
      sll.sll_family = AF_PACKET;
      sll.sll_ifindex = if_idx->ifr_ifindex;
      sll.sll_protocol = htons(ETH_P_ALL);

      if ((if_idx->ifr_flags | IFF_UP | IFF_BROADCAST | IFF_RUNNING) != if_idx->ifr_flags)
        {
          if_idx->ifr_flags |= IFF_UP | IFF_BROADCAST | IFF_RUNNING;
          if( ioctl( sk, SIOCSIFFLAGS, if_idx ) < 0 )
            {
              std::cout << " No FLAG" << std::endl;
              exit(1);
            }
        }

      if (bind(sk, (struct sockaddr *) &sll, sizeof(sll)) < 0)
        {
          std::cout << " No bind" << std::endl;
          exit(1);
        }

      memset(&if_mac, 0, sizeof(struct ifreq));
      strncpy(if_mac.ifr_name, "veth1", IFNAMSIZ-1);

      if (ioctl(sk, SIOCGIFHWADDR, &if_mac) < 0)
        {
          std::cout << " No ifhw" << std::endl;
          exit(1);
        }

      d->SetFileDescriptor(sk);
      d->SetEncapsulationMode(FdNetDevice::LLC);

      for (it = pkts.begin(); it != pkts.end(); ++it)
        {
          d->SendFrom(*it, Mac48Address("b6:b3:95:e9:46:6a"),
                      Mac48Address("ff:ff:ff:ff:ff:ff"), 0);
        }

      free(if_idx);
    }

}

