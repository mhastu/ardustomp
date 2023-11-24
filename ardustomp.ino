#include <MIDI.h>
#include <midi_Defs.h>
#include <midi_Message.h>
#include <midi_Namespace.h>
#include <midi_Settings.h>

MIDI_CREATE_INSTANCE(HardwareSerial, Serial, midiOut);

// =================================== CONFIG ======================================
// -------------------- pin selection --------------------
const uint8_t diPins[] = { 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 };  // digital pins to use for note on/off
const uint8_t anPins[] = { 0, 1, 2 };  // analog pins to use for MIDI control changes
// -------------------------------------------------------

// ------- digital input contact bounce prevention -------
const uint8_t diAntiBounceLength = 16;  // number of digital values to store to prevent contact bounce (0-16)
const uint16_t diHighCompare = 255 << (16-diAntiBounceLength) >> (16-diAntiBounceLength);  // valid value of diAntiBounceBuffer for a digital value change
// -------------------------------------------------------

// ----------- analog input noise suppression ------------
const uint8_t anMovingAverage = 4;  // window size of moving average for analog pins to tackle noise
// moving average is not enough:
const uint8_t anDirChangeThreshold = 4;  // values to overcome for direction change (in MIDI space, i.e. 0-127)
// -------------------------------------------------------
// =================================================================================

// =========================== GLOBAL STATE VARIABLES ==============================
// ---------------- toggle-able states -------------------
uint8_t noteMode[sizeof(diPins)];  // 0: sent note-on/off represents hardware button state, 1: toggle note-on/off on hardware button press
uint8_t noteState[sizeof(diPins)];  // current state of toggle-able notes (1=on, 0=off)
// -------------------------------------------------------

// ----------- for change detection in loop() ------------
uint8_t diVal[sizeof(diPins)];  // value for each digital pin in current iteration
uint8_t diLastVal[sizeof(diPins)];  // value for each digital pin in last iteration

uint8_t anVal[sizeof(anPins)];  // value for each analog pin (0-127) in current iteration
uint8_t anLastVal[sizeof(anPins)];  // value for each analog pin (0-127) in last iteration
bool anDirUp[sizeof(anPins)];  // current change direction of analog value (0-127). false: down, true: up
// -------------------------------------------------------

// --- for digital sensor value mapping in readInput() ---
uint16_t diAntiBounceBuffer[sizeof(diPins)];  // last diAntiBounceLength boolean values
// -------------------------------------------------------

// --- for analog sensor value mapping in readInput() ----
uint16_t anWindow[sizeof(anPins)][anMovingAverage];  // last anMovingAverage sensor values (0-1023)
uint8_t anWindowIndex = 0;  // current index in window array
// -------------------------------------------------------
// =================================================================================

void setup () {
  for (uint8_t i = 0; i < sizeof(diPins); i++) {
    pinMode(diPins[i], INPUT_PULLUP);
  }
  Serial.begin(31250);

  // set note mode by holding down hardware buttons during boot
  for (uint8_t i = 0; i < sizeof(diPins); i++) {
    if (digitalRead(diPins[i]) == LOW) {  // LOW = pressed
      noteMode[i] = 1;
    } else {
      noteMode[i] = 0;
    }
  }
}

// use anVal and diVal to detect change and send MIDI signal
void loop() {
  readInput();
  
  //analogPins
  for (uint16_t i = 0; i < sizeof(anPins); i++) {
    bool changed = false;
    if (anDirUp[i]) {
      if (anVal[i] < (anLastVal[i] - anDirChangeThreshold)) {
        anDirUp[i] = false;
        changed = true;
      } else if (anVal[i] > anLastVal[i]) {
        changed = true;
      }
    } else {
      if (anVal[i] > (anLastVal[i] + anDirChangeThreshold)) {
        anDirUp[i] = true;
        changed = true;
      } else if (anVal[i] < anLastVal[i]) {
        changed = true;
      }
    }
    if (changed) {
      midiOut.sendControlChange(16 + i, anVal[i], 1);
      anLastVal[i] = anVal[i];
    }
  }
  
  //digitalPins
  for (uint16_t i = 0; i < sizeof(diPins); i++) {
    if (diVal[i] != diLastVal[i]) {
      if (diVal[i] == LOW) {  // note ON at LOW value: Atmega has internal Pullup-Resistors, so GND is being switched
        if (noteMode[i] == 0) {  // "normal" button mode
          midiOut.sendNoteOn(i, 127, 2);
        } else { // toggle-button mode
          if (noteState[i] == 0) {
            midiOut.sendNoteOn(i, 127, 2);
            noteState[i] = 1;
          } else {
            midiOut.sendNoteOff(i, 127, 2);
            noteState[i] = 0;
          }
        }
      } else {
        if (noteMode[i] == 0) {  // "normal" button mode
          midiOut.sendNoteOff(i, 127, 2);
        }  // else: nothing to do in toggle mode
      }
      diLastVal[i] = diVal[i];
    }
  }
}

// reads analog and digital input, filters them (moving average for analog,
// contact bounce correction for digital) and sets anVal and diVal
void readInput() {
  //analogPins
  anWindowIndex = anWindowIndex % anMovingAverage;  // fix index overflow
  for (uint8_t i = 0; i < sizeof(anPins); i++) {
    anWindow[i][anWindowIndex] = analogRead(anPins[i]);
    delay(1); //delay for stability
    uint16_t sum = 0;
    for (uint8_t j = 0; j < anMovingAverage; j++) {
      sum += anWindow[i][j];
    }
    
    anVal[i] = (uint8_t) (map(sum / anMovingAverage, 0, 1023, 0, 127));
  }
  anWindowIndex++;
  
  //digitalPins
  for (uint8_t i = 0; i < sizeof(diPins); i++) {
    diAntiBounceBuffer[i] = (diAntiBounceBuffer[i] & diHighCompare);  // apply mask to remove too old values
    uint8_t sens = digitalRead(diPins[i]);
    if ((diAntiBounceBuffer[i] == 0x0) || (diAntiBounceBuffer[i] == diHighCompare)) {  // new value is only valid if no bouncing in buffer
      diVal[i] = sens;
    }
    diAntiBounceBuffer[i] = diAntiBounceBuffer[i] << 1;  // make space for new value
    if (sens == HIGH) {
      bitSet(diAntiBounceBuffer[i], 0);
    }
  }
}
