#!/bin/bash

killall thermostat
killall tcsimd
rm ./thermostat
rm ./thermostat.o
make -f makefile-x86
../thermocouple/tcsimd
./thermostat
tail -f /var/log/syslog

