// Notes:
// - For Leonardo: On Ubuntu, make sure ModemManager does not claim the 
//   Arduino. Either deinstall ModemManager (sudo apt-get remove modemmanager),
//   or follow the advise here:
//   http://starter-kit.nettigo.eu/2015/serial-port-busy-for-avrdude-on-ubuntu-with-arduino-leonardo-eth/
// - For Nano: Can't use A6/A7 as digital inputs, see http://forum.arduino.cc/index.php?topic=123176.0
// - Can't use D13 as input, see https://arduinodiy.wordpress.com/2012/04/28/solving-the-d13-problem-on-your-arduino/

#include "PureKeyboard.h"

#define DEBUG

// Input pins coming from keyboard matrix.
// If you're wondering why I'm using A3 instead of 5: It seems that the cheap
// chinese Leonardo clone I'm using is somehow buggy. When reading out row 2, 
// pin 5 would *always* return LOW. No idea what is causing this (it worked on
// on a *real* Leonardo), but switching to A3 instead seems to do the trick.
const int PB_PINS[8] = { 0, 1,  2,  3,  4, A3,  6,  7 };
const int RESTORE_PIN = A2;

// Output pins, selecting the keyboard matrix row to read.
const int PA_PINS[8] = { 8, 9, 10, 11, 12, 13, A0, A1 }; // A0 is pin 18 on Leonardo...

int cur_state = 0;
byte keyboard_states[2][9]; // Restore is last byte, MSB

byte KEYCODES[9][8] {
             /* Col 7          Col 6            Col 5            Col 4            Col 3      Col 2            Col 1                Col 0
/* Row 0 */  {KEY_UP_ARROW,   KEY_F5,          KEY_F3,          KEY_F1,          KEY_F7,    KEY_LEFT_ARROW, KEY_ENTER,           KEY_BACKSPACE },
/* Row 1 */  {KEY_LEFT_SHIFT, KEY_E,           KEY_S,           KEY_Z,           KEY_4,     KEY_A,          KEY_W,               KEY_3, },
/* Row 2 */  {KEY_X,          KEY_T,           KEY_F,           KEY_C,           KEY_6,     KEY_D,          KEY_R,               KEY_5},
/* Row 3 */  {KEY_V,          KEY_U,           KEY_H,           KEY_B,           KEY_8,     KEY_G,          KEY_Y,               KEY_7},
/* Row 4 */  {KEY_N,          KEY_O,           KEY_K,           KEY_M,           KEY_0,     KEY_J,          KEY_I,               KEY_9},
/* Row 5 */  {KEY_COMMA,      KEY_PAGEUP/*@*/, KEY_PAGEUP/*:*/, KEY_PERIOD,      KEY_MINUS, KEY_L,          KEY_P,               KEY_PAGEUP /* + */},
/* Row 6 */  {KEY_SLASH,      KEY_PAGEUP/*^*/, KEY_EQUALS,      KEY_RIGHT_SHIFT, KEY_HOME,  KEY_SEMICOLON,  KEY_PAGEUP/* * */,   KEY_PAGEUP /* Pound */},
/* Row 7 */  {KEY_ESCAPE,     KEY_Q,           KEY_LEFT_GUI,    KEY_SPACE,       KEY_2,     KEY_LEFT_CTRL,  KEY_PAGEUP /* <- */, KEY_1 },
  
  {KEY_PAGEUP /* Restore */, 0, 0, 0, 0, 0, 0, 0}
};

void setup() {
  PureKeyboard.begin();
#ifdef DEBUG
  Serial.begin(115200);
#endif  

  for (int i = 0; i < 9; i++) {
    keyboard_states[0][i] = 0xff;
    keyboard_states[1][i] = 0xff;    
  }
  cur_state = 0;

  for (int i = 0; i < 8; i++) {
    pinMode(PB_PINS[i], INPUT_PULLUP);
  }
  pinMode(RESTORE_PIN, INPUT_PULLUP);
}

byte readKeys() {  
  int val = 0;
  int b = 1;
  for (int i = 0; i < 8; i++) {
    if (digitalRead(PB_PINS[i]) == HIGH) {
      val += b;
    }
    b <<= 1;
  }
  return val;
}

void readState() {  
  // Note: If all the PA pins are kept as OUTPUT all the time, multiple
  // key presses in the same column would eradicate each other, e.g.
  // "space" and "right shift" would result in no keypress being detected.
  // With switching back to INPUT, as seen on http://biosrhythm.com/?p=910,
  // all works fine... I have not the slightest clue why, though :-(
  for (int i = 0; i < 8; i++) {
    pinMode(PA_PINS[i], OUTPUT);
    digitalWrite(PA_PINS[i], LOW);
    keyboard_states[cur_state][i] = readKeys();
    pinMode(PA_PINS[i], INPUT);
  }
  keyboard_states[cur_state][8] = digitalRead(RESTORE_PIN) == HIGH ? 0xff : 0x7f;
}

void printByte(byte val) {
  byte mask = 0x80;
  char s[9];
  for (int i = 0; i < 8; i++) {
    s[i] = val & mask ? '1' : '0';
    mask >>= 1;
  }
  s[8] = 0;
  Serial.print(s);
}

void handleChanges() {
  boolean dump = false;
  int old_state = (cur_state + 1) % 2;
  for (int i = 0; i < 9; i++) {
    byte oldState = keyboard_states[old_state][i];
    byte newState = keyboard_states[cur_state][i];
    if (oldState != newState) {
      dump = true;
      byte changed = oldState ^ newState;
#ifdef DEBUG            
      Serial.print("changed == "); printByte(changed); Serial.println();
#endif            
      int mask = 0x80;
      for (int j = 0; j < 8; j++) {
#ifdef DEBUG            
        Serial.print("mask == "); printByte(mask); 
        Serial.print("  |  change & mask == "); printByte(changed & mask); 
        Serial.println();
#endif            
        if (changed & mask) {
          int newBit = newState & mask;
          int key = KEYCODES[i][j];            
          if (newBit == 0) {
#ifdef DEBUG            
            Serial.print("pressing "); Serial.println(key);
#endif            
            PureKeyboard.press(key);
          } else {            
#ifdef DEBUG            
            Serial.print("releasing "); Serial.println(key);
#endif            
            PureKeyboard.release(key);
          }
        }
        mask >>= 1;       
      }
    }
  }  
#ifdef DEBUG  
  if (dump) {
    Serial.print("New state: ");
    for (int i = 0; i < 9; i++) {
      printByte(keyboard_states[cur_state][i]); Serial.print(" ");
    }
    Serial.println();
  }
#endif  
}

void loop() {
  // read new state
  readState();
  handleChanges();
  cur_state = (cur_state + 1) % 2;
}

