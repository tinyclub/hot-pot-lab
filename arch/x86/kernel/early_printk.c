#include <asm/io.h>
#include <hot-pot/console.h>

/* Simple VGA output */
#define VGABASE		(__ISA_IO_base + 0xb0000)

static int max_ypos = 25, max_xpos = 80;
static int current_ypos = 25, current_xpos;

static char *vidmem;
static int vidport;
static int lines, cols;
__u8  orig_x;		/* 0x00 */
__u8  orig_y;		/* 0x01 */


#define XMTRDY          0x20

#define DEFAULT_SERIAL_PORT 0x3f8 /* ttyS0 */

#define DLAB		0x80

#define TXR             0       /*  Transmit register (WRITE) */
#define RXR             0       /*  Receive register  (READ)  */
#define IER             1       /*  Interrupt Enable          */
#define IIR             2       /*  Interrupt ID              */
#define FCR             2       /*  FIFO control              */
#define LCR             3       /*  Line control              */
#define MCR             4       /*  Modem control             */
#define LSR             5       /*  Line Status               */
#define MSR             6       /*  Modem Status              */
#define DLL             0       /*  Divisor Latch Low         */
#define DLH             1       /*  Divisor latch High        */

int early_serial_base;

void early_serial_init(int port, int baud)
{
	unsigned char c;
	unsigned divisor;

	outb(0x3, port + LCR);	/* 8n1 */
	outb(0, port + IER);	/* no interrupt */
	outb(0, port + FCR);	/* no fifo */
	outb(0x3, port + MCR);	/* DTR + RTS */

	divisor	= 115200 / baud;
	c = inb(port + LCR);
	outb(c | DLAB, port + LCR);
	outb(divisor & 0xff, port + DLL);
	outb((divisor >> 8) & 0xff, port + DLH);
	outb(c & ~DLAB, port + LCR);

	early_serial_base = port;
}


void serial_putchar(int ch)
{
	unsigned timeout = 0xffff;
	early_serial_base = 0x3f8;
	int loop = 0;
	for (loop = 0; loop < 0xfffff; loop++)
	{
		;
	}
	while ((inb(early_serial_base + LSR) & XMTRDY) == 0 && --timeout)
		cpu_relax();

	if (timeout > 0)
		outb(ch, early_serial_base + TXR);
	else
		//fixme: inifinite retry.
		serial_putchar(ch);
}

void serial_putstr(const char *s)
{
	int x, y, pos;
	char c;

	if (early_serial_base) {
		const char *str = __pa(s);
		while (*str) {
			if (*str == '\n')
				serial_putchar('\r');
			serial_putchar(*str++);
		}
	}
}


static void scroll(void)
{
	int i;

	memcpy(vidmem, vidmem + cols * 2, (lines - 1) * cols * 2);
	for (i = (lines - 1) * cols * 2; i < lines * cols * 2; i += 2)
		vidmem[i] = ' ';
}

void vga_putstr(const char *s)
{
	int x, y, pos;
	char c;

	vidmem = (char *) 0xb8000 + 0xC000000;
	vidport = 0x3d4;

	x = orig_x;
	y = orig_y;

	while ((c = *s++) != '\0') {
		if (c == '\n') {
			x = 0;
			if (++y >= lines) {
				scroll();
				y--;
			}
		} else {
			vidmem[(x + cols * y) * 2] = c;
			if (++x >= cols) {
				x = 0;
				if (++y >= lines) {
					scroll();
					y--;
				}
			}
		}
	}

	orig_x = x;
	orig_y = y;

	pos = (x + cols * y) * 2;	/* Update cursor position */
	outb(14, vidport);
	outb(0xff & (pos >> 9), vidport+1);
	outb(15, vidport);
	outb(0xff & (pos >> 1), vidport+1);
}

void early_vga_write(struct console *con, const char *str, unsigned n)
{
	char c;
	int  i, k, j;

//return;
	while ((c = *str++) != '\0' && n-- > 0) {
		if (current_ypos >= max_ypos) {
			/* scroll 1 line up */
			for (k = 1, j = 0; k < max_ypos; k++, j++) {
				for (i = 0; i < max_xpos; i++) {
					writew(readw(VGABASE+2*(max_xpos*k+i)),
					       VGABASE + 2*(max_xpos*j + i));
				}
			}
			for (i = 0; i < max_xpos; i++)
				writew(0x720, VGABASE + 2*(max_xpos*j + i));
			current_ypos = max_ypos-1;
		}

		if (c == '\n') {
			current_xpos = 0;
			current_ypos++;
		} else if (c != '\r')  {
			writew(((0x7 << 8) | (unsigned short) c),
			       VGABASE + 2*(max_xpos*current_ypos +
						current_xpos++));
			if (current_xpos >= max_xpos) {
				current_xpos = 0;
				current_ypos++;
			}
		}
	}
}

