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
#define BUF_SIZE 512


typedef struct TCP_SERVER_T_ {
    struct tcp_pcb *server_pcb;
    struct tcp_pcb *client_pcb;
    bool complete;
    uint8_t buffer_sent[BUF_SIZE];
    uint8_t buffer_recv[BUF_SIZE];
    int sent_len;
    int recv_len;
    int run_count;
} TCP_SERVER_T;



TCP_SERVER_T* tcp_server_start(void);
void tcp_server_stop(TCP_SERVER_T *state);
err_t tcp_server_send(TCP_SERVER_T *state, const uint8_t *data, size_t len);
size_t tcp_server_receive(TCP_SERVER_T *state, uint8_t *buffer, size_t max_len);

#endif /* TCP_SERVER_H */