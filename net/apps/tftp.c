#include <lwip/sockets.h>
#include <lwip/ip.h>
#include <lwip/arch.h>
#include <lwip/inet.h>
#include <uapi/asm/fcntl.h>


//#define TFTP_DEBUG

#define TFTP_BLOCKSIZE_DEFAULT 512	/* according to RFC 1350, don't change */
#define TFTP_TIMEOUT 1	/* seconds */
#define TFTP_NUM_RETRIES 1 /* number of retries */

#define TFTP_PORT	69

static const char * const MODE_OCTET = "octet";
#define MODE_OCTET_LEN 6 /* sizeof(MODE_OCTET)*/

static const char * const OPTION_BLOCKSIZE = "blksize";
#define OPTION_BLOCKSIZE_LEN 8 /* sizeof(OPTION_BLOCKSIZE) */

/* opcodes we support */
#define TFTP_RRQ   1
#define TFTP_WRQ   2
#define TFTP_DATA  3
#define TFTP_ACK   4
#define TFTP_ERROR 5
#define TFTP_OACK  6

static const char *const tftp_bb_error_msg[] = {
	"Undefined error",
	"File not found",
	"Access violation",
	"Disk full or allocation error",
	"Illegal TFTP operation",
	"Unknown transfer ID",
	"File already exists",
	"No such user"
};

#define tftp_cmd_get 1
#define tftp_cmd_put 2

static int tftp_is_running = 0;

extern int lwip_file_open(const char *pathname, int flags, mode_t mode);
extern int lwip_file_unlink(const char *a);
extern ssize_t lwip_file_write(int a, const void *b, size_t c);
extern ssize_t lwip_file_read(int a, void *b, size_t c);


#ifdef FEATURE_TFTP_BLOCKSIZE
static int tftp_blocksize_check(int blocksize, int bufsize)
{
	/* Check if the blocksize is valid:
	 * RFC2348 says between 8 and 65464,
	 * but our implementation makes it impossible
	 * to use blocksizes smaller than 22 octets.
	 */

	if ((bufsize && (blocksize > bufsize)) ||
		(blocksize < 8) || (blocksize > 65564)) {
		printk("bad blocksize\n");
		return 0;
	}

	return blocksize;
}

static char *tftp_option_get(char *buf, int len, const char * const option)
{
	int opt_val = 0;
	int opt_found = 0;
	int k;

	while (len > 0) {

		/* Make sure the options are terminated correctly */

		for (k = 0; k < len; k++) {
			if (buf[k] == '\0') {
				break;
			}
		}

		if (k >= len) {
			break;
		}

		if (opt_val == 0) {
			if (strcasecmp(buf, option) == 0) {
				opt_found = 1;
			}
		} else {
			if (opt_found) {
				return buf;
			}
		}

		k++;

		buf += k;
		len -= k;

		opt_val ^= 1;
	}

	return NULL;
}
#endif

static int tftp_main(const int cmd, const u32_t serverip,
		   const char *remotefile, const int localfd,
		   const unsigned short port, int tftp_bufsize)
{
	struct sockaddr_in sa, from;
	struct timeval tv;
	socklen_t fromlen;
	fd_set rfds;
	int socketfd, len, opcode = 0, finished = 0;
	int timeout = TFTP_NUM_RETRIES;
	unsigned short block_nr = 1, tmp;
	char *cp, *buf;
#ifdef FEATURE_TFTP_BLOCKSIZE
	int want_option_ack = 0;
#endif

	buf = dim_sum_mem_alloc(tftp_bufsize += 4, MEM_NORMAL);
	if (!buf) {
		printk("alloc mem fail\n");
		return -1;
	}

	if ((socketfd = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		/* need to unlink the localfile, so don't use bb_xsocket here. */
		printk("socket\n");
		return -1;
	}

	len = sizeof(sa);

	memset(&sa, 0, len);
	//bind(socketfd, (struct sockaddr *)&sa, len);  /* LWIP¸æ¾¯£¬È¥µô */

	sa.sin_family = AF_INET;
	sa.sin_port = port;
	sa.sin_addr.s_addr = serverip;

	/* build opcode */
	if (cmd & tftp_cmd_get) {
		opcode = TFTP_RRQ;
	}
	if (cmd & tftp_cmd_put) {
		opcode = TFTP_WRQ;
	}

	while (1) {

		cp = buf;
		/* first create the opcode part */
		*((unsigned short *) cp) = htons(opcode);
		cp += 2;
		/* add filename and mode */
		if (((cmd & tftp_cmd_get) && (opcode == TFTP_RRQ)) ||
			((cmd & tftp_cmd_put) && (opcode == TFTP_WRQ)))
		{
			int too_long = 0;

			/* see if the filename fits into buf
			 * and fill in packet.  */
			len = strlen(remotefile) + 1;

			if ((cp + len) >= &buf[tftp_bufsize - 1]) {
				too_long = 1;
			} else {
				strncpy(cp, remotefile, len);
				cp += len;
			}

			if (too_long || ((&buf[tftp_bufsize - 1] - cp) < MODE_OCTET_LEN)) {
				printk("remote filename too long\n");
				break;
			}

			/* add "mode" part of the package */
			memcpy(cp, MODE_OCTET, MODE_OCTET_LEN);
			cp += MODE_OCTET_LEN;

#ifdef FEATURE_TFTP_BLOCKSIZE
			len = tftp_bufsize - 4;	/* data block size */

			if (len != TFTP_BLOCKSIZE_DEFAULT) {

				if ((&buf[tftp_bufsize - 1] - cp) < 15) {
					printk("remote filename too long\n");
					break;
				}

				/* add "blksize" + number of blocks  */
				memcpy(cp, OPTION_BLOCKSIZE, OPTION_BLOCKSIZE_LEN);
				cp += OPTION_BLOCKSIZE_LEN;
				cp += snprintf(cp, 6, "%d", len) + 1;
				want_option_ack = 1;
			}
#endif
		}

		/* add ack and data */

		if (((cmd & tftp_cmd_get) && (opcode == TFTP_ACK)) ||
			((cmd & tftp_cmd_put) && (opcode == TFTP_DATA))) {

			*((unsigned short *) cp) = htons(block_nr);

			cp += 2;

			block_nr++;

			if ((cmd & tftp_cmd_put) && (opcode == TFTP_DATA)) {
				len = lwip_file_read(localfd, cp, tftp_bufsize - 4);

				if (len < 0) {
					printk("read fail\n");
					break;
				} else {
					printk("*");
				}

				if (len != (tftp_bufsize - 4)) {
					finished++;
				}

				cp += len;
			}
		}

		/* send packet */
		timeout = TFTP_NUM_RETRIES;	/* re-initialize */
		do {
			int ret;

			len = cp - buf;

#ifdef TFTP_DEBUG
			printk("sending %u bytes\n", len);
			for (cp = buf; cp < &buf[len]; cp++)
				printk("%02x ", (unsigned char) *cp);
			printk("\n");
#endif

			if ((ret = sendto(socketfd, buf, len, 0,
					   (struct sockaddr *) &sa, sizeof(sa))) < 0) {
				printk("send fail, ret:%d\n", ret);
				len = -1;
				break;
			}


			if (finished && (opcode == TFTP_ACK)) {
				break;
			}

			/* receive packet */
			memset(&from, 0, sizeof(from));
			fromlen = sizeof(from);

			tv.tv_sec = TFTP_TIMEOUT;
			tv.tv_usec = 0;

			FD_ZERO(&rfds);
			FD_SET(socketfd, &rfds);

			switch (select(socketfd + 1, &rfds, NULL, NULL, &tv)) {
			case 1:
				len = recvfrom(socketfd, buf, tftp_bufsize, 0,
							   (struct sockaddr *) &from, &fromlen);

				if (len < 0) {
					printk("recvfrom fail\n");
					break;
				}

				timeout = 0;

				if (sa.sin_port == port) {
					sa.sin_port = from.sin_port;
				}
				if (sa.sin_port == from.sin_port) {
					break;
				}

				/* fall-through for bad packets! */
				/* discard the packet - treat as timeout */
				timeout = TFTP_NUM_RETRIES;
			case 0:
				printk("timeout\n");

				timeout--;
				if (timeout == 0) {
					len = -1;
					printk("last timeout\n");
				}
				break;
			default:
				printk("select\n");
				len = -1;
			}

		} while (timeout && (len >= 0));

		if ((finished) || (len < 0)) {
			break;
		}

		/* process received packet */

		opcode = ntohs(*((unsigned short *) buf));
		tmp = ntohs(*((unsigned short *) &buf[2]));

#ifdef TFTP_DEBUG
		printk("received %d bytes: %04x %04x\n", len, opcode, tmp);
#endif

		if (opcode == TFTP_ERROR) {
			const char *msg = NULL;

			if (buf[4] != '\0') {
				msg = &buf[4];
				buf[tftp_bufsize - 1] = '\0';
			} else if (tmp < (sizeof(tftp_bb_error_msg)
							  / sizeof(char *))) {

				msg = tftp_bb_error_msg[tmp];
			}

			if (msg) {
				printk("server says: %s\n", msg);
			}

			break;
		}

#ifdef FEATURE_TFTP_BLOCKSIZE
		if (want_option_ack) {

			want_option_ack = 0;

			if (opcode == TFTP_OACK) {

				/* server seems to support options */

				char *res;

				res = tftp_option_get(&buf[2], len - 2, OPTION_BLOCKSIZE);

				if (res) {
					int blksize = atoi(res);

					if (tftp_blocksize_check(blksize, tftp_bufsize - 4)) {

						if (cmd & tftp_cmd_put) {
							opcode = TFTP_DATA;
						} else {
							opcode = TFTP_ACK;
						}
#ifdef TFTP_DEBUG
						printk("using %s %u\n", OPTION_BLOCKSIZE,
								blksize);
#endif

						tftp_bufsize = blksize + 4;
						block_nr = 0;
						continue;
					}
				}
				/* FIXME:
				 * we should send ERROR 8 */
				printk("bad server option\n");
				break;
			}

			printk("warning: blksize not supported by server - reverting to 512\n");

			tftp_bufsize = TFTP_BLOCKSIZE_DEFAULT + 4;
		}
#endif

		if ((cmd & tftp_cmd_get) && (opcode == TFTP_DATA)) {

			if (tmp == block_nr) {

				len = lwip_file_write(localfd, &buf[4], len - 4);
				if (len < 0) {
					printk("write fail\n");
					break;
				} else {
					printk("*");
				}

				if (len != (tftp_bufsize - 4)) {
					finished++;
				}
				opcode = TFTP_ACK;
				continue;
			}
			/* in case the last ack disappeared into the ether */
			if (tmp == (block_nr - 1)) {
				--block_nr;
				opcode = TFTP_ACK;
				continue;
			} else if (tmp + 1 == block_nr) {
				/* Server lost our TFTP_ACK.  Resend it */
				block_nr = tmp;
				opcode = TFTP_ACK;
				continue;
			}
		}

		if ((cmd & tftp_cmd_put) && (opcode == TFTP_ACK)) {

			if (tmp == (unsigned short) (block_nr - 1)) {
				if (finished) {
					break;
				}

				opcode = TFTP_DATA;
				continue;
			}
		}
	}

	close(socketfd);
	dim_sum_mem_free(buf);

	return finished ? 0 : -1;
}

int net_tftp_cmd(int argc, char **args)
{
	int ret = -1, cmd, flags = 0, fd;
	const char *localfile, *remotefile;
	struct in_addr ipaddr;
	int blocksize = TFTP_BLOCKSIZE_DEFAULT;

	if (tftp_is_running == 1) {
		printk("tftp is running, please wait a moment and retry...\n");
		return 0;
	} else {
		tftp_is_running = 1;
	}

	if (argc != 4) {
		printk("Usage: tftp get/put filename serverip\n");
		goto end;
	}

	if (!strcmp(args[1], "get")) {
		cmd = tftp_cmd_get;
		flags = O_WRONLY | O_CREAT | O_TRUNC;
	} else if (!strcmp(args[1], "put")) {
		cmd = tftp_cmd_put;
		flags = O_RDONLY;
	} else {
		printk("Usage: tftp get/put filename serverip\n");
		goto end;
	}

	if (inet_aton((const char *)args[3], &ipaddr) == 0) {
		printk("Invalid ip address, Usage: tftp get/put filename serverip\n");
		goto end;
	}

	localfile = remotefile = args[2];
	fd = lwip_file_open(localfile, flags, 0644); /* fail below */
	if (fd < 0) {
		printk("open file %s fail\n", args[2]);
		goto end;
	}

	ret = tftp_main(cmd, ipaddr.s_addr, remotefile, fd, htons(TFTP_PORT), blocksize);
	if (ret == 0) {
		printk("\n tftp done!\n");
	} else {
		printk("\n tftp fail!\n");
	}

	close(fd);

	if ((cmd == TFTP_RRQ) && (ret != 0)) {
		lwip_file_unlink(localfile);
	}

end:
	tftp_is_running = 0;

	return ret;
}

