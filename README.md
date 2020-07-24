# esp8266door

![pic1](https://github.com/gdmn/esp8266door/blob/master/pic1.jpg)

![pic2](https://github.com/gdmn/esp8266door/blob/master/pic2.jpg)

## How it works

Project uses ATTiny45 (although it should fit into 25).
ATTiny monitors one pin for the change.
It is important to pull that pin HIGH or LOW depending on the used sensor.
ATTiny can detect any change (raising and falling) using pin change interrupt.

When the change is detected, pin "client power" goes HIGH.
The client (ESP8266 in this case) runs its code.
ATTiny waits for the client blinking every second (if LED is connected to "status led" pin).
When processing is finished, ESP8266 sends LOW state to the "client callback" pin on the ATTiny.
ATTiny changes "client power" pin to LOW, effectively powering down ESP8266.

Pin names are in sketches.

Remember about pull-up and pull-down resistors. I used 10K.

## Configuration

If there is no configuration yet, ESP8266 turns itself in Access Point mode.
The pin "configuration portal" can be pulled down to force Access Point mode.

The project uses [WiFiSettings by Juerd](https://github.com/Juerd/ESP-WiFiSettings):

![WiFiSettings example](https://github.com/Juerd/ESP-WiFiSettings/blob/master/screenshots/basic-example.png)

Additional configuration options (not present in the screenshot):

- ``alarm_text`` - the text to be sent when pin changes state. Default: "⚠️ Alarm! ⚡ ${VCC} volts. 📶 ${RSSI} ${SSID}, ${IP}, ${MAC}.",
- ``telegram_bot_token`` - token of your telegram bot,
- ``telegram_user_id`` - your telegram user id.

### ``alarm_text`` configuration

Following parts of the text will be replaced before the message is sent:

- ``${VCC}`` - internal voltage, when a regulator is used, will be 3V most of the time,
- ``${RSSI}`` - network signal strength, 100 % is the strongest signal, 0 % - the weakest,
- ``${MAC}`` - mac address,
- ``${SSID}`` - network name,
- ``${IP}`` - local IP address.

### How to add your own telegram bot and get the token

1. Install Telegram on local PC or your phone: [from Play Store](https://play.google.com/store/apps/details?id=org.telegram.messenger) or [from F-Droid](https://f-droid.org/en/packages/org.telegram.messenger/).
1. Talk to [@BotFather](https://telegram.me/botfather) to create your bot. [Telegram bot manual can be found here](https://core.telegram.org/bots).
1. Note the token to access the HTTP API.
1. Do not forget to start conversation with your newly created bot!

### How to find telegram user id

1. Talk to [@userinfobot](https://telegram.me/userinfobot).
1. The bot will give you your user id.
