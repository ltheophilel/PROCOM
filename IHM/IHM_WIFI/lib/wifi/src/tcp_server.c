
#include "../include/tcp_server.h"

char *IP4ADDR = "0.0.0.0";

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
    pbuf_free(p);

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



err_t tcp_server_send(TCP_SERVER_T *state, const char *msg) {
    if (!state->client_pcb) return ERR_CLSD;
    return tcp_write(state->client_pcb, msg, strlen(msg), TCP_WRITE_FLAG_COPY);
}
