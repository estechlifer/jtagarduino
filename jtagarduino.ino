/*
 * Jtagarduino.ino
 *
 * Description: Jtag low level interface for Arduino
 *
 */

#define PIN_TDI		6	/* pin mapping for arduino */
#define PIN_TDO		0
#define PIN_TMS		10
#define PIN_TCLK 	13

#define PIN_TDI_HIGH	1
#define PIN_TDO_HIGH	1
#define PIN_TMS_HIGH	1
#define PIN_TCLK_HIGH	1

#define TIME_DELAY	10	/* in ms */

union my_uint32_union{
        uint32_t mdword;
        uint16_t mword[2];
} idcode;



float convertAnalog(int i, int vTop=5)
{ 
  return (float(vTop) / 1024.00) * float(i);
}

void tclk(void)
{
  delay(TIME_DELAY);
  digitalWrite(PIN_TCLK, HIGH);
  delay(TIME_DELAY);
  digitalWrite(PIN_TCLK, LOW);
  delay(TIME_DELAY);
}

void tms(boolean b)
{
  /* not xor gives appropriate logic results here */
  digitalWrite(PIN_TMS, b);	
  delay(TIME_DELAY);
}

void tdi(boolean b)
{
  digitalWrite(PIN_TDI, b);
  delay(TIME_DELAY);
}

boolean getTdo(void)
{
  return 1;
  /*digitalRead(PIN_TDO);*/
}

boolean pushTdi(boolean tmsTerminated=false)
{
  boolean out;
  if(tmsTerminated)
    tms(1);
  out = getTdo();
  tclk();
  return out;
}

void printHelp(void)
{
  Serial.println("");
  Serial.println("Serial Jtag Bitbanger v0.1");
  Serial.println("based heavily off pesco's code");
  Serial.println("Legend:");
  Serial.println("c: tclk pulse(up and down)");
  Serial.println("!: tclk pulse and read tdo");
  Serial.println("?: this help	");
  Serial.println("T: tms on	t: tms off");
  Serial.println("D: tdi on	d: tdi off");
  Serial.println("I: get IDcode R: get Router type");
  Serial.println("");
}

void testLogicReset()
{
    tms(1); tclk();  tclk();  tclk();  tclk();  tclk();  
}

void getIdcode()
{
  testLogicReset();
  tms(0);  tclk();  tms(1);  tclk();  tms(0);  tclk();  tclk();

  idcode.mword[0] = 0;
  idcode.mword[1] = 0;
  Serial.println("");
  for(int c=0; c < 2 ; c++) {
    for(int i=0; i < 16; i++) {
      if(convertAnalog(analogRead(0)) > 3.0 ) {
        idcode.mword[1 - c] = idcode.mword[1 - c] | (1 << i);
        Serial.print(1);
      } 
      else {
        Serial.print(0);
      }           
      tclk();
    }
  }
  Serial.println("");
  Serial.print(idcode.mword[0],HEX);
  Serial.print(idcode.mword[1],HEX);
}

void getRouterType()
{
  getIdcode();
  Serial.println("");
  switch(idcode.mword[0]) {
  case 0x2535:
    if(idcode.mword[1] == 0x417F) {

      Serial.println("Broadcom BCM5354 KFBG Rev 2 CPU chip Found!");  
    }
    break;
  default:
    Serial.println("Router not found!");
  }
  
}



void parseSerial(void)
{
  char *c = new char;
  if(serialInput(c)){
    switch(*c) {
    case 'C':
    case 'c':	
      tclk();
      break;
      /*			case '!':    Serial.print(pushTdi());break;*/
    case '!':    
      pushTdi();
      Serial.println(convertAnalog(analogRead(0)));
      break;
    case 'T':	
      tms(1);
      break;
    case 't':	
      tms(0);
      break;
    case 'D':	
      tdi(1);
      break;
    case 'd':	
      tdi(0);
      break;
    case 'I':       
      getIdcode();
      break;
    case 'R':
      getRouterType();
      break;
    case '?': 	
      printHelp();
      break;
    default: 
      break;
    };
  }
  delete c;

}

boolean serialInput(char *c)
{
  if(Serial.available() > 0) {
    *c = Serial.read();
    return true;
  } 
  else {
    return false;
  }
}

void setup()
{
  pinMode(PIN_TDI, OUTPUT);
  pinMode(PIN_TDO, INPUT);
  pinMode(PIN_TMS, OUTPUT);
  pinMode(PIN_TCLK, OUTPUT);
  Serial.begin(9600);

}

void loop()
{
  parseSerial();
}

