#include <zmq.h>

#include <named/supervisor.h>

int supervisor_init(supervisor_t *sv, const char *zmq_addr_rpc) {
    sv->zmq_ctx = zmq_ctx_new();
    sv->zmq_addr_rpc = zmq_addr_rpc;
    
    return 0;
}

int supervisor_destroy(supervisor_t *sv) {
    zmq_ctx_destroy(sv->zmq_ctx);
    
    return 0;
}

int supervisor_call(supervisor_t *sv, supervisor_query_t *query) {
    
    return 0;
}
