/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This code was ported from NS-2.34, with licence:
 *
 * Copyright (c) 1990-1997 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *
 * This port: Copyright © 2011 Marcos Talau (talau@users.sourceforge.net)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Thanks to: Duy Nguyen<duy@soe.ucsc.edu> by RED efforts in NS3
 *
 */

#include "ns3/log.h"
#include "ns3/enum.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "red-queue.h"
#include "ns3/simulator.h"
#include "ns3/nstime.h"
#include "ns3/random-variable.h"

#include <cstdlib>

NS_LOG_COMPONENT_DEFINE ("RedQueue");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (RedQueue);

TypeId RedQueue::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RedQueue")
                      .SetParent<Queue> ()
                      .AddConstructor<RedQueue> ()
    .AddAttribute ("Mode",
                   "Bytes or Packets",
                   EnumValue (PACKETS),
                   MakeEnumAccessor (&RedQueue::SetMode),
                   MakeEnumChecker (BYTES, "Bytes",
                                    PACKETS, "Packets"))
    .AddAttribute ("MeanPktSize",
                   "Average of Packet Size",
                   UintegerValue (500),
                   MakeUintegerAccessor (&RedQueue::m_meanPktSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("IdlePktSize",
                   "Avg pkt size used during idle times. Used when m_cautions = 3",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RedQueue::m_idlePktSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Wait",
                   "True for waiting between dropped packets",
                   BooleanValue (true),
                   MakeBooleanAccessor (&RedQueue::m_wait),
                   MakeBooleanChecker ())
    .AddAttribute ("Gentle",
                   "True to increases dropping prob. slowly when ave queue exceeds maxthresh",
                   BooleanValue (true),
                   MakeBooleanAccessor (&RedQueue::m_gentle),
                   MakeBooleanChecker ())
    .AddAttribute ("m_minTh",
                   "Min avg length threshold in packets/bytes",
                   DoubleValue (5),
                   MakeDoubleAccessor (&RedQueue::m_minTh),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("m_maxTh",
                   "Max avg length threshold in packets/bytes",
                   DoubleValue (15),
                   MakeDoubleAccessor (&RedQueue::m_maxTh),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("QueueLimit",
                   "Queue limit in bytes/packets",
                   UintegerValue (25),
                   MakeUintegerAccessor (&RedQueue::m_queueLimit),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("m_qW",
                   "Queue weight given to cur q size sample",
                   DoubleValue (0.002),
                   MakeDoubleAccessor (&RedQueue::m_qW),
                   MakeDoubleChecker <double> ())
    .AddAttribute ("m_lInterm",
                   "The max probability of dropping a packet",
                   DoubleValue (50),
                   MakeDoubleAccessor (&RedQueue::m_lInterm),
                   MakeDoubleChecker <double> ())
    .AddAttribute ("m_ns1Compat",
                   "NS-1 compatibility",
                   BooleanValue (false),
                   MakeBooleanAccessor (&RedQueue::m_ns1Compat),
                   MakeBooleanChecker ())
    .AddAttribute ("LinkBandwidth", 
                   "The RED link bandwidth",
                   DataRateValue (DataRate ("1.5Mbps")),
                   MakeDataRateAccessor (&RedQueue::m_linkBandwidth),
                   MakeDataRateChecker ())
    .AddAttribute ("LinkDelay", 
                   "The RED link delay",
                   TimeValue (MilliSeconds (20)),
                   MakeTimeAccessor (&RedQueue::m_linkDelay),
                   MakeTimeChecker ())
  ;

  return tid;
}

void
RedQueue::SetQueueLimit(uint32_t lim)
{
  m_queueLimit = lim;
}

void
RedQueue::SetTh(double min, double max)
{
  m_minTh = min;
  m_maxTh = max;
}

/*
 * Note: if the link bandwidth changes in the course of the
 * simulation, the bandwidth-dependent RED parameters do not change.
 * This should be fixed, but it would require some extra parameters,
 * and didn't seem worth the trouble...
 */
void
RedQueue::InitializeParams (void)
{
  m_cautious = 0;
  m_ptc = m_linkBandwidth.GetBitRate () / (8.0 * m_meanPktSize);
  
  m_curMaxP = 0.02;

  m_qAvg = 0.0;
  m_count = 0;
  m_countBytes = 0;
  m_old = 0;
  m_idle = 1;

  double th_diff = (m_maxTh - m_minTh);
  if (th_diff == 0)
    {
      //XXX this last check was added by a person who knows
      //nothing of this code just to stop FP div by zero.
      //Values for thresholds were equal at time 0.  If you
      //know what should be here, please cleanup and remove
      //this comment.
      th_diff = 1.0; 
    }
  m_vA = 1.0 / th_diff;
  m_curMaxP = 1.0 / m_lInterm;
  m_vB = - m_minTh / th_diff;
  
  if (m_gentle)
    {
      m_vC = (1.0 - m_curMaxP) / m_maxTh;
      m_vD = 2.0 * m_curMaxP - 1.0;
    }
  m_idleTime = NanoSeconds (0);

/*
 * If q_weight=0, set it to a reasonable value of 1-exp(-1/C)
 * This corresponds to choosing q_weight to be of that value for
 * which the packet time constant -1/ln(1-q_weight) per default RTT 
 * of 100ms is an order of magnitude more than the link capacity, C.
 *
 * If q_weight=-1, then the queue weight is set to be a function of
 * the bandwidth and the link propagation delay.  In particular, 
 * the default RTT is assumed to be three times the link delay and 
 * transmission delay, if this gives a default RTT greater than 100 ms. 
 *
 * If q_weight=-2, set it to a reasonable value of 1-exp(-10/C).
 */
  if (m_qW == 0.0)
    {
      m_qW = 1.0 - exp(-1.0 / m_ptc);
    }
  else if (m_qW == -1.0)
    {
      double rtt = 3.0 * (m_linkDelay.GetSeconds () + 1.0 / m_ptc);

      if (rtt < 0.1)
        {
          rtt = 0.1;
        }
      m_qW = 1.0 - exp(-1.0 / (10 * rtt * m_ptc));
    }
  else if (m_qW == -2.0)
    {
      m_qW = 1.0 - exp(-10.0 / m_ptc);
    }

  if (m_minPacketsTh == 0)
    {
      m_minPacketsTh = 5.0;

      // TODO: implement adaptative RED
    }

  // std::cout << "m_delay " << m_linkDelay.GetSeconds () << "; m_wait " << m_wait << "; m_qW " << m_qW << "; m_ptc " << m_ptc << "; m_minTh " << m_minTh << "; m_maxTh " << m_maxTh << "; m_gentle " << m_gentle << "; th_diff" << th_diff << "; lInternm " << m_lInterm << "; va " << m_vA <<  "; cur_max_p " << m_curMaxP << "; v_b " << m_vB <<  "; m_vC " << m_vC << "; m_vD " <<  m_vD << std::endl;
}

RedQueue::RedQueue ()
  : Queue (),
    m_packets (),
    m_bytesInQueue (0),
    m_redParams(false)
{

}

RedQueue::~RedQueue ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
RedQueue::SetMode (enum Mode mode)
{
  NS_LOG_FUNCTION (mode);
  m_mode = mode;
}

RedQueue::Mode
RedQueue::GetMode (void)
{
  return m_mode;
}

// compute the average queue size
double
RedQueue::Estimator (uint32_t nQueued, uint32_t m, double qAvg, double qW)
{
  double newAve;

  newAve = qAvg;
  while (--m >= 1)
    {
      newAve *= 1.0 - qW;
    }
  newAve *= 1.0 - qW;
  newAve += qW * nQueued;

  // TODO: implement adaptative RED
  // new param: m_adaptative; m_fengAdaptative
  /*
  Time now = Simulator::Now ();
  if (m_adaptative == 1)
    {
      if (m_fengAdaptative == 1)
        {
          UpdateMaxPFeng(newAve);
        }
      else if ()
    }
  */

  return newAve;
}

double 
RedQueue::ModifyP(double p, uint32_t count, uint32_t countBytes,
                  uint32_t meanPktSize, bool wait, uint32_t size)
{
  double count1 = (double) count;

  if (GetMode () == BYTES)
    {
      count1 = (double) (countBytes / meanPktSize);
    }

  if (wait)
    {
      if (count1 * p < 1.0)
        {
          p = 0.0;
        }
      else if (count1 * p < 2.0)
        {
          p /= (2.0 - count1 * p);
        }
      else
        {
          p = 1.0;
        }
    }
  else
    {
      if (count1 * p < 1.0)
        {
          p /= (1.0 - count1 * p);
        }
      else
        {
          p = 1.0;
        }
    }

  if ((GetMode () == BYTES) && (p < 1.0))
    {
      p = (p * size) / meanPktSize;
      //p = p * (size / meanPktSize);
    }

  if (p > 1.0)
    {
      p = 1.0;
    }

  return p;
}

double
RedQueue::CalculatePNew (double qAvg, double maxTh, bool gentle, double vA,
                         double vB, double vC, double vD, double maxP)
{
  double p;

  if (gentle && qAvg >= maxTh)
    {
      // p ranges from maxP to 1 as the average queue
      // size ranges from maxTh to twice maxTh
      p = vC * qAvg + vD;
    }
  else if (!gentle && qAvg >= maxTh)
    {
      // OLD: p continues to range linearly above max_p as
      // the average queue size ranges above th_max.
      // NEW: p is set to 1.0 
      p = 1.0;
    }
  else
    {
      // p ranges from 0 to max_p as the average queue
      // size ranges from th_min to th_max 
      p = vA * qAvg + vB;
      // p = (qAvg - m_minTh) / (maxTh - minTh)
      p *= maxP;
    }

  if (p > 1.0)
    {
      p = 1.0;
    }

  return p;
}

uint32_t
RedQueue::DropEarly (Ptr<Packet> p, uint32_t qSize)
{
  
  m_vProb1 = CalculatePNew(m_qAvg, m_maxTh, m_gentle, m_vA, m_vB, m_vC, m_vD, m_curMaxP);
  m_vProb = ModifyP(m_vProb1, m_count, m_countBytes, m_meanPktSize, m_wait, p->GetSize ());


  // drop probability is computed, pick random number and act
  if (m_cautious == 1)
    {
      // Don't drop/mark if the instantaneous queue is much
      //  below the average.
      // For experimental purposes only.
      // pkts: the number of packets arriving in 50 ms
      double pkts = m_ptc * 0.05;
      double fraction = pow ((1 - m_qW), pkts);
      // double fraction = 0.9;      
      if ((double) qSize < fraction * m_qAvg)
        {
          // queue could have been empty for 0.05 seconds
          // printf("fraction: %5.2f\n", fraction); don't worry :P
          return 0;
        }
    }

  UniformVariable uv;
  double u = uv.GetValue ();

  if (m_cautious == 2)
    {
      // Decrease the drop probability if the instantaneous
      //   queue is much below the average.
      // For experimental purposes only.
      // pkts: the number of packets arriving in 50 ms
      double pkts = m_ptc * 0.05;
      double fraction = pow ((1 - m_qW), pkts);
      // double fraction = 0.9;
      double ratio = qSize / (fraction * m_qAvg);

      if (ratio < 1.0)
        {
          u *= 1.0 / ratio;
        }
    }

  if (u <= m_vProb)
    {
      // DROP or MARK
      m_count = 0;
      m_countBytes = 0;
      /* TODO: implement set bit to mark */

      return 1; // drop
    }

  return 0; // no drop/mark
}

uint32_t
RedQueue::GetQueueSize (void)
{
  if (GetMode () == BYTES)
    {
      return m_bytesInQueue;
    }
  else // packets
    {
      return m_packets.size ();
    }
}

bool
RedQueue::DoEnqueue (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);

  if (! m_redParams )
    {
      InitializeParams ();
      m_redParams = true;
    }

  uint32_t nQueued;

  if (GetMode () == BYTES)
    {
      /*
      if (m_bytesInQueue + p->GetSize () >= m_maxBytes)
        {
          NS_LOG_LOGIC ("Queue full (packet would exceed max bytes) -- droppping pkt");
          Drop (p);
          return false;
        }
      */
      NS_LOG_DEBUG("Enqueue in bytes mode");
      nQueued = m_bytesInQueue;
    }
  else if (GetMode () == PACKETS)
    {
      /*
      if (m_packets.size () >= m_maxPackets)
        {
          NS_LOG_LOGIC ("Queue full (at max packets) -- droppping pkt");
          Drop (p);
          return false;
        }
      */
      NS_LOG_DEBUG("Enqueue in packets mode");
      nQueued = m_packets.size ();
    }

  uint32_t m = 0;

  if (m_idle == 1)
    {
      Time now = Simulator::Now ();

      if (m_cautious == 3)
        {
          double ptc = m_ptc * m_meanPktSize / m_idlePktSize;
          m = uint32_t(ptc * (now - m_idleTime).GetSeconds());
        }
      else
        {
          m = uint32_t(m_ptc * (now - m_idleTime).GetSeconds());
        }

      m_idle = 0;
    }

  m_qAvg = Estimator (nQueued, m + 1, m_qAvg, m_qW);

  NS_LOG_DEBUG ("\t bytesInQueue  " << m_bytesInQueue << "\tQavg " << m_qAvg);
  NS_LOG_DEBUG ("\t packetsInQueue  " << m_packets.size () << "\tQavg " << m_qAvg);

  m_count++;
  m_countBytes += p->GetSize ();

  uint32_t dropType = DTYPE_NONE;
  if (m_qAvg >= m_minTh && nQueued > 1)
    {
      if ((! m_gentle && m_qAvg >= m_maxTh) ||
          (m_gentle && m_qAvg >= 2 * m_maxTh))
          {
            NS_LOG_DEBUG ("adding DROP FORCED MARK");
            dropType = DTYPE_FORCED;
          }
      else if (m_old == 0)
        {
          /* 
           * The average queue size has just crossed the
           * threshold from below to above "minthresh", or
           * from above "minthresh" with an empty queue to
           * above "minthresh" with a nonempty queue.
           */
          m_count = 1;
          m_countBytes = p->GetSize();
          m_old = 1;
        }
      else if (DropEarly(p, nQueued))
        {
          drop_early_test++;
          dropType = DTYPE_UNFORCED;
        }
    }
  else 
    {
      /* No packets are being dropped.  */
      m_vProb = 0.0;
      m_old = 0;
    }

  if (nQueued >= m_queueLimit)
    {
      NS_LOG_LOGIC ("Queue full -- droppping pkt");
      // see if we've exceeded the queue size
      dropType = DTYPE_FORCED;
    }

  if (dropType == DTYPE_UNFORCED)
    {
      NS_LOG_DEBUG ("\t Dropping due to Prob Mark " << m_qAvg);
      m_stats.unforcedDrop++;
      Drop (p);
      return false;
    }
  else if (dropType == DTYPE_FORCED)
    {
      NS_LOG_DEBUG ("\t Dropping due to Hard Mark " << m_qAvg);
      m_stats.forcedDrop++;
      Drop (p);
      if (m_ns1Compat)
        {
          m_count = 0;
          m_countBytes = 0;
        }
      return false;
    }

  m_bytesInQueue += p->GetSize ();
  m_packets.push_back (p);

  NS_LOG_LOGIC ("Number packets " << m_packets.size ());
  NS_LOG_LOGIC ("Number bytes " << m_bytesInQueue);
  
  return true;
}

Ptr<Packet>
RedQueue::DoDequeue (void)
{
  NS_LOG_FUNCTION (this);

  if (m_packets.empty ())
    {
      NS_LOG_LOGIC ("Queue empty");
      m_idle = 1;
      m_idleTime = Simulator::Now ();

      return 0;
    }
  else
    {
      m_idle = 0;
      Ptr<Packet> p = m_packets.front ();
      m_packets.pop_front ();
      m_bytesInQueue -= p->GetSize ();

      NS_LOG_LOGIC ("Popped " << p);

      NS_LOG_LOGIC ("Number packets " << m_packets.size ());
      NS_LOG_LOGIC ("Number bytes " << m_bytesInQueue);

      return p;
    }
}

Ptr<const Packet>
RedQueue::DoPeek (void) const
{
  NS_LOG_FUNCTION (this);

  if (m_packets.empty ())
    {
      NS_LOG_LOGIC ("Queue empty");
      return 0;
    }

  Ptr<Packet> p = m_packets.front ();

  NS_LOG_LOGIC ("Number packets " << m_packets.size ());
  NS_LOG_LOGIC ("Number bytes " << m_bytesInQueue);

  return p;
}

} // namespace ns3
