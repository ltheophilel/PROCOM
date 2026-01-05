#define WIFI_SSID_colloc "JOSEPH91_2"
#define WIFI_PASSWORD_colloc "JOSEPH91_2"

#define WIFI_SSID_iphone "Lathyos´s iPhone"
#define WIFI_PASSWORD_iphone "0766020128"

#define WIFI_SSID_maison "Livebox-DD76"
#define WIFI_PASSWORD_maison "rGUZuDsVDzJgirwmRU"



#ifndef WIFI_CONNECT_H
#define WIFI_CONNECT_H

typedef struct wifi_network_t {
    const char *ssid;
    const char *password;
} wifi_network_t;

static wifi_network_t known_networks[] = {
    {WIFI_SSID_colloc, WIFI_PASSWORD_colloc},
    {WIFI_SSID_iphone, WIFI_PASSWORD_iphone},
    {WIFI_SSID_maison, WIFI_PASSWORD_maison}
    // {"Theophile", "AAAAAAAA"}//,
    //{"invite", NULL}
};
static int known_networks_count = sizeof(known_networks) / sizeof(known_networks[0]);

#endif /* WIFI_CONNECT_H */