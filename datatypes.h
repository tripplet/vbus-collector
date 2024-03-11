#ifndef DATATYPES_H
#define DATATYPES_H

// DeltaSol BS Plus
typedef union __attribute__((packed)) {
  struct {
    short TempSensor1;
    short TempSensor2;
    short TempSensor3;
    short TempSensor4;
    unsigned char PumpSpeed1;
    unsigned char PumpSpeed2;
    unsigned char RelayMask;
    unsigned char ErrorMask;
    unsigned short SystemTime;
    unsigned char Scheme;
    unsigned char OptionCollectorMax:1;
    unsigned char OptionCollectorMin:1;
    unsigned char OptionCollectorFrost:1;
    unsigned char OptionTubeCollector:1;
    unsigned char OptionRecooling:1;
    unsigned char OptionHQM:1;
    unsigned char rfu:2;
    unsigned short OperatingHoursRelay1;
    unsigned short OperatingHoursRelay2;
    unsigned short HeatQuantityWH;
    unsigned short HeatQuantityKWH;
    unsigned short HeatQuantityMWH;
    unsigned short Version;
  } bsPlusPkt;
  unsigned char asBytes[28];
} Data_Packet_BS;

// DeltaSol BS 2009
// http://danielwippermann.github.io/resol-vbus/#/vsf/bytes/00_0010_427B_10_0100
typedef union __attribute__((packed)) {
  struct {
    short TempSensor1;
    short TempSensor2;
    short TempSensor3;
    short TempSensor4;

    unsigned char PumpSpeed1;
    unsigned char ignore_1;

    unsigned short OperatingHoursRelay1;

    unsigned char PumpSpeed2;
    unsigned char ignore_2;

    unsigned short OperatingHoursRelay2;

    unsigned char UnitType;
    unsigned char System;

    unsigned char ignore_3;
    unsigned char ignore_4;

    unsigned short ErrorMask;

    unsigned short SystemTime;

    unsigned int StatusMask;
    unsigned int HeatQuantityWH;

    unsigned short Version;

    unsigned short Variant;
  } bsPlusPkt;
  unsigned char asBytes[36];
} Data_Packet_DeltaSol_BS_2009;

#endif