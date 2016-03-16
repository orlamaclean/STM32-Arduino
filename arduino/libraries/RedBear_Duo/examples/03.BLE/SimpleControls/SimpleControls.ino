/******************************************************
 *Download RedBear "BLE Controller" APP 
 *From APP store(IOS)/Play Store(Android)                                         
 ******************************************************/
/******************************************************
 *                      Macros
 ******************************************************/
#if defined(ARDUINO) 
SYSTEM_MODE(MANUAL);//do not connect to cloud
#else
SYSTEM_MODE(AUTOMATIC);//connect to cloud
#endif

#define DEVICE_NAME                "Simple Controls"

#define CHARACTERISTIC1_MAX_LEN    3
#define CHARACTERISTIC2_MAX_LEN    3

#define DIGITAL_OUT_PIN            D2
#define DIGITAL_IN_PIN             A4
#define PWM_PIN                    D3
#define SERVO_PIN                  D4
#define ANALOG_IN_PIN              A5

Servo                             myservo;

/******************************************************
 *               Variable Definitions
 ******************************************************/
static uint8_t service1_uuid[16]       ={0x71,0x3d,0x00,0x00,0x50,0x3e,0x4c,0x75,0xba,0x94,0x31,0x48,0xf1,0x8d,0x94,0x1e};
static uint8_t service1_tx_uuid[16]    ={0x71,0x3d,0x00,0x03,0x50,0x3e,0x4c,0x75,0xba,0x94,0x31,0x48,0xf1,0x8d,0x94,0x1e};
static uint8_t service1_rx_uuid[16]    ={0x71,0x3d,0x00,0x02,0x50,0x3e,0x4c,0x75,0xba,0x94,0x31,0x48,0xf1,0x8d,0x94,0x1e};

static uint8_t  appearance[2]    = {0x00, 0x02};
static uint8_t  change[2]        = {0x00, 0x00};
static uint8_t  conn_param[8]    = {0x28, 0x00, 0x90, 0x01, 0x00, 0x00, 0x90, 0x01};

static uint16_t character1_handle = 0x0000;
static uint16_t character2_handle = 0x0000;
static uint16_t character3_handle = 0x0000;

static uint8_t characteristic1_data[CHARACTERISTIC1_MAX_LEN]={0x01};
static uint8_t characteristic2_data[CHARACTERISTIC2_MAX_LEN]={0x00};

static btstack_timer_source_t characteristic2;

static advParams_t adv_params;
static uint8_t adv_data[]={0x02,0x01,0x06,0x08,0x08,'B','i','s','c','u','i','t',0x11,0x07,0x1e,0x94,0x8d,0xf1,0x48,0x31,0x94,0xba,0x75,0x4c,0x3e,0x50,0x00,0x00,0x3d,0x71};

static boolean analog_enabled = false;
static byte old_state         = LOW;

 /******************************************************
 *               Function Definitions
 ******************************************************/

 
void deviceConnectedCallback(BLEStatus_t status, uint16_t handle) {
    switch (status){
        case BLE_STATUS_OK:
            Serial.println("Device connected!");
            break;
        default:
            break;
    }
}

void deviceDisconnectedCallback(uint16_t handle){
    Serial.println("Disconnected.");
}

int gattWriteCallback(uint16_t value_handle, uint8_t *buffer, uint16_t size)
{
    Serial.print("Write value handler: ");
    Serial.println(value_handle, HEX);

    if(character1_handle == value_handle)
    {
        memcpy(characteristic1_data, buffer, CHARACTERISTIC1_MAX_LEN);
        Serial.print("Characteristic1 write value: ");
        for(uint8_t index=0; index<CHARACTERISTIC1_MAX_LEN; index++)
        {
            Serial.print(characteristic1_data[index], HEX);
            Serial.print(" ");
        }
        Serial.println(" ");
        //Process the data
        if (characteristic1_data[0] == 0x01)  // Command is to control digital out pin
        {
            if (characteristic1_data[1] == 0x01)
                digitalWrite(DIGITAL_OUT_PIN, HIGH);
            else
                digitalWrite(DIGITAL_OUT_PIN, LOW);
        }
        else if (characteristic1_data[0] == 0xA0) // Command is to enable analog in reading
        {
            if (characteristic1_data[1] == 0x01)
                analog_enabled = true;
            else
                analog_enabled = false;
        }
        else if (characteristic1_data[0] == 0x02) // Command is to control PWM pin
        {
            analogWrite(PWM_PIN, characteristic1_data[1]);
        }
        else if (characteristic1_data[0] == 0x03)  // Command is to control Servo pin
        {
            myservo.write(characteristic1_data[1]);
        }
        else if (characteristic1_data[0] == 0x04)
        {
            analog_enabled = false;
            myservo.write(0);
            analogWrite(PWM_PIN, 0);
            digitalWrite(DIGITAL_OUT_PIN, LOW);
            old_state = LOW;
        }
    }
    return 0;
}

static void  characteristic2_notify(btstack_timer_source_t *ts)
{
    if (analog_enabled)  // if analog reading enabled
    {
        //Serial.println("characteristic2_notify analog reading ");
        // Read and send out
        uint16_t value = analogRead(ANALOG_IN_PIN);
        characteristic2_data[0] = (0x0B);
        characteristic2_data[1] = (value >> 8);
        characteristic2_data[2] = (value);
        if(ble.attServerCanSendPacket())
            ble.sendNotify(character2_handle, characteristic2_data, CHARACTERISTIC2_MAX_LEN);
    }
    // If digital in changes, report the state
    if (digitalRead(DIGITAL_IN_PIN) != old_state)
    {
        Serial.println("characteristic2_notify digital reading ");
        old_state = digitalRead(DIGITAL_IN_PIN);
        if (digitalRead(DIGITAL_IN_PIN) == HIGH)
        {
            characteristic2_data[0] = (0x0A);
            characteristic2_data[1] = (0x01);
            characteristic2_data[2] = (0x00);
            ble.sendNotify(character2_handle, characteristic2_data, CHARACTERISTIC2_MAX_LEN);
        }
        else
        {
            characteristic2_data[0] = (0x0A);
            characteristic2_data[1] = (0x00);
            characteristic2_data[2] = (0x00);
            ble.sendNotify(character2_handle, characteristic2_data, CHARACTERISTIC2_MAX_LEN);
        }
    }
    // reset
    ble.setTimer(ts, 200);
    ble.addTimer(ts);
}

void setup()
{
    Serial.begin(115200);
    delay(5000);
    Serial.println("Simple Controls demo.");
    //ble.debugLogger(true);
    ble.init();

    ble.onConnectedCallback(deviceConnectedCallback);
    ble.onDisconnectedCallback(deviceDisconnectedCallback);
    ble.onDataWriteCallback(gattWriteCallback);

    ble.addService(0x1800);
    ble.addCharacteristic(0x2A00, ATT_PROPERTY_READ|ATT_PROPERTY_WRITE, (uint8_t*)DEVICE_NAME, sizeof(DEVICE_NAME));
    ble.addCharacteristic(0x2A01, ATT_PROPERTY_READ, appearance, sizeof(appearance));
    ble.addCharacteristic(0x2A04, ATT_PROPERTY_READ, conn_param, sizeof(conn_param));
    ble.addService(0x1801);
    ble.addCharacteristic(0x2A05, ATT_PROPERTY_INDICATE, change, sizeof(change));

    ble.addService(service1_uuid);
    character1_handle = ble.addCharacteristicDynamic(service1_tx_uuid, ATT_PROPERTY_NOTIFY|ATT_PROPERTY_WRITE|ATT_PROPERTY_WRITE_WITHOUT_RESPONSE, characteristic1_data, CHARACTERISTIC1_MAX_LEN);
    character2_handle = ble.addCharacteristicDynamic(service1_rx_uuid, ATT_PROPERTY_NOTIFY, characteristic2_data, CHARACTERISTIC2_MAX_LEN);
    
    adv_params.adv_int_min = 0x00A0;
    adv_params.adv_int_max = 0x01A0;
    adv_params.adv_type    = 0;
    adv_params.dir_addr_type = 0;
    memset(adv_params.dir_addr,0,6);
    adv_params.channel_map = 0x07;
    adv_params.filter_policy = 0x00;
    
    ble.setAdvParams(&adv_params);
    
    ble.setAdvData(sizeof(adv_data), adv_data);

    ble.startAdvertising();

    pinMode(DIGITAL_OUT_PIN, OUTPUT);
    pinMode(DIGITAL_IN_PIN, INPUT_PULLUP);
    pinMode(PWM_PIN, OUTPUT);

    // Default to internally pull high, change it if you need
    digitalWrite(DIGITAL_IN_PIN, HIGH);

    myservo.attach(SERVO_PIN);
    myservo.write(0);

    // set one-shot timer
    characteristic2.process = &characteristic2_notify;
    ble.setTimer(&characteristic2, 500);//2000ms
    ble.addTimer(&characteristic2);
    
    Serial.println("BLE start advertising.");
}

void loop()
{
    
}