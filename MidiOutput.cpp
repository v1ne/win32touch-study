#include "MidiOutput.h"

#include <Windows.h>
#include <sstream>
#include <iostream>

bool MidiOutput::open(unsigned int numDevice) {
  const auto numDevices = ::midiOutGetNumDevs();
  if(numDevice >= numDevices)
    return false;

  MIDIOUTCAPSW caps;
  if(::midiOutGetDevCapsW(numDevice, &caps, sizeof(MIDIOUTCAPSW)) == MMSYSERR_NOERROR)
    mDeviceName = std::wstring(caps.szPname);

  MMRESULT result;
  if((result = ::midiOutOpen((HMIDIOUT*)&mhMidiOut, numDevice, NULL, NULL, CALLBACK_NULL)) != MMSYSERR_NOERROR)
    return false;

  return true;
}

MidiOutput::~MidiOutput() {
  if(mhMidiOut) {
    auto ret = ::midiOutReset(HMIDIOUT(mhMidiOut));
    auto ret2 = ::midiOutClose(HMIDIOUT(mhMidiOut));

    if(ret != MMSYSERR_NOERROR || ret2 != MMSYSERR_NOERROR)
      ::DebugBreak();
  }
}

bool MidiOutput::sendControllerChange(uint8_t controller, uint8_t value) {
  const uint8_t channel = 0;

  RawMsg msg;
  msg.status = 0b1011 << 4 | channel;
  msg.byte1 = controller & 0b0111'1111;
  msg.byte2 = value & 0b0111'1111;

  return sendRaw(msg);
}

bool MidiOutput::sendRaw(RawMsg msg) {
  if(!mhMidiOut)
    return false;

  static_assert(sizeof(msg) == sizeof(DWORD), "packing mismatch");
  auto dwMsg = *reinterpret_cast<DWORD*>(&msg);
  if(LOWORD(dwMsg) != ((uint16_t(msg.byte1) << 8) | msg.status))
    ::DebugBreak();
  if((HIWORD(dwMsg) & 0xFF) != msg.byte2)
    ::DebugBreak();

  MMRESULT result;
  while ((result = ::midiOutShortMsg(HMIDIOUT(mhMidiOut), dwMsg)) == MIDIERR_NOTREADY)
    ::Sleep(10);

  return result == MMSYSERR_NOERROR;
}
