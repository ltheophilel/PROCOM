#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#include "../include/wifi_login.h"


err_t connect_to_wifi(const char *ssid, const char *password, const int timeout_ms, const bool init_wifi);
err_t wifi_auto_connect(void);



