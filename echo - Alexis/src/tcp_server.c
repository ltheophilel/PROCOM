

#include "include/tcp_server.h"
#define TEST_ITERATIONS 10
#define POLL_TIME_S 5

/// @brief Initialise the TCP server state structure
/// @return Pointer to allocated and initialised state, or NULL on failure
static TCP_SERVER_T* tcp_server_init(void) {
    TCP_SERVER_T *state = calloc(1, sizeof(TCP_SERVER_T));
    if (!state) {
        DEBUG_printf("failed to allocate state\n");
        return NULL;
    }
    return state;
}

/// @brief Close the TCP server and client connections and free resources
/// @param arg Pointer to TCP server state
/// @return lwIP error code
static err_t tcp_server_close(void *arg) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    err_t err = ERR_OK;
    if (state->client_pcb != NULL) {
        tcp_arg(state->client_pcb, NULL);
        tcp_poll(state->client_pcb, NULL, 0);
        tcp_sent(state->client_pcb, NULL);
        tcp_recv(state->client_pcb, NULL);
        tcp_err(state->client_pcb, NULL);
        err = tcp_close(state->client_pcb);
        if (err != ERR_OK) {
            DEBUG_printf("close failed %d, calling abort\n", err);
            tcp_abort(state->client_pcb);
            err = ERR_ABRT;
        }
        state->client_pcb = NULL;
    }
    if (state->server_pcb) {
        tcp_arg(state->server_pcb, NULL);
        tcp_close(state->server_pcb);
        state->server_pcb = NULL;
    }
    return err;
}

/// @brief Handle test result, mark complete and close connections
/// @param arg Pointer to TCP server state
/// @param status 0 for success, non-zero for failure
/// @return lwIP error code
static err_t tcp_server_result(void *arg, int status) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    if (status == 0) {
        DEBUG_printf("test success\n");
    } else {
        DEBUG_printf("test failed %d\n", status);
    }
    state->complete = true;
    return tcp_server_close(arg);
}

/// @brief Callback from lwIP when data has been sent
/// @param arg Pointer to TCP server state
/// @param tpcb Pointer to TCP protocol control block
/// @param len Number of bytes sent
/// @return lwIP error code
static err_t tcp_server_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    DEBUG_printf("tcp_server_sent %u\n", len);
    state->sent_len += len;

    if (state->sent_len >= BUF_SIZE) {

        // We should get the data back from the client
        state->recv_len = 0;
        DEBUG_printf("Waiting for buffer from client\n");
    }

    return ERR_OK;
}

/// @brief Send a buffer of random data to the client
/// @param arg Pointer to TCP server state
/// @param tpcb Pointer to TCP protocol control block
/// @return lwIP error code
static err_t tcp_server_send_data(void *arg, struct tcp_pcb *tpcb)
{
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    for(int i=0; i< BUF_SIZE; i++) {
        state->buffer_sent[i] = rand();
    }

    state->sent_len = 0;
    DEBUG_printf("Writing %ld bytes to client\n", BUF_SIZE);
    // this method is callback from lwIP, so cyw43_arch_lwip_begin is not required, however you
    // can use this method to cause an assertion in debug mode, if this method is called when
    // cyw43_arch_lwip_begin IS needed
    cyw43_arch_lwip_check();
    err_t err = tcp_write(tpcb, state->buffer_sent, BUF_SIZE, TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK) {
        DEBUG_printf("Failed to write data %d\n", err);
        return tcp_server_result(arg, -1);
    }
    return ERR_OK;
}

/// @brief Callback from lwIP when data has been received
/// @param arg Pointer to TCP server state
/// @param tpcb Pointer to TCP protocol control block
/// @param p Pointer to received pbuf
/// @param err lwIP error code
/// @return lwIP error code
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    if (!p) {
        return tcp_server_result(arg, -1);
    }
    // this method is callback from lwIP, so cyw43_arch_lwip_begin is not required, however you
    // can use this method to cause an assertion in debug mode, if this method is called when
    // cyw43_arch_lwip_begin IS needed
    cyw43_arch_lwip_check();
    if (p->tot_len > 0) {
        DEBUG_printf("tcp_server_recv %d/%d err %d\n", p->tot_len, state->recv_len, err);

        // Receive the buffer
        const uint16_t buffer_left = BUF_SIZE - state->recv_len;
        state->recv_len += pbuf_copy_partial(p, state->buffer_recv + state->recv_len,
                                             p->tot_len > buffer_left ? buffer_left : p->tot_len, 0);
        tcp_recved(tpcb, p->tot_len);
    }
    pbuf_free(p);

    // Have we have received the whole buffer
    if (state->recv_len == BUF_SIZE) {

        // check it matches
        if (memcmp(state->buffer_sent, state->buffer_recv, BUF_SIZE) != 0) {
            DEBUG_printf("buffer mismatch\n");
            return tcp_server_result(arg, -1);
        }
        DEBUG_printf("tcp_server_recv buffer ok\n");

        // Test complete?
        state->run_count++;
        if (state->run_count >= TEST_ITERATIONS) {
            tcp_server_result(arg, 0);
            return ERR_OK;
        }

        // Send another buffer
        return tcp_server_send_data(arg, state->client_pcb);
    }
    return ERR_OK;
}

/// @brief Callback from lwIP when polling the connection
/// @param arg Pointer to TCP server state
/// @param tpcb Pointer to TCP protocol control block
/// @return lwIP error code
static err_t tcp_server_poll(void *arg, struct tcp_pcb *tpcb) {
    DEBUG_printf("tcp_server_poll_fn\n");
    return tcp_server_result(arg, -1); // no response is an error?
}

/// @brief Callback from lwIP on connection error
/// @param arg Pointer to TCP server state
/// @param err lwIP error code
static void tcp_server_err(void *arg, err_t err) {
    if (err != ERR_ABRT) {
        DEBUG_printf("tcp_client_err_fn %d\n", err);
        tcp_server_result(arg, err);
    }
}

/// @brief Callback from lwIP when a client connects
/// @param arg Pointer to TCP server state
/// @param client_pcb Pointer to client TCP protocol control block
/// @param err lwIP error code
/// @return lwIP error code
static err_t tcp_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    if (err != ERR_OK || client_pcb == NULL) {
        DEBUG_printf("Failure in accept\n");
        tcp_server_result(arg, err);
        return ERR_VAL;
    }
    DEBUG_printf("Client connected\n");

    state->client_pcb = client_pcb;
    tcp_arg(client_pcb, state);
    tcp_sent(client_pcb, tcp_server_sent);
    tcp_recv(client_pcb, tcp_server_recv);
    tcp_poll(client_pcb, tcp_server_poll, POLL_TIME_S * 2);
    tcp_err(client_pcb, tcp_server_err);

    return tcp_server_send_data(arg, state->client_pcb);
}

/// @brief Open the TCP server and start listening for connections
/// @param arg Pointer to TCP server state
/// @return true on success, false on failure
static err_t tcp_server_open(void *arg) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    DEBUG_printf("Starting server at %s on port %u\n", ip4addr_ntoa(netif_ip4_addr(netif_list)), TCP_PORT);

    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!pcb) {
        DEBUG_printf("failed to create pcb\n");
        return -1;
    }

    err_t err = tcp_bind(pcb, NULL, TCP_PORT);
    if (err) {
        DEBUG_printf("failed to bind to port %u\n", TCP_PORT);
        return -1;
    }

    state->server_pcb = tcp_listen_with_backlog(pcb, 1);
    if (!state->server_pcb) {
        DEBUG_printf("failed to listen\n");
        if (pcb) {
            tcp_close(pcb);
        }
        return false;
    }

    tcp_arg(state->server_pcb, state);
    tcp_accept(state->server_pcb, tcp_server_accept);

    return 0;
}

/// @brief Run the TCP server test
/// @param arg Pointer to TCP server state
void run_tcp_server_test(void) {
    TCP_SERVER_T *state = tcp_server_init();
    if (!state) {
        return;
    }
    if (!tcp_server_open(state)) {
        tcp_server_result(state, -1);
        return;
    }
    while(!state->complete) {
        // the following #ifdef is only here so this same example can be used in multiple modes;
        // you do not need it in your code
#if PICO_CYW43_ARCH_POLL
        // if you are using pico_cyw43_arch_poll, then you must poll periodically from your
        // main loop (not from a timer) to check for Wi-Fi driver or lwIP work that needs to be done.
        cyw43_arch_poll();
        // you can poll as often as you like, however if you have nothing else to do you can
        // choose to sleep until either a specified time, or cyw43_arch_poll() has work to do:
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(1000));
#else
        // if you are not using pico_cyw43_arch_poll, then WiFI driver and lwIP work
        // is done via interrupt in the background. This sleep is just an example of some (blocking)
        // work you might be doing.
        sleep_ms(1000);
#endif
    }
    free(state);
}























/// Démarrer et initialiser le serveur TCP
TCP_SERVER_T* tcp_server_start(void) {
    TCP_SERVER_T *state = tcp_server_init();
    if (!state) return NULL;

    if (tcp_server_open(state) != 0) {
        tcp_server_close(state);
        return NULL;
    }

    DEBUG_printf("TCP server started.\n");
    return state;
}

/// Arrêter le serveur TCP et libérer les ressources
void tcp_server_stop(TCP_SERVER_T *state) {
    if (!state) return;
    tcp_server_close(state);
    free(state);
    DEBUG_printf("TCP server stopped.\n");
}

err_t tcp_server_send(TCP_SERVER_T *state, const uint8_t *data, size_t len) {
    if (!state || !state->client_pcb || len == 0) return ERR_VAL;

    if (len > BUF_SIZE) len = BUF_SIZE;
    memcpy(state->buffer_sent, data, len);
    state->sent_len = 0;

    cyw43_arch_lwip_check();  // sécurité: vérifie le contexte lwIP

    err_t err = tcp_write(state->client_pcb, state->buffer_sent, len, TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK) {
        DEBUG_printf("Failed to send data %d\n", err);
        return err;
    }

    err = tcp_output(state->client_pcb);  // <-- envoi immédiat du buffer
    if (err != ERR_OK) {
        DEBUG_printf("tcp_output failed %d\n", err);
        return err;
    }

    return ERR_OK;
}


/// Recevoir des données du client connecté
/// retourne le nombre d'octets reçus, 0 si rien n'est disponible
size_t tcp_server_receive(TCP_SERVER_T *state, uint8_t *buffer, size_t max_len) {
    if (!state || !buffer || state->recv_len == 0) return 0;

    size_t to_copy = (state->recv_len > max_len) ? max_len : state->recv_len;
    memcpy(buffer, state->buffer_recv, to_copy);

    // Décaler le buffer interne si nécessaire
    if (to_copy < state->recv_len) {
        memmove(state->buffer_recv, state->buffer_recv + to_copy, state->recv_len - to_copy);
    }
    state->recv_len -= to_copy;

    return to_copy;
}
