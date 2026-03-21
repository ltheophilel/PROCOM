#ifndef WIFI_CONNECT_H
#define WIFI_CONNECT_H

typedef struct wifi_network_t {
    const char *ssid;
    const char *password;
} wifi_network_t;

// Liste des réseaux Wi-Fi connus (SSID et mot de passe) pour la connexion automatique
static wifi_network_t known_networks[] = {
    {"WIFI_SSID", "WIFI_PASSWORD"},
    {"example_wifi_name", "example_password"}
    //{"invite", NULL} // NULL password indicates an open network
};
static int known_networks_count = sizeof(known_networks) / sizeof(known_networks[0]);

#endif /* WIFI_CONNECT_H */