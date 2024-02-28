#include "int-header-niux.h"

namespace ns3 {

MyIntHeader::MyIntHeader() {
	hinfo.buf = 0;
	hinfo.totalLength = 9;
	for (int i = 0; i < idNum; ++i)
		iinfo[i].buf = 0;
	for (int i = 0; i < maxNum; ++i) {
		dinfo[i].buf[0] = 0;
		dinfo[i].buf[1] = 0;
		rinfo[i].buf[0] = 0;
		rinfo[i].buf[1] = 0;
	}
}

uint32_t MyIntHeader::GetStaticSize() {
	return sizeof(hinfo)+sizeof(idInfo)+sizeof(dinfo)+sizeof(rinfo);
}

void MyIntHeader::PushRoute(uint8_t _id, uint8_t _port) {
	if (hinfo.nodeNum < idNum)
		if (rand()%3 == 0)
			iinfo[hinfo.nodeNum++].Set(_id, _port);
}

int MyIntHeader::PushDepth(uint8_t _id, uint8_t _port, uint16_t _depth, uint32_t _ts, uint8_t _maxRate) {
	_depth = _depth / qlenUnit;
	_depth = (_depth < 0xffff)? _depth : 0xffff;
	
	if (_depth <= 0) {
		return -1;
	}

	if (hinfo.depthNum < maxNum) {
		dinfo[hinfo.depthNum++].Set(_id, _port, _depth, _ts, _maxRate);
		return 1;
	}
	else {
		uint16_t min_depth = dinfo[0].depth;
		int min_idx = 0;
		for (int i = 1; i < maxNum; ++i) {
			if (dinfo[i].depth < min_depth) {
				min_depth = dinfo[i].depth;
				min_idx = i;
			}
		}
		if (_depth > min_depth) {
			dinfo[min_idx].Set(_id, _port, _depth, _ts, _maxRate);
			return 1;
		}
		return 0;
	}
}

int MyIntHeader::PushRatio(uint8_t _id, uint8_t _port, uint16_t _ratio, uint32_t _ts, uint8_t _maxRate) {
	if (hinfo.ratioNum < maxNum) {
		rinfo[hinfo.ratioNum++].Set(_id, _port, _ratio, _ts, _maxRate);
		return 1;
	}
	else {
		uint16_t min_ratio = rinfo[0].ratio;
		int min_idx = 0;
		for (int i = 1; i < maxNum; ++i) {
			if (rinfo[i].ratio < min_ratio) {
				min_ratio = rinfo[i].ratio;
				min_idx = i;
			}
		}
		if (_ratio > min_ratio) {
			rinfo[min_idx].Set(_id, _port, _ratio, _ts, _maxRate);
			return 1;
		}
		return 0;
	}
}

void MyIntHeader::Serialize (Buffer::Iterator start) const{
	Buffer::Iterator i = start;
	i.WriteU16(hinfo.buf);
	for (int j = 0; j < idNum; ++j)
		i.WriteU16(iinfo[j].buf);
	for (int j = 0; j < maxNum; ++j) {
		i.WriteU32(dinfo[j].buf[0]);
		i.WriteU32(dinfo[j].buf[1]);
	}
	for (int j = 0; j < maxNum; ++j) {
		i.WriteU32(rinfo[j].buf[0]);
		i.WriteU32(rinfo[j].buf[1]);
	}
}

uint32_t MyIntHeader::Deserialize (Buffer::Iterator start){
	Buffer::Iterator i = start;
	hinfo.buf = i.ReadU16();
	for (int j = 0; j < idNum; ++j)
		iinfo[j].buf = i.ReadU16();
	for (int j = 0; j < maxNum; ++j) {
		dinfo[j].buf[0] = i.ReadU32();
		dinfo[j].buf[1] = i.ReadU32();
	}
	for (int j = 0; j < maxNum; ++j) {
		rinfo[j].buf[0] = i.ReadU32();
		rinfo[j].buf[1] = i.ReadU32();
	}
	return sizeof(hinfo)+sizeof(iinfo)+sizeof(dinfo)+sizeof(rinfo);
}

}
