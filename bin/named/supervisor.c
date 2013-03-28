#include <errno.h>
#include <zmq.h>
#include <netinet/in.h>
#include <stdlib.h>

#include <named/supervisor.h>

static int daoc_true = 1;

int supervisor_init(supervisor_t **supervisor, const char *zmq_addr_rpc) {    
    supervisor_t *sv = (supervisor_t*) malloc(sizeof(supervisor_t));
    
    sv->zmq_ctx = zmq_ctx_new();
    if (sv->zmq_ctx == NULL) {
        supervisor_log(ISC_LOG_ERROR, "error creating zmq context");
        return -1;
    }
    sv->zmq_addr_rpc = zmq_addr_rpc;
    sv->zmq_sock_rpc = zmq_socket(sv->zmq_ctx, ZMQ_REQ);
    if (sv->zmq_sock_rpc == NULL) {
        supervisor_log(ISC_LOG_ERROR, "error creating zmq socket: %s", zmq_addr_rpc);
        return -1;
    }
    
    if (zmq_setsockopt (sv->zmq_sock_rpc, ZMQ_DELAY_ATTACH_ON_CONNECT,
            &daoc_true, sizeof(int)) == -1) {
        supervisor_log(ISC_LOG_ERROR, "unable to set zmq socket recv timeout");
        return -1;
    }
    
    if (zmq_connect(sv->zmq_sock_rpc, zmq_addr_rpc) == -1) {
        supervisor_log(ISC_LOG_ERROR, "error connecting zmq socket: %s, msg: %s",
            zmq_addr_rpc, strerror(errno));
        return -1;
    }
    
    *supervisor = sv;
    
    supervisor_log(ISC_LOG_INFO, "created zmq client socket: %s", zmq_addr_rpc);
    
    return 0;
}

int supervisor_destroy(supervisor_t *sv) {
    zmq_close(sv->zmq_sock_rpc);
    zmq_ctx_destroy(sv->zmq_ctx);
    free(sv);
    supervisor_log(ISC_LOG_DEBUG(1), "supervisor destroyed");

    return 0;
}

/**
 *   msb  .....  lsb
 * 0b a b c 0 0 0 0 0
 *  | | | | | | | |
 *  | | | | | | | \- zero
 *  | | | | | | \--- zero
 *  | | | | | \----- zero
 *  | | | | \------- zero
 *  | | | \--------- zero
 *  | | \----------- if 0 then bind_ip is ipv4; if 1 then ipv6
 *  | \------------- if 1 then bind_ip present in message
 *  \--------------- if 0 then client_ip is ipv4; if 1 then ipv6
 * 
 * next 4 bytes (or 16 if "a" is 1) are ip of client
 * 
 * if "b" is 1 then
 *   if "c" is 0 then
 *     next 4 bytes are ip of bind server
 *   if "c" is 1 then
 *     next 16 bytes are ip of bind server
 * 
 * next bytes till the end are the domain itself
 */
int supervisor_call(supervisor_t *sv, supervisor_query_t *query) {
    int len, n = 0;
    zmq_msg_t request;
    zmq_msg_t response;
    size_t domain_len = strlen(query->domain);
    size_t peer_len = (query->flags & SUPERVISOR_PEER_IPV6) ? 16 : 4;
    size_t dest_len = 0;

    if (SUPERVISOR_DEST == (query->flags & SUPERVISOR_DEST))
        dest_len = (query->flags & SUPERVISOR_DEST_IPV6) ? 16 : 4;
    
    if (zmq_msg_init_size(&request, 1 + peer_len + dest_len + domain_len) == -1) {
        supervisor_log(ISC_LOG_ERROR, "error initializing request message: %s", strerror(errno));
        return -1;
    }
    
    memcpy(zmq_msg_data(&request), &query->flags, 1);
    n++;
    
    memcpy(zmq_msg_data(&request) + n, query->peer_ip, peer_len);
    n += peer_len;
    
    if (dest_len > 0) {
        memcpy(zmq_msg_data(&request) + n, query->dest_ip, dest_len);
        n += dest_len;
    }
    
    memcpy(zmq_msg_data(&request) + n, query->domain, domain_len);
    
    if (zmq_msg_send(&request, sv->zmq_sock_rpc, ZMQ_DONTWAIT) == -1) {
        supervisor_log(ISC_LOG_ERROR, "error sending message: %s", strerror(errno));
        return -1;
    }
    
    if (zmq_msg_close(&request) == -1) {
        supervisor_log(ISC_LOG_ERROR, "error closing request message: %s", strerror(errno));
        return -1;
    }
    
    if (zmq_msg_init(&response) == -1) {
        supervisor_log(ISC_LOG_ERROR, "error initializing response message: %s", strerror(errno));
        return -1;
    }
    
    if ((len = zmq_msg_recv(&response, sv->zmq_sock_rpc, 0)) == -1) {
        supervisor_log(ISC_LOG_ERROR, "error receiving message: %s", strerror(errno));
        return -1;
    }

    if (len > 4) {
        query->rsp_len = len - 4;
        memcpy(&query->rsp_ttl, zmq_msg_data(&response), 4);
        query->rsp_ttl = ntohl((uint32_t) query->rsp_ttl);
        memcpy(query->rsp, zmq_msg_data(&response) + 4, query->rsp_len);
    }
    
    if (zmq_msg_close(&response) == -1) {
        supervisor_log(ISC_LOG_ERROR, "error closing response message: %s", strerror(errno));
        return -1;
    }
    
    return 0;
}
