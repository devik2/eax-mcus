#include "cpu.h"
#include <string.h>
#include "ip.h"
#include "stm32_eth.h"
#include "stm_util.h"

// Very basic IP/UDP support

#if 0
#define TRACE(msg, ...) jprintf (msg, ## __VA_ARGS__)
#define TRACE1(msg,arg) jprintf (msg " %d",arg)
#else
#define TRACE1(msg,arg)
#define TRACE(msg, ...) 
#endif

struct eth_iodef eth; // single global impl
static struct mac_addr current_mac;	// src mac of current request
volatile u32_t _my_ip;

static char tx_buff[0x600];
void send_packet(struct packet *p) 
{
	eth_wait();
	memcpy(tx_buff,&p->dst,p->len);
//	jprintf("send %d, %X %X",p->len,tx_buff,tx_buff[0]);
	eth_send(tx_buff,p->len);
}

unsigned short swap16(unsigned short s)
{
	return (s<<8)|(s>>8);
}

unsigned swap32(unsigned s)
{
	return (s<<24)|((s<<8)&0xFF0000)|((s>>8)&0xFF00)|(s>>24);
}

u16_t htons(u16_t x)
{
	return swap16(x);
}

u16_t ntohs(u16_t x)
{
	return swap16(x);
}

u32_t htonl(u32_t x)
{
	return swap32(x);
}

u32_t ntohl(u32_t x)
{
	return swap32(x);
}

u16_t checksum(u16_t *buffer, int size)
{
    unsigned cksum=0;
    while(size >1)
    {
        cksum+=*buffer++;
        size -=sizeof(u16_t);
    }
    if(size)
        cksum += *(u8_t*)buffer;

    cksum = (cksum >> 16) + (cksum & 0xffff);
    cksum += (cksum >>16);
    return (u16_t)(~cksum);
}

void fill_our_mac_src(struct packet *pkt)
{
	memcpy(&pkt->src,&eth.mac,sizeof(struct mac_addr));
}

// ip.daddr,dst,protocol must be filled
void fill_ip_hdr(struct packet *pkt,int len)
{
	fill_our_mac_src(pkt);
	pkt->len = 20+14+len;
	pkt->ethertype = 0x8;
	pkt->ip.tot_len = swap16(20+len); // IP + data
	pkt->ip.check = 0;
	pkt->ip.saddr = _my_ip;
	pkt->ip.ttl = 64;
	pkt->ip.ihl = 5;
	pkt->ip.version = 4;
	pkt->ip.id = 0;
	pkt->ip.tos = 0;
	pkt->ip.frag_off = 0;
	pkt->ip.check = checksum((short*)&pkt->ip,20);
}

static struct host_info *arp_qry;
extern u32_t tick;
static void arp_reply(struct arphdr *rply)
{
	if (!arp_qry) return;
	if (rply->ar_sip == arp_qry->ip) {
		arp_qry->mac = rply->ar_sha;
		arp_qry->tm = tick;
	}
}

static void process_arp(struct packet *pkt)
{
	struct arphdr *req = &pkt->arp;
	TRACE("ARP %X %X\n",req->ar_tip,_my_ip);

	if (req->ar_op == 0x200) arp_reply(req);
	if (req->ar_tip != _my_ip || req->ar_op != 0x100) return; 

	TRACE("it is our ARP\n",pkt->ethertype);

	struct packet reply;
	fill_our_mac_src(&reply);
	memcpy(&reply.dst,&req->ar_sha,sizeof(struct mac_addr));
	reply.ethertype = 0x608;
	memcpy(&reply.arp,req,sizeof(reply.arp)); // copy request as base
	reply.arp.ar_op = 0x200; // reply code
	memcpy(&reply.arp.ar_sha,&eth.mac,sizeof(struct mac_addr));
	memcpy(&reply.arp.ar_tha,&req->ar_sha,sizeof(struct mac_addr));
	reply.arp.ar_sip = _my_ip;
	reply.arp.ar_tip = req->ar_sip;
	reply.len = sizeof(struct arphdr)+14;
	send_packet(&reply);
}


static void prepare_ip_reply(const struct iphdr *ip,struct packet *reply)
{
	reply->ethertype = 0x8;
	reply->dst = current_mac;
	reply->src = *(struct mac_addr*)eth.mac;
	reply->ip.check = 0;
	unsigned tmp = ip->saddr; // because ip might == reply
	reply->ip.saddr = ip->daddr;
	reply->ip.daddr = tmp;
	reply->ip.ttl = 64;
	reply->ip.check = checksum((short*)&reply->ip,20);
}

static void unreach_icmp(struct iphdr *ip,unsigned err)
{
	if (ip->daddr == 0xffffffff) return; // don't send err to broadcasts
#define UNR_LEN (20+36)
	unsigned buf[(sizeof(struct packet)+UNR_LEN)/4+1];
	struct packet *icmp = (struct packet *)buf;
	icmp->ip = *ip;
	icmp->ip.tot_len = swap16(UNR_LEN); // IP + ICMP with 28 bytes of data
	icmp->ip.protocol = 1;    // icmp
	prepare_ip_reply(ip,icmp);
	icmp->ip.icmp.type = 3;
	icmp->ip.icmp.code = err;
	icmp->ip.icmp.checksum = 0;
	icmp->ip.icmp.id = 0;
	icmp->ip.icmp.sequence = 0;
	memcpy(icmp->ip.icmp.data,ip,28);
	icmp->ip.icmp.checksum = checksum((short*)&icmp->ip.icmp,36);
	icmp->len = UNR_LEN+14;
	send_packet(icmp);
}

static void process_udp(struct packet *pkt)
{
	struct iphdr *ip = &pkt->ip;
	unsigned sz = swap16(ip->udp.len);
	if (sz > swap16(ip->tot_len)-20)
		return; // invalid length 

	struct udp_rx_map *m = udp_map;
	for (;m->port && m->fn;m++) 
		if (m->port == swap16(ip->udp.dport)) {
			m->fn(pkt);
			return;
		}	
	unreach_icmp(ip,3);
}

static void process_icmp(struct iphdr *ip)
{
	if (ip->icmp.type != 8) return;
	// handle PING
	unsigned buf[400];
	struct packet *pong = (struct packet *)buf;
	unsigned len = swap16(ip->tot_len);
	if (len>400) len = 400;
	TRACE1("PONG len",len);
	memcpy(&pong->ip,ip,len);
	prepare_ip_reply(ip,pong);
	pong->ip.icmp.type = 0; // ping reply
	pong->ip.icmp.checksum = 0;
	pong->ip.icmp.checksum = checksum((short*)&pong->ip.icmp,len-20);
	pong->len = len+14;
	send_packet(pong);
}

static void process_ip(struct packet *pkt)
{
	unsigned err;
	struct iphdr *ip = &pkt->ip;
	if (ip->version != 4) return; 
	if (ip->daddr != _my_ip && ip->daddr != 0xffffffff) return; 
	if (ip->ihl != 5) { err = ICMP_PKT_FILTERED; goto icmp; }
	if (checksum((short*)ip,20)) return; // bad sum
	TRACE1("frag",ip->frag_off);
	if (ip->frag_off & 0xffbf) { err = ICMP_FRAG_NEEDED; goto icmp; }
	TRACE1("got IP proto",ip->protocol);
	switch (ip->protocol) {
		case 1:  process_icmp(ip); return;
		case 17: process_udp(pkt);  return;
		default: err = ICMP_PROT_UNREACH;
	}
icmp:	TRACE1("send icmp err",err);
	unreach_icmp(ip,err);	
}

void process_packet(struct packet *pkt)
{
	TRACE("got type %X\n",pkt->ethertype);
	current_mac = pkt->src;
	switch (pkt->ethertype) {
		case 0x608: process_arp(pkt); break;
		case 0x008: process_ip(pkt); break;
	}
}

void query_host_mac(struct host_info *hi)
{
	arp_qry = hi;
	struct packet arp;
	fill_our_mac_src(&arp);
	memset(&arp.dst,0xff,sizeof(struct mac_addr));
	arp.ethertype = 0x608;
	arp.arp.ar_op = 0x100; // query
	arp.arp.ar_hrd = 0x100;
	arp.arp.ar_pro = 8;
	arp.arp.ar_hln = 6;
	arp.arp.ar_pln = 4;
	memcpy(&arp.arp.ar_sha,&eth.mac,sizeof(struct mac_addr));
	memset(&arp.arp.ar_tha,0,sizeof(struct mac_addr));
	arp.arp.ar_sip = _my_ip;
	arp.arp.ar_tip = hi->ip;
	arp.len = sizeof(struct arphdr)+14;
	send_packet(&arp);
}
