#include "include/wifi_connect.h"

static wifi_network_t network = {NULL, NULL};

err_t connect_to_wifi(const char *ssid, const char *password, const int timeout_ms, const bool init_wifi) {
    if (init_wifi) {
        if (cyw43_arch_init()) {
            printf("failed to initialise\n");
            return -1;
        }
    }
    cyw43_arch_enable_sta_mode();

    printf("Connecting to Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(ssid, password, CYW43_AUTH_WPA2_AES_PSK, timeout_ms)) {
        printf("failed to connect.\n");
        cyw43_arch_deinit();
        return -1;
    } else {
        printf("Connected.\n");
        pico_toggle_led();
    }
    return 0;
}




// Callback pour traiter les résultats du scan (version bloquante)
static int scan_result(void *env, const cyw43_ev_scan_result_t *result) {
    if (!result) return 0;
    printf("Réseau détecté : %.*s (RSSI: %d dBm)\n", result->ssid_len, result->ssid, result->rssi);
    // Compare le SSID détecté avec les SSID connus
    for (int i = 0; i < known_networks_count; i++) {
        if (result->ssid_len == strlen(known_networks[i].ssid) &&
            memcmp(result->ssid, known_networks[i].ssid, result->ssid_len) == 0) {
                ((wifi_network_t*)env)->ssid = known_networks[i].ssid;
                ((wifi_network_t*)env)->password = known_networks[i].password;
                return 1;  // Réseau connu trouvé
        }
    }
    return 0;  // Continue le scan
}

// Fonction principale pour scanner et se connecter
err_t wifi_auto_connect(void) {
    // Initialise le Wi-Fi
    if (cyw43_arch_init()) {
        printf("Échec de l'initialisation Wi-Fi\n");
        return -1;
    }
    // https://forums.raspberrypi.com/viewtopic.php?t=384766
    // Configuration du scan
    cyw43_arch_enable_sta_mode();
    cyw43_wifi_scan_options_t scan_options = {0};
    printf("Lancement du scan Wi-Fi...\n");
    // Lance un scan bloquant (attend la fin du scan)
    
    int err = cyw43_wifi_scan(&cyw43_state, &scan_options, &network, scan_result);
    while(cyw43_wifi_scan_active(&cyw43_state)) {
        cyw43_arch_poll();
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(1000));
    }
    printf("Scan terminé.\n");
    printf("Connexion à %s\n", network.ssid);
    printf("mdp : %s\n", network.password);
    // sleep_ms(1000); // Attendre un peu avant de tenter la connexion

    if (network.ssid == NULL) {
        printf("Aucun réseau connu trouvé.\n");
        return -1;
    }

    if (connect_to_wifi(network.ssid, network.password, 10000, false) == 0) {
        printf("Connecté avec succès à %s !\n", network.ssid);
        return 1;  // Arrête le scan après connexion
    } else {
        printf("Échec de la connexion à %s.\n", network.ssid);
        return 0;  // Continue le scan si la connexion échoue
    }
    return err;
}

