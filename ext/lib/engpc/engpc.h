#ifndef __ENGPC_H__
#define __ENGPC_H__

#define UART_DEV	"UART_2"
#define ENG_LOG(...)	    printf(__VA_ARGS__)
//#define ENG_LOG(fmt,...) do {}while(0)
#define DATA_EXT_DIAG_SIZE (1024)

extern int get_ser_diag_fd(void);

#endif
