#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include "BluetoothSerial.h"

#include "Preferences.h"

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 128 // OLED display height, in pixels
#define OLED_DC 2 //33
#define OLED_CS 5
#define OLED_RST 4 //26
#define SH110X_NO_SPLASH 
#define LED_ALARM  22

Adafruit_SH1107 tft = Adafruit_SH1107(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RST, OLED_CS);

volatile float press[4] = {0.0, 0.0, 0.0, 0.0};
volatile int volt[4] = {0, 0, 0, 0};
volatile int temp[4] = {0, 0, 0, 0};
volatile uint8_t alarms[4] = {0, 0, 0, 0};

bool updates[4] = {0, 0, 0, 0};
int Xinit[4] = {5, 70, 5, 70};
int Yinit[4] = {5, 5, 70, 70};

uint8_t j;
uint32_t temp16;

String TPMS_BLE_addr[] = {"80:ea:ca:13:55:4f", "81:ea:ca:23:4f:c6", "82:ea:ca:33:2c:79", "83:ea:ca:43:2b:f1"};

BLEScan* pBLEScan;

BluetoothSerial ESP_BT;
String bt_str;

Preferences preferences;

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        String addr = advertisedDevice.getAddress().toString().c_str();
        uint8_t* payload = advertisedDevice.getPayload();
        uint8_t payloadlen = advertisedDevice.getPayloadLength();
  
        for (int i = 0; i < (sizeof(TPMS_BLE_addr) / sizeof(TPMS_BLE_addr[0])); i++) {
            if (strcmp(advertisedDevice.getAddress().toString().c_str(), TPMS_BLE_addr[i].c_str()) == 0) {
                if (payloadlen > 25) {
                    temp16 = ((payload[22]<<8)|payload[21]);
                    temp[i] = floor(temp16/100.0 + 0.5);
                    temp16 = ((payload[19]<<8)|payload[18]);
                    press[i] = temp16*0.00256 + 0.053;
                    if (payload[25] > 99) {
                        volt[i] = 99;
                    }
                    else {
                        volt[i] = payload[25];
                    }
                    alarms[i] = payload[26];
                    updates[i] = 1;
                }
            }
        }
    }  // void
};  // class


void setup() {
    pinMode(LED_ALARM, OUTPUT);

    preferences.begin("ford_tpms_cfg", false);
    TPMS_BLE_addr[0] = preferences.getString("addr_s1", "80:ea:ca:13:55:4f");
    TPMS_BLE_addr[1] = preferences.getString("addr_s2", "81:ea:ca:23:4f:c6");
    TPMS_BLE_addr[2] = preferences.getString("addr_s3", "82:ea:ca:33:2c:79");
    TPMS_BLE_addr[3] = preferences.getString("addr_s4", "83:ea:ca:43:2b:f1");

    delay(250); // wait for the OLED to power up
    tft.begin(0x3D, true); // Address 0x3D default
    tft.setContrast(80); // dim display
    tft.setRotation(3);
    tft.display();
    delay(10);
  
    // Clear the buffer.
    tft.clearDisplay();
    tft.drawLine(64, 0, 64, 127, SH110X_WHITE);
    tft.drawLine(0, 64, 127, 64, SH110X_WHITE);
    tft.display();
  
    tft.setTextColor(SH110X_WHITE);
  
    for (j = 0; j < 4; j++) {
        tft.setCursor(Xinit[j] + 38, Yinit[j] + 3);
        tft.setTextSize(1);
        tft.println("bar");
        tft.setTextSize(2);
        tft.setCursor(Xinit[j], Yinit[j]);
        tft.println(press[j], 1);
        
        tft.setCursor(Xinit[j] + 5, Yinit[j] + 20);
        tft.println(temp[j]);
        tft.drawCircle(Xinit[j] + 33, Yinit[j] + 20, 2, SH110X_WHITE);
        tft.setCursor(Xinit[j] + 39, Yinit[j] + 20);
        tft.println("C");
      
        tft.setCursor(Xinit[j] + 5, Yinit[j] + 40);
        tft.println(volt[j]);
        tft.setCursor(Xinit[j] + 33, Yinit[j] + 40);
        tft.println("%V");
      
        tft.display();
    }

    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
	
	ESP_BT.begin("Ford_TPMS_CFG"); //Name of your Bluetooth Signal
}

void loop() {
    BLEScanResults * foundDevices = pBLEScan->start(5, false);

    for (j = 0; j < 4; j++) {
        if (updates[j] == 1) {
            updates[j] = 0;
            tft.setTextSize(2);

            tft.fillRect(Xinit[j], Yinit[j], 37, 17, SH110X_BLACK);
            tft.fillRect(Xinit[j] + 5, Yinit[j] + 20, 26, 38, SH110X_BLACK);

            tft.setCursor(Xinit[j], Yinit[j]);
            tft.println(press[j], 1);
            
            tft.setCursor(Xinit[j] + 5, Yinit[j] + 20);
            if (temp[j] < 0) {
                tft.println(-temp[j]);
                tft.fillRect(Xinit[j] + 31, Yinit[j] + 26, 6, 2, SH110X_WHITE);
            }
            else {
                tft.println(temp[j]);
                tft.fillRect(Xinit[j] + 31, Yinit[j] + 26, 6, 2, SH110X_BLACK);
            }
            
            tft.setCursor(Xinit[j] + 5, Yinit[j] + 40);
            tft.println(volt[j]);
            tft.display();
        }
    }
    if ( (alarms[0] + alarms[1] + alarms[2] + alarms[3]) > 0 ) {
        digitalWrite(LED_ALARM, HIGH);
    }
    else {
        digitalWrite(LED_ALARM, LOW);
    }

    pBLEScan->clearResults();
	
	
	//---------------------------
    // Update Settings
    if (ESP_BT.available()) {
      bt_str = ESP_BT.readString();
      if (bt_str[0] == 'A') {
        if (bt_str[1] == '1') {
          TPMS_BLE_addr[0] = bt_str.substring(3);
          preferences.putString("addr_s1", TPMS_BLE_addr[0]);
          ESP_BT.println("TPMS_BLE_addr[0] saved");
        }
        if (bt_str[1] == '2') {
          TPMS_BLE_addr[1] = bt_str.substring(3);
          preferences.putString("addr_s2", TPMS_BLE_addr[1]);
          ESP_BT.println("TPMS_BLE_addr[1] saved");
        }
		if (bt_str[1] == '3') {
          TPMS_BLE_addr[2] = bt_str.substring(3);
          preferences.putString("addr_s3", TPMS_BLE_addr[2]);
          ESP_BT.println("TPMS_BLE_addr[2] saved");
        }
		if (bt_str[1] == '4') {
          TPMS_BLE_addr[3] = bt_str.substring(3);
          preferences.putString("addr_s4", TPMS_BLE_addr[3]);
          ESP_BT.println("TPMS_BLE_addr[3] saved");
        }
      }

      if (bt_str[0] == 'R') {
        if (bt_str[1] == '1') {
          ESP_BT.println(TPMS_BLE_addr[0]);
        }
        if (bt_str[1] == '2') {
          ESP_BT.println(TPMS_BLE_addr[1]);
        }
        if (bt_str[1] == '3') {
          ESP_BT.println(TPMS_BLE_addr[2]);
        }
        if (bt_str[1] == '4') {
          ESP_BT.println(TPMS_BLE_addr[3]);
        }
      }
      
    }
}
