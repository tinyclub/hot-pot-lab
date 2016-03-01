

/**********************************************************
 *                       Includers                        *
 **********************************************************/
#include "sh_utils.h"
#include "sh_command.h"

/**********************************************************
 *                         Macro                          *
 **********************************************************/

/**********************************************************
 *                  Extern Declareation                   *
 **********************************************************/
extern int net_ping_cmd(int argc, char *argv[]);
extern int net_tftp_cmd(int argc, char *argv[]);
extern int net_ip_cmd(int argc, char *argv[]);
extern int net_tcptest_cmd(int argc, char **args);
extern int net_caep_test_cmd(int argc, char **args);

/**********************************************************
 *                    Global Variables                    *
 **********************************************************/

/**********************************************************
 *                    Static Variables                    *
 **********************************************************/

/**********************************************************
 *                       Implements                       *
 **********************************************************/
static int sh_tftp_cmd(int argc, char *argv[])
{
	return net_tftp_cmd(argc, argv);
}

static int sh_ping_cmd(int argc, char *argv[])
{
	return net_ping_cmd(argc, argv);
}

static int sh_ip_cmd(int argc, char *argv[])
{
	return net_ip_cmd(argc, argv);
}

static int sh_tcptest_cmd(int argc, char *argv[])
{
	return net_tcptest_cmd(argc, argv);
}

void _initialize_network_cmds(void)
{
	register_shell_command("ip", sh_ip_cmd, 
		"Show the network address or set the network address", 
		"ip show|set ipaddr netmask", 
		"This command shows the network address or sets the network address.", 
		sh_noop_completer);

	register_shell_command("tftp", sh_tftp_cmd,
		"Upload or download file", 
		"tftp get|put filename serverip", 
		"This command uploads file to server or downloads the file from server.",
		sh_noop_completer);

	register_shell_command("ping", sh_ping_cmd,
		"Send ICMP echo request packets, and waiting for replies", 
		"ping ipaddr", 
		"This command tests that a remote host is reachable by sending ICMP echo \n\t"
		"request packets, and waiting for replies.", 
		sh_noop_completer);

	register_shell_command("tcptest", sh_tcptest_cmd,
		"Tcp test", 
		"tcptest/tcptest svraddr", 
		"tcptest -- start a tcp server, tcptest svraddr -- start a tcp client",
		sh_noop_completer);

	return;
}



