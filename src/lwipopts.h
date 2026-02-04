#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

// NO_SYS 1 to use pico_cyw43_arch_lwip_threadsafe_background
#define NO_SYS                      1
#define LWIP_SOCKET                 0
#define LWIP_NETCONN                0

#define LWIP_NETIF_HOSTNAME         1
#define LWIP_DHCP                   1
#define LWIP_DNS                    1

#define LWIP_TIMEVAL_PRIVATE        0

#define SYS_LIGHTWEIGHT_PROT        1
#define MEM_ALIGNMENT               4

#define TCPIP_THREAD_STACKSIZE      1024
#define TCPIP_THREAD_PRIO           (configMAX_PRIORITIES - 2)
#define TCPIP_MBOX_SIZE             8
#define DEFAULT_TCP_RECVMBOX_SIZE   8
#define DEFAULT_UDP_RECVMBOX_SIZE   8
#define DEFAULT_RAW_RECVMBOX_SIZE   8
#define DEFAULT_ACCEPTMBOX_SIZE     8

#endif