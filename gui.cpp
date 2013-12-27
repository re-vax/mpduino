#include "gui.h"

extern uint8_t SmallFont[];
extern uint8_t BigFont[];

GUI_Object::GUI_Object()
{
	this->type  = GUI_OBJECT_TYPE_NONE;
        this->action=NULL;
}

void GUI_Object::setCallbackFunction(objectCallbackFunction action){
  this->action=action;
}


GUI_Button:: GUI_Button(int x1,int y1,int x2,int y2,String label)

{
    type  = GUI_OBJECT_TYPE_BUTTON;
    this->x1=x1;
    this->y1=y1;
    this->x2=x2;
    this->y2=y2;
    this->label=label;
    this->btn_status=GUI_BUTTON_UP;
}

//  GUI_Button(x1,y1,x2,y2,label,true);


GUI_Button:: GUI_Button(int x1,int y1,int x2,int y2,String label,boolean enabled)
{
    type  = GUI_OBJECT_TYPE_BUTTON;
    this->x1=x1;
    this->y1=y1;
    this->x2=x2;
    this->y2=y2;
    this->label=label;
    if (enabled) {
      this->btn_status=GUI_BUTTON_UP;
    } else {
      this->btn_status=GUI_BUTTON_GRAYED;
    }
}

void GUI_Button::draw(UTFT glcd)
{

  glcd.setFont(BigFont);

  switch (this->btn_status) {
    case GUI_BUTTON_UP:
      glcd.setBackColor(VGA_SILVER);
      glcd.setColor(VGA_SILVER);
      glcd.fillRoundRect (x1, y1, x2, y2);
      glcd.setColor(VGA_GRAY);
      glcd.drawRoundRect (x1, y1, x2, y2);
      glcd.setColor(VGA_BLACK);
      glcd.print(label,x1+5,y1+3);
    break;
    case GUI_BUTTON_DOWN:
      glcd.setBackColor(VGA_BLUE);
      glcd.setColor(VGA_BLUE);
      glcd.fillRoundRect (x1, y1, x2, y2);
      glcd.setColor(VGA_NAVY);
      glcd.drawRoundRect (x1, y1, x2, y2);
      glcd.setColor(VGA_BLACK);
      glcd.print(label,x1+5,y1+3);
    break;
    case GUI_BUTTON_GRAYED:
      glcd.setBackColor(VGA_SILVER);
      glcd.setColor(VGA_SILVER);
      glcd.fillRoundRect (x1, y1, x2, y2);
      glcd.setColor(VGA_SILVER);
      glcd.drawRoundRect (x1, y1, x2, y2);
//      glcd.drawRect (x1+1, y1+1, x2-1, y2-1);
      glcd.setColor(VGA_GRAY);
      glcd.print(label,x1+5,y1+3);
    break;
  }
}


GUI_Screen::  GUI_Screen(){
  this->root=NULL;
  
}

int GUI_Screen::  draw(UTFT glcd){
  int count=0;
  GUI_ObjectList *current_object=this->root;
  while (current_object!=NULL && count<20) {
    count++;
    GUI_Object *obj= current_object->obj;
    obj->draw(glcd);
    current_object=current_object->next;
  }
  return count;
}

GUI_ObjectList* GUI_Screen::  getLastObject(){
  GUI_ObjectList *obj=this->root;
  if (obj==NULL) return obj; 
  while (obj->next!=NULL){
    obj=obj->next;
  }
  return obj;
}

void GUI_Screen:: add(GUI_Object *new_obj){

  GUI_ObjectList *obj=this->getLastObject();
  GUI_ObjectList *new_obj_item= new GUI_ObjectList();
  new_obj_item->next=NULL;
  new_obj_item->obj=new_obj;
  if (obj==NULL) {
    this->root=new_obj_item;
  } else {
    obj->next=new_obj_item;
  }
  
  
}

String  GUI_Screen::list_obj(){
  String retval="list obj:\n";
  int count=0;
  GUI_ObjectList *current_object=this->root;
  while (current_object!=NULL && count<20) {
    count++;
    GUI_Object *obj= current_object->obj;
    retval+=obj->type;
    retval+="\n";
    current_object=current_object->next;
  }
  retval+="Total:";
  retval+=count;
  retval+="\n";
  return retval;
}

GUI_Object * GUI_Screen::test_touch(int x,int y){
  GUI_ObjectList *obj=this->root;
  if (obj==NULL) return NULL;
  else
  while (obj!=NULL) {
    if (obj->obj->type == GUI_OBJECT_TYPE_BUTTON) {
      GUI_Button *btn=(GUI_Button*)(obj->obj);
      if (x>btn->x1 && x<btn->x2 && y>btn->y1 && y<btn->y2 ) {
        return btn;
      }
     obj=obj->next;
    }
  }
/*
  while (obj->next!=NULL){
    obj=obj->next;
  }
  */
  return NULL;
}


