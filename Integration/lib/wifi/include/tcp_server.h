/* led.h - simple LED management for Pico / Pico W
 * Provides pico_led_init() and pico_set_led()
 */
#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <string.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
#endif

#define TCP_PORT 4242
#define DEBUG_printf printf
#define BUF_SIZE 1400 // 1024
#define LEN_GENERAL_MSG 20
#define LEN_FLOAT_MSG 8

extern char *IP4ADDR;

// typedef struct TCP_SERVER_T_ {
//     struct tcp_pcb *server_pcb;
//     struct tcp_pcb *client_pcb;
//     bool complete;
//     uint8_t buffer_sent[BUF_SIZE];
//     uint8_t buffer_recv[BUF_SIZE];
//     int sent_len;
//     int recv_len;
//     int run_count;
// } TCP_SERVER_T;


typedef struct {
    struct tcp_pcb *server_pcb;
    struct tcp_pcb *client_pcb;
    uint8_t buffer_recv[256];
    size_t recv_len;
} TCP_SERVER_T;

typedef enum {
    PACKET_TYPE_START_IMG = 0,
    PACKET_TYPE_IMG = 1,
    PACKET_TYPE_END_IMG = 2,
    PACKET_TYPE_MOT_0 = 3,
    PACKET_TYPE_MOT_1 = 4,
    PACKET_TYPE_GENERAL = 5,
    PACKET_TYPE_ALL_IMG = 6,
    PACKET_TYPE_ALL_IN_ONE = 7
} PACKET_TYPE;

TCP_SERVER_T* tcp_server_start(void);
// void tcp_server_stop(TCP_SERVER_T *state);
err_t tcp_server_send(TCP_SERVER_T *state, const char *msg, PACKET_TYPE type);
size_t tcp_server_receive(TCP_SERVER_T *state, uint8_t *buffer, size_t max_len);
err_t tcp_send_large_img(TCP_SERVER_T *state, const char *data, size_t len);
err_t tcp_server_send_all_in_one(TCP_SERVER_T *state, 
                                const char *general_msg, 
                                const int v_mot_droit,
                                const int v_mot_gauche,
                                const double p,
                                const double m,
                                const double angle,
                                const double p_aplati,
                                const double m_aplati,
                                const double angle_aplati,  
                                const uint8_t *coded_image, 
                                size_t len_img);
#endif /* TCP_SERVER_H */