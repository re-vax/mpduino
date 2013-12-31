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
  GUI_NULL,
  GUI_INIT,
  GUI_CONNECT,
  GUI_MPD_PLAYER,
  GUI_SETUP,
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
osw_dt_timer dt2sec;

gui_screen current_displayed_gui_screen=GUI_NULL;
GUI_Screen *current_displayed_gui_screen_OBJ=NULL;
gui_screen next_displayed_gui_screen=GUI_INIT;

//boolean process_touch_screen_in_progress=false;

MPD_Info mpd_info;
osw_semaphore sem_mpd_info;
osw_semaphore sem_mpc_status;
osw_semaphore sem_current_screen;
osw_semaphore sem_process_touchscreen;

//*******************************



//*******************************
// GFX and LCD 
//*******************************

// Declare which fonts we will be using
extern uint8_t SmallFont[];
extern uint8_t BigFont[];
extern unsigned int background[16];
extern unsigned int btn1[2528];
//extern unsigned int qrcode_revax_fr_export1[16383];


// Uncomment the next two lines for the Arduino 2009/UNO
//UTFT        myGLCD(ITDB24D,19,18,17,16);   // Remember to change the model parameter to suit your display module!
//UTouch      myTouch(15,10,14,9,8);

// Uncomment the next two lines for the Arduino Mega
UTFT        myGLCD(ITDB32S, 38,39,40,41);   // Remember to change the model parameter to suit your display module!
UTouch      myTouch(6,5,4,3,2);

// TouchScreen
int touch_x, touch_y;

// -------------====== GUI ==========-----------

// ---------- MPD PLAYER ---------- 
GUI_Screen gui_mpd_player=GUI_Screen();
GUI_Button *button_play;
GUI_Button *button_next_track;
GUI_Button *button_previous_track;

GUI_Button *button_setup;
// --------- /MPD PLAYER ---------- 




//*******************************
// Ethernet
//*******************************

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress local_ip(192,168,90, 177);
boolean dhcp=false;
IPAddress ip(192,168,90, 19);

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
//  sem_current_screen.semTake();

  // Is this a new gui?
  boolean isNewGui=false;
  if (next_displayed_gui_screen!=GUI_NULL) {
    Serial.print("Switch to wew gui:");
    switch (next_displayed_gui_screen) {
      case GUI_INIT:
        Serial.println("GUI_INIT");
        break;
      case GUI_CONNECT:
        Serial.println("GUI_CONNECT");
        break;
      case GUI_MPD_PLAYER:
        Serial.println("GUI_MPD_PLAYER");
        break;
      case GUI_SETUP:
        Serial.println("GUI_SETUP");
        break;
    }
    
    current_displayed_gui_screen=next_displayed_gui_screen;
    next_displayed_gui_screen=GUI_NULL;
    current_displayed_gui_screen_OBJ=NULL;
    isNewGui=true;
    
  }

  switch (current_displayed_gui_screen) {
    case GUI_INIT:
      if (isNewGui) {
        init_gui_init();
      }
      draw_gui_init();
    break;

    case GUI_CONNECT:
      if (isNewGui) {
        init_gui_connect();
      }
      draw_gui_connect();
    break;

    case GUI_MPD_PLAYER:
      if (isNewGui) {
        init_gui_mpd_player();
        current_displayed_gui_screen_OBJ=&gui_mpd_player;        
      }
      draw_gui_mpd_player();
    break;

    case GUI_TEST:
//      draw_gui_test();
    break;
  }

//  sem_current_screen.semGive();
}



void init_gui_init() {
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



void init_gui_connect() {
}


void draw_gui_connect() {
  myGLCD.clrScr();
  sem_mpc_status.semTake();
  myGLCD.setColor(255, 0, 0);
  if (mpc_status==MPC_DISCONNECTED){
    myGLCD.print("DISCONNECTED!", CENTER , 80);
  } else if (mpc_status==MPC_CONNECTING){
    myGLCD.print("Connecting to MPD server!", CENTER , 80);
  } else {
//    myGLCD.print("Unable to connect to MPD server!", CENTER , 80);
  }
  sem_mpc_status.semGive();

}


void init_gui_mpd_player(){
  if (!gui_mpd_player.init_done) {
    Serial.println("init_gui_mpd_player");
    gui_mpd_player.backColor=VGA_BLACK;  
    gui_mpd_player.frontColor=VGA_WHITE;  
  
    button_play =new GUI_Button(5,20,95,60);
    button_play->action=action_button_play;
    gui_mpd_player.add(button_play);
  
    button_setup=new GUI_Button(5,200,95,220,"SETUP");
    button_setup->action=action_button_setup;
    gui_mpd_player.add(button_setup);
    
    button_previous_track=new GUI_Button(5,90,95,120,"|<<");
    button_previous_track->action=action_button_previous_track;
    gui_mpd_player.add(button_previous_track);

    button_next_track=new GUI_Button(100,90,195,120,">>|");
    button_next_track->action=action_button_next_track;
    gui_mpd_player.add(button_next_track);

    // init done
    gui_mpd_player.init_done=true;
  }

}

void draw_gui_mpd_player() {

  sem_mpd_info.semTake();

  myGLCD.fillScr(gui_mpd_player.backColor);
  myGLCD.setColor(gui_mpd_player.frontColor);
//  myGLCD.setFont(BigFont);


  switch (mpd_info.state) {
    case MPD_STATE_PLAY:
      myGLCD.print("Now playing:",5,5);
      button_play->text ="PAUSE";
      break;
    case MPD_STATE_PAUSE:
      myGLCD.print("Now paused:",5,5);
      button_play->text="PLAY";
      break;
    case MPD_STATE_STOP:
      myGLCD.print("Nothing if actually playing",5,5);
      button_play->text="PLAY";
      break;
  }
  
  myGLCD.drawLine(0,18,102,18);
  myGLCD.drawLine(102,18,102,85);
  myGLCD.drawLine(102,85,319,85);

  if (mpd_info.state==MPD_STATE_PLAY || mpd_info.state==MPD_STATE_PAUSE) {
    myGLCD.print(mpd_info.artist,105,5);
    myGLCD.print(mpd_info.title,105,25);
    myGLCD.print(mpd_info.album,105,45);
    myGLCD.print(mpd_info.album_date,105,65);
  }

//  myGLCD.drawBitmap(180,200-128,128,128,qrcode_revax_fr_export1);

  gui_mpd_player.draw(myGLCD);

  sem_mpd_info.semGive();
  
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

void action_button_play(){
//       mpd_client.println("pause"); 
  sem_mpd_info.semTake();
  if (mpd_info.state==MPD_STATE_STOP){
    sendMPDCommandAndWaitForResponse("play");
  } else {
    sendMPDCommandAndWaitForResponse("pause");
  }
  sem_mpd_info.semGive();
}

void action_button_previous_track(){
  sem_mpd_info.semTake();
  if (mpd_info.state!=MPD_STATE_STOP){
    sendMPDCommandAndWaitForResponse("previous");
  }
  sem_mpd_info.semGive();
}

void action_button_next_track(){
  sem_mpd_info.semTake();
  if (mpd_info.state!=MPD_STATE_STOP){
    sendMPDCommandAndWaitForResponse("next");
  }
  sem_mpd_info.semGive();
}



void action_button_setup(){
    sem_current_screen.semTake();
    next_displayed_gui_screen=GUI_SETUP;
    sem_current_screen.semGive();
    
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

//  Serial.print("process touch!");
  
  sem_process_touchscreen.semTake();
//  if (process_touch_screen_in_progress) {
//    return;
//  }

//  process_touch_screen_in_progress=true;
  if (myTouch.dataAvailable()) { 
    myTouch.read();
    touch_x=myTouch.getX();
    touch_y=myTouch.getY();

    GUI_Object * obj = gui_mpd_player.test_touch(touch_x,touch_y);
    if (obj!=NULL) {
        if (obj->type==GUI_OBJECT_TYPE_BUTTON) {
          if (((GUI_Button*) obj)->btn_status==GUI_BUTTON_GRAYED) {
          //  Serial.println("grayed button");
          } else {
            
            if (((GUI_Button*) obj)->btn_status==GUI_BUTTON_UP) {
           //   Serial.print("Button pressed: ");
           //   Serial.println(((GUI_Button*)obj)->label);
           //   Serial.print("pos down: ");
           //   Serial.print(touch_x);
           //   Serial.print(",");
           //   Serial.println(touch_y);

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
                  lastObj = gui_mpd_player.test_touch(touch_x,touch_y);
                  if (lastObj==obj) {
                    if (((GUI_Button*) obj)->btn_status==GUI_BUTTON_UP) {
                     // Serial.println("lastObj==obj and btn status is UP");
  
                      ((GUI_Button*) obj)->btn_status=GUI_BUTTON_DOWN;
                      obj->draw(myGLCD);
                      delay(100);
                    }
                  }
                  else {
                  //  Serial.println("lastObj != obj");
  
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
             /* 
              Serial.print("pos UP: ");
              Serial.print(touch_x);
              Serial.print(",");
              Serial.println(touch_y);

              
              Serial.println("released");
*/
            if (lastObj==obj && ((GUI_Button*) obj)->btn_status==GUI_BUTTON_DOWN ) {
              //Serial.println("same obj and status is down");
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
//  process_touch_screen_in_progress=false;
  sem_process_touchscreen.semGive();

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


void check_mpc_status() {
//  if (mpd_client_status!=MPD_CLIENT_CONNECTED && mpd_client_status!=MPD_CLIENT_CONNECTING) {

  sem_current_screen.semTake();
  sem_mpc_status.semTake();

  if (mpc_status!=MPC_CONNECTED) {
    connect_mpc();
  }

  if (mpc.connected() || fake_connected ) {
    // is connected -> switch to player
    if (current_displayed_gui_screen!=GUI_MPD_PLAYER){
      next_displayed_gui_screen=GUI_MPD_PLAYER;
    }  
  } else {
    // is not connected
    if (current_displayed_gui_screen!=GUI_CONNECT) { // || mpd_client_status==MPD_CLIENT_UNKNOWN) {
      // but was in previous test
      // we need to update the display
      next_displayed_gui_screen=GUI_CONNECT;
      mpc_status=MPC_DISCONNECTED;
     }
  }


  sem_mpc_status.semGive();
  sem_current_screen.semGive();


}


void connect_mpc(){
  if (mpc_status!=MPC_CONNECTED && mpc_status!=MPC_CONNECTING) {
    // Trying to connnect

    // connect client to mpd server
    mpc_status=MPC_CONNECTING;
    Serial.println("Connecting to MPD server...");

/*    while (!mpc){
      delay(1);
    }
*/
    if (mpc.connect(mpd_ip, mpd_port)) {
      Serial.println("Connected to MPD server");
      mpc_status=MPC_CONNECTED;

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
      mpc_status=MPC_DISCONNECTED;
      mpc.stop();

//      mpd_client_status=MPD_CLIENT_DISCONNECTED;
    }
  }

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

boolean update_mpd_info(){
  
  boolean retval=false;
  
  MPD_Info old_mpd_info;
  old_mpd_info.state=mpd_info.state;
  old_mpd_info.volume=mpd_info.volume;
  old_mpd_info.time=mpd_info.time;
  old_mpd_info.artist=mpd_info.artist;
  old_mpd_info.album=mpd_info.album;
  old_mpd_info.title=mpd_info.title;
  old_mpd_info.album_date=mpd_info.album_date;
  
  mpd_info.artist=String();
  mpd_info.album=String();
  mpd_info.title=String();
  mpd_info.album_date=String();
  
  String currentLine=String();
  sendMPDCommandAndWaitForResponse("status");
  if (mpc.available()) {
    while (mpc.available()) {
      char c = mpc.read();
      if (c==10) {
           if (currentLine.startsWith("volume:")) {
             mpd_info.volume=currentLine.substring(currentLine.indexOf(":")+2).toInt();
           } else if (currentLine.startsWith("time:")) {
             mpd_info.time=currentLine.substring(currentLine.indexOf(":")+2).toInt();
           } else if (currentLine.startsWith("state:")) {
             Serial.println(currentLine);
             if (currentLine.endsWith("play")) {
               mpd_info.state=MPD_STATE_PLAY;
             }
             if (currentLine.endsWith("stop")) {
               mpd_info.state=MPD_STATE_STOP;
             }
             if (currentLine.endsWith("pause")) {
               mpd_info.state=MPD_STATE_PAUSE;
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
//        Serial.println(currentLine);
           if (currentLine.startsWith("Artist:")) {
             mpd_info.artist=currentLine.substring(currentLine.indexOf(":")+2);
//mpd_info.artist="popof";
           } else if (currentLine.startsWith("Title:")) {
             mpd_info.title=currentLine.substring(currentLine.indexOf(":")+2);
           } else if (currentLine.startsWith("Album:")) {
             mpd_info.album=currentLine.substring(currentLine.indexOf(":")+2);
           } else if (currentLine.startsWith("Date:")) {
             mpd_info.album_date=currentLine.substring(currentLine.indexOf(":")+2);
           }        
         currentLine=String();
      } else { 
       currentLine=String(currentLine+c);
      }
    }
  } else {
    Serial.println("mpd client not ready");
  }

  if ( (old_mpd_info.state!=mpd_info.state) ||  (!old_mpd_info.artist.equals(mpd_info.artist)) || (!old_mpd_info.album.equals(mpd_info.album)) || (!old_mpd_info.title.equals(mpd_info.title)) || (!old_mpd_info.album_date.equals(mpd_info.album_date)) ) {
//  if ( (!old_mpd_info.artist.equals(mpd_info.artist)) || (!old_mpd_info.album.equals(mpd_info.album)) || (!old_mpd_info.title.equals(mpd_info.title)) || (!old_mpd_info.album_date.equals(mpd_info.album_date)) ) {
    retval=true;
  }
  

  return retval;
}

//***************************
// MPDuino 'OS Like' functions
//***************************

void executive_init(void)
{
  dt10sec.start(2000); // First start after 2sec
  osw_evt_register(1, evt10s);
  dt2sec.start(5000); // First start after 2sec
  osw_evt_register(2, evt2s);
}

void* executive(void* _pData)
{
  static int count = 0;
  if (dt10sec.timedOut()) {
    osw_evt_publish(1);
    dt10sec.start(10000);
  }

  if (dt2sec.timedOut()) {
    osw_evt_publish(2);
    dt2sec.start(2000);
  }
  

  // ---------- Refresh GUI ----------
  sem_current_screen.semTake();
  if ( (current_displayed_gui_screen_OBJ!=NULL && current_displayed_gui_screen_OBJ->need_refresh ) || (next_displayed_gui_screen!=GUI_NULL) ){
    
    // reset the need_refresh flag
    if (current_displayed_gui_screen_OBJ!=NULL && current_displayed_gui_screen_OBJ->need_refresh ) {
      current_displayed_gui_screen_OBJ->need_refresh=false;
    }
    
    draw_GUI();

  }  
  sem_current_screen.semGive();
  // ---------- End Of Refresh GUI ----------



  // Test the touchscreen
  process_touch_screen();

/*
  int res_sem=sem_process_touchscreen.semTake(10);
  if (res_sem==1) {
    sem_process_touchscreen.semGive();
    process_touch_screen();
  }
  */
}


void evt10s(int _evt)
{
//  Serial.println("evt10s");
  check_mpc_status();
}

void evt2s(int _evt)
{
  osw_print_mem_free();  
  
  sem_mpc_status.semTake();
  int local_mpc_status=mpc_status;
  sem_mpc_status.semGive();

  if (local_mpc_status==MPC_CONNECTED) {

    sem_mpd_info.semTake();
    boolean need_update=update_mpd_info();
    sem_mpd_info.semGive();

    if (need_update) {
    // if true we need to refresh (data changes)
      sem_current_screen.semTake();
      if (current_displayed_gui_screen==GUI_MPD_PLAYER){
//        draw_GUI();
        current_displayed_gui_screen_OBJ->need_refresh=true;
      }
      sem_current_screen.semGive();
    }
  }
}


/*************************
**  Required functions  **
*************************/

void setup()
{

  // disable SD SPI
  pinMode(chipSelect,OUTPUT);
  digitalWrite(chipSelect,HIGH);


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
    if (dhcp) {
      connected=Ethernet.begin(mac);
    } else {
      Ethernet.begin(mac,ip);
      connected=1;
    }
  delay(500);
  }

  
//  and the server:
//  server.begin();
  Serial.print("Local IP:");
  Serial.println(Ethernet.localIP());

  // System
  sem_mpd_info.semCreate("smi");
  sem_mpc_status.semCreate("sms");
  sem_current_screen.semCreate("scs");
  sem_process_touchscreen.semCreate("spt");

  executive_init();
  exec.taskCreate("exec", executive);
  next_displayed_gui_screen=GUI_CONNECT;
//  draw_GUI();
  
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


