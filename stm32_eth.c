#include "cpu.h"
#include "stm32_eth.h"
#include <string.h>
#include "stm_util.h"

#define ETH_DMATxDesc_OWN                     ((uint32_t)0x80000000)  /*!< OWN bit: descriptor is owned by DMA engine */
#define ETH_DMATxDesc_IC                      ((uint32_t)0x40000000)  /*!< Interrupt on Completion */
#define ETH_DMATxDesc_LS                      ((uint32_t)0x20000000)  /*!< Last Segment */
#define ETH_DMATxDesc_FS                      ((uint32_t)0x10000000)  /*!< First Segment */
#define ETH_DMATxDesc_DC                      ((uint32_t)0x08000000)  /*!< Disable CRC */
#define ETH_DMATxDesc_DP                      ((uint32_t)0x04000000)  /*!< Disable Padding */
#define ETH_DMATxDesc_TTSE                    ((uint32_t)0x02000000)  /*!< Transmit Time Stamp Enable */
#define ETH_DMATxDesc_CIC                     ((uint32_t)0x00C00000)  /*!< Checksum Insertion Control: 4 cases */
#define ETH_DMATxDesc_CIC_ByPass              ((uint32_t)0x00000000)  /*!< Do Nothing: Checksum Engine is bypassed */ 
#define ETH_DMATxDesc_CIC_IPV4Header          ((uint32_t)0x00400000)  /*!< IPV4 header Checksum Insertion */ 
#define ETH_DMATxDesc_CIC_TCPUDPICMP_Segment  ((uint32_t)0x00800000)  /*!< TCP/UDP/ICMP Checksum Insertion calculated over segment only */ 
#define ETH_DMATxDesc_CIC_TCPUDPICMP_Full     ((uint32_t)0x00C00000)  /*!< TCP/UDP/ICMP Checksum Insertion fully calculated */ 
#define ETH_DMATxDesc_TER                     ((uint32_t)0x00200000)  /*!< Transmit End of Ring */
#define ETH_DMATxDesc_TCH                     ((uint32_t)0x00100000)  /*!< Second Address Chained */
#define ETH_DMATxDesc_TTSS                    ((uint32_t)0x00020000)  /*!< Tx Time Stamp Status */
#define ETH_DMATxDesc_IHE                     ((uint32_t)0x00010000)  /*!< IP Header Error */
#define ETH_DMATxDesc_ES                      ((uint32_t)0x00008000)  /*!< Error summary: OR of the following bits: UE || ED || EC || LCO || NC || LCA || FF || JT */
#define ETH_DMATxDesc_JT                      ((uint32_t)0x00004000)  /*!< Jabber Timeout */
#define ETH_DMATxDesc_FF                      ((uint32_t)0x00002000)  /*!< Frame Flushed: DMA/MTL flushed the frame due to SW flush */
#define ETH_DMATxDesc_PCE                     ((uint32_t)0x00001000)  /*!< Payload Checksum Error */
#define ETH_DMATxDesc_LCA                     ((uint32_t)0x00000800)  /*!< Loss of Carrier: carrier lost during tramsmission */
#define ETH_DMATxDesc_NC                      ((uint32_t)0x00000400)  /*!< No Carrier: no carrier signal from the tranceiver */
#define ETH_DMATxDesc_LCO                     ((uint32_t)0x00000200)  /*!< Late Collision: transmission aborted due to collision */
#define ETH_DMATxDesc_EC                      ((uint32_t)0x00000100)  /*!< Excessive Collision: transmission aborted after 16 collisions */
#define ETH_DMATxDesc_VF                      ((uint32_t)0x00000080)  /*!< VLAN Frame */
#define ETH_DMATxDesc_CC                      ((uint32_t)0x00000078)  /*!< Collision Count */
#define ETH_DMATxDesc_ED                      ((uint32_t)0x00000004)  /*!< Excessive Deferral */
#define ETH_DMATxDesc_UF                      ((uint32_t)0x00000002)  /*!< Underflow Error: late data arrival from the memory */
#define ETH_DMATxDesc_DB                      ((uint32_t)0x00000001)  /*!< Deferred Bit */
#define ETH_DMARxDesc_OWN         ((uint32_t)0x80000000)  /*!< OWN bit: descriptor is owned by DMA engine  */
#define ETH_DMARxDesc_AFM         ((uint32_t)0x40000000)  /*!< DA Filter Fail for the rx frame  */
#define ETH_DMARxDesc_FL          ((uint32_t)0x3FFF0000)  /*!< Receive descriptor frame length  */
#define ETH_DMARxDesc_ES          ((uint32_t)0x00008000)  /*!< Error summary: OR of the following bits: DE || OE || IPC || LC || RWT || RE || CE */
#define ETH_DMARxDesc_DE          ((uint32_t)0x00004000)  /*!< Desciptor error: no more descriptors for receive frame  */
#define ETH_DMARxDesc_SAF         ((uint32_t)0x00002000)  /*!< SA Filter Fail for the received frame */
#define ETH_DMARxDesc_LE          ((uint32_t)0x00001000)  /*!< Frame size not matching with length field */
#define ETH_DMARxDesc_OE          ((uint32_t)0x00000800)  /*!< Overflow Error: Frame was damaged due to buffer overflow */
#define ETH_DMARxDesc_VLAN        ((uint32_t)0x00000400)  /*!< VLAN Tag: received frame is a VLAN frame */
#define ETH_DMARxDesc_FS          ((uint32_t)0x00000200)  /*!< First descriptor of the frame  */
#define ETH_DMARxDesc_LS          ((uint32_t)0x00000100)  /*!< Last descriptor of the frame  */
#define ETH_DMARxDesc_IPV4HCE     ((uint32_t)0x00000080)  /*!< IPC Checksum Error: Rx Ipv4 header checksum error   */
#define ETH_DMARxDesc_LC          ((uint32_t)0x00000040)  /*!< Late collision occurred during reception   */
#define ETH_DMARxDesc_FT          ((uint32_t)0x00000020)  /*!< Frame type - Ethernet, otherwise 802.3    */
#define ETH_DMARxDesc_RWT         ((uint32_t)0x00000010)  /*!< Receive Watchdog Timeout: watchdog timer expired during reception    */
#define ETH_DMARxDesc_RE          ((uint32_t)0x00000008)  /*!< Receive error: error reported by MII interface  */
#define ETH_DMARxDesc_DBE         ((uint32_t)0x00000004)  /*!< Dribble bit error: frame contains non int multiple of 8 bits  */
#define ETH_DMARxDesc_CE          ((uint32_t)0x00000002)  /*!< CRC error */
#define ETH_DMARxDesc_MAMPCE      ((uint32_t)0x00000001)  /*!< Rx MAC Address/Payload Checksum Error: Rx MAC address matched/ Rx Payload Checksum Error */

#define RX_BUFS 4
struct stm_eth_tx_desc {
	uint32_t sts;
	uint16_t sz[2];
	uint32_t addr[2];
} volatile rxdp[RX_BUFS],txdp[2];

static char mac_rx_buffer[RX_BUFS][0x604] __attribute__((aligned(4)));

void eth_load_mac(const struct eth_iodef *e)
{
	const char *m = e->mac;
	ETH->MACA0HR = m[4]|(m[5]<<8)|0x80000000;
	ETH->MACA0LR = m[0]|(m[1]<<8)|(m[2]<<16)|(m[3]<<24);
}

void eth_init(struct eth_iodef *e)
{
	gpio_setup_one(e->io_rst,PM_OUT,0);
	gpio_setup_one(e->io_mdc,PM_OUT,0);
	udelay(1000);
	gpio_out(e->io_rst,1);

	ETH->DMABMR |= ETH_DMABMR_SR;
	while(ETH->DMABMR & ETH_DMABMR_SR);
	ETH->MACCR = 0;
	ETH->DMATDLAR = (uint32_t)txdp;
	ETH->DMARDLAR = (uint32_t)rxdp;
	ETH->DMABMR = ETH_DMABMR_PBL_4Beat;
//	ETH->DMAOMR   = ETH_DMAOMR_FTF;
//	while (ETH->DMAOMR & ETH_DMAOMR_FTF);
	memset((void*)txdp,0,sizeof(txdp));
	memset((void*)rxdp,0,sizeof(rxdp));
	eth_load_mac(e);
	udelay(100);
	ETH->DMAOMR = ETH_DMAOMR_TSF|ETH_DMAOMR_ST;
}

static char rx_ptr;
static void fill_rx_ring()
{
	int i;
	rx_ptr = 0;
	for (i=0;i<RX_BUFS;i++) {
		rxdp[i].sz[0] = i>=RX_BUFS-1 ? 0x8000:0; // RER flag
		rxdp[i].sz[1] = sizeof(mac_rx_buffer[i]);
		rxdp[i].addr[1] = (uint32_t)mac_rx_buffer[i]+2; // reserve 2 bytes
		rxdp[i].sts = ETH_DMARxDesc_OWN;
	}
}

int eth_running()
{
	return ETH->MACCR & (ETH_MACCR_TE|ETH_MACCR_RE) ? 1 : 0;
}

int eth_ovf;
// if packet is ready, returns its size and its pointer goes to pkt,
// return 0 for no pkt
int eth_try_rx(struct eth_iodef *e,char **pkt)
{
	if (!eth_running()) return 0;
	if (rxdp[rx_ptr].sts & ETH_DMARxDesc_OWN) return 0; // no pkt
	int sz = (rxdp[rx_ptr].sts >> 16) & 0x3fff;
	char *dat = mac_rx_buffer[rx_ptr]+2;
	if (pkt) *pkt = dat;
	eth_ovf += ETH->DMAMFBOCR & 0xffff;
#if 0
	if (dat[0]==0x70)
	jprintf("************* got pkt #%d %d %x:%x:%x:%x:%x:%x %x:%x %x a=%X\n",
		rx_ptr,sz, dat[0],dat[1], dat[2],dat[3], dat[4],dat[5], 
		dat[12],dat[13], eth_ovf, dat);
#endif
	return sz==0?1:sz; 
}

// mark valid packet from eth_try_rx as done
void eth_complete_rx(struct eth_iodef *e)
{
	rxdp[rx_ptr].sts = ETH_DMARxDesc_OWN;
	if (++rx_ptr >= RX_BUFS) rx_ptr = 0;
}

void eth_start(int fd,int fast)
{
	ETH->MACCR = (fd ? ETH_MACCR_DM:0)|
		(fast ? ETH_MACCR_FES:0)|ETH_MACCR_TE
		|ETH_MACCR_RE;
	fill_rx_ring();
	ETH->DMAOMR = ETH_DMAOMR_TSF|ETH_DMAOMR_ST
		|ETH_DMAOMR_RSF|ETH_DMAOMR_SR;
	jprintf("eth start %X\n",ETH->MACCR);
}

void eth_stop()
{
	ETH->DMAOMR = ETH_DMAOMR_TSF|ETH_DMAOMR_RSF;
	ETH->MACCR = 0;
	udelay(1000);
}

void eth_wait()
{
	if (!eth_running()) return;
	while (txdp[0].sts & ETH_DMATxDesc_OWN);
}

void handle_eth_phy_simple(struct eth_iodef *e)
{
	int st = eth_smi_rd(e,0x1e);
	int up = st & 256 ? 1 : 0;
	if (eth_running() == up) return;
	if (!up) {
		eth_stop();
		return;
	}
	eth_start(st & 4,st & 2);
}

// simple send routine
void eth_send(const char *pkt,int sz)
{
	if (!eth_running()) return;
	if (sz<14) return;
	txdp[0].sz[1] = sz;
	txdp[0].addr[1] = (uint32_t)pkt;
	txdp[0].sts = ETH_DMATxDesc_LS|ETH_DMATxDesc_FS|ETH_DMATxDesc_OWN
		|ETH_DMATxDesc_TER|0xffff;
	ETH->DMATPDR = 1;
#if 0
	jprintf("eth sent sz %d addr %X sts %X dbg %X\n",sz,pkt,
			ETH->DMASR,ETH->RESERVED1[1]);
	jprintf("eth2 sent sts %X hdr %X\n", ETH->DMASR,txdp[0].sts);
#endif
}

// -1 means no change, io==2/3 means input/output mode
static int eth_smi_set(struct eth_iodef *e,int clk,int io)
{
	int rv = gpio_in(e->io_mdio);
	if (clk>=0) gpio_out(e->io_mdc,clk);
	switch (io) {
		case 0:
		case 1: gpio_out(e->io_mdio,io); break;
		case 2: gpio_setup_one(e->io_mdio,PM_IN|PM_PU,0); break;
		case 3: gpio_setup_one(e->io_mdio,PM_OUT,0);
	}
	udelay(1);
	return rv;
}

uint32_t eth_smi_xact(struct eth_iodef *e,uint32_t x,int rd)
{
	int i; uint32_t r;
	eth_smi_set(e,0,3); // IO out
	for (i=0;i<32;i++,x<<=1) {
		if (rd & i==14) { eth_smi_set(e,-1,2); x = -1; }
		eth_smi_set(e,0,(x>>31)&1);
		r = (r<<1)|eth_smi_set(e,1,-1);
	}
	eth_smi_set(e,0,2);
	return r;
}

int eth_smi_rd(struct eth_iodef *e,int reg)
{
	eth_smi_xact(e,0xffffffff,0);
	uint32_t r = eth_smi_xact(e,0x60000000|(reg<<18),1);
	r &= 0xffff;
	//jprintf("SMI %x: %X\n",reg,r);
	return r;
}

void eth_smi_wr(struct eth_iodef *e,int reg,uint16_t v)
{
	eth_smi_xact(e,0xffffffff,0);
	uint32_t r = eth_smi_xact(e,0x50020000|(reg<<18)|v,0);
}
