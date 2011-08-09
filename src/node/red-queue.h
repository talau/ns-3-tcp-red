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

#ifndef RED_QUEUE_H
#define RED_QUEUE_H

#include <queue>
#include "ns3/packet.h"
#include "ns3/queue.h"
#include "ns3/nstime.h"
#include "ns3/random-variable.h"
#include "ns3/boolean.h"
#include "ns3/data-rate.h"
#include "ns3/nstime.h"

#define	DTYPE_NONE	0	/* ok, no drop */
#define	DTYPE_FORCED	1	/* a "forced" drop */
#define	DTYPE_UNFORCED	2	/* an "unforced" (random) drop */

namespace ns3 {

class TraceContainer;

/**
 * \ingroup queue
 *
 * \brief A RED packet queue
 */
class RedQueue : public Queue
{
public:
  typedef std::vector< uint32_t> Uint32tVector;

  struct Stats
  {
    uint32_t unforcedDrop;  ///< Early probability drops
    uint32_t forcedDrop;  ///< Forced drops, qavg > max threshold
    uint32_t pdrop;  ///< Drops due to queue limits
    uint32_t other;  ///< Drops due to drop calls
    uint32_t backlog;
  };

  static TypeId GetTypeId (void);
  /**
   * \brief RedQueue Constructor
   *
   */
  RedQueue ();

  virtual ~RedQueue ();

  /**
   * Enumeration of the modes supported in the class.
   *
   */
  enum Mode
  {
    ILLEGAL,     /**< Mode not set */
    PACKETS,     /**< Use number of packets for maximum queue size */
    BYTES,       /**< Use number of bytes for maximum queue size */
  };

  /**
   * Set the operating mode of this device.
   *
   * \param mode The operating mode of this device.
   *
   */
  void SetMode (RedQueue::Mode mode);

  /**
   * Get the encapsulation mode of this device.
   *
   * \returns The encapsulation mode of this device.
   */
  RedQueue::Mode  GetMode (void);

  uint32_t GetQueueSize (void);
  void SetQueueLimit(uint32_t lim);
  void SetTh(double min, double max);

private:
  void SetParams (uint32_t minTh, uint32_t maxTh,
                  uint32_t wLog, uint32_t pLog, uint64_t scellLog );

  void InitializeParams (void);
  double Estimator (uint32_t nQueued, uint32_t m, double qAvg, double qW);
  double ModifyP(double p, uint32_t count, uint32_t countBytes,
                 uint32_t meanPktSize, bool wait, uint32_t size);
  double CalculatePNew (double qAvg, double maxTh, bool gentle, double vA,
                        double vB, double vC, double vD, double maxP);
  uint32_t DropEarly (Ptr<Packet> p, uint32_t qSize);

  virtual bool DoEnqueue (Ptr<Packet> p);
  virtual Ptr<Packet> DoDequeue (void);
  virtual Ptr<const Packet> DoPeek (void) const;

  std::list<Ptr<Packet> > m_packets;

  uint32_t m_bytesInQueue;
  bool m_redParams;
  Stats m_stats;

  //** variables supplied by user
  // bytes or packets?
  Mode m_mode;
  /* avg pkt size */
  uint32_t m_meanPktSize;
  /* avg pkt size used during idle times */
  uint32_t m_idlePktSize;
  /* true for waiting between dropped packets */
  bool m_wait;
  /* true to increases dropping prob. slowly *
   * when ave queue exceeds maxthresh. */
  bool m_gentle;
  double m_minTh; ///> Min avg length threshold (bytes), should be >= 2*minTh
  double m_maxTh; ///> Max avg length threshold (bytes), should be >= 2*minTh
  // queue limit in bytes / packets
  uint32_t m_queueLimit;
  /* queue weight given to cur q size sample */
  double m_qW;
  // The max probability of dropping a packet
  double m_lInterm;
  // ns-1 compatibility
  bool m_ns1Compat;
  // link bandwidth
  DataRate m_linkBandwidth;
  // link delay
  Time m_linkDelay;

  //** variables maintained by RED
  /* prob. of packet drop before "count". */
  double m_vProb1;
  double m_vA;		/* v_prob = v_a * v_ave + v_b */
  double m_vB;
  double m_vC;		/* used for "gentle" mode */
  double m_vD;		/* used for "gentle" mode */
  // current max_p
  double m_curMaxP;
  /* prob. of packet drop */
  double m_vProb;
  /* # of bytes since last drop */
  uint32_t m_countBytes;
  /* 0 when average queue first exceeds thresh */
  uint32_t m_old;
  uint32_t m_idle; // 0/1 idle status
  /* packet time constant in packets/second */
  double m_ptc;
  double m_qAvg; ///> average q length; is double..
  uint32_t m_count;  ///> number of packets since last random number generation
  /* 0 for default RED */
  /* 1 for not dropping/marking when the */
  /*  instantaneous queue is much below the */
  /*  average */
  uint32_t m_cautious;
  double m_minPacketsTh; /* always maintained in packets */
  double m_maxPacketsTh; /* always maintained in packets */
  Time m_idleTime; ///> start of current idle period

  //remover
  uint32_t drop_early_test;
};

}; // namespace ns3

#endif /* RED_QUEUE_H */
