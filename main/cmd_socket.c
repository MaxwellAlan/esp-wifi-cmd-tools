#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include <sys/socket.h>
#include "argtable3/argtable3.h"
#include "freertos/FreeRTOS.h"
#include "esp_netif.h"
#include "cmd_socket.h"

static socket_cfg_t socket_cfgs;
wifi_socket_t socket_args;
static bool s_socket_is_running = false;
static const char *TAG = "cmd_socket";

static void socket_task(void *arg)
{
    socket_cfg_t *cfg = (socket_cfg_t *)arg;
    struct sockaddr_in sock_daddr;
    sock_daddr.sin_family = AF_INET;
    sock_daddr.sin_addr.s_addr = cfg->des_ip;
    sock_daddr.sin_port = htons(cfg->dport);
    cfg->buffer = (uint8_t *)malloc( cfg->length );
    if (! cfg->buffer ) {
        ESP_LOGE(TAG, "No Memory");
    }
    memset(cfg->buffer, 0, cfg->length);
    for(int i = 0; i < cfg->cnt; i++){
        sendto(cfg->socket_id, cfg->buffer, cfg->length, 0, (struct sockaddr *)&sock_daddr, sizeof(struct sockaddr_in));
        vTaskDelay(cfg->interval / portTICK_PERIOD_MS);
    }
    ESP_LOGI(TAG, "Socket Task Done");
    free(cfg->buffer);
    s_socket_is_running = false;
    vTaskDelete(NULL);
}

static esp_err_t socket_start(socket_cfg_t *cfg)
{
    BaseType_t ret;

    if (!cfg) {
        return ESP_FAIL;
    }

    if (s_socket_is_running) {
        ESP_LOGW(TAG, "socket is running");
        return ESP_FAIL;
    }

    s_socket_is_running = true;
    
    ret = xTaskCreate(socket_task, "socTsk", 4096, cfg, 10, NULL);

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "create task %s failed", "socTsk");
        return ESP_FAIL;
    }
    return ESP_OK;
}

int wifi_cmd_socket(int argc, char **argv)
{
    int opt = 1;
    int nerrors = arg_parse(argc, argv, (void **) &socket_args);
    struct sockaddr_in sock_saddr;

    if (nerrors != 0) {
        arg_print_errors(stderr, socket_args.end, argv[0]);
        return 0;
    }

    // socket bind command
    // soc -B -t UDP
    if ( socket_args.bind->count ) {
        if ( socket_args.port->count == 0 ) {
            socket_cfgs.sport = SOCKET_DEFAULT_SPORT;
        } else {
            socket_cfgs.sport = socket_args.port->ival[0];
        }
        if ( socket_args.type->count == 0 ) {
            socket_cfgs.flag |= SOCKET_TYPE_UDP;
        } else {
            if ( memcmp(socket_args.type->sval[0], "UDP", 3) == 0 ) {
                socket_cfgs.flag |= SOCKET_TYPE_UDP;
            } else {
                socket_cfgs.flag |= SOCKET_TYPE_TCP;
            }
        }
        if ( (socket_cfgs.flag & SOCKET_TYPE_UDP) == SOCKET_TYPE_UDP ) {
            socket_cfgs.socket_id = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        } else {
            socket_cfgs.socket_id = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        }
        setsockopt(socket_cfgs.socket_id, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sock_saddr.sin_family = AF_INET;
        sock_saddr.sin_addr.s_addr = 0;
        sock_saddr.sin_port = htons(socket_cfgs.sport);
        if ( bind(socket_cfgs.socket_id, (struct sockaddr *)&sock_saddr, sizeof(sock_saddr)) != 0 ) {
            ESP_LOGE(TAG,"Socket bind fail!!");
            closesocket(socket_cfgs.socket_id);
        } else {
            ESP_LOGI(TAG,"Socket = %d bind success",socket_cfgs.socket_id);
        }
    }
    // socket send command
    // soc -S -i 192.168.1.1 -p 10000 -l 1000 -n 2000000 -j 500
    if ( socket_args.send->count ) {
        // parse ip
        if (socket_args.ip->count) {
            socket_cfgs.des_ip = esp_ip4addr_aton(socket_args.ip->sval[0]);
        } else {
            ESP_LOGE(TAG,"No Des IP addr!!");
        }
        // parse des port
        if ( socket_args.port->count ) {
            socket_cfgs.dport = socket_args.port->ival[0];
        } else {
            socket_cfgs.dport = SOCKET_DEFAULT_DPORT;
        }
        // parse pkt length
        if (socket_args.length->count) {
            socket_cfgs.length = socket_args.length->ival[0];
        } else {
            socket_cfgs.length = 1000;
        }
        // parse tx interval
        if (socket_args.interval->count) {
            socket_cfgs.interval = socket_args.interval->ival[0];
        } else {
            socket_cfgs.interval = 1000; // ms
        }
        // parse pkt counts
        if (socket_args.cnt->count) {
            socket_cfgs.cnt = socket_args.cnt->ival[0];
        } else {
            socket_cfgs.cnt = 1;
        }
        socket_start(&socket_cfgs);
    }
    return ESP_OK;
}