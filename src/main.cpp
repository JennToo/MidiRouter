#include <Arduino.h>
#include <SD.h>

const uint32_t MIDI_BAUD = 31250;
const size_t MIDI_PORTS = 8;
const size_t READ_BUFFER = 256;

HardwareSerial *ports[MIDI_PORTS] = {
    &Serial1,
    &Serial2,
    &Serial3,
    &Serial4,
    &Serial5,
    &Serial6,
    &Serial7,
    &Serial8,
};

using PortMatrix = HardwareSerial *[MIDI_PORTS][MIDI_PORTS];

struct MidiMatrix
{
  String name;
  PortMatrix ports;

  MidiMatrix() : MidiMatrix("EMPTY") {}

  MidiMatrix(String name) : name(name)
  {
    memset(&ports, 0, sizeof(PortMatrix));
  }
};

MidiMatrix matrix;
int preset;
char buffer[READ_BUFFER];

void duplicateInput();
bool loadPreset(int);

void setup()
{
  Serial.begin(115200);

  for (size_t i = 0; i < MIDI_PORTS; ++i)
  {
    ports[i]->begin(MIDI_BAUD);
  }
  preset = 0;

  if (!SD.begin(BUILTIN_SDCARD))
  {
    Serial.println("Failed to load SD card!");
  }

  loadPreset(preset);
}

void loop()
{
  duplicateInput();
}

void duplicateInput()
{
  for (size_t input = 0; input < MIDI_PORTS; ++input)
  {
    if (!ports[input]->available())
    {
      return;
    }

    int byteCount = Serial.readBytes(buffer, READ_BUFFER);

    for (unsigned output = 0; output < MIDI_PORTS; ++output)
    {
      HardwareSerial *port = matrix.ports[input][output];
      if (port == nullptr)
      {
        continue;
      }

      port->write(buffer, byteCount);
    }
  }
}

bool loadPreset(int preset)
{
  String filename = String(preset) + String("_matrix.txt");
  if (!SD.exists(filename.c_str()))
  {
    Serial.print("Preset does not exist: ");
    Serial.println(filename);
    return false;
  }

  File file = SD.open(filename.c_str(), FILE_READ);
  int count = file.readBytesUntil('\n', buffer, READ_BUFFER);
  buffer[count] = 0;
  matrix.name = String(buffer);

  for(size_t input = 0; input < MIDI_PORTS; ++input)
  {
    int count = file.readBytesUntil('\n', buffer, READ_BUFFER);
    size_t output = 0;
    for(size_t i = 0; i < count; ++i)
    {
      int index = ((int)(buffer[i])) - ((int)'1');
      if(index >= 0 && index < MIDI_PORTS)
      {
        matrix.ports[input][output] = ports[index];
      }
      else
      {
        Serial.printf("Unrecognized output port: %c\n", buffer[i]);
      }
    }
  }

  return true;
}
