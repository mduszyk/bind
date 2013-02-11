#ifndef NAMED_SUPERVISOR_H
#define NAMED_SUPERVISOR_H 1

typedef struct supervisor {
    void *zmq_ctx;
    void *zmq_sock_rpc;
    const char *zmq_addr_rpc;
} supervisor_t;

typedef struct supervisor_query {
    char *domain;
    char *peer;
    char *rsp;
    unsigned int rsp_len;
} supervisor_query_t;

int supervisor_init(supervisor_t *sv, const char *zmq_addr_rpc);

int supervisor_destroy(supervisor_t *sv);

int supervisor_call(supervisor_t *sv, supervisor_query_t *query);

#endif /* NAMED_SUPERVISOR_H */
