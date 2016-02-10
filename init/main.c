#include <linux/stddef.h>
#include <hot-pot/console.h>

extern void early_vga_write(struct console *con, const char *str, unsigned n);
extern void realmode_putstr(const char *s);
extern void early_serial_init(int port, int baud);
extern void serial_putstr(const char *s);
extern void serial_putchar(int ch);


void start_kernel(void)
{
	early_serial_init(0x3f8,115200);
	serial_putchar('x');
	serial_putchar('b');
	serial_putchar('y');
	serial_putchar('_');
	serial_putchar('d');
	serial_putchar('e');
	serial_putchar('b');
	serial_putchar('u');
	serial_putchar('g');
	serial_putchar('!');
	serial_putstr("hello hot-pot\n");
	//early_vga_write(NULL, "hello hot-pot\n", 14);
	while (1);
}

