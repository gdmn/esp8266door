// ESP8266 Boards (2.7.2), lolin(wemos) d1 r2 & mini, 80 mhz

/*
                      +--USB--+
                  RST |       | TX  GPIO1
            ADC0  A0  | Wemos | RX  GPIO3
          GPIO16  D0  | D1    | D1  GPIO5
 callback GPIO14  D5  | mini  | D2  GPIO4 configuration portal
          GPIO12  D6  |       | D3  GPIO0
          GPIO13  D7  |       | D4  GPIO2
          GPIO15  D8  |       | G   GND
                  3V3 |       | 5V
                      +-------+

CHIP_EN (internal Pin7) on Wemos boards is pulled up.
Connection must be cut, CHIP_EN must be connected to
"client power" pin (PB3) on attiny.
To reflash chip, CHIP_EN should be pulled high with 10K resistor.

"callback" (D5) must be connected to "client callback" pin (PB4) on attiny.

To force start of configuration portal (192.168.4.1),
"configuration portal" (D2) must be grounded.

wemos d1 mini + attiny45 uses 0.15mA during deep sleep.
 */

#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include <WiFiSettings.h>

#include "CTBot.h"
CTBot myBot;

const uint32_t LED_PIN = LED_BUILTIN;
const uint32_t SHUTDOWN_PIN = D5;
const uint32_t PORTAL_PIN = D2;
#define LED_ON  LOW
#define LED_OFF HIGH

String conf_alarm_text;
String conf_bot_token;
int conf_user_id;

ADC_MODE(ADC_VCC);  // allows you to monitor the internal VCC level; it varies with WiFi load
                    // don't connect anything to the analog input pin(s)!

void setupSettings() {
  LittleFS.begin();  // Will format on the first run after failing to mount

  // Set custom callback functions
  WiFiSettings.onSuccess  = []() {
      digitalWrite(LED_PIN, LED_ON);
  };
  WiFiSettings.onFailure  = []() {
      digitalWrite(LED_PIN, LED_OFF);
  };
  WiFiSettings.onWaitLoop = []() {
      pinMode(LED_PIN, OUTPUT);
      digitalWrite(LED_PIN, LED_ON);
      return 500; // Delay next function call by 500ms
  };

  // Callback functions do not have to be lambda's, e.g.
  // WiFiSettings.onPortalWaitLoop = blink;

  // Define custom settings saved by WifiSettings
  // These will return the default if nothing was set before
  conf_alarm_text = WiFiSettings.string("alarm_text", "⚠️ Alarm! ⚡ ${VCC} volts. 📶 ${RSSI} ${SSID}, ${IP}, ${MAC}.");
  conf_bot_token = WiFiSettings.string("telegram_bot_token", "123:XXXXX");
  conf_user_id = WiFiSettings.integer("telegram_user_id", 0);

  Serial.print("\nconf_alarm_text: ");
  Serial.println(conf_alarm_text);

  Serial.print("conf_bot_token: ");
  Serial.println(conf_bot_token);

  Serial.print("conf_user_id: ");
  Serial.println(conf_user_id);
}

void goSleep() {
  WiFi.mode(WIFI_SHUTDOWN);  // Forced Modem Sleep for a more Instant Deep Sleep
  Serial.println("\nSetting shutdown pin low...");
  Serial.flush();
  digitalWrite(SHUTDOWN_PIN, LOW);
  Serial.println("\nGoing into Deep Sleep now...");
  Serial.flush();
  ESP.deepSleepInstant(0, WAKE_RF_DEFAULT);
  Serial.println(F("?!?"));  // it will never get here
}

void sendTelegramMessage() {
  uint32_t t0 = millis();
  float volts = ESP.getVcc();
  Serial.printf("\nThe internal VCC reads %1.2f volts\n", volts / 1000);

  float rssi = WiFi.RSSI();
  Serial.printf("Raw connection quality: %1.0f RSSI\n", rssi);
  rssi = isnan(rssi) ? -100.0 : rssi;
  rssi = min(max(2 * (rssi + 100.0), 0.0), 100.0);
  Serial.printf("Connection quality: %1.0f %%\n", rssi);

  myBot.setTelegramToken(conf_bot_token);
  if (myBot.testConnection()) {
    Serial.println("Telegram connection OK");
    char volts_char[6];
    sprintf(volts_char, "%1.2f", volts / 1000);
    conf_alarm_text.replace("${VCC}", volts_char);
    sprintf(volts_char, "%1.0f %%", rssi);
    conf_alarm_text.replace("${RSSI}", volts_char);
    conf_alarm_text.replace("${MAC}", WiFi.macAddress());
    conf_alarm_text.replace("${SSID}", WiFi.SSID());
    conf_alarm_text.replace("${IP}", WiFi.localIP().toString());

    myBot.sendMessage(conf_user_id, conf_alarm_text);
    if (volts < 3005) {
      Serial.println("Voltage low!");
      myBot.sendMessage(conf_user_id, "🔋 Voltage low. Replace battery!");
    }
  } else {
    Serial.println("Telegram connection FAILED");
  }
  float t1 = (millis() - t0);
  Serial.printf("Telegram message time: %1.2f seconds\n", t1 / 1000);
}

void startWifi() {
  uint32_t t0 = millis();
  WiFi.persistent(false);
  WiFi.forceSleepWake();
  delay(1);

  Serial.println("\nTrying to connect");
  if (WiFiSettings.connect(false, 30)) {
    float t1 = (millis() - t0);
    Serial.printf("WiFi connect time: %1.2f seconds\n", t1 / 1000);
  } else {
    Serial.println("WiFi connection FAILED");
    pinMode(LED_PIN, OUTPUT);
    for (int i = 0; i < 20; i++) {
      digitalWrite(LED_PIN, LED_ON);
      delay(25);
      digitalWrite(LED_PIN, LED_OFF);
      delay(25);
    }
    goSleep();
  }

  sendTelegramMessage();
}

void setup() {
  pinMode(SHUTDOWN_PIN, OUTPUT);
  digitalWrite(SHUTDOWN_PIN, HIGH);

  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);

  pinMode(PORTAL_PIN, INPUT_PULLUP);
  pinMode(D1, INPUT_PULLUP);

  Serial.print(F("\nReset reason: "));
  String resetCause = ESP.getResetReason();
  Serial.println(resetCause);
}

void loop() {
  Serial.println("");

  int statePortalPin = digitalRead(PORTAL_PIN);
  Serial.print("PORTAL_PIN: ");
  Serial.println(statePortalPin, DEC);

  int stateD1 = digitalRead(D1);
  Serial.print("D1: ");
  Serial.println(stateD1, DEC);

  setupSettings();
  if (conf_user_id == 0 || statePortalPin == 0) {
    Serial.println("");
    WiFiSettings.portal();
  } else {
    startWifi();
  }

  goSleep();
  delay(1000);
}
