#include <string.h>
#include "ip.h"
#include "opts.h"

#ifdef USE_XPRINTF
 #include "xprintf.h"
 #define snprintf xsnprintf
#else
 #include <stdio.h>
 #include <stdarg.h>
#endif

//static uint16_t seq;
//extern struct eth_iodef *eth;

const char *syslog_hostname;
const char *syslog_appname;

// can be used as jprintf_xtra callback
void syslog_udp(const char *s,int _sz)
{
	struct packet p;
	memset(&p,0,sizeof(p));
	p.ip.daddr = 0xffffffff; // broadcast for now TODO
	memset(p.dst.addr,0xff,6);
	p.ip.protocol = 17; // UDP

	char *logm = p.ip.udp.data;
	int sz;
	const char *fmt = "<14>1 - %s %s - - - %s";
	sz = snprintf(logm,500,fmt,syslog_hostname?syslog_hostname:"-",
			syslog_appname?syslog_appname:"-",s);

	fill_ip_hdr(&p,8+sz);

	p.ip.udp.sport = swap16(514);
	p.ip.udp.dport = swap16(512);
	p.ip.udp.len = swap16(8+sz);
	send_packet(&p);
}
