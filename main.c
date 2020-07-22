//****************************************************************************
// main.c
//
// (c) Hewell Technology Ltd. 2014
//
// Tobias Tangemann 2017
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

#include "datatypes.h"
#include "kbhit.h"
#include "serial.h"
#include "vbus.h"
#include "mqtt.h"

#ifdef __SQLITE__
    #include "sqlite.h"
#endif

char serial_buffer[256];

int main(int argc, char *argv[])
{
    Data_Packet packet;
    PVBUS_V1_CMD pPacket = (PVBUS_V1_CMD)&serial_buffer[0];
    unsigned char i = 0;
    int headerSync = 0;
    int loopforever = 0;
    int packet_displayed = 0;
    unsigned long delay = 0;
    bool withSql = false;
    bool print_result = true;
    bool verbose = false;
    bool use_mqtt = false;

    start:
    headerSync = 0; packet_displayed = 0;

    // last option is the serial port
    if (argc < 2 || !serial_open_port(argv[argc - 1]))
    {
        printf("Errno(%d) opening %s: %s\n", errno, argv[1], strerror(errno));
        return 2;
    }

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
                verbose = true;
            }

            if (strcmp("-m", option) == 0 || strcmp("--mqtt", option) == 0)
            {
                use_mqtt = true;
            }

            if (strcmp("-d", option)==0 || strcmp("--delay", option)==0)
            {
                if (argc <= idx + 2) {
                    printf("Missing value for delay option\n");
                    return 4;
                }

                // Use next option as delay value
                idx++;
                delay = strtoul(argv[idx], NULL, 10);

                #ifdef __WXMSW__
                    delay *= 1000;
                #endif
            }

            #ifdef __SQLITE__
                if (strcmp("--db", option)==0)
                {
                    if (argc <= idx + 2)
                    {
                        printf("Missing value for sqlite db path\n");
                        return 5;
                    }

                    // Use next option as delay value
                    idx++;
                    if (!sqlite_open(argv[idx]))
                    {
                        return 6;
                    }

                    sqlite_create_table();

                    withSql = true;
                }
            #endif

            if (strcmp("--no-print", option) == 0)
            {
                print_result = false;
            }
        }
    }

    if (verbose)
    {
        printf("Setting baudrate...\n");
    }

    if (!serial_set_baud_rate(9600))
    {
        printf("Failed to set baud rate: %s\n", serial_get_error());
        return 3;
    }

    if (use_mqtt)
    {
        if (verbose)
        {
            printf("Connecting to mqtt server...\n");
        }

    	reconnect_mqtt("heizung/status");
    }


    if (verbose)
    {
        printf("Collecting data...\n");
    }

    do
    {
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

            if (verbose)
            {
                printf("\n\n");
            }
        }

        if (verbose)
        {
            printf("%02x ", serial_buffer[i]);
        }

        i++;
        if (i % 16 == 0 && verbose)
        {
            printf("\n");
        }

        if (headerSync)
        {
            if (verbose)
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

                if (verbose)
                {
                    printf("\nPacket size: %d. Source: 0x%04x, Destination: 0x%04x, Command: 0x%04x, No of frames: %d, crc: 0x%02x(0x%02x)\n",
                        i, pPacket->h.source, pPacket->h.dest, pPacket->cmd, pPacket->frameCnt, pPacket->crc, crc);
                }

                if (pPacket->crc != crc)
                {
                    if (verbose)
                    {
                        printf("CRC Error!\n");
                    }

                    continue;
                }

                //Not sure what this packet is
                if (pPacket->cmd != 0x0100 || pPacket->h.dest != 0x10)
                {
                    if (verbose)
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

                    if (verbose)
                    {
                        printf("Bytes: 0x%02x%02x%02x%02x, Septett: 0x%02x, crc: 0x%02x(0x%02x)\n",
                            pPacket->frame[j].bytes[0], pPacket->frame[j].bytes[1], pPacket->frame[j].bytes[2], pPacket->frame[j].bytes[3],
                            pPacket->frame[j].septett, pPacket->frame[j].crc, crc);
                    }

                    crcOK = (pPacket->frame[j].crc == crc);
                    if (!crcOK)
                    {
                        if (verbose)
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
                    if (withSql)
                    {
                        sqlite_insert_data(&packet);
                    }
                #endif

                if (print_result)
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

                if (use_mqtt)
                {
                    publish("heizung/ofen/temp", packet.bsPlusPkt.TempSensor1 * 0.1, "%.1f");
                    publish("heizung/ofen/pump", packet.bsPlusPkt.PumpSpeed1);
                    publish("heizung/ruecklauf/temp", packet.bsPlusPkt.TempSensor4 * 0.1, "%.1f");
                    publish("heizung/ruecklauf/valve", packet.bsPlusPkt.PumpSpeed2 / 100);
                    publish("heizung/speicher/oben/temp", packet.bsPlusPkt.TempSensor3 * 0.1, "%.1f");
                    publish("heizung/speicher/unten/temp", packet.bsPlusPkt.TempSensor2 * 0.1, "%.1f");
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

    if (delay > 0)
    {
        if (delay == 60)
        {
            time_t rawtime;
            struct tm * timeinfo;

            time (&rawtime);
            timeinfo = localtime (&rawtime);

            if (timeinfo->tm_sec < 59)
            {
                sleep(delay - timeinfo->tm_sec);
            }
            else
            {
                sleep(delay);
            }
        }
        else
        {
            sleep(delay);
        }

        goto start;
    }

    return 0;
}
