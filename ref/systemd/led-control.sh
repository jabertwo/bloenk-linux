#!/bin/bash

# Check if the correct number of arguments is provided
if [ "$#" -ne 5 ]; then
    echo "Usage: $0 <lednum> <brightness> <red> <green> <blue>"
    exit 1
fi

# Assign input variables
LEDNUM=$1
BRIGHTNESS=$2
RED=$3
GREEN=$4
BLUE=$5

# LED sysfs path (update if different)
LED_PATH="/sys/class/leds/bloenk0:$LEDNUM"

echo $LED_PATH

# Set LED brightness
echo $BRIGHTNESS > $LED_PATH/brightness

# Set individual RGB color intensities
echo $RED $GREEN $BLUE  > $LED_PATH/multi_intensity

