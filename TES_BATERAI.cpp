#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <Adafruit_INA219.h>

Adafruit_INA219 ina219;

#define ON_Board_LED 2 // On board LED, indicator when connecting to a wifi router

const char *ssid = "MAKERINDO2";        // Your wifi name
const char *password = "makerindo2019"; // Your wifi password

//----------------------------------------Host & httpsPort
const char *host = "script.google.com";
const int httpsPort = 443;
//----------------------------------------

WiFiClientSecure client; // Create a WiFiClientSecure object

// Google spreadsheet script ID
String GAS_ID = "AKfycbz5sDcy3Z6VeczMARN7YZwGKzcdcAlGuFWFgRyw05RFLcZUEC6CcU4-sM6ACuuwRCZb";

void sendData(float bus, float shunt, float load, float current, float power);

void setup()
{
    Serial.begin(115200);

    // for sensor INA219
    if (!ina219.begin())
    {
        Serial.println("Failed to find INA219 chip");
        while (1)
        {
            delay(10);
        }
    }
    ina219.setCalibration_16V_400mA();

    WiFi.begin(ssid, password); // Connect to your WiFi router
    Serial.println("");

    pinMode(ON_Board_LED, OUTPUT);    // On board LED port as output
    digitalWrite(ON_Board_LED, HIGH); // Turn off Led on board

    //----------------------------------------Wait for connection
    Serial.print("Connecting");
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        //----------------------------------------Make LED flashing when connecting to the wifi router
        digitalWrite(ON_Board_LED, LOW);
        delay(200);
        digitalWrite(ON_Board_LED, HIGH);
        delay(200);
        //----------------------------------------
    }
    //----------------------------------------
    digitalWrite(ON_Board_LED, HIGH); // Turn off the LED when it is connected to the wifi router
    Serial.println("");
    Serial.print("Successfully connected to : ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println();
    //----------------------------------------

    client.setInsecure();
}

void loop()
{
    float shuntvoltage = 0;
    float busvoltage = 0;
    float current_mA = 0;
    float loadvoltage = 0;
    float power_mW = 0;

    shuntvoltage = ina219.getShuntVoltage_mV();
    busvoltage = ina219.getBusVoltage_V();
    current_mA = ina219.getCurrent_mA();
    power_mW = ina219.getPower_mW();
    loadvoltage = busvoltage + (shuntvoltage / 1000);

    String Bus = "Bus Voltage : " + String((int)(busvoltage * 1000)) + " mV";
    String Shunt = "Shunt Volatge : " + String(shuntvoltage) + " mV";
    String Load = "Load Voltage : " + String(loadvoltage * 1000) + " mV";
    String Current = "Current : " + String(current_mA) + " mA";
    String Power = "Power : " + String(power_mW) + " mW";
    Serial.println(Bus);
    Serial.println(Shunt);
    Serial.println(Load);
    Serial.println(Current);
    Serial.println(Power);

    sendData(busvoltage, shuntvoltage, loadvoltage, current_mA, power_mW); // Call the sendData subroutine
    delay(1000);
}

// Subroutine for sending data to Google Sheets
void sendData(float bus, float shunt, float load, float current, float power)
{
    Serial.println("==========");
    Serial.print("connecting to ");
    Serial.println(host);

    //----------------------------------------Connect to Google host
    if (!client.connect(host, httpsPort))
    {
        Serial.println("connection failed");
        return;
    }
    //----------------------------------------

    //----------------------------------------Processing data and sending data
    String Bus = String((int)(bus * 1000));
    String Shunt = String(shunt);
    String Load = String(load * 1000);
    String Current = String(current);
    String Power = String(power);
    String url = "/macros/s/" + GAS_ID + "/exec?busVolatge=" + Bus + "&shuntVoltage=" + Shunt + "&loadVoltage=" + Load + "&current=" + Current + "&power=" + Power;
    Serial.print("requesting URL: ");
    Serial.println(url);

    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "User-Agent: BuildFailureDetectorESP8266\r\n" +
                 "Connection: close\r\n\r\n");

    Serial.println("request sent");
    //----------------------------------------

    //----------------------------------------Checking whether the data was sent successfully or not
    while (client.connected())
    {
        String line = client.readStringUntil('\n');
        if (line == "\r")
        {
            Serial.println("headers received");
            break;
        }
    }
    String line = client.readStringUntil('\n');
    if (line.startsWith("{\"state\":\"success\""))
    {
        Serial.println("esp32/Arduino CI successfull!");
    }
    else
    {
        Serial.println("esp32/Arduino CI has failed");
    }
    Serial.print("reply was : ");
    Serial.println(line);
    Serial.println("closing connection");
    client.stop();
    Serial.println("==========");
    Serial.println();
    //----------------------------------------
}
