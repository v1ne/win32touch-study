// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#ifndef D2DDRIVER_H
#define D2DDRIVER_H

#include "Geometry.h"

#include <d2d1.h>
#include <d2d1helper.h>	
#include <dwrite.h>	
#include <comdef.h>

_COM_SMARTPTR_TYPEDEF(ID2D1Factory, __uuidof(ID2D1Factory));
_COM_SMARTPTR_TYPEDEF(ID2D1HwndRenderTarget, __uuidof(ID2D1HwndRenderTarget));
_COM_SMARTPTR_TYPEDEF(ID2D1LinearGradientBrush, __uuidof(ID2D1LinearGradientBrush));
_COM_SMARTPTR_TYPEDEF(ID2D1SolidColorBrush, __uuidof(ID2D1SolidColorBrush));
_COM_SMARTPTR_TYPEDEF(ID2D1RectangleGeometry, __uuidof(ID2D1RectangleGeometry));
_COM_SMARTPTR_TYPEDEF(ID2D1RoundedRectangleGeometry, __uuidof(ID2D1RoundedRectangleGeometry));
_COM_SMARTPTR_TYPEDEF(ID2D1EllipseGeometry, __uuidof(ID2D1EllipseGeometry));
_COM_SMARTPTR_TYPEDEF(ID2D1PathGeometry, __uuidof(ID2D1PathGeometry));
_COM_SMARTPTR_TYPEDEF(IDWriteFactory, __uuidof(IDWriteFactory));
_COM_SMARTPTR_TYPEDEF(IDWriteTextFormat, __uuidof(IDWriteTextFormat));

class CD2DDriver {
public:
    CD2DDriver(HWND hwnd);
    ~CD2DDriver();
    HRESULT Initialize();

    // D2D Methods

    HRESULT CreateRenderTarget();
    HRESULT CreateDeviceIndependentResources();
    HRESULT CreateDeviceResources();
    VOID DiscardDeviceResources();
    
    HRESULT RenderBackground(Point2F size);
    ID2D1HwndRenderTargetPtr GetRenderTarget();
    ID2D1LinearGradientBrushPtr get_GradBrush(unsigned int uBrushType);

    void RenderText(D2D1_RECT_F rect, const wchar_t* buf, size_t len, ID2D1Brush* pBrush);
    void RenderMediumText(D2D1_RECT_F rect, const wchar_t* buf, size_t len, ID2D1Brush* pBrush);
    void RenderTiltedRect(Point2F basePos, float distance, float degAngle, Point2F size, ID2D1Brush* pBrush);

    VOID BeginDraw();
    VOID EndDraw();

    enum {GRB_Glossy, GRB_Blue, GRB_Orange, GRB_Red, GRB_Green};

    ID2D1FactoryPtr m_spD2DFactory;

    ID2D1SolidColorBrushPtr m_spBlackBrush;
    ID2D1SolidColorBrushPtr m_spWhiteBrush;
    ID2D1SolidColorBrushPtr m_spLightGreyBrush;
    ID2D1SolidColorBrushPtr m_spDarkGreyBrush;
    ID2D1SolidColorBrushPtr m_spCornflowerBrush;
    ID2D1SolidColorBrushPtr m_spSomePinkishBlueBrush;
    ID2D1SolidColorBrushPtr m_spSomeGreenishBrush;
    ID2D1SolidColorBrushPtr m_spSemitransparentDarkBrush;
    ID2D1SolidColorBrushPtr m_spDimGreyBrush;

private:
    // Helper to create gradient resource
    HRESULT CreateGradient(ID2D1GradientStopCollection* pStops, 
        ID2D1LinearGradientBrush** pplgBrush, 
        D2D1::ColorF::Enum startColor, 
        FLOAT startOpacity, 
        FLOAT startPos, 
        D2D1::ColorF::Enum endColor, 
        FLOAT endOpacity, 
        FLOAT endPos);

    // Handle to the main window
    HWND m_hWnd;

    IDWriteFactoryPtr m_spDWriteFactory;
    ID2D1HwndRenderTargetPtr m_spRT;

    // Gradient Brushes
    ID2D1LinearGradientBrushPtr m_spGLBrush;
    ID2D1LinearGradientBrushPtr m_spBLBrush;
    ID2D1LinearGradientBrushPtr m_spORBrush;
    ID2D1LinearGradientBrushPtr m_spGRBrush;
    ID2D1LinearGradientBrushPtr m_spREBrush;
    ID2D1LinearGradientBrushPtr m_spBGBrush;

    // Solid Brushes

    // Gradient Stops for Gradient Brushes
    ID2D1GradientStopCollection *pGradientStops;
    ID2D1GradientStopCollection *pGradientStops2;
    ID2D1GradientStopCollection *pGradientStops3;
    ID2D1GradientStopCollection *pGradientStops4;
    ID2D1GradientStopCollection *pGradientStops5;
    ID2D1GradientStopCollection *pGlossyStops;

    IDWriteTextFormatPtr m_spFormatSmallText;
    IDWriteTextFormatPtr m_spFormatMediumText;
};
#endif