#pragma once

#include <stdbool.h>

typedef struct {
    char* serial_port;

    unsigned long delay;

    char* database;
    bool withSql;

    bool print_result;
    bool verbose;

    bool mqtt_enabled;
    char* mqtt_user;
    char* mqtt_password;
    char* mqtt_server;
    char* mqtt_base_topic;
    char* mqtt_client_id;
} CONFIG;

int parseConfig(const char* file, CONFIG* cfg);