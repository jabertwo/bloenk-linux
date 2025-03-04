#!/bin/bash

# Space API URL (bitte anpassen!)
SPACE_API_URL="https://api.warpzone.ms/spaceapi"

# Liste aller LEDs
LED_PATHS=(
    "/sys/class/leds/bloenk0:0"
    "/sys/class/leds/bloenk0:1"
    "/sys/class/leds/bloenk0:2"
    "/sys/class/leds/bloenk0:3"
)

while true; do
    # Warpzone-Status abrufen (1=open, 0=closed)
    STATUS=$(curl -s "$SPACE_API_URL" | jq -r '.state.open')

    if [[ "$STATUS" == "true" ]]; then
        for LED in "${LED_PATHS[@]}"; do
            echo 0 255 0 > "$LED/multi_intensity"
            echo 255 > "$LED/brightness"
        done
    else
        for LED in "${LED_PATHS[@]}"; do
	    echo 255 0 0 > "$LED/multi_intensity"
            echo 255 > "$LED/brightness"
        done
    fi

    sleep 30 # Alle 30 Sekunden neu prüfen
done
