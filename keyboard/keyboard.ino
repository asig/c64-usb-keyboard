// Notes:
// - Can't use A6/A7 as digital inputs, see http://forum.arduino.cc/index.php?topic=123176.0
// - Can't use D13 as input, see https://arduinodiy.wordpress.com/2012/04/28/solving-the-d13-problem-on-your-arduino/

const int RESTORE_PIN = 2;
const int PB_PIN = RESTORE_PIN + 1;
const int PA_PIN = PB_PIN + 8;

int cur_state = 0;
byte keyboard_states[2][9]; // Restore is last byte, MSB

void setup() {
  for (int i = 0; i < 9; i++) {
    keyboard_states[0][i] = 0xff;
    keyboard_states[1][i] = 0xff;    
  }
  cur_state = 0;


  Serial.begin(115200);
  Serial.print("PA = "); Serial.println(PA_PIN);
  Serial.print("PA + 8 = "); Serial.println(PA_PIN + 8);
  Serial.print("PB = "); Serial.println(PB_PIN);
  for (int i = PB_PIN; i < PB_PIN + 8; i++) {
    pinMode(i, INPUT_PULLUP);
  }
  for (int i = PA_PIN; i < PA_PIN + 8; i++) {
    pinMode(i, OUTPUT);
  }
  pinMode(RESTORE_PIN, INPUT_PULLUP);
}

byte readKeys() {  
  int val = 0;
  for (int i = PB_PIN + 7; i >= PB_PIN; i--) {
    val = val << 1;
    if (digitalRead(i) == HIGH) {
      val += 1;
    }
  }
  return val;
}

void writeMask(int b) {
  for (int i = 0; i < 8; i++) {
    digitalWrite(PA_PIN + i, i == b ? LOW : HIGH);
  }
}

void readState() {  
  for (int i = 0; i < 8; i++) {
    writeMask(i);
    keyboard_states[cur_state][i] = readKeys();
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

void loop() {
  // read new state
  readState();

  bool changed = false;
  int old_state = (cur_state + 1) % 2;
  for (int i = 0; i < 9; i++) {
    if (keyboard_states[old_state][i] != keyboard_states[cur_state][i]) {
      changed = true;
      break;
    }
  }

  if (changed) {
    Serial.print("New state: ");
    for (int i = 0; i < 9; i++) {
      printByte(keyboard_states[cur_state][i]);
    }
    Serial.println();
  }

  cur_state = (cur_state + 1) % 2;
  
//
//  Serial.print("-----\n");
//  Serial.print("RESTORE : "); Serial.print(digitalRead(RESTORE_PIN)); Serial.println();
//  delay(1000);
//  Keyboard.press('A');
//  delay(500);
//  Keyboard.release('A');
}
