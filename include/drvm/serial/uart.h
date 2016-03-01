#ifndef _VIRT_UART_H
#define _VIRT_UART_H

typedef struct outbyte_struct{
      volatile unsigned char *tx_fifo;
      volatile unsigned int *tx_pro;
      volatile unsigned int  *tx_con;
      volatile unsigned char *rx_fifo;
      volatile unsigned int *rx_pro;
      volatile unsigned int *rx_con;
      volatile int *status;
}virt_uart;

#define MOSM_WRITING 0x100
#define MOSM_IDLE 0x0
#define USER_RESULT_SIZE (7*1024)
#define VIRT_USER_CMD_SIZE (1*1024)
#endif

