#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#include <dim-sum/task.h>
#include <dim-sum/time.h>

static int count = 0;
int task1(void *data)
{
	while (1)
	{
		SysSleep(100);
		count++; 
	}
}

#if 0
static void write_jiffies64_to_file(char *filename)
{
	int fd;
	unsigned long long jiffies = get_jiffies_64();
	int len;
	
  	fd = open(filename, O_RDWR | O_CREAT, 0644); /* fail below */
	len = write(fd, &jiffies, sizeof(jiffies));
	close(fd);
	printf("write %llu to [%s], %d\n", jiffies, filename, len);
}

static void cat_file(char *filename)
{
	int fd;
	unsigned long long jiffies = 0;

	fd = open(filename, O_RDWR | O_CREAT, 0644); /* fail below */
	read(fd, &jiffies, sizeof(jiffies));
	close(fd);

	printf("xby_debug in cat_file, jiffies is %llu\n", jiffies);
}
#else
static void write_jiffies64_to_file(char *filename)
{
	FILE *fd;
	unsigned long long jiffies = get_jiffies_64();
	int len;
	
  	fd = fopen(filename, "w"); /* fail below */
	len = fwrite(&jiffies, sizeof(jiffies), 1, fd);
	fclose(fd);
	printf("write %llu to [%s], %d\n", jiffies, filename, len);
}

static void read_jiffies64_from_file(char *filename)
{
	FILE *fd;
	unsigned long long jiffies = 0;

	fd = fopen(filename, "r"); /* fail below */
	fread(&jiffies, sizeof(jiffies), 1, fd);
	fclose(fd);

	printf("xby_debug in cat_file, jiffies is %llu\n", jiffies);
}

static void cat_file(char *filename)
{
	FILE *fd;
	char buf[255];

	fd = fopen(filename, "r"); /* fail below */
	fread(buf, sizeof(buf), 1, fd);
	fclose(fd);

	printf("file content:\n%s\n", buf);
}

#endif

int xby_test_cmd(int argc, char *argv[])
{
	int fun = 1;
	
	
	if (argc >= 2)
	{
		fun = atoi(argv[1]);
	}

	if (fun == 1)
	{
		printf("xby_debug in xby_test_cmd, sleep count:[%d], tick:[%llu]\n", count, get_jiffies_64());
	}
	else if (fun == 2)
	{
		if (argc >= 3){
			write_jiffies64_to_file(argv[2]);
		}
		else {
			printf("sorry, can not understand you!\n");
		}
	}
	else if (fun == 3)
	{
		if (argc >= 3){
			read_jiffies64_from_file(argv[2]);
		}
		else {
			printf("sorry, can not understand you!\n");
		}
	}
	else if (fun == 4)
	{
		if (argc >= 3){
			cat_file(argv[2]);
		}
		else {
			printf("sorry, can not understand you!\n");
		}
	}
	else if (fun == 5)
	{
		sync();
	}

		
	return 0;
}



extern int dim_sum_lwip_init(void);
extern int cpsw_net_init(void);
extern int start_shell(char *str, int prio, void *arg);
/**
 * 用户应用程序入口，在系统完全初始化完成后运行
 * 系统处于开中断状态!!!!!!
 * 不能在此过程中阻塞 !!!!!!
 */
void usrAppInit(void)
{
	SysTaskCreate(task1,
			NULL,
			"task1",
			10,
			NULL,
			0
		);
	//to do
	/** 注册cpsw网卡驱动 **/
	cpsw_net_init();
	/** 初始化lwip协议栈 **/
	dim_sum_lwip_init();

	start_shell("shell", 6, NULL);	
}

