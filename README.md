# vbus-collector
[![Build Status](https://travis-ci.org/tripplet/vbus-collector.svg?branch=master)](https://travis-ci.org/tripplet/vbus-collector)
[![](https://img.shields.io/docker/build/ttobias/vbus-collector.svg)](https://hub.docker.com/r/ttobias/vbus-collector/)
[![](https://images.microbadger.com/badges/image/ttobias/vbus-collector.svg)](https://microbadger.com/images/ttobias/vbus-collector)
[![GitHub license](https://img.shields.io/github/license/tripplet/vbus-collector.svg)](https://github.com/tripplet/vbus-collector/blob/master/LICENSE.txt)


Data visualization is done by [vbus-server](https://github.com/tripplet/vbus-server)

**The easiest way to use this project if Homeassistant is already used is to install the Addon [Hassio VBUS](https://github.com/tripplet/hassio-vbus)**

## Features
* Save data to sqlite database
* Send data to mqtt brocker (for integration in other home automation software)
* Direct HTTP integration to Homeassistant

## Docker container
https://hub.docker.com/r/ttobias/vbus-collector/

## HowTo Build
The RaspberryPi or other linux machine should be running and connected to the internet, ssh sould be available.

* Get root via `sudo -s`, `su` or other ways :smile:

Get the necessary packages (raspbian)
```shell
$ apt-get update
$ apt-get install git build-essential cmake libsqlite3-dev sqlite
```

Get the necessary packages (archlinux-arm)
```shell
$ pacman -Syu
$ pacman -S git base-devel cmake libsqlite3-dev sqlite3
```

Download the source code
```shell
$ mkdir -p /srv/vbus
$ cd /srv/vbus
$ git clone --recurse-submodules https://github.com/tripplet/vbus-collector.git collector
```

Compile the data collector service and the included libraries
```shell
$ cd /srv/vbus/collector/paho.mqtt.c
$ mkdir build && cd build
$ cmake -DPAHO_BUILD_STATIC=TRUE ..
$ make -j
$ cd "/srv/vbus/collector/cJSON"
$ mkdir build && cd build
$ cmake -DBUILD_SHARED_LIBS=OFF -DENABLE_CJSON_TEST=OFF -DENABLE_CJSON_UTILS=OFF -DENABLE_LOCALES=OFF ..
$ make -j
$ cd /srv/vbus/collector
$ make
```


## Setting up system files

Now the udev rule and systemd service file need to be soft linked to the right locations
```shell
$ ln -s /srv/vbus/collector/00-resol-vbus-usb.rules /etc/udev/rules.d/
$ ln -s /srv/vbus/collector/monitor-vbus.service /etc/systemd/system/
```

Get the connected usb devices, identify the vbus adapter and make sure the
_00-resol-vbus-usb.rules_ file contains the correct vid and pid.
For mine it was vid=1fef, and pid=2018, if these are different on your device,  change them in the provided `00-resol-vbus-usb.rules` file.
```
$ lsusb
  Bus 001 Device 011: ID 1fef:2018
  ...
```

Now reload the udev rules
```shell
$ udevadm control --reload-rules
```

If the VBUS-USB adapter is connected the file /dev/tty_resol should exist
```shell
$ stat /dev/tty_resol
  File: '/dev/tty_resol' -> 'ttyACM0'
  Size: 7    Blocks: 0    IO Block: 4096    symbolic link
```

Check if the collector is working (stop with Ctrl+C)
```shell
$ /srv/vbus/collector/vbus-collector --delay 1 /dev/tty_resol
  System time:13:19, Sensor1 temp:20.7C, Sensor2 temp:21.0C, Sensor3 temp:22.9C, Sensor4 temp:24.0C, Pump speed1:0%, Pump speed2:0%, Hours1:2302, Hours2:2425
  System time:13:19, Sensor1 temp:20.7C, Sensor2 temp:21.0C, Sensor3 temp:22.9C, Sensor4 temp:24.0C, Pump speed1:0%, Pump speed2:0%, Hours1:2302, Hours2:2425
  System time:13:19, Sensor1 temp:20.7C, Sensor2 temp:21.0C, Sensor3 temp:22.9C, Sensor4 temp:24.0C, Pump speed1:0%, Pump speed2:0%, Hours1:2302, Hours2:2425
```

Start the monitor-vbus service, remove the "--mqtt" parameter form the service file if no mqtt server is a available at localhost:1883
```shell
$ systemctl start monitor-vbus
```

Check wether the service is running properly
```shell
$ systemctl status monitor-vbus
  ● monitor-vbus.service - Monitor resol vbus temperatures
     Loaded: loaded (/srv/vbus/collector/monitor-vbus.service; linked; vendor preset: disabled)
     Active: active (running) since Mi 2015-09-02 13:29:23 CEST; 10min ago
   Main PID: 12422 (vbus-collector)
     CGroup: /system.slice/monitor-vbus.service
             └─12422 /srv/vbus/collector/vbus-collector --no-print --delay 60 --db /srv/vbus/collector/data.db /dev/tty_resol
```

Check that data is being written to the sqlite database
```shell
$ sqlite3 /srv/vbus/collector/data.db "SELECT * FROM data ORDER BY id DESC LIMIT 4;"
  174837|2015-09-02 11:28:10|10:24|18.8|20.9|22.6|22.9|0|0|2302|2425
  174836|2015-09-02 11:29:07|10:22|18.9|20.9|22.7|22.9|0|0|2302|2425
  174835|2015-09-02 11:30:05|10:21|18.8|20.9|22.6|22.9|0|0|2302|2425
  174834|2015-09-02 11:31:03|10:20|18.9|20.9|22.6|22.9|0|0|2302|2425
```
> Date/Time values in the sqlite database are stored in UTC.
> To get the correct local time ensure that the timezone on the system is set properly and use:
> ```shell
> $ sqlite3 /srv/vbus/collector/data.db "SELECT datetime(time, 'localtime'),* FROM data;"
> ```

## Options file

*vbus-collector* can be controlled using cli options for fast testing but the preferred way is to use an options file.

For production the following file could be used.
```json
{
    "device": "/dev/serial/by-id/usb-1fef_2018-if00",
    "interval": 60,
    "verbose": false,
    "database": "/srv/vbus/collector/data.db",
    "print_stdout": false,
    "mqtt": {
        "enabled": false,
        "base_topic": "heizung",
        "server": "tcp://localhost:1883",
        "client_id": "vbus",
        "user": null,
        "password": null
    },
    "homeassistant": {
        "enabled": false,
        "entity_id_base": "sensor.heating"
    }
}
```

Usage:
```shell
$ vbus-collector --config options.json
```

## Homeassistant integration

If Homeassistant is enabled in the options file, *vbus-collector* will send all sensor updates to the  Homeassitant instance using the [HTTP REST API](https://developers.home-assistant.io/docs/api/rest).

All sensors are created using the `entity_id_base` appended with and underscore and the sensor name.
For example: `sensor.heating_furnace`


How to enable:
1. Enable the homeassistant in the options.json.
2. Enable the [API component](https://www.home-assistant.io/integrations/api/) in Homeassitant.
3. Create a [long lived access token](https://www.home-assistant.io/docs/authentication/).
4. Provide the access token using the environment variable `SUPERVISOR_TOKEN`.
4. Provide the Homeassistant URL using the environment variable `HOMEASSISTANT_API_URL`.
5. Optionally change the `entity_id_base` option.

Setting environment variables for the systemd service can be done by creating the `/srv/vbus/collector/homeassistant-secrets` with the following content:

```shell
SUPERVISOR_TOKEN=....
HOMEASSISTANT_API_URL=http://127.0.0.1:8123
```

Optionally if Homeassistant is running on a different system the environment variable `HOMEASSISTANT_API_URL` can be used to send the sensor values to a different system.

Additionally I created a Homeassistant Addon [Hassio VBUS](https://github.com/tripplet/hassio-vbus) which runs *vbus-collector* and *vbus-server* inside a Homeassistant Addon.

**The Addon is functional and working but lacks documentation**

Have fun
