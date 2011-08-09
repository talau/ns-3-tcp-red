/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 * Authors: Marcos Talau <talau@users.sourceforge.net>
 *          Duy Nguyen <duy@soe.ucsc.edu>
 *
 */

/**
 * These validation tests are detailed in http://icir.org/floyd/papers/redsims.ps
 */

/** Network topology
 *
 *    10Mb/s, 2ms                            10Mb/s, 4ms
 * n0--------------|                    |---------------n4
 *                 |   1.5Mbps/s, 20ms  |
 *                 n2------------------n3
 *    10Mb/s, 3ms  |                    |    10Mb/s, 5ms
 * n1--------------|                    |---------------n5
 *
 *
 */

#include "ns3/core-module.h"
#include "ns3/simulator-module.h"
#include "ns3/node-module.h"
#include "ns3/helper-module.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/red-queue.h"
#include "ns3/uinteger.h"

#define FILE_PLOT_QUEUE "/tmp/red-queue.plotme"
#define FILE_PLOT_QUEUE_AVG "/tmp/red-queue_avg.plotme"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("RedEx1");

uint32_t checkTimes;
double avgQueueSize;

void
CheckQueueSize (Ptr<Queue> queue)
{
  uint32_t qSize = StaticCast<RedQueue> (queue)->GetQueueSize ();

  avgQueueSize += qSize;
  checkTimes++;

  // check queue size every 1/100 of a second
  Simulator::Schedule (Seconds (0.01), &CheckQueueSize, queue);

  std::ofstream fPlotQueue (FILE_PLOT_QUEUE, std::ios::out|std::ios::app);
  fPlotQueue << Simulator::Now ().GetSeconds () << " " << qSize << std::endl;
  fPlotQueue.close();

  std::ofstream fPlotQueueAvg (FILE_PLOT_QUEUE_AVG, std::ios::out|std::ios::app);
  fPlotQueueAvg << Simulator::Now ().GetSeconds () << " " << avgQueueSize / checkTimes << std::endl;
  fPlotQueueAvg.close();
}

int
main (int argc, char *argv[])
{
  // LogComponentEnable ("RedQueue", LOG_LEVEL_ALL);
  // LogComponentEnable ("TcpNewReno", LOG_LEVEL_INFO);

  bool flowMonitor = true;
  bool m_writeResults = true;
  uint32_t redTest = 0;

  // The times
  double global_start_time;
  double global_stop_time;
  double sink_start_time;
  double sink_stop_time;
  double client_start_time;
  double client_stop_time;

  std::string redDataRate = "1.5Mbps";
  std::string redLinkDelay = "20ms";

  checkTimes = 0;
  avgQueueSize = 0;

  global_start_time = 0.0;
  global_stop_time = 15; 
  sink_start_time = global_start_time;
  sink_stop_time = global_stop_time + 3.0;
  client_start_time = sink_start_time + 0.2;
  client_stop_time = global_stop_time - 2.0;

  // Configuration and command line parameter parsing
  CommandLine cmd;
  cmd.AddValue ("testnumber", "Run test 1 or 3", redTest);
  cmd.Parse (argc, argv);

  if ((redTest == 0) || ((redTest != 1) && (redTest != 3)))
    {
      std::cout << "Please, use arg --testnumber=1/3" << std::endl;
      exit(1);
    }

  NS_LOG_INFO ("Create nodes");
  NodeContainer c;
  c.Create (6);
  Names::Add ( "N0", c.Get (0));
  Names::Add ( "N1", c.Get (1));
  Names::Add ( "N2", c.Get (2));
  Names::Add ( "N3", c.Get (3));
  Names::Add ( "N4", c.Get (4));
  Names::Add ( "N5", c.Get (5));
  NodeContainer n0n2 = NodeContainer (c.Get (0), c.Get (2));
  NodeContainer n1n2 = NodeContainer (c.Get (1), c.Get (2));
  NodeContainer n2n3 = NodeContainer (c.Get (2), c.Get (3));
  NodeContainer n3n4 = NodeContainer (c.Get (3), c.Get (4));
  NodeContainer n3n5 = NodeContainer (c.Get (3), c.Get (5));

  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpReno"));
  // 42 = headers size
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1000 - 42));
  Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (1));
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (false));

  // RED params
  Config::SetDefault ("ns3::RedQueue::Mode", EnumValue(RedQueue::PACKETS));
  Config::SetDefault ("ns3::RedQueue::MeanPktSize", UintegerValue (500));
  Config::SetDefault ("ns3::RedQueue::Wait", BooleanValue (true));
  Config::SetDefault ("ns3::RedQueue::Gentle", BooleanValue (true));
  Config::SetDefault ("ns3::RedQueue::m_minTh", DoubleValue (5));
  Config::SetDefault ("ns3::RedQueue::QueueLimit", UintegerValue (25));
  Config::SetDefault ("ns3::RedQueue::LinkBandwidth", StringValue(redDataRate));
  Config::SetDefault ("ns3::RedQueue::LinkDelay", StringValue(redLinkDelay));

  if (redTest == 1)
    {
      Config::SetDefault ("ns3::RedQueue::m_maxTh", DoubleValue (15));
      Config::SetDefault ("ns3::RedQueue::m_qW", DoubleValue (0.002));
    }
  else // test 3
    {
      Config::SetDefault ("ns3::RedQueue::m_maxTh", DoubleValue (10));
      Config::SetDefault ("ns3::RedQueue::m_qW", DoubleValue (0.003));
    }

  // fix the TCP window size
  uint16_t wnd = 15000;
  GlobalValue::Bind ("GlobalFixedTcpWindowSize", IntegerValue (wnd));

  InternetStackHelper internet;
  internet.Install (c);

  NS_LOG_INFO ("Create channels");
  PointToPointHelper p2p;

  p2p.SetQueue ("ns3::DropTailQueue");
  p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
  NetDeviceContainer devn0n2 = p2p.Install (n0n2);

  p2p.SetQueue ("ns3::DropTailQueue");
  p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("3ms"));
  NetDeviceContainer devn1n2 = p2p.Install (n1n2);

  p2p.SetQueue("ns3::RedQueue"); // yeah, only backbone link have special queue
  p2p.SetDeviceAttribute ("DataRate", StringValue (redDataRate));
  p2p.SetChannelAttribute ("Delay", StringValue (redLinkDelay));
  NetDeviceContainer devn2n3 = p2p.Install (n2n3);

  p2p.SetQueue ("ns3::DropTailQueue");
  p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("4ms"));
  NetDeviceContainer devn3n4 = p2p.Install (n3n4);

  p2p.SetQueue ("ns3::DropTailQueue");
  p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("5ms"));
  NetDeviceContainer devn3n5 = p2p.Install (n3n5);

  NS_LOG_INFO ("Assign IP Addresses");

  Ipv4AddressHelper ipv4;

  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i0i2 = ipv4.Assign (devn0n2);

  ipv4.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer i1i2 = ipv4.Assign (devn1n2);

  ipv4.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer i2i3 = ipv4.Assign (devn2n3);

  ipv4.SetBase ("10.1.4.0", "255.255.255.0");
  Ipv4InterfaceContainer i3i4 = ipv4.Assign (devn3n4);

  ipv4.SetBase ("10.1.5.0", "255.255.255.0");
  Ipv4InterfaceContainer i3i5 = ipv4.Assign (devn3n5);

  ///> set up the routing
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // SINK is in the right side
  uint16_t port = 50000;
  Address sinkLocalAddress(InetSocketAddress (Ipv4Address::GetAny (), port));
  PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
  ApplicationContainer sinkApp = sinkHelper.Install (n3n4.Get(1));
  sinkApp.Start (Seconds (sink_start_time));
  sinkApp.Stop (Seconds (sink_stop_time));

  // Connection one
  // Clientes are in left side
  // Create the OnOff applications to send TCP to the server
  // onoffhelper is a client that send data to TCP destination
  OnOffHelper clientHelper1 ("ns3::TcpSocketFactory", Address ());
  clientHelper1.SetAttribute 
      ("OnTime", RandomVariableValue (ConstantVariable (1)));
  clientHelper1.SetAttribute 
      ("OffTime", RandomVariableValue (ConstantVariable (0)));
  clientHelper1.SetAttribute 
      ("DataRate", DataRateValue (DataRate ("10Mb/s")));
  clientHelper1.SetAttribute 
      ("PacketSize", UintegerValue (1000));

  ApplicationContainer clientApps1;
  AddressValue remoteAddress
    (InetSocketAddress (i3i4.GetAddress (1), port));
  clientHelper1.SetAttribute ("Remote", remoteAddress);
  clientApps1.Add(clientHelper1.Install (n0n2.Get(0)));
  clientApps1.Start (Seconds (client_start_time));
  clientApps1.Stop (Seconds (client_stop_time));


  // Connection two
  OnOffHelper clientHelper2 ("ns3::TcpSocketFactory", Address ());
  clientHelper2.SetAttribute 
      ("OnTime", RandomVariableValue (ConstantVariable (1)));
  clientHelper2.SetAttribute 
      ("OffTime", RandomVariableValue (ConstantVariable (0)));
  clientHelper2.SetAttribute 
      ("DataRate", DataRateValue (DataRate ("10Mb/s")));
  clientHelper2.SetAttribute 
      ("PacketSize", UintegerValue (1000));

  ApplicationContainer clientApps2;
  clientHelper2.SetAttribute ("Remote", remoteAddress);
  clientApps2.Add(clientHelper2.Install (n1n2.Get(0)));
  clientApps2.Start (Seconds (3.0));
  clientApps2.Stop (Seconds (client_stop_time));


  if (m_writeResults)
    {
      PointToPointHelper ptp;
      ptp.EnablePcapAll ("red");
    }

  Ptr<FlowMonitor> flowmon;

  if (flowMonitor)
    {
      FlowMonitorHelper flowmonHelper;
      flowmon = flowmonHelper.InstallAll ();
    }

  remove(FILE_PLOT_QUEUE);
  remove(FILE_PLOT_QUEUE_AVG);
  Ptr<PointToPointNetDevice> nd = StaticCast<PointToPointNetDevice> (devn2n3.Get (0));
  Ptr<Queue> queue = nd->GetQueue ();
  Simulator::ScheduleNow (&CheckQueueSize, queue);

  Simulator::Stop (Seconds (sink_stop_time));
  Simulator::Run ();

  if (flowMonitor)
    {
      flowmon->SerializeToXmlFile ("red.flowmon", false, false);
    }

  Simulator::Destroy ();

  return 0;
}
