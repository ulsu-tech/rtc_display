#include <Wire.h>
#include <LiquidCrystal.h>

/*  The circuit:
 * LCD RS pin to digital pin 12
 * LCD Enable pin to digital pin 11
 * LCD D4 pin to digital pin 5
 * LCD D5 pin to digital pin 4
 * LCD D6 pin to digital pin 3
 * LCD D7 pin to digital pin 2
 * LCD R/W pin to ground
 * LCD VSS pin to ground
 * LCD VCC pin to 5V
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)
 */
// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 8, en = 9, d4 = 4, d5 = 5, d6 = 6, d7 = 7;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

unsigned long lastExec =0;
unsigned long measureDuration = 4 * 1000;
//value for now

struct _registers 
{
  uint8_t sec;
  uint8_t mins;
  uint8_t hour;
  uint8_t day;
  uint8_t month;
  uint8_t year;
  uint16_t temp;
};

union _arrayed 
{
  struct _registers regs;
  uint8_t Carray[8];
};

union _arrayed lastRead = {0};
union _arrayed prevRead = {0};

// TODO add pin reading for "make now"


void setup() {
  // put your setup code here, to run once:
  lcd.begin(16, 2);
  delay(500);
  Serial.begin(9600);
  Wire.begin();
  lcd.setCursor(0,0);
  lcd.print("Hello");
}

void loop() {
  // put your main code here, to run repeatedly:
  unsigned long now = millis();
  if (now - lastExec > measureDuration)
  {
    lastExec = now;
    shiftNow2Prev();
    
    readMeasurements(); // modifies n_* values
    showValues();
    sendValues();
    // TODO switch to sleep + WD + interrupt
    delay(1000);
  }
}

void shiftNow2Prev()
{
  prevRead.regs.sec = lastRead.regs.sec;
  prevRead.regs.mins = lastRead.regs.mins;
  prevRead.regs.hour = lastRead.regs.hour;
  prevRead.regs.day = lastRead.regs.day;
  prevRead.regs.month = lastRead.regs.month;
  prevRead.regs.year = lastRead.regs.year;
  prevRead.regs.temp = lastRead.regs.temp;
}


void setZeros(struct  _registers *rg)
{
  rg->sec  = 0;
  rg->mins = 0;
  rg->hour = 0;
  rg->day = 0;
  rg->month = 0;
  rg->year = 0;
  rg->temp = 0;
}

uint8_t RTCaddr = 0x55;
void readMeasurements()
{
  
  Wire.beginTransmission(RTCaddr);
  Wire.write(0x1C);
  Wire.write(0x20);
  Wire.endTransmission();
  
  Wire.beginTransmission(RTCaddr);
  Wire.write(0x22);
  Wire.write(0x01);
  Wire.endTransmission();
  
  Wire.beginTransmission(RTCaddr);
  Wire.write(1);
  Wire.endTransmission();
  Wire.requestFrom(RTCaddr, (uint8_t)6 );
  uint8_t recv = 0;
  unsigned long start = millis();
  unsigned now = start;
  while(recv < 6 && now - start < 10000 )
  {
    now = millis();
    if (Wire.available() > 0)
    {
      int c = Wire.read();
      lastRead.Carray[recv] = (c/16 * 10) + c %16;
      recv++;
    }
  }
  if (recv < 6)
  {
    setZeros(&lastRead.regs);
    return;
  }

  //reading temp
  Wire.beginTransmission(RTCaddr);
  Wire.write(0x1E);
  Wire.endTransmission();
  Wire.requestFrom(RTCaddr, (uint8_t)2 );
  recv = 0;
  start = millis();
  now = start;
  uint8_t tempR[2];
  while(recv < 2 && now - start < 10000 )
  {
    now = millis();
    if (Wire.available() > 0)
    {
      int c = Wire.read();
      tempR[recv] = c;
      recv++;
    }
  }
  if (recv < 6)
  {
    setZeros(&lastRead.regs);
    return;
  }
  lastRead.regs.temp = ((uint16_t)tempR[0])<<4 + (tempR[1]>>6);
}

void outputLine(int row, const struct _registers *rg)
{
  lcd.setCursor(0,row);
  if( rg->day < 10)
  {
    lcd.print(" ");
  }
  lcd.print(rg->day);
  lcd.print("/");
  if (rg->month < 10)
  {
    lcd.print(" ");
  }
  lcd.print(rg->month);
  lcd.print(" ");

  if (rg->hour < 10)
  {
    lcd.print(" ");
  }
  lcd.print(rg->hour);
  lcd.print(":");
  if (rg->mins > 10)
  {
    lcd.print("0");
  }
  lcd.print(rg->mins);
  lcd.print(":");
    
  if (rg->sec < 10) {
    lcd.print("0");
  }
  lcd.print(rg->sec);
  lcd.print(" ");

  float tem = rg->temp * 0.5 - 273;
  lcd.print(tem);
  lcd.print(" C");
}

void showValues()
{
  outputLine(0, &prevRead.regs);
  outputLine(1, &lastRead.regs);
}

void sendValues()
{
  Serial.write(lastRead.Carray, 8);
  
  uint8_t sum = 0;
  for(uint8_t i = 0; i < 8; ++i)
  {
    sum += lastRead.Carray[i];
  }

  delay(500);
  if(Serial.available() < 1)
  {
    lcd.setCursor(6, 0);
    lcd.print("F");
    lcd.setCursor(5, 1);
    lcd.print("E");
    lcd.setCursor(7, 1);
    lcd.print("G");
    return;
  }
  int v = Serial.read();
  if (v != sum)
  {
    lcd.setCursor(6, 0);
    lcd.print("H");
    lcd.setCursor(5, 1);
    lcd.print("O");
    lcd.setCursor(7, 1);
    lcd.print("P");
  }
}
