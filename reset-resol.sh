#!/bin/bash

# gpioset is not sufficent as after exit the state is not defined

# Create GPIO
echo 21 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio21/direction

# Reset Resol
echo 0 > /sys/class/gpio/gpio21/value
sleep 2
echo 1 > /sys/class/gpio/gpio21/value
