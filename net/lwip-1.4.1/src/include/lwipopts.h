
#ifndef __LWIPOPTS_H__
#define __LWIPOPTS_H__

#include <asm/posix_types.h>
#include <asm/string.h>
#include <dim-sum/base.h>

//#define TCPIP_THREAD_PRIO 10

/* LWIPµ÷ÊÔÑ¡Ïî */
//#define LWIP_DEBUG
//#define NETIF_DEBUG		LWIP_DBG_ON
//#define ETHARP_DEBUG	LWIP_DBG_ON
//#define SOCKETS_DEBUG            LWIP_DBG_ON
//#define IP_DEBUG                 LWIP_DBG_ON
//#define ICMP_DEBUG               LWIP_DBG_ON
//#define TCP_DEBUG                LWIP_DBG_ON
//#define UDP_DEBUG			LWIP_DBG_ON
//#define TCP_INPUT_DEBUG          LWIP_DBG_ON
//#define TCPIP_DEBUG		LWIP_DBG_ON


#define MEM_LIBC_MALLOC 1
#define MEMP_MEM_MALLOC 1
/* ---------- Memory options ---------- */
/* MEM_ALIGNMENT: should be set to the alignment of the CPU for which
   lwIP is compiled. 4 byte alignment -> define MEM_ALIGNMENT to 4, 2
   byte alignment -> define MEM_ALIGNMENT to 2. */

#define MEM_ALIGNMENT           4

/* MEM_SIZE: the size of the heap memory. If the application will send
a lot of data that needs to be copied, this should be set high. */
#define MEM_SIZE                256000

/* MEMP_NUM_PBUF: the number of memp struct pbufs. If the application
   sends a lot of data out of ROM (or other static memory), this
   should be set high. */
#define MEMP_NUM_PBUF           1600
/* MEMP_NUM_UDP_PCB: the number of UDP protocol control blocks. One
   per active UDP "connection". */
#define MEMP_NUM_UDP_PCB        6
/* MEMP_NUM_TCP_PCB: the number of simulatenously active TCP
   connections. */
#define MEMP_NUM_TCP_PCB        500
/* MEMP_NUM_TCP_PCB_LISTEN: the number of listening TCP
   connections. */
#define MEMP_NUM_TCP_PCB_LISTEN 8
/* MEMP_NUM_TCP_SEG: the number of simultaneously queued TCP
   segments. */
#define MEMP_NUM_TCP_SEG        32
/* MEMP_NUM_SYS_TIMEOUT: the number of simulateously active
   timeouts. */
#define MEMP_NUM_SYS_TIMEOUT    6


/* The following four are used only with the sequential API and can be
   set to 0 if the application only will use the raw API. */
/* MEMP_NUM_NETBUF: the number of struct netbufs. */
#define MEMP_NUM_NETBUF         32 /* 2 */
/* MEMP_NUM_NETCONN: the number of struct netconns. */
#define MEMP_NUM_NETCONN        64
/* MEMP_NUM_APIMSG: the number of struct api_msg, used for
   communication between the TCP/IP stack and the sequential
   programs. */
#define MEMP_NUM_API_MSG        8 /* 8 */
#undef MEMP_NUM_API_MSG
/* MEMP_NUM_TCPIPMSG: the number of struct tcpip_msg, which is used
   for sequential API communication and incoming packets. Used in
   src/api/tcpip.c. */
#define MEMP_NUM_TCPIP_MSG      16
#undef MEMP_NUM_TCPIP_MSG

/* These two control is reclaimer functions should be compiled
   in. Should always be turned on (1). */
#define MEM_RECLAIM             1
#define MEMP_RECLAIM            1

/* ---------- Pbuf options ---------- */
/* PBUF_POOL_SIZE: the number of buffers in the pbuf pool. */
//yangye 2003-1-22
//seems there is something wrong when PBUF_POOL_SIZE = 6 
#define PBUF_POOL_SIZE          1024

/* PBUF_POOL_BUFSIZE: the size of each pbuf in the pbuf pool. */
#define PBUF_POOL_BUFSIZE       2048 

/* PBUF_LINK_HLEN: the number of bytes that should be allocated for a
   link level header. */
#define PBUF_LINK_HLEN          16

/* ---------- TCP options ---------- */
#define LWIP_TCP                1
#define TCP_TTL                 255

#define SO_REUSE   1

/* Controls if TCP should queue segments that arrive out of
   order. Define to 0 if your device is low on memory. */
#define TCP_QUEUE_OOSEQ         1

/* TCP Maximum segment size. */
#define TCP_MSS                 1460

/* TCP sender buffer space (bytes). */
#define TCP_SND_BUF             16*TCP_MSS

/* TCP sender buffer space (pbufs). This must be at least = 2 *
   TCP_SND_BUF/TCP_MSS for things to work. */
#define TCP_SND_QUEUELEN        32

/* TCP receive window. */
#define TCP_WND                 5*TCP_MSS//24000

/* Maximum number of retransmissions of data segments. */
#define TCP_MAXRTX              12

/* Maximum number of retransmissions of SYN segments. */
#define TCP_SYNMAXRTX           4

/* ---------- ARP options ---------- */
#define ARP_TABLE_SIZE 10

/* ---------- IP options ---------- */
/* Define IP_FORWARD to 1 if you wish to have the ability to forward
   IP packets across network interfaces. If you are going to run lwIP
   on a device with only one network interface, define this to 0. */
#define IP_FORWARD              1
#undef IP_FORWARD

/* If defined to 1, IP options are allowed (but not parsed). If
   defined to 0, all packets with IP options are dropped. */
#define IP_OPTIONS              1

/* ---------- ICMP options ---------- */
#define ICMP_TTL                255

/*--------chaoshi option------------*/
/*tftp*/
#define LWIP_SO_RCVTIMEO        1

#define LWIP_TCP_KEEPALIVE 1

/* ---------- DHCP options ---------- */
/* Define LWIP_DHCP to 1 if you want DHCP configuration of
   interfaces. DHCP is not implemented in lwIP 0.5.1, however, so
   turning this on does currently not work. */
#define LWIP_DHCP               0

/* 1 if you want to do an ARP check on the offered address
   (recommended). */
#define DHCP_DOES_ARP_CHECK     0

/* ---------- UDP options ---------- */
#define LWIP_UDP                1
#define UDP_TTL                 255
#define LWIP_UDP_TODO

#define LWIP_PROVIDE_ERRNO		1

#define LWIP_HAVE_LOOPIF		1
#define LWIP_NETIF_LOOPBACK		1

#define LWIP_NETIF_API			1
#define LWIP_SOCKET				1
#define LWIP_NETCONN			1

/* ---------- Statistics options ---------- */
#define LWIP_STATS 1  //yangwei debug

#ifdef STATS
#define LINK_STATS
#define IP_STATS
#define ICMP_STATS
#define UDP_STATS
#define TCP_STATS
#define MEM_STATS
#define MEMP_STATS
#define PBUF_STATS
#define SYS_STATS
#endif /* STATS */

#define LWIP_COMPAT_MUTEX 0

#define LWIP_TIMEVAL_PRIVATE 0

#define LWIP_TCPIP_CORE_LOCKING 1

#endif /* __LWIPOPTS_H__ */

