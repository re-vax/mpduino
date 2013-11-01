/* TODO

add ethernet link status: get signal from the LED on the shield and plug it on the mega

*/



#include <os_wrap.h>
#include <SD.h>
#include <Dhcp.h>
#include <Dns.h>
#include <Ethernet.h>
#include <EthernetClient.h>
#include <EthernetServer.h>
#include <EthernetUdp.h>
#include <util.h>
#include <SPI.h>
#include <UTFT.h>
#include <UTouch.h>
#include "gui.h"
//*******************************

enum mpd_client_status {
  MPD_CLIENT_UNKNOWN,
  MPD_CLIENT_CONNECTING,
  MPD_CLIENT_DISCONNECTED,
  MPD_CLIENT_CONNECTED
};

enum gui_screen {
  GUI_INIT,
  GUI_CONNECT,
  GUI_MPC_PLAYING
};


//*******************************
// System
//*******************************

osw_task exec;
osw_dt_timer dt10sec;

gui_screen current_displayed_gui_screen=GUI_INIT;

//*******************************



//*******************************
// GFX and LCD 
//*******************************

// Declare which fonts we will be using
extern uint8_t SmallFont[];
extern uint8_t BigFont[];
extern unsigned int background[16];
extern unsigned int btn1[2528];


// Uncomment the next two lines for the Arduino 2009/UNO
//UTFT        myGLCD(ITDB24D,19,18,17,16);   // Remember to change the model parameter to suit your display module!
//UTouch      myTouch(15,10,14,9,8);

// Uncomment the next two lines for the Arduino Mega
UTFT        myGLCD(ITDB32S, 38,39,40,41);   // Remember to change the model parameter to suit your display module!
UTouch      myTouch(6,5,4,3,2);

// TouchScreen
int touch_x, touch_y;

// GUI

GUI_Screen gui_test=GUI_Screen();


//*******************************
// Ethernet
//*******************************

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress local_ip(192,168,90, 177);

IPAddress mpd_server_ip(192,168,90,10);
int mpd_server_port=6600;

// Initialize the Ethernet server library
// with the IP address and port you want to use 
// (port 80 is default for HTTP):
//EthernetServer server(80);

EthernetClient mpd_client;
int mpd_client_connected=MPD_CLIENT_UNKNOWN;
boolean fake_connected=false;


//*******************************
// SD Card
//*******************************

// set up variables using the SD utility library functions:
Sd2Card card;
SdVolume volume;
SdFile root;

// change this to match your SD shield or module;
// Arduino Ethernet shield: pin 4
// Adafruit SD shields and modules: pin 10
// Sparkfun SD shield: pin 8
// vAx Hack: Ethernet shield with LCD touch screen : pin 9
const int chipSelect = 9;    






/*************************
**   Custom functions   **
*************************/


//************************
//   ====--GUI--====
//************************

void draw_GUI(){

  switch (current_displayed_gui_screen) {
    case GUI_INIT:
      draw_gui_init();
    break;
    case GUI_CONNECT:
      draw_gui_connect();
    break;
    case GUI_MPC_PLAYING:
      draw_gui_mpc_playing();
    break;
  }
}



void draw_gui_init() {
  myGLCD.fillScr(VGA_GRAY);
  myGLCD.setFont(SmallFont);
  myGLCD.setBackColor(VGA_GRAY);
  myGLCD.setColor(VGA_BLACK);
  myGLCD.drawRoundRect (1, 1, 318, 238);
  myGLCD.print("MP-Duino-mote v0.1",CENTER,50);
  myGLCD.print("(c) Dr.vAx 2057",CENTER,70);
}

void draw_gui_connect() {
  myGLCD.clrScr();
}

void draw_gui_mpc_playing() {
  myGLCD.fillScr(VGA_BLACK);
//  draw_MPC_Buttons();
Serial.println("draw_gui_mpc_playing");

  GUI_Button *button_status=new GUI_Button(10,10,130,40,"STATUS");
  GUI_Button *button_test=new GUI_Button(10,110,130,140,"Test!");
  
//  button_status->draw(myGLCD);
  gui_test.add(button_status);
  gui_test.add(button_test);
  Serial.println("button added type is:");
  Serial.println(button_status->type);
  Serial.println(gui_test.list_obj());
  int c= gui_test.draw(myGLCD);
  Serial.print("draw count: ");
  Serial.println(c);

}


void init_GUI_MPC(){
}

void draw_MPC_Buttons(){
  
//  myGLCD.drawBitmap (10, 100, 79, 32, btn1);
//  myGLCD.setColor(VGA_YELLOW);
//  myGLCD.drawLine(0,0,50,20);
//  button_status.draw();

}


void draw_background(){
  int x,y;
  for (y=0; y<(240/4)-1; y++) {
    for (x=0; x<(320/4)-1; x++) {
      myGLCD.drawBitmap (x*4, y*4, 4, 4, background);
    }
  }  
}



void process_touch_screen(){
  if (myTouch.dataAvailable()) {
    myTouch.read();
    touch_x=myTouch.getX();
    touch_y=myTouch.getY();

    Serial.println(gui_test.list_obj());

    GUI_Object * obj = gui_test.test_touch(touch_x,touch_y);
    if (obj!=NULL) {
//      getSongTitle();
        Serial.print("obj type:");
        Serial.println(obj->type);
    } else {
      Serial.println("null obj returned by test_touch :(");
    }
      
      /*
      if ((touch_y>=10) && (touch_y<=60))  // Upper row
      {
        if ((touch_x>=10) && (touch_x<=100))  // Button: 1
        {
          //waitForIt(10, 10, 100, 60);
//          updateStr('1');
//            mpd_client.println("pause");            
            getSongTitle();
        }
      }*/
      
    }
}



// Draw a red frame while a button is touched
void waitForIt(int x1, int y1, int x2, int y2)
{
  myGLCD.setColor(255, 0, 0);
  myGLCD.drawRoundRect (x1, y1, x2, y2);
  while (myTouch.dataAvailable())
    myTouch.read();
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (x1, y1, x2, y2);
}


void sdCardInit(){
    // we'll use the initialization code from the utility libraries
  // since we're just testing if the card is working!
  if (!card.init(SPI_HALF_SPEED, chipSelect)) {
    Serial.println("initialization failed. Things to check:");
    Serial.println("* is a card is inserted?");
    Serial.println("* Is your wiring correct?");
    Serial.println("* did you change the chipSelect pin to match your shield or module?");
    return;
  } else {
   Serial.println("Wiring is correct and a card is present."); 
  }

  // print the type of card
  Serial.print("\nCard type: ");
  switch(card.type()) {
    case SD_CARD_TYPE_SD1:
      Serial.println("SD1");
      break;
    case SD_CARD_TYPE_SD2:
      Serial.println("SD2");
      break;
    case SD_CARD_TYPE_SDHC:
      Serial.println("SDHC");
      break;
    default:
      Serial.println("Unknown");
  }

  // Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
  if (!volume.init(card)) {
    Serial.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
    return;
  }


  // print the type and size of the first FAT-type volume
  uint32_t volumesize;
  Serial.print("\nVolume type is FAT");
  Serial.println(volume.fatType(), DEC);
  Serial.println();
  
  volumesize = volume.blocksPerCluster();    // clusters are collections of blocks
  volumesize *= volume.clusterCount();       // we'll have a lot of clusters
  volumesize *= 512;                            // SD card blocks are always 512 bytes
  Serial.print("Volume size (bytes): ");
  Serial.println(volumesize);
  Serial.print("Volume size (Kbytes): ");
  volumesize /= 1024;
  Serial.println(volumesize);
  Serial.print("Volume size (Mbytes): ");
  volumesize /= 1024;
  Serial.println(volumesize);

  
  Serial.println("\nFiles found on the card (name, date and size in bytes): ");
  root.openRoot(volume);
  
  // list all files in the card with date and size
  root.ls(LS_R | LS_DATE | LS_SIZE);
}

/*
void process_ethernet_server(){
  EthernetClient client = server.available();
  if (client) {
    Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
                    // add a meta refresh tag, so the browser pulls again every 5 seconds:
          client.println("<meta http-equiv=\"refresh\" content=\"5\">");
          // output the value of each analog input pin
          for (int analogChannel = 0; analogChannel < 6; analogChannel++) {
            int sensorReading = analogRead(analogChannel);
            client.print("analog input ");
            client.print(analogChannel);
            client.print(" is ");
            client.print(sensorReading);
            client.println("<br />");       
          }
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
    Serial.println("client disonnected");
  }
}
*/




void connect_mpd_client(){
  if (mpd_client_connected!=MPD_CLIENT_CONNECTED) {
    // Trying to connnect
    // connect client to mpd server

    mpd_client_connected=MPD_CLIENT_CONNECTING;
    myGLCD.setColor(255, 255, 255);
    myGLCD.print("Connecting to MPD server...", CENTER, 100);

    if (mpd_client.connect(mpd_server_ip, mpd_server_port)) {
      Serial.println("Connected to MPD server");
    }
    else {
      // if you didn't get a connection to the server:
      Serial.println("Connection to MPD server failed");
    }



  }

}


void update_mpd_client_status(){
  if (mpd_client.connected()|| fake_connected) {
    // is connected
    if (mpd_client_connected!=MPD_CLIENT_CONNECTED ) {
      // but was not in previous test
      // we need to update the display

      myGLCD.setColor(0, 255, 0);
      myGLCD.print("Connected", CENTER, 100);
      mpd_client_connected=MPD_CLIENT_CONNECTED;
      current_displayed_gui_screen=GUI_MPC_PLAYING;
      draw_GUI();

    }
  } else {
    // is not connected
    if (mpd_client_connected!=MPD_CLIENT_DISCONNECTED) {
      // but was in previous test
      // we need to update the display
      current_displayed_gui_screen=GUI_CONNECT;
      draw_GUI();
      myGLCD.setColor(255, 0, 0);
      if (mpd_client_connected==MPD_CLIENT_CONNECTED){
        myGLCD.print("Lost connection to MPD server!", CENTER , 100);
      } else {
        myGLCD.print("Unable to connect to MPD server!", CENTER , 100);
      }
      mpd_client_connected=MPD_CLIENT_DISCONNECTED;
   }
  }
}


void process_ethernet_mpd_client(){
  
  
}


void getSongTitle(){
// Will became getStatus when ok to parse all the answer

    mpd_client.println("currentsong");
//    mpd_client.println("command_list\nstatus\ncurrentsong\ncommand_list_end");

// Wait for the answer
  int maxLoop=20;
  while (maxLoop>0 && !mpd_client.available()) {
    Serial.println("waiting loop");
    delay(100);
    maxLoop--;
  }

   // if there are incoming bytes available
  // from the server, read them and print them:

  int currentLCD_Line=1;
  
  String currentLine=String();
  if (mpd_client.available()) {
    while (mpd_client.available())
    {
      char c = mpd_client.read();
      if (c==10) {
         if (currentLCD_Line<10) {
           if (currentLine.startsWith(String("Artist:")) || currentLine.startsWith("Title:") || currentLine.startsWith("Album:") || currentLine.startsWith("Date:")) {
             myGLCD.print(currentLine,5,50+currentLCD_Line*10);
             currentLCD_Line++;
           } 
           currentLine=String();
         }
       
      } else { 
       currentLine=String(currentLine+c);
      }
    }
//    Serial.print(c);
  } else {
    Serial.println("mpd client not ready");
  }
}

//***************************
// MPDuino 'OS Like' functions
//***************************

void executive_init(void)
{
  dt10sec.start(100); // First time is only 100ms
  osw_evt_register(1, evt10s);
}

void* executive(void* _pData)
{
  static int count = 0;
//  Serial.println("executive");
  if (dt10sec.timedOut()) {
//  Serial.println("publish1");
    osw_evt_publish(1);
    dt10sec.start(10000);
  }
  
  process_touch_screen();
}


void evt10s(int _evt)
{
//  Serial.println("evt10s");
  check_mpd_client();
}


/*************************
**  Required functions  **
*************************/

void setup()
{
  
   Serial.begin(9600);
   while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
   }

  pinMode(53, OUTPUT);     // change this to 53 on a mega
   
  // start the Ethernet connection
  Ethernet.begin(mac, local_ip);
//  and the server:
//  server.begin();
//  Serial.print("server is at ");
//  Serial.println(Ethernet.localIP());

   
// Initial LCD/Touch setup
  myGLCD.InitLCD();

  myTouch.InitTouch();
  myTouch.setPrecision(PREC_MEDIUM);

  init_GUI_MPC();
  draw_GUI();
  
  // sd card initialisation
//  sdCardInit();

  //  GFX init
//  draw_background();
//  draw_MPC_Buttons();

  delay(2000);

  // System
  executive_init();
  exec.taskCreate("exec", executive);
//  osw_list_tasks();
  current_displayed_gui_screen=GUI_CONNECT;
  draw_GUI();
  
}



void check_mpd_client() {
  if (mpd_client_connected!=MPD_CLIENT_CONNECTED) {
    connect_mpd_client(); 
  }
  update_mpd_client_status();
}


void loop()
{
  
  osw_tasks_go();

/*
  while (true)
  {
    

//    process_ethernet_server();
//    process_ethernet_mpd_client();
  
  
//             mpd_client.println("currentsong");
   
    
//    delay(100);
  }*/
}


