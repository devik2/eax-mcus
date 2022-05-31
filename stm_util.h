#ifndef STM_UTIL_H
#define STM_UTIL_H 1

// patch V[B+C-1:B]<=x
#define S_BITFLD(V,B,C,X) if(1){unsigned v=V;v&=~(((1<<(C))-1)<<(B));v|=(X)<<(B);V=v;}

#define S_GPIO_MODER(G,P,M) S_BITFLD(G->MODER,P*2,2,M);
#define S_GPIO_OSPEEDR(G,P,S) S_BITFLD(G->OSPEEDR,P*2,2,S);
// zapne ALT funkci N na pinu P portu G
#define S_GPIO_ALT(G,P,N) S_BITFLD(G->AFR[(P>>3)&1],(P&7)*4,4,N); \
			S_GPIO_MODER(G,P,2)

// for STM32F1xx, MOD bits:
#define S_GPO_OD 4
#define S_GPO_ALT 8
#define S_GPI_ANALOG 0
#define S_GPI_FLOAT 4
#define S_GPI_PULL 8
#define S_GPO_OUT10 1
#define S_GPO_OUT2 2
#define S_GPO_OUT50 3

#define S_GPIO_MODE_L(G,P,MOD) S_BITFLD(G->CRL,(P&7)*4,4,MOD)
#define S_GPIO_MODE_H(G,P,MOD) S_BITFLD(G->CRH,(P&7)*4,4,MOD)
#define S_GPIO_MODE(G,P,MOD) if ((P)>=8) {S_GPIO_MODE_H(G,P,MOD)} else {S_GPIO_MODE_L(G,P,MOD)}

static inline int seq16_after(int16_t a,int16_t b)
{
	return (b-a)&0x8000;
}

static inline int seq_after(int a,int b)
{
	return (b-a)<0;
}

void jprintf(const char *f,...);
extern void (*jprintf_xtra)(const char *s,int sz);
void syslog_udp(const char *s,int sz);

#define PM_A(N) (0x10|(N))
#define PM_B(N) (0x20|(N))
#define PM_C(N) (0x30|(N))
#define PM_D(N) (0x40|(N))
#define PM_E(N) (0x50|(N))
#define PM_F(N) (0x60|(N))
#define PM_IN  1
#define PM_OUT 2
#define PM_AN 3
#define PM_PU  4
#define PM_PD  8
#define PM_SPD10  16
#define PM_SPD50  32
#define PM_OD 64
#define PM_ALT 128
struct gpio_setup_t {
	uint8_t port; // port 0x10=A, 0x20=B.. 0=end_mark
	uint8_t mode,alt;
	uint8_t val;
};

void gpio_setup(const struct gpio_setup_t *p,int cnt);
void gpio_setup_same(const uint8_t *lst,uint8_t mode,uint8_t alt);
void gpio_setup_one(uint8_t port,uint8_t mode,uint8_t alt);
void gpio_out(uint8_t port,int val);
int gpio_in(uint8_t port);

void flash_unlock();
void flash_wait();
int flash_sect_no(uint32_t pga);
int flash_erase(uint32_t sno);
void flash_write(uint16_t *addr,uint16_t val);

// provided by platform code
uint16_t get_utime();
void udelay(int us);

#endif
