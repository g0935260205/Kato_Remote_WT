# 1 "C:\\Users\\g0357\\Documents\\Arduino\\Kato_Remote_WT\\Kato_Remote_WT.ino"
# 15 "C:\\Users\\g0357\\Documents\\Arduino\\Kato_Remote_WT\\Kato_Remote_WT.ino"
# 16 "C:\\Users\\g0357\\Documents\\Arduino\\Kato_Remote_WT\\Kato_Remote_WT.ino" 2





# 22 "C:\\Users\\g0357\\Documents\\Arduino\\Kato_Remote_WT\\Kato_Remote_WT.ino" 2
# 23 "C:\\Users\\g0357\\Documents\\Arduino\\Kato_Remote_WT\\Kato_Remote_WT.ino" 2

# 25 "C:\\Users\\g0357\\Documents\\Arduino\\Kato_Remote_WT\\Kato_Remote_WT.ino" 2
# 26 "C:\\Users\\g0357\\Documents\\Arduino\\Kato_Remote_WT\\Kato_Remote_WT.ino" 2
# 27 "C:\\Users\\g0357\\Documents\\Arduino\\Kato_Remote_WT\\Kato_Remote_WT.ino" 2
# 39 "C:\\Users\\g0357\\Documents\\Arduino\\Kato_Remote_WT\\Kato_Remote_WT.ino"
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
                digitalWrite(D5, 0x1);
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
    digitalWrite(TX, 0x1);
    digitalWrite(RX, 0x1);
    digitalWrite(D0, 0x1);
}
void setup()
{

    // Serial.begin(115200);
    Serial.begin(115200);;
    Serial.println("Setting port");
# 188 "C:\\Users\\g0357\\Documents\\Arduino\\Kato_Remote_WT\\Kato_Remote_WT.ino"
    pinMode(D5, 0x01);
    digitalWrite(D5, 0x0);
    uint8_t failCount = 0;
    pinMode(D1, 0x02);
    pinMode(D2, 0x02);
    pinMode(D6, 0x02);

    pinMode(A0, 0x00);
    // Config FS

    // TestCode
    // clean FS
    // SPIFFS.format();
    // Start
    Serial.println("Setting FS");
    if (!SPIFFS.begin())
    {
        // FS Fail and halt here
        failCount++;
        while (failCount >= 0)
            ;
    }
    Serial.println("FS set");
    Serial.println("Setting File");
    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile)
    {
        Serial.println("File Fail");
        while (1)
        {
        }
    }

    Serial.println("File exist");
    size_t size = configFile.size();
    // Create content
    if (size != 0)
    {
        Serial.println(size);
        std::unique_ptr<char[]> buf(new char[size]);
        configFile.readBytes(buf.get(), size);
        // Serial.println("File Set");
        // Serial.println("Setting Json");

        // Serial.println("Setting Json Here");
        DynamicJsonDocument json(1024);
        // Serial.println("Setting Json Here");
        auto deserializeError = deserializeJson(json, buf.get());
        // Serial.println("Setting Json Here");
        serializeJson(json, Serial);
        // Serial.println("Setting Json Here");
        if (!deserializeError)
# 249 "C:\\Users\\g0357\\Documents\\Arduino\\Kato_Remote_WT\\Kato_Remote_WT.ino"
        {
            Serial.println("Setting Json Error");
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

    Serial.println("Setting param");
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
        Serial.println(Status);

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
    MDNS.begin(String("L9601") + " Controller");
    bool Connect_R;
    SleepCount = 0;
    Serial.println("get WT server");
    while (Connect_R = !resolve_mdns_service("withrottle", "tcp", "192.168.137.1", &serverip, &port_number))
    {
        Serial.println("Try get withrottle server");
        if (SleepCount == 10)
        {

            Serial.println("get into deep sleep Mode");
            ESP.deepSleep(0);
            return;
        }




        delay(500);
    }
    Serial.println("WT server get");
    Serial.println("Setting WT");
    SleepCount = 0;
    while (Connect_R = !client1.connect(serverip, port_number))
    {
        if (SleepCount == 10)
        {
            Serial.println("get into deep sleep Mode");
            ESP.deepSleep(0);
            return;
        }
        Serial.println("WiThrottle connection failed");
        Serial.println("After 1 sec Retry Next Time");





        delay(500);
    }
    Serial.println("WT Set");
    status_TCP = true;

    Serial.println("Client1 Connected to the server");
    WT.setDelegate(&Delegate);
    WT.connect(&client1);
    status_WiT = true;

    Serial.println("WiThrottle connected");




    WT.setDeviceName(String("L9601") + " Controller");
    WT.addLocomotive("L9601");
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
                Serial.println("get into deep sleep Mode");
                ESP.deepSleep(0);
                return;
            }




            delay(500);
        }
        SleepCount = 0;
        while (Conncet_R = !client1.connect(serverip, port_number))
        {
            if (SleepCount == 10)
            {
                Serial.println("get into deep sleep Mode");
                ESP.deepSleep(0);
                return;
            }




            delay(500);
        }




    }
    status_WiT = WT.check();
    WT.checkHeartbeat();
    // WT.heartbeatPeriod = 5;
    Delegate.updateState();
}
