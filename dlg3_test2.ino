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
int brightness = 450; // 175 with built-in vref
#define MAX_PWM 250
int pwmVal = 80;
int jumpVal = 1;
int offCount = 0;  // counts how many off requests we've seen
#define OFF_THRESH 10 // how many to make us turn off
unsigned long senseRead, analogAdder = 0;

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
  Serial.begin(38400);  // actually 38400 behaves like 19200
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

  analogAdder = 0;
  for (int i = 0; i < 50; i++) analogAdder += analogRead(BATTERY);
  analogAdder /= 50;
  Serial.print(analogAdder,HEX);
  Serial.print("battery    (");
  Serial.println(analogRead(BATTERY),HEX);

  delay(4000); // actual time 1/32 this number of milliseconds
  
  if (digitalRead(BUTTON_SENSE)) {
    Serial.print("+");
    offCount++;
  } else {
    offCount = 0; // if (--offCount < 0) offCount = 0;
  }

  if (offCount > OFF_THRESH) {
    Serial.println("off");
    digitalWrite(LED_PIN,HIGH);  // LED ON
    digitalWrite(ONFET,LOW); // turn power off
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

