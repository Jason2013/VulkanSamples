/*
*--------------------------------------------------------------------------
* Copyright (c) 2015-2016 The Khronos Group Inc.
* Copyright (c) 2015-2016 Valve Corporation
* Copyright (c) 2015-2016 LunarG, Inc.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* Author: Rene Lindsay <rene@lunarg.com>
*
*--------------------------------------------------------------------------
* FIFO Buffer is used in the few cases where event messages need to be buffered.
* EventType contains a union struct of all possible message types that may be retured by PollEvent.
* WindowImpl is the abstraction layer base class for the platform-specific windowing code.
*--------------------------------------------------------------------------
*/

#ifndef WINDOWIMPL_H
#define WINDOWIMPL_H

#include "CInstance.h"
#include "keycodes.h"

typedef unsigned int uint;
enum eMouseAction{ mMOVE, mDOWN, mUP };
enum eKeyAction  { keyDOWN, keyUP };

//======================== FIFO Buffer =========================
template <typename T,uint SIZE> class FIFO{
    int head, tail;
    T buf[SIZE]={};
public:
    FIFO():head(0),tail(0){}
    bool isEmpty(){return head==tail;}                                    // Check if queue is empty.
    void push(T const& item){ ++head; buf[head%=SIZE]=item; }             // Add item to queue
    T* pop(){ if(head==tail)return 0; ++tail; return &buf[tail%=SIZE]; }  // Returns item ptr, or null if queue is empty
};
//==============================================================

//========================Event Message=========================
struct EventType{
    enum{NONE, MOUSE, KEY, TEXT, SHAPE, FOCUS, TOUCH} tag;                      // event type
    union{
        struct {eMouseAction action; int16_t x; int16_t y; uint8_t btn;}mouse;  // mouse move/click
        struct {eKeyAction   action; uint8_t keycode;}key;                      // Keyboard key state
        struct {const char* str;}text;                                          // Text entered
        struct {int16_t x; int16_t y; uint16_t width; uint16_t height;}shape;   // Window move/resize
        struct {bool hasFocus;}focus;                                           // Window gained/lost focus
        struct {eMouseAction action; float x; float y; uint8_t id;}touch;       // multi-touch screen
    };
    void Clear(){tag=NONE;}
};
//==============================================================

//=====================WSIWindow base class=====================
class WindowImpl {
    struct {int16_t x; int16_t y;}mousepos = {};                           // mouse position
    bool btnstate[5]   = {};                                               // mouse btn state
    bool keystate[256] = {};                                               // keyboard state
protected:
    CInstance* instance;
    VkSurfaceKHR surface;
    FIFO<EventType,4> eventFIFO;                        //Event message queue buffer (max 4 items)

    EventType MouseEvent(eMouseAction action, int16_t x, int16_t y, uint8_t btn);  // Mouse event
    EventType KeyEvent  (eKeyAction action, uint8_t key);                          // Keyboard event
    EventType TextEvent (const char* str);                                         // Text event
    EventType ShapeEvent(int16_t x, int16_t y, uint16_t width, uint16_t height);   // Window move/resize
    EventType FocusEvent(bool hasFocus);                                           // Window gained/lost focus  TODO: Expose this as an event?
public:
    bool running;
    bool textinput;
    bool has_focus;
    struct shape_t {int16_t x; int16_t y; uint16_t width; uint16_t height;}shape;  // window shape

    WindowImpl() : instance(0), running(false), textinput(false), has_focus(false){}
    virtual ~WindowImpl() {}
    virtual void Close() { running = false; }
    CInstance& Instance() { return *instance; }
    bool HasFocus() { return has_focus; }                                  // returns true if window has focus
    bool KeyState(eKeycode key){ return keystate[key]; }                   // returns true if key is pressed
    bool BtnState(uint8_t  btn){ return (btn<5)  ? btnstate[btn]:0; }      // returns true if mouse btn is pressed
    void MousePos(int16_t& x, int16_t& y){x=mousepos.x; y=mousepos.y; }    // returns mouse x,y position

    virtual void TextInput(bool enabled);          //Enable TextEvent, (and on Android, show the soft-keyboard)
    virtual bool TextInput(){return textinput;}    //Returns true if text input is enabled (and on android, keyboard is visible.)

    virtual EventType GetEvent()=0; //fetch one event from the queue
};
//==============================================================

#endif
