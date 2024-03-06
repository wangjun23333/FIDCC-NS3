#include <ns3/simulator.h>
#include <ns3/seq-ts-header.h>
#include <ns3/udp-header.h>
#include <ns3/ipv4-header.h>
#include "ns3/ppp-header.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/data-rate.h"
#include "ns3/pointer.h"
#include "rdma-hw.h"
#include "ppp-header.h"
#include "qbb-header.h"
#include "cn-header.h"
#include "ns3/sequence-number.h"
#include "ns3/tcp-header.h"

namespace ns3{

TypeId RdmaHw::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::RdmaHw")
        .SetParent<Object> ()
        .AddAttribute("MinRate",
                "Minimum rate of a throttled flow",
                DataRateValue(DataRate("100Mb/s")),
                MakeDataRateAccessor(&RdmaHw::m_minRate),
                MakeDataRateChecker())
        .AddAttribute("Mtu",
                "Mtu.",
                UintegerValue(1000),
                MakeUintegerAccessor(&RdmaHw::m_mtu),
                MakeUintegerChecker<uint32_t>())
        .AddAttribute ("CcMode",
                "which mode of DCQCN is running",
                UintegerValue(0),
                MakeUintegerAccessor(&RdmaHw::m_cc_mode),
                MakeUintegerChecker<uint32_t>())
        .AddAttribute("NACK Generation Interval",
                "The NACK Generation interval",
                DoubleValue(500.0),
                MakeDoubleAccessor(&RdmaHw::m_nack_interval),
                MakeDoubleChecker<double>())
        .AddAttribute("L2ChunkSize",
                "Layer 2 chunk size. Disable chunk mode if equals to 0.",
                UintegerValue(0),
                MakeUintegerAccessor(&RdmaHw::m_chunk),
                MakeUintegerChecker<uint32_t>())
        .AddAttribute("L2AckInterval",
                "Layer 2 Ack intervals. Disable ack if equals to 0.",
                UintegerValue(0),
                MakeUintegerAccessor(&RdmaHw::m_ack_interval),
                MakeUintegerChecker<uint32_t>())
        .AddAttribute("L2BackToZero",
                "Layer 2 go back to zero transmission.",
                BooleanValue(false),
                MakeBooleanAccessor(&RdmaHw::m_backto0),
                MakeBooleanChecker())
        .AddAttribute("EwmaGain",
                "Control gain parameter which determines the level of rate decrease",
                DoubleValue(1.0 / 16),
                MakeDoubleAccessor(&RdmaHw::m_g),
                MakeDoubleChecker<double>())
        .AddAttribute ("RateOnFirstCnp",
                "the fraction of rate on first CNP",
                DoubleValue(1.0),
                MakeDoubleAccessor(&RdmaHw::m_rateOnFirstCNP),
                MakeDoubleChecker<double> ())
        .AddAttribute("ClampTargetRate",
                "Clamp target rate.",
                BooleanValue(false),
                MakeBooleanAccessor(&RdmaHw::m_EcnClampTgtRate),
                MakeBooleanChecker())
        .AddAttribute("RPTimer",
                "The rate increase timer at RP in microseconds",
                DoubleValue(1500.0),
                MakeDoubleAccessor(&RdmaHw::m_rpgTimeReset),
                MakeDoubleChecker<double>())
        .AddAttribute("RateDecreaseInterval",
                "The interval of rate decrease check",
                DoubleValue(4.0),
                MakeDoubleAccessor(&RdmaHw::m_rateDecreaseInterval),
                MakeDoubleChecker<double>())
        .AddAttribute("FastRecoveryTimes",
                "The rate increase timer at RP",
                UintegerValue(5),
                MakeUintegerAccessor(&RdmaHw::m_rpgThreshold),
                MakeUintegerChecker<uint32_t>())
        .AddAttribute("AlphaResumInterval",
                "The interval of resuming alpha",
                DoubleValue(55.0),
                MakeDoubleAccessor(&RdmaHw::m_alpha_resume_interval),
                MakeDoubleChecker<double>())
        .AddAttribute("RateAI",
                "Rate increment unit in AI period",
                DataRateValue(DataRate("5Mb/s")),
                MakeDataRateAccessor(&RdmaHw::m_rai),
                MakeDataRateChecker())
        .AddAttribute("RateHAI",
                "Rate increment unit in hyperactive AI period",
                DataRateValue(DataRate("50Mb/s")),
                MakeDataRateAccessor(&RdmaHw::m_rhai),
                MakeDataRateChecker())
        .AddAttribute("VarWin",
                "Use variable window size or not",
                BooleanValue(false),
                MakeBooleanAccessor(&RdmaHw::m_var_win),
                MakeBooleanChecker())
        .AddAttribute("FastReact",
                "Fast React to congestion feedback",
                BooleanValue(true),
                MakeBooleanAccessor(&RdmaHw::m_fast_react),
                MakeBooleanChecker())
        .AddAttribute("MiThresh",
                "Threshold of number of consecutive AI before MI",
                UintegerValue(5),
                MakeUintegerAccessor(&RdmaHw::m_miThresh),
                MakeUintegerChecker<uint32_t>())
        .AddAttribute("TargetUtil",
                "The Target Utilization of the bottleneck bandwidth, by default 95%",
                DoubleValue(0.95),
                MakeDoubleAccessor(&RdmaHw::m_targetUtil),
                MakeDoubleChecker<double>())
        .AddAttribute("UtilHigh",
                "The upper bound of Target Utilization of the bottleneck bandwidth, by default 98%",
                DoubleValue(0.98),
                MakeDoubleAccessor(&RdmaHw::m_utilHigh),
                MakeDoubleChecker<double>())
        .AddAttribute("RateBound",
                "Bound packet sending by rate, for test only",
                BooleanValue(true),
                MakeBooleanAccessor(&RdmaHw::m_rateBound),
                MakeBooleanChecker())
        .AddAttribute("MultiRate",
                "Maintain multiple rates in HPCC",
                BooleanValue(true),
                MakeBooleanAccessor(&RdmaHw::m_multipleRate),
                MakeBooleanChecker())
        .AddAttribute("SampleFeedback",
                "Whether sample feedback or not",
                BooleanValue(false),
                MakeBooleanAccessor(&RdmaHw::m_sampleFeedback),
                MakeBooleanChecker())
        .AddAttribute("TimelyAlpha",
                "Alpha of TIMELY",
                DoubleValue(0.875),
                MakeDoubleAccessor(&RdmaHw::m_tmly_alpha),
                MakeDoubleChecker<double>())
        .AddAttribute("TimelyBeta",
                "Beta of TIMELY",
                DoubleValue(0.8),
                MakeDoubleAccessor(&RdmaHw::m_tmly_beta),
                MakeDoubleChecker<double>())
        .AddAttribute("TimelyTLow",
                "TLow of TIMELY (ns)",
                UintegerValue(50000),
                MakeUintegerAccessor(&RdmaHw::m_tmly_TLow),
                MakeUintegerChecker<uint64_t>())
        .AddAttribute("TimelyTHigh",
                "THigh of TIMELY (ns)",
                UintegerValue(500000),
                MakeUintegerAccessor(&RdmaHw::m_tmly_THigh),
                MakeUintegerChecker<uint64_t>())
        .AddAttribute("TimelyMinRtt",
                "MinRtt of TIMELY (ns)",
                UintegerValue(20000),
                MakeUintegerAccessor(&RdmaHw::m_tmly_minRtt),
                MakeUintegerChecker<uint64_t>())
        .AddAttribute("DctcpRateAI",
                "DCTCP's Rate increment unit in AI period",
                DataRateValue(DataRate("1000Mb/s")),
                MakeDataRateAccessor(&RdmaHw::m_dctcp_rai),
                MakeDataRateChecker())
        .AddAttribute("PintSmplThresh",
                "PINT's sampling threshold in rand()%65536",
                UintegerValue(65536),
                MakeUintegerAccessor(&RdmaHw::pint_smpl_thresh),
                MakeUintegerChecker<uint32_t>())
        ;
    return tid;
}

RdmaHw::RdmaHw(){
}

void RdmaHw::SetNode(Ptr<Node> node){
    m_node = node;
}
void RdmaHw::Setup(QpCompleteCallback cb){
    for (uint32_t i = 0; i < m_nic.size(); i++){
        Ptr<QbbNetDevice> dev = m_nic[i].dev;
        if (dev == NULL)
            continue;
        // share data with NIC
        dev->m_rdmaEQ->m_qpGrp = m_nic[i].qpGrp;
        // setup callback
        dev->m_rdmaReceiveCb = MakeCallback(&RdmaHw::Receive, this);
        dev->m_rdmaLinkDownCb = MakeCallback(&RdmaHw::SetLinkDown, this);
        dev->m_rdmaPktSent = MakeCallback(&RdmaHw::PktSent, this);
        // config NIC
        dev->m_rdmaEQ->m_rdmaGetNxtPkt = MakeCallback(&RdmaHw::GetNxtPacket, this);
    }
    // setup qp complete callback
    m_qpCompleteCallback = cb;
}

uint32_t RdmaHw::GetNicIdxOfQp(Ptr<RdmaQueuePair> qp){
    
    auto it = m_routerMap.find(qp->dip.Get());

    if (it != m_routerMap.end()) {
        int valueForKey = it->second;
        return valueForKey;
    } else {
        NS_ASSERT_MSG(false, "We assume at least one NIC is alive");
    }
}
uint64_t RdmaHw::GetQpKey(uint32_t dip, uint16_t sport, uint16_t pg){
    return ((uint64_t)dip << 32) | ((uint64_t)sport << 16) | (uint64_t)pg;
}
Ptr<RdmaQueuePair> RdmaHw::GetQp(uint32_t dip, uint16_t sport, uint16_t pg){
    uint64_t key = GetQpKey(dip, sport, pg);

    auto it = m_qpMap.find(key);
    if (it != m_qpMap.end())
        return it->second;
    return NULL;
}
void RdmaHw::AddQueuePair(uint64_t size, uint16_t pg, Ipv4Address sip, Ipv4Address dip, uint16_t sport, uint16_t dport, uint32_t win, uint64_t baseRtt, Callback<void> notifyAppFinish){
    // create qp
    Ptr<RdmaQueuePair> qp = CreateObject<RdmaQueuePair>(pg, sip, dip, sport, dport);
    qp->SetSize(size);
    qp->SetWin(win);
    qp->SetBaseRtt(baseRtt);
    qp->SetVarWin(m_var_win);
    qp->SetAppNotifyCallback(notifyAppFinish);

    // add qp
    uint32_t nic_idx = GetNicIdxOfQp(qp);
    m_nic[nic_idx].qpGrp->AddQp(qp);
    uint64_t key = GetQpKey(dip.Get(), sport, pg);
    m_qpMap[key] = qp;
    std::cout<<"qp pg:"<<pg<<"  qp sip:"<<sip.Get()<<"  qp dip:"<<dip.Get()<<"qp sport"<<sport<<"qp dport"<<dport<<std::endl;

    // set init variables
    DataRate m_bps = m_nic[nic_idx].dev->GetDataRate();
    qp->m_rate = m_bps;
    qp->m_max_rate = m_bps;
    if (m_cc_mode == 1){
        qp->mlx.m_targetRate = m_bps;
    }else if (m_cc_mode == 3){
        qp->hp.m_curRate = m_bps;
        if (m_multipleRate){
            for (uint32_t i = 0; i < IntHeader::maxHop; i++)
                qp->hp.hopState[i].Rc = m_bps;
        }
    }else if (m_cc_mode == 7){
        qp->tmly.m_curRate = m_bps;
    }else if (m_cc_mode == 10){
        qp->hpccPint.m_curRate = m_bps;
    }else
        qp->mycc.m_currentWinSize = win;

    // Notify Nic
    m_nic[nic_idx].dev->NewQp(qp);
}

void RdmaHw::DeleteQueuePair(Ptr<RdmaQueuePair> qp){
    // remove qp from the m_qpMap
    uint64_t key = GetQpKey(qp->dip.Get(), qp->sport, qp->m_pg);
    m_qpMap.erase(key);
}

Ptr<RdmaRxQueuePair> RdmaHw::GetRxQp(uint32_t sip, uint32_t dip, uint16_t sport, uint16_t dport, uint16_t pg, bool create){
    uint64_t key = ((uint64_t)dip << 32) | ((uint64_t)pg << 16) | (uint64_t)dport;
    auto it = m_rxQpMap.find(key);
    if (it != m_rxQpMap.end())
        return it->second;
    if (create){
        // create new rx qp
        Ptr<RdmaRxQueuePair> qp = CreateObject<RdmaRxQueuePair>();
        // init the qp
        qp->sip = sip;
        qp->dip = dip;
        qp->sport = sport;
        qp->dport = dport;
        qp->m_ecn_source.qIndex = pg;
        // store in map
        m_rxQpMap[key] = qp;
        return qp;
    }
    return NULL;
}
uint32_t RdmaHw::GetNicIdxOfRxQp(Ptr<RdmaRxQueuePair> qp){
    
    auto it = m_routerMap.find(qp->dip);

    if (it != m_routerMap.end()) {
        int valueForKey = it->second;
        return valueForKey;
    } else {
        NS_ASSERT_MSG(false, "We assume at least one NIC is alive");
    }
}
void RdmaHw::DeleteRxQp(uint32_t dip, uint16_t pg, uint16_t dport){
    uint64_t key = ((uint64_t)dip << 32) | ((uint64_t)pg << 16) | (uint64_t)dport;
    m_rxQpMap.erase(key);
}

int RdmaHw::ReceiveUdp(Ptr<Packet> p, MyCustomHeader &ch){
    uint8_t ecnbits = ch.GetIpv4EcnBits();

    uint32_t payload_size = p->GetSize() - ch.GetSerializedSize();

    // TODO find corresponding rx queue pair
    /*Ptr<RdmaRxQueuePair> rxQp = GetRxQp(ch.dip, ch.sip, ch.udp.dport, ch.udp.sport, ch.udp.pg, true);
    if (ecnbits != 0){
        rxQp->m_ecn_source.ecnbits |= ecnbits;
        rxQp->m_ecn_source.qfb++;
    }
    rxQp->m_ecn_source.total++;
    rxQp->m_milestone_rx = m_ack_interval;

    int x = ReceiverCheckSeq(ch.udp.seq, rxQp, payload_size);
    if (x == 1 || x == 2){ //generate ACK or NACK
        qbbHeader seqh;
        seqh.SetSeq(rxQp->ReceiverNextExpectedSeq);
        seqh.SetPG(ch.udp.pg);
        seqh.SetSport(ch.udp.dport);
        seqh.SetDport(ch.udp.sport);
        seqh.SetIntHeader(ch.udp.ih);
        if (ecnbits)
            seqh.SetCnp();

        Ptr<Packet> newp = Create<Packet>(std::max(60-14-20-(int)seqh.GetSerializedSize(), 0));
        newp->AddHeader(seqh); //将ppp头部的上述信息写入到buffer中，方便后续在receive数据包时，ch从buffer中读取

        Ipv4Header head;    // Prepare IPv4 header
        head.SetDestination(Ipv4Address(ch.sip));
        head.SetSource(Ipv4Address(ch.dip));
        head.SetProtocol(x == 1 ? 0xFC : 0xFD); //ack=0xFC nack=0xFD
        head.SetTtl(64);
        head.SetPayloadSize(newp->GetSize());
        head.SetIdentification(rxQp->m_ipid++);

        newp->AddHeader(head);
        AddHeader(newp, 0x800);    // Attach PPP header
        // send
        uint32_t nic_idx = GetNicIdxOfRxQp(rxQp);
        m_nic[nic_idx].dev->RdmaEnqueueHighPrioQ(newp);
        m_nic[nic_idx].dev->TriggerTransmit();
    }*/
    return 0;
}


int RdmaHw::ReceiveTcp(Ptr<Packet> p, MyCustomHeader &ch){
		// std::cout<< "node:" << m_node->GetId()<< "  tcp sip:" << ch.sip << "    tcp dip:" << ch.dip<< " tcp-seq:"<< ch.tcp.seq<<std::endl;
    uint8_t ecnbits = ch.GetIpv4EcnBits();
    // std::cout<< "node\t" << m_node->GetId() << "tcp packet node num\t" << ch.tcp.ih.hinfo.nodeNum << "depth num\t" << ch.tcp.ih.hinfo.depthNum << "ratio num\t" << ch.tcp.ih.hinfo.ratioNum << "\tseq"<< ch.tcp.seq <<std::endl;

    uint32_t payload_size = p->GetSize() - ch.GetSerializedSize();
    // if (ch.tcp.ih.hinfo.depthNum !=0)
    // {
    //     std::cout << "depth routerID:" << ch.tcp.ih.dinfo[0].iinfo.id << " depth routerPORT:"<< ch.tcp.ih.dinfo[0].iinfo.port << std::endl;
    //     std::cout << "depth:" << ch.tcp.ih.dinfo[0].depth << " ts:"<< ch.tcp.ih.dinfo[0].ts << std::endl;
    // }
    
    // std::cout << "packet_size" << p->GetSize() << "ch_size\t"<< ch.GetSerializedSize() << std::endl;
    // TODO find corresponding rx queue pairstd::cout << "node:" << m_node->GetId()<< " sip:" << ch.sip << "  dip:"<< ch.dip << std::endl;
    
		std::cout << "tcp info: " << std::endl;
		for (int i = 0; i < ch.tcp.ih.hinfo.depthNum; ++i) {
			std::cout << i << std::endl;
			std::cout << "node: " << (int)ch.tcp.ih.dinfo[i].iinfo.id << " port: " << ch.tcp.ih.dinfo[i].iinfo.port << std::endl;
			std::cout << "tcp depth: " << ch.tcp.ih.dinfo[i].depth << std::endl;
		}
		for (int i = 0; i < ch.tcp.ih.hinfo.ratioNum; ++i) {
			std::cout << i << std::endl;
			std::cout << "node: " << (int)ch.tcp.ih.rinfo[i].iinfo.id << " port: " << ch.tcp.ih.rinfo[i].iinfo.port << std::endl;
			std::cout << "tcp ratio: " << ch.tcp.ih.rinfo[i].ratio << std::endl;
		}
		std::cout << std::endl;

    Ptr<RdmaRxQueuePair> rxQp = GetRxQp(ch.dip, ch.sip, ch.tcp.dport, ch.tcp.sport, 0, true);
    if (ecnbits != 0){
        rxQp->m_ecn_source.ecnbits |= ecnbits;
        rxQp->m_ecn_source.qfb++;
    }
    rxQp->m_ecn_source.total++;
    rxQp->m_milestone_rx = m_ack_interval;

    int x = ReceiverCheckSeq(ch.tcp.seq, rxQp, payload_size);
		//x = 1;
    // std::cout<< "tcp-seq:"<< ch.tcp.seq << std::endl;
    if (x == 1 || x == 2){ //generate ACK or NACK
//        qbbHeader seqh;
        encHeader encH;
        encH.SetSeq(rxQp->ReceiverNextExpectedSeq);
        encH.SetPG(0);
        encH.SetSport(ch.tcp.dport);
        encH.SetDport(ch.tcp.sport);
        encH.SetFin(ch.tcp.tcpFlags&0x01);//添加fin标志位
        encH.SetMyIntHeader(ch.tcp.ih);
//        if (ecnbits)
//            seqh.SetCnp();
        // std::cout<< "node:" << m_node->GetId()  << "    ack-seq"<< rxQp->ReceiverNextExpectedSeq <<std::endl;
        Ptr<Packet> newp = Create<Packet>(std::max(60-14-20-(int)encH.GetSerializedSize(), 0));
        newp->AddHeader(encH); //将ppp头部的上述信息写入到buffer中，方便后续在receive数据包时，ch从buffer中读取

        Ipv4Header head;    // Prepare IPv4 header
        head.SetDestination(Ipv4Address(ch.sip));
        head.SetSource(Ipv4Address(ch.dip));
        head.SetProtocol(x == 1 ? 0xFC : 0xFD); //ack=0xFC nack=0xFD
        head.SetTtl(64);
        head.SetPayloadSize(newp->GetSize());
        head.SetIdentification(rxQp->m_ipid++);

        newp->AddHeader(head);
        AddHeader(newp, 0x800);    // Attach PPP header
        // send
        uint32_t nic_idx = GetNicIdxOfRxQp(rxQp);
        m_nic[nic_idx].dev->RdmaEnqueueHighPrioQ(newp);
        m_nic[nic_idx].dev->TriggerTransmit();
    }
    return 0;
}


int RdmaHw::ReceiveCnp(Ptr<Packet> p, CustomHeader &ch){
    // QCN on NIC
    // This is a Congestion signal
    // Then, extract data from the congestion packet.
    // We assume, without verify, the packet is destinated to me
    uint32_t qIndex = ch.cnp.qIndex;
    if (qIndex == 1){        //DCTCP
        std::cout << "TCP--ignore\n";
        return 0;
    }
    uint16_t udpport = ch.cnp.fid; // corresponds to the sport
    uint8_t ecnbits = ch.cnp.ecnBits;
    uint16_t qfb = ch.cnp.qfb;
    uint16_t total = ch.cnp.total;

    uint32_t i;
    // get qp
    Ptr<RdmaQueuePair> qp = GetQp(ch.sip, udpport, qIndex);
    if (qp == NULL)
        std::cout << "ERROR: QCN NIC cannot find the flow\n";
    // get nic
    uint32_t nic_idx = GetNicIdxOfQp(qp);
    Ptr<QbbNetDevice> dev = m_nic[nic_idx].dev;

    if (qp->m_rate == 0)            //lazy initialization
    {
        qp->m_rate = dev->GetDataRate();
        if (m_cc_mode == 1){
            qp->mlx.m_targetRate = dev->GetDataRate();
        }else if (m_cc_mode == 3){
            qp->hp.m_curRate = dev->GetDataRate();
            if (m_multipleRate){
                for (uint32_t i = 0; i < IntHeader::maxHop; i++)
                    qp->hp.hopState[i].Rc = dev->GetDataRate();
            }
        }else if (m_cc_mode == 7){
            qp->tmly.m_curRate = dev->GetDataRate();
        }else if (m_cc_mode == 10){
            qp->hpccPint.m_curRate = dev->GetDataRate();
        }
    }
    return 0;
}

int RdmaHw::ReceiveAck(Ptr<Packet> p, MyCustomHeader &ch){
    uint16_t qIndex = ch.ack.pg;
    uint16_t port = ch.ack.dport;
    uint32_t seq = ch.ack.seq;
    //0 std::cout<< "node:" << m_node->GetId()<< "  ack sip:" << ch.sip << "    ack dip:" << ch.dip<<"  ch-ack-flag:"<< ch.ack.flags <<"    ack-seq:"<< ch.ack.seq<<std::endl;

    /*uint8_t cnp = (ch.ack.flags >> qbbHeader::FLAG_CNP) & 1;
    int i;*/
    Ptr<RdmaQueuePair> qp = GetQp(ch.sip, port, qIndex);
    if (qp == NULL){
        std::cout << "ERROR: " << "node:" << m_node->GetId() << ' ' << (ch.l3Prot == 0xFC ? "ACK" : "NACK") << " NIC cannot find the flow\n";
        return 0;
    }

		std::cout << "ack info: " << std::endl;
		for (int i = 0; i < ch.ack.ih.hinfo.depthNum; ++i) {
			std::cout << i << std::endl;
			std::cout << "node: " << (int)ch.ack.ih.dinfo[i].iinfo.id << " port: " << ch.ack.ih.dinfo[i].iinfo.port << std::endl;
			std::cout << "ack depth: " << ch.ack.ih.dinfo[i].depth << std::endl;
		}
		for (int i = 0; i < ch.ack.ih.hinfo.ratioNum; ++i) {
			std::cout << i << std::endl;
			std::cout << "node: " << (int)ch.ack.ih.rinfo[i].iinfo.id << " port: " << ch.ack.ih.rinfo[i].iinfo.port << std::endl;
			std::cout << "ack ratio: " << ch.ack.ih.rinfo[i].ratio << std::endl;
		}
		std::cout << std::endl;

    if ((ch.ack.flags&0x01) == 0) { //自身数据包
        uint32_t nic_idx = GetNicIdxOfQp(qp);
        Ptr<QbbNetDevice> dev = m_nic[nic_idx].dev;
        if (m_ack_interval == 0)
            std::cout << "ERROR: shouldn't receive ack\n";
        else {
            if (!m_backto0){
                qp->Acknowledge(seq);
            }else {
                uint32_t goback_seq = seq / m_chunk * m_chunk;
                qp->Acknowledge(goback_seq);
            }
            if (qp->IsFinished()){
                QpComplete(qp);
            }
        }
        if (ch.l3Prot == 0xFD) // NACK
            RecoverQueue(qp);

        // handle cnp
        /*if (cnp){
            if (m_cc_mode == 1){ // mlx version
                cnp_received_mlx(qp);
            }
        }*/

        if (m_cc_mode == 3){
            //HandleAckHp(qp, p, ch);
        }else if (m_cc_mode == 7){
            //HandleAckTimely(qp, p, ch);
        }else if (m_cc_mode == 8){
            //HandleAckDctcp(qp, p, ch);
        }else if (m_cc_mode == 10){
            //HandleAckHpPint(qp, p, ch);
        }else{
            HandleAckMycc(qp, p, ch);
        }
        // ACK may advance the on-the-fly window, allowing more packets to send
        dev->TriggerTransmit();
        return 0;
    }else{
        HandleAckMycc(qp, p, ch);
    }
    return 0;
    

}

int RdmaHw::Receive(Ptr<Packet> p, MyCustomHeader &ch){
    if (ch.l3Prot == 0x06){ // UDP
        ReceiveTcp(p, ch);
    }else if (ch.l3Prot == 0xFF){ // CNP
        //ReceiveCnp(p, ch);
    }else if (ch.l3Prot == 0xFD){ // NACK
        ReceiveAck(p, ch);
    }else if (ch.l3Prot == 0xFC){ // ACK
        ReceiveAck(p, ch);
    }
    return 0;
}

int RdmaHw::ReceiverCheckSeq(uint32_t seq, Ptr<RdmaRxQueuePair> qp, uint32_t size){
    uint32_t expected = qp->ReceiverNextExpectedSeq;
    if (seq == expected){
        qp->ReceiverNextExpectedSeq = expected + size;
        if (qp->ReceiverNextExpectedSeq >= qp->m_milestone_rx){
            qp->m_milestone_rx += m_ack_interval;
            return 1; //Generate ACK
        }else if (qp->ReceiverNextExpectedSeq % m_chunk == 0){
            return 1;
        }else {
            return 5;
        }
    } else if (seq > expected) {
        // Generate NACK
        if (Simulator::Now() >= qp->m_nackTimer || qp->m_lastNACK != expected){
            qp->m_nackTimer = Simulator::Now() + MicroSeconds(m_nack_interval);
            qp->m_lastNACK = expected;
            if (m_backto0){
                qp->ReceiverNextExpectedSeq = qp->ReceiverNextExpectedSeq / m_chunk*m_chunk;
            }
            return 2;
        }else
            return 4;
    }else {
        // Duplicate.
        return 3;
    }
}
void RdmaHw::AddHeader (Ptr<Packet> p, uint16_t protocolNumber){
    PppHeader ppp;
    ppp.SetProtocol (EtherToPpp (protocolNumber));
    p->AddHeader (ppp);
}
uint16_t RdmaHw::EtherToPpp (uint16_t proto){
    switch(proto){
        case 0x0800: return 0x0021;   //IPv4
        case 0x86DD: return 0x0057;   //IPv6
        default: NS_ASSERT_MSG (false, "PPP Protocol number not defined!");
    }
    return 0;
}

void RdmaHw::RecoverQueue(Ptr<RdmaQueuePair> qp){
    qp->snd_nxt = qp->snd_una;
}

void RdmaHw::QpComplete(Ptr<RdmaQueuePair> qp){
    NS_ASSERT(!m_qpCompleteCallback.IsNull());
    if (m_cc_mode == 1){
        Simulator::Cancel(qp->mlx.m_eventUpdateAlpha);
        Simulator::Cancel(qp->mlx.m_eventDecreaseRate);
        Simulator::Cancel(qp->mlx.m_rpTimer);
    }

    // This callback will log info
    // It may also delete the rxQp on the receiver
    m_qpCompleteCallback(qp);

    qp->m_notifyAppFinish();

    // delete the qp
    DeleteQueuePair(qp);
}

void RdmaHw::SetLinkDown(Ptr<QbbNetDevice> dev){
    printf("RdmaHw: node:%u a link down\n", m_node->GetId());
}

void RdmaHw::AddTableEntry(Ipv4Address &dstAddr, uint32_t intf_idx){
    uint32_t dip = dstAddr.Get();
    m_routerMap[dip]= intf_idx;
}

void RdmaHw::ClearTable(){
    m_routerMap.clear();
}

void RdmaHw::RedistributeQp(){
    // clear old qpGrp
    for (uint32_t i = 0; i < m_nic.size(); i++){
        if (m_nic[i].dev == NULL)
            continue;
        m_nic[i].qpGrp->Clear();
    }

    // redistribute qp
    for (auto &it : m_qpMap){
        Ptr<RdmaQueuePair> qp = it.second;
        uint32_t nic_idx = GetNicIdxOfQp(qp);
        m_nic[nic_idx].qpGrp->AddQp(qp);
        // Notify Nic
        m_nic[nic_idx].dev->ReassignedQp(qp);
    }
}

Ptr<Packet> RdmaHw::GetNxtPacket(Ptr<RdmaQueuePair> qp){
    uint32_t payload_size = qp->GetBytesLeft();
    bool fin = false;
    if (m_mtu < payload_size)
        payload_size = m_mtu;
    else //剩余数据量小于等于一个MTU
        fin = true;
    
    Ptr<Packet> p = Create<Packet> (payload_size);
    // add SeqTsHeader
    SeqTsHeader seqTs;

    // seqTs.SetSeq (qp->snd_nxt);
    seqTs.SetPG (qp->m_pg);
    p->AddHeader(seqTs);
    // add udp header
    TcpHeader tcpHeader;
    tcpHeader.SetDestinationPort (qp->dport);
    tcpHeader.SetSourcePort (qp->sport);
    tcpHeader.SetFin(fin);//添加fin标志位
    tcpHeader.SetSequenceNumber(SequenceNumber32((uint32_t)(qp->snd_nxt)));
    p->AddHeader (tcpHeader);
    // add ipv4 header
    Ipv4Header ipHeader;
    ipHeader.SetSource (qp->sip);
    ipHeader.SetDestination (qp->dip);
    ipHeader.SetProtocol (0x06);
    ipHeader.SetPayloadSize (p->GetSize());
    ipHeader.SetTtl (64);
    ipHeader.SetTos (0);
    ipHeader.SetIdentification (qp->m_ipid);
    p->AddHeader(ipHeader);
    // add ppp header
    PppHeader ppp;
    ppp.SetProtocol (0x0021); // EtherToPpp(0x800), see point-to-point-net-device.cc
    p->AddHeader (ppp);

    // update state
    qp->snd_nxt += payload_size;
    qp->m_ipid++;

    // return
    return p;
}

void RdmaHw::PktSent(Ptr<RdmaQueuePair> qp, Ptr<Packet> pkt, Time interframeGap){
    qp->lastPktSize = pkt->GetSize();
    UpdateNextAvail(qp, interframeGap, pkt->GetSize());
}

void RdmaHw::UpdateNextAvail(Ptr<RdmaQueuePair> qp, Time interframeGap, uint32_t pkt_size){
    Time sendingTime;
    if (m_rateBound)
        sendingTime = interframeGap + Seconds(qp->m_rate.CalculateTxTime(pkt_size));
    else
        sendingTime = interframeGap + Seconds(qp->m_max_rate.CalculateTxTime(pkt_size));
    qp->m_nextAvail = Simulator::Now() + sendingTime;
}

void RdmaHw::ChangeRate(Ptr<RdmaQueuePair> qp, DataRate new_rate){
    #if 1
    Time sendingTime = Seconds(qp->m_rate.CalculateTxTime(qp->lastPktSize));
    Time new_sendintTime = Seconds(new_rate.CalculateTxTime(qp->lastPktSize));
    qp->m_nextAvail = qp->m_nextAvail + new_sendintTime - sendingTime;
    // update nic's next avail event
    uint32_t nic_idx = GetNicIdxOfQp(qp);
    m_nic[nic_idx].dev->UpdateNextAvail(qp->m_nextAvail);
    #endif

    // change to new rate
    qp->m_rate = new_rate;
}

#define PRINT_LOG 0
/******************************
 * Mellanox's version of DCQCN
 *****************************/
void RdmaHw::UpdateAlphaMlx(Ptr<RdmaQueuePair> qp){
    #if PRINT_LOG
    //std::cout << Simulator::Now() << " alpha update:" << m_node->GetId() << ' ' << qp->mlx.m_alpha << ' ' << (int)qp->mlx.m_alpha_cnp_arrived << '\n';
    //printf("%lu alpha update: %08x %08x %u %u %.6lf->", Simulator::Now().GetTimeStep(), qp->sip.Get(), qp->dip.Get(), qp->sport, qp->dport, qp->mlx.m_alpha);
    #endif
    if (qp->mlx.m_alpha_cnp_arrived){
        qp->mlx.m_alpha = (1 - m_g)*qp->mlx.m_alpha + m_g;     //binary feedback
    }else {
        qp->mlx.m_alpha = (1 - m_g)*qp->mlx.m_alpha;     //binary feedback
    }
    #if PRINT_LOG
    //printf("%.6lf\n", qp->mlx.m_alpha);
    #endif
    qp->mlx.m_alpha_cnp_arrived = false; // clear the CNP_arrived bit
    ScheduleUpdateAlphaMlx(qp);
}
void RdmaHw::ScheduleUpdateAlphaMlx(Ptr<RdmaQueuePair> qp){
    qp->mlx.m_eventUpdateAlpha = Simulator::Schedule(MicroSeconds(m_alpha_resume_interval), &RdmaHw::UpdateAlphaMlx, this, qp);
}

void RdmaHw::cnp_received_mlx(Ptr<RdmaQueuePair> qp){
    qp->mlx.m_alpha_cnp_arrived = true; // set CNP_arrived bit for alpha update
    qp->mlx.m_decrease_cnp_arrived = true; // set CNP_arrived bit for rate decrease
    if (qp->mlx.m_first_cnp){
        // init alpha
        qp->mlx.m_alpha = 1;
        qp->mlx.m_alpha_cnp_arrived = false;
        // schedule alpha update
        ScheduleUpdateAlphaMlx(qp);
        // schedule rate decrease
        ScheduleDecreaseRateMlx(qp, 1); // add 1 ns to make sure rate decrease is after alpha update
        // set rate on first CNP
        qp->mlx.m_targetRate = qp->m_rate = m_rateOnFirstCNP * qp->m_rate;
        qp->mlx.m_first_cnp = false;
    }
}

void RdmaHw::CheckRateDecreaseMlx(Ptr<RdmaQueuePair> qp){
    ScheduleDecreaseRateMlx(qp, 0);
    if (qp->mlx.m_decrease_cnp_arrived){
        #if PRINT_LOG
        printf("%lu rate dec: %08x %08x %u %u (%0.3lf %.3lf)->", Simulator::Now().GetTimeStep(), qp->sip.Get(), qp->dip.Get(), qp->sport, qp->dport, qp->mlx.m_targetRate.GetBitRate() * 1e-9, qp->m_rate.GetBitRate() * 1e-9);
        #endif
        bool clamp = true;
        if (!m_EcnClampTgtRate){
            if (qp->mlx.m_rpTimeStage == 0)
                clamp = false;
        }
        if (clamp)
            qp->mlx.m_targetRate = qp->m_rate;
        qp->m_rate = std::max(m_minRate, qp->m_rate * (1 - qp->mlx.m_alpha / 2));
        // reset rate increase related things
        qp->mlx.m_rpTimeStage = 0;
        qp->mlx.m_decrease_cnp_arrived = false;
        Simulator::Cancel(qp->mlx.m_rpTimer);
        qp->mlx.m_rpTimer = Simulator::Schedule(MicroSeconds(m_rpgTimeReset), &RdmaHw::RateIncEventTimerMlx, this, qp);
        #if PRINT_LOG
        printf("(%.3lf %.3lf)\n", qp->mlx.m_targetRate.GetBitRate() * 1e-9, qp->m_rate.GetBitRate() * 1e-9);
        #endif
    }
}
void RdmaHw::ScheduleDecreaseRateMlx(Ptr<RdmaQueuePair> qp, uint32_t delta){
    qp->mlx.m_eventDecreaseRate = Simulator::Schedule(MicroSeconds(m_rateDecreaseInterval) + NanoSeconds(delta), &RdmaHw::CheckRateDecreaseMlx, this, qp);
}

void RdmaHw::RateIncEventTimerMlx(Ptr<RdmaQueuePair> qp){
    qp->mlx.m_rpTimer = Simulator::Schedule(MicroSeconds(m_rpgTimeReset), &RdmaHw::RateIncEventTimerMlx, this, qp);
    RateIncEventMlx(qp);
    qp->mlx.m_rpTimeStage++;
}
void RdmaHw::RateIncEventMlx(Ptr<RdmaQueuePair> qp){
    // check which increase phase: fast recovery, active increase, hyper increase
    if (qp->mlx.m_rpTimeStage < m_rpgThreshold){ // fast recovery
        FastRecoveryMlx(qp);
    }else if (qp->mlx.m_rpTimeStage == m_rpgThreshold){ // active increase
        ActiveIncreaseMlx(qp);
    }else { // hyper increase
        HyperIncreaseMlx(qp);
    }
}

void RdmaHw::FastRecoveryMlx(Ptr<RdmaQueuePair> qp){
    #if PRINT_LOG
    printf("%lu fast recovery: %08x %08x %u %u (%0.3lf %.3lf)->", Simulator::Now().GetTimeStep(), qp->sip.Get(), qp->dip.Get(), qp->sport, qp->dport, qp->mlx.m_targetRate.GetBitRate() * 1e-9, qp->m_rate.GetBitRate() * 1e-9);
    #endif
    qp->m_rate = (qp->m_rate / 2) + (qp->mlx.m_targetRate / 2);
    #if PRINT_LOG
    printf("(%.3lf %.3lf)\n", qp->mlx.m_targetRate.GetBitRate() * 1e-9, qp->m_rate.GetBitRate() * 1e-9);
    #endif
}
void RdmaHw::ActiveIncreaseMlx(Ptr<RdmaQueuePair> qp){
    #if PRINT_LOG
    printf("%lu active inc: %08x %08x %u %u (%0.3lf %.3lf)->", Simulator::Now().GetTimeStep(), qp->sip.Get(), qp->dip.Get(), qp->sport, qp->dport, qp->mlx.m_targetRate.GetBitRate() * 1e-9, qp->m_rate.GetBitRate() * 1e-9);
    #endif
    // get NIC
    uint32_t nic_idx = GetNicIdxOfQp(qp);
    Ptr<QbbNetDevice> dev = m_nic[nic_idx].dev;
    // increate rate
    qp->mlx.m_targetRate += m_rai;
    if (qp->mlx.m_targetRate > dev->GetDataRate())
        qp->mlx.m_targetRate = dev->GetDataRate();
    qp->m_rate = (qp->m_rate / 2) + (qp->mlx.m_targetRate / 2);
    #if PRINT_LOG
    printf("(%.3lf %.3lf)\n", qp->mlx.m_targetRate.GetBitRate() * 1e-9, qp->m_rate.GetBitRate() * 1e-9);
    #endif
}
void RdmaHw::HyperIncreaseMlx(Ptr<RdmaQueuePair> qp){
    #if PRINT_LOG
    printf("%lu hyper inc: %08x %08x %u %u (%0.3lf %.3lf)->", Simulator::Now().GetTimeStep(), qp->sip.Get(), qp->dip.Get(), qp->sport, qp->dport, qp->mlx.m_targetRate.GetBitRate() * 1e-9, qp->m_rate.GetBitRate() * 1e-9);
    #endif
    // get NIC
    uint32_t nic_idx = GetNicIdxOfQp(qp);
    Ptr<QbbNetDevice> dev = m_nic[nic_idx].dev;
    // increate rate
    qp->mlx.m_targetRate += m_rhai;
    if (qp->mlx.m_targetRate > dev->GetDataRate())
        qp->mlx.m_targetRate = dev->GetDataRate();
    qp->m_rate = (qp->m_rate / 2) + (qp->mlx.m_targetRate / 2);
    #if PRINT_LOG
    printf("(%.3lf %.3lf)\n", qp->mlx.m_targetRate.GetBitRate() * 1e-9, qp->m_rate.GetBitRate() * 1e-9);
    #endif
}


/***********************
 * My CC
 ***********************/
void RdmaHw::HandleAckMycc(Ptr<RdmaQueuePair> qp, Ptr<Packet> p, MyCustomHeader &ch){
    //std::cout<< "node:" << m_node->GetId()<<"current windowsSIZE:"<<qp->mycc.m_currentWinSize<<std::endl;
    //可能是自身的ack数据包，也可能是同set主机的ack数据包
    //如果是第一个窗口或者当前窗口和上一个窗口的大小未发生改变的情况，此时不考虑过度反应
    if (qp->mycc.m_lastUpdateSeq == 0 || qp->mycc.m_currentWinSize == qp->mycc.m_lastWinSize) {
        if (ch.ack.ih.hinfo.depthNum != 0) {//数据包携带了队列长度信息
            int maxDepthIndex = 0;
            for (int i = 0; i < ch.ack.ih.hinfo.depthNum; ++i) { //获取最长的队列的索引
                if (ch.ack.ih.dinfo[i].depth > ch.ack.ih.dinfo[maxDepthIndex].depth) {
                    maxDepthIndex = i;
                }
            }
            int64_t dDelta_time = Simulator::Now().GetTimeStep()%((1<<24)*100) - 100*ch.ack.ih.dinfo[maxDepthIndex].ts;
            if (dDelta_time<0)
            {
                dDelta_time += (1<<24)*100;
            }
            
            if (dDelta_time < qp->mycc.m_congestTimeStamp) {
                qp->mycc.m_depth = ch.ack.ih.dinfo[maxDepthIndex].depth;
                qp->mycc.m_congestTimeStamp = dDelta_time;
                qp->mycc.m_dTs = ch.ack.ih.dinfo[maxDepthIndex].ts*100;
//                qp->mycc.m_dIsOwn = ch.ack.isOwn;
                qp->mycc.m_max_dRate = ch.ack.ih.dinfo[maxDepthIndex].maxRate;
                
            }else if (qp->mycc.m_congestTimeStamp == 0){
                qp->mycc.m_depth = ch.ack.ih.dinfo[maxDepthIndex].depth;
                qp->mycc.m_congestTimeStamp = dDelta_time;
                qp->mycc.m_dTs = ch.ack.ih.dinfo[maxDepthIndex].ts*100;
//                qp->mycc.m_dIsOwn = ch.ack.isOwn;
                qp->mycc.m_max_dRate = ch.ack.ih.dinfo[maxDepthIndex].maxRate;

            }
            
        }else if(ch.ack.ih.hinfo.ratioNum != 0 && ch.ack.ih.hinfo.depthNum == 0){//数据包只携带了速率比值信息
            int maxRadioIndex = 0;
            for (int i = 0; i < ch.ack.ih.hinfo.ratioNum; ++i) { //获取最长的队列的索引
                if (ch.ack.ih.rinfo[i].ratio > ch.ack.ih.rinfo[maxRadioIndex].ratio) {
                    maxRadioIndex = i;
                }
            }
            // uint64_t rDelta_time = Simulator::Now().GetTimeStep() - ch.ack.ih.rinfo[maxRadioIndex].ts;
            int64_t rDelta_time = Simulator::Now().GetTimeStep()%((1<<24)*100) - 100*ch.ack.ih.rinfo[maxRadioIndex].ts;;
            if (rDelta_time<0)
            {
                rDelta_time += (1<<24)*100;
            }
            if (rDelta_time < qp->mycc.m_idleTimeStamp) {
                qp->mycc.m_ratio = ch.ack.ih.rinfo[maxRadioIndex].ratio;
                qp->mycc.m_idleTimeStamp = rDelta_time;
                qp->mycc.m_rTs = ch.ack.ih.rinfo[maxRadioIndex].ts*100;
//                qp->mycc.m_rIsOwn = ch.ack.isOwn;
                qp->mycc.m_max_rRate = ch.ack.ih.rinfo[maxRadioIndex].maxRate;

            }else if (qp->mycc.m_congestTimeStamp == 0){
                qp->mycc.m_ratio = ch.ack.ih.rinfo[maxRadioIndex].ratio;
                qp->mycc.m_idleTimeStamp = rDelta_time;
                qp->mycc.m_rTs = ch.ack.ih.rinfo[maxRadioIndex].ts*100;
//                qp->mycc.m_rIsOwn = ch.ack.isOwn;
                qp->mycc.m_max_rRate = ch.ack.ih.rinfo[maxRadioIndex].maxRate;

            }
        }
    }else if(qp->mycc.m_currentWinSize < qp->mycc.m_lastWinSize){//当前窗口的大小小于上一个窗口的大小，防止对同一个拥塞事件作出反应
        if (ch.ack.ih.hinfo.depthNum != 0) {//数据包携带了队列长度信息,只选择同set主机且在窗口内发生拥塞的数据包做为判断
            int maxDepthIndexOverReaction = 0;
            for (int i = 0; i < ch.ack.ih.hinfo.depthNum; ++i) { //获取最长的队列的索引
                if (ch.ack.ih.dinfo[i].depth > ch.ack.ih.dinfo[maxDepthIndexOverReaction].depth) {
                    maxDepthIndexOverReaction = i;
                }
            }
            uint64_t dDelta_overReactionTime = Simulator::Now().GetTimeStep() - ch.ack.ih.dinfo[maxDepthIndexOverReaction].ts;
            if (ch.ack.flags == 1 && dDelta_overReactionTime < qp->mycc.m_congestTimeStamp && ch.ack.ih.dinfo[maxDepthIndexOverReaction].ts > qp->mycc.m_lastUpdateTime + 0.25*qp->m_baseRtt) {//
                qp->mycc.m_depth = ch.ack.ih.dinfo[maxDepthIndexOverReaction].depth;
                qp->mycc.m_congestTimeStamp = dDelta_overReactionTime;
                qp->mycc.m_dTs = ch.ack.ih.dinfo[maxDepthIndexOverReaction].ts*100;
//                qp->mycc.m_dIsOwn = ch.ack.isOwn;
                qp->mycc.m_max_dRate = ch.ack.ih.dinfo[maxDepthIndexOverReaction].maxRate;

            }else if(qp->mycc.m_congestTimeStamp == 0 && ch.ack.flags == 1 && ch.ack.ih.dinfo[maxDepthIndexOverReaction].ts > qp->mycc.m_lastUpdateTime + 0.25*qp->m_baseRtt){
                qp->mycc.m_depth = ch.ack.ih.dinfo[maxDepthIndexOverReaction].depth;
                qp->mycc.m_congestTimeStamp = dDelta_overReactionTime;
                qp->mycc.m_dTs = ch.ack.ih.dinfo[maxDepthIndexOverReaction].ts*100;
//                qp->mycc.m_dIsOwn = ch.ack.isOwn;
                qp->mycc.m_max_dRate = ch.ack.ih.dinfo[maxDepthIndexOverReaction].maxRate;

            }
        }else if(ch.ack.ih.hinfo.ratioNum != 0 && ch.ack.ih.hinfo.depthNum == 0){//数据包只携带了速率比值信息，只选择
            int maxRadioIndexOverReaction = 0;
            for (int i = 0; i < ch.ack.ih.hinfo.ratioNum; ++i) { //获取最长的队列的索引
                if (ch.ack.ih.rinfo[i].ratio > ch.ack.ih.rinfo[maxRadioIndexOverReaction].ratio) {
                    maxRadioIndexOverReaction = i;
                }
            }
            uint64_t rDelta_overReactionTime = Simulator::Now().GetTimeStep() - ch.ack.ih.rinfo[maxRadioIndexOverReaction].ts;
            if (rDelta_overReactionTime < qp->mycc.m_idleTimeStamp && ch.ack.ih.rinfo[maxRadioIndexOverReaction].ts > qp->mycc.m_lastUpdateCongestTime) {
                qp->mycc.m_ratio = ch.ack.ih.rinfo[maxRadioIndexOverReaction].ratio;
                qp->mycc.m_idleTimeStamp = rDelta_overReactionTime;
                qp->mycc.m_rTs = ch.ack.ih.rinfo[maxRadioIndexOverReaction].ts*100;
//                qp->mycc.m_rIsOwn = ch.ack.isOwn;
                qp->mycc.m_max_rRate = ch.ack.ih.rinfo[maxRadioIndexOverReaction].maxRate;
                
            }else if(qp->mycc.m_idleTimeStamp == 0 && ch.ack.ih.rinfo[maxRadioIndexOverReaction].ts > qp->mycc.m_lastUpdateCongestTime){
                qp->mycc.m_ratio = ch.ack.ih.rinfo[maxRadioIndexOverReaction].ratio;
                qp->mycc.m_idleTimeStamp = rDelta_overReactionTime;
                qp->mycc.m_rTs = ch.ack.ih.rinfo[maxRadioIndexOverReaction].ts*100;
//                qp->mycc.m_rIsOwn = ch.ack.isOwn;
                qp->mycc.m_max_rRate = ch.ack.ih.rinfo[maxRadioIndexOverReaction].maxRate;

            }

        }
    }else{//当前窗口的大小大于上一个窗口的大小，防止对同一个空闲事件作出反应
        if (ch.ack.ih.hinfo.depthNum != 0) {//数据包携带了队列长度信息
            int maxDepthIndexOverReaction = 0;
            for (int i = 0; i < ch.ack.ih.hinfo.depthNum; ++i) { //获取最长的队列的索引
                if (ch.ack.ih.dinfo[i].depth > ch.ack.ih.dinfo[maxDepthIndexOverReaction].depth) {
                    maxDepthIndexOverReaction = i;
                }
            }
            // uint64_t dDelta_overReactionTime = Simulator::Now().GetTimeStep() - ch.ack.ih.dinfo[maxDepthIndexOverReaction].ts;
            int64_t dDelta_overReactionTime = Simulator::Now().GetTimeStep()%((1<<24)*100) - 100*ch.ack.ih.dinfo[maxDepthIndexOverReaction].ts;
            if (dDelta_overReactionTime<0)
            {
                dDelta_overReactionTime += (1<<24)*100;
            }
            if (dDelta_overReactionTime < qp->mycc.m_congestTimeStamp && ch.ack.ih.dinfo[maxDepthIndexOverReaction].ts > qp->mycc.m_idleTimeStamp) {//
                qp->mycc.m_depth = ch.ack.ih.dinfo[maxDepthIndexOverReaction].depth;
                qp->mycc.m_congestTimeStamp = dDelta_overReactionTime;
                qp->mycc.m_dTs = ch.ack.ih.dinfo[maxDepthIndexOverReaction].ts*100;
//                qp->mycc.m_dIsOwn = ch.ack.isOwn;
                qp->mycc.m_max_dRate = ch.ack.ih.dinfo[maxDepthIndexOverReaction].maxRate;

            }else if(qp->mycc.m_congestTimeStamp == 0 && ch.ack.ih.dinfo[maxDepthIndexOverReaction].ts > qp->mycc.m_idleTimeStamp){
                qp->mycc.m_depth = ch.ack.ih.dinfo[maxDepthIndexOverReaction].depth;
                qp->mycc.m_congestTimeStamp = dDelta_overReactionTime;
                qp->mycc.m_dTs = ch.ack.ih.dinfo[maxDepthIndexOverReaction].ts*100;
//                qp->mycc.m_dIsOwn = ch.ack.isOwn;
                qp->mycc.m_max_dRate = ch.ack.ih.dinfo[maxDepthIndexOverReaction].maxRate;

            }
        }else if(ch.ack.ih.hinfo.ratioNum != 0 && ch.ack.ih.hinfo.depthNum == 0){//数据包只携带了速率比值信息
            int maxRadioIndexOverReaction = 0;
            for (int i = 0; i < ch.ack.ih.hinfo.ratioNum; ++i) { //获取最长的队列的索引
                if (ch.ack.ih.rinfo[i].ratio > ch.ack.ih.rinfo[maxRadioIndexOverReaction].ratio) {
                    maxRadioIndexOverReaction = i;
                }
            }
            // uint64_t rDelta_overReactionTime = Simulator::Now().GetTimeStep() - ch.ack.ih.rinfo[maxRadioIndexOverReaction].ts;
            int64_t rDelta_overReactionTime = Simulator::Now().GetTimeStep()%((1<<24)*100) - 100*ch.ack.ih.dinfo[maxRadioIndexOverReaction].ts;
            if (rDelta_overReactionTime<0)
            {
                rDelta_overReactionTime += (1<<24)*100;
            }
            if (ch.ack.flags == 1 && rDelta_overReactionTime < qp->mycc.m_idleTimeStamp && ch.ack.ih.rinfo[maxRadioIndexOverReaction].ts > qp->mycc.m_lastUpdateTime + 0.25*qp->m_baseRtt) {
                qp->mycc.m_ratio = ch.ack.ih.rinfo[maxRadioIndexOverReaction].ratio;
                qp->mycc.m_idleTimeStamp = rDelta_overReactionTime;
                qp->mycc.m_rTs = ch.ack.ih.rinfo[maxRadioIndexOverReaction].ts*100;
//                qp->mycc.m_rIsOwn = ch.ack.isOwn;
                qp->mycc.m_max_rRate = ch.ack.ih.rinfo[maxRadioIndexOverReaction].maxRate;

            }else if(qp->mycc.m_idleTimeStamp == 0 && ch.ack.flags == 1 && ch.ack.ih.rinfo[maxRadioIndexOverReaction].ts > qp->mycc.m_lastUpdateTime + 0.25*qp->m_baseRtt){
                qp->mycc.m_ratio = ch.ack.ih.rinfo[maxRadioIndexOverReaction].ratio;
                qp->mycc.m_idleTimeStamp = rDelta_overReactionTime;
                qp->mycc.m_rTs = ch.ack.ih.rinfo[maxRadioIndexOverReaction].ts*100;
//                qp->mycc.m_rIsOwn = ch.ack.isOwn;
                qp->mycc.m_max_rRate = ch.ack.ih.rinfo[maxRadioIndexOverReaction].maxRate;

            }

        }
    }
    
    if (ch.ack.flags == 0) {//自身的数据包并且一个完整的RTT窗口之后，更新发送速率
        std::cout<<"current depth:"<<qp->mycc.m_depth<<std::endl;
        std::cout<<"current ratio:"<<qp->mycc.m_ratio<<std::endl;

        DataRate new_rate;
        uint32_t ack_seq = ch.ack.seq;
        uint32_t next_seq = qp->snd_nxt;//snd_nxt为下一个发送的位置，它指向未发送但可以发送的第一个字节的序列号。
        if (ack_seq > qp->mycc.m_lastUpdateSeq) { //一个完整RTT的窗口，此时更新发送速率
            //todo:计算速率
            if ((qp->mycc.m_dTs != 0 || qp->mycc.m_rTs != 0) && (qp->mycc.m_dTs > qp->mycc.m_rTs || qp->mycc.m_rTs-qp->mycc.m_dTs>1000*1000000)) {//在一个窗口内拥塞事件最后发生
                qp->mycc.m_lastUpdateTime = Simulator::Now().GetTimeStep();
                qp->mycc.m_lastUpdateSeq = next_seq;
                //1std::cout<<"next_seq:"<<next_seq<<std::endl;
                qp->mycc.m_lastWinSize = qp->mycc.m_currentWinSize;
                qp->mycc.m_lastUpdateCongestTime = qp->mycc.m_dTs;

                //1std::cout<<"next_seq:"<<next_seq<<std::endl;
                // xiugai
                double alpha = qp->mycc.m_depth/(qp->mycc.m_depth + (qp->mycc.m_max_dRate * qp->m_baseRtt)/8000);
                qp->mycc.m_currentWinSize = qp->mycc.m_currentWinSize * (1-alpha);
                new_rate = qp->m_rate * (1-alpha);
                //更新速率
                if (new_rate < m_minRate)
                    new_rate = m_minRate;
                if (new_rate > qp->m_max_rate)
                    new_rate = qp->m_max_rate;
                qp->m_rate = new_rate;
                //1std::cout<<"***************alpha***********************:"<<alpha<<std::endl;
                
                //重置下面的变量
                qp->mycc.m_congestTimeStamp = 0;
                qp->mycc.m_idleTimeStamp = 0;//节点空闲发生到接收到该数据包的目前窗口为止最小的时间
//                    qp->mycc.m_dIsOwn = 3;
//                    qp->mycc.m_rIsOwn = 3;
                qp->mycc.m_depth = 0;
                qp->mycc.m_ratio = 10000;
                qp->mycc.m_dTs = 0;//发生拥塞时的时间
                qp->mycc.m_rTs = 0;


                
            }else if((qp->mycc.m_dTs != 0 || qp->mycc.m_rTs != 0) && (qp->mycc.m_dTs < qp->mycc.m_rTs || qp->mycc.m_dTs-qp->mycc.m_rTs>1000*1000000)){//在一个窗口内空闲事件最后发生
                qp->mycc.m_lastUpdateTime = Simulator::Now().GetTimeStep();
                qp->mycc.m_lastUpdateSeq = next_seq;
                qp->mycc.m_lastWinSize = qp->mycc.m_currentWinSize;
                qp->mycc.m_lastUpdateCongestTime = qp->mycc.m_dTs;
                qp->mycc.m_currentWinSize = (double)qp->mycc.m_currentWinSize/((double)qp->mycc.m_ratio/10000)+m_rai.GetBitRate()*qp->m_baseRtt;
                //重置下面的变量
                qp->mycc.m_congestTimeStamp = 0;
                qp->mycc.m_idleTimeStamp = 0;//节点空闲发生到接收到该数据包的目前窗口为止最小的时间
//                    qp->mycc.m_dIsOwn = 3;
//                    qp->mycc.m_rIsOwn = 3;
                qp->mycc.m_depth = 0;
                qp->mycc.m_ratio = 10000;
                qp->mycc.m_dTs = 0;//发生拥塞时的时间
                qp->mycc.m_rTs = 0;
                
                new_rate = qp->mycc.m_currentWinSize / qp->m_baseRtt;
                if (new_rate < m_minRate)
                    new_rate = m_minRate;
                if (new_rate > qp->m_max_rate)
                    new_rate = qp->m_max_rate;
                qp->m_rate = new_rate;
            }
        }
    }
    //1std::cout<< "node:" << m_node->GetId()<<"change windowsSIZE:"<<qp->mycc.m_currentWinSize<<std::endl;
}






/***********************
 * High Precision CC
 ***********************/
void RdmaHw::HandleAckHp(Ptr<RdmaQueuePair> qp, Ptr<Packet> p, CustomHeader &ch){
    uint32_t ack_seq = ch.ack.seq;
    // update rate
    if (ack_seq > qp->hp.m_lastUpdateSeq){ // if full RTT feedback is ready, do full update
        UpdateRateHp(qp, p, ch, false);
    }else{ // do fast react
        FastReactHp(qp, p, ch);
    }
}

void RdmaHw::UpdateRateHp(Ptr<RdmaQueuePair> qp, Ptr<Packet> p, CustomHeader &ch, bool fast_react){
    uint32_t next_seq = qp->snd_nxt;
    bool print = !fast_react || true;
    if (qp->hp.m_lastUpdateSeq == 0){ // first RTT
        qp->hp.m_lastUpdateSeq = next_seq;
        // store INT
        IntHeader &ih = ch.ack.ih;
        NS_ASSERT(ih.nhop <= IntHeader::maxHop);
        for (uint32_t i = 0; i < ih.nhop; i++)
            qp->hp.hop[i] = ih.hop[i];
        #if PRINT_LOG
        if (print){
            printf("%lu %s %08x %08x %u %u [%u,%u,%u]", Simulator::Now().GetTimeStep(), fast_react? "fast" : "update", qp->sip.Get(), qp->dip.Get(), qp->sport, qp->dport, qp->hp.m_lastUpdateSeq, ch.ack.seq, next_seq);
            for (uint32_t i = 0; i < ih.nhop; i++)
                printf(" %u %lu %lu", ih.hop[i].GetQlen(), ih.hop[i].GetBytes(), ih.hop[i].GetTime());
            printf("\n");
        }
        #endif
    }else {
        // check packet INT
        IntHeader &ih = ch.ack.ih;
        if (ih.nhop <= IntHeader::maxHop){
            double max_c = 0;
            bool inStable = false;
            #if PRINT_LOG
            if (print)
                printf("%lu %s %08x %08x %u %u [%u,%u,%u]", Simulator::Now().GetTimeStep(), fast_react? "fast" : "update", qp->sip.Get(), qp->dip.Get(), qp->sport, qp->dport, qp->hp.m_lastUpdateSeq, ch.ack.seq, next_seq);
            #endif
            // check each hop
            double U = 0;
            uint64_t dt = 0;
            bool updated[IntHeader::maxHop] = {false}, updated_any = false;
            NS_ASSERT(ih.nhop <= IntHeader::maxHop);
            for (uint32_t i = 0; i < ih.nhop; i++){
                if (m_sampleFeedback){
                    if (ih.hop[i].GetQlen() == 0 && fast_react)
                        continue;
                }
                updated[i] = updated_any = true;
                #if PRINT_LOG
                if (print)
                    printf(" %u(%u) %lu(%lu) %lu(%lu)", ih.hop[i].GetQlen(), qp->hp.hop[i].GetQlen(), ih.hop[i].GetBytes(), qp->hp.hop[i].GetBytes(), ih.hop[i].GetTime(), qp->hp.hop[i].GetTime());
                #endif
                uint64_t tau = ih.hop[i].GetTimeDelta(qp->hp.hop[i]);;
                double duration = tau * 1e-9;
                double txRate = (ih.hop[i].GetBytesDelta(qp->hp.hop[i])) * 8 / duration;
                double u = txRate / ih.hop[i].GetLineRate() + (double)std::min(ih.hop[i].GetQlen(), qp->hp.hop[i].GetQlen()) * qp->m_max_rate.GetBitRate() / ih.hop[i].GetLineRate() /qp->m_win;//获取的是该窗口内收到的队列长度最小
                #if PRINT_LOG
                if (print)
                    printf(" %.3lf %.3lf", txRate, u);
                #endif
                if (!m_multipleRate){
                    // for aggregate (single R)
                    if (u > U){
                        U = u;
                        dt = tau;
                    }
                }else {
                    // for per hop (per hop R)
                    if (tau > qp->m_baseRtt)
                        tau = qp->m_baseRtt;
                    qp->hp.hopState[i].u = (qp->hp.hopState[i].u * (qp->m_baseRtt - tau) + u * tau) / double(qp->m_baseRtt);
                }
                qp->hp.hop[i] = ih.hop[i];
            }

            DataRate new_rate;
            int32_t new_incStage;
            DataRate new_rate_per_hop[IntHeader::maxHop];
            int32_t new_incStage_per_hop[IntHeader::maxHop];
            if (!m_multipleRate){
                // for aggregate (single R)
                if (updated_any){
                    if (dt > qp->m_baseRtt)
                        dt = qp->m_baseRtt;
                    qp->hp.u = (qp->hp.u * (qp->m_baseRtt - dt) + U * dt) / double(qp->m_baseRtt);
                    max_c = qp->hp.u / m_targetUtil;

                    if (max_c >= 1 || qp->hp.m_incStage >= m_miThresh){
                        new_rate = qp->hp.m_curRate / max_c + m_rai;
                        new_incStage = 0;
                    }else{
                        new_rate = qp->hp.m_curRate + m_rai;
                        new_incStage = qp->hp.m_incStage+1;
                    }
                    if (new_rate < m_minRate)
                        new_rate = m_minRate;
                    if (new_rate > qp->m_max_rate)
                        new_rate = qp->m_max_rate;
                    #if PRINT_LOG
                    if (print)
                        printf(" u=%.6lf U=%.3lf dt=%u max_c=%.3lf", qp->hp.u, U, dt, max_c);
                    #endif
                    #if PRINT_LOG
                    if (print)
                        printf(" rate:%.3lf->%.3lf\n", qp->hp.m_curRate.GetBitRate()*1e-9, new_rate.GetBitRate()*1e-9);
                    #endif
                }
            }else{
                // for per hop (per hop R)
                new_rate = qp->m_max_rate;
                for (uint32_t i = 0; i < ih.nhop; i++){
                    if (updated[i]){
                        double c = qp->hp.hopState[i].u / m_targetUtil;
                        if (c >= 1 || qp->hp.hopState[i].incStage >= m_miThresh){
                            new_rate_per_hop[i] = qp->hp.hopState[i].Rc / c + m_rai;
                            new_incStage_per_hop[i] = 0;
                        }else{
                            new_rate_per_hop[i] = qp->hp.hopState[i].Rc + m_rai;
                            new_incStage_per_hop[i] = qp->hp.hopState[i].incStage+1;
                        }
                        // bound rate
                        if (new_rate_per_hop[i] < m_minRate)
                            new_rate_per_hop[i] = m_minRate;
                        if (new_rate_per_hop[i] > qp->m_max_rate)
                            new_rate_per_hop[i] = qp->m_max_rate;
                        // find min new_rate
                        if (new_rate_per_hop[i] < new_rate)
                            new_rate = new_rate_per_hop[i];
                        #if PRINT_LOG
                        if (print)
                            printf(" [%u]u=%.6lf c=%.3lf", i, qp->hp.hopState[i].u, c);
                        #endif
                        #if PRINT_LOG
                        if (print)
                            printf(" %.3lf->%.3lf", qp->hp.hopState[i].Rc.GetBitRate()*1e-9, new_rate.GetBitRate()*1e-9);
                        #endif
                    }else{
                        if (qp->hp.hopState[i].Rc < new_rate)
                            new_rate = qp->hp.hopState[i].Rc;
                    }
                }
                #if PRINT_LOG
                printf("\n");
                #endif
            }
            if (updated_any)
                ChangeRate(qp, new_rate);
            if (!fast_react){
                if (updated_any){
                    qp->hp.m_curRate = new_rate;
                    qp->hp.m_incStage = new_incStage;
                }
                if (m_multipleRate){
                    // for per hop (per hop R)
                    for (uint32_t i = 0; i < ih.nhop; i++){
                        if (updated[i]){
                            qp->hp.hopState[i].Rc = new_rate_per_hop[i];
                            qp->hp.hopState[i].incStage = new_incStage_per_hop[i];
                        }
                    }
                }
            }
        }
        if (!fast_react){
            if (next_seq > qp->hp.m_lastUpdateSeq)
                qp->hp.m_lastUpdateSeq = next_seq; //+ rand() % 2 * m_mtu;
        }
    }
}

void RdmaHw::FastReactHp(Ptr<RdmaQueuePair> qp, Ptr<Packet> p, CustomHeader &ch){
    if (m_fast_react)
        UpdateRateHp(qp, p, ch, true);
}

/**********************
 * TIMELY
 *********************/
void RdmaHw::HandleAckTimely(Ptr<RdmaQueuePair> qp, Ptr<Packet> p, CustomHeader &ch){
    uint32_t ack_seq = ch.ack.seq;
    // update rate
    if (ack_seq > qp->tmly.m_lastUpdateSeq){ // if full RTT feedback is ready, do full update
        UpdateRateTimely(qp, p, ch, false);
    }else{ // do fast react
        FastReactTimely(qp, p, ch);
    }
}
void RdmaHw::UpdateRateTimely(Ptr<RdmaQueuePair> qp, Ptr<Packet> p, CustomHeader &ch, bool us){
    uint32_t next_seq = qp->snd_nxt;
    uint64_t rtt = Simulator::Now().GetTimeStep() - ch.ack.ih.ts;
    bool print = !us;
    if (qp->tmly.m_lastUpdateSeq != 0){ // not first RTT
        int64_t new_rtt_diff = (int64_t)rtt - (int64_t)qp->tmly.lastRtt;
        double rtt_diff = (1 - m_tmly_alpha) * qp->tmly.rttDiff + m_tmly_alpha * new_rtt_diff;
        double gradient = rtt_diff / m_tmly_minRtt;
        bool inc = false;
        double c = 0;
        #if PRINT_LOG
        if (print)
            printf("%lu node:%u rtt:%lu rttDiff:%.0lf gradient:%.3lf rate:%.3lf", Simulator::Now().GetTimeStep(), m_node->GetId(), rtt, rtt_diff, gradient, qp->tmly.m_curRate.GetBitRate() * 1e-9);
        #endif
        if (rtt < m_tmly_TLow){
            inc = true;
        }else if (rtt > m_tmly_THigh){
            c = 1 - m_tmly_beta * (1 - (double)m_tmly_THigh / rtt);
            inc = false;
        }else if (gradient <= 0){
            inc = true;
        }else{
            c = 1 - m_tmly_beta * gradient;
            if (c < 0)
                c = 0;
            inc = false;
        }
        if (inc){
            if (qp->tmly.m_incStage < 5){
                qp->m_rate = qp->tmly.m_curRate + m_rai;
            }else{
                qp->m_rate = qp->tmly.m_curRate + m_rhai;
            }
            if (qp->m_rate > qp->m_max_rate)
                qp->m_rate = qp->m_max_rate;
            if (!us){
                qp->tmly.m_curRate = qp->m_rate;
                qp->tmly.m_incStage++;
                qp->tmly.rttDiff = rtt_diff;
            }
        }else{
            qp->m_rate = std::max(m_minRate, qp->tmly.m_curRate * c);
            if (!us){
                qp->tmly.m_curRate = qp->m_rate;
                qp->tmly.m_incStage = 0;
                qp->tmly.rttDiff = rtt_diff;
            }
        }
        #if PRINT_LOG
        if (print){
            printf(" %c %.3lf\n", inc? '^':'v', qp->m_rate.GetBitRate() * 1e-9);
        }
        #endif
    }
    if (!us && next_seq > qp->tmly.m_lastUpdateSeq){
        qp->tmly.m_lastUpdateSeq = next_seq;
        // update
        qp->tmly.lastRtt = rtt;
    }
}
void RdmaHw::FastReactTimely(Ptr<RdmaQueuePair> qp, Ptr<Packet> p, CustomHeader &ch){
}

/**********************
 * DCTCP
 *********************/
void RdmaHw::HandleAckDctcp(Ptr<RdmaQueuePair> qp, Ptr<Packet> p, CustomHeader &ch){
    uint32_t ack_seq = ch.ack.seq;
    uint8_t cnp = (ch.ack.flags >> qbbHeader::FLAG_CNP) & 1;
    bool new_batch = false;

    // update alpha
    qp->dctcp.m_ecnCnt += (cnp > 0);
    if (ack_seq > qp->dctcp.m_lastUpdateSeq){ // if full RTT feedback is ready, do alpha update
        #if PRINT_LOG
        printf("%lu %s %08x %08x %u %u [%u,%u,%u] %.3lf->", Simulator::Now().GetTimeStep(), "alpha", qp->sip.Get(), qp->dip.Get(), qp->sport, qp->dport, qp->dctcp.m_lastUpdateSeq, ch.ack.seq, qp->snd_nxt, qp->dctcp.m_alpha);
        #endif
        new_batch = true;
        if (qp->dctcp.m_lastUpdateSeq == 0){ // first RTT
            qp->dctcp.m_lastUpdateSeq = qp->snd_nxt;
            qp->dctcp.m_batchSizeOfAlpha = qp->snd_nxt / m_mtu + 1;
        }else {
            double frac = std::min(1.0, double(qp->dctcp.m_ecnCnt) / qp->dctcp.m_batchSizeOfAlpha);
            qp->dctcp.m_alpha = (1 - m_g) * qp->dctcp.m_alpha + m_g * frac;
            qp->dctcp.m_lastUpdateSeq = qp->snd_nxt;
            qp->dctcp.m_ecnCnt = 0;
            qp->dctcp.m_batchSizeOfAlpha = (qp->snd_nxt - ack_seq) / m_mtu + 1;
            #if PRINT_LOG
            printf("%.3lf F:%.3lf", qp->dctcp.m_alpha, frac);
            #endif
        }
        #if PRINT_LOG
        printf("\n");
        #endif
    }

    // check cwr exit
    if (qp->dctcp.m_caState == 1){
        if (ack_seq > qp->dctcp.m_highSeq)
            qp->dctcp.m_caState = 0;
    }

    // check if need to reduce rate: ECN and not in CWR
    if (cnp && qp->dctcp.m_caState == 0){
        #if PRINT_LOG
        printf("%lu %s %08x %08x %u %u %.3lf->", Simulator::Now().GetTimeStep(), "rate", qp->sip.Get(), qp->dip.Get(), qp->sport, qp->dport, qp->m_rate.GetBitRate()*1e-9);
        #endif
        qp->m_rate = std::max(m_minRate, qp->m_rate * (1 - qp->dctcp.m_alpha / 2));
        #if PRINT_LOG
        printf("%.3lf\n", qp->m_rate.GetBitRate() * 1e-9);
        #endif
        qp->dctcp.m_caState = 1;
        qp->dctcp.m_highSeq = qp->snd_nxt;
    }

    // additive inc
    if (qp->dctcp.m_caState == 0 && new_batch)
        qp->m_rate = std::min(qp->m_max_rate, qp->m_rate + m_dctcp_rai);
}

/*********************
 * HPCC-PINT
 ********************/
void RdmaHw::SetPintSmplThresh(double p){
       pint_smpl_thresh = (uint32_t)(65536 * p);
}
void RdmaHw::HandleAckHpPint(Ptr<RdmaQueuePair> qp, Ptr<Packet> p, CustomHeader &ch){
       uint32_t ack_seq = ch.ack.seq;
       if (rand() % 65536 >= pint_smpl_thresh)
               return;
       // update rate
       if (ack_seq > qp->hpccPint.m_lastUpdateSeq){ // if full RTT feedback is ready, do full update
               UpdateRateHpPint(qp, p, ch, false);
       }else{ // do fast react
               UpdateRateHpPint(qp, p, ch, true);
       }
}

void RdmaHw::UpdateRateHpPint(Ptr<RdmaQueuePair> qp, Ptr<Packet> p, CustomHeader &ch, bool fast_react){
       uint32_t next_seq = qp->snd_nxt;
       if (qp->hpccPint.m_lastUpdateSeq == 0){ // first RTT
               qp->hpccPint.m_lastUpdateSeq = next_seq;
       }else {
               // check packet INT
               IntHeader &ih = ch.ack.ih;
               double U = Pint::decode_u(ih.GetPower());

               DataRate new_rate;
               int32_t new_incStage;
               double max_c = U / m_targetUtil;

               if (max_c >= 1 || qp->hpccPint.m_incStage >= m_miThresh){
                       new_rate = qp->hpccPint.m_curRate / max_c + m_rai;
                       new_incStage = 0;
               }else{
                       new_rate = qp->hpccPint.m_curRate + m_rai;
                       new_incStage = qp->hpccPint.m_incStage+1;
               }
               if (new_rate < m_minRate)
                       new_rate = m_minRate;
               if (new_rate > qp->m_max_rate)
                       new_rate = qp->m_max_rate;
               ChangeRate(qp, new_rate);
               if (!fast_react){
                       qp->hpccPint.m_curRate = new_rate;
                       qp->hpccPint.m_incStage = new_incStage;
               }
               if (!fast_react){
                       if (next_seq > qp->hpccPint.m_lastUpdateSeq)
                               qp->hpccPint.m_lastUpdateSeq = next_seq; //+ rand() % 2 * m_mtu;
               }
       }
}

}
