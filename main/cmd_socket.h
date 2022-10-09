#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define SOCKET_DEFAULT_SPORT 8080
#define SOCKET_DEFAULT_DPORT 10000
#define SOCKET_TYPE_TCP (1 << 0)
#define SOCKET_TYPE_UDP (1 << 1)

typedef struct {
    struct arg_str *ip;
    struct arg_int *port;
    struct arg_int *length;
    struct arg_int *interval;
    struct arg_int *cnt;
    struct arg_int *socket;    
    struct arg_str *type;
    struct arg_lit *bind;
    struct arg_lit *send;
    struct arg_end *end;
} wifi_socket_t;

typedef struct {
    uint32_t flag;
    int32_t socket_id;
    uint32_t des_ip;
    uint32_t src_ip;
    uint16_t dport;
    uint16_t sport;
    uint32_t interval;
    uint32_t length;
    uint8_t * buffer;
    uint32_t cnt;
} socket_cfg_t;

int wifi_cmd_socket(int argc, char **argv);

#ifdef __cplusplus
}
#endif