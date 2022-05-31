#ifndef __IP_H__
#define __IP_H__
#include <stdint.h>

typedef unsigned u32_t;
typedef unsigned short u16_t;
typedef unsigned char u8_t;
struct mac_addr {
	unsigned char addr[6];
};
struct host_info {
	u32_t ip;
	u32_t tm;
	struct mac_addr mac;
};

struct arphdr
{
	u16_t		ar_hrd;		/* format of hardware address	*/
	u16_t		ar_pro;		/* format of protocol address	*/
	unsigned char	ar_hln;		/* length of hardware address	*/
	unsigned char	ar_pln;		/* length of protocol address	*/
	u16_t		ar_op;		/* ARP opcode (command)		*/

	 /*
	  *	 Ethernet looks like this : This bit is variable sized however...
	  */
	struct mac_addr		ar_sha;	
	unsigned 		ar_sip __attribute__((packed));
	struct mac_addr		ar_tha;
	unsigned 		ar_tip __attribute__((packed));

};

struct ip_udp {
	u16_t sport;
	u16_t dport;
	u16_t len;
	u16_t checksum;
	u8_t data[0]; 		// data
};

struct icmphdr
{
  u8_t type;		/* message type */
  u8_t code;		/* type sub-code */
  u16_t checksum;
  u16_t	id;
  u16_t	sequence;
  u8_t data[0];	
};

#define ICMP_ECHOREPLY		0	/* Echo Reply			*/
#define ICMP_DEST_UNREACH	3	/* Destination Unreachable	*/
#define ICMP_SOURCE_QUENCH	4	/* Source Quench		*/
#define ICMP_REDIRECT		5	/* Redirect (change route)	*/
#define ICMP_ECHO		8	/* Echo Request			*/
#define ICMP_TIME_EXCEEDED	11	/* Time Exceeded		*/
#define ICMP_PARAMETERPROB	12	/* Parameter Problem		*/
#define ICMP_TIMESTAMP		13	/* Timestamp Request		*/
#define ICMP_TIMESTAMPREPLY	14	/* Timestamp Reply		*/
#define ICMP_INFO_REQUEST	15	/* Information Request		*/
#define ICMP_INFO_REPLY		16	/* Information Reply		*/
#define ICMP_ADDRESS		17	/* Address Mask Request		*/
#define ICMP_ADDRESSREPLY	18	/* Address Mask Reply		*/
#define NR_ICMP_TYPES		18


/* Codes for UNREACH. */
#define ICMP_NET_UNREACH	0	/* Network Unreachable		*/
#define ICMP_HOST_UNREACH	1	/* Host Unreachable		*/
#define ICMP_PROT_UNREACH	2	/* Protocol Unreachable		*/
#define ICMP_PORT_UNREACH	3	/* Port Unreachable		*/
#define ICMP_FRAG_NEEDED	4	/* Fragmentation Needed/DF set	*/
#define ICMP_SR_FAILED		5	/* Source Route failed		*/
#define ICMP_NET_UNKNOWN	6
#define ICMP_HOST_UNKNOWN	7
#define ICMP_HOST_ISOLATED	8
#define ICMP_NET_ANO		9
#define ICMP_HOST_ANO		10
#define ICMP_NET_UNR_TOS	11
#define ICMP_HOST_UNR_TOS	12
#define ICMP_PKT_FILTERED	13	/* Packet filtered */
#define ICMP_PREC_VIOLATION	14	/* Precedence violation */
#define ICMP_PREC_CUTOFF	15	/* Precedence cut off */
#define NR_ICMP_UNREACH		15	/* instead of hardcoding immediate value */

struct iphdr {
	u8_t	ihl:4,
		version:4;
	u8_t	tos;
	u16_t	tot_len;
	u16_t	id;
	u16_t	frag_off;
	u8_t	ttl;
	u8_t	protocol;
	u16_t	check;
	u32_t	saddr;
	u32_t	daddr;
	union {
		struct icmphdr icmp;
		struct ip_udp udp;
	};
};

//#define ETH_PKT_SIZE 0x250
#define ETH_PKT_SIZE 0x600

struct packet {
	short len;		// also natural align pk_data to 32 bits
	struct mac_addr dst;
	struct mac_addr src;
	unsigned short ethertype;
	union {
		struct arphdr arp;
		struct iphdr ip;
		char raw[ETH_PKT_SIZE-16];
	};
};

typedef void (*ip_rx_routine_f)(struct packet *p);
struct udp_rx_map {
	ip_rx_routine_f fn;
	uint16_t port;
} extern *udp_map;

#define MAKE_IP(a,b,c,d) d<<24|c<<16|b<<8|a

volatile extern u32_t _my_ip;
void send_packet(struct packet *p);
void fill_our_mac_src(struct packet *pkt);
void fill_ip_hdr(struct packet *pkt,int len);
unsigned short swap16(unsigned short s);
void process_packet(struct packet *pkt);
// fill MAC by ARP, only one query can be pending
void query_host_mac(struct host_info *hi); 

struct eth_iodef;

// bootp
void send_bootp_req(struct eth_iodef *e,const char *img_name);
void bootp_rx(struct packet *pkt);
extern char bootp_file[64];
extern uint32_t bootp_srv_ip;

// tftp small version, 512 byte chunks
// request transfer, if read, rdbuf is first destination, NULL for WR
void tftp_req(u32_t host,u8_t *mac,const char *file,char *rdbuf);
// For writes use new data buffer with ssz 512 for mid segments
// and less for last seg.
// For reads, ssz is -1, buf is new buffer to use if last was filled
// (which is indicated by >0 response)
// Errs: -2 for error, 0 means correct end, -1 means try-again
int tftp_next(char *buf,int ssz);
// tftp callback, user routine tftp_regcb(int port) must register it
void tftp_rx(struct packet *pkt);
#endif

