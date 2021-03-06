// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once

#ifndef WINVER
#define WINVER 0x0A00 // Windows 10
#endif

#include "ComTouchDriver.h"
#include "MidiOutput.h"

#include <memory>
#include <tchar.h>
#include <tpcshrd.h>
#include <windows.h>


HWND ghWnd;
std::unique_ptr<CComTouchDriver> gpTouchDriver;
MidiOutput gMidiOutput;

ATOM MyRegisterClass(HINSTANCE hInst);
BOOL InitInstance(HINSTANCE hinst, int nCmdShow, ATOM hClass);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void SetTabletInputServiceProperties();
void FillInputData(TOUCHINPUT* inData, DWORD cursor, DWORD eType, DWORD time, int x, int y);

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR pCmdLine, int nCmdShow) {
  UNREFERENCED_PARAMETER(pCmdLine);
  UNREFERENCED_PARAMETER(nCmdShow);

  if(FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)))
  return 0;

  if (!InitInstance(hInstance, SW_SHOWMAXIMIZED, MyRegisterClass(hInstance))) {
    wprintf(L"Failed to initialize application");
    return 0;
  }

  if (!gMidiOutput.open(1)) {
    printf("Failed to open MIDI output\n");
    ::OutputDebugStringA("Failed to open MIDI output\n");
  } else {
    wprintf(L"Connected to MIDI device: %s\n", gMidiOutput.mDeviceName.c_str());
    ::OutputDebugStringA("Connected to MIDI device: ");
    ::OutputDebugStringW(gMidiOutput.mDeviceName.c_str());
    ::OutputDebugStringA("\n");
  }

  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return 1;
}

// Register Window Class
ATOM MyRegisterClass(HINSTANCE hInst)
{
  WNDCLASSEX wc;
  ZeroMemory(&wc, sizeof(wc));
  wc.cbSize = sizeof(WNDCLASSEX);
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = WndProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = hInst;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
  wc.lpszClassName = L"Win32TouchSlidersExperiment";

  return RegisterClassEx(&wc);
}

// Initialize Instance
BOOL InitInstance(HINSTANCE hInst, int nCmdShow, ATOM hClass)
{
  BOOL success = TRUE;

  ghWnd = CreateWindowEx(0, MAKEINTATOM(hClass), L"Win32 Touch Sliders", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
    CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInst, 0);


  if(!ghWnd)
  {
    success = FALSE;
  }

  // Create and initialize touch and mouse handler

  if(success)
  {
    gpTouchDriver = std::make_unique<CComTouchDriver>(ghWnd);
    success = gpTouchDriver->Initialize();
  }

  if(success)
  {
    // Disable UI feedback for penflicks
    SetTabletInputServiceProperties();

    // Ready for handling WM_TOUCH messages
    RegisterTouchWindow(ghWnd, 0);

    ShowWindow(ghWnd, nCmdShow);
    UpdateWindow(ghWnd);
  }
  return success;
}

// Processes messages for main Window
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  PTOUCHINPUT pInputs;
  TOUCHINPUT tInput;
  HTOUCHINPUT hInput;
  PAINTSTRUCT ps;

  // Handle each type of inData and based on which event we handle continue processing
  // the inData for manipulations by calling the ComTouchDriver

  switch (msg)
  {
  case WM_TOUCH: {
    auto NumContacts = LOWORD(wParam);
    hInput = (HTOUCHINPUT)lParam;

    pInputs = new TOUCHINPUT[NumContacts];
    if(::GetTouchInputInfo(hInput, NumContacts, pInputs, sizeof(TOUCHINPUT))) {
      for(int i = 0; i < NumContacts; i++) {
        POINT physicalPoint = {pInputs[i].x/100, pInputs[i].y/100};
        ::ScreenToClient(ghWnd, &physicalPoint);
        pInputs[i].x = physicalPoint.x;
        pInputs[i].y = physicalPoint.y;
        gpTouchDriver->ProcessInputEvent(&pInputs[i]);
      }
      gpTouchDriver->RunInertiaProcessorsAndRender();
    }

    delete [] pInputs;
    CloseTouchInputHandle(hInput);
    break; }

  case WM_LBUTTONDOWN:
    FillInputData(&tInput, MOUSE_CURSOR_ID, TOUCHEVENTF_DOWN, (DWORD)GetMessageTime(),LOWORD(lParam),HIWORD(lParam));
    gpTouchDriver->ProcessInputEvent(&tInput);
    break;

  case WM_MOUSEMOVE:
    if(LOWORD(wParam) & MK_LBUTTON) {
      FillInputData(&tInput, MOUSE_CURSOR_ID, TOUCHEVENTF_MOVE, (DWORD)GetMessageTime(),LOWORD(lParam), HIWORD(lParam));
      gpTouchDriver->ProcessInputEvent(&tInput);
      gpTouchDriver->RunInertiaProcessorsAndRender();
    }
    break;

  case WM_LBUTTONUP:
    FillInputData(&tInput, MOUSE_CURSOR_ID, TOUCHEVENTF_UP, (DWORD)GetMessageTime(),LOWORD(lParam), HIWORD(lParam));
    gpTouchDriver->ProcessInputEvent(&tInput);
    break;

  case WM_DESTROY:
    PostQuitMessage(0);
    return 1;

  case WM_SIZE: {
    RECT rect;
    ::GetClientRect(ghWnd, &rect);
    gpTouchDriver->RenderInitialState({rect.right - rect.left, rect.bottom - rect.top});
    break; }

  case WM_PAINT:
    BeginPaint(ghWnd, &ps);
    gpTouchDriver->RenderObjects();
    EndPaint(ghWnd, &ps);
    break;

  case WM_TIMER:
    gpTouchDriver->RunInertiaProcessorsAndRender();
    break;

  case WM_KEYDOWN:
  case WM_KEYUP:
    if (wParam == 0x10)
      gShiftPressed = msg == WM_KEYDOWN;
    break;

  case WM_KILLFOCUS:
    gShiftPressed = false;
    break;

  default:
    return DefWindowProc(hWnd, msg, wParam, lParam);
  }
  return 0;
}

// Fills the input data received from the mouse and builds the tagTOUCHINPUT struct

void FillInputData(TOUCHINPUT* inData, DWORD cursor, DWORD eType, DWORD time, int x, int y)
{
  inData->dwID = cursor;
  inData->dwFlags = eType;
  inData->dwTime = time;
  inData->x = x;
  inData->y = y;
}

void SetTabletInputServiceProperties()
{
  DWORD_PTR dwHwndTabletProperty =
  TABLET_DISABLE_PRESSANDHOLD | // disables press and hold (right-click) gesture
  TABLET_DISABLE_PENTAPFEEDBACK | // disables UI feedback on pen up (waves)
  TABLET_DISABLE_PENBARRELFEEDBACK | // disables UI feedback on pen button down (circle)
  TABLET_DISABLE_FLICKS; // disables pen flicks (back, forward, drag down, drag up)

  ATOM atom = ::GlobalAddAtom(MICROSOFT_TABLETPENSERVICE_PROPERTY);
  SetProp(ghWnd, MICROSOFT_TABLETPENSERVICE_PROPERTY, reinterpret_cast<HANDLE>(dwHwndTabletProperty));
  GlobalDeleteAtom(atom);
}
