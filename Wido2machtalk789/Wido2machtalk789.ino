
#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>

#include <JsonParser.h>
#include <Servo.h>


#define Wido_IRQ   7
#define Wido_VBAT  5
#define Wido_CS    10

Adafruit_CC3000 Wido = Adafruit_CC3000(Wido_CS, Wido_IRQ, Wido_VBAT,SPI_CLOCK_DIVIDER); 
Adafruit_CC3000_Client WidoClient;

#define WLAN_SECURITY   WLAN_SEC_WPA2


#define WLAN_SSID       "zhoutianqi1"           // cannot be longer than 32 characters!
#define WLAN_PASS       "w12345678"          // For connecting router or AP, don't forget to set the SSID and password here!!


#define TCP_TIMEOUT      3000
//#define CC3000_TINY_DRIVER

#define WEBSITE  "api.machtalk.net"
#define API_key  "7a19bd7874a541a6b4c50a831ea0b3b2"  
                                                    

uint32_t ip = 0;
                       

unsigned long last = 0;
unsigned long lasttime = 0;
String returnvalue="";
String receivevalue="";
String lastreceive="";
boolean Response = true;
int counter=0;
int mid=1;
unsigned long lastRead = millis();


char json[120];

Servo myservo;
void setup(){
  Serial.begin(115200);
  pinMode(13, OUTPUT); 
}


void loop(){

  char strmid[3];
  char strtemp[3];

 if(!Wido.checkConnected())
 {

   Wido.begin();
  if (!Wido.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println(F("Failed!"));
    while(1);
  }
  Serial.println("debug2");
  while (!Wido.checkDHCP())
  {
    delay(100); // ToDo: Insert a DHCP timeout!
  }  
  static unsigned long RetryMillis = 0;

 // lian jie ip
 delay(5000);
  if(!WidoClient.connected() && millis() - RetryMillis > TCP_TIMEOUT)
  {
    RetryMillis = millis();
    WidoClient.close();
    ip =Wido.IP2U32(120,132,49,137);
    WidoClient = Wido.connectTCP(ip, 6779);     
    Serial.println("debug3");    
  }
 //fa song deng lu cao zuo 
  delay(1000);
  WidoClient.fastrprintln(F("{\"cmd\":\"login\",\"mid\":\"1\",\"type\":2,\"usr\":\"552cfd991f714c2bbef6b923be4a8543\",\"pwd\":\"1493e8729227186d9d43fb59b2e58afb\"}"));

  //jie shou lian jie fu wu qi (ip port) jie xi
   recv();
   JsonParser<32> parser;
   JsonHashTable hashTable = parser.parseHashTable(json);
   hashTable.success(); 

    char* ipok = hashTable.getString("ip");  
    uint8_t a,b,c,d;
    sscanf(ipok,"%d.%d.%d.%d",&a,&b,&c,&d); 
    ip =Wido.IP2U32(a,b,c,d);
    
    char* port=hashTable.getString("port");
    uint16_t portok=atoi(port);
    WidoClient.close();
    
    ip =Wido.IP2U32(120,132,49,137);
    WidoClient = Wido.connectTCP(ip, portok);     
    delay(1000);
  // que ren lian jie  
    WidoClient.fastrprint(F("{\"cmd\":\"connect\",\"mid\":\""));
    mid+=1;
    itoa(mid,strmid,10); 
    WidoClient.fastrprint(strmid);
    WidoClient.fastrprintln(F("\",\"usr\":\"552cfd991f714c2bbef6b923be4a8543\",\"type\":2,\"encode\":\"\"}"));  
 }
 else
 {
    static unsigned long lasttime = 0;      
    static unsigned long last = 0; 
    static unsigned long sensorstamp = 0; 

    if(WidoClient.available())
     {
        char c = WidoClient.read();
        if(c=='{')
        Response=true;
        else if(c =='}')
        {
        Response=false;
        returnvalue+=c;
        }
        if (Response)
        returnvalue +=c;                  
     }      
    if(millis()-lasttime>10000)
      {
        lasttime=millis();
        WidoClient.fastrprintln("{1}");
      }
        
     if(returnvalue.length() !=0&&Response==false)
     {       
      Serial.println(returnvalue);       
      int i;  

      //jie xi
      for(i=0;i<returnvalue.length();i++)
      json[i]=returnvalue[i];
      json[i]='\0';
      returnvalue="";    
      JsonParser<32> parser;
      JsonHashTable hashTable = parser.parseHashTable(json);
      hashTable.success(); 
      char* scmd = hashTable.getString("cmd");
      char* smid = hashTable.getString("mid");
      char* svalue = hashTable.getString("value");  
      char* sdvid=hashTable.getString("dvid");
      int idvid = hashTable.getLong("dvid");


      if(strcmp(scmd,"opt")==0)
      {

         WidoClient.fastrprint(F("{\"cmd\":\"post\",\"from\":\"552cfd991f714c2bbef6b923be4a8543\",\"values\":[{\"dvid\":"));
         WidoClient.fastrprint(sdvid);
         WidoClient.fastrprint(F(",\"value\":\""));
         WidoClient.fastrprint(svalue);
         WidoClient.fastrprint(F("\"}],\"needack\":1,\"mid\":\""));
         WidoClient.fastrprint(smid);
         WidoClient.fastrprintln(F("\"}"));        
       if(idvid==2)
        {
          int b;
          b=atoi(svalue);
          digitalWrite(13, b);         
        }
        
      }
      

     }
   
    if(millis()-sensorstamp>10000)
     {
      sensorstamp=millis();
      int reading = analogRead(0);
  
      
      WidoClient.fastrprint(F("{\"cmd\":\"post\",\"from\":\"552cfd991f714c2bbef6b923be4a8543\",\"values\":[{\"dvid\":1,\"value\":\""));
      itoa(reading,strtemp,10); 
      WidoClient.fastrprint(strtemp);
      WidoClient.fastrprint(F("\"}],\"needack\":1,\"mid\":\""));
      mid+=1;  
      itoa(mid,strmid,10); 
      WidoClient.fastrprint(strmid);
      WidoClient.fastrprintln(F("\"}"));
      
     }              
    
  
 }
 
}
 void recv(){
 unsigned long rTimer = millis();
 while(millis() - rTimer < 2000)
 {
  while (WidoClient.connected() && WidoClient.available())
   {
        char c = WidoClient.read();
        if(c =='{')
        Response=true;
        else if(c =='}')
        {
        Response=false;
        Response+=c;
        }
        if (Response)
        returnvalue +=c;        
   }
 }    
  if (returnvalue.length() !=0)
  {        
    int i;  
    Serial.print(returnvalue);
    for(i=0;i<returnvalue.length();i++)
    json[i]=returnvalue[i];
    json[i]='\0';
    returnvalue="";    
  } 
 }  
 


