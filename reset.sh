#!/bin/sh

#/usr/bin/modprobe -r cdc_acm
sleep 1
/usr/bin/picocom -q --imap lfcrlf --baud 19200 --noreset -t "$(echo -ne 'getpin\r\n')" -x 1000 /dev/tty_radio
/usr/bin/picocom -q --imap lfcrlf --baud 19200 --noreset -t "$(echo -ne 'setpin 0\r\n')" -X /dev/tty_radio
/usr/bin/picocom -q --imap lfcrlf --baud 19200 --noreset -t "$(echo -ne 'getpin\r\n')" -x 1000 /dev/tty_radio
/usr/bin/picocom -q --imap lfcrlf --baud 19200 --noreset -t "$(echo -ne 'setpin 1\r\n')" -X /dev/tty_radio
/usr/bin/picocom -q --imap lfcrlf --baud 19200 --noreset -t "$(echo -ne 'getpin\r\n')"  -x 1000 /dev/tty_radio
sleep 1
#/usr/bin/modprobe cdc_acm
sleep 1 
/usr/bin/systemctl restart monitor-vbus
