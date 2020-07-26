#pragma once

#include "config.h"

int reconnect_mqtt(const CONFIG* cfg);
void disconnect_mqtt();

void publish_str(const char* topic, const char* payload);
void publish_int(const char* topic, int payload);
void publish_double(const char* topic, double payload, const char* format);

#define publish_mqtt(_1, ...) _Generic((FIRST(__VA_ARGS__)), \
                              char*:         publish_str,   \
			                  unsigned char: publish_int,   \
			                  int:           publish_int,   \
			                  double:        publish_double \
			                )(_1, __VA_ARGS__)
#define FIRST(A, ...) A
