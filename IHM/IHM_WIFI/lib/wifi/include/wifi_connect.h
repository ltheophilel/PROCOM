#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#include "../include/wifi_login.h"


// void start_wifi_scan(void);
err_t connect_to_wifi(const char *ssid, const char *password, const int timeout_ms, const bool init_wifi);
err_t wifi_auto_connect(void);
// void connect_to_any_known_wifi(void);
// static int scan_result(void *env, const cyw43_ev_scan_result_t *result);
// // Start a wifi scan
// static void scan_worker_fn(async_context_t *context, async_at_time_worker_t *worker);



