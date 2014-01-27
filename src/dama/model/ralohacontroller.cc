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
#include "ralohacontroller.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("RAlohaController");


// TODO
// 1 - Move lot of tdma in the tdmacontroller
// 2 - fix the gaurdtime and the intervalframe time
// 3 - Use a backoff for the waiting period after a collision
// 4 - Check why the PCAP file doesn't have any packet
// 5 - Code SatelliteChannel
namespace ns3 {

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT if (!m_mac.empty()) \
            {std::clog << "[NodeId=" << \
             m_mac.front ()->GetDevice ()->GetNode ()->GetId() << "] "; }

NS_OBJECT_ENSURE_REGISTERED (RAlohaController);

TypeId
RAlohaController::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RAlohaController")
    .SetParent<TdmaController> ()
    .AddConstructor<RAlohaController> ()
    ;
  return tid;
}

RAlohaController::RAlohaController() :
  TdmaController (),
  m_currentSlot (0),
  m_waitingSlot (0)
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
RAlohaController::ScheduleTdmaSessions()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (GetSlotTime () != 0);

  NS_LOG_LOGIC ("Scheduling a session in " << (GetSlotTime () -
                                               NanoSeconds(10)).GetMilliSeconds () << " ms");
  Simulator::Schedule (GetSlotTime () - NanoSeconds (10),
                       &RAlohaController::StartTransmission, this);
}

void
RAlohaController::DoDispose ()
{
  m_nextScheduledTransmission.Cancel ();

  TdmaController::DoDispose ();
}

void
RAlohaController::DoInitialize ()
{
  NS_LOG_FUNCTION_NOARGS ();
  for (uint32_t i=0; i<m_slotNumber; ++i)
    {
      m_slotStatus[i] = std::bitset<2> ();
      NS_LOG_LOGIC("Slot " << i << " status " << m_slotStatus[i]);
    }

  TdmaController::DoInitialize ();
}

/*
 * [ ]  [  ]
 *  ^    ^
 *  |    |
 * allowe|d = 1
 *       |
 *      forbidden = 1
 */
bool
RAlohaController::IsCurrentSlotAllowed ()
{
  return m_slotStatus[m_currentSlot].test(0);
}

bool
RAlohaController::IsCurrentSlotForbidden ()
{
  return m_slotStatus[m_currentSlot].test(1);
}

void
RAlohaController::SetCurrentSlotAllowed ()
{
  m_slotStatus[m_currentSlot].set(0);
}

void
RAlohaController::SetCurrentSlotForbidden ()
{
  m_slotStatus[m_currentSlot].set(1);
}

void
RAlohaController::ResetCurrentSlotAllowed ()
{
    m_slotStatus[m_currentSlot].reset(0);
}

void
RAlohaController::ResetCurrentSlotForbidden ()
{
    m_slotStatus[m_currentSlot].reset(1);
}

void
RAlohaController::StartTransmission ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (! m_mac.empty ());

  ++m_currentSlot;

  if (m_currentSlot > m_slotNumber - 1)
    {
      m_currentSlot = 0;
    }

  NS_LOG_LOGIC ("Current slot: " << m_currentSlot);

  if (IsCurrentSlotAllowed () || ! IsCurrentSlotForbidden ())
    {
      NS_LOG_LOGIC ("The slot is allowed or not forbidden");

      if (m_mac.front ()->HasDataToSend ())
        {
          NS_LOG_LOGIC ("The MAC has data to send");

          if (m_waitingSlot == 0)
            {
              m_mac.front ()->SendQueueHead ();
              SetCurrentSlotAllowed ();

              NS_LOG_LOGIC ("Slot status after a Send: " << m_slotStatus[m_currentSlot]);
            }
          else
            {
              NS_LOG_LOGIC ("Waiting a backoff period");
              --m_waitingSlot;
            }
        }
      else
        {
          ResetCurrentSlotAllowed ();
          NS_LOG_LOGIC ("No data to transmit, slot reset: " <<
                        m_slotStatus[m_currentSlot]);
        }
    }

  NS_LOG_LOGIC ("Scheduling next slot at time " <<
                GetSlotTime ().GetMilliSeconds ()<< " ms from now");

  m_nextScheduledTransmission = Simulator::Schedule (GetSlotTime (),
                       &RAlohaController::StartTransmission, this);
}

void
RAlohaController::NotifyRx ()
{
  NS_LOG_FUNCTION_NOARGS ();
  if (! IsCurrentSlotAllowed ())
    {
      SetCurrentSlotForbidden ();
    }
}

void RAlohaController::NotifyPromiscRx ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NotifyRx();
}

void
RAlohaController::NotifyCollision ()
{
  NS_LOG_FUNCTION_NOARGS ();
  ResetCurrentSlotAllowed ();
  ResetCurrentSlotForbidden ();

  Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();

  m_waitingSlot = x->GetInteger (0, m_slotNumber);

  NS_LOG_LOGIC ("Slot status after a collision: " << m_slotStatus[m_currentSlot]);
}

}
