#include <MIDI.h>
#include <EveryTimer.h>

#if defined(USBCON)
#include <midi_UsbTransport.h>

static const unsigned sUsbTransportBufferSize = 16;
typedef midi::UsbTransport<sUsbTransportBufferSize> UsbTransport;



UsbTransport sUsbTransport;

MIDI_CREATE_INSTANCE(UsbTransport, sUsbTransport, MIDI);

#else // No USB available, fallback to Serial
MIDI_CREATE_DEFAULT_INSTANCE();
#endif

// --

EveryTimer timer;

const int TONEPIN = 10;
int NOTE = 0;
unsigned long NOTE_START;  // When the current note started
unsigned long SOUND_START; // When the current sound started (use for evolving vibrato)
unsigned int TONE_FREQ;
signed int GLISS_OFFSET;

signed int TRANSPOSE = 0;

const float tonefactor = 1.059463094359295;

byte VIBRATO_DEPTH = 8;
byte VIBRATO_SPEED = 100;
byte GLISSANDO = 10;

unsigned int tone_freq(int number)
{
  return (440/pow(2,4))*pow(tonefactor, number);
}

void oscillatorCallback() {
  unsigned long millis_from_start = millis() - NOTE_START;
  signed long offset = VIBRATO_DEPTH/4 * sin((float)millis_from_start * ((float)VIBRATO_SPEED/2048));
  if (NOTE > 0) {
    signed int gliss_offset = (millis_from_start < (GLISSANDO*8)) ? 
      (1-((float)millis_from_start / (GLISSANDO*8))) * (float)GLISS_OFFSET
      : 0;
    //Serial.println(String(gliss_offset));
    tone(
      TONEPIN,
      tone_freq(NOTE) + offset + gliss_offset
    );
  } else {
    noTone(TONEPIN);
  }
}

void setOscNoteOn(byte note) {
  if(NOTE > 0) {
    GLISS_OFFSET = tone_freq(NOTE) - tone_freq(note);
  } else {
    SOUND_START = millis();
  }

  NOTE_START = millis();
  NOTE = note;
}

void setOscNoteOff() {
  NOTE = 0;
  GLISS_OFFSET = 0;
}

int transposedNote(byte note) {
  return note + 3 + TRANSPOSE;
}

void handleNoteOn(byte inChannel, byte inNumber, byte inVelocity)
{
    Serial.print("NoteOn  ");
    Serial.print(inNumber);
    Serial.print("\tvelocity: ");
    Serial.println(inVelocity);
    setOscNoteOn(transposedNote(inNumber));
}

void handleNoteOff(byte inChannel, byte inNumber, byte inVelocity)
{
    Serial.print("NoteOff ");
    Serial.print(inNumber);
    Serial.print("\tvelocity: ");
    Serial.println(inVelocity);
    if(NOTE == transposedNote(inNumber)) {
      setOscNoteOff();
    }
}

void setup() {
    Serial.begin(115200);
    while (!Serial);
    MIDI.begin();
    MIDI.setHandleNoteOn(handleNoteOn);
    MIDI.setHandleNoteOff(handleNoteOff);
    timer.Every(10, oscillatorCallback);
    Serial.println("Arduino ready.");
}

void loop() {
    MIDI.read();
    timer.Update();
}
