#define BUTTON_SENSE 2  // PD2/2
#define ONFET        4  // PD4/4
#define CCFL_PIN     5  // pin 10 old, pin 5 new (for 32khz special)
#define LED_PIN      7  // PD7/7
#define CHARGE123    9  // PB1/9 prototype
#define DRAIN1       11 // PB3/11 prototype
#define DRAIN2       3  // PD3/3
#define DRAIN3       10 // PB2/10 prototype
#define BOOST        6  // PD6/6 prototype
#define B1THERM      A0
#define B2THERM      A1
#define B3THERM      A2
#define B1P          A3
#define B2P          A4
#define BATTERY      A5
#define CCFL_SENSE   A6
#define JACK_SENSE   A7

#define B1P_COEFF     203.518 // ADC counts per volt
#define B2P_COEFF     122.460
#define BATTERY_COEFF 81.936
#define BATT_EMPTY   2.95 // how many volts to DIE at

int brightness = 450; // 175 with built-in vref
#define MAX_PWM 250
int pwmVal = 80;
int jumpVal = 1;
int offCount = 0;  // counts how many off requests we've seen
#define OFF_THRESH 5 // how many to make us turn off
unsigned long senseRead = 0;
#define DELAYFACTOR 64 // millis() and delay() this many times faster
float batt1,batt2,batt3; // voltage of battery cells
int debugMode = 0; // allows for debugging modes to be triggered in production software

void setup() {
pinMode(ONFET,OUTPUT);
digitalWrite(ONFET,HIGH);  // keep power on

pinMode(LED_PIN,OUTPUT);
digitalWrite(LED_PIN,HIGH);  // LED ON

pinMode(CCFL_PIN,OUTPUT); // 32khz
// setPwmFrequency(CCFL_PIN,1); // set PWM freq to 31,250 Hz
// WGM02 = 0, WGM01 = 1, WGM00 = 1 see page 108
// COM0B1 = 1, COM0B0 = 0 see page 107
// CS02 = 0, CS01 = 0, CS00 = 1 see page 110
  CLKPR = 0x80;  // enable write to clkps see page 37
  CLKPR = 0x00;  // set divisor to 1  see page 38
  TCCR0A = 0b10100011;
  TCCR0B = 0b00000001;
  Serial.begin(76800); // to get 38400 baud, put 76800 baud here
}

void loop() {
  if (debugMode == 0) {
    senseRead = averageRead(CCFL_SENSE);
    if (senseRead < brightness) {
      digitalWrite(LED_PIN,HIGH);  // LED ON
      pwmVal += jumpVal;
      if (pwmVal > MAX_PWM) pwmVal = MAX_PWM;
      analogWrite(CCFL_PIN,pwmVal);
    }
    if (senseRead > brightness) {
      digitalWrite(LED_PIN,LOW);  // LED OFF
      pwmVal -= jumpVal;
      if (pwmVal < 1) pwmVal = 1;
      analogWrite(CCFL_PIN,pwmVal);
    }
  }
  getBattVoltages();
  printAnalogs();

  if (batt1 < BATT_EMPTY) die("batt1 dead! ");
  if (batt2 < BATT_EMPTY) die("batt2 dead! ");
  if (batt3 < BATT_EMPTY) die("batt3 dead! ");

  delay(125*DELAYFACTOR); // actual time = n * DELAYFACTOR milliseconds
  
  if (digitalRead(BUTTON_SENSE)) {
    Serial.print("+");
    offCount++;
  } else {
    offCount = 0; // if (--offCount < 0) offCount = 0;
  }

  if (offCount > OFF_THRESH) die("offCount > OFF_THRESH");
}

float averageRead(int pin) {
  float analogAdder = 0;
  for (int i = 0; i < 50; i++) analogAdder += analogRead(pin);
  analogAdder /= 50;
  return analogAdder;
}

void getBattVoltages() {
  batt1 = averageRead(B1P) / B1P_COEFF;
  batt2 = (averageRead(B2P) / B2P_COEFF) - batt1;
  batt3 = (averageRead(BATTERY) / BATTERY_COEFF) - batt2 - batt1;
}

void die(String reason) {
  Serial.println("off because: "+reason);
  digitalWrite(LED_PIN,HIGH);  // LED ON
  digitalWrite(ONFET,LOW); // turn power off
  analogWrite(CCFL_PIN,0); // turn off ccfl
  while (true) { // wait here until they let go of button
    Serial.print(reason);
    printAnalogs();
    delay((unsigned)1000*DELAYFACTOR);
  }
}

void printAnalogs() {
  Serial.print(millis());
  Serial.print("  ");
  if (debugMode > 0) { // print detailed battery analogRead information
    Serial.print(averageRead(B1P),3);
    Serial.print(" B1P  (");
    Serial.print(averageRead(B1P)/B1P_COEFF,3);
    Serial.print(")   ");
    Serial.print(averageRead(B2P),3);
    Serial.print(" B2P  (");
    Serial.print(averageRead(B2P)/B2P_COEFF,3);
    Serial.print(")   ");
    Serial.print(averageRead(BATTERY),3);
    Serial.print(" BATTERY  (");
    Serial.print(averageRead(BATTERY)/BATTERY_COEFF,3);
    Serial.print(")      TH1: ");
    Serial.print(averageRead(B1THERM),2);
    Serial.print("  TH2: ");
    Serial.print(averageRead(B2THERM),2);
    Serial.print("  TH3: ");
    Serial.print(averageRead(B3THERM),2);
  } else {
    Serial.print("batt1=");
    Serial.print(batt1,3);
    Serial.print("   batt2=");
    Serial.print(batt2,3);
    Serial.print("   batt3=");
    Serial.print(batt3,3);
  }
  Serial.println("");
}

void setPwmFrequency(int pin, int divisor) {
  byte mode;
  if(pin == 5 || pin == 6 || pin == 9 || pin == 10) {
    switch(divisor) {
    case 1: 
      mode = 0x01; 
      break;
    case 8: 
      mode = 0x02; 
      break;
    case 64: 
      mode = 0x03; 
      break;
    case 256: 
      mode = 0x04; 
      break;
    case 1024: 
      mode = 0x05; 
      break;
    default: 
      return;
    }
    if(pin == 5 || pin == 6) {
      TCCR0B = TCCR0B & 0b11111000 | mode;
    } 
    else {
      TCCR1B = TCCR1B & 0b11111000 | mode;
    }
  } 
  else if(pin == 3 || pin == 11) {
    switch(divisor) {
    case 1: 
      mode = 0x01; 
      break;
    case 8: 
      mode = 0x02; 
      break;
    case 32: 
      mode = 0x03; 
      break;
    case 64: 
      mode = 0x04; 
      break;
    case 128: 
      mode = 0x05; 
      break;
    case 256: 
      mode = 0x06; 
      break;
    case 1024: 
      mode = 0x7; 
      break;
    default: 
      return;
    }
    TCCR2B = TCCR2B & 0b11111000 | mode;
  }
}

/*     voltage: 4.0v        8.0v             12.0v
 *     206 per volt         123 per volt     82 per volt
 *     339 B1P  (33A)   3DD B2P  (3E0)   3E3 BATTERY  (3E3)      TH1: 304  TH2: 312  TH3: 30C
 *     339 B1P  (33A)   3DC B2P  (3DF)   3E0 BATTERY  (3E2)      TH1: 329  TH2: 319  TH3: 309
 *     339 B1P  (339)   3DD B2P  (3DF)   3E3 BATTERY  (3E0)      TH1: 321  TH2: 312  TH3: 30D
 *     339 B1P  (33A)   3DA B2P  (3E0)   3E0 BATTERY  (3E2)      TH1: 308  TH2: 30E  TH3: 30D
 *     339 B1P  (338)   3DA B2P  (3E0)   3E0 BATTERY  (3E3)      TH1: 329  TH2: 317  TH3: 30C
 *     33A B1P  (339)   3DC B2P  (3E1)   3E0 BATTERY  (3E1)      TH1: 326  TH2: 311  TH3: 30C
 *     33A B1P  (339)   3DA B2P  (3DF)   3E2 BATTERY  (3E3)      TH1: 300  TH2: 30F  TH3: 30D
 *     339 B1P  (339)   3DD B2P  (3C8)   3E1 BATTERY  (3E5)      TH1: 305  TH2: 310  TH3: 30C
 *     339 B1P  (339)   3DC B2P  (3DE)   3E2 BATTERY  (3E1)      TH1: 30A  TH2: 312  TH3: 30C
 *     339 B1P  (339)   3DD B2P  (3DF)   3E1 BATTERY  (3E4)      TH1: 31D  TH2: 30F  TH3: 30F
 *     339 B1P  (335)   3DA B2P  (3E1)   3E1 BATTERY  (3E1)      TH1: 326  TH2: 318  TH3: 30C
 *     339 B1P  (338)   3DA B2P  (3DF)   3E1 BATTERY  (3E5)      TH1: 320  TH2: 315  TH3: 308
 *     33A B1P  (338)   3DB B2P  (3E4)   3E2 BATTERY  (3E1)      TH1: 31A  TH2: 30D  TH3: 30D
 *     339 B1P  (33A)   3DA B2P  (3DF)   3E1 BATTERY  (3E3)      TH1: 308  TH2: 311  TH3: 30D
 *     339 B1P  (339)   3DA B2P  (3DF)   3E1 BATTERY  (3E4)      TH1: 325  TH2: 316  TH3: 309
 *     339 B1P  (339)   3DD B2P  (3DF)   3E3 BATTERY  (3E0)      TH1: 326  TH2: 318  TH3: 30A
 *     339 B1P  (339)   3DC B2P  (3DF)   3E2 BATTERY  (3E0)      TH1: 324  TH2: 317  TH3: 30C
 *     339 B1P  (339)   3DD B2P  (3DF)   3E3 BATTERY  (3E1)      TH1: 326  TH2: 315  TH3: 30D
 */
