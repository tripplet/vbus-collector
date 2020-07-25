# vbus-collector
[![Build Status](https://travis-ci.org/tripplet/vbus-collector.svg?branch=master)](https://travis-ci.org/tripplet/vbus-collector)
[![](https://img.shields.io/docker/build/ttobias/vbus-collector.svg)](https://hub.docker.com/r/ttobias/vbus-collector/)
[![](https://images.microbadger.com/badges/image/ttobias/vbus-collector.svg)](https://microbadger.com/images/ttobias/vbus-collector)
[![GitHub license](https://img.shields.io/github/license/tripplet/vbus-collector.svg)](https://github.com/tripplet/vbus-collector/blob/master/LICENSE.txt)


Data visualization is done by [vbus-server](https://github.com/tripplet/vbus-server)

## Features
* Save data to sqlite database
* Send data to mqtt brocker (for integration in other home automation software, e.g. [Home Assistant](https://www.home-assistant.io/))

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

Compile the data collector service and the include mqtt library
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


### Setting up config files

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

Check that data is beeing written to the sqlite database
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
