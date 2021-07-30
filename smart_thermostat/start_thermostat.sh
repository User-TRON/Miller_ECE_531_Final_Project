#!/bin/bash

killall -9 thermostat
killall -9 tcsimd
rm ./thermostat
rm ./thermostat.o
make -f makefile-x86
../thermocouple/tcsimd
./thermostat
tail -f /var/log/syslog

