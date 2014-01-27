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
#include "simplebroadcastchannel.h"
#include "damacontroller.h"
#include "simplebroadcastmac.h"

NS_LOG_COMPONENT_DEFINE ("SimpleBroadcastChannel");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (SimpleBroadcastChannel);

TypeId
SimpleBroadcastChannel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SimpleBroadcastChannel")
    .SetParent<DamaChannel> ()
    .AddConstructor<SimpleBroadcastChannel> ()
    .AddAttribute ("PropagationTime",
                   "Time taken by the channel to propagate the signal",
                   TimeValue (MilliSeconds (200.0)),
                   MakeTimeAccessor (&SimpleBroadcastChannel::m_propagationTime),
                   MakeTimeChecker ())
    ;
  return tid;
}

SimpleBroadcastChannel::SimpleBroadcastChannel()
    : m_isInUse(false),
      m_activeSender(0)
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
SimpleBroadcastChannel::SetPropagationTimeMs (uint32_t p)
{
  NS_LOG_FUNCTION (this << p);
  m_propagationTime = MicroSeconds(p);
}

void SimpleBroadcastChannel::FreeChannel ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_isInUse = false;
  m_activeSender = 0;
  m_events.clear ();
}

void
SimpleBroadcastChannel::NotifyTrasmitToOtherNodes (Ptr<const Packet> p)
{
  NS_LOG_FUNCTION (this << p);
  DamaMacList::const_iterator it;

  for (it = m_damaMacList.begin (); it != m_damaMacList.end (); ++it)
    {
      Ptr<DamaMac> tmp = *it;
      if (tmp->GetDevice () == m_activeSender->GetDevice ())
        {
          continue;
        }

      NS_LOG_LOGIC ("Scheduling a receive in the node " <<
                    tmp->GetDevice ()->GetNode ()->GetId ());

      Simulator::ScheduleWithContext (tmp->GetDevice ()->GetNode ()->GetId (),
                                      MilliSeconds(0), &DamaMac::Receive, tmp,
                                      p->Copy ());
    }
}

void
SimpleBroadcastChannel::Send (Ptr<const Packet> p, Ptr<DamaMac> sender)
{
  NS_LOG_FUNCTION (this << p << sender);

  std::vector<EventId>::iterator it;

  if (m_isInUse)
    {
      NS_LOG_LOGIC ("Collision detected");

      sender->GetDevice ()->GetController ()->NotifyCollision ();
      m_activeSender->GetDevice ()->GetController ()->NotifyCollision ();

      for (it = m_events.begin (); it != m_events.end (); ++it)
        {
          (*it).Cancel ();
        }

      m_events.clear ();
    }
  else
    {
      NS_LOG_LOGIC ("Channel was free, is occupied now");

      m_activeSender = sender;
      m_isInUse = true;

      m_events.push_back (Simulator::Schedule (
                              m_propagationTime,
                              &SimpleBroadcastChannel::NotifyTrasmitToOtherNodes,
                              this, p));

    }

  Simulator::Schedule (m_propagationTime, &SimpleBroadcastChannel::FreeChannel,
                       this);
}

}
