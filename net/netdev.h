
#include <lwip/netif.h>

struct dim_sum_netdev {
	char name[16];
	unsigned char enetaddr[6];
	struct netif lwip_netif;
	int state;

	int  (*initialize) (struct dim_sum_netdev *netdev);
	int  (*send_pkt) (struct dim_sum_netdev *netdev, void *packet, int length);
	int  (*recv_pkt) (struct dim_sum_netdev *netdev, void *packet, int *length);
	void (*halt_netdev) (struct dim_sum_netdev *netdev);

	struct ip_addr ipaddr, netmask, gateway;

	struct dim_sum_netdev *next;
	void *send_buff;
	int index;
	void *priv;
};

int dim_sum_netdev_register(struct dim_sum_netdev *netdev);



