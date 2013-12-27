/* TODO

add ethernet link status: get signal from the LED on the shield and plug it on the mega

*/

/*
Used pin 

TFT D29->D41
UTouch D2->D6
Ethernet+SD card:   D9,D10,D50->D52 + D53:reserved  (MEGA D10 -> ETHERNET D4 !)

Free pins:
D0,D1 D7,D8 D11->D28,D42->D49

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

enum mpc_status {
  MPC_UNKNOWN,
  MPC_CONNECTING,
  MPC_DISCONNECTED,
  MPC_CONNECTED
};

enum gui_screen {
  GUI_INIT,
  GUI_CONNECT,
  GUI_MPC_PLAYING,
  GUI_TEST
};


enum mpd_state {
  MPD_STATE_STOP,
  MPD_STATE_PLAY,
  MPD_STATE_PAUSE
};
  
struct MPD_Info{
  mpd_state state;
  int volume;
  int time;
  String artist;
  String title;
  String album;
  String album_date;
};


//*******************************
// System
//*******************************

boolean test_gui_mode=false;

osw_task exec;
osw_dt_timer dt10sec;

gui_screen current_displayed_gui_screen=GUI_INIT;

boolean process_touch_screen_in_progress=false;

MPD_Info mpd_info;

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

IPAddress mpd_ip(192,168,90,10);
int mpd_port=6600;

// Initialize the Ethernet server library
// with the IP address and port you want to use 
// (port 80 is default for HTTP):
//EthernetServer server(80);

EthernetClient mpc;
int mpc_status=MPC_UNKNOWN;
boolean fake_connected=false;
//boolean fake_connected=true;


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

    case GUI_TEST:
      draw_gui_test();
    break;
  }
}



void draw_gui_init() {
  myGLCD.fillScr(VGA_BLACK);
  myGLCD.setFont(BigFont);
  myGLCD.setBackColor(VGA_BLACK);
  myGLCD.setColor(VGA_GREEN);
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
//  Serial.println("draw_gui_mpc_playing");
  myGLCD.setColor(VGA_WHITE);

  update_mpd_info();
  switch (mpd_info.state) {
    case MPD_STATE_PLAY:
      myGLCD.print("Now playing...",LEFT,5);
      break;
    case MPD_STATE_PAUSE:
      myGLCD.print("Now paused",LEFT,5);
      break;
    case MPD_STATE_STOP:
      myGLCD.print("Nothing if actually playing",LEFT,5);
      break;
  }

  if (mpd_info.state==MPD_STATE_PLAY || mpd_info.state==MPD_STATE_PAUSE) {
    myGLCD.print(mpd_info.artist,LEFT,25);
    myGLCD.print(mpd_info.title,LEFT,45);
    myGLCD.print(mpd_info.album,LEFT,65);
    myGLCD.print(mpd_info.album_date,LEFT,85);
  }

  GUI_Button *button_setup=new GUI_Button(10,200,100,220,"SETUP");
  button_setup->action=action_button_setup;
  gui_test.add(button_setup);

//  GUI_Button *button_test=new GUI_Button(10,110,130,140,"Test!");
//  button_test->btn_status=GUI_BUTTON_DOWN;
//  gui_test.add(button_test);

//  GUI_Button *button_test2=new GUI_Button(140,110,290,140,"DISABLED",false);
//  gui_test.add(button_test2);

  gui_test.draw(myGLCD);

}


void test_btn1(){
  myGLCD.setColor(VGA_GREEN);
  myGLCD.fillRect(200,0,210,10);
  delay(1000);
  myGLCD.setColor(VGA_BLACK);
  myGLCD.fillRect(200,0,210,10);
//  gui_test.draw(myGLCD);
}


void test_btn2(){
  myGLCD.setColor(VGA_BLUE);
  myGLCD.fillRect(200,0,210,10);
  delay(1000);
  myGLCD.setColor(VGA_BLACK);
  myGLCD.fillRect(200,0,210,10);
//  gui_test.draw(myGLCD);
}

void test_btn3(){
  myGLCD.setColor(VGA_RED);
  myGLCD.fillRect(200,0,210,10);
  delay(1000);
  myGLCD.setColor(VGA_BLACK);
  myGLCD.fillRect(200,0,210,10);
//  gui_test.draw(myGLCD);
}


void draw_gui_test() {
  myGLCD.fillScr(VGA_BLACK);

  GUI_Button *button_status=new GUI_Button(10,10,130,40,"BUTTON1");
  button_status->action=test_btn1;
  gui_test.add(button_status);

  GUI_Button *button_test=new GUI_Button(10,110,130,140,"Test!");
//  button_test->btn_status=GUI_BUTTON_DOWN;
  button_test->action=test_btn2;
  gui_test.add(button_test);

  GUI_Button *button_test2=new GUI_Button(140,110,290,140,"BUTTON3");
  button_test2->action=test_btn3;
  gui_test.add(button_test2);

  gui_test.draw(myGLCD);

}




void init_GUI_MPC(){
}



void draw_background(){
  int x,y;
  for (y=0; y<(240/4)-1; y++) {
    for (x=0; x<(320/4)-1; x++) {
      myGLCD.drawBitmap (x*4, y*4, 4, 4, background);
    }
  }  
}

void mpd_play_pause_action(){

}
//            mpd_client.println("pause");            


void action_button_setup(){
  
}


/*void test_action(){
  myGLCD.fillScr(VGA_BLACK);
  myGLCD.setFont(SmallFont);
  myGLCD.setColor(VGA_WHITE);
  myGLCD.print("test action!",CENTER,50);
  update_mpd_info();
  delay(5000);
  gui_test.draw(myGLCD);

}
*/

void process_touch_screen(){
  if (process_touch_screen_in_progress) {
    return;
  }
  process_touch_screen_in_progress=true;
  if (myTouch.dataAvailable()) { 
    myTouch.read();
    touch_x=myTouch.getX();
    touch_y=myTouch.getY();

    GUI_Object * obj = gui_test.test_touch(touch_x,touch_y);
    if (obj!=NULL) {
        if (obj->type==GUI_OBJECT_TYPE_BUTTON) {
          if (((GUI_Button*) obj)->btn_status==GUI_BUTTON_GRAYED) {
          //  Serial.println("grayed button");
          } else {
            
            if (((GUI_Button*) obj)->btn_status==GUI_BUTTON_UP) {
              Serial.print("Button pressed: ");
              Serial.println(((GUI_Button*)obj)->label);
              Serial.print("pos down: ");
              Serial.print(touch_x);
              Serial.print(",");
              Serial.println(touch_y);

              ((GUI_Button*) obj)->btn_status=GUI_BUTTON_DOWN;
              obj->draw(myGLCD);
              delay(100);
              
              GUI_Object * lastObj;
              if (myTouch.dataAvailable()) {
                while (myTouch.dataAvailable()) {
                  myTouch.read();
                  touch_x=myTouch.getX();
                  touch_y=myTouch.getY();
                  // Get the object under the pointer
                  lastObj = gui_test.test_touch(touch_x,touch_y);
                  if (lastObj==obj) {
                    if (((GUI_Button*) obj)->btn_status==GUI_BUTTON_UP) {
                      Serial.println("lastObj==obj and btn status is UP");
  
                      ((GUI_Button*) obj)->btn_status=GUI_BUTTON_DOWN;
                      obj->draw(myGLCD);
                      delay(100);
                    }
                  }
                  else {
                    Serial.println("lastObj != obj");
  
                    if (((GUI_Button*) obj)->btn_status==GUI_BUTTON_DOWN) {
                      ((GUI_Button*) obj)->btn_status=GUI_BUTTON_UP;
                      obj->draw(myGLCD);
                      delay(100);
                    }
                  }
                }
              } else {
                // Already released
                lastObj=obj;
              }
              
              Serial.print("pos UP: ");
              Serial.print(touch_x);
              Serial.print(",");
              Serial.println(touch_y);

              
              Serial.println("released");
              if (lastObj==obj && ((GUI_Button*) obj)->btn_status==GUI_BUTTON_DOWN ) {
              Serial.println("same obj and status is down");
                if (obj->action != NULL){
                  ((GUI_Button*) obj)->btn_status=GUI_BUTTON_UP;
                  obj->draw(myGLCD);
                  obj->action();
                }
              } else {
//              if (lastObj!= NULL){
//                ((GUI_Button*) lastObj)->btn_status=GUI_BUTTON_UP;
//                lastObj->draw(myGLCD);
//              }
                if (obj != NULL){
                  ((GUI_Button*) obj)->btn_status=GUI_BUTTON_UP;
                  obj->draw(myGLCD);
                }
              }             
            }
          }
        }
    } else {
//      Serial.println("null obj returned by test_touch :(");
        // We wait for the touch to be released
        while (myTouch.dataAvailable()) {
          myTouch.read();
        }

    }
  }
  process_touch_screen_in_progress=false;

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




void connect_mpc(){
  if (mpc_status!=MPC_CONNECTED) {
    // Trying to connnect
    // connect client to mpd server

    mpc_status=MPC_CONNECTING;
    myGLCD.setColor(VGA_WHITE);
    myGLCD.print("Connecting to MPD server...", CENTER, 100);
    Serial.println("Connecting to MPD server...");

//    while (!mpd_client){
//      ;
//    }

    if (mpc.connect(mpd_ip, mpd_port)) {
      Serial.println("Connected to MPD server");
      // Flush the OK
      if (mpc.available()) {
        while (mpc.available()) {
          mpc.read();
        }
      }

//      mpd_client_status=MPD_CLIENT_CONNECTED;
    }
    else {
      // if you didn't get a connection to the server:
      Serial.println("Connection to MPD server failed");
      mpc.stop();

//      mpd_client_status=MPD_CLIENT_DISCONNECTED;
    }
  }

}


void update_mpc_status(){
 
//   if (mpd_client_status!=MPD_CLIENT_CONNECTING ) {

     if (mpc.connected() || fake_connected ) {
      // is connected
      if (mpc_status!=MPC_CONNECTED ) {
        // but was not in previous test
        // we need to update the display
  
        myGLCD.setColor(VGA_RED);
        myGLCD.print("Connected", CENTER, 100);
        mpc_status=MPC_CONNECTED;
        current_displayed_gui_screen=GUI_MPC_PLAYING;
        draw_GUI();
  
      }
    } else {
      // is not connected
      if (mpc_status==MPC_CONNECTED) { // || mpd_client_status==MPD_CLIENT_UNKNOWN) {
        // but was in previous test
        // we need to update the display
        current_displayed_gui_screen=GUI_CONNECT;
        draw_GUI();
        myGLCD.setColor(255, 0, 0);
        if (mpc_status==MPC_CONNECTED){
          myGLCD.print("Lost connection to MPD server!", CENTER , 100);
        } else {
          myGLCD.print("Unable to connect to MPD server!", CENTER , 100);
        }
        mpc_status=MPC_DISCONNECTED;
     }
    }
//   }
}




void sendMPDCommandAndWaitForResponse(String command){
// Will became getStatus when ok to parse all the answer
    mpc.flush();
    mpc.println(command);
    delay(100);
//    mpd_client.println("command_list\nstatus\ncurrentsong\ncommand_list_end");

// Wait for the answer
  int maxLoop=20;
  while (maxLoop>0 && !mpc.available()) {
    Serial.println("waiting loop");
    delay(100);
    maxLoop--;
  }
}

void getMPDStatus(){
}

void update_mpd_info(){

  String currentLine=String();
  sendMPDCommandAndWaitForResponse("status");
  if (mpc.available()) {
    while (mpc.available()) {
      char c = mpc.read();
      if (c==10) {
        Serial.println(currentLine);
           if (currentLine.startsWith("volume:")) {
             mpd_info.volume=currentLine.substring(currentLine.indexOf(":")).toInt();
           } else if (currentLine.startsWith("time:")) {
             mpd_info.time=currentLine.substring(currentLine.indexOf(":")).toInt();
           } else if (currentLine.startsWith("state:")) {
             if (currentLine.endsWith("play")) {
               mpd_info.state=MPD_STATE_PLAY;
             }
           }        
         currentLine=String();
      } else { 
       currentLine=String(currentLine+c);
      }
    }
  } else {
    Serial.println("mpd client not ready");
  }
  
  sendMPDCommandAndWaitForResponse("currentsong");
  if (mpc.available()) {
    while (mpc.available()) {
      char c = mpc.read();
      if (c==10) {
        Serial.println(currentLine);
           if (currentLine.startsWith("Artist:")) {
             mpd_info.artist=currentLine.substring(currentLine.indexOf(":")+1);
//mpd_info.artist="popof";
           } else if (currentLine.startsWith("Title:")) {
             mpd_info.title=currentLine.substring(currentLine.indexOf(":"));
           } else if (currentLine.startsWith("Album:")) {
             mpd_info.album=currentLine.substring(currentLine.indexOf(":"));
           } else if (currentLine.startsWith("Date:")) {
             mpd_info.album_date=currentLine.substring(currentLine.indexOf(":"));
           }        
         currentLine=String();
      } else { 
       currentLine=String(currentLine+c);
      }
    }
  } else {
    Serial.println("mpd client not ready");
  }
  
}

//***************************
// MPDuino 'OS Like' functions
//***************************

void executive_init(void)
{
  dt10sec.start(2000); // First start after 2sec
  osw_evt_register(1, evt10s);
}

void* executive(void* _pData)
{
  static int count = 0;
  if (dt10sec.timedOut()) {
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

  myGLCD.fillScr(VGA_BLACK);
  myGLCD.setFont(SmallFont);
  myGLCD.setBackColor(VGA_BLACK);
  myGLCD.setColor(VGA_GREEN);
  myGLCD.drawRoundRect (1, 1, 318, 238);
  myGLCD.print("DHCP Client...",CENTER,50);


  // start the Ethernet connection
//  Ethernet.begin(mac, local_ip);

  int connected=0;
  while (connected==0){
    connected=Ethernet.begin(mac);
    delay(500);
  }
  
  
//  and the server:
//  server.begin();
//  Serial.print("server is at ");
//  Serial.println(Ethernet.localIP());

  // System
  if (!test_gui_mode) {
    executive_init();
  }
  exec.taskCreate("exec", executive);

//  osw_list_tasks();

  if (test_gui_mode) {
    current_displayed_gui_screen=GUI_TEST;
  } else {
    current_displayed_gui_screen=GUI_CONNECT;
  }
  draw_GUI();
  
}



void check_mpd_client() {
//  if (mpd_client_status!=MPD_CLIENT_CONNECTED && mpd_client_status!=MPD_CLIENT_CONNECTING) {
  if (mpc_status!=MPC_CONNECTED) {
    connect_mpc(); 
  }
  update_mpc_status();
}


void loop()
{
 
 
//  process_touch_screen();
 
  osw_tasks_go();

/*
  while (true)
  {
//    process_ethernet_server();
//    process_ethernet_mpd_client();
//    delay(100);
  }*/
}


