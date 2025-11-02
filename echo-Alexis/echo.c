/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "hardware/i2c.h"

#include "include/tcp_server.h"
#include "include/led.h"
#include "include/wifi_login.h"
#include "include/wifi_connect.h"
#include "include/i2cSend.h"
#include "include/digitRenderer.h"

#define I2C_SDA 0
#define I2C_SCL 1

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
    // IP4ADDR
    // uint64_t t_us_previous = time_us_64();
    // uint64_t t_us_current = time_us_64();


    i2c_init(i2c_default, 400 * 1000); // Initialize I2C with clock fequency 400kHz
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    sleep_ms(200);
    initDisplay();
    renderSymbol(RPI,1,56); // Render the Rpi Symbol
    renderIPString(IP4ADDR);

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