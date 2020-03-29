#include <stdio.h>
#include <uv.h>

#include "config.h"
#include "toml.h"

struct config config;

struct config *handle_error(char *str, char *errstr) {
    fprintf(stderr, "%s%s\n", str, errstr);
    return NULL;
}

struct config *parse_config(char *filename) {
    FILE* fp;
    char errbuf[200];
    toml_table_t* conf;
    toml_table_t* server;
    const char* raw;
    char *priority_client;
    int64_t tmp_int;

    if (NULL == (fp = fopen(filename, "r")))
        return handle_error("Unable to open config file: ", strerror(errno));

    conf = toml_parse_file(fp, errbuf, sizeof(errbuf));
    fclose(fp);

    if (NULL == conf)
        return handle_error("Invalid config file: ", errbuf);

    if (NULL == (server = toml_table_in(conf, "bridge")))
        return handle_error("Invalid config file: ", errbuf);

    if (NULL == (raw = toml_raw_in(server, "serial-port")))
        return handle_error("Invalid serial port: ", errbuf);
    if (toml_rtos(raw, &config.serial_port))
        return handle_error("Invalid serial port: ", errbuf);
    
    if (NULL == (raw = toml_raw_in(server, "baud-rate")))
        return handle_error("Invalid baud rate: ", errbuf);
    if (toml_rtoi(raw, &tmp_int))
        return handle_error("Invalid baud rate: ", errbuf);
    config.baud_rate = (int)tmp_int;

    if (NULL == (raw = toml_raw_in(server, "tcp-port")))
        return handle_error("Invalid TCP port: ", errbuf);
    if (toml_rtoi(raw, &tmp_int))
        return handle_error("Invalid TCP port: ", errbuf);
    config.tcp_port = (int)tmp_int;

    uv_ip4_addr("0.0.0.0", 0, &config.priority_client);
    if ((raw = toml_raw_in(server, "priority-client"))) {
        if (toml_rtos(raw, &priority_client))
            return handle_error("Invalid priority client: ", errbuf);
        uv_ip4_addr(priority_client, 0, &config.priority_client);
    }

    config.control_cmd_fmt = NULL;
    if ((raw = toml_raw_in(server, "control-command-format"))) {
        if (toml_rtos(raw, &config.control_cmd_fmt))
            return handle_error("Invalid control command format: ", errbuf);
    }

    config.debug_level = 0;
    if (raw = toml_raw_in(server, "debug-level")) {
        if (0 == toml_rtoi(raw, &tmp_int))
            config.debug_level = (int)tmp_int;
    }

    toml_free(conf);

    return &config;
}