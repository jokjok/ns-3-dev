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
#include "netmap-priv-impl.h"
#include "netmap_user.h"
#include "netmap-net-device.h"

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>

#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("NetmapPrivImpl");

NetmapPrivImpl::NetmapPrivImpl (NetmapNetDevice *q)
  : m_q (q),
    m_ifName (""),
    m_infosAreValid (false),
    m_txRings (0),
    m_rxRings (0),
    m_slotTx (0),
    m_slotRx (0),
    m_mmapMemSize (0),
    m_mmapMem(0),
    m_fd(-1)
{
}

void
NetmapPrivImpl::SetIfName (const std::string &ifName)
{
  m_infosAreValid = false;
  m_ifName = ifName;
}

bool NetmapPrivImpl::OpenFd()
{
  if (m_fd != -1)
    CloseFd ();

  m_fd = open ("/dev/netmap", O_RDWR);
  if (m_fd == -1)
    {
      NS_LOG_WARN ("Unable to open /dev/netmap");
      return false;
    }

  return true;
}

bool NetmapPrivImpl::CloseFd()
{
  close(m_fd);

  return true;
}


bool
NetmapPrivImpl::StartNmMode()
{
  struct nmreq nmr;
  memset (&nmr, sizeof(nmr), 0);

  if (m_ifName.empty ())
    {
      NS_LOG_WARN ("No interface specified");
      return false;
    }

  /*
   * Register the interface on the netmap device: from now on,
   * we can operate on the network interface without any
   * interference from the legacy network stack.
   *
   * We decide to put the first interface registration here to
   * give time to cards that take a long time to reset the PHY.
   */
  nmr.nr_version = NETMAP_API;
  strncpy (nmr.nr_name, m_ifName.c_str (), m_ifName.length () + 1);

  if (ioctl (m_fd, NIOCREGIF, &nmr) == -1)
    {
          NS_LOG_WARN ("Unable to register interface in netmap mode");
          m_mmapMem = 0;
          return false;
    }

  m_mmapMem = (char*) mmap (0, GetMmapMemSize (),
                            PROT_WRITE | PROT_READ,
                            MAP_SHARED, m_fd, 0);
  if (m_mmapMem == MAP_FAILED)
    {
      NS_LOG_WARN ("Can't mmap"); // nmr.nr_memsize >> 10
      return false;
    }

  m_nifp = NETMAP_IF(m_mmapMem, nmr.nr_offset);

  m_begin = 0;
  m_end = GetRxRingsNumber (); // XXX max of rx or tx
  m_tx = NETMAP_TXRING (m_nifp, 0);
  m_rx = NETMAP_RXRING (m_nifp, 0);

  short flags = getFlags ();

  if ((flags & IFF_UP) == 0)
    {
      flags |= IFF_UP;
      if (! setFlags (flags))
        {
          NS_LOG_WARN ("Can't get if up");
          return false;
        }
    }

  return true;
}

bool
NetmapPrivImpl::setFlags (short flags)
{
  struct ifreq ifr;
  int error;
  int fd;

  fd = socket (AF_INET, SOCK_DGRAM, 0);
  if (fd < 0)
    {
      return false;
    }

  memset (&ifr, 0, sizeof (ifr));
  strncpy (ifr.ifr_name, m_ifName.c_str (), m_ifName.length () + 1);

  ifr.ifr_flags = flags;

  error = ioctl (fd, SIOCSIFFLAGS, &ifr);

  close (fd);

  if (error)
    return false;

  return true;
}

short
NetmapPrivImpl::getFlags ()
{
  struct ifreq ifr;
  int error;
  int fd;

  fd = socket (AF_INET, SOCK_DGRAM, 0);
  if (fd < 0)
    {
      return false;
    }

  memset (&ifr, 0, sizeof (ifr));
  strncpy (ifr.ifr_name, m_ifName.c_str (), m_ifName.length () + 1);

  ifr.ifr_flags = 0 & 0xffff;

  error = ioctl (fd, SIOCGIFFLAGS, &ifr);

  close (fd);

  if (error)
    return -1;

  return ifr.ifr_flags;
}

bool
NetmapPrivImpl::Send (Ptr<PacketBurst> packets, const EthernetHeader &header)
{
  uint32_t sent = 0;
  uint32_t i;
  char *p;
  struct pollfd fds[1];
  std::list< Ptr<Packet> >::const_iterator iterator;
  Ptr<Packet> pkt;
  struct netmap_ring *txring;

  /* setup poll(2) mechanism. */
  memset(fds, 0, sizeof(fds));
  fds[0].fd = m_fd;
  fds[0].events = (POLLOUT);

  iterator = packets->Begin();
  while (sent < packets->GetNPackets())
    {
      /*
       * wait for available room in the send queue(s)
       */
      if (poll(fds, 1, 2000) <= 0)
        {
          return false;
        }

      /*
       * scan our queues and send on those with room
       */

      for (i = m_begin; i < m_end; i++)
        {
          uint32_t limit = m_slotTx/2;
          uint32_t enqueued;
          uint32_t cur;

          if (packets->GetNPackets() > 0 && packets->GetNPackets() - sent < limit)
            limit = packets->GetNPackets() - sent;

          txring = NETMAP_TXRING(m_nifp, i);

          if (txring->avail == 0)
            continue;

          cur = txring->cur;

          if (txring->avail < limit)
                  limit = txring->avail;

          for (enqueued = 0; enqueued < limit; ++enqueued)
            {
              struct netmap_slot *slot = &txring->slot[cur];
              p = NETMAP_BUF(txring, slot->buf_idx);
              pkt = *iterator;
              pkt->AddHeader (header);
              pkt->CopyData((uint8_t*) p, pkt->GetSize());

              slot->len = pkt->GetSize();

              if (enqueued == limit - 1)
                slot->flags |= NS_REPORT;

              cur = NETMAP_RING_NEXT(txring, cur);
            }

          txring->avail -= enqueued;
          txring->cur = cur;

          sent += enqueued;
        }
    }

  /* flush any remaining packets */
  ioctl(fds[0].fd, NIOCTXSYNC, NULL);

  /* final part: wait all the TX queues to be empty. */
  for (i = m_begin; i < m_end; i++)
    {
      txring = NETMAP_TXRING(m_nifp, i);
      while (!NETMAP_TX_RING_EMPTY(txring))
        {
          ioctl(fds[0].fd, NIOCTXSYNC, NULL);
          usleep(1); /* wait 1 tick */
        }
    }

  return true;
}


bool
NetmapPrivImpl::RegisterHwRingId(uint32_t ringId)
{
  m_begin = ringId & NETMAP_RING_MASK;
  m_end = m_begin + 1;
  m_tx = NETMAP_TXRING (m_nifp, m_begin);
  m_rx = NETMAP_RXRING (m_nifp, m_begin);

  return true;
}

bool
NetmapPrivImpl::RegisterSwRing ()
{
  m_begin = GetRxRingsNumber ();
  m_end = m_begin + 1;
  m_tx = NETMAP_TXRING(m_nifp, GetTxRingsNumber ());
  m_rx = NETMAP_RXRING(m_nifp, GetRxRingsNumber ());

  return true;
}

bool
NetmapPrivImpl::StopNmMode()
{
  if (! IsFdOpened())
    return false;

  struct nmreq nmr;
  memset (&nmr, sizeof(nmr), 0);

  nmr.nr_version = NETMAP_API;
  strncpy (nmr.nr_name, m_ifName.c_str (), m_ifName.length () + 1);

  ioctl(m_fd, NIOCUNREGIF, &nmr);

  munmap(m_mmapMem, GetMmapMemSize ());

  return true;
}

bool
NetmapPrivImpl::IsFdOpened () const
{
  return (m_fd != -1);
}

void
NetmapPrivImpl::ResetInfos ()
{
  m_txRings = 0;
  m_rxRings = 0;
  m_slotTx = 0;
  m_slotRx = 0;
  m_mmapMemSize = 0;
}


void
NetmapPrivImpl::UpdateInfos ()
{
  m_infosAreValid = false;

  if (m_ifName.empty ())
    {
      ResetInfos ();
      return;
    }
  else
    {
      struct nmreq nmr;

      if (! IsFdOpened ())
        {
          if (! OpenFd ())
            {
              ResetInfos ();
              return;
            }
        }

      memset (&nmr, sizeof(nmr), 0);
      nmr.nr_version = NETMAP_API;

      if ((ioctl (m_fd, NIOCGINFO, &nmr)) == -1)
        {
          NS_LOG_WARN ("Unable to get if info without name");
        }

      memset (&nmr, sizeof(nmr), 0);
      nmr.nr_version = NETMAP_API;

      strncpy (nmr.nr_name, m_ifName.c_str (), m_ifName.length () + 1);

      if ((ioctl (m_fd, NIOCGINFO, &nmr)) == -1)
        {
          NS_LOG_WARN ("Unable to get if info");
          ResetInfos ();
        }
      else
        {
          m_rxRings = nmr.nr_rx_rings;
          m_txRings = nmr.nr_tx_rings;

          m_slotRx  = nmr.nr_rx_slots;
          m_slotTx  = nmr.nr_tx_slots;

          m_mmapMemSize = nmr.nr_memsize;
        }
    }

  m_infosAreValid = true;
}

uint16_t NetmapPrivImpl::GetTxRingsNumber ()
{
  if (! m_infosAreValid)
    UpdateInfos ();

  return m_txRings;
}

uint16_t NetmapPrivImpl::GetRxRingsNumber ()
{
  if (! m_infosAreValid)
    UpdateInfos ();

  return m_rxRings;
}

uint32_t NetmapPrivImpl::GetSlotsInTxRing ()
{
  if (! m_infosAreValid)
    UpdateInfos ();

  return m_slotTx;
}

uint32_t NetmapPrivImpl::GetSlotsInRxRing ()
{
  if (! m_infosAreValid)
    UpdateInfos ();

  return m_slotRx;
}

uint32_t NetmapPrivImpl::GetMmapMemSize()
{
  if (! m_infosAreValid)
    UpdateInfos ();

  return m_mmapMemSize;
}

bool
NetmapPrivImpl::IsDeviceNetmapCapable (const std::string &ifName)
{
  struct nmreq nmr;
  int fd;
  bool ret = true;

  memset (&nmr, sizeof(nmr), 0);
  nmr.nr_version = NETMAP_API;

  fd = open ("/dev/netmap", O_RDWR);
  if (fd == -1)
    {
      return false;
    }
  else
    {
      memset (&nmr, sizeof(nmr), 0);
      nmr.nr_version = NETMAP_API;
      strncpy (nmr.nr_name, ifName.c_str (), ifName.length () + 1);

      if ((ioctl (fd, NIOCGINFO, &nmr)) == -1)
        ret = false;
    }

  close (fd);

  return ret;
}

bool
NetmapPrivImpl::IsSystemNetmapCapable ()
{
  int  fd  = open ("/dev/netmap", O_RDWR);
  bool ret = (fd != -1);

  close (fd);

  return ret;
}

}
