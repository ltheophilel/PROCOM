
#include "../include/tcp_server.h"

char *IP4ADDR = "0.0.0.0";
static uint8_t header[3]; // En-tête : [type(1), taille(2)]
static uint8_t buffer[BUF_SIZE + 3]; // Buffer pour données + en-tête
static size_t sent = 0;
static size_t chunk;

static err_t on_tcp_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;

    if (!p) {
        state->client_pcb = NULL;
        return tcp_close(tpcb);
    }

    // Copier les données reçues
    size_t len = p->tot_len;
    if (len > sizeof(state->buffer_recv) - state->recv_len)
        len = sizeof(state->buffer_recv) - state->recv_len;

    pbuf_copy_partial(p, state->buffer_recv + state->recv_len, len, 0);
    state->recv_len += len;
    tcp_recved(tpcb, p->tot_len);
    printf("Received %u bytes, total buffered: %u bytes\n", (unsigned int)len, (unsigned int)state->recv_len);
    if (p->ref == 1) {
        pbuf_free(p);  // Libérer uniquement si c'est la dernière référence
    }


    return ERR_OK;
}

static err_t on_tcp_accept(void *arg, struct tcp_pcb *newpcb, err_t err) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;

    printf("Client connected\n");
    state->client_pcb = newpcb;

    tcp_arg(newpcb, state);
    tcp_recv(newpcb, on_tcp_recv);

    return ERR_OK;
}

TCP_SERVER_T* tcp_server_start(void) {
    TCP_SERVER_T *state = calloc(1, sizeof(TCP_SERVER_T));

    state->server_pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    tcp_bind(state->server_pcb, NULL, TCP_PORT); // 6543
    state->server_pcb = tcp_listen_with_backlog(state->server_pcb, 1);

    tcp_arg(state->server_pcb, state);
    tcp_accept(state->server_pcb, on_tcp_accept);
    DEBUG_printf("Starting server at %s on port %u\n", ip4addr_ntoa(netif_ip4_addr(netif_list)), TCP_PORT);
    IP4ADDR = ip4addr_ntoa(netif_ip4_addr(netif_list));
    return state;
}

size_t tcp_server_receive(TCP_SERVER_T *state, uint8_t *buf, size_t maxlen) {
    size_t n = state->recv_len < maxlen ? state->recv_len : maxlen;
    memcpy(buf, state->buffer_recv, n);
    memmove(state->buffer_recv, state->buffer_recv + n, state->recv_len - n);
    state->recv_len -= n;
    return n;
}



err_t tcp_server_send(TCP_SERVER_T *state, const char *msg, PACKET_TYPE type) {
    if (!state->client_pcb) return ERR_CLSD;

    // Construire l'en-tête
    header[0] = type;
    chunk = strlen(msg);
    header[1] = (chunk >> 8) & 0xFF; // Octet haut de la taille
    header[2] = chunk & 0xFF;         // Octet bas de la taille

    // Copier l'en-tête + les données dans le buffer
    memcpy(buffer, header, 3);
    memcpy(buffer + 3, msg, chunk);

    err_t err = tcp_write(state->client_pcb, buffer, chunk + 3, TCP_WRITE_FLAG_COPY);
    if (err == ERR_OK) {
        tcp_output(state->client_pcb);
        while (state->client_pcb->unsent != NULL) {
            sleep_ms(10); // Attendre 10ms entre chaque envoi
        }

    } 
    return err;

    // return tcp_write(state->client_pcb, msg, strlen(msg), TCP_WRITE_FLAG_COPY);
}


#include <string.h> // Pour memcpy

err_t tcp_send_large_img(TCP_SERVER_T *state, const char *data, size_t len) {
    sent = 0;

    while (sent < len) {
        chunk = len - sent;
        if (chunk > BUF_SIZE) {
            chunk = BUF_SIZE;
        }

        // Déterminer le type de paquet
        uint8_t packet_type;
        if (sent == 0 && chunk >= len) {
            packet_type = PACKET_TYPE_ALL_IMG; // Image complète en un seul paquet
        } else if (sent == 0) {
            packet_type = PACKET_TYPE_START_IMG; // Début de l'image
        } else if (sent + chunk >= len) {
            packet_type = PACKET_TYPE_END_IMG; // Dernier paquet
        } else {
            packet_type = PACKET_TYPE_IMG; // Milieu de l'image
        }

        // Construire l'en-tête
        header[0] = packet_type;
        header[1] = (chunk >> 8) & 0xFF; // Octet haut de la taille
        header[2] = chunk & 0xFF;         // Octet bas de la taille

        // Copier l'en-tête + les données dans le buffer
        memcpy(buffer, header, 3);
        memcpy(buffer + 3, data + sent, chunk);

        // Envoyer le buffer
        err_t err = tcp_write(state->client_pcb, buffer, chunk + 3, TCP_WRITE_FLAG_COPY);
        if (err == ERR_OK) {
            sent += chunk;
            tcp_output(state->client_pcb);
            while (state->client_pcb->unsent != NULL) {
                sleep_ms(10); // Attendre 10ms entre chaque envoi
            }

        } else if (err == ERR_MEM) {
            sleep_ms(1);
            continue;
        } else {
            return err;
        }
    }
    printf("Sent: %d/%d, unsent: %d\n", sent, len, state->client_pcb->unsent ? 1 : 0);
    return ERR_OK;
}

