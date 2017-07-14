#pragma once

#include <cstdint>
#include <string>

class MidiOutput {
public:
  ~MidiOutput();

  #pragma pack(push,1)
  #pragma warning(disable: 4201)
  struct RawMsg {
    uint8_t status;
    union {
      uint8_t data[2];
      struct {
        uint8_t byte1;
        uint8_t byte2;
      };
    };
    uint8_t dummy;
  };
  #pragma pack(pop)

  bool open(unsigned int numDevice);
  bool sendControllerChange(uint8_t controller, uint8_t value); // on channel 0
  bool sendRaw(RawMsg);

  std::wstring mDeviceName;

private:
  void* mhMidiOut = nullptr;
};
