// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef _DRAWBOTBOT_H_
#define _DRAWBOTBOT_H_

#include "AEGP_SuiteHandler.h"


class DrawbotBot
{
  public:
  
    DrawbotBot(struct SPBasicSuite *pica_basicP, PF_ContextH contextH, A_long appl_id);
    ~DrawbotBot();
    
    void MoveTo(DRAWBOT_PointF32 pos) { _brush_pos = pos; }
    void MoveTo(float x, float y) { _brush_pos.x = x; _brush_pos.y = y; }
    void Move(float x = 0, float y = 0) { _brush_pos.x += x; _brush_pos.y += y; }
    
    void SetColor(PF_App_ColorType color, float a = 1.f);
    void SetColor(DRAWBOT_ColorRGBA color) { _brush_color = color; }
    void SetColor(float r, float g, float b, float a = 1.f)
        {   _brush_color.red = r; _brush_color.green = g;
            _brush_color.blue = b; _brush_color.alpha = a; }

    DRAWBOT_PointF32 Pos() const { return _brush_pos; }
    float FontSize() const { return _font_size; }
    
    void DrawLineTo(float x, float y, float brush_size = 0.5f);

    void DrawRect(float w, float h, float brush_size = 0.5f) const;
    void PaintRect(float w, float h) const;
    
    void PaintTriangle(float w, float h) const;

    void DrawString(const DRAWBOT_UTF16Char *str,
                    DRAWBOT_TextAlignment align = kDRAWBOT_TextAlignment_Default,
                    DRAWBOT_TextTruncation truncate = kDRAWBOT_TextTruncation_None,
                    float truncation_width = 0.f) const;
    void DrawString(const char *str,
                    DRAWBOT_TextAlignment align = kDRAWBOT_TextAlignment_Default,
                    DRAWBOT_TextTruncation truncate = kDRAWBOT_TextTruncation_None,
                    float truncation_width = 0.f) const;
    
    
  private:
    AEGP_SuiteHandler       suites;
    A_long                  _appl_id;
    
    DRAWBOT_SupplierSuiteCurrent *_suiteP;

    DRAWBOT_DrawRef         _drawbot_ref;
    DRAWBOT_SupplierRef     _supplier_ref;
    DRAWBOT_SurfaceRef      _surface_ref;
    
    DRAWBOT_PointF32        _brush_pos;
    DRAWBOT_ColorRGBA       _brush_color;
    float                   _font_size;
};


#endif // _DRAWBOTBOT_H_