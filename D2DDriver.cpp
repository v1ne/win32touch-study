// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved
// Copyright (c) v1ne
// Copyright (c) uucidl

#include "D2DDriver.h"

CD2DDriver::CD2DDriver(HWND hwnd)
  : m_hWnd(hwnd)
{ }

CD2DDriver::~CD2DDriver() {
  DiscardDeviceResources();
}

HRESULT CD2DDriver::Initialize() {
  HRESULT hr = CreateDeviceIndependentResources();
  hr = SUCCEEDED(hr) ? CreateDeviceResources() : hr;
  return hr;
}

HRESULT CD2DDriver::CreateDeviceIndependentResources() {
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_spD2DFactory);
    hr = SUCCEEDED(hr) ? DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&m_spDWriteFactory)) : hr;
    hr = SUCCEEDED(hr) ? m_spDWriteFactory->CreateTextFormat(
      L"Calibri", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
      12, L"", &m_spFormatSmallText) : hr;
    hr = SUCCEEDED(hr) ? m_spDWriteFactory->CreateTextFormat(
      L"Calibri", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
      18, L"", &m_spFormatMediumText) : hr;
    if (SUCCEEDED(hr)) {
      m_spFormatSmallText->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
      m_spFormatSmallText->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
      m_spFormatSmallText->SetReadingDirection(DWRITE_READING_DIRECTION_LEFT_TO_RIGHT);
      m_spFormatMediumText->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
      m_spFormatMediumText->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
      m_spFormatMediumText->SetReadingDirection(DWRITE_READING_DIRECTION_LEFT_TO_RIGHT);
    }

    return hr;
}

HRESULT CD2DDriver::CreateDeviceResources() {
    HRESULT hr = S_OK;

    if(!m_spRT) {
      hr = CreateRenderTarget();
      if (FAILED(hr))
        return hr;

      CreateGradient(pGlossyStops, &m_spGLBrush,
        D2D1::ColorF::White, 0.5f, 0.3f,
        D2D1::ColorF::White, 0.0f, 1.0f);
      CreateGradient(pGradientStops, &m_spBLBrush,
        D2D1::ColorF::Aqua, 1.0f, 1.0f,
        D2D1::ColorF::DarkBlue, 1.0f, 0.0f);
      CreateGradient(pGradientStops2, &m_spORBrush,
        D2D1::ColorF::Yellow, 1.0f, 1.0f,
        D2D1::ColorF::OrangeRed, 1.0f, 0.0f);
      CreateGradient(pGradientStops3, &m_spREBrush,
        D2D1::ColorF::Red, 1.0f, 1.0f,
        D2D1::ColorF::Maroon, 1.0f, 0.0f);
      CreateGradient(pGradientStops4, &m_spGRBrush,
        D2D1::ColorF::GreenYellow, 1.0f, 1.0f,
        D2D1::ColorF::Green, 1.0f, 0.0f);
      CreateGradient(pGradientStops5, &m_spBGBrush,
        D2D1::ColorF::LightSlateGray, 1.0f, 1.0f,
        D2D1::ColorF::Black, 1.0f, 0.0f);

      m_spRT->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Enum::LightGray), &m_spLightGreyBrush);
      m_spRT->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Enum::DarkGray), &m_spDarkGreyBrush);
      m_spRT->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Enum::CornflowerBlue), &m_spCornflowerBrush);
      m_spRT->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Enum::DimGray), &m_spDimGreyBrush);
      m_spRT->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Enum::MediumSlateBlue), &m_spSomePinkishBlueBrush);
      m_spRT->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Enum::MediumSeaGreen), &m_spSomeGreenishBrush);
      m_spRT->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Enum::Black), &m_spSemitransparentDarkBrush);
      m_spSemitransparentDarkBrush->SetOpacity(0.4f);
      m_spRT->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Enum::Black), &m_spBlackBrush);
      m_spRT->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Enum::White), &m_spWhiteBrush);
    }

    return hr;
}


HRESULT CD2DDriver::RenderBackground(Point2F size) {
    m_spBGBrush->SetStartPoint(D2D1::Point2F(size.x/2, 0.0f));
    m_spBGBrush->SetEndPoint(D2D1::Point2F(size.x/2, size.y));
    D2D1_RECT_F background = D2D1::RectF(0, 0, size.x, size.y);
    m_spRT->FillRectangle(&background, m_spBGBrush);

    return S_OK;
}

VOID CD2DDriver::DiscardDeviceResources() {
    m_spRT.Release();
    m_spBLBrush.Release();
    m_spORBrush.Release();
    m_spGRBrush.Release();
    m_spREBrush.Release();
    m_spGLBrush.Release();
    m_spBGBrush.Release();
    m_spSemitransparentDarkBrush.Release();
    m_spDimGreyBrush.Release();
    m_spLightGreyBrush.Release();
    m_spDarkGreyBrush.Release();
    m_spCornflowerBrush.Release();
    m_spSomePinkishBlueBrush.Release();
    m_spSomeGreenishBrush.Release();
    m_spBlackBrush.Release();
    m_spWhiteBrush.Release();
}

HRESULT CD2DDriver::CreateRenderTarget() {
  RECT rc;
  GetClientRect(m_hWnd, &rc);
  D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
  return m_spD2DFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(),
    D2D1::HwndRenderTargetProperties(m_hWnd, size), &m_spRT);
}

VOID CD2DDriver::BeginDraw() {
  m_spRT->BeginDraw();
  m_spRT->Clear(D2D1::ColorF(D2D1::ColorF::White));
}

VOID CD2DDriver::EndDraw() {
  auto hr = m_spRT->EndDraw();
  if(hr == D2DERR_RECREATE_TARGET)
    DiscardDeviceResources();
}

HRESULT CD2DDriver::CreateGradient(ID2D1GradientStopCollection* pStops, ID2D1LinearGradientBrush** pplgBrush,
    D2D1::ColorF::Enum startColor, FLOAT startOpacity, FLOAT startPos,
    D2D1::ColorF::Enum endColor, FLOAT endOpacity, FLOAT endPos) {
  D2D1_GRADIENT_STOP stops[2];
  stops[0].color = D2D1::ColorF(startColor, startOpacity);
  stops[0].position = startPos;
  stops[1].color = D2D1::ColorF(endColor, endOpacity);
  stops[1].position = endPos;

  auto hr = m_spRT->CreateGradientStopCollection(stops, 2, &pStops);
  if(SUCCEEDED(hr)) {
    hr = m_spRT->CreateLinearGradientBrush(D2D1::LinearGradientBrushProperties(
        D2D1::Point2F(0.0f, 0.0f),
        D2D1::Point2F(0.0f, 0.0f)),
      D2D1::BrushProperties(),
      pStops,
      pplgBrush);
  }

  return hr;
}


ID2D1HwndRenderTargetPtr CD2DDriver::GetRenderTarget() {
    return m_spRT;
}

ID2D1LinearGradientBrushPtr CD2DDriver::get_GradBrush(unsigned int uBrushType) {
  switch(uBrushType) {
    case GRB_Glossy: return m_spGLBrush;
    case GRB_Blue: return m_spBLBrush;
    case GRB_Orange: return m_spORBrush;
    case GRB_Red: return m_spREBrush;
    case GRB_Green: return m_spGRBrush;
    default: return m_spGRBrush;
  }
}

void CD2DDriver::RenderText(D2D1_RECT_F rect, const wchar_t* buf, size_t len, ID2D1Brush* pBrush) {
  m_spRT->DrawTextW(buf, UINT32(len), m_spFormatSmallText, rect, pBrush);
}

void CD2DDriver::RenderMediumText(D2D1_RECT_F rect, const wchar_t* buf, size_t len, ID2D1Brush* pBrush) {
  m_spRT->DrawTextW(buf, UINT32(len), m_spFormatMediumText, rect, pBrush);
}



void CD2DDriver::RenderTiltedRect(Point2F base, float distance, float degAngle, Point2F size, ID2D1Brush* pBrush) {
  const auto middleOfRectNearBase = base + rotateDeg(Vec2Right(distance), degAngle);
  const auto halfRectHeightR = rotateDeg(Vec2Up(size.y/2), degAngle);
  const auto rectLengthR = rotateDeg(Vec2Right(size.x), degAngle);
  const auto p1 = middleOfRectNearBase + halfRectHeightR;
  const auto p2 = middleOfRectNearBase - halfRectHeightR;
  const auto p3 = p2 + rectLengthR;
  const auto p4 = p1 + rectLengthR;
  const D2D1_POINT_2F d2dPoints[4] =
    {p2.to<D2D1_POINT_2F>(), p3.to<D2D1_POINT_2F>(), p4.to<D2D1_POINT_2F>(), p1.to<D2D1_POINT_2F>()};

  ID2D1PathGeometryPtr pathGeometry;
  auto hr = m_spD2DFactory->CreatePathGeometry(&pathGeometry);
  if (FAILED(hr))
    return;

  ID2D1GeometrySink *pSink = NULL;
  hr = pathGeometry->Open(&pSink);
  if (FAILED(!hr))
    return;

  pSink->SetFillMode(D2D1_FILL_MODE_WINDING);
  pSink->BeginFigure(p1.to<D2D_POINT_2F>(), D2D1_FIGURE_BEGIN_FILLED);
  pSink->AddLines(d2dPoints, 4);
  pSink->EndFigure(D2D1_FIGURE_END_CLOSED);
  hr = pSink->Close();
  m_spRT->FillGeometry(pathGeometry, pBrush);
}
