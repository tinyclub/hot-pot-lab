ccflags-y := -I$(srctree)/net/lwip-1.4.1/src/include \
			 -I$(srctree)/net/lwip-1.4.1/src/include/ipv4
			 
obj-y += init.o dns.o dhcp.o mem.o memp.o netif.o pbuf.o tcp.o tcp_in.o tcp_out.o raw.o sys.o udp.o timers.o def.o stats.o

obj-y += ipv4/
obj-$(CONFIG_IPV6) += ipv6/

