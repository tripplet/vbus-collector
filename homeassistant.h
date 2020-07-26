#pragma once

#include "config.h"
#include "datatypes.h"

bool homeassistant_init(CONFIG* cfg);
void homeassistant_cleanup();
void publish_homeassistant(CONFIG* cfg, Data_Packet* data);