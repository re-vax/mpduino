#include <UTFT.h>

#ifndef GUI_h
#define GUI_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif


#define LABEL_DEBUG_BORDER false

extern "C"
{
  // callback functions always follow the signature: void cmd(void);
  typedef void (*objectCallbackFunction) (void);
}

typedef void (*event_cb_t)(int userdata);

struct event_cb {
    event_cb_t cb;
    int data;
};


enum gui_object_type {
  GUI_OBJECT_TYPE_NONE,
  GUI_OBJECT_TYPE_BUTTON,
  GUI_OBJECT_TYPE_LABEL,
  GUI_OBJECT_TYPE_SLIDER,
  GUI_OBJECT_TYPE_PROGRESS_BAR
};

enum gui_button_status {
  GUI_BUTTON_UP,
  GUI_BUTTON_DOWN,
  GUI_BUTTON_GRAYED
};

enum gui_slider_status {
  GUI_SLIDER_UP,
  GUI_SLIDER_DOWN,
  GUI_SLIDER_GRAYED
};

// just a class definition
class GUI_Screen;



// Common object class
class GUI_Object
{
public:
  int	type;
  boolean need_refresh;
  boolean visible;
  GUI_Object();
  virtual void draw(UTFT glcd)=0;
  void setCallbackFunction(objectCallbackFunction action);
//private:
  objectCallbackFunction action;
  struct event_cb *callback;

  GUI_Screen *gui_screen;
};


struct GUI_ObjectList{
  GUI_Object *obj;
  GUI_ObjectList * next;
};



// Button
class GUI_Button :  public GUI_Object
{
public:
//  int type;
  int x,y,xsize,ysize;
  int btn_status;
  String text;
  uint8_t* font;
  word buttonColor,textColor,borderColor,pressedButtonColor;
  //GUI_Button();
  GUI_Button(int x, int y, int xsize, int ysize);
  GUI_Button(int x, int y, int xsize, int ysize,String text);
  GUI_Button(int x, int y, int xsize, int ysize,String text,boolean enabled);
  void setText(String text);
  void setStatus(gui_button_status new_button_status);
  void setFont(uint8_t* font);
  void setColors(word buttonColor,word textColor,word borderColor,word pressedButtonColor);
  virtual void draw(UTFT glcd);
};


// Slider
class GUI_Slider: public GUI_Object
{
  public:
  int x,y,xsize,ysize;
  int slider_status;
  String text;
  uint8_t* font;
  word buttonColor,textColor,borderColor,pressedButtonColor;
  GUI_Slider(int x, int y, int xsize, int ysize);
  GUI_Slider(int x, int y, int xsize, int ysize,String text);
  GUI_Slider(int x, int y, int xsize, int ysize,String text,boolean enabled);
  void setText(String text);
  void setStatus(gui_slider_status new_slider_status);
  void setFont(uint8_t* font);
  void setColors(word sliderColor,word textColor,word borderColor,word pressedSliderColor);
  virtual void draw(UTFT glcd);
};

// Progress Bar
class GUI_ProgressBar: public GUI_Object{
  public:
  int x,y,xsize,ysize;
  int fill_percentage;
  word borderColor,fillColor;
 
  GUI_ProgressBar(int x, int y, int xsize, int ysize,int fill_percentage);
  void setColors(word borderColor,word fillColor);
  virtual void draw(UTFT glcd);
};

// Label
class GUI_Label :  public GUI_Object
{
public:
  int x,y,xsize,ysize;
  String text;
  word textColor;
  uint8_t* font;
  GUI_Label(int x, int y);
  GUI_Label(int x, int y,String text);
//  GUI_Label(int x, int y,int xsize,int ysize);
//  GUI_Label(int x, int y,int xsize,int ysize,String text);
  void setText(String text);
  void setFont(uint8_t* font);
  void setColor(word textColor);
  virtual void draw(UTFT glcd);
};


// Screen
class GUI_Screen {
public:
  GUI_ObjectList *root;
  GUI_Screen();
  boolean init_done;
  boolean need_refresh;
  long backColor;
  long defaultFrontColor;
  uint8_t *defaultFont;
  void add(GUI_Object *new_obj);
  int draw(UTFT glcd);
  int draw(UTFT glcd,boolean fullRedraw);
  GUI_Object * test_touch(int x,int y);
  
  String list_obj();
private:
  GUI_ObjectList* getLastObject();
};




#endif



