/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "include/tcp_server.h"
#include "include/led.h"
#include "include/wifi_login.h"
#include "include/wifi_connect.h"

#define WIFI_SSID WIFI_SSID_colloc
#define WIFI_PASSWORD WIFI_PASSWORD_colloc

int main() {
    stdio_init_all();
    // sleep_ms(10000);

    // err_t wifi_fail = connect_to_wifi(WIFI_SSID, WIFI_PASSWORD, 10000);
    // if (!wifi_fail) {
    //     pico_set_led(true); // on success
    // }
    wifi_auto_connect();
    pico_set_led(true); 
    
    TCP_SERVER_T* state = tcp_server_start();
    // uint64_t t_us_previous = time_us_64();
    // uint64_t t_us_current = time_us_64();
    uint8_t rx_buffer[BUF_SIZE];
    while (true) {
        size_t received = tcp_server_receive(state, rx_buffer, BUF_SIZE);
        if (received != 0) {
            printf("Received: %zu\n", received); // Affiche la valeur de 'received'
            pico_toggle_led();
            tcp_server_send(state, rx_buffer, received);
        }
        
        // uint64_t t_us_current = time_us_64();
        // if (t_us_current - t_us_previous >= 1000000) { // 1 second
        //     t_us_previous = t_us_current;
        //     pico_toggle_led();
        //     // printf("hello\n");
        //     tcp_server_send(state, (const uint8_t *)"Hello, client!", 14);
        // }
    }

    tcp_server_stop(state);
    pico_set_led(false);
    cyw43_arch_deinit();
    return 0;
}