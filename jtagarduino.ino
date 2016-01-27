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
        uint8_t  mbyte[4];
};

struct router_info{
        uint8_t IRLength;
        my_uint32_union idcode;
        my_uint32_union impcode;
} router;


float convertAnalog(int i, int vTop=5)
{ 
  return (float(vTop) / 1024.00) * float(i);
}

boolean getIRLength()
{ 
   testLogicReset();
   tms(0); tclk(); tms(1); tclk(); tclk(); tms(0); tclk(); tclk();
   
   tdi(1);
   for(uint8_t i=0;i<99;i++) {
    tclk();
   }
   
   tdi(0);  
   uint8_t l=0;
   for(l=0;l<99;l++) {
    if(!getTdo())
     break;
    tclk(); 
   }
   
   if(l==99){
     Serial.println("Error: Unable to determine Instruction Register length!");
     return false;
   }
   Serial.print("\r\nInstruction Register Length: ");
   Serial.print(l);
   Serial.print("\r\n");
   router.IRLength = l;
   return true;
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
      if(convertAnalog(analogRead(0)) > 3.0 ) {
        return true;
      } 
      else {
        return false;
      }           
}

void clockBits(uint8_t *b, uint8_t *out, uint8_t i, boolean tmsTerminated=true)
{
  int c=0;
  if(!tmsTerminated){
    for(c=0;c<i;c++) {
      tdi( (b[int(c/8)] & (1 << (c % 8))) ? 1 : 0 );
      if(NULL!=out)
        out[int(c/8)] = out[int(c/8)] | ((1 << (c % 8)) & (getTdo()?0xFF:0));
      tclk();
    }
  } else {
    for(c=0;c<i-1;c++) {
      tdi( (b[int(c/8)] & (1 << (c % 8))) ? 1 : 0 );
      if(NULL!=out)
        out[int(c/8)] = out[int(c/8)] | ((1 << (c % 8)) & (getTdo()?0xFF:0));
      tclk();
    }
    tms(1);
    tdi( (b[int(c/8)] & (1 << (c % 8))) ? 1 : 0 );
    out[int((i-1)/8)] = out[int((i-1)/8)] | ((1 << ((i-1) % 8)) & (getTdo()?0xFF:0));
    tclk();
  }
}


void testLogicReset()
{
    tms(1); tclk();  tclk();  tclk();  tclk();  tclk();
}

void gotoShiftDR()
{
  tms(0);  tclk();  tms(1);  tclk();  tms(0);  tclk();  tclk();  
}

void getIdcode()
{
  testLogicReset();
  gotoShiftDR();
  my_uint32_union in;
  in.mword[0]=0; in.mword[1]=0; router.idcode.mword[0]=0; router.idcode.mword[1]=0;
  clockBits(in.mbyte,router.idcode.mbyte,32);
  Serial.println("");  
  /* lsb endianness assumed here */
  Serial.print(router.idcode.mword[1],HEX);
  Serial.print(router.idcode.mword[0],HEX);
  Serial.println("");
}

/*
boolean getImpCode()
{
  if(getIRLength() && router.IRLength==8) {
    testLogicReset();
    tms(0);tclk();tms(1);tclk();tclk();tms(0);tclk();tclk();
    uint8_t in=b'00000111';
    uint32_t out=0;
    clockBits(in,out,8);
    tclk();tclk();tms(0);tclk();
    clockBits(in,out,32);
    return true;
  } else {
    return false;
  }
  
}*/

boolean getRouterType()
{
  getIdcode();
  Serial.println("");
  switch(router.idcode.mword[1]) {
  case 0x2535:
    if(router.idcode.mword[0] == 0x417F) {

      Serial.println("Broadcom BCM5354 KFBG Rev 2 CPU chip Found!");  
    }
    break;
  default:
    Serial.println("Router not found!");
    return false;
  }
  return true;
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
  Serial.println("L: get IR length");
  Serial.println("");
}

void parseSerial(void)
{
  char *c = new char;
  if(serialInput(c)){
    switch(*c) {
    case 'C':
    case 'c':	tclk();break;
      /*			case '!':    Serial.print(());break;*/
    case '!':      Serial.print(getTdo()); tclk();      break;
    case 'T':      tms(1);      break;
    case 't':	   tms(0);      break;
    case 'D':	   tdi(1);      break;
    case 'd':	   tdi(0);      break;
    case 'I':      getIdcode(); break;
    case 'L':      getIRLength(); break;
    case 'R':      getRouterType();   break;
    case '?': 	   printHelp();      break;
    default:       break;
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

