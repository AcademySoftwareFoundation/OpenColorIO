/*
Copyright (c) 2003-2012 Sony Pictures Imageworks Inc., et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include "DrawbotBot.h"


DrawbotBot::DrawbotBot(struct SPBasicSuite *pica_basicP, PF_ContextH contextH, A_long appl_id) :
    suites(pica_basicP),
    _appl_id(appl_id),
    _suiteP(NULL),
    _drawbot_ref(NULL),
    _supplier_ref(NULL),
    _surface_ref(NULL)
{
    suites.EffectCustomUISuite1()->PF_GetDrawingReference(contextH, &_drawbot_ref);
    
    _suiteP = suites.SupplierSuiteCurrent();
    
    suites.DrawbotSuiteCurrent()->GetSupplier(_drawbot_ref, &_supplier_ref);
    suites.DrawbotSuiteCurrent()->GetSurface(_drawbot_ref, &_surface_ref);
    
    _brush_pos.x = 0.f;
    _brush_pos.y = 0.f;
    
    SetColor(PF_App_Color_TEXT);
    
    _suiteP->GetDefaultFontSize(_supplier_ref, &_font_size);
}


DrawbotBot::~DrawbotBot()
{

}


void DrawbotBot::SetColor(PF_App_ColorType color, float a)
{
    if(_appl_id == 'FXTC')
    {
        PF_App_Color app_color;
    
        suites.AppSuite4()->PF_AppGetColor(color, &app_color);
        
        _brush_color.red    = (float)app_color.red / (float)PF_MAX_CHAN16;
        _brush_color.green  = (float)app_color.green / (float)PF_MAX_CHAN16;
        _brush_color.blue   = (float)app_color.blue / (float)PF_MAX_CHAN16;
    }
    else
    {
        // Premiere isn't doing this properly, so I'll have to.
        // Only supporting the colors I'm actually using at the moment.
        switch(color)
        {
            case PF_App_Color_BLACK:
                _brush_color.red = _brush_color.green = _brush_color.blue = 0.f;
                break;
            
            case PF_App_Color_WHITE:
                _brush_color.red = _brush_color.green = _brush_color.blue = 1.f;
                break;

            case PF_App_Color_RED:
                _brush_color.red = 1.f;
                _brush_color.green = _brush_color.blue = 0.f;
                break;

            case PF_App_Color_TEXT_DISABLED:
                _brush_color.red = _brush_color.green = _brush_color.blue = 0.6f;
                break;

            case PF_App_Color_SHADOW:
                _brush_color.red = _brush_color.green = _brush_color.blue = 0.3f;
                break;

            case PF_App_Color_HILITE:
                _brush_color.red = _brush_color.green = _brush_color.blue = 0.8f;
                break;

            case PF_App_Color_LIGHT_TINGE:
                _brush_color.red = _brush_color.green = _brush_color.blue = 0.7f;
                break;

            case PF_App_Color_BUTTON_FILL:
                _brush_color.red = _brush_color.green = _brush_color.blue = 0.5f;
                break;
                
            case PF_App_Color_BUTTON_PRESSED_FILL:
                _brush_color.red = _brush_color.green = _brush_color.blue = 0.3f;
                break;
            
            case PF_App_Color_PANEL_BACKGROUND:
                {
                    PF_App_Color app_color;
                    suites.AppSuite4()->PF_AppGetBgColor(&app_color);
                    
                    _brush_color.red    = (float)app_color.red / (float)65535;
                    _brush_color.green  = (float)app_color.green / (float)65535;
                    _brush_color.blue   = (float)app_color.blue / (float)65535;
                }
                break;
            
            default:
                _brush_color.red = _brush_color.green = _brush_color.blue = 0.9f;
                break;
        }
    }
    
    _brush_color.alpha  = a;
}


void DrawbotBot::DrawLineTo(float x, float y, float brush_size)
{
    DRAWBOT_PathP pathP(_suiteP, _supplier_ref);
    DRAWBOT_PenP penP(_suiteP, _supplier_ref, &_brush_color, brush_size);
    
    suites.PathSuiteCurrent()->MoveTo(pathP.Get(), _brush_pos.x, _brush_pos.y);
    
    suites.PathSuiteCurrent()->LineTo(pathP.Get(), x, y);
    
    suites.SurfaceSuiteCurrent()->StrokePath(_surface_ref, penP.Get(), pathP.Get());
    
    MoveTo(x, y);
}


void DrawbotBot::DrawRect(float w, float h, float brush_size) const
{
    DRAWBOT_PathP pathP(_suiteP, _supplier_ref);
    DRAWBOT_PenP penP(_suiteP, _supplier_ref, &_brush_color, brush_size);
    
    DRAWBOT_RectF32 rect;
    
    rect.left   = _brush_pos.x - 0.5f;
    rect.top    = _brush_pos.y - 0.5f;
    rect.width  = w;
    rect.height = h;

    suites.PathSuiteCurrent()->AddRect(pathP.Get(), &rect);
    
    suites.SurfaceSuiteCurrent()->StrokePath(_surface_ref, penP.Get(), pathP.Get());
}

void DrawbotBot::PaintRect(float w, float h) const
{
    DRAWBOT_RectF32 rect;
    
    rect.left   = _brush_pos.x;
    rect.top    = _brush_pos.y;
    rect.width  = w;
    rect.height = h;
    
    suites.SurfaceSuiteCurrent()->PaintRect(_surface_ref, &_brush_color, &rect);
}


void DrawbotBot::PaintTriangle(float w, float h) const
{
    DRAWBOT_PathP pathP(_suiteP, _supplier_ref);
    DRAWBOT_BrushP brushP(_suiteP, _supplier_ref, &_brush_color);
    
    suites.PathSuiteCurrent()->MoveTo(pathP.Get(), _brush_pos.x, _brush_pos.y);
    
    suites.PathSuiteCurrent()->LineTo(pathP.Get(), _brush_pos.x + w, _brush_pos.y);
    
    suites.PathSuiteCurrent()->LineTo(pathP.Get(), _brush_pos.x + (w / 2.f),
                                        _brush_pos.y + h);
    
    suites.PathSuiteCurrent()->Close(pathP.Get());
    
    suites.SurfaceSuiteCurrent()->FillPath(_surface_ref, brushP.Get(), pathP.Get(),
                                            kDRAWBOT_FillType_Default);
}


void DrawbotBot::DrawString(
    const DRAWBOT_UTF16Char *str,
    DRAWBOT_TextAlignment align,
    DRAWBOT_TextTruncation truncate,
    float truncation_width) const
{
    DRAWBOT_BrushP brushP(_suiteP, _supplier_ref, &_brush_color);
    DRAWBOT_FontP fontP(_suiteP, _supplier_ref, _font_size);
    
    suites.SurfaceSuiteCurrent()->DrawString(_surface_ref, brushP.Get(), fontP.Get(), str,
                        &_brush_pos, align, truncate, truncation_width);
}


void DrawbotBot::DrawString(
    const char *str,
    DRAWBOT_TextAlignment align,
    DRAWBOT_TextTruncation truncate,
    float truncation_width) const
{
    DRAWBOT_UTF16Char u_str[256] = {'\0'};
    
    DRAWBOT_UTF16Char *u = u_str;
    const char *c = str;
    
    if(*c != '\0')
    {
        do{
            *u++ = *c++;
            
        }while(*c != '\0');
        
        *u = '\0';
    }
    
    DrawString(u_str, align, truncate, truncation_width);
}