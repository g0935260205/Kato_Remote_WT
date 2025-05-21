#include <Arduino.h>
#line 1 "C:\\Users\\g0357\\Documents\\Arduino\\Kato_Remote_WT\\Kato_Remote_WT.ino"

#define DEBUG
// #define TEST_CODE
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

// #ifdef ESP32
// #include <SPIFFS.h>
// #endif

#include <ArduinoJson.h>
#include <WiThrottle.h>

#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <ESP8266mDNS.h>
#define WM_DEBUG_LEVEL DEBUG_DEV
#define revrP D1
#define forwP D2
#define stopP D6
#define RledP TX
#define GledP RX
#define BledP D0
#define ThroP A0
#define LockP D5

// #define R1 "L9601"

//  Config
////// necessary
// ADDR: S1-S127/L128-9999
// WIFI_SSID
// WIFI_PSWD
////// option
// WT_IP:192.168.4.1
// WT_Port:12090

bool shouldSaveConfig = false;
bool configM = false;
#line 51 "C:\\Users\\g0357\\Documents\\Arduino\\Kato_Remote_WT\\Kato_Remote_WT.ino"
void saveConfigCallback();
#line 184 "C:\\Users\\g0357\\Documents\\Arduino\\Kato_Remote_WT\\Kato_Remote_WT.ino"
void ToggleLED(int Pin);
#line 189 "C:\\Users\\g0357\\Documents\\Arduino\\Kato_Remote_WT\\Kato_Remote_WT.ino"
void ResetLed();
#line 195 "C:\\Users\\g0357\\Documents\\Arduino\\Kato_Remote_WT\\Kato_Remote_WT.ino"
void setup();
#line 384 "C:\\Users\\g0357\\Documents\\Arduino\\Kato_Remote_WT\\Kato_Remote_WT.ino"
void loop();
#line 403 "C:\\Users\\g0357\\Documents\\Arduino\\Kato_Remote_WT\\Kato_Remote_WT.ino"
bool resolve_mdns_service(char *service_name, char *protocol, char *desired_host, IPAddress *ip_addr, uint16_t *port_number);
#line 419 "C:\\Users\\g0357\\Documents\\Arduino\\Kato_Remote_WT\\Kato_Remote_WT.ino"
bool GetServer(IPAddress *serverip, u16 *port_number);
#line 446 "C:\\Users\\g0357\\Documents\\Arduino\\Kato_Remote_WT\\Kato_Remote_WT.ino"
bool Connect_to_Server(IPAddress *serverip, u16 *port_number);
#line 473 "C:\\Users\\g0357\\Documents\\Arduino\\Kato_Remote_WT\\Kato_Remote_WT.ino"
void ReadParam(char *_IP, char *Port, char *Slot);
#line 539 "C:\\Users\\g0357\\Documents\\Arduino\\Kato_Remote_WT\\Kato_Remote_WT.ino"
void SaveParam(char *_IP, char *Port, char *Slot);
#line 51 "C:\\Users\\g0357\\Documents\\Arduino\\Kato_Remote_WT\\Kato_Remote_WT.ino"
void saveConfigCallback()
{
    D_println("Config_saved");
    shouldSaveConfig = true;
    configM = false;
}
enum LedMode
{
    Error,
    WarrningB,
    Warrning,
    Go,
    Close
} LedState;
char *WT_IP = "255.255.255.255";
char Port[5] = "0";
char Slot[5] = "3";
// 255.255.255.255

WiThrottle WT;
WiFiClient client1;
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
        Direction T_dir = R1D;
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
                TransmitRosterV();
            }
            if (R1V == 128)
            {
                if (millis() - FastSleepCount > 10000)
                {
                    D_println("going to Deep Sleep");
                    WT.disconnect();
                    // client1.abort();
                    client1.flush();
                    while (client1.read() != -1)
                        ;
                    client1.stop();
                    client1.~WiFiClient();
                    digitalWrite(LockP, HIGH);
                    delay(500);
                    D_println("init Deep Sleep complete");
                    pinMode(D8, OUTPUT);
                    digitalWrite(D8, LOW);
                    ESP.deepSleep(0);
                }
            }
            else
                FastSleepCount = millis();

            R1V_B = R1V;
            // if (R1V != 0)
            // {
            //     // float T_R1V = R1V;
            //     // R1V = 0;
            //     TransmitRosterV();
            //     // R1V = T_R1V;
            // }

            // return;
        }
        if (R1D != T_dir)
        {
            R1D = T_dir;
            TransmitRosterD();
        }

        // D_println(analogRead(ThroP));
        if ((int)R1V != (int)R1V_B)
        {
            // TransmitRosterV();
            // D_print("normal transmit");
            // D_println(R1V);
            // delay(50);
            R1V_B = R1V;
            // return;
        }
        TransmitRosterV();
        delay(100);
        // if (millis() - UpdateCount > 50)
        // {

        //     UpdateCount = millis();
        // }
        R1V_B = R1V;
    }
    void TransmitRosterV()
    {
        // D_println("transmit");
        WT.setSpeed(0, P_status != 0 ? (int)R1V > 126 ? 126 : (int)R1V : (int)0);
    }
    void TransmitRosterD()
    {
        WT.setDirection(R1D);
    }
};

MyDelegate Delegate;
bool status_TCP = false;
bool status_WiT = false;
bool LedStatus[16];

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

    pinMode(revrP, INPUT_PULLUP);
    pinMode(forwP, INPUT_PULLUP);
    pinMode(stopP, INPUT_PULLUP);
    // TIM_DIV256
    // TIM_EDGE
    pinMode(ThroP, INPUT);

#ifdef TEST_CODE
    if (!digitalRead(stopP))
        configM = true;
#else
    u8 Avalue = u8(analogRead(ThroP) / 8);
    if (Avalue > 60 && Avalue < 90)
        configM = true;
#endif
    u8 failCount = 0;
    // Config FS
    D_println("Setting FS");
    while (!SPIFFS.begin())
    {
        // FS Fail and halt here
        failCount++;
        while (failCount > 10)
        {
            digitalWrite(LockP, HIGH);
            D_println("get into deep sleep Mode");
            ESP.deepSleep(0);
        };
        delay(500);
    }
    failCount = 0;
    D_println("FS set");

    ReadParam(WT_IP, Port, Slot);
    D_println("Setting param");
#ifdef DEBUG
    WiFiManager wifiManager(Serial);
    wifiManager.setDebugOutput(true);
#else
    WiFiManager wifiManager;
#endif

    String Title = "<p>Setting WiThrottle Parameter</p>";
    Title += "last SSID is </br>";
    Title += wifiManager.getWiFiSSID() + "</br>";
    Title += "last PSWD is </br>";
    Title += wifiManager.getWiFiPass() + "</br>";

    // char *TitleC = Title.c_str();
    WiFiManagerParameter custom_text(Title.c_str());
    WiFiManagerParameter Slot_Var(
        "TrainNumber",
        "Train Number(Slot)*",
        Slot, 5,
        "inputmode = \"numeric\" placeholder =\"insert Slot ex:3\" required");
    WiFiManagerParameter WT_IP_Var(
        "WTIP",
        "withrottle IP(option) ",
        WT_IP, 15,
        "pattern='\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}' placeholder = \" EX:192.168.4.1\"");
    WiFiManagerParameter Port_Var(
        "Port",
        "Server Port(option)",
        Port, 5,
        "inputmode = \"numeric\" placeholder =\"JMRI:12090/DCCex:2560 \"");

    wifiManager.addParameter(&custom_text);
    wifiManager.setSaveConfigCallback(saveConfigCallback);
    wifiManager.setSaveParamsCallback(saveConfigCallback);
    wifiManager.addParameter(&Slot_Var);
    wifiManager.addParameter(&WT_IP_Var);
    wifiManager.addParameter(&Port_Var);

    // if (!digitalRead(stopP))
    //     wifiManager.erase();

    if (configM)
    {
        unsigned long SleepCounter = millis();
        bool initSleep = false;
        // std::vector<const char *> wmMenuItems = {"param", "exit"};
        // wifiManager.setMenu(wmMenuItems);
        // wifiManager.setBreakAfterConfig(true);
        wifiManager.setParamsPage(true);
        // wifiManager.configPortalActive = true;
        wifiManager.setConfigPortalBlocking(false);
        wifiManager.startConfigPortal();
        while (1)
        {
            wifiManager.process();
            // if (128 == analogRead(ThroP) / 8)
            // {
            //     if (millis() - SleepCounter > 10000)
            //     {
            //         D_println("get into deep sleep Mode");
            //         wifiManager.stopConfigPortal();
            //         digitalWrite(LockP, HIGH);
            //         ESP.deepSleep(0);
            //     }
            // }
            // else
            //     SleepCounter = millis();
            if (!configM)
            {
                wifiManager.stopConfigPortal();
                break;
            }
        }
    }
    configM = false;
    while (!configM && !wifiManager.autoConnect())
    {
        //     wifiManager.getWiFiSSID();
        //     wifiManager.getWiFiPass();
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

    D_println("Datasave");
    D_println(atoi(Slot));
    D_println(WT_IP);
    D_println(Port);
    D_println(Slot);

    // configFile.
    IPAddress serverip;
    uint16_t port_number = atoi(Slot);
    serverip.fromString(WT_IP);

    MDNS.begin(String(Slot) + " Controller");

    GetServer(&serverip, &port_number);
    String T_IP = serverip.toString();
    strcpy(WT_IP, T_IP.c_str());
    strcpy(Port, String(port_number).c_str());
    D_println(serverip.toString() + "End");
    D_println(T_IP);
    char T_IP_Char[20];
    strcpy(T_IP_Char, T_IP.c_str());
    SaveParam(T_IP_Char, Port, Slot);
    Connect_to_Server(&serverip, &port_number);

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
    strcpy(Slot, (String(atoi(Slot) > 127 ? 'L' : 'S') + String(Slot)).c_str());
    WT.setDeviceName(String(Slot) + " Controller");
    WT.addLocomotive(Slot);
}

void loop()
{
    // return;
    if (!client1.connected())
    {
        IPAddress serverip;
        uint16_t port_number;
        D_println("WT Disconnected");
        GetServer(&serverip, &port_number);
        Connect_to_Server(&serverip, &port_number);
#ifndef DEBUG
        ResetLed();
        digitalWrite(GledP, LOW);
#endif
    }
    status_WiT = WT.check();
    WT.checkHeartbeat();
    Delegate.updateState();
}
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

// Get from
bool GetServer(IPAddress *serverip, u16 *port_number)
{
    bool Connect_R;
    u8 SleepCount = 0;

    while (Connect_R = !resolve_mdns_service("withrottle", "tcp", "192.168.137.1", serverip, port_number))
    {
        D_println("Try get withrottle server");
        if (SleepCount == 10)
        {
            D_println("get into deep sleep Mode");
            digitalWrite(LockP, HIGH);
            ESP.deepSleep(0);
            return false;
        }
        SleepCount++;
#ifndef DEBUG
        ToggleLED(RledP);
        ToggleLED(GledP);
#endif
        delay(500);
    }
    D_println(serverip->toString());
    D_println("WT server get");
    return Connect_R;
}

bool Connect_to_Server(IPAddress *serverip, u16 *port_number)
{
    uint8_t failCount = 0;
    D_println("Setting WT");
    u8 SleepCount = 0;
    bool Connect_R;
    while (Connect_R = !client1.connect(*serverip, *port_number))
    {
        if (SleepCount == 10)
        {
            D_println("get into deep sleep Mode");
            digitalWrite(LockP, HIGH);
            ESP.deepSleep(0);
            return false;
        }
        D_println("WiThrottle connection failed");
        D_println("After 1 sec Retry Next Time");
        SleepCount++;
#ifndef DEBUG
        ToggleLED(RledP);
        ToggleLED(GledP);
#endif
        delay(500);
    }
    D_println("WT Set");
    return true;
}
void ReadParam(char *_IP, char *Port, char *Slot)
{
    u8 failCount = 0;
    // TestCode
    // clean FS
    // SPIFFS.format();
    // Start

    D_println("Setting File");
    File configFile = SPIFFS.open("/config.json", "r");
    if (!configFile)
    {
        D_println("File Fail");
        while (1)
        {
            D_println("get into deep sleep Mode");
            digitalWrite(LockP, HIGH);
            ESP.deepSleep(0);
        }
    }

    D_println("File exist");
    size_t size = configFile.size();
    D_println(size);
    // Create content
    std::unique_ptr<char[]> buf(new char[size]);
    configFile.readBytes(buf.get(), size);

    // return;
    if (size != 0)
    {
        D_println(size);

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
            D_println("Read");
            strcpy(_IP, json["WT_IP"]);
            strcpy(Port, json["Port"]);
            strcpy(Slot, json["Slot"]);
        }
        else
        {
            D_println("Setting Json Error");
            // format
        }
        configFile.close();
    }
    else
    {
        // call SaveParam and Save Default value
        // Add default setting to Json file
        configFile.close();
        D_println("Create");
        SaveParam(_IP, Port, Slot);
    }
}
void SaveParam(char *_IP, char *Port, char *Slot)
{
#if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
    DynamicJsonDocument json(1024);

#else
    DynamicJsonBuffer jsonBuffer;
    JsonObject &json = jsonBuffer.createObject();
#endif
    json["WT_IP"] = _IP;
    json["Port"] = Port;
    json["Slot"] = Slot;
    File configFile = SPIFFS.open("/config.json", "w+");
    if (!configFile)
    {
        D_println("File Fail");
        while (1)
        {
            D_println("get into deep sleep Mode");
            digitalWrite(LockP, HIGH);
            ESP.deepSleep(0);
        }
    }
#if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
    D_println("Write File");
    serializeJson(json, configFile);
    serializeJson(json, Serial);
#else
    json.printTo(configFile);
#endif
    configFile.close();
}

// void SetLedMode(LedMode Mode)
// {
//     // switch (Mode)
//     // {
//     // case Error:
//     //     /* code */
//     //     break;
//     // case default:

//     //     break;
//     // } b

// }
// /Z6XXJ8T
