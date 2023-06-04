// ******************************************LIBRARIES*********************************************

#include <SPI.h>
#include <Wire.h>
#include <OneWire.h>
#include <EEPROM.h>
#include <BH1750.h>
#include <DallasTemperature.h>
#include "SparkFunHTU21D.h"
#include "nRF24L01.h"
#include "RF24.h"
#include <iarduino_RTC.h>

// ************************************************************************************************

// ***************************WIRELESS SETTINGS****************************************************

#define CH_NUM 0x60 // chanel number (must be same on trasmitter and reciver)

// power level of the signal
#define SIG_POWER RF24_PA_LOW //RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
// SPEED
// enableAckPayload will not work on 250 kbps!
#define SIG_SPEED RF24_1MBPS    //RF24_2MBPS, RF24_1MBPS, RF24_250KBPS

// ************************************************************************************************

// ***************************PINS SETUP***********************************************************

// Valve digital pins
#define VALVE_1_PIN 2
#define VALVE_2_PIN 3
#define VALVE_3_PIN 255
#define VALVE_4_PIN 255
#define VALVE_5_PIN 255
#define VALVE_6_PIN 255

// buttom digital pin
#define BUTTON_PIN 4


// led digital pins
#define GREEN_LED_PIN 5
#define RED_LED_PIN 6

// one wire bus pin
#define ONE_WIRE_PIN 7

// analog pins
#define ANALOG_SOIL_PIN_1 A0
#define ANALOG_SOIL_PIN_2 A2
#define ANALOG_SOIL_PIN_3 A3
#define ANALOG_SOIL_PIN_4 A4
#define ANALOG_SOIL_PIN_5 A5
#define ANALOG_SOIL_PIN_6 A6

// wireless moudle pins
#define nRF_PIN_CE 48
#define nRF_PIN_CS 49

// ************************************************************************************************

// ***********************************SETTINGS CONSTANTS*******************************************

#define NO_OF_TEMPSENS 2        // number of temperature sensors on one wire bus

#define EEPROM_PARAM_SPACE 4    // space reserved for parameters in EEPROM

#define RECIEVE_SIZE 6          // wireless connection massage size reciving from master 
#define RESPONCE_SIZE 4         // wireless connection massage size respond to master

#define LOG_CHANK_SIZE 16       // size for every massage in EEPROM logging 

// ************************************************************************************************

// ************************************EEPROM ADDRESS**********************************************

#define SLAVE_ID 0              // byte
#define NUMBER_OF_PODS 1        // byte
#define HIS_MEM_POINTER 2       // int

// ************************************************************************************************

// ************************************variabules**************************************************

// wireless transmission 
byte pipeNo;
byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"}; // adresses 
uint8_t recieved_data[RECIEVE_SIZE];        // recived data from master  
byte response[RESPONCE_SIZE];               // data transmitted to master
RF24 radio(nRF_PIN_CE, nRF_PIN_CS);         // radio initiation 


uint8_t pc_responce[3] = {0, 0, 0};         // massage from PC via serial port 

// one wire
OneWire Bus(ONE_WIRE_PIN);                  // one wire bus initiation 
DallasTemperature TempSensors(&Bus);        // temperature sensors assigned to one wire 
DeviceAddress DevAdr;                       // one wire devices addresess 
float SoilTemp[NO_OF_TEMPSENS];             // data from temperature sensors 


// I2C
BH1750 LuxSensor;                           // light sensor                          
HTU21D AirTM;                               // air temperature/humidity sensor 
iarduino_RTC time(RTC_DS3231);              // real time clock 



// arrays with pins 
byte Pod_Pins[6];                           // valves control pins 
int analog_pins[6];                         // analog soil moisture pins 


// read data from sensors 
byte light_buf;
byte airtemp_buf[4];
byte airmoist_buf[4];
byte soiltemp_buf[4];
byte soilmoist_buf[2];

// ************************************************************************************************

// **********************************MAIN FUNCTIONS************************************************

void setup() {
    Serial.begin(9600);                     // PC communication
    
    // pins define 
    pinMode(VALVE_1_PIN, OUTPUT);
    pinMode(VALVE_2_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT);
    pinMode(GREEN_LED_PIN, OUTPUT);
    pinMode(RED_LED_PIN, OUTPUT);
   
    // I2C start
    LuxSensor.begin();                      // light
    AirTM.begin();                          // air
    time.begin();                           // RTC

    radioSetup();                           //radio startup
    
    
    // pins assign 
    Pod_Pins[0] = VALVE_1_PIN;
    Pod_Pins[1] = VALVE_2_PIN;
    Pod_Pins[2] = VALVE_3_PIN;
    Pod_Pins[3] = VALVE_4_PIN;
    Pod_Pins[4] = VALVE_5_PIN;
    Pod_Pins[5] = VALVE_6_PIN;

    analog_pins[0] = ANALOG_SOIL_PIN_1;
    analog_pins[1] = ANALOG_SOIL_PIN_2;
    analog_pins[2] = ANALOG_SOIL_PIN_3;
    analog_pins[3] = ANALOG_SOIL_PIN_4;
    analog_pins[4] = ANALOG_SOIL_PIN_5;
    analog_pins[5] = ANALOG_SOIL_PIN_6;

    // Green LED to indicate work
    TurnOn(GREEN_LED_PIN);
    
}


// main loop
void loop() {

    ListenToMaster();               // read command from master 
    

    // push button to go to PC communication mode 
    if (digitalRead(BUTTON_PIN) == 1)
    {  
        ReadPcMode();
    }
}


// ************************************************************************************************

// **********************************WORK FUNCTIONS************************************************


// split int vvalue into 2 bytes 
void int2bytes(int val, uint8_t* bytes_array) {
    
    union {
        int int_variable;
        uint16_t temp_array[2];
    } u;
    
    u.int_variable = val;
    
    memcpy(bytes_array, u.temp_array, 2);

}


void TurnOn(int pin){
    digitalWrite(pin, HIGH); 
  
}

void TurnOff(int pin){
    digitalWrite(pin, LOW); 
  
}


// ************************************************************************************************

// *******************************HARDWARE FUNCTIONS***********************************************

// closes all valves, except chosen one
void SetValve(uint8_t PodId){
    int i = 0;
    while (i < 6){
        if ((Pod_Pins[i] != 255) && (i != PodId)){
            TurnOn(Pod_Pins[i]);
        }
        ++i;

    }
    WriteHistoryWatering(PodId, 0);
}

// opens all valves 
void ResetValve(){
    int i = 0;
    while (i < 6){
        if (Pod_Pins[i] != 255){
            TurnOff(Pod_Pins[i]);
        }
        ++i;

    }

}

// updates pod assigned sensor values - soil temperature, soil moist.
// writes values to EEPROM log 
void PreparePodsSens(uint8_t pod_id){
    
    GetSoilTemp();

    
    float soiltemp = SoilTemp[pod_id];  
   
    int soilmoist = analogRead(analog_pins[pod_id]);
    
    byte * soiltemp_b = (byte *) &soiltemp;
    byte * soilmoist_b = (byte *) &soilmoist;
    for (int i = 0; i < 4; ++i)
    {
        soiltemp_buf[i] = soiltemp_b[i];
    }

    for (int i = 0; i < 2; ++i)
    {
        soilmoist_buf[i] = soilmoist_b[i];
    }
    WriteHistoryPodSens(pod_id, soiltemp, soilmoist); // log write 
  
}

// updates slave assigned sensor values - light intensity, air temperature, air humidity.
// writes values to EEPROM log 
void PrepareSlaveSens(){

    
    light_buf = LuxSensor.readLightLevel(); //1
    float airtemp = AirTM.readTemperature(); //4
    float airmoist = AirTM.readHumidity();  //4
    byte * airtemp_b = (byte *) &airtemp;
    byte * airmoist_b = (byte *) &airmoist;
    for (int i = 0; i < 4; ++i)
    {
        airtemp_buf[i] = airtemp_b[i];
    }

    for (int i = 0; i < 4; ++i)
    {
        airmoist_buf[i] = airmoist_b[i];
    }
    WriteHistorySalveSens(EEPROM.read(SLAVE_ID), light_buf, airtemp, airmoist);
}


// update soil temperature values 
void GetSoilTemp(){
    TempSensors.requestTemperatures();
  
    for (uint8_t i = 0; i < NO_OF_TEMPSENS; i++)
    {
        SoilTemp[i] = TempSensors.getTempCByIndex(i);
    }
    
}

// ************************************************************************************************

// *********************************RADIO COM FUNCTIONS********************************************

// prepare ok respond to master 
void OK_respond(){
    response[0] = 1;
    response[1] = 1;
    response[2] = 1;
    response[3] = 1;

}

// clear massage recieved from master 
void ClearRecieve(){
    int i = 0;
    while (i < RECIEVE_SIZE){
        recieved_data[i] = 0;
        ++i;
    
    }
  
}

// clear massage prepared for transmission
void ClearResponse(){
    int i = 0;
    while (i < RESPONCE_SIZE){
        response[i] = 0;
        ++i;
    
    }
  
}


// analyze massage from master and execute request 
void ListenToMaster(){
    int trig_s = 0;
    int trig_p = -1;
    while (radio.available(&pipeNo)) {                          // listen air
        radio.read( &recieved_data, sizeof(recieved_data));     // read massage
        switch (recieved_data[0]) {                             // first byte specify command 

            // prepare value for watering, closes all valves, except chosen one
            case 0:
                OK_respond();
                radio.writeAckPayload(pipeNo, &response, sizeof(response));
                SetValve(recieved_data[1]);
                ClearRecieve();

            break;

            // watering end, opens all valves 
            case 1:
                OK_respond();
                radio.writeAckPayload(pipeNo, &response, sizeof(response));
                ResetValve();
                ClearRecieve();
                
            break;

            // send slave sensors values 
            case 2:

                // second byte specifies request 

                if (recieved_data[1] == 0){         // update and save snesor values to memory 
                    OK_respond();
                    

                } else if(recieved_data[1] == 1){   // send light value 

                    response[0] = light_buf;

                } else if(recieved_data[1] == 2){   // send air temp. value 
                    for (int i = 0; i < 4; ++i)
                    {
                         response[i] = airtemp_buf[i];
                    }

                }else if(recieved_data[1] == 3){    // send air temp. humidity
                    for (int i = 0; i < 4; ++i)
                    {
                         response[i] = airmoist_buf[i];
                    }

                }

                //send prepared massage back 
                radio.writeAckPayload(pipeNo, &response, sizeof(response));

                // due to timeout OK massage must be sent before prepare opartation 
                if (recieved_data[1] == 0){
                    PrepareSlaveSens();
                }

                // clear massage buffer 
                ClearRecieve();
                
            break;

            // send pod sensors values 
            case 3:

                // second byte specifies request 


                if (recieved_data[1] == 0){             // update and save snesor values to memory 
                    OK_respond();
                } else if(recieved_data[1] == 1){       // send soil temperature

                    for (int i = 0; i < 4; ++i){

                        response[i] = soiltemp_buf[i];
                    }
                   

                } else if(recieved_data[1] == 2){       // send soil moisture 

                    for (int i = 0; i < 2; ++i){

                        response[i] = soilmoist_buf[i];
                    }
                   

                } 

                //send prepared massage back 
                radio.writeAckPayload(pipeNo, &response, sizeof(response));
                
                // due to timeout OK massage must be sent before prepare opartation 
                if (recieved_data[1] == 0){
                    PreparePodsSens(recieved_data[2]);
                }

                // clear massage buffer 
                ClearRecieve();
                
            break;

            // clear history log in EEPROM
            case 4:
                OK_respond();
                radio.writeAckPayload(pipeNo, &response, sizeof(response));
                ClearHis();
                ClearRecieve();
                
            break;


            // set time to RTC
            case 5:
                OK_respond();
                radio.writeAckPayload(pipeNo, &response, sizeof(response));

                time.settime(0, recieved_data[1], recieved_data[2], recieved_data[3],
                 recieved_data[4], recieved_data[5], 1);
                
                ClearRecieve();
                
            break;

         
        }
       
        // clear massage sent to master 
        ClearResponse();  

        
    }

}


// radio startup 
void radioSetup() {
    radio.begin();                  // activate module 
    radio.setAutoAck(1);            // auto ack
    radio.setRetries(0, 15);        // retries times (15)
    radio.enableAckPayload();       // enable posability to sent massages to master 
    radio.setPayloadSize(32);       // size of pack, 32 bytes 
    radio.openReadingPipe(1, address[EEPROM.read(SLAVE_ID)]); // set salve address 
    radio.setChannel(CH_NUM);               // channel set 
    radio.setPALevel(SIG_POWER);            // set power of signal 
    radio.setDataRate(SIG_SPEED);           // set speed 
                                            // must be same on master 
   
    radio.powerUp();                // satart work
    radio.startListening();         // start listening 
}


// ************************************************************************************************

// *************************************PC COMM FUNCTIONS******************************************

// PC massage mode 
void ReadPcMode(){
    int ledState = LOW;
    unsigned long previousMillis = 0;
    unsigned long currentMillis = 0;

    while(1){

        // read massage from PC
        ReadFromPc();

        // blinks every second with red led 
        currentMillis = millis();
        if (currentMillis - previousMillis >= 1000) {
            
            previousMillis = currentMillis;

            
            if (ledState == LOW) {
                ledState = HIGH;
            } else {
                ledState = LOW;
            }

            
            digitalWrite(RED_LED_PIN, ledState);
        }
    }
}

// send ack to PC
void SendAckPc(){
    uint8_t dataToSend[3] = {2, 55, 250};
    Serial.write(dataToSend, 3);

}

// send error to PC
void SendErrPc(){
    uint8_t dataToSend[3] = {2, 0, 250};
    Serial.write(dataToSend, 3);

}

// send EEPROM history pointer to PC
uint8_t SendHisPointPc(){
    uint8_t dataToSend[4];
    dataToSend[0] = 3;
    dataToSend[1] = EEPROM.read(HIS_MEM_POINTER);
    dataToSend[2] = EEPROM.read(HIS_MEM_POINTER + 1);
    dataToSend[3] = 250;
    Serial.write(dataToSend, 4);
}

// send EEPROM history massage to PC
uint8_t SendHisChankPc(int count){
    uint8_t dataToSend[LOG_CHANK_SIZE + 2];
    dataToSend[0] = LOG_CHANK_SIZE + 1;
    for (int i = 0; i < LOG_CHANK_SIZE; ++i)
    {
       dataToSend[i + 1] = EEPROM.read(EEPROM_PARAM_SPACE + (LOG_CHANK_SIZE * count) + i);
    }
   
    dataToSend[LOG_CHANK_SIZE + 1] = 250;
    Serial.write(dataToSend, LOG_CHANK_SIZE + 2);
}

// analyze massage from PC and execute request 
void ReadFromPc(){
   
    byte newmsg = 0;
    if(Serial.available() > 0){
        Serial.readBytes(pc_responce, 3);
        newmsg = 1;
    } 
    // if new massage avalible 
    if (newmsg == 1){

        switch(pc_responce[0]){          // first byte specify command 

            // configurate which pods are assigned to this slave, and change slave ID
            case 1:
                SendAckPc();
                SetConfigSalve(pc_responce[1], pc_responce[2]);
                while(1){
                    delay(1000);
                }
            break;

            // clear EEPROM history log 
            case 8:
                SendAckPc();
                ClearHis();
            break;

            // send EEPROM history log to PC
            case 13:
                if (pc_responce[1]==0){
                        SendHisPointPc();
                    } else if (pc_responce[1]==1){
                        
                        SendHisChankPc(pc_responce[2]);
                    }                    
            break;
            default:
                SendErrPc();
                    
            break;


        }
       
            
    }
  
}

// ************************************************************************************************

// ***********************************EEPROM FUNCTIONS*********************************************

// configurate which pods are assigned to this slave, and change slave ID 
void SetConfigSalve(uint8_t id, uint8_t n_pods){
    EEPROM.put(SLAVE_ID, id);
    EEPROM.put(NUMBER_OF_PODS, n_pods);
}

// clear EEPROM history log 
void ClearHis(){
    int dat = EEPROM_PARAM_SPACE;
    EEPROM.put(HIS_MEM_POINTER, dat);
}


// write daytime to EERPOM
void WriteHistoryTime(int position){
    time.gettime();                   
    uint8_t timedata[5] = {time.day, time.month, time.year, time.Hours, time.minutes};
    
    for (int i = 0; i < 5; ++i)
    {
        EEPROM.write(position + i, timedata[i]);
      
    }
    
}

// changes history pointer to new value 
void NewHisPointer(int new_position){

    // if overflows clear history, red led on to indicate 
    if (new_position > 4095){
        TurnOn(RED_LED_PIN);
        ClearHis();

    }else{

        EEPROM.put(HIS_MEM_POINTER, new_position);

    }
 
}
       
// write to log watreing event 
void WriteHistoryWatering(uint8_t pod_id, unsigned int water_amount){


    uint8_t type = 1;
    int pos;

    EEPROM.get(HIS_MEM_POINTER, pos);

    int new_position = pos + LOG_CHANK_SIZE;

    WriteHistoryTime(pos);
    pos = pos + 5;    

    EEPROM.put(pos, type);
    ++pos;                

    EEPROM.put(pos, pod_id + 1);
    ++pos;             

    EEPROM.put(pos, water_amount);

    NewHisPointer(new_position);
}

// write to log salve sensors 
void WriteHistorySalveSens(uint8_t slave_id, uint8_t light, float airtemp, float airmoist){


    uint8_t type = 3;
    int pos;

    EEPROM.get(HIS_MEM_POINTER, pos);

    int new_position = pos + LOG_CHANK_SIZE;

    WriteHistoryTime(pos);
    pos = pos + 5;       

    EEPROM.put(pos, type);
    ++pos;                

    EEPROM.put(pos, slave_id);
    ++pos;                

    EEPROM.put(pos, light);
    ++pos;                 

    EEPROM.put(pos, airtemp);
    pos = pos + 4;    

    EEPROM.put(pos, airmoist);
    
    NewHisPointer(new_position);
}

// write to log pod sensors 
void WriteHistoryPodSens(uint8_t pod_id, float soiltemp, int soilmoist){


    uint8_t type = 4;
    int pos;

    EEPROM.get(HIS_MEM_POINTER, pos);

    int new_position = pos + LOG_CHANK_SIZE;

    WriteHistoryTime(pos);
    pos = pos + 5;        

    EEPROM.put(pos, type);
    ++pos;                 

    EEPROM.put(pos, pod_id);
    ++pos;                         

    EEPROM.put(pos, soiltemp);
    pos = pos + 4;        
          
    EEPROM.put(pos, soilmoist);

    NewHisPointer(new_position);
}

// ************************************************************************************************
