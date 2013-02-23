#include <errno.h>
#include <zmq.h>
#include <netinet/in.h>
#include <stdlib.h>

#include <named/supervisor.h>

int supervisor_init(supervisor_t **supervisor, const char *zmq_addr_rpc) {
    int timeout = 1000;
    
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
    if (zmq_setsockopt (sv->zmq_sock_rpc, ZMQ_RCVTIMEO, &timeout, 4) == -1) {
        supervisor_log(ISC_LOG_ERROR, "unable to set zmq socket recv timeout");
        return -1;
    }
    if (zmq_setsockopt (sv->zmq_sock_rpc, ZMQ_SNDTIMEO, &timeout, 4) == -1) {
        supervisor_log(ISC_LOG_ERROR, "unable to set zmq socket send timeout");
        return -1;
    }
    
    if (zmq_connect(sv->zmq_sock_rpc, zmq_addr_rpc) == -1) {
        supervisor_log(ISC_LOG_ERROR, "error connecting zmq socket: %s, msg: %s",
            zmq_addr_rpc, strerror(errno));
        return -1;
    }
    
    *supervisor = sv;
    
    supervisor_log(ISC_LOG_INFO, "connected to broker: %s", zmq_addr_rpc);
    
    return 0;
}

int supervisor_destroy(supervisor_t *sv) {
    zmq_close(sv->zmq_sock_rpc);
    zmq_ctx_destroy(sv->zmq_ctx);
    free(sv);
    supervisor_log(ISC_LOG_DEBUG(1), "supervisor destroyed");

    return 0;
}

int supervisor_call(supervisor_t *sv, supervisor_query_t *query) {
    int len;
    zmq_msg_t request;
    zmq_msg_t response;
    size_t domain_len = strlen(query->domain);
    
    if (zmq_msg_init_size(&request, 1 + query->peer_len + domain_len) == -1) {
        supervisor_log(ISC_LOG_ERROR, "error initializing request message: %s", strerror(errno));
        return -1;
    }
    
    memcpy(zmq_msg_data(&request), &query->peer_len, 1);
    memcpy(zmq_msg_data(&request) + 1, query->peer, query->peer_len);
    memcpy(zmq_msg_data(&request) + 1 + query->peer_len, query->domain, domain_len);
    
    if (zmq_msg_send(&request, sv->zmq_sock_rpc, 0) == -1) {
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
    
    len = zmq_msg_recv(&response, sv->zmq_sock_rpc, 0);
    if (len == -1) {
        supervisor_log(ISC_LOG_ERROR, "error receiving message: %s", strerror(errno));
        return -1;
    }

    if (len > 0) {
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
