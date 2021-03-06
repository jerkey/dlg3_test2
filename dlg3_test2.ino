#define BUTTON_SENSE 2  // PD2/2
#define ONFET        4  // PD4/4
#define CCFL_PIN     5  // pin 10 old, pin 5 new (for 32khz special)
#define LED_PIN      7  // PD7/7
// #define CHARGE123    9  // PB1/9 prototype
#define DRAIN1       11 // PB3/11 prototype
#define DRAIN2       3  // PD3/3
#define DRAIN3       10 // PB2/10 prototype
#define BOOST        6  // PD6/6 prototype
#define CHARGE123    BOOST // we changed the board
#define B1THERM      A0
#define B2THERM      A1
#define B3THERM      A2
#define B1P          A3
#define B2P          A4
#define BATTERY      A5
#define CCFL_SENSE   A6
#define JACK_SENSE   A7
// TODO: write charging into master branch
// TODO: write balancing algorithm in master branch

#define B1P_COEFF     203.518 // ADC counts per volt
#define B2P_COEFF     122.460
#define BATTERY_COEFF 81.936
#define BATT_EMPTY   2.95 // how many volts to DIE at
#define BATT_FULL    4.19 // how many volts to STOP CHARGING
#define BATT_LAGGING 3.85 // if some cells are lagging and others are nearfull, use drainers
#define BATT_NEARFULL 4.10 // when charging must slow to match drainers on NEARFULL cells
#define CHARGEI_SLOW    0.2 // current to slow-charge with when one or two cells NEARFULL
#define CHARGEI_MAX     1.8 // MAXIMUM current to charge with
#define CHARGEI_MIN     0.1 // current indicating charger is connected and operating
#define JACK_COEFF (BATTERY_COEFF ) // using same sense resistors but..
#define R901_OHMS 0.462 / 0.92 // ohms of R between JACK_SENSE and BATTERY
#define MAX_CHARGEPWM 30 // going lower than this PWM does not increase charging
#define CHARGEPWM_JUMP 0.1 // amount to change chargePWM each time updateCharging() runs

int brightness = 450; // 175 with built-in vref
#define MAX_PWM 250
int pwmVal = 80;
int jumpVal = 1;
int offCount = 0;  // counts how many off requests we've seen
#define OFF_THRESH 5 // how many to make us turn off
unsigned long senseRead = 0;
#define DELAYFACTOR 32 // millis() and delay() this many times faster
float batt1,batt2,batt3,battery_total ; // voltage of battery cells and total
float r901, chargeI, targetChargeI; // voltage at R901_OHMS, current across it, target charge current
unsigned short debugMode = 0, lightMode = 0; // allows for debugging modes to be triggered in production software
#define CHARGEMODE 100 // we are only on to charge the battery; no light
unsigned short drainPWM[3] = {0,0,0}; // individual battery balancers, 254 = ON
unsigned short lastDrainPWM[3] = {0,0,0}; // to prevent unnecessary analogWrites
float chargePWM=255; // charge123 is a PFET, 255 = none, 254=min 1=max charging
int lastChargePWM = 0; // different so updatePWM() sets it immediately

void setup() {
pinMode(ONFET,OUTPUT);
digitalWrite(ONFET,HIGH);  // keep power on
pinMode(CHARGE123,OUTPUT);
pinMode(DRAIN1   ,OUTPUT);
pinMode(DRAIN2   ,OUTPUT);
pinMode(DRAIN3   ,OUTPUT);

pinMode(LED_PIN,OUTPUT);
digitalWrite(LED_PIN,HIGH);  // LED ON

pinMode(CCFL_PIN,OUTPUT); // 15.63khz
setPwmFrequency(DRAIN3,8); // timer1 = pin 9,10 = OLD CHARGE123, DRAIN3
setPwmFrequency(DRAIN2,8); // timer2 = pin 3,11 = DRAIN2, DRAIN1
// WGM02 = 0, WGM01 = 1, WGM00 = 1 see page 108
// COM0B1 = 1, COM0B0 = 0 see page 107
// CS02 = 0, CS01 = 0, CS00 = 1 see page 110
  CLKPR = 0x80;  // enable write to clkps see page 37
  CLKPR = 0x00;  // set divisor to 1  see page 38
  TCCR0A = 0b10100011; // timer0 affects pin 5,6 = CCFL_PIN,BOOST
  TCCR0B = 0b00000001;
  Serial.begin(76800); // to get 38400 baud, put 76800 baud here
  analogWrite(CHARGE123,MAX_CHARGEPWM); // to find out if charger is connected
  getBattVoltages();
  if (chargeI > CHARGEI_MIN) { // charger is connected
    lightMode = CHARGEMODE;
  }
  analogWrite(CHARGE123,255); // turn off charging for now
}

void loop() {
  if (lightMode == 0) {
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
  } else {
    analogWrite(CCFL_PIN,0);
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

  if (offCount > OFF_THRESH) {
    if (chargeI < CHARGEI_MIN) {
      die("offCount > OFF_THRESH");
    } else {
      lightMode = CHARGEMODE;
    }
  }

  if ((lightMode == CHARGEMODE) && (chargeI < CHARGEI_MIN)) {
      die("charging complete");
  }

  updateCharging(); // set PWMs according to charging situation
  updateDrains(); // make sure PWMs are set
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
  battery_total = averageRead(BATTERY) / BATTERY_COEFF;
  batt3 = battery_total - batt2 - batt1;
  r901 = averageRead(JACK_SENSE) / JACK_COEFF;
  chargeI = (r901 - battery_total) / R901_OHMS;
}

void die(String reason) {
  Serial.println("off because: "+reason);
  digitalWrite(LED_PIN,LOW);  // LED OFF
  digitalWrite(ONFET,LOW); // turn power off
  analogWrite(CCFL_PIN,0); // turn off ccfl
  while (true) { // wait here until they let go of button
    Serial.print(reason);
    updateCharging(); // set PWMs according to charging situation
    updateDrains(); // make sure PWMs are set
    getBattVoltages();
    printAnalogs();
    delay((unsigned)1000*DELAYFACTOR);
  }
}

void printAnalogs() {
  Serial.print(millis());
  Serial.print("  ");
  if (debugMode > 0) { // print detailed battery analogRead information
    Serial.print(averageRead(B1P),3);
    Serial.print(" B1P (");
    Serial.print(averageRead(B1P)/B1P_COEFF,3);
    Serial.print(")  ");
    Serial.print(averageRead(B2P),3);
    Serial.print(" B2P (");
    Serial.print(averageRead(B2P)/B2P_COEFF,3);
    Serial.print(")  ");
    Serial.print(averageRead(BATTERY),3);
    Serial.print(" BATTERY (");
    float battery_print = averageRead(BATTERY)/BATTERY_COEFF;
    Serial.print(battery_print,3);
    Serial.print(")  ");
    float jack_print = averageRead(JACK_SENSE)/JACK_COEFF;
    Serial.print(jack_print,3);
    Serial.print(" R901 (");
    Serial.print(averageRead(JACK_SENSE),3);
    Serial.print(")  CHARGEI: ");
    Serial.print((jack_print - battery_print) / R901_OHMS,2);
    Serial.print("  CHARGE123: ");
    Serial.print(chargePWM);
    Serial.print("  DRAIN1: ");
    Serial.print(drainPWM[0]);
    Serial.print("  DRAIN2: ");
    Serial.print(drainPWM[1]);
    Serial.print("  DRAIN3: ");
    Serial.print(drainPWM[2]);
    Serial.print("  TH1:");
    Serial.print(averageRead(B1THERM),1);
    Serial.print("  TH2:");
    Serial.print(averageRead(B2THERM),1);
    Serial.print("  TH3:");
    Serial.print(averageRead(B3THERM),1);
  } else {
    Serial.print("batt1=");
    Serial.print(batt1,3);
    Serial.print("   batt2=");
    Serial.print(batt2,3);
    Serial.print("   batt3=");
    Serial.print(batt3,3);
    Serial.print("  CHARGEI: ");
    Serial.print(chargeI,2);
  }
  Serial.println("");
}

void updateDrains() {
  if (drainPWM[0] != lastDrainPWM[0]) {
    analogWrite(DRAIN1,drainPWM[0]);
    lastDrainPWM[0]=drainPWM[0];
  }
  if (drainPWM[1] != lastDrainPWM[1]) {
    analogWrite(DRAIN2,drainPWM[1]);
    lastDrainPWM[1]=drainPWM[1];
  }
  if (drainPWM[2] != lastDrainPWM[2]) {
    analogWrite(DRAIN3,drainPWM[2]);
    lastDrainPWM[2]=drainPWM[2];
  }
  if ((int)chargePWM != lastChargePWM) {
    lastChargePWM=(int)chargePWM;
    analogWrite(CHARGE123,lastChargePWM);
  }
}

// http://electronics.stackexchange.com/questions/120996/lipo-charger-question
void updateCharging() {
  targetChargeI = CHARGEI_MAX; // default is to charge at full current
  if ((batt1 >= BATT_NEARFULL) || (batt2 >= BATT_NEARFULL) || (batt3 >= BATT_NEARFULL)) { // if any cell is nearfull
    if ((batt1 < BATT_LAGGING) || (batt2 < BATT_LAGGING) || (batt3 < BATT_LAGGING)) { // if any cell is lagging
      targetChargeI = CHARGEI_SLOW;
      drainPWM[0] = (batt1 >= BATT_NEARFULL) ? 254 : 0; // turn on drainer if battery nearfull
      drainPWM[1] = (batt2 >= BATT_NEARFULL) ? 254 : 0;
      drainPWM[2] = (batt3 >= BATT_NEARFULL) ? 254 : 0;
    }
    // do something to tell the user that pack is nearly charged
  }
  if (chargeI < 0.1) { // don't drain anything if we're not actually charging!
    drainPWM[0] = 0;
    drainPWM[1] = 0;
    drainPWM[2] = 0;
  }
  if ((batt1 >= BATT_FULL) || (batt2 >= BATT_FULL) || (batt3 >= BATT_FULL)) {
    targetChargeI = 0.0;
  }

  if (chargeI > targetChargeI) { // charge current is too high
    chargePWM += CHARGEPWM_JUMP; // reduce charge by bringing PWM higher
    if (chargePWM > 255) chargePWM = 255; // don't bring it higher than 255 = fully off
  }
  if (chargeI < targetChargeI) { // charge current is too low
    chargePWM -= CHARGEPWM_JUMP; // increase charge by bringing PWM lower
    if (chargePWM < MAX_CHARGEPWM) chargePWM = MAX_CHARGEPWM; // don't bring it lower than MAX_CHARGEPWM
  }
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
