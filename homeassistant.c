#include "homeassistant.h"

#include "cJSON/cJSON.h"

#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

#define AUTHORIZATION_HEADER "Authorization: Bearer %s"
//#define HOMEASSISTANT_API_URL "http://supervisor/core/api/states/%s"
#define HOMEASSISTANT_API_URL "http://172.20.28.201:8080/core/api/states/%s"


CURL* curl = NULL;
struct curl_slist *headers = NULL;

void publish_json(const char* sensor, CONFIG* cfg, cJSON* json);

int homeassistant_init(CONFIG* cfg)
{
    curl = curl_easy_init();

    const char* token = getenv("SUPERVISOR_TOKEN");
    if (token == NULL) {
        return 0;
    }

    int bearer_length = strlen(AUTHORIZATION_HEADER) + strlen(token) + 1;
    char *bearer = malloc(bearer_length);
    snprintf(bearer, bearer_length, AUTHORIZATION_HEADER, token);

    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, bearer);
    free(bearer);

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    if (cfg->verbose)
    {
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    }

    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, -1L);

    return 1;
}

void homeassistant_cleanup()
{
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}

void publish_homeassistant(CONFIG* cfg, Data_Packet* data)
{
    if (curl == NULL)
    {
        return;
    }

    cJSON* root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "state", data->bsPlusPkt.TempSensor1 * 0.1);
    cJSON* attributes = cJSON_AddObjectToObject(root, "attributes");
    cJSON_AddStringToObject(attributes, "unit_of_measurement", "°C");
    cJSON_AddStringToObject(attributes, "device_class", "temperature");
    publish_json("_furnace", cfg, root);
    cJSON_Delete(root);

    root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "state", data->bsPlusPkt.PumpSpeed1 / 2.55);
    attributes = cJSON_AddObjectToObject(root, "attributes");
    cJSON_AddStringToObject(attributes, "unit_of_measurement", "%");
    publish_json("_furnace_pump", cfg, root);
    cJSON_Delete(root);

    root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "state", data->bsPlusPkt.TempSensor4 * 0.1);
    attributes = cJSON_AddObjectToObject(root, "attributes");
    cJSON_AddStringToObject(attributes, "unit_of_measurement", "°C");
    cJSON_AddStringToObject(attributes, "device_class", "temperature");
    cJSON_AddNumberToObject(attributes, "valve", data->bsPlusPkt.PumpSpeed2 / 100);
    publish_json("_returnflow", cfg, root);
    cJSON_Delete(root);

    root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "state", data->bsPlusPkt.PumpSpeed2 / 100);
    publish_json("_returnflow_valve", cfg, root);
    cJSON_Delete(root);

    root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "state", data->bsPlusPkt.TempSensor3 * 0.1);
    attributes = cJSON_AddObjectToObject(root, "attributes");
    cJSON_AddStringToObject(attributes, "unit_of_measurement", "°C");
    cJSON_AddStringToObject(attributes, "device_class", "temperature");
    publish_json("_tank_top", cfg, root);
    cJSON_Delete(root);

    root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "state", data->bsPlusPkt.TempSensor2 * 0.1);
    attributes = cJSON_AddObjectToObject(root, "attributes");
    cJSON_AddStringToObject(attributes, "unit_of_measurement", "°C");
    cJSON_AddStringToObject(attributes, "device_class", "temperature");
    publish_json("_tank_bottom", cfg, root);
    cJSON_Delete(root);
}

void publish_json(const char* sensor, CONFIG* cfg, cJSON* json)
{
    int url_length = strlen(HOMEASSISTANT_API_URL) + strlen(cfg->homeassistant_entity_id_base) + strlen(sensor) + 1;
    char *url = malloc(url_length);

    // Set url
    snprintf(url, url_length, HOMEASSISTANT_API_URL"%s", cfg->homeassistant_entity_id_base, sensor);
    curl_easy_setopt(curl, CURLOPT_URL, url);

    // Set json payload
    char* jsonString = cJSON_PrintUnformatted(json);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonString);

    // Send HTTP POST request
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        char errbuf[CURL_ERROR_SIZE] = { 0, };
        size_t len = strlen(errbuf);

        fprintf(stderr, "\nlibcurl: (%d) ", res);
        if (len)
        {
            fprintf(stderr, "%s%s", errbuf, ((errbuf[len - 1] != '\n') ? "\n" : ""));
        }

        fprintf(stderr, "%s\n\n", curl_easy_strerror(res));
    }

    free(url);
    free(jsonString);
}