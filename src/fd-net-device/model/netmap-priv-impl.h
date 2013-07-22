/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 Natale Patriciello <natale.patriciello@gmail.com>
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
#ifndef NETMAPPRIVIMPL_H
#define NETMAPPRIVIMPL_H

#include <string>
#include <stdint.h>

#include "ns3/ethernet-header.h"
#include "ns3/packet-burst.h"
#include "ns3/system-mutex.h"
#include "ns3/system-thread.h"
#include "ns3/system-condition.h"

struct netmap_if;
struct netmap_ring;

namespace ns3 {

class NetmapNetDevice;

struct NetmapPacketCB
{
  uint8_t *buffer;
  uint16_t len;
  uint16_t id;
  NetmapPacketCB *next;
};

/**
 * \ingroup netmap-net-device
 * \internal
 *
 * \brief The implementation (private) class
 *
 *
 */
class NetmapPrivImpl
{
public:
  static const int BUFFER_LENGTH = 1024;
  /**
   * Check if the given device is netmap capable
   *
   * To be netmap capable, a device should have its netmap-ready
   * kernel module loaded. Actually, only the following list of
   * network card are netmap capable:
   * - card 1
   * - card 2
   * - ...
   * - card n
   *
   * \param Name of the device to check
   * \return true if the device is netmap capable
   */
  static bool IsDeviceNetmapCapable (const std::string& ifName);

  /**
   * Check if the system supports Netmap
   *
   * A system is Netmap capable if the netmap kernel module
   * is currently loaded and running. Actually, the kernel module
   * exists only for Linux and FreeBSD. Check Netmap homepage
   * for an updated version.
   *
   * \return true if the system is netmap capable
   */
  static bool IsSystemNetmapCapable ();

  NetmapPrivImpl (NetmapNetDevice *q);
  ~NetmapPrivImpl ();

  void SetIfName (const std::string& ifName);

  bool OpenFd();
  bool CloseFd();
  bool StartNmMode();
  bool StopNmMode();
  bool IsFdOpened () const;

  bool RegisterHwRingId(uint32_t ringId);
  bool RegisterSwRing();

  bool setFlags (short flags);
  short getFlags ();

  uint32_t Send (Ptr<PacketBurst> packets);

  uint16_t GetTxRingsNumber ();
  uint16_t GetRxRingsNumber ();
  uint32_t GetSlotsInTxRing ();
  uint32_t GetSlotsInRxRing ();
  uint32_t GetMmapMemSize   ();

protected:
  void DoRead ();
  void Consume ();

  NetmapNetDevice *m_q;

private:

  void UpdateInfos();
  void ResetInfos();

  NetmapPacketCB *m_readBuffer_bit;
  NetmapPacketCB *m_readBuffer_eit;

  Ptr<SystemThread> m_readThread;
  Ptr<SystemThread> m_consumerThread;
  bool m_readThreadRun;
  bool m_consumerThreadRun;

  SystemCondition m_consumerCond;

  std::string m_ifName;
  bool m_infosAreValid;

  uint16_t m_txRings;
  uint16_t m_rxRings;
  uint32_t m_slotTx;
  uint32_t m_slotRx;

  uint32_t m_mmapMemSize;

  char *m_mmapMem;

  int m_fd;

  struct netmap_if *m_nifp;

  uint32_t m_begin, m_end;            /* first..last+1 rings to check */
  struct netmap_ring *m_tx, *m_rx;    /* shortcuts */


  // TEST

  uint64_t m_received;
  uint64_t m_processed;
};

}
#endif // NETMAPPRIVIMPL_H
