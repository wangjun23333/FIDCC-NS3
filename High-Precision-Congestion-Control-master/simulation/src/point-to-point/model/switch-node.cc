#include "ns3/simulator.h"
#include "ns3/ipv4.h"
#include "ns3/packet.h"
#include "ns3/ipv4-header.h"
#include "ns3/flow-id-tag.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "switch-node.h"
//#include "enc-net-device.h"
#include "qbb-net-device.h"
#include "ppp-header.h"
#include "ns3/int-header-niux.h"
//#include "../../network/utils/int-header-niux.h"
#include <cmath>

namespace ns3 {

TypeId SwitchNode::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SwitchNode")
    .SetParent<Node> ()
    .AddConstructor<SwitchNode> ()
	.AddAttribute("EcnEnabled",
			"Enable ECN marking.",
			BooleanValue(false),
			MakeBooleanAccessor(&SwitchNode::m_ecnEnabled),
			MakeBooleanChecker())
	.AddAttribute("CcMode",
			"CC mode.",
			UintegerValue(0),
			MakeUintegerAccessor(&SwitchNode::m_ccMode),
			MakeUintegerChecker<uint32_t>())
	.AddAttribute("AckHighPrio",
			"Set high priority for ACK/NACK or not",
			UintegerValue(0),
			MakeUintegerAccessor(&SwitchNode::m_ackHighPrio),
			MakeUintegerChecker<uint32_t>())
	.AddAttribute("MaxRtt",
			"Max Rtt of the network",
			UintegerValue(9000),
			MakeUintegerAccessor(&SwitchNode::m_maxRtt),
			MakeUintegerChecker<uint32_t>())
  ;
  return tid;
}

SwitchNode::SwitchNode() {

    //id = 0;
	m_node_type = 1;

    m_mmu = CreateObject<SwitchMmu>();
	for (uint32_t i = 0; i < pCnt; i++)
		for (uint32_t j = 0; j < pCnt; j++)
			for (uint32_t k = 0; k < qCnt; k++)
				m_bytes[i][j][k] = 0;
	for (uint32_t i = 0; i < pCnt; i++)
		m_txBytes[i] = 0;
	for (uint32_t i = 0; i < pCnt; i++)
		m_lastPktSize[i] = m_lastPktTs[i] = 0;
	for (uint32_t i = 0; i < pCnt; i++)
		m_u[i] = 0;
	for (uint32_t i = 0; i < pCnt; i++)
		max_rate[i] = 0;
	for (uint32_t i = 0; i < pCnt; i++)
		m_rate[i] = 0;
	for (uint32_t i = 0; i < pCnt; i++)
		m_cnt[i] = 0;
}

void SwitchNode::SetMaxRate(uint8_t _port, uint64_t _max_rate) {
	max_rate[_port] = _max_rate;
	Ptr<QbbNetDevice> device = DynamicCast<QbbNetDevice>(m_devices[_port]);
	device->SetDataRate(_max_rate);
}

int SwitchNode::GetOutDev(Ptr<const Packet> p, MyCustomHeader &ch){
	// look up entries
	auto entry = m_rtTable.find(ch.dip);

	// no matching entry
	if (entry == m_rtTable.end())
		return -1;

	// entry found
	int nexthops = entry->second;

	// pick one next hop based on hash
	union {
		uint8_t u8[4+4+2+2];
		uint32_t u32[3];
	} buf;
	buf.u32[0] = ch.sip;
	buf.u32[1] = ch.dip;
	if (ch.l3Prot == 0x6)
		buf.u32[2] = ch.tcp.sport | ((uint32_t)ch.tcp.dport << 16);
	else if (ch.l3Prot == 0x11)
		buf.u32[2] = ch.udp.sport | ((uint32_t)ch.udp.dport << 16);
	else if (ch.l3Prot == 0xFC || ch.l3Prot == 0xFD)
		buf.u32[2] = ch.ack.sport | ((uint32_t)ch.ack.dport << 16);

	return nexthops;
}

void SwitchNode::CheckAndSendPfc(uint32_t inDev, uint32_t qIndex){
	Ptr<QbbNetDevice> device = DynamicCast<QbbNetDevice>(m_devices[inDev]);
	if (m_mmu->CheckShouldPause(inDev, qIndex)){
		m_mmu->SetPause(inDev, qIndex);
	}
}
void SwitchNode::CheckAndSendResume(uint32_t inDev, uint32_t qIndex){
	Ptr<QbbNetDevice> device = DynamicCast<QbbNetDevice>(m_devices[inDev]);
	if (m_mmu->CheckShouldResume(inDev, qIndex)){
		m_mmu->SetResume(inDev, qIndex);
	}
}

void SwitchNode::SendToDev(Ptr<Packet>p, MyCustomHeader &ch){
	int idx = GetOutDev(p, ch);
	if (idx >= 0){
		NS_ASSERT_MSG(m_devices[idx]->IsLinkUp(), "The routing table look up should return link that is up");

		// determine the qIndex
		uint32_t qIndex;
		/*if (ch.l3Prot == 0xFF || ch.l3Prot == 0xFE || (m_ackHighPrio && (ch.l3Prot == 0xFD || ch.l3Prot == 0xFC))){  //QCN or PFC or NACK, go highest priority
			qIndex = 0;
		}else{
			qIndex = (ch.l3Prot == 0x06 ? 1 : 1); // if TCP, put to queue 1
		}*/
		qIndex = 1;

		// admission control
		FlowIdTag t;
		p->PeekPacketTag(t);
		uint32_t inDev = t.GetFlowId();
		if (qIndex != 0){ //not highest priority
			if (m_mmu->CheckIngressAdmission(inDev, qIndex, p->GetSize()) && m_mmu->CheckEgressAdmission(idx, qIndex, p->GetSize())){			// Admission control
				m_mmu->UpdateIngressAdmission(inDev, qIndex, p->GetSize());
				m_mmu->UpdateEgressAdmission(idx, qIndex, p->GetSize());
			}else{
				return; // Drop
			}
			// 不需要暂停，丢包即可
			// CheckAndSendPfc(inDev, qIndex);
		}
		m_bytes[inDev][idx][qIndex] += p->GetSize();
		m_devices[idx]->SwitchSend(qIndex, p, ch);
	}else
		return; // Drop
}

void SwitchNode::AddTableEntry(Ipv4Address &dstAddr, uint32_t intf_idx){
	uint32_t dip = dstAddr.Get();
	m_rtTable[dip] = intf_idx;
}

void SwitchNode::ClearTable(){
	m_rtTable.clear();
}

// This function can only be called in switch mode
bool SwitchNode::SwitchReceiveFromDevice(Ptr<NetDevice> device, Ptr<Packet> packet, MyCustomHeader &ch){
	//std::cout << "node:" << m_id<< " sip:" << ch.sip << "  dip:"<< ch.dip << std::endl;
	SendToDev(packet, ch);
	return true;
}

void SwitchNode::SwitchNotifyDequeue(uint32_t ifIndex, uint32_t qIndex, Ptr<Packet> p){
	FlowIdTag t;
	p->PeekPacketTag(t);
	if (qIndex != 0){
		uint32_t inDev = t.GetFlowId();
		m_mmu->RemoveFromIngressAdmission(inDev, qIndex, p->GetSize());
		m_mmu->RemoveFromEgressAdmission(ifIndex, qIndex, p->GetSize());
		m_bytes[inDev][ifIndex][qIndex] -= p->GetSize();
		/*if (m_ecnEnabled){
			bool egressCongested = m_mmu->ShouldSendCN(ifIndex, qIndex);
			if (egressCongested){
				PppHeader ppp;
				Ipv4Header h;
				p->RemoveHeader(ppp);
				p->RemoveHeader(h);
				h.SetEcn((Ipv4Header::EcnType)0x03);
				p->AddHeader(h);
				p->AddHeader(ppp);
			}
		}*/
		// 不需要暂停，丢包即可
		// CheckAndSendPfc(inDev, qIndex);
		// CheckAndSendResume(inDev, qIndex);
	}

	m_txBytes[ifIndex] += p->GetSize();
	m_lastPktSize[ifIndex] = p->GetSize();
	++m_cnt[ifIndex];

	// 用于计算实时速率
	uint64_t now_ts = Simulator::Now().GetTimeStep() +  (uint64_t)(p->GetSize())*8*1000000000/max_rate[ifIndex];
	uint64_t dt = now_ts - m_lastPktTs[ifIndex];
	uint64_t now_rate = 0;

	// 或者也可以在.h里加一个各端口计数变量，每10个包或过去10us测一次
	if (m_cnt[ifIndex] == 10 || dt >= 1000*(100000000000/max_rate[ifIndex])) {
		m_rate[ifIndex] = m_txBytes[ifIndex]*8*1000000000/dt;
		m_lastPktTs[ifIndex] = now_ts;
		m_txBytes[ifIndex] = 0;
		m_cnt[ifIndex] = 0;
	}

	now_rate = m_rate[ifIndex];

	
	uint8_t* buf = p->GetBuffer();
	if (buf[PppHeader::GetStaticSize() + 9] == 0x06) {
		MyIntHeader *ih = (MyIntHeader*)&buf[PppHeader::GetStaticSize() + 20 + 20 + 6];
		Ptr<QbbNetDevice> dev = DynamicCast<QbbNetDevice>(m_devices[ifIndex]);

		uint8_t id = m_id;
		uint32_t ts = m_lastPktTs[ifIndex];
		int push_rst;
		// 放置在数据包中的最大速率信息单位为100MB/s
		uint64_t _max_rate = max_rate[ifIndex]/8/100000000;
		uint32_t depth = dev->GetQueue()->GetNBytesTotal();
		push_rst = ih->PushDepth(id, ifIndex, depth, ts, _max_rate);

		std::cout << "node: " << (int)id << " port: " << ifIndex << std::endl;
		std::cout << "depth: " << depth << std::endl;
		std::cout << "rate: " << now_rate << std::endl;
		std::cout << "max rate: " << dev->GetDataRate().GetBitRate() << std::endl;
		std::cout << "ratio: " << (now_rate*10000)/max_rate[ifIndex] << std::endl;
		std::cout << std::endl;

		if (push_rst < 0) {
			// uint64_t _ratio = 0;
			// dev->GetDataRate().GetBitRate() 实际上和 max_rate[ifIndex]是一样的值
			// 乘10000将比值转化为整数值，比方90.12%会变为9012
			// uint64_t _ratio = (dev->GetDataRate().GetBitRate()*10000)/max_rate[ifIndex];
			uint64_t _ratio = (now_rate*10000)/max_rate[ifIndex];
			_ratio = _ratio<10000 ? _ratio:10000;
			push_rst = ih->PushRatio(id, ifIndex, _ratio, ts, _max_rate);
			//if (_ratio >= 10000) {
			//	std::cout << "size: " << p->GetSize() << std::endl;
			//	std::cout << "dt: " << dt << std::endl;
			//	std::cout << now_rate << std::endl;
			//	std::cout << "ratio: " <<  _ratio << std::endl;
			//	exit(0);
			//}
		}

		if (1/*push_rst <= 0*/) {
			ih->PushRoute(id, ifIndex);
		}
		// std::cout<<"id:"<<id<<"	ifIndex:"<<ifIndex<<std::endl;
	}
	
}

} /* namespace ns3 */
