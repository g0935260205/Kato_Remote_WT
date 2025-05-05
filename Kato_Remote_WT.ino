#include <FS.h>

#ifdef ESP32
#include <SPIFFS.h>
#endif

#include <ArduinoJson.h>
#include <WiThrottle.h>

#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <ESP8266mDNS.h>

#define revrP D1
#define forwP D2
#define stopP D6
#define RledP TX
#define GledP RX
#define BledP D0
#define ThroP A0

#define R1 "L9601"

//  Config
////// necessary
// ADDR: S1-S127/L128-9999
// WIFI_SSID
// WIFI_PSWD
////// option
// WT_IP:192.168.4.1
// WT_Port:12090
bool shouldSaveConfig = false;
void saveConfigCallback()
{
    shouldSaveConfig = true;
}

char WT_IP[15];
char Port[5];
char Slot[5];
// 255.255.255.255

WiThrottle WT;
class MyDelegate : public WiThrottleDelegate
{
public:
    float R1V = 0;
    float R1V_B = 0;
    const float Ts = 0.1;
    Direction R1D = Reverse;
    bool PState = false;

    void updateState()
    {
        Direction T_dir;
        if (!digitalRead(revrP))
        {
            // Reverse
            T_dir = Reverse;
        }
        else if (!digitalRead(forwP))
        {
            // Forward
            T_dir = Forward;
        }
        else
        { // stopped
            if (R1V != 0)
            {
                R1V = 0;
                TransmitRosterV();
            }
            return;
        }
        if (R1D != T_dir)
        {
            R1D = T_dir;
            TransmitRosterD();
        }
        R1V = R1V_B * Ts + (1 - Ts) * analogRead(ThroP) / 4;
        if (R1V != R1V_B)
        {
            TransmitRosterV();
            delay(50);
        }
        R1V_B = R1V;
    }
    void TransmitRosterV()
    {
        WT.setSpeed(0, R1V);
    }
    void TransmitRosterD()
    {
        WT.setDirection(R1D);
    }
};

bool resolve_mdns_service(char *service_name, char *protocol, char *desired_host, IPAddress *ip_addr, uint16_t *port_number)
{
    int n = MDNS.queryService(service_name, protocol);
    if (n == 0)
        return false;

    for (int i = 0; i < n; ++i)
    {
        *ip_addr = MDNS.IP(i);
        *port_number = MDNS.port(i);
        return true;
    }
    return false;
}

WiFiClient client1;
MyDelegate Delegate;
bool status_TCP = false;
bool status_WiT = false;
bool LedStatus[16];
int SleepCount = 0;
void ToggleLED(int Pin)
{
    digitalWrite(Pin, !LedStatus[Pin]);
    LedStatus[Pin] = !LedStatus[Pin];
}
void ResetLed()
{
    digitalWrite(RledP, HIGH);
    digitalWrite(GledP, HIGH);
    digitalWrite(BledP, HIGH);
}
void setup()
{
    uint8_t failCount = 0;
    pinMode(revrP, INPUT_PULLUP);
    pinMode(forwP, INPUT_PULLUP);
    pinMode(stopP, INPUT_PULLUP);

    pinMode(ThroP, INPUT);

#ifdef DEBUG
    Serial.begin(115200);
#else
    pinMode(RledP, OUTPUT);
    pinMode(GledP, OUTPUT);
    pinMode(BledP, OUTPUT);
    ResetLed();
#endif

    // Config FS

    // TestCode
    // clean FS
    // SPIFFS.format();
    // Start
    if (!SPIFFS.begin())
    {
        // FS Fail and halt here
        failCount++;
        while (failCount >= 0)
            ;
    }
    File configFile = SPIFFS.open("/config.json", "w");
    // if (configFile == NULL)
    // {
    //     failCount++;
    //     while (failCount >= 0)
    //         ;
    // }
    size_t size = configFile.size();
    std::unique_ptr<char[]> buf(new char[size]);
    configFile.readBytes(buf.get(), size);

#if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
    DynamicJsonDocument json(1024);
    auto deserializeError = deserializeJson(json, buf.get());
    serializeJson(json, Serial);
    if (!deserializeError)

#else
    DynamicJsonBuffer jsonBuffer;
    JsonObject &json = jsonBuffer.parseObject(buf.get());
    json.printTo(Serial);
    if (json.success())

#endif
    {
    }
    WiFiManagerParameter WT_IP_Var("WT IP", "withrottle ip etc. 192.168.1.111", WT_IP, 15);
    WiFiManagerParameter Port_Var("Port", "default is 12050", Port, 5);
    WiFiManagerParameter Slot_Var("Train Number", "Train Number like '3'", Slot, 5);
    WiFiManager wifiManager;
    failCount = 0;
    while (!wifiManager.autoConnect())
    {
        String Status = wifiManager.getWLStatusString();
#ifdef DEBUG
        Serial.println(Status);
#endif
        delay(1000);
        while (failCount >= 5)
        {
            // fail and halt here
        }
        failCount++;
    }
    IPAddress serverip;
    uint16_t port_number;
    // MDNS.begin("Test");
    MDNS.begin(String(R1) + " Controller");
    bool Connect_R;
    SleepCount = 0;
    while (Connect_R = !resolve_mdns_service("withrottle", "tcp", "192.168.137.1", &serverip, &port_number))
    {
        if (SleepCount == 10)
        {
#ifdef DEBUG
            Serial.println("get into deep sleep Mode");
#endif
            ESP.deepSleep(0);
            return;
        }
#ifndef DEBUG
        ToggleLED(RledP);
        ToggleLED(GledP);
#endif
        delay(1000);
    }
    SleepCount = 0;
    while (Connect_R = !client1.connect(serverip, port_number))
    {
        if (SleepCount == 10)
        {
#ifdef DEBUG
            Serial.println("get into deep sleep Mode");
#endif
            ESP.deepSleep(0);
            return;
        }
#ifdef DEBUG
        Serial.println("WiThrottle connection failed");
        Serial.println("After 1 sec Retry Next Time");
#else
        ToggleLED(RledP);
        ToggleLED(GledP);
#endif

        delay(1000);
    }
    status_TCP = true;

#ifdef DEGUG
    Serial.println("Client1 Connected to the server");
#endif
    WT.setDelegate(&Delegate);
    WT.connect(&client1);
    status_WiT = true;
#ifdef DEBUG
    Serial.println("WiThrottle connected");
#else
    ResetLed();
    digitalWrite(GledP, LOW);
#endif
    WT.setDeviceName(String(R1) + " Controller");
    WT.addLocomotive(R1);
    wifiManager.~WiFiManager();
}

void loop()
{
    if (!client1.connected())
    {
        IPAddress serverip;
        uint16_t port_number;
        bool Conncet_R;
        SleepCount = 0;
        while (Conncet_R = !resolve_mdns_service("withrottle", "tcp", "192.168.137.1", &serverip, &port_number))
        {
            if (SleepCount == 10)
            {
#ifdef DEBUG
                Serial.println("get into deep sleep Mode");
#endif
                ESP.deepSleep(0);
                return;
            }
#ifndef DEBUG
            ToggleLED(RledP);
            ToggleLED(GledP);
#endif
            delay(1000);
        }
        SleepCount = 0;
        while (Conncet_R = !client1.connect(serverip, port_number))
        {
            if (SleepCount == 10)
            {
#ifdef DEBUG
                Serial.println("get into deep sleep Mode");
#endif
                ESP.deepSleep(0);
                return;
            }
#ifndef DEBUG
            ToggleLED(RledP);
            ToggleLED(GledP);
#endif
            delay(1000);
        }
#ifndef DEBUG
        ResetLed();
        digitalWrite(GledP, LOW);
#endif
    }
    status_WiT = WT.check();
    Delegate.updateState();
}