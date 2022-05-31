//#include "cpu.h"
#include <string.h>
#include <stdint.h>
#include "ip.h"
#include "stm32_eth.h"
#include "stm_util.h"
#include "cpu.h"

static uint16_t my_tid,rmt_tid; // dynamic ports
static uint16_t seq; // data sequence

#define MO_DONE 0
#define MO_CMD 1	// cmd sent
#define MO_DATA 2	// cmd confirmed, in data
#define MO_FIN 3	// last segment sent, waiting for ACK
#define MO_ERROR 4
static char mode;
static uint8_t retries; // how many send retries remain
static short rx_sz; // size of last RX data or -1

static char tmac[6];
static char tfile[64];
static uint32_t rmt_ip;
//extern struct eth_iodef eth;
extern volatile uint32_t tick;
static uint32_t tmo;
static char *rtgt; // read target buffer

#define  TF_RRQ 1
#define  TF_WRQ 2
#define  TF_DATA 3
#define  TF_ACK 4
#define  TF_ERROR 5

static void tftp_send(struct packet *p,int sz)
{
	p->ip.daddr = rmt_ip;
	memcpy(p->dst.addr,tmac,sizeof(tmac));
	p->ip.protocol = 17; // UDP
	fill_ip_hdr(p,sz);
	p->ip.udp.sport = swap16(my_tid);
	p->ip.udp.dport = swap16(rmt_tid);
	p->ip.udp.len = swap16(sz);
	send_packet(p);
}

const short retry_tab[] = { 500,700,1000,0 };

void tftp_regcb(int port);
void tftp_req(u32_t host,u8_t *mac,const char *file,char *rbuf)
{
	if (++my_tid<1024) my_tid = 1024;
	tftp_regcb(my_tid);
	rmt_tid = 69;
	rmt_ip = host;
	rtgt = rbuf;
	memcpy(tmac,mac,sizeof(tmac));
	strncpy(tfile,file,sizeof(tfile)-1);

	tmo = tick+1000; // any reasonable time
	rx_sz = 0; // force tftp_next to send
	seq = 0;
	mode = MO_CMD;
}

int tftp_next(char *buf,int ssz)
{
	if (mode == MO_DONE) return 0;
	if (mode != MO_DATA && mode != MO_CMD) {
		mode = MO_ERROR;
		return -3;
	}
	if (rx_sz>=0 || seq_after(tick,tmo)) { // send reply
		if (rx_sz<0) {
#if 1
			jprintf("TFTP ACK %d %d %d\n",rx_sz,seq,retries);
#endif
			if (!retry_tab[retries]) {
				mode = MO_ERROR; // no more retries
				jprintf("tmo %d %d\n",tick,tmo);
				return -2;
			}
		}
		struct packet p;
		memset(&p,0,sizeof(p));
		uint8_t *s = p.ip.udp.data;
		int sz = 0;
		if (mode == MO_CMD) {
			s[1] = rtgt ? TF_RRQ : TF_WRQ;
			s += 2;
			strcpy(s,tfile);
			s += strlen(s)+1;
			strcpy(s,"octet");
			s += strlen(s)+1;
			sz = s-p.ip.udp.data;
			rx_sz = -1; // we just abused rx_sz
		} else {
			s[1] = ssz>=0 ? TF_DATA:TF_ACK;
			((uint16_t*)s)[1] = swap16(seq);
			if (ssz > 0) {
				memcpy(p.ip.udp.data+4,buf,ssz);
				if (ssz < 512) mode = MO_FIN;
			}
			sz = 4+(ssz>0 ? ssz : 0);
		}
		tftp_send(&p,8+sz);
		tmo = tick+retry_tab[retries++];
	}
	if (rx_sz>=0) {
		int sz = rx_sz;
		if (ssz<0) { 
			rtgt = buf; // replace buffer
			if (rx_sz<512) {
				mode = MO_DONE;
				jprintf("done\n");
			}
		}
		if (mode == MO_FIN) {
			mode = MO_DONE;
			jprintf("wdone\n");
			return 0;
		}
		rx_sz = -1; 
		return sz;
	}
	return -1;
}

void tftp_rx(struct packet *pkt)
{
	const char *s = pkt->ip.udp.data;
	if (mode != MO_DATA && mode != MO_CMD && mode != MO_FIN) return;

	int sz = swap16(pkt->ip.udp.len) - 8 - 4;
	uint16_t exp = seq+(rtgt!=NULL);
	uint8_t exp_type = rtgt ? TF_DATA:TF_ACK;
	uint16_t id = swap16(*(uint16_t*)(s+2));
	if (s[0] || s[1]!=exp_type || sz > 512 || sz < 0) {
		jprintf("TFTP ERR RX %d %X\n",sz,s[1]&255);
		mode = MO_ERROR; return;
	}
	if (id!=exp) {
		jprintf("TFTP OOS %d %d vs %d\n",sz,id,exp);
		return; // ignore out of seq (dup probably)
	}
	mode = MO_DATA;
	rmt_tid = swap16(pkt->ip.udp.sport);
	if (rtgt) memcpy(rtgt,s+4,512);
	rx_sz = sz; seq = exp+(rtgt==NULL);
	retries = 0;
}
