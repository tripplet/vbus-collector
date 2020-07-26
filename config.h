#pragma once

#include <stdbool.h>

typedef struct {
    const char* serial_port;

    int delay;

    const char* database;
    bool withSql;

    bool print_result;
    bool verbose;

    bool mqtt_enabled;
    const char* mqtt_user;
    const char* mqtt_password;
    const char* mqtt_server;
    const char* mqtt_base_topic;
    const char* mqtt_client_id;

    bool homeassistant_enabled;
    const char* homeassistant_entity_id;
} CONFIG;

int parseConfig(const char* file, CONFIG* cfg);