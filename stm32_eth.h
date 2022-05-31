
struct eth_iodef {
	char io_rst,io_mdc,io_mdio;
	char mac[6]; // our local MAC
};
void eth_send(const char *pkt,int sz);
void eth_wait();
int eth_running();
void eth_start(int fd,int fast);
void eth_stop();
int eth_smi_rd(struct eth_iodef *e,int reg);
void eth_smi_wr(struct eth_iodef *e,int reg,uint16_t v);
void eth_init(struct eth_iodef *e);
int eth_try_rx(struct eth_iodef *e,char **pkt);
void eth_complete_rx(struct eth_iodef *e);

// read PHY and setup MAC
void handle_eth_phy_simple(struct eth_iodef *e);

