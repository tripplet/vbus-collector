//****************************************************************************
// main.c
//
// (c) Hewell Technology Ltd. 2014
//
// Tobias Tangemann 2020
//****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include "config.h"
#include "datatypes.h"
#include "kbhit.h"
#include "serial.h"
#include "vbus.h"
#include "mqtt.h"
#include "homeassistant.h"

#ifdef __SQLITE__
    #include "sqlite.h"
#endif

#if !defined(GIT_VERSION)
    #define GIT_VERSION "unknown"
#endif

char serial_buffer[256];

int main(int argc, char *argv[])
{
    printf("vbus-collector "GIT_VERSION"\n");

    Data_Packet packet;
    PVBUS_V1_CMD pPacket = (PVBUS_V1_CMD)&serial_buffer[0];
    unsigned char i = 0;
    int headerSync = 0;
    int loopforever = 0;
    int packet_displayed = 0;

    CONFIG cfg = {
        .serial_port = NULL,
        .delay = 0,

        .database = NULL,
        .withSql = false,

        .print_result = true,
        .verbose = false,

        .mqtt_enabled = false,
        .mqtt_user = NULL,
        .mqtt_password = NULL,
        .mqtt_server = NULL,
        .mqtt_base_topic = NULL,
        .mqtt_client_id = NULL,

        .homeassistant_enabled = false,
        .homeassistant_entity_id_base = NULL
    };

    if (argc > 2)
    {
        for (int idx = 1; idx < argc; ++idx)
        {
            char *option = argv[idx];

            if (strcmp("-f", option)==0 || strcmp("--forever", option)==0)
            {
                loopforever = true;
            }

            if (strcmp("-v", option) == 0 || strcmp("--verbose", option) == 0)
            {
                cfg.verbose = true;
            }

            if (strcmp("-m", option) == 0 || strcmp("--mqtt", option) == 0)
            {
                cfg.mqtt_enabled = true;
            }

            if (strcmp("-d", option)==0 || strcmp("--delay", option)==0)
            {
                if (argc <= idx + 2) {
                    printf("Missing value for delay option\n");
                    return 4;
                }

                // Use next option as delay value
                idx++;
                cfg.delay = strtoul(argv[idx], NULL, 10);
            }

            #ifdef __SQLITE__
                if (strcmp("--db", option)==0)
                {
                    if (argc <= idx + 2)
                    {
                        printf("Missing value for sqlite db path\n");
                        return 5;
                    }

                    // Use next option as path to sqlite database
                    idx++;
                    cfg.database = argv[idx];
                }
            #endif

            if (strcmp("--no-print", option) == 0)
            {
                cfg.print_result = false;
            }

            if (strcmp("-c", option) == 0 || strcmp("--config", option) == 0)
            {
                #ifndef __JSON__
                    printf("Config file is not supported");
                    return 7;
                #else
                    if (argc <= idx + 1)
                    {
                        printf("Missing config file\n");
                        return 7;
                    }

                    // Use next option as file name/path
                    idx++;

                    if (parseConfig(argv[idx], &cfg) != 0)
                    {
                        printf("Error parsing config file\n");
                        return 7;
                    }

                #endif
            }
        }
    }

    start:
    headerSync = 0; packet_displayed = 0;

    // last option is the serial port if no config file is used
    if (cfg.serial_port == NULL)
    {
        if (argc > 2)
        {
            cfg.serial_port = argv[argc - 1];
        }
        else
        {
            printf("No serial port set");
            return 2;
        }
    }

    if (!serial_open_port(cfg.serial_port))
    {
        printf("Errno(%d) opening serial port %s: %s\n", errno, cfg.serial_port, strerror(errno));
        return 2;
    }

    if (cfg.database != NULL)
    {
        if (cfg.verbose)
        {
            printf("Opening database %s\n", cfg.database);
        }

        if (!sqlite_open(cfg.database))
        {
            return 6;
        }

        sqlite_create_table();
        cfg.withSql = true;
    }

    if (cfg.homeassistant_enabled)
    {
        if (!homeassistant_init(&cfg))
        {
            printf("Error initializing homeassistant");
            return 20;
        }
    }

    if (cfg.verbose)
    {
        printf("Setting baudrate...\n");
    }

    if (!serial_set_baud_rate(9600))
    {
        printf("Failed to set baud rate: %s\n", serial_get_error());
        return 3;
    }

    if (cfg.mqtt_enabled)
    {
        if (cfg.verbose)
        {
            printf("Connecting to mqtt server...\n");
        }

    	reconnect_mqtt(&cfg);
    }


    if (cfg.verbose)
    {
        printf("Collecting data...\n");
    }

    do
    {
        // sleep 50ms
        nanosleep((const struct timespec[]){{.tv_sec = 0, .tv_nsec = 50000000L}}, NULL);
        
        if (caughtSigQuit())
        {
            break;
        }

        int count = serial_read(&(serial_buffer[i]), 1);//sizeof(serial_buffer));
        if (count < 1)
        {
            continue;
        }

        if ((serial_buffer[i] & 0xFF) == 0xAA)
        {
            serial_buffer[0] = serial_buffer[i];
            i=0;
            headerSync = 1;

            if (cfg.verbose)
            {
                printf("\n\n");
            }
        }

        if (cfg.verbose)
        {
            printf("%02x ", serial_buffer[i]);
        }

        i++;
        if (i % 16 == 0 && cfg.verbose)
        {
            printf("\n");
        }

        if (headerSync)
        {
            if (cfg.verbose)
            {
                printf("Header sync\n");
            }

            if (i > sizeof(VBUS_HEADER))
            {
                //we have nearly all the header
                if ((pPacket->h.ver & 0xF0) != 0x10)
                {
                    headerSync = 0;
                    continue;
                }

                if (i < sizeof(VBUS_V1_CMD)) {
                    continue;
                }

                if (i < ((pPacket->frameCnt * sizeof(FRAME_STRUCT)) + sizeof(VBUS_V1_CMD)))
                {
                    continue;
                }

                headerSync = 0;

                //We have a whole packet..
                unsigned char crc = vbus_calc_crc((void*)serial_buffer, 1, 8);

                if (cfg.verbose)
                {
                    printf("\nPacket size: %d. Source: 0x%04x, Destination: 0x%04x, Command: 0x%04x, No of frames: %d, crc: 0x%02x(0x%02x)\n",
                        i, pPacket->h.source, pPacket->h.dest, pPacket->cmd, pPacket->frameCnt, pPacket->crc, crc);
                }

                if (pPacket->crc != crc)
                {
                    if (cfg.verbose)
                    {
                        printf("CRC Error!\n");
                    }

                    continue;
                }

                //Not sure what this packet is
                if (pPacket->cmd != 0x0100 || pPacket->h.dest != 0x10)
                {
                    if (cfg.verbose)
                    {
                        printf("Ignoring unkown packet!\n");
                    }

                    continue;
                }

                //Packet is from DeltaSol BS Plus
                //This is the packet we've been waiting for! Lets decode it....
                int crcOK = 0;
                for (unsigned char j = 0; j < pPacket->frameCnt; j++)
                {
                    crc = vbus_calc_crc((void*)&pPacket->frame[j], 0, 5);

                    if (cfg.verbose)
                    {
                        printf("Bytes: 0x%02x%02x%02x%02x, Septett: 0x%02x, crc: 0x%02x(0x%02x)\n",
                            pPacket->frame[j].bytes[0], pPacket->frame[j].bytes[1], pPacket->frame[j].bytes[2], pPacket->frame[j].bytes[3],
                            pPacket->frame[j].septett, pPacket->frame[j].crc, crc);
                    }

                    crcOK = (pPacket->frame[j].crc == crc);
                    if (!crcOK)
                    {
                        if (cfg.verbose)
                        {
                            printf("Frame CRC Error!\n");
                            crcOK = 0;
                        }

                        break;
                    }

                    vbus_inject_septett((void *)&(pPacket->frame[j]), 0, 4);
                    for (unsigned char k = 0; k < 4; k++)
                    {
                        packet.asBytes[(j * 4) + k] = pPacket->frame[j].bytes[k];
                    }
                }

                if (!crcOK)
                {
                    continue;
                }

                //printf("%d\n", sizeof(BS_Plus_Data_Packet));

                #if __SQLITE__
                    if (cfg.withSql)
                    {
                        sqlite_insert_data(&packet);
                    }
                #endif

                if (cfg.print_result)
                {
                    printf("System time:%02d:%02d"
                        ", Sensor1 temp:%.1fC"
                        ", Sensor2 temp:%.1fC"
                        ", Sensor3 temp:%.1fC"
                        ", Sensor4 temp:%.1fC"
                        ", Pump speed1:%d%%"
                        ", Pump speed2:%d%%"
                        //", RelayMask:%d"
                        //", ErrorMask:%d"
                        //", Scheme:%d, %d, %d, %d, %d, %d, %d"
                        ", Hours1:%d, Hours2:%d"
                        //", %dWH, %dkWH, %dMWH"
                        //", Version:%.2f"
                        "\n",
                        packet.bsPlusPkt.SystemTime / 60,
                        packet.bsPlusPkt.SystemTime % 60,
                        packet.bsPlusPkt.TempSensor1 * 0.1,
                        packet.bsPlusPkt.TempSensor2 * 0.1,
                        packet.bsPlusPkt.TempSensor3 * 0.1,
                        packet.bsPlusPkt.TempSensor4 * 0.1,
                        packet.bsPlusPkt.PumpSpeed1,
                        packet.bsPlusPkt.PumpSpeed2,
                        //packet.bsPlusPkt.RelayMask,
                        //packet.bsPlusPkt.ErrorMask,
                        //packet.bsPlusPkt.Scheme,
                        //packet.bsPlusPkt.OptionCollectorMax,
                        //packet.bsPlusPkt.OptionCollectorMin,
                        //packet.bsPlusPkt.OptionCollectorFrost,
                        //packet.bsPlusPkt.OptionTubeCollector,
                        //packet.bsPlusPkt.OptionRecooling,
                        //packet.bsPlusPkt.OptionHQM,
                        packet.bsPlusPkt.OperatingHoursRelay1,
                        packet.bsPlusPkt.OperatingHoursRelay2
                        //packet.bsPlusPkt.HeatQuantityWH,
                        //packet.bsPlusPkt.HeatQuantityKWH,
                        //packet.bsPlusPkt.HeatQuantityMWH
                        //packet.bsPlusPkt.Version * 0.01
                    );
                }

                if (cfg.mqtt_enabled)
                {
                    publish_mqtt("ofen/temp", packet.bsPlusPkt.TempSensor1 * 0.1, "%.1f");
                    publish_mqtt("ofen/pump", packet.bsPlusPkt.PumpSpeed1);
                    publish_mqtt("ruecklauf/temp", packet.bsPlusPkt.TempSensor4 * 0.1, "%.1f");
                    publish_mqtt("ruecklauf/valve", packet.bsPlusPkt.PumpSpeed2 / 100);
                    publish_mqtt("speicher/oben/temp", packet.bsPlusPkt.TempSensor3 * 0.1, "%.1f");
                    publish_mqtt("speicher/unten/temp", packet.bsPlusPkt.TempSensor2 * 0.1, "%.1f");
                }

                if (cfg.homeassistant_enabled)
                {
                    publish_homeassistant(&cfg, &packet);
                }

                packet_displayed++;

                fflush(stdout);

                continue;
            }
        }

    } while (loopforever == true || packet_displayed == 0);

    serial_close_port();

    #if __SQLITE__
        sqlite_close();
    #endif

    if (cfg.delay > 0)
    {
        if (cfg.delay == 60)
        {
            time_t rawtime;
            struct tm * timeinfo;

            time (&rawtime);
            timeinfo = localtime (&rawtime);

            if (timeinfo->tm_sec < 59)
            {
                sleep(cfg.delay - timeinfo->tm_sec);
            }
            else
            {
                sleep(cfg.delay);
            }
        }
        else
        {
            sleep(cfg.delay);
        }

        goto start;
    }

    if (cfg.homeassistant_enabled)
    {
        homeassistant_cleanup();
    }

    return 0;
}
