#include <lwip/netif.h>
#include <lwip/tcpip.h>
#include <lwip/err.h>
#include <netif/etharp.h>
#include <dim-sum/sched.h>
#include <kapi/dim-sum/task.h>
#include <dim-sum/mem.h>
#include "netdev.h"


extern int ker_Sleep(int iMiniSec);

struct dim_sum_netdev *netdev_list = NULL;
static int netdev_index = 0;

static unsigned char rcv_pkt[2048];

int dim_sum_netdev_register(struct dim_sum_netdev *netdev)
{
	struct dim_sum_netdev *dev = netdev_list;

	if (!dev) {
		netdev_list = netdev;
		netdev->index = netdev_index++;
		return 0;
	}

	while (dev->next) {
		dev = dev->next;
	}

	dev->next = netdev;
	netdev->index = netdev_index++;
	return 0;
}

static err_t dim_sum_send_pkt(struct netif *netif, struct pbuf *p)
{
	struct dim_sum_netdev *ndev;
	struct pbuf *q;
	unsigned char *buff, *pos;
	int ret;

	ndev = list_entry(netif, struct dim_sum_netdev, lwip_netif);

	if (p->next) {
		pos = buff = (unsigned char *)ndev->send_buff;
		for (q = p; q != NULL; q = q->next) {
			memcpy(pos, q->payload, q->len);
			pos += q->len;
		}
	} else {
		buff = p->payload;
	}

	ret = ndev->send_pkt(ndev, buff, p->tot_len);

	return ret;
}

static err_t dim_sum_netif_init(struct netif *netif)
{
	struct dim_sum_netdev *ndev;

	ndev = list_entry(netif, struct dim_sum_netdev, lwip_netif);

	netif->name[0] = ndev->name[0];
	netif->name[1] = ndev->name[1];

	netif->output = etharp_output;
	netif->linkoutput = dim_sum_send_pkt;

	netif->mtu = 1500;
	netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;
	netif->hwaddr_len = 6;
	memcpy(netif->hwaddr, ndev->enetaddr, 6);

	ndev->send_buff = dim_sum_mem_alloc((netif->mtu + 4095) & ~4095, MEM_NORMAL);
	if (!ndev->send_buff) {
		printk("%s %d, dim_sum_mem_alloc fail\n", __FUNCTION__, __LINE__);
		return ERR_MEM;
	}

	ndev->halt_netdev(ndev);
	ndev->initialize(ndev);

	return ERR_OK;
}

static int dim_sum_net_poll_task(void *argv)
{
	while (1) {
		struct dim_sum_netdev *ndev = netdev_list;

		while (ndev) {
			int ret, len;
			void *inpkt = &rcv_pkt[0];
			struct pbuf *p, *q;
			struct eth_hdr *ethhdr;

			ret = ndev->recv_pkt(ndev, &rcv_pkt[0], &len);
			if (ret <= 0)   { /* no packet */
				goto next_try;
			}

			if (!(p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL))) {
				return -1;
			}

			for(q = p; q != NULL; q = q->next) {
				memcpy(q->payload, (void*)inpkt, q->len);
				inpkt += q->len;
			}
			ethhdr = (struct eth_hdr *)p->payload;

			switch (htons(ethhdr->type)) {
				case ETHTYPE_IP:
				case ETHTYPE_ARP:
					if (ndev->lwip_netif.input(p, &ndev->lwip_netif)!= ERR_OK) { 
						pbuf_free(p);
					}
					break;
				default:
					pbuf_free(p);
					break;
			}

			ndev = ndev->next;
		}

next_try:
		ker_Sleep(1);
	}
}

void dim_sum_netdev_startup(void)
{
	struct dim_sum_netdev *dev = netdev_list;

	while (dev) {
		netif_set_default(&dev->lwip_netif);

		if (netif_add(&dev->lwip_netif, &dev->ipaddr, &dev->netmask, &dev->gateway,
					NULL, dim_sum_netif_init, tcpip_input) == NULL) {
			printk("netif add fail\n");
			return;
		}
		netif_set_up(&dev->lwip_netif);
		dev = dev->next;
	}
	if (netdev_list)
		SysTaskCreate(dim_sum_net_poll_task, NULL, "lwip_net_poll", 10, NULL, 0);
}


