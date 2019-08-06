/*
Based on:
  1.- 
  Sketch: Sending Atmospheric Pressure Data to ThingSpeak via SIM800L GPRS

  By Roland Pelayo

  Wiring:

  SIM800L TX -> Tx0
  SIM800L RX -> Rx0
  SIM800L RST -> D2
  SIM800L VCC -> 3.7V BAT+
  SIM800L GND -> 3.7V GND / Arduino GND
  Arduino 5V -> 3.7V BAT+

  Full tutorial on: https://www.teachmemicro.com/send-data-sim800-gprs-thingspeak
  
#########################################################
Created by : Atanacio Saquelares
Will use this for a water flood level sensor information
#########################################################
*/

#include <Adafruit_FONA.h>
#include <SoftwareSerial.h>
#include <avr/sleep.h>

#define FONA_RX 8
#define FONA_TX 9
#define FONA_RST 7
#define LEVEL_1 2
#define LEVEL_2 3
#define LEVEL_3 4
#define LEVEL_4 5
#define LEVEL_5 6
#define LEVEL_6 10

// functions prototype
void wakeUp (void);
void startUp (void);
void sendLevel (unsigned char flood_level_tx);


// setups copied from the thinkspeak implementation
SoftwareSerial SIM800ss = SoftwareSerial(FONA_TX, FONA_RX);
Adafruit_FONA SIM800 = Adafruit_FONA(FONA_RST);

int LED = 13;

char http_cmd[80];
char url_string[] = "api.thingspeak.com/update?api_key=0F1D2F1QQUL2OHKH&field1"; //aquí se cambia por el url deseado
int net_status;

unsigned short statuscode;
short length;
String response = "";
char buffer[512];

boolean gprs_on = false;
boolean tcp_on = false;


void setup() {
  // LED pin defined above, set as OUTPUT
  pinMode(LED, OUTPUT);
  // LEVEL_1,2,3,4,5,6 pins defined as pullup inputs.
  pinMode(LEVEL_1, INPUT_PULLUP);
  pinMode(LEVEL_2, INPUT_PULLUP);
  pinMode(LEVEL_3, INPUT_PULLUP);
  pinMode(LEVEL_4, INPUT_PULLUP);
  pinMode(LEVEL_5, INPUT_PULLUP);
  pinMode(LEVEL_6, INPUT_PULLUP);
  // set the sleep mode to the one desired when we don´t want the system to work
  set_sleep_mode (SLEEP_MODE_PWR_DOWN); 
  // disable ADC for consumption reasons
  ADCSRA = 0;
  interrupts();
}


void loop() {
  
  // make sure we have connection to the network
  startUp();
  // LED turned on when detecting flood level
  digitalWrite(LED, true);
  
  // define flood_level variable for main routine.
  unsigned char flood_level = 0;
  unsigned char flood_level_old = 0;

  // define and assign assign values to our levels
  boolean LEVEL_1_val = digitalRead(LEVEL_1);
  boolean LEVEL_2_val = digitalRead(LEVEL_2);
  boolean LEVEL_3_val = digitalRead(LEVEL_3);
  boolean LEVEL_4_val = digitalRead(LEVEL_4);
  boolean LEVEL_5_val = digitalRead(LEVEL_5);
  boolean LEVEL_6_val = digitalRead(LEVEL_6);
  // print values
  Serial.print("LEVEL_1_val = ");
  Serial.println(LEVEL_1_val);
  delay(1000);
  Serial.print("LEVEL_2_val = ");
  Serial.println(LEVEL_2_val);
  delay(1000);
  Serial.print("LEVEL_3_val = ");
  Serial.println(LEVEL_3_val);
  delay(1000);
  Serial.print("LEVEL_4_val = ");
  Serial.println(LEVEL_4_val);
  delay(1000);
  Serial.print("LEVEL_5_val = ");
  Serial.println(LEVEL_5_val);
  delay(1000);
  Serial.print("LEVEL_6_val = ");
  Serial.println(LEVEL_6_val);
  delay(1000);
  
  // sensor logic for assigning the value to flood_level
  while (LEVEL_1_val == 0){
    if (LEVEL_6_val == 0){
      flood_level = 6;
    }
    else if (LEVEL_5_val == 0){
      flood_level = 5;
    }
    else if (LEVEL_4_val == 0){
      flood_level = 4;
    }
    else if (LEVEL_3_val == 0){
      flood_level = 3;
    }
    else if (LEVEL_2_val == 0){
      flood_level = 2;
    }
    else if (LEVEL_1_val == 0){
      flood_level = 1;
    }
    else {
      flood_level = 0;
    }
    
    Serial.print("flood_level = ");
    Serial.println(flood_level);
    // compare old and new flood levels, if it is different, send the flood_level to the server
    if (flood_level != flood_level_old) {
      // send the sensored flood level to the server in case it has changed
      sendLevel(flood_level);
      // now we have a new flood_level_old for comparing
      flood_level_old = flood_level;
    }
   // get values again
    LEVEL_1_val = digitalRead(LEVEL_1);
    LEVEL_2_val = digitalRead(LEVEL_2);
    LEVEL_3_val = digitalRead(LEVEL_3);
    LEVEL_4_val = digitalRead(LEVEL_4);
    LEVEL_5_val = digitalRead(LEVEL_5);
    LEVEL_6_val = digitalRead(LEVEL_6);
  // print values again
    Serial.print("LEVEL_1_val = ");
    Serial.println(LEVEL_1_val);
    delay(1000);
    Serial.print("LEVEL_2_val = ");
    Serial.println(LEVEL_2_val);
    delay(1000);
    Serial.print("LEVEL_3_val = ");
    Serial.println(LEVEL_3_val);
    delay(1000);
    Serial.print("LEVEL_4_val = ");
    Serial.println(LEVEL_4_val);
    delay(1000);
    Serial.print("LEVEL_5_val = ");
    Serial.println(LEVEL_5_val);
    delay(1000);
    Serial.print("LEVEL_6_val = ");
    Serial.println(LEVEL_6_val);
    delay(1000);
  }

  // if LEVEL_1 isn´t activated, we are going into sleep mode for saving battery power
  // this allows us to send the microchip to sleep
  sleep_enable();

  
  // advice of going to sleep
  Serial.println("Going to sleep in 3...");
  delay(1000);
  Serial.println("2...");
  delay(1000);
  Serial.println("1...");
  delay(1000);
  Serial.println("Good bye...");
  delay(1000);

  // turn off LED
  digitalWrite(LED, false);

  // this create the interruption instance for when LEVEL_1 goes down (falling), it goes to the wakeUp ISR
  attachInterrupt (digitalPinToInterrupt(LEVEL_1), wakeUp, FALLING);
  // now let´s go to sleep until we get the interruption 
  sleep_cpu();
}

void wakeUp(){
  //print on serial about interruption fired
  delay(1000);
  Serial.println("Interrupt Fired");
  delay(1000);
  
  // cancel sleep as a precaution for an infinite sleep loop
  sleep_disable();
  
  // precautionary while we do other stuff
  detachInterrupt (digitalPinToInterrupt(LEVEL_1));
  return;
} // end of wake


void sendLevel(unsigned char flood_level_tx){
  Serial.print("flood_level_changed = ");
  Serial.println(flood_level_tx);
  delay(1000);
  
  return;
}

void startUp(){
  // workaround of a bug where serial communication was losing data at start
  while (!Serial);
  
  // start 115200 serial comunication
  Serial.begin(115200);
  
  // print log messages to serial...
  Serial.println("Flooding Data to Server");
  delay(1000);
  Serial.println("Initializing SIM800L....");
  delay(1000);
  
  // start serial communication between SIM800 and arduino
  SIM800ss.begin(4800); // if you're using software serial
  
  // repeat this until serial communication with SIM800 is stablished
  if (!SIM800.begin(SIM800ss)) {            
    Serial.println("Couldn't find SIM800L");
    while (1);
  }
  
  // print ready message when serial communication detected.
  Serial.println("SIM800L is OK"); 
  delay(1000);

  // start network registration, retry after a 2 second delay every time
  Serial.println("Waiting to be registered to network...");
  net_status = SIM800.getNetworkStatus();
  while(net_status != 1){
     net_status = SIM800.getNetworkStatus();
     delay(2000);
  }

  // print success messages about the network registration, and now try to get GPRS service
  Serial.println("Registered to home network!");
  Serial.print("Turning on GPRS... ");
  delay(2000); 
  // try to get GPRS service
  while(!gprs_on){
    if (!SIM800.enableGPRS(true)){  
        Serial.println("Failed to turn on GPRS");
        Serial.println("Trying again...");
        delay(2000);
        gprs_on = false;
    }
    else{
        Serial.println("GPRS now turned on");
        delay(2000);
        gprs_on = true;   
    } 
  } 
}

/*  NEED TO UNDERSTAND HOW THIS WORKS FOR THE CORRESPONDING SERVER WE WILL BE USING 
  digitalWrite(LED, LOW);
  delay(2000);
  while(!tcp_on){
    if (!SIM800.HTTP_GET_start(http_cmd, &statuscode, (uint16_t *)&length)) {
         Serial.println("Failed!");
           Serial.println("Trying again...");
           tcp_on = false;
      }
      else{
        tcp_on = true;
        digitalWrite(LED, HIGH);
        while (length > 0) {
           while (SIM800.available()) {
             char c = SIM800.read();
             response += c;
             length--;
           }
        }
        Serial.println(response);
        if(statuscode == 200){
          Serial.println("Success!");
          tcp_on = false;
        }
        digitalWrite(LED, LOW);
      }
      delay(2000);
    }
    delay(2000);
} */
