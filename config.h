#ifndef _CONFIG_H
#define _CONFIG_H

#include <uv.h>
#include "toml.h"

struct config *parse_config(char *);

struct config {
    char *serial_port;
    int64_t   tcp_port;
    struct sockaddr_in priority_client;
    char *control_cmd_fmt;
};

#endif
