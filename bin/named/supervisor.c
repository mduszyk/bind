#include <errno.h>
#include <zmq.h>

#include <named/supervisor.h>

int supervisor_init(supervisor_t *sv, const char *zmq_addr_rpc) {
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
    if (zmq_connect(sv->zmq_sock_rpc, zmq_addr_rpc) == -1) {
        supervisor_log(ISC_LOG_ERROR, "error connecting zmq socket: %s, msg: %s",
            zmq_addr_rpc, strerror(errno));
        return -1;
    }
    
    supervisor_log(ISC_LOG_INFO, "initialized supervisor context");
    
    return 0;
}

int supervisor_destroy(supervisor_t *sv) {
    zmq_close(sv->zmq_sock_rpc);
    zmq_ctx_destroy(sv->zmq_ctx);
    supervisor_log(ISC_LOG_DEBUG(1), "supervisor destroyed");
    
    return 0;
}

int supervisor_call(supervisor_t *sv, supervisor_query_t *query) {
    size_t domain_len = strlen(query->domain);
    size_t peer_len = strlen(query->peer);
    
    zmq_msg_t request;
    zmq_msg_init_size(&request, domain_len + peer_len + 1);
    memcpy(zmq_msg_data(&request), query->domain, domain_len + 1);
    memcpy(zmq_msg_data(&request) + domain_len + 1, query->peer, peer_len);
    zmq_msg_send(&request, sv->zmq_sock_rpc, 0);
    zmq_msg_close(&request);

    zmq_msg_t reply;
    zmq_msg_init(&reply);
    zmq_msg_recv(&reply, sv->zmq_sock_rpc, 0);
    //zmq_msg_data(&reply);
    zmq_msg_close(&reply);

    return 0;
}
