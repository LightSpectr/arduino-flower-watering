// ***************************LIBRARIES***********************************************************
#include <SPI.h>
#include <EEPROM.h>
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
#define SIG_SPEED RF24_1MBPS //RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
// ************************************************************************************************

// ***********************************PINS SETUP***************************************************
#define PUMP_PIN 2              // pump pin

#define BUTTON_PIN 3            // button pin 

// led pins
#define GREEN_LED_PIN 4       
#define RED_LED_PIN 5

// ultrasonic sensor pins
#define SR04_TRIG_PIN 6         //trigger pin
#define SR04_ECHO_PIN 7         //echo pin

// wireless moudle pins
#define nRF_PIN_CE 48
#define nRF_PIN_CS 49
// ************************************************************************************************

// ***********************************SETTINGS CONSTANTS*******************************************
#define EEPROM_PARAM_SPACE 44       // space reserved for parameters in EEPROM

#define MSG_SIZE 6                  // wireless connection massage size sent to slave
#define RESPONCE_SIZE 4             // wireless connection massage size responded from slave

#define LOG_CHANK_SIZE 16           // size for every massage in EEPROM logging 
// ************************************************************************************************

// ***********************************EEPROM ADDRESS***********************************************
#define LIM_DIS_SUPPLY 0            //  distance from to to bottom of supply water tank, byte
#define LOCKOUT 1                   // lockout number, byte 
#define PUMP_CONST 2                // pump constant, byte

#define NUMBER_OF_PODS 3            // number of available pods 
#define NUMBER_OF_SLAVES 4          // number of available slaves 

#define TIME_CD_POD_1 5             // cooldown time for pod 1 in hours, byte
#define TIME_CD_POD_2 6             // cooldown time for pod 2 in hours, byte
#define TIME_CD_POD_3 7             // cooldown time for pod 3 in hours, byte
#define TIME_CD_POD_4 8             // cooldown time for pod 4 in hours, byte
#define TIME_CD_POD_5 9             // cooldown time for pod 5 in hours, byte
#define TIME_CD_POD_6 10            // cooldown time for pod 6 in hours, byte

#define WATER_AMOUNT_POD_1 11       // water amount in ml needed for pod 1 per one shot, int
#define WATER_AMOUNT_POD_2 13       // water amount in ml needed for pod 2 per one shot, int
#define WATER_AMOUNT_POD_3 15       // water amount in ml needed for pod 3 per one shot, int
#define WATER_AMOUNT_POD_4 17       // water amount in ml needed for pod 4 per one shot, int
#define WATER_AMOUNT_POD_5 19       // water amount in ml needed for pod 5 per one shot, int
#define WATER_AMOUNT_POD_6 21       // water amount in ml needed for pod 6 per one shot, int

#define LOW_MOIST_LEVEL_POD_1 23    // minimum soil moisture level for pod 1, int
#define LOW_MOIST_LEVEL_POD_2 25    // minimum soil moisture level for pod 2, int
#define LOW_MOIST_LEVEL_POD_3 27    // minimum soil moisture level for pod 3, int
#define LOW_MOIST_LEVEL_POD_4 29    // minimum soil moisture level for pod 4, int
#define LOW_MOIST_LEVEL_POD_5 31    // minimum soil moisture level for pod 5, int
#define LOW_MOIST_LEVEL_POD_6 33    // minimum soil moisture level for pod 6, int

#define POD_1_SLAVE_ID 35           // slave id which pod 1 is assigned, byte
#define POD_2_SLAVE_ID 36           // slave id which pod 2 is assigned, byte
#define POD_3_SLAVE_ID 37           // slave id which pod 3 is assigned, byte
#define POD_4_SLAVE_ID 38           // slave id which pod 4 is assigned, byte
#define POD_5_SLAVE_ID 39           // slave id which pod 5 is assigned, byte
#define POD_6_SLAVE_ID 40           // slave id which pod 6 is assigned, byte

#define WATER_MODE 41               // water mode, 0 - no automation, 1 - cd mode, 2 daytime mode
#define HIS_MEM_POINTER 42          // pointer for empty space in EEPROM, int
// ************************************************************************************************

// *****************************************VARIABLES**********************************************
// wireless transmission 
byte address[][6] = {"1Node", "2Node", "3Node",
                     "4Node", "5Node", "6Node"}; // возможные номера труб
uint8_t massage[MSG_SIZE];          // data transmitted to salve
byte responce[RESPONCE_SIZE];       // recived fromsalve
RF24 radio(nRF_PIN_CE, nRF_PIN_CS); // radio initiation 



iarduino_RTC time(RTC_DS3231);      // RTC initiation 

uint8_t pc_responce[7] = {0, 0, 0, 0, 0, 0, 0};
uint8_t data_buffer[9];


// sensor values 
float soiltemp_buf;
unsigned int soilmoist_buf[6];
uint8_t light_buf;
float airtemp_buf;
float airmoist_buf;
long dis;

// flags for water fail detection 
unsigned int water_flag_moist[6] = {0, 0, 0, 0, 0, 0};
unsigned long water_flag_time[6] = {0, 0, 0, 0, 0, 0};

// for millis value 
unsigned long pod_cd_time[6];

// daytime flag
unsigned int daytime_flag[6] = {0, 0, 0, 0, 0, 0};
unsigned int current_day;

// for snesor check millis value 
unsigned long previousMillis = 0;

// ************************************************************************************************

// **********************************MAIN FUNCTIONS************************************************

void setup() {
    Serial.begin(9600);                 // PC communication
    
    radioSetup();                       //radio startup

    // pins define 
    pinMode(PUMP_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT);
    pinMode(GREEN_LED_PIN, OUTPUT);
    pinMode(RED_LED_PIN, OUTPUT);
    pinMode(SR04_TRIG_PIN, OUTPUT);
    pinMode(SR04_ECHO_PIN, INPUT);

    // RTC
    time.begin();

    // for snesor check millis value
    previousMillis = millis();

    // water fail detection  millis value
    for (int i = 0; i < 6; ++i)
    {
        pod_cd_time[i] = previousMillis;
    }

    time.gettime();
    current_day = time.day;
    // Green LED to indicate work
    TurnOn(GREEN_LED_PIN);
    
}

// main loop
void loop() {
    // update datetime
    time.gettime();

    // check lockout 
    if (EEPROM.read(LOCKOUT) != 0){
        TurnOff(GREEN_LED_PIN);
        Blink(RED_LED_PIN, EEPROM.read(LOCKOUT));   // blinks indicates lockout number
        TurnOn(GREEN_LED_PIN);
        delay(500);
        if (digitalRead(BUTTON_PIN)==1){            // reset lockout 
            TurnOn(RED_LED_PIN);
            SetLockout(0);
            delay(2000);
            TurnOff(RED_LED_PIN);
            
        }

    }
    else{       // no lockout

        // check supply water amount, set lockout 1 if there is lack of water 
        if (TankCheck()==1){

            SetLockout(1);
        }
        
        // check mode 
        switch(EEPROM.read(WATER_MODE)){
            
            case 1: 
                
                // water plant with settime interval
                CoolDownMode();
            break;
            case 2: 

                // water plant based on day time, and date 
                DaytimeMode();
                
            break;
        }
    }

    // if plant was watered checks that soil moisture has changed
    // to ensure that pipe has no faliure 
    WaterFailCheck();

    // update sensors values every hour 
    // previousMillis = UpdateSensors(previousMillis);

    // check if day changes 
    CheckDay();

    // read massage from PC
    ReadFromPc();
   
}

// ************************************************************************************************

// **********************************WORK FUNCTIONS************************************************


// water plant with settime interval
void CoolDownMode(){
    unsigned long current_time = millis();
    for (int i = 0; i < 6; ++i)
    {   

        // slave 255 = pod is disabled 
        if (EEPROM.read(POD_1_SLAVE_ID + i) != 255)
        {   
            // check time interval from memmory 
            if (current_time - pod_cd_time[i] >= 3600000 * EEPROM.read(TIME_CD_POD_1 + i)){

                // check moisture level 
                ReadPodsSensors(i, data_buffer);
                unsigned int min_soil_moist;
                EEPROM.get(LOW_MOIST_LEVEL_POD_1 + i, min_soil_moist);

                // if lower than needed water plant 
                if (soilmoist_buf[i] >= min_soil_moist){

                    WaterPod(i);
                }

            }

        }
    }

}

// water plant based on day time, and date 
void DaytimeMode(){
    
    for (int i = 0; i < 6; ++i)
    {   
        // slave 255 = pod is disabled 
        if (EEPROM.read(POD_1_SLAVE_ID + i) != 255)
        {   
            // day time logic
            if (DaytimeCheck() == 1 && daytime_flag[i] == 0){

                // check moisture level 
                ReadPodsSensors(i, data_buffer);
                unsigned int min_soil_moist;
                EEPROM.get(LOW_MOIST_LEVEL_POD_1 + i, min_soil_moist);

                // if lower than needed water plant 
                if (soilmoist_buf[i] >= min_soil_moist){

                    WaterPod(i);

                    // flag that today it was watered 
                    daytime_flag[i] = 1;
                }

            }

        }
    }


}

// checks if is it winter or summer, in summer watering will be at evening
// in winter at morring 
int DaytimeCheck(){
   
    if (time.month >=5 && time.month <=10){

        //summer
        if (time.Hours >=20 &&  time.Hours <=23){
            return 1;
        } else {
             return 0;
        }

    } else {

        // winter
        if (time.Hours >=10 &&  time.Hours <=12){
            return 1;
        } else {
             return 0;
        }
    }

}


// if day changes plants are ready for watering 
void CheckDay(){
   
    if (current_day != time.day){

        for (int i = 0; i < 6; ++i)
        {
           daytime_flag[i] = 0;

        }
        current_day = time.day;
    }


}


// cheks that soil moisture changes after watreing to ensure correct work 
void WaterFailCheck(){
    

    for (int i = 0; i < 6; ++i)
    {
        if (water_flag_moist[i] != 0){

            // checks in 15 minutes after watering 
            if (millis() - water_flag_time[i] >= 900000){   
                ReadPodsSensors(i, data_buffer);
                if (soilmoist_buf[i] < water_flag_moist[i] - 5){
                    water_flag_moist[i] = 0;

                } else {
                    water_flag_moist[i] = 0;
                    SetLockout(2);
                }
            }

        }
    }

}


// update sensors values from slave every hour 
unsigned long UpdateSensors(unsigned long previousMillis){
    
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= 3600000){  

        for (int i = 0; i < EEPROM.read(NUMBER_OF_SLAVES); ++i)
        {
            ReadSlaveSensors(i, data_buffer);

        }

        for (int i = 0; i < 6; ++i)
        {   
            if (EEPROM.read(POD_1_SLAVE_ID + i) != 255)
            {
                ReadPodsSensors(i, data_buffer);


            }
        }
        
        previousMillis = currentMillis;

    }

    return previousMillis;
}


// water pod 
uint8_t WaterPod(uint8_t PodId){
    unsigned int water_amount;


    //check for avalibule water 
    if (EEPROM.read(LOCKOUT) != 0){
        WriteHistoryWateringFail(PodId);
        return 1;
    }
    else{
        
        // send to salve close values command 
        //if (SetValve(EEPROM.read(POD_1_SLAVE_ID + PodId ) - 1, PodId) != 0){
       //     WriteHistoryWateringFail(PodId);
        //    return 1;

       // }

        
        
        delay(500);
        

        EEPROM.get(WATER_AMOUNT_POD_1 + PodId * 2, water_amount);
        
        // start pump 
        //TurnOn(PUMP_PIN);

        // wait time based on amount of water and pump consatnt 
        delay((water_amount / EEPROM.read(PUMP_CONST))*1000);

        // stop pump 
        //TurnOff(PUMP_PIN);

        // set flags fro watering fail check
        water_flag_moist[PodId] = soilmoist_buf[PodId];
        water_flag_time[PodId] = millis();

        // send to clave reset valve command 
        //ResetValve(EEPROM.read(POD_1_SLAVE_ID + PodId) - 1); 
        
        // write log 
        WriteHistoryWatering(PodId, water_amount);

        return 0;
        
    }
}


// convert 4 bytes to float 
float byte_to_float(uint8_t * bytes){
      float data;
      union {
          uint8_t bytes[4];
          float value;
      }u;
      
      memcpy(u.bytes, bytes, 4);
      
      return u.value;
}

// convert 2 bytes to unsigned int
unsigned int byte_to_uint(uint8_t * bytes){
      float data;
      union {
          uint8_t bytes[2];
          unsigned int value;
      }u;
      
      memcpy(u.bytes, bytes, 2);
      
      return u.value;
}

// ************************************************************************************************

// *********************************RADIO COM FUNCTIONS********************************************

// sand massage to slave 
void SendToSlave(uint8_t SlaveId){

    // set address
    radio.openWritingPipe(address[SlaveId]);
    

    if (radio.write(&massage, sizeof(massage))) {    // send data 
        

        if (!radio.available()) {                                  
        } else {
            while (radio.available()) {                    
                radio.read(&responce, sizeof(responce));    // read responce 
            }
        }
    } 

}

// clear responce from slave 
void ResponceClear(){
    for (int i = 0; i < RESPONCE_SIZE; ++i)
    {
        responce[i] = 0;
    }
    
}

// send close valves command 
uint8_t SetValve(uint8_t SlaveId, uint8_t PodId){
    massage[0] = 0;
    massage[1] = PodId;
    massage[2] = 0;

    SendToSlave(SlaveId);
    if (responce[1] == 1){
        ResponceClear(); 
        return 0;
    } else {
        ResponceClear();
        return 1;

    }

}

// send reset valves command 
uint8_t ResetValve(uint8_t SlaveId){
    massage[0] = 1;
    massage[1] = 0;
    massage[2] = 0;
    massage[3] = 0;
    massage[4] = 0;
    massage[5] = 0;
    SendToSlave(SlaveId);
    if (responce[1] == 1){
        ResponceClear(); //todo
        return 0;
    } else {
        ResponceClear();
        return 1;

    }

}

// read sensors from salve 
void ReadSlaveSensors(uint8_t SlaveId,  uint8_t * data){
    
    massage[0] = 2;
    massage[1] = 0;
    massage[2] = 0;
    massage[3] = 0;
    massage[4] = 0;
    massage[5] = 0;
    //SendToSlave(SlaveId);
    
    ResponceClear();
    delay(250);
    massage[0] = 2;
    massage[1] = 1;
    massage[2] = 0;
    massage[3] = 0;
    massage[4] = 0;
    massage[5] = 0;
    //SendToSlave(SlaveId);
    responce[0] = 50;
    
    
    data[0]=responce[0];
    light_buf = data[0];
    ResponceClear();
    
    massage[0] = 2;
    massage[1] = 2;
    massage[2] = 0;
    massage[3] = 0;
    massage[4] = 0;
    massage[5] = 0;
    //SendToSlave(SlaveId);
    responce[0] = 143;
    responce[1] = 194;
    responce[2] = 177;
    responce[3] = 65;
   
    for (int i = 0; i < 4; ++i)
    {
        data[i+1]=responce[i];
    }
    airtemp_buf = byte_to_float(responce);
    ResponceClear();
    
    massage[0] = 2;
    massage[1] = 3;
    massage[2] = 0;
    massage[3] = 0;
    massage[4] = 0;
    massage[5] = 0;
    //SendToSlave(SlaveId);

    responce[0] = 195;
    responce[1] = 245;
    responce[2] = 36;
    responce[3] = 65;
   
    
    for (int i = 0; i < 4; ++i)
    {
        data[i+5]=responce[i];
    }
    airmoist_buf = byte_to_float(responce);
    ResponceClear();
    
    // write log 
    WriteHistorySalveSens(SlaveId + 1, light_buf, airtemp_buf, airmoist_buf);
    

}

// read pods sensors from salve 
void ReadPodsSensors(uint8_t PodId, uint8_t * data){

    massage[0] = 3;
    massage[1] = 0;
    massage[2] = PodId;
    massage[3] = 0;
    massage[4] = 0;
    massage[5] = 0;

    //SendToSlave(EEPROM.read(POD_1_SLAVE_ID + PodId) - 1);
    ResponceClear();
    delay(250);


    massage[0] = 3;
    massage[1] = 1;
    massage[2] = 0;
    massage[3] = 0;
    massage[4] = 0;
    massage[5] = 0;
    //SendToSlave(EEPROM.read(POD_1_SLAVE_ID + PodId) - 1);
    responce[0] = 154;
    responce[1] = 153;
    responce[2] = 179;
    responce[3] = 65;
    for (int i = 0; i < 4; ++i)
    {
        data[i]=responce[i];
        
    }
    soiltemp_buf = byte_to_float(responce);
    ResponceClear();
   

    massage[0] = 3;
    massage[1] = 2;
    massage[2] = 0;
    massage[3] = 0;
    massage[4] = 0;
    massage[5] = 0;
    //SendToSlave(EEPROM.read(POD_1_SLAVE_ID + PodId) - 1);
    responce[0] = 45;
    responce[1] = 1;
    for (int i = 0; i < 2; ++i)
    {
        data[i+4]=responce[i];
    }
    soilmoist_buf[PodId] = byte_to_uint(responce);
    ResponceClear();

    // write log 
    WriteHistoryPodSens(PodId + 1, soiltemp_buf, soilmoist_buf[PodId]);
    
}

// clean slave history log command 
uint8_t CleanSlaveHis(uint8_t SlaveId){
    massage[0] = 4;
    massage[1] = 0;
    massage[2] = 0;
    massage[3] = 0;
    massage[4] = 0;
    massage[5] = 0;

    SendToSlave(SlaveId);
    if (responce[1] == 1){
        ResponceClear(); 
        return 0;

    } else {
        ResponceClear();
        return 1;

    }

}

// sends time to slave  
uint8_t SendTimeSlave(uint8_t SlaveId){


    time.gettime();         
    massage[0] = 5;
    massage[1] = time.minutes;
    massage[2] = time.Hours;
    massage[3] = time.day;  
    massage[4] = time.month;         
    massage[5] = time.year;

    SendToSlave(SlaveId);
    if (responce[1] == 1){
        ResponceClear(); //todo
        return 0;

    } else {
        ResponceClear();
        return 1;

    }

}

// set time to slave to all avalibule salves 
uint8_t SetTimeToSlave(){
        
    for (int i = 0; i < EEPROM.read(NUMBER_OF_SLAVES); ++i)
    {   
    
        if (SendTimeSlave(i)!=0){
            return 1;
        }

    }
    
    return 0;
    
}
// ************************************************************************************************

// *************************************PC COMM FUNCTIONS******************************************

// send master parameters to PC
void SendParamPc(){
    uint8_t dataToSend[EEPROM_PARAM_SPACE + 2];
    dataToSend[0] = EEPROM_PARAM_SPACE + 1;
    for (int i = 1; i <= EEPROM_PARAM_SPACE; ++i)
    {
        dataToSend[i] = EEPROM.read(i - 1);
    }
    dataToSend[EEPROM_PARAM_SPACE + 1] = 250;
    Serial.write(dataToSend, EEPROM_PARAM_SPACE + 2);

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
    delay(100);
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

// analyze massage from PC and execute request 
void ReadFromPc(){
   
    byte newmsg = 0;
    if(Serial.available() > 0){
        Serial.readBytes(pc_responce, 7);
        newmsg = 1;
    } 

    // if new massage avalible 
    if (newmsg == 1){       
            switch(pc_responce[0]){         // first byte specify command 

                // measure and save to EEPROM new distance from supply tank
                case 1:
                    SendAckPc();
                    CalibrateDistance();
                    
                break;
                
                // set time to RTC
                case 2:
                    // [sec, min, hour, day, month, year, weekd]           
                    time.settime(pc_responce[1], pc_responce[2], pc_responce[3], pc_responce[4], 
                        pc_responce[5], pc_responce[6], 1); 

                    // set time to slaves 
                    if (SetTimeToSlave()==0){
                        SendAckPc();
                    } else {
                        SendErrPc();
                    }
                    
                break;

                // change pump constant 
                case 3:
                    SendAckPc();
                    SetPumpConst(pc_responce[1]);
                    
                break;

                // reconfig pods parameters 
                case 4:
                    SendAckPc();
                    ChangePodsParam(pc_responce);
                    
                break;

                // change pods cooldown time 
                case 5:
                    SendAckPc();
                    SetPodCooldown(pc_responce[1], pc_responce[2]);
                    
                break;

                // change water amount needed per shot 
                case 6:
                    SendAckPc();
                    SetPodWaterAmount(pc_responce[1], pc_responce[2], pc_responce[3]);
                    
                break;

                // change minimum moisture level 
                case 7:
                    SendAckPc();
                    SetPodMoistLevel(pc_responce[1], pc_responce[2], pc_responce[3]);
                    
                break;

                // clean master history 
                case 8:
                    SendAckPc();
                    CleanMasterHis();
                    
                break;

                // clean slave histiry 
                case 9:
                    
                    if (CleanSlaveHis(pc_responce[1])==0){
                        SendAckPc();
                    } else {
                        SendErrPc();
                    }
                    
                break;

                // change watering mode
                case 10:
                    SendAckPc();
                    ChangeWaterMode(pc_responce[1]);
                    
                break;

                // read slave sensors 
                case 11:
                    uint8_t dataToSendSlave[11];
                    uint8_t dataSlave[9];

                    
                    for (int i = 0; i < 8; ++i)
                    {
                        dataToSendSlave[i] = 0;
                    }
                    dataToSendSlave[0] = 10;
                    dataToSendSlave[10] = 250;
                    ReadSlaveSensors(pc_responce[1], dataSlave);
                    for (int i = 0; i < 9; ++i)
                    {
                        dataToSendSlave[i + 1] = dataSlave[i];
                    }
                    Serial.write(dataToSendSlave, 11);
                    
                break;

                // read pods sensors 
                case 12:
                    uint8_t dataToSendPods[8];
                    uint8_t dataPods[6];
                    for (int i = 0; i < 8; ++i)
                    {
                        dataToSendPods[i] = 0;
                    }
                    dataToSendPods[0] = 7;
                    dataToSendPods[7] = 250;

                    ReadPodsSensors(pc_responce[1], dataPods);
                    for (int i = 0; i < 6; ++i)
                    {
                        dataToSendPods[i + 1] = dataPods[i];
                    }
                    Serial.write(dataToSendPods, 8);
                    
                break;

                // send EEPROM history log to PC
                case 13:
                    if (pc_responce[1]==0){
                        SendHisPointPc();
                    } else if (pc_responce[1]==1){
                        
                        SendHisChankPc(pc_responce[2]);
                    }                    
                    
                    
                break;

                // reset lockout
                case 14:
                    SendAckPc();
                    SetLockout(0);
                    
                break;

                // send parameters to PC
                case 15:
                    SendParamPc();
                 
                break;

                // force water command
                case 16:
                    
                    if (WaterPod(pc_responce[1])==0){
                        SendAckPc();
                    } else {
                        SendErrPc();
                    }

                break;

                default:
                    SendErrPc();
                    
                break;

            }       
            
    }
  
}

// ************************************************************************************************

// ********************************MASTER HARDWARE FUNTIONS****************************************
void TurnOn(int pin){
    digitalWrite(pin, HIGH); 
  
}

void TurnOff(int pin){
    digitalWrite(pin, LOW); 
  
}

void Blink(uint8_t pin, uint8_t blinks){


    for (int i = 0; i < blinks; ++i)
    {
        TurnOn(pin);
        delay(200);
        TurnOff(pin);
        delay(200);
    }

}

// measure distance vie ultrasonic sensor 
uint8_t DisMes(){

    digitalWrite(SR04_TRIG_PIN, LOW);
    delayMicroseconds(5);
    digitalWrite(SR04_TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(SR04_TRIG_PIN, LOW);
    pinMode(SR04_ECHO_PIN, INPUT);

    return (pulseIn(SR04_ECHO_PIN, HIGH)/2) / 29.1;

}

// check that there is enough water in supply tank
uint8_t TankCheck() {
    uint8_t limit = EEPROM.read(LIM_DIS_SUPPLY);
   
    if (DisMes() >  limit - 3) {
        // low water 
        return 1;

    } else {
        // enough water
        return 0;

    }
}  

// radio startup  
void radioSetup() {
    radio.begin();                      // activate module 
    radio.setAutoAck(1);                // auto ack
    radio.setRetries(0, 15);            // retries times (15)
    radio.enableAckPayload();           // enable posability to read massages from slave
    radio.setPayloadSize(32);           // size of pack, 32 bytes 
    radio.setChannel(CH_NUM);           // channel set 
    radio.setPALevel(SIG_POWER);        // set power of signal 
    radio.setDataRate(SIG_SPEED);       // set speed 
                                        // must be same on salves
    radio.powerUp();                    // start work
    radio.stopListening();              // not listen we are transmitter

}
// ************************************************************************************************

// ***********************************EEPROM FUNCTIONS*********************************************

// set distance parameter 
void CalibrateDistance(){
    EEPROM.put(LIM_DIS_SUPPLY, DisMes());

}

// set lockout
void SetLockout(uint8_t LckNum){
    if (LckNum != 0){
       WriteHistoryLock(LckNum);
    }
    
    EEPROM.put(LOCKOUT, LckNum);

}

// change pump constant 
void SetPumpConst(uint8_t PConst){
    EEPROM.put(PUMP_CONST, PConst);

}

// change cooldown time 
void SetPodCooldown(uint8_t PodId,  uint8_t CD_time){
    if (PodId < 6 && PodId >= 0){

        EEPROM.put(TIME_CD_POD_1 + PodId, CD_time);
    }
    
}

// change water amount 
void SetPodWaterAmount(uint8_t PodId, uint8_t lsb,  uint8_t msb){

    unsigned int water_int =  lsb + (256 * msb);
    if (PodId < 6 && PodId >= 0){

        EEPROM.put(WATER_AMOUNT_POD_1 + PodId * 2, water_int);
    }
    
}

// change minimum soil moisture level
void SetPodMoistLevel(uint8_t PodId,  uint8_t MostLvl_lsb, uint8_t MostLvl_msb){
    if (PodId < 6 && PodId >= 0){

        EEPROM.put(LOW_MOIST_LEVEL_POD_1 + PodId * 2, MostLvl_lsb);
        EEPROM.put(LOW_MOIST_LEVEL_POD_1 + PodId * 2 + 1, MostLvl_msb);
    }
    
}

// assging pods to salves, calculates number of salves and pods 
void ChangePodsParam(uint8_t * aPodParam){
    uint8_t no_of_act_pods = 0;
    uint8_t no_of_act_slave = 0;
    
    for (int i = 1; i <= 6; ++i)
    {
        
        if (aPodParam[i] != 255){
            ++no_of_act_pods;
            if (aPodParam[i] > no_of_act_slave){
                no_of_act_slave = aPodParam[i];
            }
        }

    }
 
    for (int i = 0; i < 6; ++i){   

        EEPROM.put(POD_1_SLAVE_ID + i, aPodParam[i+1]);

    }

    EEPROM.put(NUMBER_OF_PODS, no_of_act_pods);
    EEPROM.put(NUMBER_OF_SLAVES, no_of_act_slave);

}

// clear EEPROM history log 
void CleanMasterHis(){
    int data = EEPROM_PARAM_SPACE;
    EEPROM.put(HIS_MEM_POINTER, data);

}

// change watering mode
void ChangeWaterMode(uint8_t mode){
    if (mode >= 0 && mode <= 2){
        EEPROM.put(WATER_MODE, mode);

    }

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
        CleanMasterHis();

    }else{

        EEPROM.put(HIS_MEM_POINTER, new_position);

    }
 
}

// write lockout event to log      
void WriteHistoryLock(uint8_t lockout){

    uint8_t type = 0;
    int pos;

    EEPROM.get(HIS_MEM_POINTER, pos);
    int new_position = pos + LOG_CHANK_SIZE;

    WriteHistoryTime(pos);
    pos = pos + 5;

    EEPROM.put(pos, type);
    ++pos;                

    EEPROM.put(pos, lockout); 
    
    NewHisPointer(new_position);
}

// write watering event to log
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

// write failed watering event to log
void WriteHistoryWateringFail(uint8_t pod_id){


    uint8_t type = 2;
    int pos;

    EEPROM.get(HIS_MEM_POINTER, pos);

    int new_position = pos + LOG_CHANK_SIZE;

    WriteHistoryTime(pos);
    pos = pos + 5;        

    EEPROM.put(pos, type);
    ++pos;                     

    EEPROM.put(pos, pod_id + 1);

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

// write to log pod  sensors 
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
