# 1 "C:\\Users\\96\\Documents\\Arduino\\Kato_Remote_WT\\Kato_Remote_WT.ino"

// #define DEBUG
// #define TEST_CODE
# 16 "C:\\Users\\96\\Documents\\Arduino\\Kato_Remote_WT\\Kato_Remote_WT.ino"
# 17 "C:\\Users\\96\\Documents\\Arduino\\Kato_Remote_WT\\Kato_Remote_WT.ino" 2

// #ifdef ESP32
// #include <SPIFFS.h>
// #endif

# 23 "C:\\Users\\96\\Documents\\Arduino\\Kato_Remote_WT\\Kato_Remote_WT.ino" 2
# 24 "C:\\Users\\96\\Documents\\Arduino\\Kato_Remote_WT\\Kato_Remote_WT.ino" 2

# 26 "C:\\Users\\96\\Documents\\Arduino\\Kato_Remote_WT\\Kato_Remote_WT.ino" 2
# 27 "C:\\Users\\96\\Documents\\Arduino\\Kato_Remote_WT\\Kato_Remote_WT.ino" 2
# 28 "C:\\Users\\96\\Documents\\Arduino\\Kato_Remote_WT\\Kato_Remote_WT.ino" 2
// #define WM_DEBUG_LEVEL DEBUG_DEV
# 38 "C:\\Users\\96\\Documents\\Arduino\\Kato_Remote_WT\\Kato_Remote_WT.ino"
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
void saveConfigCallback()
{
    ;
    shouldSaveConfig = true;
    configM = false;
}
bool LedStatus[16];

void ToggleLED(int Pin)
{
    digitalWrite(Pin, !LedStatus[Pin]);
    LedStatus[Pin] = !LedStatus[Pin];
}
void ToggleLED(int Pin, u8 Val)
{
    analogWrite(Pin, !LedStatus[Pin] ? 255 : Val);
    LedStatus[Pin] = !LedStatus[Pin];
}
enum LedMode
{
    Error, // Error Red
    WarningBink, // warning blink yellow
    Warning, // warning yellow
    Go, // go blue
    Close, // Non
    Stop // Green
};
void SetLedMode(LedMode Mode);
LedMode LedState;
void ResetLed()
{
    digitalWrite(TX, 0x1);
    digitalWrite(RX, 0x1);
    digitalWrite(D0, 0x1);
}





void SetLedMode(LedMode Mode)
{
    ResetLed();
    switch (Mode)
    {
    case Error:
        digitalWrite(TX, 0x0);
        break;
    case WarningBink:
        ToggleLED(TX);
        ToggleLED(RX, 128);
        break;
    case Warning:
        digitalWrite(TX, 0x0);
        analogWrite(RX, 128);
        break;
    case Go:
        digitalWrite(D0, 0x0);
        break;
    case Close:
        break;
    case Stop:
        digitalWrite(RX, 0x0);
        break;
    }
}

char WT_IP[15] = "0.0.0.0";
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
        R1V = R1V_B * Ts + (1 - Ts) * analogRead(A0) / 8;
        if (!digitalRead(D1))
        {
            // Reverse
            T_dir = Reverse;
            P_status = 2;
        }
        else if (!digitalRead(D2))
        {
            // Forward
            T_dir = Forward;
            P_status = 1;
        }
        else if (!digitalRead(D6))
        { // stopped
            // init stop
            if (P_status != 0)
            {
                T_dir = _Stop;
                SetLedMode(Stop);
                FastSleepCount = millis();
                P_status = 0;
                TransmitRosterV();
            }
            if (R1V == 128)
            {
                SetLedMode(Warning);
                if (millis() - FastSleepCount > 10000)
                {
                    SetLedMode(Error);
                    ;
                    WT.disconnect();
                    // client1.abort();
                    client1.flush();
                    while (client1.read() != -1)
                        ;
                    client1.stop();
                    client1.~WiFiClient();
                    delay(500);
                    digitalWrite(D5, 0x1);
                    ;
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
        if (R1D != T_dir && T_dir != _Stop)
        {
            R1D = T_dir;
            TransmitRosterD();
            SetLedMode(Go);
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
        ;
        WT.setDirection(R1D);
    }
};

MyDelegate Delegate;
bool status_TCP = false;
bool status_WiT = false;

void setup()
{

    ;
    ;


    pinMode(TX, 0x01);
    pinMode(RX, 0x01);
    pinMode(D0, 0x01);

    SetLedMode(Warning);
    pinMode(D5, 0x01);
    digitalWrite(D5, 0x0);

    pinMode(D1, 0x02);
    pinMode(D2, 0x02);
    pinMode(D6, 0x02);
    // TIM_DIV256
    // TIM_EDGE
    pinMode(A0, 0x00);





    u8 Avalue = u8(analogRead(A0) / 8);
    if (Avalue > 60 && Avalue < 90)
        configM = true;

    u8 failCount = 0;
    // Config FS
    ;
    while (!SPIFFS.begin())
    {
        // FS Fail and halt here
        failCount++;
        SetLedMode(WarningBink);
        if (failCount > 10)
        {
            SetLedMode(Error);
            delay(2000);
            digitalWrite(D5, 0x1);
            ;
            ESP.deepSleep(0);
        };
        delay(500);
    }
    failCount = 0;
    ;

    ReadParam(WT_IP, Port, Slot);
    ;




    WiFiManager wifiManager;


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
        ;
        SetLedMode(WarningBink);
        delay(500);
        while (failCount >= 5)
        {
            SetLedMode(Error);
            delay(2000);
            ESP.deepSleep(0);
            // fail and halt here
        }
        failCount++;
    }
    // Set the finished param

    strcpy(WT_IP, WT_IP_Var.getValue());
    strcpy(Port, Port_Var.getValue());
    strcpy(Slot, Slot_Var.getValue());

    ;
    ;
    ;
    ;
    ;

    // configFile.
    IPAddress serverip(0, 0, 0, 0);
    uint16_t port_number = atoi(Slot);
    serverip.fromString(WT_IP);
    ;
    MDNS.begin(String(Slot) + " Controller");

    GetServer(&serverip, &port_number);
    String T_IP = serverip.toString();
    ;
    strcpy(WT_IP, T_IP.c_str());
    strcpy(Port, String(port_number).c_str());
    ;
    ;
    char T_IP_Char[20];
    strcpy(T_IP_Char, T_IP.c_str());
    SaveParam(T_IP_Char, Port, Slot);
    Connect_to_Server(&serverip, &port_number);

    status_TCP = true;

    ;
    WT.setDelegate(&Delegate);
    WT.connect(&client1);
    status_WiT = true;

    ;

    SetLedMode(Stop);
    strcpy(Slot, (String(atoi(Slot) > 127 ? 'L' : 'S') + String(Slot)).c_str());
    WT.setDeviceName(String(Slot) + " Controller");
    WT.addLocomotive(Slot);
}

void loop()
{
    // return;
    if (!client1.connected())
    {
        IPAddress serverip(0, 0, 0, 0);
        uint16_t port_number;
        ;
        GetServer(&serverip, &port_number);
        Connect_to_Server(&serverip, &port_number);

        ResetLed();
        digitalWrite(RX, 0x0);

    }
    status_WiT = WT.check();
    WT.checkHeartbeat();
    Delegate.updateState();
}
bool resolve_mdns_service(const char *service_name, const char *protocol, IPAddress *ip_addr, uint16_t *port_number)
{
    String Ori_IP = ip_addr->toString();
    ;
    ;
    int n = MDNS.queryService("withrottle", "tcp");
    if (n == 0)
    {
        ;
        return false;
    }

    for (int i = 0; i < n; ++i)
    {
        *ip_addr = MDNS.IP(i);
        *port_number = MDNS.port(i);
        if (Ori_IP == "0.0.0.0" || Ori_IP == ip_addr->toString())
            return true;
    }
    return false;
}

// Get from
bool GetServer(IPAddress *serverip, u16 *port_number)
{
    const char *T_SN = "withrottle";
    const char *Stype = "tcp";
    bool Connect_R;
    u8 SleepCount = 0;
    String A("aaa");
    //
    // String T_SN("w");
    ;
    while (Connect_R = !resolve_mdns_service(T_SN, Stype, serverip, port_number))
    {

        ;
        if (SleepCount == 5)
        {
            SetLedMode(Error);
            delay(2000);
            ;
            digitalWrite(D5, 0x1);
            ESP.deepSleep(0);
            return false;
        }
        SleepCount++;
        SetLedMode(WarningBink);
        delay(500);
    }
    ;
    ;
    return Connect_R;
}

bool Connect_to_Server(IPAddress *serverip, u16 *port_number)
{
    uint8_t failCount = 0;
    ;
    u8 SleepCount = 0;
    bool Connect_R;
    while (Connect_R = !client1.connect(*serverip, *port_number))
    {
        if (SleepCount == 10)
        {
            ;
            SetLedMode(Error);
            delay(2000);
            digitalWrite(D5, 0x1);
            ESP.deepSleep(0);
            return false;
        }
        ;
        ;
        SleepCount++;
        SetLedMode(WarningBink);

        delay(500);
    }
    ;
    return true;
}
bool ReadParam(char *_IP, char *Port, char *Slot)
{
    u8 failCount = 0;
    // TestCode
    // clean FS
    // SPIFFS.format();
    // Start

    ;
    // File T_File = SPIFFS.open("/config.json", "r");
    u8 TryCount = 0;
    while (!SPIFFS.open("/config.json", "r") && !SaveParam("0.0.0.0", "12090", "3"))
    {
        TryCount++;
        if (TryCount > 5)
            return false;
    }
    File configFile = SPIFFS.open("/config.json", "r");
    if (!configFile)
    {
        ;
        return false;
        while (1)
        {
            ;
            digitalWrite(D5, 0x1);
            ESP.deepSleep(0);
        }
    }

    ;
    size_t size = configFile.size();
    ;
    // Create content
    std::unique_ptr<char[]> buf(new char[size]);
    configFile.readBytes(buf.get(), size);

    // return;
    if (size != 0)
    {
        ;


        DynamicJsonDocument json(1024);
        auto deserializeError = deserializeJson(json, buf.get());
        serializeJson(json, Serial);
        if (!deserializeError)






        {
            ;
            strcpy(_IP, json["WT_IP"]);
            strcpy(Port, json["Port"]);
            strcpy(Slot, json["Slot"]);
        }
        else
        {
            ;
            return false;
            // format
        }
        configFile.close();
    }
    else
    {
        // call SaveParam and Save Default value
        // Add default setting to Json file
        configFile.close();
        ;
        SaveParam(_IP, Port, Slot);
    }
    return true;
}
bool SaveParam(char *_IP, char *Port, char *Slot)
{

    DynamicJsonDocument json(1024);





    json["WT_IP"] = _IP;
    json["Port"] = Port;
    json["Slot"] = Slot;
    File configFile = SPIFFS.open("/config.json", "w+");
    if (!configFile)
    {
        ;
        return false;
        // while (1)
        // {

        //     D_println("get into deep sleep Mode");
        //     digitalWrite(LockP, HIGH);
        //     ESP.deepSleep(0);
        // }
    }

    ;
    serializeJson(json, configFile);
    serializeJson(json, Serial);



    configFile.close();
    return true;
}

// /Z6XXJ8T
