
const int drain1 = 0; // brown
const int drain21 = 0; // red
const int charge321 = 0; // orange
const int charge21 = 0; // yellow
// jacksense = 
// battery = blue
#define CCFL_PIN 5  // pin 10 old, pin 5 new (for 32khz special)
const int onfet = 4; // 4 is PD4
#define CCFL_SENSE 20 // A6 = 20
int brightness = 450; // 175 with built-in vref
#define MAX_PWM 250
#define BUTTON_SENSE 2 // 2 is PD2
#define LED_PIN 7
int pwmVal = 80;
int jumpVal = 1;
int offCount = 0;  // counts how many off requests we've seen
#define OFF_THRESH 10 // how many to make us turn off
unsigned long senseRead = 0;


void setup() {
pinMode(onfet,OUTPUT);
digitalWrite(onfet,HIGH);  // keep power on


pinMode(LED_PIN,OUTPUT);
digitalWrite(LED_PIN,HIGH);  // LED ON

pinMode(CCFL_PIN,OUTPUT);
// setPwmFrequency(CCFL_PIN,1); // set PWM freq to 31,250 Hz
// WGM02 = 0, WGM01 = 1, WGM00 = 1 see page 108
// COM0B1 = 1, COM0B0 = 0 see page 107
// CS02 = 0, CS01 = 0, CS00 = 1 see page 110
  CLKPR = 0x80;  // enable write to clkps see page 37
  CLKPR = 0x00;  // set divisor to 1  see page 38
  TCCR0A = 0b10100011;
  TCCR0B = 0b00000001;
  //Serial.begin(38400);  // actually 38400 behaves like 9600
}

void loop() {
  senseRead = 0;
  for (int i = 0; i < 50; i++) senseRead += analogRead(CCFL_SENSE);
  senseRead /= 50;
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

  //Serial.print(pwmVal,HEX);
  //Serial.print("    ");
  //Serial.println(senseRead);
  delay(4000); // one second or four?
  
  if (digitalRead(BUTTON_SENSE)) {
    offCount++;
  } else {
    offCount = 0; // if (--offCount < 0) offCount = 0;
  }

  if (offCount > OFF_THRESH) {
    digitalWrite(LED_PIN,HIGH);  // LED ON
    digitalWrite(onfet,LOW); // turn power off
    analogWrite(CCFL_PIN,0); // turn off ccfl
    while (true);  // wait here until they let go of button
    // brightness = 400;
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

