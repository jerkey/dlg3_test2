
const int drain1 = 0; // brown
const int drain21 = 0; // red
const int charge321 = 0; // orange
const int charge21 = 0; // yellow
// jacksense = 
// battery = blue
#define CCFL_PIN 10  // pin 10 is what we're actually using
const int onfet = 4; // 4 is PD4
#define CCFL_SENSE A6
const int brightness = 150; // 175 with built-in vref
#define MAX_PWM 250
#define BUTTON_SENSE 2 // 2 is PD2
#define LED_PIN 7
int pwmVal = 0;
int jumpVal = 10;


void setup() {
pinMode(onfet,OUTPUT);
digitalWrite(onfet,HIGH);  // keep power on


pinMode(LED_PIN,OUTPUT);
digitalWrite(LED_PIN,HIGH);  // LED ON

pinMode(CCFL_PIN,OUTPUT);
setPwmFrequency(CCFL_PIN,1); // set PWM freq to 31,250 Hz

//TCCR2B &= 0b11111000;  // clear clock-select bits
//TCCR2B |= 1; // pin 3 to 31250Hz divided by 1
  Serial.begin(9600); 
}

void loop() {
  if (analogRead(CCFL_SENSE) < brightness) {
    pwmVal += jumpVal;
    if (pwmVal > MAX_PWM) pwmVal = 1;
    analogWrite(CCFL_PIN,pwmVal);
  }
  if (analogRead(CCFL_SENSE) > brightness) {
    pwmVal -= jumpVal;
    if (pwmVal < 1) pwmVal = 1;  
    analogWrite(CCFL_PIN,pwmVal);
  }

  Serial.println(analogRead(CCFL_SENSE));
  delay(50);
  
  if (digitalRead(BUTTON_SENSE)) {
    digitalWrite(onfet,LOW); // turn power off
    digitalWrite(LED_PIN,LOW); // turn LED off
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

