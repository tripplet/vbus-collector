#include "config.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define _SVID_SOURCE
#include <string.h>

#include "cJSON/cJSON.h"


int getParameter(cJSON* json, CONFIG* cfg);
int getMqttParameter(const cJSON* mqtt, CONFIG* cfg);
int getHomeassistantParameter(const cJSON* hass, CONFIG* cfg);

int parseConfig(const char* file, CONFIG* cfg)
{
    int status = 0;
    FILE *fp = fopen(file, "rt");
    if (fp == NULL)
    {
        printf("Error opening config file: %s\n", file);
        return 8;
    }

    fseek (fp, 0, SEEK_END);
    int fileLength = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *config = malloc(fileLength + 1);

    fread(config, 1, fileLength, fp);
    config[fileLength] = '\0';
    fclose(fp);

    cJSON *json = cJSON_Parse(config);

    if (json == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }

        status = 9;
        goto end;
    }

    status = getParameter(json, cfg);
    if (status != 0) {
        goto end;
    }

    const cJSON* mqtt = cJSON_GetObjectItem(json, "mqtt");
    if (mqtt == NULL || !cJSON_IsObject(mqtt)) {

        printf("Invalid value for mqtt\n");
        status = 11;
        goto end;
    }

    status = getMqttParameter(mqtt, cfg);
    if (status != 0) {
        goto end;
    }

    const cJSON* hass = cJSON_GetObjectItem(json, "homeassistant");
    if (hass == NULL || !cJSON_IsObject(hass)) {

        printf("Invalid value for homeassistant\n");
        status = 11;
        goto end;
    }

    status = getHomeassistantParameter(hass, cfg);

    printf(cfg->homeassistant_entity_id);fflush(stdout);

end:
    cJSON_Delete(json);
    free(config);

    return status;
}

int getParameter(cJSON* json, CONFIG* cfg)
{
    cJSON *value;

    // Serial port
    value = cJSON_GetObjectItem(json, "device");
    if (value == NULL || !cJSON_IsString(value))
    {
        printf("Invalid value for device\n");
        return 10;
    }

    cfg->serial_port = strdup(value->valuestring);

    // Database path
    value = cJSON_GetObjectItem(json, "database");
    if (value == NULL || !cJSON_IsString(value))
    {
        printf("Invalid value for database\n");
        return 10;
    }

    cfg->database = strdup(value->valuestring);

    // Interval
    value = cJSON_GetObjectItem(json, "interval");
    if (value == NULL || !cJSON_IsNumber(value))
    {
        printf("Invalid value for interval\n");
        return 10;
    }

    cfg->delay = value->valueint;

    // Verbose mode
    value = cJSON_GetObjectItem(json, "verbose");
    if (value == NULL || !cJSON_IsBool(value))
    {
        printf("Invalid value for verbose\n");
        return 10;
    }

    cfg->verbose = value->valueint != 0;

    // Print to stdout
    value = cJSON_GetObjectItem(json, "print_stdout");
    if (value == NULL || !cJSON_IsBool(value))
    {
        printf("Invalid value for print_stdout\n");
        return 10;
    }

    cfg->print_result = value->valueint != 0;

    return 0;
}

int getMqttParameter(const cJSON* mqtt, CONFIG* cfg)
{
    cJSON *value;

    // Enabled
    value = cJSON_GetObjectItem(mqtt, "enabled");
    if (value == NULL || !cJSON_IsBool(value))
    {
        printf("Invalid value for mqtt.enabled\n");
        return 10;
    }

    cfg->mqtt_enabled = value->valueint != 0;
    if (cfg->mqtt_enabled == 0) {
        return 0;
    }

    // MQTT server
    value = cJSON_GetObjectItem(mqtt, "server");
    if (value == NULL || !cJSON_IsString(value))
    {
        printf("Invalid value for mqtt.server\n");
        return 10;
    }

    cfg->mqtt_server = strdup(value->valuestring);

    // MQTT client_id
    value = cJSON_GetObjectItem(mqtt, "client_id");
    if (value == NULL || !cJSON_IsString(value))
    {
        printf("Invalid value for mqtt.client_id\n");
        return 10;
    }

    cfg->mqtt_client_id = strdup(value->valuestring);

    // MQTT user
    value = cJSON_GetObjectItem(mqtt, "user");
    if (value != NULL && value->valuestring != NULL) {
        if (!cJSON_IsString(value))
        {
            printf("Invalid value for mqtt.user\n");
            return 10;
        }

        cfg->mqtt_user = strdup(value->valuestring);
    }

    // MQTT password
    value = cJSON_GetObjectItem(mqtt, "password");
    if (value != NULL && value->valuestring != NULL) {
        if (!cJSON_IsString(value))
        {
            printf("Invalid value for mqtt.password\n");
            return 10;
        }

        cfg->mqtt_password = strdup(value->valuestring);
    }

    // MQTT base_topic
    value = cJSON_GetObjectItem(mqtt, "base_topic");
    if (value == NULL || !cJSON_IsString(value))
    {
        printf("Invalid value for mqtt.base_topic\n");
        return 10;
    }

    cfg->mqtt_base_topic = strdup(value->valuestring);

    return 0;
}

int getHomeassistantParameter(const cJSON* hass, CONFIG* cfg)
{
    cJSON *value;

    // Enabled
    value = cJSON_GetObjectItem(hass, "enabled");
    if (value == NULL || !cJSON_IsBool(value))
    {
        printf("Invalid value for homeassistant.enabled\n");
        return 10;
    }

    cfg->homeassistant_enabled = value->valueint != 0;
    if (!cfg->homeassistant_enabled) {
        return 0;
    }

    // Entity ID
    value = cJSON_GetObjectItem(hass, "entity_id");
    if (value == NULL || !cJSON_IsString(value))
    {
        printf("Invalid value for homeassistant.entity_id\n");
        return 10;
    }

    cfg->homeassistant_entity_id = strdup(value->valuestring);

    return 0;
}