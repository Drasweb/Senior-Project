//for wifi

#include "mbed.h"
#include "wifi.h"

//for rfid
#include "mbed.h"
#include "MFRC522.h"


/*------------------------------------------------------------------------------
Hyperterminal settings: 115200 bauds for wifi, 8-bit data, no parity

This example 
  - connects to a wifi network (SSID & PWD to set in mbed_app.json)
  - displays the IP address and creates a web page
  - then connect on its IP address on the same wifi network with another device
  - Now able to change the led status and read the temperature

This example uses SPI3 ( PE_0 PC_10 PC_12 PC_11), wifi_wakeup pin (PB_13), 
wifi_dataready pin (PE_1), wifi reset pin (PE_8)

//Test of cheap 13.56 Mhz RFID-RC522 module from eBay
//This code is based on Martin Olejar's MFRC522 library. Minimal changes
//Adapted for Nucleo STM32 F401RE. Should work on other Nucleos too
 
//Connect as follows:
//RFID pins        ->  Nucleo header CN5 (Arduino-compatible header)
//----------------------------------------
//RFID IRQ=pin5    ->   Not used. Leave open
//RFID MISO=pin4   ->   Nucleo SPI_MISO=PA_6=D12
//RFID MOSI=pin3   ->   Nucleo SPI_MOSI=PA_7=D11
//RFID SCK=pin2    ->   Nucleo SPI_SCK =PA_5=D13
//RFID SDA=pin1    ->   Nucleo SPI_CS  =PB_6=D10
//RFID RST=pin7    ->   Nucleo         =PA_9=D8
//3.3V and Gnd to the respective pins    



------------------------------------------------------------------------------*/

/* Private defines -----------------------------------------------------------*/
#define WIFI_WRITE_TIMEOUT 10000
#define WIFI_READ_TIMEOUT  10000
#define PORT           80

//define for rfid
#define MF_RESET    D8




/* Private typedef------------------------------------------------------------*/
typedef enum
{
  WS_IDLE = 0,
  WS_CONNECTED,
  WS_DISCONNECTED,
  WS_ERROR,
} WebServerState_t;

/* Private macro -------------------------------------------------------------*/
static int wifi_sample_run(void);
static void WebServerProcess(void);
static WIFI_Status_t SendWebPage(uint8_t ledIsOn, float temperature);



/* Private variables ---------------------------------------------------------*/
Serial pc(SERIAL_TX, SERIAL_RX);
static   uint8_t http[1024];
static   uint8_t resp[1024];
uint16_t respLen;
uint8_t  IP_Addr[4]; 
uint8_t  MAC_Addr[6]; 
int32_t Socket = -1;
static   WebServerState_t  State = WS_ERROR;
char     ModuleName[32];

DigitalOut led(LED2);
DigitalOut LedGreen(LED1);



//rfid
MFRC522    RfChip   (SPI_MOSI, SPI_MISO, SPI_SCK, SPI_CS, MF_RESET);
//RTC_DS1307 RTC;  // define RTC class

///vairables
int a = 0;
int led_pos = 3; // green LED on pin 3
int led_neg = 2; // red LED on pin 2
//String UID_tagA = "856a8b45";  // UID of tag that we are using
unsigned int MinsA = 0, HoursA = 0;  // working minutes and hours for tag A
//String readTag = "";  
int readCard[4];
short UIDs_No = 1;
//boolean TimeFlag[2] = {false, false};
//DateTime arrival[2];  // tiem class for arrival
//DateTime departure[2];  // time class for departure
int LastMonth=0;  // working hours till now in a month
char DataRead=0;

// Declaration of the functions
void redLED(); // red LED on
void greenLED(); // green LED + buzzer on
int getID();  // read tag
//boolean checkTag();  // check if tag is unknown
void errorBeep();  // error while reading (unknown tag)
void StoreData();  // store data to microSD

File myFile; // class file for reading/writing to file on SD card




int main()
{
    pc.baud(9600);
 //   SD.begin(4);  // start SD library
 //   pinMode(led_pos, OUTPUT);
 //   pinMode(led_neg, OUTPUT);
    
    int ret = 0;
    led = 0;
    pc.baud(115200);
    printf("\n");
    printf("************************************************************\n");
    printf("***                     Hello Class                       ***\n");
    printf("***         WIFI Web Server demonstration                ***\n\n");
    printf("*** Copy the IP address on another device connected      ***\n");
    printf("*** to the wifi network                                  ***\n");
    printf("*** Read the rfid and update the LED status              ***\n");
    printf("************************************************************\n");
    
    
      pc.printf("starting up RFID and wifi\n");
      pc.printf("proffessor please scan tag\n");
      
     
     
     
    /* Working application */
    ret = wifi_sample_run();
    
    if (ret != 0) {
        return -1;
        
    }

    
    while(1) {
        WebServerProcess ();
        RfChip.PCD_Init();
        

     

 
  while (true) {
    LedGreen = 1;
 
    // Look for new cards
    if ( ! RfChip.PICC_IsNewCardPresent())
    {
      wait_ms(500);
      continue;
    }
 
    // Select one of the cards
    if ( ! RfChip.PICC_ReadCardSerial())
    {
      wait_ms(500);
      continue;
    }
 
    LedGreen = 0;
 
    // Print Card UID
    pc.printf("Card UID: ");
    for (uint8_t i = 0; i < RfChip.uid.size; i++)
    {
      pc.printf(" %X02", RfChip.uid.uidByte[i]);
    }
    pc.printf("\n\r");
 
    // Print Card type
    uint8_t piccType = RfChip.PICC_GetType(RfChip.uid.sak);
    pc.printf("PICC Type: %s \n\r", RfChip.PICC_GetTypeName(piccType));
    wait_ms(1000);
  }
}    
    }




int wifi_sample_run(void)
{
  
    /*Initialize and use WIFI module */
    if(WIFI_Init() ==  WIFI_STATUS_OK) {
        printf("ES-WIFI Initialized.\n");
    
        if(WIFI_GetMAC_Address(MAC_Addr) == WIFI_STATUS_OK) {       
            printf("> es-wifi module MAC Address : %X:%X:%X:%X:%X:%X\n",     
                   MAC_Addr[0],
                   MAC_Addr[1],
                   MAC_Addr[2],
                   MAC_Addr[3],
                   MAC_Addr[4],
                   MAC_Addr[5]);   
        } else {
            printf("> ERROR : CANNOT get MAC address\n");
        }
    
        if( WIFI_Connect(MBED_CONF_APP_WIFI_SSID, MBED_CONF_APP_WIFI_PASSWORD, WIFI_ECN_WPA2_PSK) == WIFI_STATUS_OK) {
            printf("> es-wifi module connected \n");
      
            if(WIFI_GetIP_Address(IP_Addr) == WIFI_STATUS_OK) {
                printf("> es-wifi module got IP Address : %d.%d.%d.%d\n",     
                       IP_Addr[0],
                       IP_Addr[1],
                       IP_Addr[2],
                       IP_Addr[3]); 
        
                printf(">Start HTTP Server... \n");
                printf(">Wait for connection...  \n");
                State = WS_IDLE;
            } else {    
                printf("> ERROR : es-wifi module CANNOT get IP address\n");
                return -1;
            }
        } else {
            printf("> ERROR : es-wifi module NOT connected\n");
            return -1;
        }
    } else {
        printf("> ERROR : WIFI Module cannot be initialized.\n"); 
        return -1;
        
        
        
        
        
        
    }
    return 0;
}

/**
  * @brief  Send HTML page
  * @param  None
  * @retval None
  */
static void WebServerProcess(void)
{
  uint8_t LedState = 0;
  float student;
  switch(State)
  {
  case WS_IDLE:
    Socket = 0;
    WIFI_StartServer(Socket, WIFI_TCP_PROTOCOL, "", PORT);
    
    if(Socket != -1)
    {
      printf("> HTTP Server Started \n");  
      State = WS_CONNECTED;
    }
    else
    {
      printf("> ERROR : Connection cannot be established.\n"); 
      State = WS_ERROR;
    }    
    break;
    
  case WS_CONNECTED:
    
    WIFI_ReceiveData(Socket, resp, 1200, &respLen, WIFI_READ_TIMEOUT);
    
    if( respLen > 0)
    {
      if(strstr((char *)resp, "GET")) /* GET: put web page */
      {
        student = (adc_temp.read()*100);
        if(SendWebPage(LedState, temp) != WIFI_STATUS_OK)
        {
          printf("> ERROR : Cannot send web page\n");
          State = WS_ERROR;
        }
      }
      else if(strstr((char *)resp, "POST"))/* POST: received info */
      {
          if(strstr((char *)resp, "radio"))
          {          
            if(strstr((char *)resp, "radio=0"))
            {
              LedState = 0;
              led = 0;
            }
            else if(strstr((char *)resp, "radio=1"))
            {
              LedState = 1;
              led = 1;
            } 
            
           temp = (adc_temp.read()*100);
            if(SendWebPage(LedState, temp) != WIFI_STATUS_OK)
            {
              printf("> ERROR : Cannot send web page\n");
              State = WS_ERROR;
          }
        }
      }
    }
    
    if(WIFI_StopServer(Socket) == WIFI_STATUS_OK)
    {
      WIFI_StartServer(Socket, WIFI_TCP_PROTOCOL, "", PORT);
    }
    else
    {
      State = WS_ERROR;  
    }
    break;
  case WS_ERROR:   
  default:
    break;
    
    
    
      if (client) { // if HTTP request is available
//    Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
 //       Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
        //  client.println("Refresh: 10");  // refresh the page automatically every 5 sec
          client.println();
          client.println("<!DOCTYPE html>");
          client.println("<html><head><title>Office Atendance Logger</title><style>");
          client.println(".jumbotron{margin: 1% 3% 1% 3%; border: 1px solid none; border-radius: 30px; background-color: #AAAAAA;}");
          client.println(".dataWindow{margin: 1% 3% 1% 3%; border: 1px solid none; border-radius: 30px; background-color: #AAAAAA;padding: 1% 1% 1% 1%;}");
          client.println("</style></head><body style=\"background-color: #E6E6E6\">");
          client.println("<div class=\"jumbotron\"><div style=\"text-align: center\"> <h1>  Office Atendance Logger </h1> </div> ");
          client.println("</div><div class=\"dataWindow\"><div style=\"text-align: center\"> <h2> User A </h2>");
          myFile = SD.open("A.txt");
          if(myFile){
            
            while(myFile.available()){
                client.print("<p>");
                while(DataRead != 59){
                  DataRead = (char)myFile.read();
                  client.print(DataRead);
            //      client.print(myFile.read());
                }
                client.println("</p>");
                DataRead = 0;
            }  
            
            myFile.close();
          }
          client.println("</div></body></html>");    
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
//    Serial.println("client disconnected");
  }   
  delay(1000);
}

// FUNCTIONS
void redLED(){ // red LED on, green LED off
  digitalWrite(led_pos, LOW);
  digitalWrite(led_neg, HIGH);
}

void greenLED(){ // red LED off, green LED on
  digitalWrite(led_pos, HIGH);
  digitalWrite(led_neg, LOW);
 
}

boolean checkTag(){ // check if tag is unknown
  if(readTag == UID_tagA){UIDs_No = 1; return true;}
//  else if(readTag == UID_tagB){UIDs_No = 2; return true;}
  else {return false;}
}


int getID() { // Read RFID
    // Getting ready for Reading PICCs
  if ( ! mfrc522.PICC_IsNewCardPresent()) { //If a new PICC placed to RFID reader continue
    return 0;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {   //Since a PICC placed get Serial and continue
    return 0;
  }
  // There are Mifare PICCs which have 4 byte or 7 byte UID care if you use 7 byte PICC
  // I think we should assume every PICC as they have 4 byte UID
  // Until we support 7 byte PICCs
//  Serial.println(F("Scanned PICC's UID:"));
  readTag = "";
  for (int i = 0; i < 4; i++) {  //
    readCard[i] = mfrc522.uid.uidByte[i];
//    Serial.print(readCard[i], HEX);
    readTag=readTag+String(readCard[i], HEX);
  }
  // Serial.println(readTag);
//  Serial.println("");
  mfrc522.PICC_HaltA(); // Stop reading
  return 1;
}

void StoreData(){ // calculate and store data to SD card
  DateTime time = RTC.now(); // read time from RTC
  if(LastMonth != time.month()){ // check if there is a new month
    LastMonth = time.month();
    SD.remove("hoursA.txt");
  }
  switch(UIDs_No){ // this is set for multiple tags, as of right now is made only for one tag
    case 1:
      if(TimeFlag[0]){ // departure
        departure[0] = time;  // save departure time
        // calculate working hours and minutes
        int dh = abs(departure[0].hour()-arrival[0].hour()); 
        int dm = abs(departure[0].minute()-arrival[0].minute()); 
        unsigned int work = dh*60 + dm; // working hours in minutes
        MinsA = MinsA + work; // add working hours in minutes to working hours from this month
        HoursA = (int)MinsA/60; // calculate working hours from minutes
        myFile = SD.open("A.txt", FILE_WRITE); // open file with history and write to it
        if(myFile){ // format = " MM-DD-YYYY hh:mm (arrival), hh:mm (departure), hh (working hours today), hh (working hours this month);
          myFile.print(arrival[0].month(),DEC);
          myFile.print("-");
          myFile.print(arrival[0].day(),DEC);
          myFile.print("-");
          myFile.print(arrival[0].year(),DEC);
          myFile.print(" ");
          myFile.print(arrival[0].hour(),DEC);
          myFile.print(":");
          myFile.print(arrival[0].minute(),DEC);
          myFile.print(", ");
          myFile.print(departure[0].hour(),DEC);
          myFile.print(":");
          myFile.print(departure[0].minute(),DEC);
          myFile.print(", ");
          myFile.print(dh,DEC);
          myFile.print(":");
          myFile.print(dm,DEC);
          myFile.print(", ");
          myFile.print(HoursA,DEC);
          myFile.print(";");
          myFile.close();
        }
        TimeFlag[0] = false; // set time flag to false
      } else { // arrival; 
        arrival[0] = time;  // save time of arrival 
        TimeFlag[0] = true;  // set time flag to true
      }
      break;  
  }
}
  }
}


/**
  * @brief  Send HTML page
  * @param  None
  * @retval None
  */
static WIFI_Status_t SendWebPage(uint8_t ledIsOn, float temperature)
{
  uint8_t  temp[50];
  uint16_t SentDataLength;
  WIFI_Status_t ret;
  
  /* construct web page content */
  strcpy((char *)http, (char *)"HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nPragma: no-cache\r\n\r\n");
  strcat((char *)http, (char *)"<html>\r\n<body>\r\n");
  strcat((char *)http, (char *)"<title>STM32 Web Server</title>\r\n");
  strcat((char *)http, (char *)"<h2>InventekSys : Web Server using Es-Wifi with STM32</h2>\r\n");
  strcat((char *)http, (char *)"<br /><hr>\r\n");
  strcat((char *)http, (char *)"<p><form method=\"POST\"><strong>Temp: <input type=\"text\" size=2 value=\"");
  sprintf((char *)temp, "%f", temperature);
  strcat((char *)http, (char *)temp);
  strcat((char *)http, (char *)"\"> <sup>O</sup>C");
  
  if (ledIsOn) 
  {
    strcat((char *)http, (char *)"<p><input type=\"radio\" name=\"radio\" value=\"0\" >LED off");
    strcat((char *)http, (char *)"<br><input type=\"radio\" name=\"radio\" value=\"1\" checked>LED on");
  } 
  else 
  {
    strcat((char *)http, (char *)"<p><input type=\"radio\" name=\"radio\" value=\"0\" checked>LED off");
    strcat((char *)http, (char *)"<br><input type=\"radio\" name=\"radio\" value=\"1\" >LED on");
  }

  strcat((char *)http, (char *)"</strong><p><input type=\"submit\"></form></span>");
  strcat((char *)http, (char *)"</body>\r\n</html>\r\n");
  
  ret = WIFI_SendData(0, (uint8_t *)http, strlen((char *)http), &SentDataLength, WIFI_WRITE_TIMEOUT); 
  
  if((ret == WIFI_STATUS_OK) && (SentDataLength != strlen((char *)http)))
  {
    ret = WIFI_STATUS_ERROR;
  }
    
  return ret;
}





