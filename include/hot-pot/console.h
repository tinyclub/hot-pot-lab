#ifndef _HOT_POT_CONSOLE_H
#define _HOT_POT_CONSOLE_H

struct console
{
	char	name[16];
	void	(*write)(struct console *, const char *, unsigned);
	int	(*read)(struct console *, char *, unsigned);
};

#endif
