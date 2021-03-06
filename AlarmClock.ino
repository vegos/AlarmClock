#include <LiquidCrystal.h>
#include <Wire.h>
#include <DHT.h>
#include <DS1307RTC.h> 
#include <Time.h>
#include <SPI.h>
#include <Ethernet.h>
#include <EEPROM.h>


#define DHTPIN     9
DHT dht(DHTPIN, DHT11);

#define REDLed      A3
#define GREENLed    A2
#define PosUp       1
#define BuzzerPin     8

LiquidCrystal lcd(7, 6, 5, 4, 3, 2);

long StartMillis = 0;
long BlinkMillis = 0;
long DisplayTemp = 0;
#define UpdateTemp 20000
#define UpdateData 300
long DataMillis = 0;


float CurrentTemp, CurrentHum = -70;

int CurrentHour = -70;
int CurrentMin = -70;
boolean BlinkDot = true;
boolean TempOrHum = true;
boolean Display = true;

/*
byte cel[8] = {
  B01000,
  B10100,
  B01000,
  B00011,
  B00100,
  B00100,
  B00011,
  B00000
};
*/

byte cel[8] = {
  B00110,
  B01001,
  B00110,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000
};


byte delta[8] = {
  B00100,
  B01010,
  B10001, 
  B10001,
  B10001,
  B10001,
  B11111,
  B00000
};

byte pi[8] = {
  B11111,
  B10001,
  B10001,
  B10001,
  B10001,
  B10001,
  B10001,
  B00000
};

byte sigma[8] = {
  B11111,
  B10000,
  B01000,
  B00100,
  B00100,
  B01000,
  B11111,
  B00000
};

byte lamda[8] = {
  B00100,
  B01010,
  B10001, 
  B10001,
  B10001,
  B10001,
  B10001,
  B00000
};


byte gamma[8] = {
  B11111,
  B10000,
  B10000,
  B10000,
  B10000,
  B10000,
  B10000,
  B00000
};


byte mac[] = {  0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02 };
char serverName[] = "10.15.158.65";
byte ip[] = { 10, 15, 158, 105 };
byte subnet[] = { 255, 255, 255, 192 };
byte gateway[] = { 10, 15, 158, 99 };

EthernetUDP Udp;
EthernetClient client;

EthernetServer server(80);
String s = String(30);
String cmd = "/X";
String readString; 



IPAddress timeServer(10, 15, 158, 65); // time.nist.gov NTP server
const int NTP_PACKET_SIZE= 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

unsigned long unixtime;

int start = 0;
int x = 0;
String data;
int length=0;

int AlarmHour = 0;
int AlarmMinute = 0;

int RemainingHour = -70;
int RemainingMinute = -70;
int CurrentRemainingHour = -70;
int CurrentRemainingMinute = -70;
int CurrentMonth = -70;
int CurrentYear = -70;
int CurrentDay = -70;
int CurrentWeekday = -70;

boolean RedLedState = true;
boolean LastRedLedState = true;
boolean blinkLed = false;             // Blink LED or dots



void setup() 
{
  pinMode(REDLed, OUTPUT);
  pinMode(GREENLed, OUTPUT);
  pinMode(PosUp, INPUT_PULLUP);
  lcd.createChar(1, cel);
  lcd.createChar(2, delta);
  lcd.createChar(3, pi);
  lcd.createChar(4, sigma);
  lcd.createChar(5, lamda);
  lcd.createChar(6, gamma);
  lcd.begin(16, 2);
  lcd.setCursor(0,0);
  lcd.print("  Magla Clock");
  delay(3000);
  lcd.setCursor(0,1);
  lcd.print("Starting network");
  if (Ethernet.begin(mac) == 0) 
  {
    lcd.setCursor(0,1);
    lcd.print("Using static IP ");
    if (Ethernet.localIP())
    {    
      lcd.setCursor(0,1);
      lcd.print("IP:");
      lcd.print(Ethernet.localIP());
      lcd.setCursor(0,1);
      lcd.print("Synching...     ");
      SynchronizeRTCwithNTP();
      Udp.stop();
      delay(3000);
    }
  }
  else
  {
    lcd.setCursor(0,1);
    lcd.print("DHCP            ");
     // Connected from DHCP
    lcd.setCursor(0,1);
    lcd.print("Synching...     ");
    SynchronizeRTCwithNTP();
    Udp.stop();
    delay(1000);
  }
  setSyncProvider(RTC.get);  
  lcd.setCursor(0,1);
  Ethernet.begin(mac, ip);
  server.begin();
  pinMode(BuzzerPin, OUTPUT);
  dht.begin();
  // set up the LCD's number of columns and rows:
  lcd.setCursor(0,0);
  //         0123456789012345
  lcd.print("                "); 
  lcd.setCursor(0,1);
  lcd.print("                ");
  CurrentHum = dht.readHumidity();
  CurrentTemp = dht.readTemperature();
  setSyncProvider(RTC.get);
  CurrentHour = hour();
  CurrentMin = minute();
  lcd.setCursor(0,0);
  if (CurrentHour<10)
    lcd.print("0");
  lcd.print(CurrentHour);
  lcd.setCursor(3,0);
  if (CurrentMin<10)
    lcd.print("0");
  lcd.print(CurrentMin);
  lcd.setCursor(8,0);
  if (CurrentTemp<10)
    lcd.print("0");
  lcd.print(CurrentTemp,0);
  lcd.write(1);
  lcd.print("C");
  lcd.setCursor(13,0);
  if (CurrentHum<10)
    lcd.print("0");
  lcd.print(CurrentHum,0);
  lcd.print("%");
  BlinkDot=true;
  Display=true;  
  StartMillis = millis();
  BlinkMillis = millis();
  DisplayTemp = millis();
  DataMillis = millis();
    
  
  AlarmHour = EEPROM.read(0);
  AlarmMinute = EEPROM.read(1);
  if ((AlarmHour<0) || (AlarmHour>23))
  {
    AlarmHour=0;
    EEPROM.write(0,0);
  }
  if ((AlarmMinute<0) || (AlarmMinute>59))
  {
    AlarmMinute=0;
    EEPROM.write(1,0);
  }
  
  digitalWrite(GREENLed, LOW);
  digitalWrite(REDLed, LOW);
  if (digitalRead(PosUp)==HIGH)
  {
    RedLedState=true;
    LastRedLedState=false;
    if (!blinkLed)
    {
    digitalWrite(GREENLed, LOW);
    digitalWrite(REDLed, HIGH);
    }
  }
  else
  {
    RedLedState=false;
    LastRedLedState=true;
    if (!blinkLed)
    {
      digitalWrite(GREENLed, HIGH);
      digitalWrite(REDLed, LOW);
    }
  }
  lcd.setCursor(2,0);
  lcd.print(":");
}

void loop() 
{
  if ((AlarmHour==hour()) && (AlarmMinute==minute()) && (RedLedState==true))
  {
    while (digitalRead(PosUp)==HIGH)
    {
      lcd.setCursor(0,1);
      lcd.print("Time to wake up!");
      for (int i=0; i<3; i++)
        Buzzer();
      delay(100);
      lcd.setCursor(0,1);    
      lcd.print("                ");
      for (int i=0; i<3; i++)
        Buzzer();
      delay(100);
    }
    RedLedState=false;
    LastRedLedState=true;
  }
  
  
  
  
  if ((digitalRead(PosUp)==HIGH) && (LastRedLedState==false))
  {
    if (!blinkLed)
    {
      digitalWrite(GREENLed, LOW);
      digitalWrite(REDLed, HIGH);
    }
    RedLedState=true;
    LastRedLedState=true;
    lcd.setCursor(0,1);
    lcd.print("Wake up in   :  ");
    CurrentRemainingHour = -70;
    CurrentRemainingMinute = -70;
    Display=true;
  }
  if ((digitalRead(PosUp)==LOW) && (LastRedLedState==true))
  {
    if (!blinkLed)
    {
      digitalWrite(GREENLed, HIGH);
      digitalWrite(REDLed, LOW);
    }
    RedLedState=false;
    LastRedLedState=false;
    lcd.setCursor(0,1);
    lcd.print("       /  /     ");
    LastRedLedState=false;
    CurrentYear=-70;
    CurrentMonth=-70;
    CurrentDay=-70;
    CurrentWeekday=-70;
    Display=true;
  }
  
  if ((RedLedState==true) && (LastRedLedState==true))
  {
    int x=hour()-AlarmHour;
    if (x<0)
      x+=24;
    if (x<=6)
    {
      CountDown();
    }
    else
    {
      DisplaySomething();
    }
  }
  if ((RedLedState==false) && (LastRedLedState==false))
  {
    DisplayWeekDay();
  }

   
  if (millis()-BlinkMillis > 500)
  {
    if (BlinkDot)
    {
      if (blinkLed)
      {
        LedOn();
      }
      else
      {
        lcd.setCursor(2,0);
        lcd.print(":");
      }        
    }
    else
    {
      if (blinkLed)
      {
        LedOff();
      }
      else
      {
        lcd.setCursor(2,0);
        lcd.print(" ");
      }
    }
    BlinkDot=!BlinkDot;
    BlinkMillis = millis();
  }
  
  if ((millis()-StartMillis) > UpdateTemp)
  {
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if (!isnan(t))
    {
      if (CurrentTemp != t)
      {
        lcd.setCursor(8,0);
        if (t<10)
          lcd.print("0");
        lcd.print(t,0);
        CurrentTemp = t;
      }
    }
    if (!isnan(h))
    {
      if (CurrentHum != h)
      {
        lcd.setCursor(13,0);
        if (h<10)
          lcd.print("0");
        lcd.print(h,0);
        CurrentHum = h;
      }
    }
    StartMillis = millis();
  }

  PrintTime();


  EthernetClient client = server.available();
  if (client)
  {
    while (client.connected()) 
    {
      lcd.setCursor(0,1);
      lcd.print("** WEB ACCESS **");
      boolean currentLineIsBlank = true;
      if (client.available()) 
      {
        char c = client.read();
        if (readString.length() < 100) 
        {
          readString += c; 
        } 
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          
          if (readString.indexOf("?") > -1)
          {
            int Pos_Hour = readString.indexOf("H");
            int Pos_Minute = readString.indexOf("M", Pos_Hour);
            int End = readString.indexOf("H", Pos_Minute);
            if(End < 0)
            {
              End =  readString.length() + 1;
            }
            int bufLength = ((Pos_Minute) - (Pos_Hour+2));
            if (bufLength>3)
              bufLength=3;
            char tmpBuf[3];
            readString.substring((Pos_Hour+2), (Pos_Minute-1)).toCharArray(tmpBuf, bufLength);
            AlarmHour=atoi(tmpBuf);
            bufLength = ((End) - (Pos_Minute+2));
            if (bufLength > 3)
              bufLength = 3;
            readString.substring((Pos_Minute+2), (End-1)).toCharArray(tmpBuf, bufLength);
            AlarmMinute=atoi(tmpBuf);         
            EEPROM.write(0,AlarmHour);
            EEPROM.write(1,AlarmMinute);   
            CurrentRemainingHour=-70;
            CurrentRemainingMinute=-70;
            CurrentDay=-70;
            CurrentWeekday=-70;
            CurrentYear=-70;
            Display=true;
            RefreshState();
          }
          
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connnection: close");
          client.println();
          client.println("<html>");
                    // add a meta refresh tag, so the browser pulls again every 5 seconds:
//          client.println("<meta http-equiv=\"refresh\" content=\"180\" charset=\"UTF-8\">");
          client.println("<font face=\"Tahoma, Arial, Helvetica\" size=\"4\">");
//          client.println("<STYLE TYPE=\"text/css\">");
          client.println("<BODY>");
          client.print("Current Time: ");
          if (hour()<10)
            client.print("0");
          client.print(hour());
          client.print(":");
          if (minute()<10)
            client.print("0");
          client.print(minute());
          client.println("<BR>");
          client.print("Alarm: ");
          if (AlarmHour<10)
            client.print("0");
          client.print(AlarmHour);
          client.print(":");
          if (AlarmMinute<10)
            client.print("0");
          client.print(AlarmMinute);
          client.println("<BR><BR><BR>");
          client.println("<form method=get>Hour:<input type=text size=2 name=H> Min:<input type=text size=2 name=M>&nbsp;<input name=E type=submit value=Submit></form>");
          client.println("</body>");
          client.println("</html>");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } 
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    readString="";
    delay(100);
    LastRedLedState=!LastRedLedState;
  }  
  
  
  if (((hour()==3) && (minute()==0)) || ((hour()==15) && (minute()==0)))
  {
    resetEthernet();
  }

}





void PrintTime()
{
  if (CurrentHour!=hour())
  {
    lcd.setCursor(0,0);
    if (hour()<10)
      lcd.print("0");
    lcd.print(hour());
    CurrentHour=hour();
  }
  if (CurrentMin!=minute())
  {
    lcd.setCursor(3,0);
    if (minute()<10)
      lcd.print("0");
    lcd.print(minute());
    CurrentMin=minute();
  }
}



// --------------------------------------------------------------------------------------------------------------------------------------------------------
//                          NTP RELATED

void SynchronizeRTCwithNTP()
{
  Udp.begin(8888);
  setSyncProvider(RTC.get);
  ntpupdate();
  RTC.set(unixtime+(2*60*60));   // set the RTC and the system time to the received value
  setTime(unixtime+(2*60*60));   
}

void ntpupdate()
{
  sendNTPpacket(timeServer);
  delay(1000);  
  if ( Udp.parsePacket() ) {  
  Udp.read(packetBuffer,NTP_PACKET_SIZE);  // read the packet into the buffer
  unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
  unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);  
  unsigned long secsSince1900 = highWord << 16 | lowWord;  
  const unsigned long seventyYears = 2208988800UL;    
  unsigned long epoch = secsSince1900 - seventyYears;  
  unixtime = epoch;    // Vazoyme sto unixtime thn wra se unix time format
  }
}

unsigned long sendNTPpacket(IPAddress& address)
{
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  Udp.beginPacket(address, 123);
  Udp.write(packetBuffer,NTP_PACKET_SIZE);
  Udp.endPacket();
}  

// --------------------------------------------------------------------------------------------------------------------------------------------------------



void resetEthernet() {
  client.stop();
  delay(1000);
  Ethernet.begin(mac, ip, gateway, subnet);
  delay(1000);
}


void Buzzer()
{
  digitalWrite(BuzzerPin, HIGH);
  delay(50);
  digitalWrite(BuzzerPin, LOW);
  delay(50);
}


void CountDown()
{
  RemainingHour=AlarmHour-hour();
  if (RemainingHour<0)
    RemainingHour+=24;
    
  RemainingMinute=AlarmMinute-minute();
  if (RemainingMinute<0)
  {
    RemainingMinute+=60;
    RemainingHour-=1;
    if (RemainingHour<0)
      RemainingHour+=24;
  }
  
  if ((RemainingHour==0) && (RemainingMinute==1))
  {
    lcd.setCursor(11,1);
    int x=60-second();
    if (x<10)
      lcd.print("0");
    lcd.print(x);
    lcd.print("\"  ");
    return;
  }
  
  if (CurrentRemainingHour!=RemainingHour)
  {
    lcd.setCursor(11,1);
    if (RemainingHour<10)
      lcd.print("0");
    lcd.print(RemainingHour);
    CurrentRemainingHour=RemainingHour;
  }
  if (CurrentRemainingMinute!=RemainingMinute)
  {
    lcd.setCursor(14,1);
    if (RemainingMinute<10)
      lcd.print("0");
    lcd.print(RemainingMinute);
    CurrentRemainingMinute=RemainingMinute;
  }
}




void DisplayWeekDay()
{
  if (CurrentWeekday!=weekday())
  {
    lcd.setCursor(1,1);
    switch (weekday())
    {
      case 1:
        lcd.print("KYP");
        break;
      case 2:
        lcd.write(2);
        lcd.print("EY");
        break;
      case 3:
        lcd.print("TPI");
        break;
      case 4:
        lcd.print("TET");
        break;
      case 5:
        lcd.write(3);
        lcd.print("EM");
        break;
      case 6:
        lcd.write(3);
        lcd.print("AP");
        break;
      case 7:
        lcd.write(4);
        lcd.print("AB");
        break;
    }
    CurrentWeekday=weekday();
  }
  if (CurrentDay!=day())
  {
    lcd.setCursor(5,1);
    if (day()<0)
      lcd.print("0");
    lcd.print(day());
    CurrentDay=day();
  }
  if (CurrentMonth!=month())
  {
    lcd.setCursor(8,1);
    if (month()<0)
      lcd.print("0");
    lcd.print(month());
    CurrentMonth=month();
  }
  if (CurrentYear!=year())
  {
    lcd.setCursor(11,1);
    lcd.print(year());
    CurrentYear=year();
  }
}

void LedOn()
{
  if (RedLedState==true)
  {
    digitalWrite(REDLed, HIGH);
    digitalWrite(GREENLed, LOW);
  }
  else
  {
    digitalWrite(GREENLed, HIGH);
    digitalWrite(REDLed, LOW);
  }
}

void LedOff()
{
  digitalWrite(GREENLed, LOW);
  digitalWrite(REDLed, LOW);
}


void RefreshState()
{
  if (digitalRead(PosUp)==HIGH)
  {
    RedLedState=true;
    LastRedLedState=true;
    lcd.setCursor(0,1);
    lcd.print("Wake up in   :  ");
    CurrentRemainingHour = AlarmHour - 3;
    CurrentRemainingMinute = AlarmMinute - 3;
    Display=true;
  }
  else
  {
    RedLedState=false;
    LastRedLedState=false;
    lcd.setCursor(0,1);
    lcd.print("       /  /     ");
    LastRedLedState=false;
    CurrentYear=0;
    CurrentMonth=0;
    CurrentDay=0;
    CurrentWeekday=0;
    Display=true;
  }
}

void DisplaySomething()
{
  if (Display)
  {
    lcd.setCursor(0,1);
    if ((hour()>=9) && (hour()<13))
    {
      lcd.print("   KA");
      lcd.write(5);
      lcd.print("HMEPA!    ");
    }
    if ((hour()>=13) && (hour()<17))
    {
      lcd.print(" KA");
      lcd.write(5);
      lcd.print("O ME");
      lcd.write(4);
      lcd.print("HMEPI! ");
    }
    if ((hour()>=17) && (hour()<20))
    {
      lcd.print(" KA");
      lcd.write(5);
      lcd.print("O A");
      lcd.write(3);
      lcd.print("O");
      lcd.write(6);
      lcd.print("EYMA! ");
    }
    if ((hour()>=20) && (hour()<24))
    {
      lcd.print("  KA");
      lcd.write(5);
      lcd.print("O BPA");
      lcd.write(2);
      lcd.print("Y!   ");
    }
    if ((hour()>=0) && (hour()<9))
    {
      lcd.print("   KA");
      lcd.write(5);
      lcd.print("HNYXTA!   ");
    }
    Display=false;
  }
}

