#include <lwip/sockets.h>
#include <lwip/icmp.h>
#include <lwip/ip.h>
#include <lwip/arch.h>
#include <lwip/inet.h>


#define ICMP_ECHO_REPLY	0	/* Echo reply 			*/
#define ICMP_REDIRECT		5	/* Redirect (change route)	*/
#define ICMP_ECHO_REQUEST	8	/* Echo request			*/
#define ICMP_MINLEN	    8				/* abs minimum */
#define ICMP_RETRY_CNT   10

#define PING_TIMEOUT	20   /* 20*10ms = 200ms*/

enum {
	DEFDATALEN = 56,
	MAXIPLEN = 60,
	MAXICMPLEN = 76,
	MAXPACKET = 65468,
	MAX_DUP_CHK = (8 * 128),
	MAXWAIT = 10,
	PINGINTERVAL = 1, /* 1 second */
};

static int in_cksum(unsigned short *buf, int sz)
{
	int nleft = sz;
	int sum = 0;
	unsigned short *w = buf;
	unsigned short ans = 0;

	while (nleft > 1) {
		sum += *w++;
		nleft -= 2;
	}

	if (nleft == 1) {
		*(unsigned char *) (&ans) = *(unsigned char *) w;
		sum += ans;
	}

	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	ans = ~sum;
	return ans;
}

static int ping_main(u32_t daddr)
{
	struct sockaddr_in pingaddr;
	struct icmp_echo_hdr *pkt;
	int pingsock, c,cnt;
	char packet[DEFDATALEN + MAXIPLEN + MAXICMPLEN];
	int timeout, retry;

	pingsock = socket(AF_INET, SOCK_RAW, 1); /* 1 == ICMP */
	if (pingsock < 0) {
		printk("Create socket failed! ret is %d\n", pingsock);
		return -1;
	}

	pingaddr.sin_family = AF_INET;
	pingaddr.sin_addr.s_addr = daddr;

	pkt = (struct icmp_echo_hdr *) packet;
	memset(pkt, 0, sizeof(packet));
	pkt->type      = ICMP_ECHO_REQUEST;
	pkt->chksum  = in_cksum((unsigned short *) pkt, sizeof(packet));

	timeout = PING_TIMEOUT;
	setsockopt(pingsock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(int));
	c = sendto(pingsock, packet, DEFDATALEN + ICMP_MINLEN, 0,\
			   (struct sockaddr *) &pingaddr, sizeof(pingaddr));
	
	retry = ICMP_RETRY_CNT;
	cnt = 0;
	/* listen for replies */
	while (1) {
		struct sockaddr_in from;
		socklen_t fromlen = sizeof(from);
        	c = recvfrom(pingsock, packet, sizeof(packet), 0, (struct sockaddr *) &from, (socklen_t *)&fromlen);
		if (c <= 0) {
			if (++cnt == retry){
				close(pingsock);
				printk("ping %s failed, host is not alive !\n", inet_ntoa(pingaddr.sin_addr));
				return -1;
			}
			continue;
		}

		if (from.sin_addr.s_addr != daddr)
			continue;
		
		if (c >= 76) {			/* ip + icmp */
			struct ip_hdr *iphdr = (struct ip_hdr *) packet;
			#if 1
		   	pkt = (struct icmp_echo_hdr *) (packet +(((ntohs(iphdr->_v_hl)>> 8)&0xf)<<2)); 
			#else
			pkt = (struct icmp_echo_hdr *) (packet + (((iphdr->_v_hl_tos >> 8)&0xf) << 2));  /* skip ip hdr */
			#endif
			if (pkt->type == ICMP_ECHO_REPLY){
				break;
			}
		}
	}
	close(pingsock);

	/*need to complete*/
	printk("ping %s is alive !\n", inet_ntoa(pingaddr.sin_addr));

	return 0;
}

int net_ping_cmd(int argc, char **args)

{
	int ret;
	struct in_addr ipaddr;

	if (argc != 2) {
		printk("Usage: ping ipaddr\n");
		return 0;
	}

	if (inet_aton((const char *)args[1], &ipaddr) == 0) {
		printk("Invalid ip address \n");
		return 0;
	}

	ret = ping_main(ipaddr.s_addr);

	return ret;
}

