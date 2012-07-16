#!/system/bin/sh

# This script can be pushed to device and used as a helper to start
# ADTF testing. It's not required.
#
# Sleep times are just magic numbers that works decent enough.
# Service starts/stops are async

setprop debug.hwc.showfps 1
echo "hwc fps dump enabled"

echo "Stopping all services ..."
stop
sleep 5

echo "Starting SurfaceFlinger ..."
start surfaceflinger
sleep 5

setprop ctl.stop bootanim
echo "Boot animation stopped"

# Try setting screen brightness to max for some "known" paths
lcds=("/sys/class/leds/lcd-backlight/brightness" "/sys/class/backlight/s6e8aa0/brightness")

for lcd in $lcds
do
    if [ -a $lcd ];
    then
        echo 255 > $lcd
        echo "255 written to $lcd"
    fi
done
