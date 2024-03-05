#include <ns3/hash.h>
#include <ns3/uinteger.h>
#include <ns3/seq-ts-header.h>
#include <ns3/udp-header.h>
#include <ns3/ipv4-header.h>
#include <ns3/simulator.h>
#include "ns3/ppp-header.h"
#include "rdma-queue-pair.h"

namespace ns3 {

/**************************
 * RdmaQueuePair
 *************************/
TypeId RdmaQueuePair::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::RdmaQueuePair")
        .SetParent<Object> ()
        ;
    return tid;
}

RdmaQueuePair::RdmaQueuePair(uint16_t pg, Ipv4Address _sip, Ipv4Address _dip, uint16_t _sport, uint16_t _dport){
    startTime = Simulator::Now();
    sip = _sip;
    dip = _dip;
    sport = _sport;
    dport = _dport;
    m_size = 0;
    snd_nxt = snd_una = 0;
    m_pg = pg;
    m_ipid = 0;
    m_win = 0;
    m_baseRtt = 0;
    m_max_rate = 0;
    m_var_win = false;
    m_rate = 0;
    m_nextAvail = Time(0);
    mlx.m_alpha = 1;
    mlx.m_alpha_cnp_arrived = false;
    mlx.m_first_cnp = true;
    mlx.m_decrease_cnp_arrived = false;
    mlx.m_rpTimeStage = 0;
    
    
    mycc.m_congestTimeStamp = 0;//节点拥塞发生到接收到该数据包的目前窗口为止最小的时间
    mycc.m_idleTimeStamp = 0;//节点空闲发生到接收到该数据包的目前窗口为止最小的时间
    mycc.m_lastUpdateSeq = 0;

    mycc.m_lastUpdateTime = 0;//上一次更新窗口的时间
    mycc.m_lastUpdateCongestTime = 0;//上一次根据该数据包更新窗口时的数据包记录的拥塞发生的时间
    mycc.m_lastUpdateIdleTime = 0;//上一次根据该数据包更新窗口时的数据包记录的空闲发生的时间

    mycc.m_currentWinSize = 0;//当前窗口的大小
    mycc.m_lastWinSize = 0;//上一个窗口的大小

    mycc.m_congestTimeStamp = 0;//节点拥塞发生到接收到该数据包的目前窗口为止最小的时间
    mycc.m_idleTimeStamp = 0;//节点空闲发生到接收到该数据包的目前窗口为止最小的时间
    mycc.m_depth = 0;
    mycc.m_ratio = 10000;
    mycc.m_rTs = 0;//队列空闲时的时间
    mycc.m_dTs = 0;//发生拥塞时的时间
    mycc.m_max_dRate = 1; // 缺省队列返回最大速率时间
    mycc.m_max_rRate = 1; // 缺省比值返回最大速率时间
    
    
    hp.m_lastUpdateSeq = 0;
    for (uint32_t i = 0; i < sizeof(hp.keep) / sizeof(hp.keep[0]); i++)
        hp.keep[i] = 0;
    hp.m_incStage = 0;
    hp.m_lastGap = 0;
    hp.u = 1;
    for (uint32_t i = 0; i < IntHeader::maxHop; i++){
        hp.hopState[i].u = 1;
        hp.hopState[i].incStage = 0;
    }

    tmly.m_lastUpdateSeq = 0;
    tmly.m_incStage = 0;
    tmly.lastRtt = 0;
    tmly.rttDiff = 0;

    dctcp.m_lastUpdateSeq = 0;
    dctcp.m_caState = 0;
    dctcp.m_highSeq = 0;
    dctcp.m_alpha = 1;
    dctcp.m_ecnCnt = 0;
    dctcp.m_batchSizeOfAlpha = 0;

    hpccPint.m_lastUpdateSeq = 0;
    hpccPint.m_incStage = 0;
}

void RdmaQueuePair::SetSize(uint64_t size){
    m_size = size;
}

void RdmaQueuePair::SetWin(uint32_t win){
    m_win = win;
}

void RdmaQueuePair::SetBaseRtt(uint64_t baseRtt){
    m_baseRtt = baseRtt;
}

void RdmaQueuePair::SetVarWin(bool v){
    m_var_win = v;
}

void RdmaQueuePair::SetAppNotifyCallback(Callback<void> notifyAppFinish){
    m_notifyAppFinish = notifyAppFinish;
}

uint64_t RdmaQueuePair::GetBytesLeft(){
    return m_size >= snd_nxt ? m_size - snd_nxt : 0;
}

uint32_t RdmaQueuePair::GetHash(void){
    union{
        struct {
            uint32_t sip, dip;
            uint16_t sport, dport;
        };
        char c[12];
    } buf;
    buf.sip = sip.Get();
    buf.dip = dip.Get();
    buf.sport = sport;
    buf.dport = dport;
    return Hash32(buf.c, 12);
}

void RdmaQueuePair::Acknowledge(uint64_t ack){
    if (ack > snd_una){
        snd_una = ack;
    }
}

uint64_t RdmaQueuePair::GetOnTheFly(){
    return snd_nxt - snd_una;
}

bool RdmaQueuePair::IsWinBound(){
    uint64_t w = GetWin();
    return w != 0 && GetOnTheFly() >= w;
}

uint64_t RdmaQueuePair::GetWin(){
    if (m_win == 0)
        return 0;
    uint64_t w;
    if (m_var_win){
        w = m_win * m_rate.GetBitRate() / m_max_rate.GetBitRate();
        if (w == 0)
            w = 1; // must > 0
    }else{
        w = m_win;
    }
    return w;
}

uint64_t RdmaQueuePair::HpGetCurWin(){
    if (m_win == 0)
        return 0;
    uint64_t w;
    if (m_var_win){
        w = m_win * hp.m_curRate.GetBitRate() / m_max_rate.GetBitRate();
        if (w == 0)
            w = 1; // must > 0
    }else{
        w = m_win;
    }
    return w;
}

bool RdmaQueuePair::IsFinished(){
    return snd_una >= m_size;
}

/*********************
 * RdmaRxQueuePair
 ********************/
TypeId RdmaRxQueuePair::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::RdmaRxQueuePair")
        .SetParent<Object> ()
        ;
    return tid;
}

RdmaRxQueuePair::RdmaRxQueuePair(){
    sip = dip = sport = dport = 0;
    m_ipid = 0;
    ReceiverNextExpectedSeq = 0;
    m_nackTimer = Time(0);
    m_milestone_rx = 0;
    m_lastNACK = 0;
}

uint32_t RdmaRxQueuePair::GetHash(void){
    union{
        struct {
            uint32_t sip, dip;
            uint16_t sport, dport;
        };
        char c[12];
    } buf;
    buf.sip = sip;
    buf.dip = dip;
    buf.sport = sport;
    buf.dport = dport;
    return Hash32(buf.c, 12);
}

/*********************
 * RdmaQueuePairGroup
 ********************/
TypeId RdmaQueuePairGroup::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::RdmaQueuePairGroup")
        .SetParent<Object> ()
        ;
    return tid;
}

RdmaQueuePairGroup::RdmaQueuePairGroup(void){
}

uint32_t RdmaQueuePairGroup::GetN(void){
    return m_qps.size();
}

Ptr<RdmaQueuePair> RdmaQueuePairGroup::Get(uint32_t idx){
    return m_qps[idx];
}

Ptr<RdmaQueuePair> RdmaQueuePairGroup::operator[](uint32_t idx){
    return m_qps[idx];
}

void RdmaQueuePairGroup::AddQp(Ptr<RdmaQueuePair> qp){
    m_qps.push_back(qp);
}

#if 0
void RdmaQueuePairGroup::AddRxQp(Ptr<RdmaRxQueuePair> rxQp){
    m_rxQps.push_back(rxQp);
}
#endif

void RdmaQueuePairGroup::Clear(void){
    m_qps.clear();
}

}
