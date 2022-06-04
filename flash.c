#include "cpu.h"
#include <string.h>
#include "eax-mcus/stm_util.h"

void flash_unlock()
{
	if (FLASH->CR & FLASH_CR_LOCK) {
		FLASH->KEYR = 0x45670123;
		FLASH->KEYR = 0xCDEF89AB;
	}
}

void flash_wait()
{
	while (FLASH->SR & FLASH_SR_BSY);
}

#ifdef STM32F1
#define PG_BITS 11
#define PG_CNT 128
// convert address to sector no, returns -1 for invalid address
int flash_sect_no(uint32_t pga)
{
	if (pga & 1) return -1;
	if (pga<0x8000000) return -1;
	int sec = pga >> PG_BITS;
	if (sec>=PG_CNT) return -1;
	return sec;
}

// returns no of errors
int flash_erase(uint32_t sno)
{
	flash_wait();
	FLASH->AR = sno<<PG_BITS;
	FLASH->CR |= FLASH_CR_STRT;
	flash_wait();
	FLASH->CR = 0;
	/*
	int i,err=0;
	for (i=0;i<(1<<(PG_BITS-2));i++) 
		if (pga[i]!=0xffffffff) err++;
	return err; XXX: implement
	*/
	return 0;
}

#else
#define PG_CNT 12
// convert address to sector no, returns -1 for invalid address
// F2..4 uses 4*16k,1x64k,n*128k
int flash_sect_no(uint32_t pga)
{
	if (pga & 1) return -1;
	if (pga<0x8000000) return -1;
	pga -= 0x8000000;
	if (pga<0x10000) return pga>>14;
	if (pga<0x20000) return 4;
	int sec = (pga >> 17)+4;
	if (sec>=PG_CNT) return -1;
	return sec;
}

// returns no of errors
int flash_erase(uint32_t sno)
{
	flash_wait();
	FLASH->CR = FLASH_CR_SER|FLASH_CR_PSIZE_1|(sno<<3);
	FLASH->CR |= FLASH_CR_STRT;
	flash_wait();
	FLASH->CR = 0;
	return 0;
	/*
	int i,err=0;
	for (i=0;i<(1<<(PG_BITS-2));i++) 
		if (pga[i]!=0xffffffff) err++;
	return err; XXX: implement
	*/
	return 0;
}
#endif
void flash_write(uint16_t *addr,uint16_t val)
{
	FLASH->CR = FLASH_CR_PG
#ifndef STM32F1
		|FLASH_CR_PSIZE_0
#endif
		;
	*addr = val;
	__disable_irq();
	flash_wait();
	__enable_irq();
	FLASH->CR = 0;
}

