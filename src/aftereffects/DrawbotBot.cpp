
//
// OpenColorIO AE
//
// After Effects implementation of OpenColorIO
//
// OpenColorIO.org
//


#include "DrawbotBot.h"


DrawbotBot::DrawbotBot(struct SPBasicSuite *pica_basicP, PF_ContextH contextH) :
	suites(pica_basicP),
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


void
DrawbotBot::SetColor(PF_App_ColorType color, float a)
{
	PF_App_Color text_color;
	suites.AppSuite4()->PF_AppGetColor(color, &text_color);
	
	_brush_color.red	= (float)text_color.red / (float)PF_MAX_CHAN16;
	_brush_color.green	= (float)text_color.green / (float)PF_MAX_CHAN16;
	_brush_color.blue	= (float)text_color.blue / (float)PF_MAX_CHAN16;
	_brush_color.alpha	= a;
}


void
DrawbotBot::DrawLineTo(float x, float y, float brush_size)
{
	DRAWBOT_PathP pathP(_suiteP, _supplier_ref);
	DRAWBOT_PenP penP(_suiteP, _supplier_ref, &_brush_color, brush_size);
	
	suites.PathSuiteCurrent()->MoveTo(pathP.Get(), _brush_pos.x, _brush_pos.y);
	
	suites.PathSuiteCurrent()->LineTo(pathP.Get(), x, y);
	
	suites.SurfaceSuiteCurrent()->StrokePath(_surface_ref, penP.Get(), pathP.Get());
	
	MoveTo(x, y);
}


void
DrawbotBot::DrawRect(float w, float h, float brush_size) const
{
	DRAWBOT_PathP pathP(_suiteP, _supplier_ref);
	DRAWBOT_PenP penP(_suiteP, _supplier_ref, &_brush_color, brush_size);
	
	DRAWBOT_RectF32 rect;
	
	rect.left	= _brush_pos.x - 0.5f;
	rect.top	= _brush_pos.y - 0.5f;
	rect.width	= w;
	rect.height	= h;

	suites.PathSuiteCurrent()->AddRect(pathP.Get(), &rect);
	
	suites.SurfaceSuiteCurrent()->StrokePath(_surface_ref, penP.Get(), pathP.Get());
}

void
DrawbotBot::PaintRect(float w, float h) const
{
	DRAWBOT_RectF32 rect;
	
	rect.left	= _brush_pos.x;
	rect.top	= _brush_pos.y;
	rect.width	= w;
	rect.height	= h;
	
	suites.SurfaceSuiteCurrent()->PaintRect(_surface_ref, &_brush_color, &rect);
}


void
DrawbotBot::PaintTriangle(float w, float h) const
{
	DRAWBOT_PathP pathP(_suiteP, _supplier_ref);
	DRAWBOT_BrushP brushP(_suiteP, _supplier_ref, &_brush_color);
	
	suites.PathSuiteCurrent()->MoveTo(pathP.Get(), _brush_pos.x, _brush_pos.y);
	
	suites.PathSuiteCurrent()->LineTo(pathP.Get(), _brush_pos.x + w, _brush_pos.y);
	
	suites.PathSuiteCurrent()->LineTo(pathP.Get(), _brush_pos.x + (w / 2.f), _brush_pos.y + h);
	
	suites.PathSuiteCurrent()->Close(pathP.Get());
	
	suites.SurfaceSuiteCurrent()->FillPath(_surface_ref, brushP.Get(), pathP.Get(), kDRAWBOT_FillType_Default);
}


void
DrawbotBot::DrawString(
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


void
DrawbotBot::DrawString(
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