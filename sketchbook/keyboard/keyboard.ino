// Notes:
// - On Ubuntu, make sure ModemManager does not claim the Arduino. Either deinstall ModemManager
//   (sudo apt-get remove modemmanager), or follow the advise here:
//   http://starter-kit.nettigo.eu/2015/serial-port-busy-for-avrdude-on-ubuntu-with-arduino-leonardo-eth/
// - Can't use A6/A7 as digital inputs, see http://forum.arduino.cc/index.php?topic=123176.0
// - Can't use D13 as input, see https://arduinodiy.wordpress.com/2012/04/28/solving-the-d13-problem-on-your-arduino/

#include "PureKeyboard.h"

const int RESTORE_PIN = A2;
const int PB_PINS[8] = { 0, 1,  2,  3,  4,  5,  6,  7 };
const int PA_PINS[8] = { 8, 9, 10, 11, 12, 13, A0, A1 }; // A0 is pin 18 on Leonardo...

int cur_state = 0;
byte keyboard_states[2][9]; // Restore is last byte, MSB

byte KEYCODES[9][8] {
  {KEY_UP_ARROW,   KEY_F5,          KEY_F3,          KEY_F1,          KEY_F7,    KEY_LEFT_ARROW, KEY_ENTER,           KEY_DELETE },
  {KEY_LEFT_SHIFT, KEY_E,           KEY_S,           KEY_Z,           KEY_4,     KEY_A,          KEY_W,               KEY_3, },
  {KEY_X,          KEY_T,           KEY_F,           KEY_C,           KEY_6,     KEY_D,          KEY_R,               KEY_5},
  {KEY_V,          KEY_U,           KEY_H,           KEY_B,           KEY_8,     KEY_G,          KEY_Y,               KEY_7},
  {KEY_N,          KEY_O,           KEY_K,           KEY_M,           KEY_0,     KEY_J,          KEY_I,               KEY_9},
  {KEY_COMMA,      KEY_PAGEUP/*@*/, KEY_PAGEUP/*:*/, KEY_PERIOD,      KEY_MINUS, KEY_L,          KEY_P,               KEY_PAGEUP /* + */},
  {KEY_SLASH,      KEY_PAGEUP/*^*/, KEY_EQUALS,      KEY_RIGHT_SHIFT, KEY_HOME,  KEY_SEMICOLON,  KEY_PAGEUP/* * */,   KEY_PAGEUP /* Pound */},
  {KEY_ESCAPE,     KEY_Q,           KEY_LEFT_GUI,    KEY_SPACE,       KEY_2,     KEY_LEFT_CTRL,  KEY_PAGEUP /* <- */, KEY_1 },
  
  {KEY_PAGEUP /* Restore */, 0, 0, 0, 0, 0, 0, 0}
};

void setup() {
  PureKeyboard.begin();
  Serial.begin(115200);

  for (int i = 0; i < 9; i++) {
    keyboard_states[0][i] = 0xff;
    keyboard_states[1][i] = 0xff;    
  }
  cur_state = 0;

  for (int i = 0; i < 8; i++) {
    pinMode(PB_PINS[i], INPUT_PULLUP);
  }
  pinMode(RESTORE_PIN, INPUT_PULLUP);

  Serial.println("Keyboard ready.");
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
//      Serial.print("changed == "); printByte(changed); Serial.println();
      int mask = 0x80;
      for (int j = 0; j < 7; j++) {
//        Serial.print("mask == "); printByte(mask); Serial.println();
//        Serial.print("change & mask == "); printByte(changed & mask); Serial.println();
        if (changed & mask) {
          int newBit = newState & mask;
          int key = KEYCODES[i][j];            
          if (newBit == 0) {
//            Serial.print("pressing ");
            PureKeyboard.press(key);
          } else {            
//            Serial.print("releasing ");
            PureKeyboard.release(key);
          }
//          Serial.println(key);
        }
        mask >>= 1;       
      }
    }
  }  
//  if (dump) {
//    Serial.print("New state: ");
//    for (int i = 0; i < 9; i++) {
//      printByte(keyboard_states[cur_state][i]); Serial.print(" ");
//    }
//    Serial.println();
//  }
}

void loop() {
  // read new state
  readState();
  handleChanges();
  cur_state = (cur_state + 1) % 2;
}

