#ifndef _LINUX_CONSOLE_H_
#define _LINUX_CONSOLE_H_ 1

struct console
{
	char	name[8];
	void	(*write)(struct console *, const char *, unsigned);
	int	(*read)(struct console *, char *, unsigned);
	void	(*unblank)(void);
	int	(*setup)(struct console *, char *);
	short	flags;
	short	index;
	int	cflag;
	void	*data;
	struct	 console *next;
};
#endif
