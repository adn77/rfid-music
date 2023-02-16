Using MFRC522 with Arduino Pro Mini and ESP-01 in a power save mode. The code is provided as a working example based on the discussion here https://github.com/miguelbalboa/rfid/issues/269

Parts (all none originals from aliexpress):
- MFRC522
- Arduino Pro Mini, 3.5V
- ESP-01 ESP8266

take alook at pinout.txt for the pinout and some ideas behind the project

adn77 added note:

- I am running on three AA rechargeable batteries.
- The ESP-01 is not powered by the DC/DC converter.
- Power-LEDs from the Pro Mini, ESP-01 and MFRC522 have been desoldered.
- I compiled the sketches using the Arduino IDE, therefore had to copy the simpleserial.* into the respective folder.
- Some cards just were not read (actually most of my 30 cards) until I figured out, I had to call the *PCD_SoftPowerUp()*

This is the code I use for the UDP receiver:
```
#!/bin/bash
OLDPARAM=""
OLDTIME=$(date +%s)
while read -r udp_cmd ; do
        if [[ $udp_cmd =~ ^[A-Za-z0-9:]*$ ]] ; then
                TIME=$(date +%s)
                PARAM=${udp_cmd##*:}
                # block reading multiple times the same card
                if [ "$PARAM" != "$OLDPARAM" -o $((TIME - OLDTIME)) -gt 10 ] ; then
                        case $PARAM in
                                *)
                                        echo $PARAM
                                        echo -e "\"$(date '+%Y-%m-%d %H:%M:%S')\"\t${udp_cmd}"
                                        ;;
                        esac
                        OLDPARAM=$PARAM
                fi
                OLDTIME=$TIME
        fi
done< <(exec socat - udp4-listen:1234,reuseaddr,fork)
```
