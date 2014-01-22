
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
//#include <uText.h>
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
  GUI_MPD_PLAYER,
  GUI_SETUP,
  GUI_FAVORITE,
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
  int songid;
  int time;
  int length;
  String file;
  boolean isAStream;
  String artist;
  String title;
  String album;
  String album_date;
};


//*******************************
// System
//*******************************
#define MIN_FREEMEM_WARN 500
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
//uText       utext;
// TouchScreen
int touch_x, touch_y;

// -------------====== GUI ==========-----------

// ---------- MPD PLAYER ---------- 
GUI_Screen gui_mpd_player=GUI_Screen();
GUI_Button *button_play;
GUI_Button *button_next_track;
GUI_Button *button_previous_track;
GUI_Button *button_stop;
GUI_Label *label_mpd_status;
GUI_Label *label_artist;
GUI_Label *label_title;
GUI_Label *label_album;
GUI_Label *label_date;
GUI_ProgressBar *MPD_PLAYER_progress_bar_current_track;

GUI_Button *button_setup;
GUI_Button *MPD_PLAYER_button_favorite;

// --------- /MPD PLAYER ---------- 

// ---------  SETUP ---------- 
GUI_Screen gui_setup=GUI_Screen();
GUI_Label *SETUP_label_title;
GUI_Button *SETUP_button_exit;

// ---------  /SETUP ---------- 

// ---------  FAVORITE ---------- 

#define MAX_FAVORITE 5
#define MAX_LAN_FAVORITE_PATH 200

GUI_Screen gui_favorite=GUI_Screen();
GUI_Label *FAVORITE_label_title;
GUI_Button *FAVORITE_button_exit;
GUI_Button *FAVORITE_button_fav_X[MAX_FAVORITE];

String favorite_path[MAX_FAVORITE];


// ---------  /FAVORITE ---------- 

GUI_Screen gui_test=GUI_Screen();
GUI_Button *button_test1;


//*******************************
// Ethernet
//*******************************

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress local_ip(192,168,90, 177);
boolean dhcp=true;
IPAddress ip(192,168,90, 19);

//IPAddress mpd_ip(192,168,90,10);
IPAddress mpd_ip(172,17,30,114); // MobivAx on peymei-lan
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
    Serial.print("Switch to:");
    switch (next_displayed_gui_screen) {
      case GUI_INIT:
        Serial.println("INIT");
        break;
//      case GUI_CONNECT:
//        Serial.println("CONNECT");
//        break;
      case GUI_MPD_PLAYER:
        Serial.println("MPD_PLAYER");
        break;
      case GUI_SETUP:
        Serial.println("SETUP");
        break;
      case GUI_TEST:
        Serial.println("TEST");
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

/*    case GUI_CONNECT:
      if (isNewGui) {
        init_gui_connect();
      }
      draw_gui_connect();
    break;
*/
    case GUI_MPD_PLAYER:
      if (isNewGui) {
        init_gui_mpd_player();
        current_displayed_gui_screen_OBJ=&gui_mpd_player;        
      }
      draw_gui_mpd_player(isNewGui);
    break;

    case GUI_SETUP:
      if (isNewGui) {
        init_gui_setup();
        current_displayed_gui_screen_OBJ=&gui_setup;                     
      }
      draw_gui_setup(isNewGui);
    break;

    case GUI_FAVORITE:
      if (isNewGui) {
        init_gui_favorite();
        current_displayed_gui_screen_OBJ=&gui_favorite;                     
      }
      draw_gui_favorite(isNewGui);
    break;


    case GUI_TEST:
      if (isNewGui) {
        init_gui_test();
        current_displayed_gui_screen_OBJ=&gui_test;                     
      }
      draw_gui_test();
    break;
  }

//  sem_current_screen.semGive();
}
   
void init_gui_test(){
     if (!gui_test.init_done) {
   // Serial.println("init_gui_test");
    gui_test.backColor=VGA_BLACK;  
    gui_test.defaultFrontColor=VGA_WHITE;  
  
    button_test1 =new GUI_Button(5,20,150,40,"TEST1");
    button_test1->setFont(BigFont);
    button_test1->setColors(VGA_SILVER,VGA_BLACK,VGA_GRAY,VGA_BLUE);
    button_test1->action=test_btn1;
    gui_test.add(button_test1);
    // init done
    gui_test.init_done=true;
  }
 
}

void draw_gui_test(){
  gui_test.draw(myGLCD,true);
}


void init_gui_init() {
}

void draw_gui_init() {
/*
  myGLCD.fillScr(VGA_BLACK);
  myGLCD.setFont(BigFont);
  myGLCD.setBackColor(VGA_BLACK);
  myGLCD.setColor(VGA_GREEN);
  myGLCD.drawRoundRect (1, 1, 318, 238);
  myGLCD.print("MP-Duino-mote v0.1",CENTER,50);
  myGLCD.print("(c) Dr.vAx 2057",CENTER,70);
*/
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
//    Serial.println("init_gui_mpd_player");
    gui_mpd_player.backColor=VGA_BLACK;  
    gui_mpd_player.defaultFrontColor=VGA_WHITE;  
  
    button_play =new GUI_Button(100,140,120,40,"|>");
    button_play->setFont(BigFont);
    button_play->setColors(VGA_SILVER,VGA_BLACK,VGA_GRAY,VGA_BLUE);
    button_play->action=action_button_play;
    gui_mpd_player.add(button_play);
  
    button_setup=new GUI_Button(295,5,20,20,"*");
    button_setup->setFont(SmallFont);
    button_setup->setColors(VGA_SILVER,VGA_BLACK,VGA_GRAY,VGA_BLUE);
    button_setup->action=action_button_setup;
    gui_mpd_player.add(button_setup);
    
    button_previous_track=new GUI_Button(60,100,60,30,"|<<");
    button_previous_track->setFont(BigFont);
    button_previous_track->setColors(VGA_SILVER,VGA_BLACK,VGA_GRAY,VGA_BLUE);
    button_previous_track->action=action_button_previous_track;
    gui_mpd_player.add(button_previous_track);

    button_stop=new GUI_Button(130,100,60,30,"[]");
    button_stop->setFont(BigFont);
    button_stop->setColors(VGA_SILVER,VGA_BLACK,VGA_GRAY,VGA_BLUE);
    button_stop->action=action_button_stop;
    gui_mpd_player.add(button_stop);

    button_next_track=new GUI_Button(200,100,60,30,">>|");
    button_next_track->setFont(BigFont);
    button_next_track->setColors(VGA_SILVER,VGA_BLACK,VGA_GRAY,VGA_BLUE);
    button_next_track->action=action_button_next_track;
    gui_mpd_player.add(button_next_track);

    MPD_PLAYER_button_favorite=new GUI_Button(260,140,60,60,"^^");
    MPD_PLAYER_button_favorite->setFont(BigFont);
    MPD_PLAYER_button_favorite->setColors(VGA_SILVER,VGA_BLACK,VGA_GRAY,VGA_BLUE);
    MPD_PLAYER_button_favorite->action=action_button_favorite;
    gui_mpd_player.add(MPD_PLAYER_button_favorite);
    
    label_mpd_status=new GUI_Label(5,5);
    label_mpd_status->setFont(SmallFont);
    label_mpd_status->setColor(VGA_WHITE);
    gui_mpd_player.add(label_mpd_status);
    
    label_artist=new GUI_Label(5,25);
    label_artist->setFont(SmallFont);
    label_artist->setColor(VGA_WHITE);
    gui_mpd_player.add(label_artist);
    
    label_title=new GUI_Label(5,45);
    label_title->setFont(SmallFont);
    label_title->setColor(VGA_WHITE);
    gui_mpd_player.add(label_title);
    
    label_album=new GUI_Label(5,65);
    label_album->setFont(SmallFont);
    label_album->setColor(VGA_WHITE);
    gui_mpd_player.add(label_album);
    
    label_date=new GUI_Label(5,85);
    label_date->setFont(SmallFont);
    label_date->setColor(VGA_WHITE);
    gui_mpd_player.add(label_date);
    
    MPD_PLAYER_progress_bar_current_track=new GUI_ProgressBar(130,6,150,9,0);
    MPD_PLAYER_progress_bar_current_track->setColors(VGA_SILVER,VGA_GREEN);
    MPD_PLAYER_progress_bar_current_track->visible=false;
    gui_mpd_player.add(MPD_PLAYER_progress_bar_current_track);
    


    // init done
    gui_mpd_player.init_done=true;
  }

}

void draw_gui_mpd_player(boolean fullRedraw) {

  sem_mpd_info.semTake();
  sem_mpc_status.semTake();

  if (mpc_status==MPC_CONNECTED) {

    switch (mpd_info.state) {
      case MPD_STATE_PLAY:
        label_mpd_status->setText("Now playing:");
        button_play->setText("PAUSE");
        break;
      case MPD_STATE_PAUSE:
        label_mpd_status->setText("Now paused:");
        button_play->setText("PLAY");
        break;
      case MPD_STATE_STOP:
        label_mpd_status->setText("Nothing if actually playing");
        button_play->setText("PLAY");
        break;
    }
    
    button_play->setStatus(GUI_BUTTON_UP);
    button_previous_track->setStatus(GUI_BUTTON_UP);
    button_next_track->setStatus(GUI_BUTTON_UP);
    button_stop->setStatus(GUI_BUTTON_UP);
    
    if (mpd_info.state==MPD_STATE_PLAY || mpd_info.state==MPD_STATE_PAUSE) {
      label_artist->setText(mpd_info.artist);
      label_title->setText(mpd_info.title);
      label_album->setText(mpd_info.album);
      label_date->setText(mpd_info.album_date);

      if (mpd_info.length>0) {
        MPD_PLAYER_progress_bar_current_track->fill_percentage=100*mpd_info.time/mpd_info.length;
      } else {
        MPD_PLAYER_progress_bar_current_track->fill_percentage=100;
      }
      MPD_PLAYER_progress_bar_current_track->visible=true;
      MPD_PLAYER_progress_bar_current_track->need_refresh=true;

    } else {
      label_artist->setText(String());
      label_title->setText(String());
      label_album->setText(String());
      label_date->setText(String());
      MPD_PLAYER_progress_bar_current_track->visible=false;
    }

  } else {

    label_mpd_status->setText("Disconnected from MPD");
    button_play->setStatus(GUI_BUTTON_GRAYED);
    button_previous_track->setStatus(GUI_BUTTON_GRAYED);
    button_next_track->setStatus(GUI_BUTTON_GRAYED);
    button_stop->setStatus(GUI_BUTTON_GRAYED);
    label_artist->setText(String());
    label_title->setText(String());
    label_album->setText(String());
    label_date->setText(String());
  }
  
//  myGLCD.drawBitmap(180,200-128,128,128,qrcode_revax_fr_export1);
//Serial.println(gui_mpd_player.list_obj());
  gui_mpd_player.draw(myGLCD,fullRedraw);

  myGLCD.setColor(VGA_RED);
  myGLCD.drawLine(0,18,280,18);
  myGLCD.drawLine(280,18,280,100);
  myGLCD.drawLine(280,100,319,100);

  myGLCD.setColor(VGA_GREEN);
  myGLCD.drawLine(0,20,282,20);
  myGLCD.drawLine(282,20,282,102);
  myGLCD.drawLine(282,102,319,102);

  myGLCD.setColor(VGA_BLUE);
  myGLCD.drawLine(0,22,284,22);
  myGLCD.drawLine(284,22,284,104);
  myGLCD.drawLine(284,104,319,104);

  sem_mpd_info.semGive();
  
}


void init_gui_setup(){
  if (!gui_setup.init_done) {
    gui_setup.backColor=VGA_BLACK;  
    gui_setup.defaultFrontColor=VGA_WHITE;  
  
    SETUP_label_title=new GUI_Label(5,10);
    SETUP_label_title->setText("SETUP PAGE");
    SETUP_label_title->setFont(BigFont);
    SETUP_label_title->setColor(VGA_RED);
    gui_setup.add(SETUP_label_title);
    

    SETUP_button_exit =new GUI_Button(230,5,80,20,"Exit");
    SETUP_button_exit->setFont(SmallFont);
    SETUP_button_exit->setColors(VGA_SILVER,VGA_BLACK,VGA_GRAY,VGA_BLUE);
    SETUP_button_exit->action=action_setup_exit;
    gui_setup.add(SETUP_button_exit);
    // init done
    gui_setup.init_done=true;
  }
 
}


void draw_gui_setup(boolean fullRedraw) {
  gui_setup.draw(myGLCD,fullRedraw);
}


void action_setup_exit(){
    sem_current_screen.semTake();
    next_displayed_gui_screen=GUI_MPD_PLAYER;
    sem_current_screen.semGive();
    
}


void init_gui_favorite(){
  if (!gui_favorite.init_done) {
    gui_favorite.backColor=VGA_BLACK;  
    gui_favorite.defaultFrontColor=VGA_WHITE;  
  
    FAVORITE_label_title=new GUI_Label(5,10);
    FAVORITE_label_title->setText("FAVORITES");
    FAVORITE_label_title->setFont(BigFont);
    FAVORITE_label_title->setColor(VGA_BLUE);
    gui_favorite.add(FAVORITE_label_title);
    
    FAVORITE_button_exit =new GUI_Button(230,5,80,22,"Exit");
    FAVORITE_button_exit->setFont(BigFont);
    FAVORITE_button_exit->setColors(VGA_SILVER,VGA_BLACK,VGA_GRAY,VGA_BLUE);
    FAVORITE_button_exit->action=action_setup_exit;
    gui_favorite.add(FAVORITE_button_exit);
    
    
    if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");
  
  File dataFile = SD.open("favorite.txt");
  String currentLine=String();
  String currentTitle=String();
  int favoriteCount=0;
  boolean nextLineIsPath=false;
  // if the file is available, write to it:
  if (dataFile) {
    while (dataFile.available() && favoriteCount<MAX_FAVORITE) {
       char c = dataFile.read();
       //Serial.print(c);
       if (c==10) {
         if (currentLine.startsWith("-")) {
           if (!nextLineIsPath) {
             nextLineIsPath=true;
             currentTitle=currentLine.substring(1);
           }
           
         } else {
           if (nextLineIsPath) {
             nextLineIsPath=false;
             favorite_path[favoriteCount]=currentLine;
             
             Serial.print("adding favorite #");
             Serial.println(favoriteCount);
             Serial.println(currentTitle);
             Serial.println(currentLine);
             
             FAVORITE_button_fav_X[favoriteCount]=new GUI_Button(5,30+favoriteCount*30,180,25,currentTitle);
             
             GUI_Button *FAVORITE_button=FAVORITE_button_fav_X[favoriteCount];
             FAVORITE_button->setFont(BigFont);
             FAVORITE_button->setColors(VGA_SILVER,VGA_BLACK,VGA_GRAY,VGA_BLUE);
  //           FAVORITE_button->action=action_setup_exit;
  
             FAVORITE_button->callback=new event_cb();
             FAVORITE_button->callback->cb=action_add_favorite;
             FAVORITE_button->callback->data=favoriteCount;
             gui_favorite.add(FAVORITE_button);

             favoriteCount++;
           }
     
         }

        // empty the current buffer       
//         Serial.println(currentLine);
         currentLine=String();

      } else { 
        currentLine+=c;
      }
    }
    dataFile.close();
  }
    // if the file isn't open, pop up an error:
   else {
      Serial.println("error opening favorite.txt");
    } 
    
    // init done
    gui_favorite.init_done=true;
  }
 
}

static void action_add_favorite(int fav_index)
{
  Serial.print("action_add_favorite #");
  Serial.println(fav_index);
  Serial.println(favorite_path[fav_index]);
//  sendMPDCommandAndWaitForResponse("clear");
  sendMPDCommandAndWaitForResponse("add \""+favorite_path[fav_index]+"\"");
//  sendMPDCommandAndWaitForResponse("play");
    /* do stuff and things with the event */
}



void draw_gui_favorite(boolean fullRedraw) {
  gui_favorite.draw(myGLCD,fullRedraw);
}


void test_btn1(){
  myGLCD.setColor(VGA_GREEN);
  myGLCD.fillRect(200,0,210,10);
  delay(1000);
  myGLCD.setColor(VGA_BLACK);
  myGLCD.fillRect(200,0,210,10);
  test_gui_mode=false;
  sem_current_screen.semTake();
//  next_displayed_gui_screen=GUI_CONNECT;
  sem_current_screen.semGive();

//  gui_test.draw(myGLCD);
}



/*
void init_GUI_MPC(){
}
*/


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

void action_button_stop(){
//       mpd_client.println("pause"); 
  sem_mpd_info.semTake();
  if (mpd_info.state!=MPD_STATE_STOP){
    sendMPDCommandAndWaitForResponse("stop");
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

void action_button_favorite(){
    sem_current_screen.semTake();
    next_displayed_gui_screen=GUI_FAVORITE;
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

  
  sem_process_touchscreen.semTake();
//  if (process_touch_screen_in_progress) {
//    return;
//  }

//  process_touch_screen_in_progress=true;
  if (myTouch.dataAvailable() && current_displayed_gui_screen_OBJ!=NULL) {
    myTouch.read();
    touch_x=myTouch.getX();
    touch_y=myTouch.getY();
    sem_current_screen.semTake();
    GUI_Object * obj = current_displayed_gui_screen_OBJ->test_touch(touch_x,touch_y);
    sem_current_screen.semGive();
    if (obj!=NULL) {
      
      // ************ BUTTON ***************
        
        switch (obj->type) {
          case GUI_OBJECT_TYPE_BUTTON:
            if (((GUI_Button*) obj)->btn_status==GUI_BUTTON_GRAYED) {
            //  Serial.println("grayed button");
            } else {
              if (((GUI_Button*) obj)->btn_status==GUI_BUTTON_UP) {
                ((GUI_Button*) obj)->btn_status=GUI_BUTTON_DOWN;
                obj->draw(myGLCD);
               // Serial.println("trace A");
                GUI_Object * lastObj;
                if (myTouch.dataAvailable()) {
                  int loop_count=0;
                  int sum_x=0;
                  int sum_y=0;
                  while (myTouch.dataAvailable()) {
                    loop_count++;
                    lastObj=NULL;
                    myTouch.read();
                    touch_x=myTouch.getX();
                    touch_y=myTouch.getY();
                    sum_x+=touch_x;
                    sum_y+=touch_y;
//                    lastObj = current_displayed_gui_screen_OBJ->test_touch(sum_x/loop_count,sum_y/loop_count);
                    lastObj = current_displayed_gui_screen_OBJ->test_touch(touch_x,touch_y);
                    if (lastObj==obj) {
                     if (((GUI_Button*) obj)->btn_status==GUI_BUTTON_UP) {
                          ((GUI_Button*) obj)->setStatus(GUI_BUTTON_DOWN);
                          obj->draw(myGLCD);
                        }
                      }
                    else {
                      if (((GUI_Button*) obj)->btn_status==GUI_BUTTON_DOWN) {
                        ((GUI_Button*) obj)->setStatus(GUI_BUTTON_UP);
                        obj->draw(myGLCD);
                      }
                    }
                  }
                } else {
                  // Already released
                  lastObj=obj;
                }
              if (lastObj==obj && ((GUI_Button*) obj)->btn_status==GUI_BUTTON_DOWN ) {

                  ((GUI_Button*) obj)->btn_status=GUI_BUTTON_UP;
                  obj->draw(myGLCD);

                  // Old style callback system
                  if (obj->action != NULL){
                    obj->action();
                  }
                  
                  if (obj->callback!=NULL){
                    struct event_cb *callback=obj->callback;
                    if (callback!=NULL && callback->cb!=NULL){
                      Serial.println("Call the callback function");
                      callback->cb(callback->data);
                    }
                  }
                  
                } else {
                  
                  if (lastObj!= NULL){
                    ((GUI_Button*) lastObj)->btn_status=GUI_BUTTON_UP;
                    lastObj->draw(myGLCD);
                  }
                  
                  if (obj != NULL){
                     //   Serial.print("I");
                    ((GUI_Button*) obj)->setStatus(GUI_BUTTON_UP);
                        obj->draw(myGLCD);
                     //   Serial.print("J");
                  }
                }             
              }                        
            }
          
          break;
          
          case GUI_OBJECT_TYPE_SLIDER:
            
          
          break;
          
          
          
        }
    } else {
//      Serial.println("null obj returned by test_touch :(");
        // We wait for the touch to be released
                   //   Serial.print("K");
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

  
//  Serial.println("\nFiles found on the card (name, date and size in bytes): ");
//  root.openRoot(volume);
  
  // list all files in the card with date and size
//  root.ls(LS_R | LS_DATE | LS_SIZE);
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

//  sem_current_screen.semTake();
  sem_mpc_status.semTake();

  if (mpc_status!=MPC_CONNECTED) {
    connect_mpc();
  } else {
//    Serial.println("mpc_status==MPC_CONNECTED");
    if (!mpc.connected()){
      Serial.println("!mpc.connected()");
      mpc_status=MPC_DISCONNECTED;
    }
  }

/*
  if (mpc_status==MPC_CONNECTED || fake_connected ) {
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
*/

  sem_mpc_status.semGive();
//  sem_current_screen.semGive();


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
      Serial.println("Connection failed");
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
    Serial.println("wloop");
    delay(20);
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
  old_mpd_info.songid=mpd_info.songid;
  old_mpd_info.time=mpd_info.time;
  old_mpd_info.length=mpd_info.length;
  old_mpd_info.file=mpd_info.file;
  old_mpd_info.artist=mpd_info.artist;
  old_mpd_info.album=mpd_info.album;
  old_mpd_info.title=mpd_info.title;
  old_mpd_info.album_date=mpd_info.album_date;
    
  String currentLine=String();
  sendMPDCommandAndWaitForResponse("status");
  int count=0;
  if (mpc.available()) {
    while (mpc.available()) {
      char c = mpc.read();
      if (c==10) {
           if (currentLine.startsWith("volume:")) {
             mpd_info.volume=currentLine.substring(currentLine.indexOf(":")+2).toInt();
           } else if (currentLine.startsWith("songid:")) {
             mpd_info.songid=currentLine.substring(currentLine.indexOf(":")+2).toInt();
           } else if (currentLine.startsWith("time:")) {
             mpd_info.time=currentLine.substring(currentLine.indexOf(":")+2).toInt();
 //            Serial.print("current song pos:");
 //            Serial.println(mpd_info.time);

           } else if (currentLine.startsWith("state:")) {
             //Serial.println(currentLine);
             if (currentLine.endsWith("play")) {
               mpd_info.state=MPD_STATE_PLAY;
             }
             if (currentLine.endsWith("stop")) {
               mpd_info.state=MPD_STATE_STOP;
                mpd_info.file=String();
                mpd_info.artist=String();
                mpd_info.album=String();
                mpd_info.title=String();
                mpd_info.album_date=String();
             }
             if (currentLine.endsWith("pause")) {
               mpd_info.state=MPD_STATE_PAUSE;
             }
           }        
         currentLine=String();
      } else { 
//       currentLine=String(currentLine+c);
      count++;
       currentLine+=c;
      }
    }
  } else {
    Serial.println("not ready");
  }
  
//  Serial.print("byte read from mpc:");
//  Serial.println(count);

  if (mpd_info.songid != old_mpd_info.songid) {

    
    count=0;
    sendMPDCommandAndWaitForResponse("currentsong");
    if (mpc.available()) {
      while (mpc.available()) {
        char c = mpc.read();
        if (c==10) {
  //        Serial.println(currentLine);
             if (currentLine.startsWith("Artist:")) {
               mpd_info.artist=currentLine.substring(currentLine.indexOf(":")+2);
  //mpd_info.artist="popof";
             } else if (currentLine.startsWith("Time:")) {
               mpd_info.length=currentLine.substring(currentLine.indexOf(":")+2).toInt();
  //             Serial.print("current song length:");
  //             Serial.println(mpd_info.length);
             } else if (currentLine.startsWith("Title:")) {
               mpd_info.title=currentLine.substring(currentLine.indexOf(":")+2);
             } else if (currentLine.startsWith("Album:")) {
               mpd_info.album=currentLine.substring(currentLine.indexOf(":")+2);
             } else if (currentLine.startsWith("Date:")) {
               mpd_info.album_date=currentLine.substring(currentLine.indexOf(":")+2);
             } else if (currentLine.startsWith("file:")) {
               mpd_info.file=currentLine.substring(currentLine.indexOf(":")+2);
             }        
           currentLine=String();
        } else { 
  //       currentLine=String(currentLine+c);
         currentLine+=c;
         count++;
        }
      }
    } else {
      Serial.println("not ready");
    }
  }
  
//  Serial.print("byte read from mpc:");
//  Serial.println(count);

  if ( (old_mpd_info.state!=mpd_info.state) || (old_mpd_info.time!=mpd_info.time) || (!old_mpd_info.artist.equals(mpd_info.artist)) || (!old_mpd_info.album.equals(mpd_info.album)) || (!old_mpd_info.title.equals(mpd_info.title)) || (!old_mpd_info.album_date.equals(mpd_info.album_date)) ) {
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
  dt2sec.start(1000); // First start after 2sec
  osw_evt_register(2, evt2s);
}

void* executive(void* _pData)
{
  static int count = 0;
//  if (dt10sec.timedOut()) {
//    osw_evt_publish(1);
//    dt10sec.start(10000);
//  }

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
// if (!test_gui_mode) {
//   check_mpc_status();
// }
}

void evt2s(int _evt)
{
//  osw_print_mem_free();  
  int freeMem=memoryFree();
  if (freeMem<MIN_FREEMEM_WARN){
    Serial.print("Free mem:");
    Serial.println(freeMem);
  }
  

  sem_mpc_status.semTake();
  int local_old_mpc_status=mpc_status;
  sem_mpc_status.semGive();

  check_mpc_status();

  sem_mpc_status.semTake();
  int local_mpc_status=mpc_status;
  sem_mpc_status.semGive();

  boolean need_update=false;

  if (local_mpc_status==MPC_CONNECTED ) {
    sem_mpd_info.semTake();
    need_update=update_mpd_info();
    sem_mpd_info.semGive();
  }
  
  need_update=need_update || (local_old_mpc_status != local_mpc_status);
  
  if (need_update) {
    sem_current_screen.semTake();
    if (current_displayed_gui_screen==GUI_MPD_PLAYER){
      current_displayed_gui_screen_OBJ->need_refresh=true;
    }
    sem_current_screen.semGive();
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
//  utext=uText(&myGLCD,320,240);

  myTouch.InitTouch();
  myTouch.setPrecision(PREC_MEDIUM);

//  init_GUI_MPC();
  draw_GUI();

  
  // sd card initialisation
  sdCardInit();

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
//  Serial.print("Local IP:");
  Serial.println(Ethernet.localIP());

  // System
  sem_mpd_info.semCreate("smi");
  sem_mpc_status.semCreate("sms");
  sem_current_screen.semCreate("scs");
  sem_process_touchscreen.semCreate("spt");

  if (test_gui_mode) {
    next_displayed_gui_screen=GUI_TEST;
  } else {
//    next_displayed_gui_screen=GUI_CONNECT;
    next_displayed_gui_screen=GUI_MPD_PLAYER;
  }

  executive_init();
  exec.taskCreate("exec", executive);
  
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


