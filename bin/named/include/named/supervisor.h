#include <named/log.h>
#include <dns/types.h>

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
    dns_ttl_t rsp_ttl;
} supervisor_query_t;

int supervisor_init(supervisor_t *sv, const char *zmq_addr_rpc);

int supervisor_destroy(supervisor_t *sv);

int supervisor_call(supervisor_t *sv, supervisor_query_t *query);

#define supervisor_log(log_level, format, ...) isc_log_write(ns_g_lctx, NS_LOGCATEGORY_SUPERVISOR, \
    NS_LOGMODULE_SUPERVISOR, log_level, format, ##__VA_ARGS__)

#endif /* NAMED_SUPERVISOR_H */
