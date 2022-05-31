//#include "cpu.h"
#include <string.h>
#include <stdint.h>
#include "ip.h"
#include "stm32_eth.h"
#include "stm_util.h"

struct ip_bootp {
	uint8_t op,htype,hlen,hops;
	uint32_t xid;
	uint16_t secs,_pad;
	uint32_t ciaddr,yiaddr,siaddr,giaddr;
	uint8_t chaddr[16];
	char sname[64];
	char file[128];
	uint8_t vend[64];
};

static uint16_t seq;
void send_bootp_req(struct eth_iodef *e,const char *img_name)
{
	struct packet p;
	memset(&p,0,sizeof(p));
	p.ip.daddr = 0xffffffff;
	memset(p.dst.addr,0xff,6);
	p.ip.protocol = 17; // UDP
	fill_ip_hdr(&p,8+sizeof(struct ip_bootp));

	struct ip_bootp *b = (struct ip_bootp *)p.ip.udp.data;
	p.ip.udp.sport = swap16(68);
	p.ip.udp.dport = swap16(67);
	p.ip.udp.len = swap16(8+sizeof(struct ip_bootp));
	b->op = 1; // req
	b->htype = 1; // eth
	b->hlen = 6;

	b->secs = swap16(seq++);
	memcpy(b->chaddr,e->mac,b->hlen);
	if (img_name) strcpy(b->file,img_name);

	jprintf("send BOOTP req\n");
	send_packet(&p);
}

char bootp_file[64];
uint32_t bootp_srv_ip;
void bootp_rx(struct packet *pkt)
{
	struct ip_bootp *b = (struct ip_bootp *)pkt->ip.udp.data;
	if (b->xid || b->op != 2 || !b->yiaddr) return;

	// got new IP
	_my_ip = b->yiaddr;
	strncpy(bootp_file,b->file,sizeof(bootp_file)-1);
	bootp_srv_ip = b->siaddr;
	jprintf("BOOTP new IP is %X, srv %X, file '%s'\n",
			b->yiaddr,b->siaddr,b->file);
}
