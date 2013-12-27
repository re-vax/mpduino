#include <UTFT.h>

#ifndef GUI_h
#define GUI_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif


extern "C"
{
  // callback functions always follow the signature: void cmd(void);
  typedef void (*objectCallbackFunction) (void);
}



enum gui_object_type {
  GUI_OBJECT_TYPE_NONE,
  GUI_OBJECT_TYPE_BUTTON
};

enum gui_button_status {
  GUI_BUTTON_UP,
  GUI_BUTTON_DOWN,
  GUI_BUTTON_GRAYED
};


// Common object class
class GUI_Object
{
public:
  int	type;
  GUI_Object();
  virtual void draw(UTFT glcd)=0;
  void setCallbackFunction(objectCallbackFunction action);
//private:
  objectCallbackFunction action;
  
};


// Button
class GUI_Button :  public GUI_Object
{
public:
//  int type;
  int x1,y1,x2,y2;
  int btn_status;
  String label;
  //GUI_Button();
  GUI_Button(int x1, int y1, int x2, int y2,String label);
  GUI_Button(int x1, int y1, int x2, int y2,String label,boolean enabled);
  virtual void draw(UTFT glcd);
};


struct GUI_ObjectList{
  GUI_Object *obj;
  GUI_ObjectList * next;
};


// Screen
class GUI_Screen {
public:
  GUI_ObjectList *root;
  GUI_Screen();
  void add(GUI_Object *new_obj);
  int draw(UTFT glcd);
  GUI_Object * test_touch(int x,int y);
  
  String list_obj();
private:
  GUI_ObjectList* getLastObject();
};




#endif



