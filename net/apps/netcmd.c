#include <lwip/netif.h>
#include <lwip/inet.h>
#include <lwip/sockets.h>
#include <lwip/ip.h>
#include <lwip/arch.h>
#include <kapi/dim-sum/task.h>
#include <asm/current.h>
#include <dim-sum/sched.h>

#include "../netdev.h"


#define SERVER_PORT	8090
#define PKT_LEN_1		sizeof(unsigned int)
#define PKT_LEN_2		1500
#define SEND_NUM		0xffffffff

static char send_buf[PKT_LEN_2];
static char recv_buf[PKT_LEN_2];

static int my_safe_recv(int fd, char *buff, int length)
{
	int rcv = 0;

	while (rcv != length) {
		int ret = recv(fd, buff + rcv, length - rcv, 0);
		if (ret < 0) 
			return ret;

		rcv += ret;
	}

	return rcv;
}

static int my_safe_send(int fd, char *buff, int length)
{
	int snd = 0;

	while (snd != length) {
		int ret = send(fd, buff + snd, length - snd, 0);
		if (ret < 0) 
			return ret;

		snd += ret;
	}

	return snd;
}

static int tcp_test_server(void *argv)
{
	struct sockaddr_in sockaddr, peeraddr;
	int fd, fd2, ret = -1, addrlen, optval = 1;
	fd_set  rset;
	unsigned int last_recv = 0;
	int len = PKT_LEN_1;

	memset(&sockaddr, sizeof(struct sockaddr_in), 0);
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(SERVER_PORT);
	sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		printk("create socket failed!\n");
		return -1;
	}

	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
		printk("setsockopt failed\n");
		goto end;
	}

	if (bind(fd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) == -1) {
		printk("bind failed\n");
		goto end;
	}

	if (listen(fd, 2) == -1) {
		printk("listen failed\n");
		goto end;
	}

	memset(&peeraddr, 0, sizeof(peeraddr));
	fd2 = accept(fd, (struct sockaddr*)&peeraddr, &addrlen);
	if (fd2 == -1) {
		printk("accept error\n");
		goto end;
	}

	printk("accept client ip %s\n", inet_ntoa(peeraddr.sin_addr));

	while (1) {
		FD_ZERO(&rset);
		FD_SET(fd2, &rset);
		if (select(fd2+1, &rset, NULL, NULL, NULL) < 0) {
			printk("select error\n");
			ret = -1;
			goto end;				
		}

		if (FD_ISSET(fd2, &rset)) {
			unsigned int tmp;
			
			ret = my_safe_recv(fd2, recv_buf, len); 
			if (ret != len) {
				printk("server recv error, ret:%d\n", ret);
				break;
			}

			memcpy(&tmp, recv_buf, PKT_LEN_1);
			tmp = ntohl(tmp);

			printk("recv data:%d, len:%d\n", tmp, len);

			if (tmp == SEND_NUM) {
				printk("test over\n");
				ret = 0;
				break;
			}

			if (tmp != (last_recv + 1)) {
				printk("test error, last_recv:%d, now_recv:%d\n", last_recv, tmp);
				break;
			}

			/* 间隔10个报文收一个大报文，下一个是大报文 */
			if ((tmp + 1) % 10 == 0) 
				len = PKT_LEN_2;
			else
				len = PKT_LEN_1;

			last_recv = tmp;
		}
	}

end:
	close(fd);
	return ret;
}

static int tcp_test_client(char *svrip)
{	
	int fd, ret = -1;
	struct sockaddr_in sockaddr;
	struct in_addr ipaddr;
	unsigned int snd_token = 1;
	
	if (inet_aton((const char *)svrip, &ipaddr) == 0) {
		printk("Invalid svrip %s\n", svrip);
		return -1;
	}

	memset(&sockaddr, sizeof(struct sockaddr_in), 0);
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(SERVER_PORT);
	sockaddr.sin_addr.s_addr = ipaddr.s_addr;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		printk("Create socket failed!\n");
		return ret;
	}

	if (connect(fd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0) {
		printk("connect socket failed\n");
		goto end;
	}
	
	printk("connect server ip %s ok!\n",  inet_ntoa(sockaddr.sin_addr));
	ret = 0;

	memset(send_buf, 1, sizeof(send_buf));

	while (snd_token <= SEND_NUM) {
		unsigned int tmp = htonl(snd_token);
		int len = PKT_LEN_1;

		/* 间隔10个报文发一个大报文 */
		if (snd_token % 10 == 0)
			len = PKT_LEN_2;
		
		memcpy(send_buf, &tmp, PKT_LEN_1);
		
		ret = my_safe_send(fd, send_buf, len); 
		if (ret != len) {
			printk("tcp send failed\n");
			ret = -1;
			break;
		}
		printk("send data:%d, len:%d\n", snd_token++, len);
		SysSleep(1);  /* 间隔10ms */
	}

end:
	close(fd);
	return ret;	

}

int net_tcptest_cmd(int argc, char **args)
{
	if (argc == 1) {  /* TCP server */
		printk(" start tcp server\n");
		SysTaskCreate(tcp_test_server, NULL, "tcp_test_server", current->prio, NULL, 0);

	} else if (argc == 2) { /* TCP client */
		printk(" start tcp client\n");
		tcp_test_client(args[1]);

	} else {
		printk("Usage: tcptest -- start tcp sever\n"
			   "       tcptest svraddr -- start tcp client\n");
	}

	return 0;
}

extern int cpsw_net_debug;
extern struct dim_sum_netdev *netdev_list;

int net_ip_cmd(int argc, char **args)
{
	struct dim_sum_netdev *ndev = netdev_list;

	if ((argc < 2) || (argc > 4)) {
		goto end;
	}

	if (!strcmp(args[1], "show")) {
		char ipstr[16], maskstr[16];
		
		while (ndev) {
			inet_ntoa_r(ndev->ipaddr, ipstr, 16);
			inet_ntoa_r(ndev->netmask, maskstr, 16);
			
			printk(" Name:%s  IP: %s  Netmask: %s\n", ndev->name, ipstr, maskstr);
			
			ndev = ndev->next;
		}
		
	} else if (!strcmp(args[1], "set")) {
		struct in_addr ipaddr, ipmask;
	
		if (argc < 3) {
			goto end;
		}
	
		/** 目前只有一个网络设备cpsw，只设置该设备 **/
		while (ndev) {
			if (!strcmp(ndev->name, "cpsw"))
				break;

			ndev = ndev->next;
		} 

		if (!ndev) {
			printk(" not find netdev cpsw\n");
			return -1;
		}

		if (inet_aton((const char *)args[2], &ipaddr) == 0) {
			printk("Invalid ipaddr, ");
			goto end;
		}

		if (argc ==4) {
			if (inet_aton((const char *)args[3], &ipmask) == 0) {
				printk("Invalid netmask, ");
				goto end;
			}
		} else {  /** 若未设置掩码，且默认设置为255.255.255.0 **/
			inet_aton("255.255.255.0", &ipmask);
		}

		ndev->ipaddr.addr = ipaddr.s_addr;
		ndev->netmask.addr = ipmask.s_addr;

		netif_set_addr(&ndev->lwip_netif, &ndev->ipaddr, &ndev->netmask, &ndev->gateway);		
		
	} else if (!strcmp(args[1], "debugon")) {   /** 临时添加，用于cpsw调试使用 **/
		cpsw_net_debug = 1;
		
	} else if (!strcmp(args[1], "debugoff")) {
		cpsw_net_debug = 0;
		
	} else {
		goto end;;
	}

	return 0;

end:
	printk("Usage: ip show/set ipaddr netmask\n");
	return -1;
}


