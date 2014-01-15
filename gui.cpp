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


GUI_Label::GUI_Label(int x,int y,String text)
{
    type  = GUI_OBJECT_TYPE_LABEL;
    this->x=x;
    this->y=y;
    this->text=text;
    this->need_refresh=true;
}

GUI_Label::GUI_Label(int x,int y)
{
    type  = GUI_OBJECT_TYPE_LABEL;
    this->x=x;
    this->y=y;
    this->text=String();
    this->need_refresh=true;
}

void GUI_Label:: setText(String text)
{
    this->text=text;
    this->need_refresh=true;
}

void GUI_Label:: setFont(uint8_t* font)
{
  this->font=font;
}

void GUI_Label::setColor(word textColor) {
  this->textColor=textColor;
  this->need_refresh=true;
}


void GUI_Label:: draw(UTFT glcd)
{
   glcd.setColor(this->textColor);
   glcd.setFont(this->font);
   glcd.print(this->text,this->x,this->y);
   this->need_refresh=false;
}

GUI_Button:: GUI_Button(int x,int y,int xsize,int ysize)

{
    type  = GUI_OBJECT_TYPE_BUTTON;
    this->x=x;
    this->y=y;
    this->xsize=xsize;
    this->ysize=ysize;
    this->text=String();
    this->btn_status=GUI_BUTTON_UP;
    this->need_refresh=true;  
    
    //GUI_Button(x1,y1,xsize,ysize,String());
}

GUI_Button:: GUI_Button(int x,int y,int xsize,int ysize,String text)

{
//    GUI_Button(x1,y1,xsize,ysize,text, true);
    type  = GUI_OBJECT_TYPE_BUTTON;
    this->x=x;
    this->y=y;
    this->xsize=xsize;
    this->ysize=ysize;
    this->text=text;
    this->btn_status=GUI_BUTTON_UP;
    this->need_refresh=true;
}

//  GUI_Button(x1,y1,x2,y2,label,true);


GUI_Button:: GUI_Button(int x,int y,int xsize,int ysize,String text,boolean enabled)
{
    type  = GUI_OBJECT_TYPE_BUTTON;
    this->x=x;
    this->y=y;
    this->xsize=xsize;
    this->ysize=ysize;
    this->text=text;
    if (enabled) {
      this->btn_status=GUI_BUTTON_UP;
    } else {
      this->btn_status=GUI_BUTTON_GRAYED;
    }
    this->need_refresh=true;  
}

void GUI_Button::setStatus(gui_button_status new_button_status){
  this->btn_status=new_button_status;
  this->need_refresh=true;  
}


void GUI_Button:: setText(String text)
{
  this->text=text;
  this->need_refresh=true;  
}

void GUI_Button::setColors(word buttonColor,word textColor,word borderColor,word pressedButtonColor){
  this->buttonColor=buttonColor;
  this->textColor=textColor;
  this->borderColor=borderColor;
  this->pressedButtonColor=pressedButtonColor;
  this->need_refresh=true;
}


void GUI_Button:: setFont(uint8_t* font)
{
  this->font=font;
}

void GUI_Button::draw(UTFT glcd)
{

  glcd.setFont(font);

  switch (this->btn_status) {
    case GUI_BUTTON_UP:
      glcd.setBackColor(buttonColor);
      glcd.setColor(buttonColor);
      glcd.fillRoundRect (x, y, x+xsize-1, y+ysize-1);
      glcd.setColor(borderColor);
      glcd.drawRoundRect (x, y, x+xsize-1, y+ysize-1);
      glcd.setColor(textColor);
      glcd.print(text,x+5,y+3);
    break;
    case GUI_BUTTON_DOWN:
      glcd.setBackColor(pressedButtonColor);
      glcd.setColor(pressedButtonColor);
      glcd.fillRoundRect (x, y, x+xsize-1, y+ysize-1);
      glcd.setColor(borderColor);
      glcd.drawRoundRect (x, y, x+xsize-1, y+ysize-1);
      glcd.setColor(textColor);
      glcd.print(text,x+5,y+3);
    break;
    case GUI_BUTTON_GRAYED:
      glcd.setBackColor(VGA_SILVER);
      glcd.setColor(VGA_SILVER);
      glcd.fillRoundRect (x, y, x+xsize-1, y+ysize-1);
      glcd.setColor(VGA_SILVER);
      glcd.drawRoundRect (x, y, x+xsize-1, y+ysize-1);
      glcd.setColor(VGA_GRAY);
      glcd.print(text,x+5,y+3);
    break;
  }
  this->need_refresh=false;

}


GUI_Screen::  GUI_Screen(){
  this->root=NULL;
  this->defaultFont=NULL;
  this->need_refresh=false;
  this->init_done=false;
}


int GUI_Screen::  draw(UTFT glcd){
  return draw(glcd,false);
}

int GUI_Screen::  draw(UTFT glcd,boolean fullRedraw){
  // clrscr if full redraw
  if (fullRedraw) {
    glcd.fillScr(backColor);
  }

  int count=0;
  GUI_ObjectList *current_object=this->root;
  while (current_object!=NULL && count<50) {
    count++;
    GUI_Object *obj= current_object->obj;
    if (fullRedraw || obj->need_refresh) {
      obj->draw(glcd);
    }
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
  new_obj->gui_screen=this;
  
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
    Serial.println("TT_A");
    if (obj->obj->type == GUI_OBJECT_TYPE_BUTTON) {
      GUI_Button *btn=(GUI_Button*)(obj->obj);
      if (x>=btn->x && x<btn->x+btn->xsize && y>=btn->y && y<btn->y+btn->ysize ) {
        Serial.println("TT_B");
        return btn;
      }
      Serial.println("TT_C");
    }
    obj=obj->next;
    
  }
/*
  while (obj->next!=NULL){
    obj=obj->next;
  }
  */
  return NULL;
}


