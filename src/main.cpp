#include <Arduino.h>
#include <MIDI.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <RotaryEncoder.h>

Adafruit_SH1106G display = Adafruit_SH1106G(128, 64, &Wire);

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, midiS1);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial2, midiS2);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial3, midiS3);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial4, midiS4);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial5, midiS5);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial6, midiS6);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial7, midiS7);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial8, midiS8);

using SerialMidiT = midi::MidiInterface<midi::SerialMIDI<HardwareSerial>>;
const size_t SERIAL_MIDI_COUNT = 8;
SerialMidiT *serialMidis[] = {
    &midiS1,
    &midiS2,
    &midiS3,
    &midiS4,
    &midiS5,
    &midiS6,
    &midiS7,
    &midiS8,
};

const size_t MIDI_PORT_COUNT = SERIAL_MIDI_COUNT + 1;
const size_t USB_MIDI_PORT = MIDI_PORT_COUNT - 1;

struct ConnectivityScheme
{
  const char *name;
  boolean connections[MIDI_PORT_COUNT][MIDI_PORT_COUNT];
};

ConnectivityScheme DEFAULT_SCHEME = {
    .name = "KB+USB IN",
    .connections = {
        {1, 1, 1, 1, 1, 1, 1, 1, 1},
        {0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0},
        {1, 1, 1, 1, 1, 1, 1, 1, 0},
    }};
ConnectivityScheme SEQ_SCHEME = {
    .name = "SEQ FUNNEL",
    .connections = {
        {0, 1, 0, 0, 0, 0, 0, 0, 1},
        {0, 0, 1, 1, 1, 1, 1, 1, 1},
        {0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 1, 0, 0, 0, 0, 0, 0, 0},
    }};
ConnectivityScheme USB_SCHEME = {
    .name = "USB FUNNEL",
    .connections = {
        {0, 0, 0, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 0},
    }};
const size_t SCHEME_COUNT = 3;
ConnectivityScheme *SCHEMES[SCHEME_COUNT] = {
    &DEFAULT_SCHEME, &SEQ_SCHEME, &USB_SCHEME};
ConnectivityScheme *currentScheme = &DEFAULT_SCHEME;
size_t schemeIndex;

bool ioTable[2][MIDI_PORT_COUNT] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
};

size_t PIN_ENCODER_SWITCH = 30;
RotaryEncoder encoder(31, 32, RotaryEncoder::LatchMode::TWO03);
int encoderPosition;

void drawIoTable()
{
  for (size_t row = 0; row < 2; ++row)
  {
    for (size_t col = 0; col < MIDI_PORT_COUNT; ++col)
    {
      uint16_t color = ioTable[row][col] ? 1 : 0;
      display.fillRect(2 * 6 + 2 * col * 6, 4 * 8 + row * 8 + 1, 8, 6, color);
    }
  }
}

void drawScheme()
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.printf("MODE: %s", currentScheme->name);
  display.setCursor(0, 3 * 8);
  display.print("  1 2 3 4 5 6 7 8 U");
  display.setCursor(0, 4 * 8);
  display.print("I");
  display.setCursor(0, 5 * 8);
  display.print("O");
}

elapsedMillis drawTimer;

void setup()
{
  Serial.begin(115200);

  for (size_t i = 0; i < SERIAL_MIDI_COUNT; ++i)
  {
    serialMidis[i]->begin(MIDI_CHANNEL_OMNI);
    serialMidis[i]->setThruFilterMode(midi::Thru::Off);
  }
  usbMIDI.begin();
  Serial.println("MIDI interfaces initialized");

  pinMode(PIN_ENCODER_SWITCH, INPUT_PULLUP);

  encoder.tick();
  encoderPosition = encoder.getPosition();

  schemeIndex = 0;

  display.begin(0x3C, true);
  drawScheme();
  drawIoTable();
  display.display();
}

void loop()
{
  for (size_t inPort = 0; inPort < SERIAL_MIDI_COUNT; ++inPort)
  {
    SerialMidiT *inMidi = serialMidis[inPort];
    if (inMidi->read())
    {
      ioTable[0][inPort] = 1;
      auto message = inMidi->getMessage();

      Serial.printf("Found Serial message %d %d %d on %d\r\n", message.type, message.data1, message.data2, inPort);
      for (size_t outPort = 0; outPort < SERIAL_MIDI_COUNT; ++outPort)
      {
        if (currentScheme->connections[inPort][outPort])
        {
          serialMidis[outPort]->send(message);
          ioTable[1][outPort] = 1;
        }
      }

      if (currentScheme->connections[inPort][USB_MIDI_PORT])
      {
        if (message.type >= midi::MidiType::SystemExclusiveStart && message.type <= midi::MidiType::SystemExclusiveEnd)
        {
          usbMIDI.sendSysEx(message.getSysExSize(), message.sysexArray);
        }
        else
        {
          usbMIDI.send(message.type, message.data1, message.data2, message.channel, 0);
        }
        ioTable[1][USB_MIDI_PORT] = 1;
      }
    }
  }

  if (usbMIDI.read())
  {
    ioTable[0][USB_MIDI_PORT] = 1;
    auto type = usbMIDI.getType();
    auto data1 = usbMIDI.getData1();
    auto data2 = usbMIDI.getData2();
    auto channel = usbMIDI.getChannel();

    Serial.printf("Found USB message %d %d %d %d\r\n", type, data1, data2);
    for (size_t outPort = 0; outPort < SERIAL_MIDI_COUNT; ++outPort)
    {
      if (currentScheme->connections[USB_MIDI_PORT][outPort])
      {
        if (type >= midi::MidiType::SystemExclusiveStart && type <= midi::MidiType::SystemExclusiveEnd)
        {
          serialMidis[outPort]->sendSysEx(usbMIDI.getSysExArrayLength(), usbMIDI.getSysExArray());
        }
        else
        {
          serialMidis[outPort]->send((midi::MidiType)type, data1, data2, channel);
        }
        ioTable[1][outPort] = 1;
      }
    }
  }

  if (drawTimer > 100)
  {
    drawIoTable();
    display.display();
    drawTimer = 0;
    memset((void *)ioTable, 0, sizeof(ioTable));
  }

  encoder.tick();
  int newPosition = encoder.getPosition();
  if (newPosition != encoderPosition)
  {
    int newIndex = schemeIndex + (int)encoder.getDirection();
    if (newIndex < 0)
    {
      newIndex = SCHEME_COUNT - 1;
    }
    else if (newIndex >= (int)SCHEME_COUNT)
    {
      newIndex = 0;
    }
    schemeIndex = newIndex;
    currentScheme = SCHEMES[schemeIndex];
    drawScheme();
    display.display();
    Serial.printf("Switching to new scheme %s\r\n", currentScheme->name);
    encoderPosition = newPosition;
  }
}
