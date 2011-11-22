/*
 *  DrawBotBot.cpp
 *  OpenColorIO_AE
 *
 *  Created by Brendan Bolles on 11/21/11.
 *  Copyright 2011 fnord. All rights reserved.
 *
 */

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
DrawbotBot::SetColor(PF_App_ColorType color)
{
	PF_App_Color text_color;
	suites.AppSuite4()->PF_AppGetColor(PF_App_Color_TEXT, &text_color);
	
	_brush_color.red	= (float)text_color.red / (float)PF_MAX_CHAN16;
	_brush_color.green	= (float)text_color.green / (float)PF_MAX_CHAN16;
	_brush_color.blue	= (float)text_color.blue / (float)PF_MAX_CHAN16;
	_brush_color.alpha	= 1.0f;
}


void
DrawbotBot::DrawString(
	const DRAWBOT_UTF16Char *str,
	DRAWBOT_TextAlignment align,
	DRAWBOT_TextTruncation truncate,
	float truncation_width)
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
	float truncation_width)
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