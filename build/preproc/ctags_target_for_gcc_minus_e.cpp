# 1 "C:\\Users\\g0357\\Documents\\Arduino\\Kato_Remote_WT\\Kato_Remote_WT.ino"
// #define DEBUG
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
                    ;
                    WT.disconnect();
                    // client1.abort();
                    client1.flush();
                    while (client1.read() != -1)
                        ;
                    client1.stop();
                    client1.~WiFiClient();
                    digitalWrite(D5, 0x1);
                    delay(500);
                    ;
                    pinMode(D8, 0x01);
                    digitalWrite(D8, 0x0);
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
        delay(50);
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
    // return;
    // Serial.begin(115200);
    ;
    ;


    pinMode(TX, 0x01);
    pinMode(RX, 0x01);
    pinMode(D0, 0x01);

    ResetLed();

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
    ;
    if (!SPIFFS.begin())
    {
        // FS Fail and halt here
        failCount++;
        while (failCount >= 0)
            ;
    }
    ;
    ;
    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile)
    {
        ;
        while (1)
        {
        }
    }

    ;
    size_t size = configFile.size();
    // Create content
    if (size != 0)
    {
        ;
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
# 275 "C:\\Users\\g0357\\Documents\\Arduino\\Kato_Remote_WT\\Kato_Remote_WT.ino"
        {
            ;
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

    ;
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
        ;

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
    ;
    while (Connect_R = !resolve_mdns_service("withrottle", "tcp", "192.168.137.1", &serverip, &port_number))
    {
        ;
        if (SleepCount == 10)
        {

            ;
            ESP.deepSleep(0);
            return;
        }

        ToggleLED(TX);
        ToggleLED(RX);

        delay(500);
    }
    ;
    ;
    SleepCount = 0;
    while (Connect_R = !client1.connect(serverip, port_number))
    {
        if (SleepCount == 10)
        {
            ;
            ESP.deepSleep(0);
            return;
        }
        ;
        ;

        ToggleLED(TX);
        ToggleLED(RX);


        delay(500);
    }
    ;
    status_TCP = true;

    ;
    WT.setDelegate(&Delegate);
    WT.connect(&client1);
    status_WiT = true;

    ;

    ResetLed();
    digitalWrite(RX, 0x0);

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
                ;
                ESP.deepSleep(0);
                return;
            }

            ToggleLED(TX);
            ToggleLED(RX);

            delay(500);
        }
        SleepCount = 0;
        while (Conncet_R = !client1.connect(serverip, port_number))
        {
            if (SleepCount == 10)
            {
                ;
                ESP.deepSleep(0);
                return;
            }

            ToggleLED(TX);
            ToggleLED(RX);

            delay(500);
        }

        ResetLed();
        digitalWrite(RX, 0x0);

    }
    status_WiT = WT.check();
    WT.checkHeartbeat();
    // WT.heartbeatPeriod = 5;
    Delegate.updateState();
}
