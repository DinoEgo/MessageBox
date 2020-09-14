#include <Arduino.h>
#include <string.h>
#include <FS.h>

#include <SPI.h>
#include <TFT_eSPI.h>

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Base64.h>

#define SERIAL_DEBUG
#include "SerialDebug.h"

const char caCert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
-----END CERTIFICATE-----
)EOF";
X509List caCertX509(caCert);

//wifi
WiFiClientSecure espClient;
String ssid = "";
String password = "";
static const char *fingerprint PROGMEM = "";
int wifiStatus;

//MQTT
PubSubClient client(espClient);
String MY_UUID = "c2fbca29-ddf3-4e86-bfac-f366bbeb3eb1";
String displayMessage = "";

//screen
TFT_eSPI tft = TFT_eSPI();

enum ScreenState
{
    none,
    calibrate,
    wifi
};
ScreenState currentScreen = ScreenState::none;

const String text_keyboard[42] = {
    "OK", "Clear", "Del", "Shift", "Caps", "Sym",
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
    "q", "w", "e", "r", "t", "y", "u", "i", "o", "p",
    "a", "s", "d", "f", "g", "h", "j", "k", "l",
    "z", "x", "c", "v", "b", "n", "m"};

const String symbol_keyboard[42] = {
    "OK", "Clear", "Del", "Shift", "Caps", "txt",
    "`", "¬", "!", "\"", "£", "$", "%", "^", "&", "*",
    "(", ")", "_", "-", "+", "=", "{", "[", "}", "]",
    ";", ":", "\'", "@", "#", "~", ",", "<", ".",
    ">", "/", "?", "|", "\\", "", ""};

TFT_eSPI_Button keys[42];

// This is the file name used to store the calibration data
#define CALIBRATION_FILE "/TouchCalData"

// Set REPEAT_CAL to true instead of false to run calibration
// again, otherwise it will only be done once.
// Repeat calibration if you change the screen rotation.
#define REPEAT_CAL false

void touch_calibrate()
{
    uint16_t calData[5];
    uint8_t calDataOK = 0;

    // check file system exists
    if (!SPIFFS.begin())
    {
        Serial.println("Formating file system");
        SPIFFS.format();
        SPIFFS.begin();
    }

    // check if calibration file exists and size is correct
    if (SPIFFS.exists(CALIBRATION_FILE))
    {
        if (REPEAT_CAL)
        {
            // Delete if we want to re-calibrate
            SPIFFS.remove(CALIBRATION_FILE);
        }
        else
        {
            File f = SPIFFS.open(CALIBRATION_FILE, "r");
            if (f)
            {
                if (f.readBytes((char *)calData, 14) == 14)
                    calDataOK = 1;
                f.close();
            }
        }
    }

    if (calDataOK && !REPEAT_CAL)
    {
        // calibration data valid
        tft.setTouch(calData);
    }
    else
    {
        // data not valid so recalibrate
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(20, 0);
        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);

        tft.println("Touch corners as indicated");

        tft.setTextFont(1);
        tft.println();

        if (REPEAT_CAL)
        {
            tft.setTextColor(TFT_RED, TFT_BLACK);
            tft.println("Set REPEAT_CAL to false to stop this running again!");
        }

        tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);

        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.println("Calibration complete!");

        // store data
        File f = SPIFFS.open(CALIBRATION_FILE, "w");
        if (f)
        {
            f.write((const unsigned char *)calData, 14);
            f.close();
        }
    }
}

//setup code
void setupWifi()
{
    SerialDebugln("setupWifi");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    SerialDebug("Your are connecting to;");
    SerialDebugln(ssid);
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 15)
    {
        delay(500);
        SerialDebug(".");
        ++retries;
    }
}

void OnMessage(char *topic, byte *payload, int length)
{
    SerialDebug("Message Received: [");
    displayMessage = "";
    for (int i = 0; i < length; ++i)
    {
        SerialDebug((char)payload[i]);
        displayMessage += (char)payload[i];
    }
    SerialDebug("]");
}

void MQTTSetup()
{
    client.setBufferSize(10000);
    client.setServer("192.168.8.145", 8883);
    client.setCallback(OnMessage);
}

void setupDisplay()
{
    // Initialise the TFT screen
    tft.init();
    tft.setRotation(1);
    touch_calibrate();
}

void setup()
{
#ifdef SERIAL_DEBUG
    Serial.begin(921600);
#endif
    setupDisplay();
    //setupWifi();
    delay(200);
    espClient.setTrustAnchors(&caCertX509); //set the certificate
    espClient.setFingerprint(fingerprint);  //only accept connections from certs with this fingerprint
    espClient.allowSelfSignedCerts();       //allow my certs
    //espClient.setInsecure(); //this will allow connections from any server
    MQTTSetup();
    SerialDebugln("Setup Complete")
}

void ReconnectMQTT()
{
    while (!client.connected())
    {
        SerialDebug(" . ");
        if (client.connect(MY_UUID.c_str()))
        {
            // Once connected, publish an announcement...
            client.publish("ConnectedClients", MY_UUID.c_str());
            // subscribe
            client.subscribe(("MessageBox/" + MY_UUID).c_str());
        }
        else
        {
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

void MQTTLoop()
{
    if (!client.connected())
    {
        SerialDebugln("MQTT not Connected - Reconnecting")
            ReconnectMQTT();
    }
    client.loop();
}

void drawWifi()
{
    if (currentScreen != ScreenState::wifi) // draw screen
    {
        currentScreen = ScreenState::wifi;
        tft.fillScreen(0x000000); //fill black
        tft.setCursor(5, 20, 2);
        tft.setTextSize(1);
        tft.print("SSID: ");
        //draw a box to the right
        const int32_t ssid_x = 40;
        const int32_t ssid_y = 20;
        const int32_t ssid_w = 150;
        const int32_t ssid_h = 20;
        tft.drawRect(ssid_x, ssid_y, ssid_w, ssid_h, 0xFFFFFF);

        tft.setCursor(220, 20, 2);
        tft.print("Password: ");
        //draw a box to the right
        const int32_t pp_x = 281;
        const int32_t pp_y = 20;
        const int32_t pp_w = 150;
        const int32_t pp_h = 20;
        tft.drawRect(pp_x, pp_y, pp_w, pp_h, 0xFFFFFF);

        int x = 40;
        int y = 180;
        for (uint i = 0; i < 42; ++i)
        {
            //first 6 reserved, then new line
            if (i <= 5)
            {
                keys[i].initButton(&tft, x, y, 40, 25, TFT_WHITE, TFT_BLUE, TFT_WHITE, (char *)(text_keyboard[i].c_str()), 1);
                x += 45;
            }
            else
            {
                keys[i].initButton(&tft, x, y, 35, 25, TFT_WHITE, TFT_LIGHTGREY, TFT_BLACK, (char *)(text_keyboard[i].c_str()), 1);
                x += 40;
            }

            keys[i].drawButton();

            if (
                i == 5 ||  //ok,clear,del,shift,caps,txt
                i == 15 || //0123456789
                i == 25 || //qwertyuiop
                i == 34)   //asdfghjkl
            {
                x = 60;
                y += 30;
            }
        }
    }
    return;
}

void wifiSetup()
{
    //draw screen once
    drawWifi();

    uint16_t t_x = 0, t_y = 0; // To store the touch coordinates

    uint8_t touched = tft.getTouch(&t_x, &t_y);
    SerialDebug("x ");
    SerialDebug(t_x);
    SerialDebug(" y ");
    SerialDebug(t_y);

    for (uint8_t i = 0; i < 42; ++i)
    {
        if (touched && keys[i].contains(t_x, t_y))
        {
           
                keys[i].press(true);
                SerialDebugln(text_keyboard[i]);
        }
        else
        {
            keys[i].press(false);
        }
    }

    for (uint8_t i = 0; i < 42; ++i)
    {
        if (keys[i].justReleased())
        {
            keys[i].drawButton(false);
        }

        if (keys[i].justPressed())
        {
            keys[i].drawButton(true);
            switch (i)
            {
            case 0: //OK
                /* if on ssid move to password,
                if on password try to connect*/
                break;
            case 1: //Clear
                /* clear ssid or password, whatever is selected */
                if(ssidActive)
                    ssid = "";
                else
                    password = "";
                break;
            case 2: //Del
                /* clear one char from whatever is selected */
                break;
            case 3: //Shift
                /* capitalise next char */
                break;
            case 4: //Caps
                /* capatalise all chars until toggled */
                break;
            case 5: //Sym
                /* switch keyboards */
                break;

            default:
                ssid += text_keyboard[i];
                break;
            }
        }
    }

    tft.setCursor(42, 22);
    tft.print(ssid);
    delay(25);

    return;
}

/**
 * There are multiple states of screen:
 * 1) on startup there is no calibration file, calibration screen will show NOTE: this is not handled here
 * 2) on startup when wifi is not configured, when wifi cannot connect for any reason or when the user wants to reconfigure wifi settings
 * 3) message display screen - this will have an area for the latest message to be displayed, and a reserved area for buttons (new drawing, settings).
 * 4) drawing screen - this will have a area for the user to draw as well as a button to go back a colour selector and drawing tool select,
 * using the same area reserved in the message display screen
 * 5) settings screen, a menu for configuring the device (wifi settings, re-calibration, partner device registration)
 * 6) partner device registration - this will allow the user to add/change the device they are sending to
 * 
 * Each screen will handle resetting and control independantly, state will be tracked elsewhere
 **/
void loopScreen()
{

    // Pressed will be set true is there is a valid touch on the screen
    wifiStatus = WiFi.status();

    //SerialDebugln(wifiStatus);
    //if(wifiStatus != WL_CONNECTED)
    {
        wifiSetup();
        return;
    }
    // / Check if any key coordinate boxes contain the touch coordinates
    // for (uint8_t b = 0; b < 15; b++) {
    //   if (pressed && key[b].contains(t_x, t_y)) {
    //     key[b].press(true);  // tell the button it is pressed
    //   } else {
    //     key[b].press(false);  // tell the button it is NOT pressed
    //   }
    // }
}

void loop(void)
{
    loopScreen();
    // tft.fillScreen(random(0xFFFF));
    // tft.setCursor(0, 0, 2);
    // // Set the font colour to be white with a black background, set text size multiplier to 1
    // tft.setTextColor(TFT_WHITE,TFT_BLACK);
    // tft.setTextSize(1);
    // // We can now plot text on screen using the "print" class
    // tft.println("Hello World!");
    // SerialDebugln(".");
    // //MQTTLoop();

    // wifiStatus = WiFi.status();

    // if(wifiStatus == WL_CONNECTED){
    //     SerialDebugln("");
    //     SerialDebugln("Your ESP is connected!");
    //     // tft.setCursor(10,10,2);
    //     // tft.setTextColor(TFT_WHITE, TFT_BLACK);
    //     // tft.setTextSize(1);
    //     // tft.println("Connected to" + ssid);
    //     // tft.println("Your IP address is: " + WiFi.localIP().toString());
    //     SerialDebugln("Your IP address is: ");
    //     SerialDebugln("Message:  " + displayMessage);
    //     SerialDebugln(WiFi.localIP());
    // }
    // else{
    //   SerialDebugln("");
    //   SerialDebugln("WiFi not connected");
    // }
}