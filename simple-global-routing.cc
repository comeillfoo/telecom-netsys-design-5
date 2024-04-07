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
 */

//
// Network topology
//
//      300 Mb/s, 2ms
//  n0----------------n1
//
// - all links are point-to-point links with indicated one-way BW/delay
// - CBR/UDP flows from n0 to n3, and from n3 to n1
// - UDP packet size of 210 bytes, with per-packet interval 0.00375 sec.
//   (i.e., DataRate of 448,000 bps)
// - DropTail queues
// - Tracing of queues and packet receptions to file "simple-global-routing.tr"

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

#include "ns3/bulk-send-helper.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SimpleGlobalRoutingExample");

int
main(int argc, char* argv[])
{
    // Users may find it convenient to turn on explicit debugging
    // for selected modules; the below lines suggest how to do this
#if 1
  LogComponentEnable ("SimpleGlobalRoutingExample", LOG_LEVEL_INFO);
#endif

    // Set up some default values for the OnOffApplications' simulation.  Use the
    Config::SetDefault("ns3::OnOffApplication::PacketSize", UintegerValue(210));
    Config::SetDefault("ns3::OnOffApplication::DataRate", StringValue("1Mb/s"));

    // DefaultValue::Bind ("DropTailQueue::m_maxPackets", 30);

    // Allow the user to override any of the defaults and the above
    // DefaultValue::Bind ()s at run-time, via command-line arguments
    CommandLine cmd(__FILE__);
    bool enableFlowMonitor = false;
    cmd.AddValue("EnableMonitor", "Enable Flow Monitor", enableFlowMonitor);

    bool rttOrBandwidthGenerator = false;
    cmd.AddValue("RTTorBandwidthGenerator", "Choose between measuring RTT or Bandwidth",
		 rttOrBandwidthGenerator);
    cmd.Parse(argc, argv);

    // Here, we will explicitly create two nodes.  In more sophisticated
    // topologies, we could configure a node factory.
    NS_LOG_INFO("Create nodes.");
    NodeContainer c;
    c.Create(2);

    InternetStackHelper internet;
    internet.Install(c);

    // We create the channels first without any IP addressing information
    NS_LOG_INFO("Create channels.");
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("300Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));
    NetDeviceContainer d0d1 = p2p.Install(c);

    // Later, we add IP addresses.
    NS_LOG_INFO("Assign IP Addresses.");
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("192.168.211.0", "255.255.255.0");
    Ipv4InterfaceContainer i0i1 = ipv4.Assign(d0d1);

    // Create router nodes, initialize routing database and set up the routing
    // tables in the nodes.
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();


    const uint16_t port = 5201;
    ApplicationContainer apps;
    if (rttOrBandwidthGenerator) {
	// Generate slowed TCP for RTT
    	// Create the OnOff application to send TCP segments at a rate of 1Mb/s
    	NS_LOG_INFO("Create Applications.");
    	OnOffHelper onoff("ns3::TcpSocketFactory",
        	          Address(InetSocketAddress(i0i1.GetAddress(1), port)));
    	onoff.SetConstantRate(DataRate("1Mb/s"));
    	apps = onoff.Install(c.Get(0));
    	apps.Start(Seconds(1.0));
    	apps.Stop(Seconds(10.0));

    	// Create a packet sink to receive these packets
    	PacketSinkHelper sink("ns3::TcpSocketFactory",
        		      Address(InetSocketAddress(Ipv4Address::GetAny(), port)));
    	apps = sink.Install(c.Get(1));
    	apps.Start(Seconds(1.0));
    	apps.Stop(Seconds(10.0));

    } else {
	// Generate TCP/UDP for maximum bandwidth
	BulkSendHelper bulksend("ns3::TcpSocketFactory",
				Address(InetSocketAddress(i0i1.GetAddress(1), port)));
	// Set the amount of data to send in bytes. Zero is unlimited
	const uint32_t maxBytes = 30 * 1024 * 1024;
	bulksend.SetAttribute("MaxBytes", UintegerValue(maxBytes));

	apps = bulksend.Install(c.Get(0));
	apps.Start(Seconds(1.0));
	apps.Stop(Seconds(10.0));

	PacketSinkHelper sink("ns3::TcpSocketFactory",
			      Address(InetSocketAddress(Ipv4Address::GetAny(), port)));
	apps = sink.Install(c.Get(1));
	apps.Start(Seconds(1.0));
	apps.Stop(Seconds(10.0));
    }
    
    const char* pcap_files_names = rttOrBandwidthGenerator ? "rtt-simple-global-routing" : "speed-simple-global-routing";

    AsciiTraceHelper ascii;
    p2p.EnableAsciiAll(ascii.CreateFileStream("simple-global-routing.tr"));
    p2p.EnablePcapAll(pcap_files_names);

    // Flow Monitor
    FlowMonitorHelper flowmonHelper;
    if (enableFlowMonitor)
    {
        flowmonHelper.InstallAll();
    }

    NS_LOG_INFO("Run Simulation.");
    Simulator::Stop(Seconds(11));
    Simulator::Run();
    NS_LOG_INFO("Done.");

    if (enableFlowMonitor)
    {
	NS_LOG_INFO("Enabled Flow Monitor");
        flowmonHelper.SerializeToXmlFile("simple-global-routing.flowmon", false, false);
    }

    Simulator::Destroy();

    Ptr<PacketSink> sink1 = DynamicCast<PacketSink>(apps.Get(0));
    std::cout << "Total Bytes Received: " << sink1->GetTotalRx() << std::endl;
    return 0;
}
