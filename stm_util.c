#include "cpu.h"
#include "stm_util.h"

GPIO_TypeDef *gpio_get(uint8_t port)
{
	return (GPIO_TypeDef*)(GPIOA_BASE+0x400*((port>>4)-1));
}

int gpio_in(uint8_t port)
{
	GPIO_TypeDef *g = gpio_get(port);
	return (g->IDR >> (port&15)) & 1;
}

void gpio_out(uint8_t port,int val)
{
	GPIO_TypeDef *g = gpio_get(port);
	int msk = 1 << (port&15);
	g->BSRR = (msk<<16)|(val ? msk:0);
}

#ifdef STM32F1
void gpio_setup_one(uint8_t port,uint8_t mode,uint8_t alt)
{
	GPIO_TypeDef *g = gpio_get(port);
	int mod = S_GPI_ANALOG;
	switch (mode & 3) {
		case PM_IN: if (mode & (PM_PD|PM_PU)) {
				mod = S_GPI_PULL;
				gpio_out(port,mode & PM_PU);
			     } else mod = S_GPI_FLOAT;
			     break;
		case PM_OUT: mod = mode & PM_SPD50 ? S_GPO_OUT50 
			     : mode & PM_SPD10 ? S_GPO_OUT10:S_GPO_OUT2; break;
	}
	if (mode & PM_OD) mod |= S_GPO_OD;
	if (mode & PM_ALT) mod |= S_GPO_ALT;
	S_GPIO_MODE(g,(port&15),mod);
}
#else
void gpio_setup_one(uint8_t port,uint8_t mode,uint8_t alt)
{
	GPIO_TypeDef *g = gpio_get(port);
	switch (mode & 3) {
		case PM_IN:  S_GPIO_MODER(g,(port&15),0);
			     S_BITFLD(g->PUPDR,(port&15)*2,2,mode & PM_PU ? 1:
					     mode & PM_PD ? 2 :0);
			     break;
		case PM_OUT: S_GPIO_MODER(g,(port&15),1);
			     S_GPIO_OSPEEDR(g,(port&15),mode & PM_SPD50 ? 3
					     : mode & PM_SPD10 ? 2 : 0);
			     S_BITFLD(g->OTYPER,(port&15),1,mode & PM_OD ? 1:0);
			     break;
		default:     S_GPIO_MODER(g,(port&15),3);
			     break;
	}
	if (mode & PM_ALT) {
		S_GPIO_ALT(g,(port&15),alt); // macro with 2 cmds !
	}
}
#endif

void gpio_setup(const struct gpio_setup_t *p,int cnt)
{
	for (;p->port && cnt;p++,cnt--) gpio_setup_one(p->port,p->mode,p->alt);
}

void gpio_setup_same(const uint8_t *lst,uint8_t mode,uint8_t alt)
{
	for (;*lst;lst++) gpio_setup_one(*lst,mode,alt);
}

