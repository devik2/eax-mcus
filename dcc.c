#include "cpu.h"
#include <string.h>
#ifdef USE_XPRINTF
 #include "xprintf.h"
#else
 #include <stdio.h>
 #include <stdarg.h>
#endif

char no_dcc;
#define DCC_EMUA 0x20000000
int send_dcc(uint32_t c)
{
	volatile unsigned int *p = (void*)DCC_EMUA;
	if (no_dcc && (*p & 1)) return 0;
	no_dcc = 0;
	int tmo = 5000000;
	while (*p & 1) {
		if (--tmo <= 0) {
			no_dcc = 1;
			return 0;
		}
	}
	p[1] = c; p[0] = 1;
	return 1;
}

int dcc_req(const char *mem,int sz,int cnt)
{
	if (!send_dcc(1|(sz<<8)|(cnt<<16))) return 0;

	cnt *= sz ? sz:1;
	for (;cnt>0;mem+=4,cnt-=4) {
		uint32_t x = 0; 
		memcpy(&x,mem,cnt>4?4:cnt);
		if (!send_dcc(x)) return 0;
	}
}

void (*jprintf_xtra)(const char *s,int sz);
void jprintf (const char *fmt, ...)
{
	va_list arp;
	va_start(arp, fmt);
	char buff[100];
	int sz;
#ifdef USE_XPRINTF
	xstream_t f;
	f.ptr = buff;
	f.space = sizeof(buff)-1;
	xvfprintf(&f,fmt,arp);
	sz = f.len;
#else
	sz = vsnprintf(buff,sizeof(buff),fmt,arp);
#endif
	va_end(arp);
	if (jprintf_xtra)
		jprintf_xtra(buff,sz); // offer to other channels (TCP)
	dcc_req(buff,0,sz);
}
