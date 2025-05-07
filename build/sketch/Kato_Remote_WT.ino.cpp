#include <Arduino.h>
#line 1 "C:\\Users\\g0357\\Documents\\Arduino\\Kato_Remote_WT\\Kato_Remote_WT.ino"
#define DEBUG

#ifdef DEBUG
#define D_begin(...) Serial.begin(__VA_ARGS__);
#define D_print(...) Serial.print(__VA_ARGS__)
#define D_write(...) Serial.print(__VA_ARGS__)
#define D_println(...) Serial.println(__VA_ARGS__)
#else
#define D_begin(...)
#define D_print(...)
#define D_write(...)
#define D_println(...)
#endif

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
#define LockP D5

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
#line 49 "C:\\Users\\g0357\\Documents\\Arduino\\Kato_Remote_WT\\Kato_Remote_WT.ino"
void saveConfigCallback();
#line 142 "C:\\Users\\g0357\\Documents\\Arduino\\Kato_Remote_WT\\Kato_Remote_WT.ino"
bool resolve_mdns_service(char *service_name, char *protocol, char *desired_host, IPAddress *ip_addr, uint16_t *port_number);
#line 163 "C:\\Users\\g0357\\Documents\\Arduino\\Kato_Remote_WT\\Kato_Remote_WT.ino"
void ToggleLED(int Pin);
#line 168 "C:\\Users\\g0357\\Documents\\Arduino\\Kato_Remote_WT\\Kato_Remote_WT.ino"
void ResetLed();
#line 174 "C:\\Users\\g0357\\Documents\\Arduino\\Kato_Remote_WT\\Kato_Remote_WT.ino"
void setup();
#line 356 "C:\\Users\\g0357\\Documents\\Arduino\\Kato_Remote_WT\\Kato_Remote_WT.ino"
void loop();
#line 49 "C:\\Users\\g0357\\Documents\\Arduino\\Kato_Remote_WT\\Kato_Remote_WT.ino"
void saveConfigCallback()
{

    shouldSaveConfig = true;
}

char WT_IP[15] = "192.168.4.1";
char Port[5] = "2560";
char Slot[5] = "S3";
// 255.255.255.255

WiThrottle WT;
class MyDelegate : public WiThrottleDelegate
{
public:
    float R1V = 0;
    float R1V_B = 0;
    const float Ts = 0.4;
    Direction R1D = Reverse;
    bool PState = false;
    unsigned long FastSleepCount = 0;
    unsigned long UpdateCount = 0;
    // 1 forward 2 backward 0 stop
    uint8_t P_status = 0;
    void updateState()
    {
        Direction T_dir;
        R1V = R1V_B * Ts + (1 - Ts) * analogRead(ThroP) / 8;
        if (!digitalRead(revrP))
        {
            // Reverse
            T_dir = Reverse;
            P_status = 2;
        }
        else if (!digitalRead(forwP))
        {
            // Forward
            T_dir = Forward;
            P_status = 1;
        }
        else
        { // stopped
            // init stop
            if (P_status != 0)
            {
                FastSleepCount = millis();
                P_status = 0;
            }
            if (millis() - FastSleepCount > 10000 && (R1V > 60 && R1V < 100))
            {
                digitalWrite(LockP, HIGH);
                ESP.deepSleep(0);
            }
            R1V_B = R1V;
            if (R1V != 0)
            {
                R1V = 0;
                TransmitRosterV();
            }

            // return;
        }
        if (R1D != T_dir)
        {
            R1D = T_dir;
            TransmitRosterD();
        }

        // D_println(analogRead(ThroP));
        if (R1V != R1V_B)
        {
            TransmitRosterV();
            delay(50);
            R1V_B = R1V;
            return;
        }
        if (millis() - UpdateCount > 1000)
        {
            TransmitRosterV();
            UpdateCount = millis();
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

    // Serial.begin(115200);
    D_begin(115200);
    D_println("Setting port");

#ifndef DEBUG
    pinMode(RledP, OUTPUT);
    pinMode(GledP, OUTPUT);
    pinMode(BledP, OUTPUT);

    ResetLed();
#endif
    pinMode(LockP, OUTPUT);
    digitalWrite(LockP, LOW);
    uint8_t failCount = 0;
    pinMode(revrP, INPUT_PULLUP);
    pinMode(forwP, INPUT_PULLUP);
    pinMode(stopP, INPUT_PULLUP);

    pinMode(ThroP, INPUT);
    // Config FS

    // TestCode
    // clean FS
    // SPIFFS.format();
    // Start
    D_println("Setting FS");
    if (!SPIFFS.begin())
    {
        // FS Fail and halt here
        failCount++;
        while (failCount >= 0)
            ;
    }
    D_println("FS set");
    D_println("Setting File");
    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile)
    {
        D_println("File Fail");
        while (1)
        {
        }
    }

    D_println("File exist");
    size_t size = configFile.size();
    // Create content
    if (size != 0)
    {
        D_println(size);
        std::unique_ptr<char[]> buf(new char[size]);
        configFile.readBytes(buf.get(), size);
        // Serial.println("File Set");
        // Serial.println("Setting Json");
#if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
        // Serial.println("Setting Json Here");
        DynamicJsonDocument json(1024);
        // Serial.println("Setting Json Here");
        auto deserializeError = deserializeJson(json, buf.get());
        // Serial.println("Setting Json Here");
        serializeJson(json, Serial);
        // Serial.println("Setting Json Here");
        if (!deserializeError)

#else
        // Serial.println("Setting Json Here1");
        DynamicJsonBuffer jsonBuffer;
        JsonObject &json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success())

#endif
        {
            D_println("Setting Json Error");
        }
    }
    else
    {
    }

    // return;
    // if (configFile == NULL)
    // {
    //     failCount++;
    //     while (failCount >= 0)
    //         ;
    // }

    D_println("Setting param");
    WiFiManagerParameter WT_IP_Var("WTIP", "withrottle ip etc. 192.168.1.111", WT_IP, 15);
    WiFiManagerParameter Port_Var("Port", "default is 12090", Port, 5);
    WiFiManagerParameter Slot_Var("TrainNumber", "Train Number like '3'", Slot, 5);
    WiFiManager wifiManager;
    wifiManager.setSaveConfigCallback(saveConfigCallback);
    wifiManager.addParameter(&WT_IP_Var);
    wifiManager.addParameter(&Port_Var);
    wifiManager.addParameter(&Slot_Var);
    failCount = 0;
    while (!wifiManager.autoConnect())
    {
        String Status = wifiManager.getWLStatusString();
        D_println(Status);

        delay(500);
        while (failCount >= 5)
        {
            // fail and halt here
        }
        failCount++;
    }
    // Set the finished param
    // if ()
    // {
    // }
    strcpy(WT_IP, WT_IP_Var.getValue());
    strcpy(Port, Port_Var.getValue());
    strcpy(Slot, Slot_Var.getValue());

    IPAddress serverip;
    uint16_t port_number;
    // MDNS.begin("Test");
    MDNS.begin(String(R1) + " Controller");
    bool Connect_R;
    SleepCount = 0;
    D_println("get WT server");
    while (Connect_R = !resolve_mdns_service("withrottle", "tcp", "192.168.137.1", &serverip, &port_number))
    {
        D_println("Try get withrottle server");
        if (SleepCount == 10)
        {

            D_println("get into deep sleep Mode");
            ESP.deepSleep(0);
            return;
        }
#ifndef DEBUG
        ToggleLED(RledP);
        ToggleLED(GledP);
#endif
        delay(500);
    }
    D_println("WT server get");
    D_println("Setting WT");
    SleepCount = 0;
    while (Connect_R = !client1.connect(serverip, port_number))
    {
        if (SleepCount == 10)
        {
            D_println("get into deep sleep Mode");
            ESP.deepSleep(0);
            return;
        }
        D_println("WiThrottle connection failed");
        D_println("After 1 sec Retry Next Time");
#ifndef DEBUG
        ToggleLED(RledP);
        ToggleLED(GledP);
#endif

        delay(500);
    }
    D_println("WT Set");
    status_TCP = true;

    D_println("Client1 Connected to the server");
    WT.setDelegate(&Delegate);
    WT.connect(&client1);
    status_WiT = true;

    D_println("WiThrottle connected");
#ifndef DEBUG
    ResetLed();
    digitalWrite(GledP, LOW);
#endif
    WT.setDeviceName(String(R1) + " Controller");
    WT.addLocomotive(R1);
    // wifiManager.~WiFiManager();
}

void loop()
{
    // return;
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
                D_println("get into deep sleep Mode");
                ESP.deepSleep(0);
                return;
            }
#ifndef DEBUG
            ToggleLED(RledP);
            ToggleLED(GledP);
#endif
            delay(500);
        }
        SleepCount = 0;
        while (Conncet_R = !client1.connect(serverip, port_number))
        {
            if (SleepCount == 10)
            {
                D_println("get into deep sleep Mode");
                ESP.deepSleep(0);
                return;
            }
#ifndef DEBUG
            ToggleLED(RledP);
            ToggleLED(GledP);
#endif
            delay(500);
        }
#ifndef DEBUG
        ResetLed();
        digitalWrite(GledP, LOW);
#endif
    }
    status_WiT = WT.check();
    WT.checkHeartbeat();
    // WT.heartbeatPeriod = 5;
    Delegate.updateState();
}
