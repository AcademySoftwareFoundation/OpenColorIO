
//
//	mmm...our custom UI
//
//

#include "OpenColorIO_AE.h"

#include "OpenColorIO_AE_Context.h"

#include "OpenColorIO_AE_Dialogs.h"

#include "DrawbotBot.h"


using namespace std;


#define LEFT_MARGIN 50
#define TOP_MARGIN	50
#define RIGHT_MARGIN	50

#define FILE_LABEL_WIDTH	30

#define FIELD_HEIGHT	22

#define FIELD_TEXT_INDENT_H	10
#define FIELD_TEXT_INDENT_V	4

#define MENUS_GAP_V			20

#define MENU_LABEL_WIDTH	100
#define MENU_LABEL_SPACE	5

#define MENU_WIDTH	150
#define MENU_HEIGHT	20

#define MENU_TEXT_INDENT_H	10
#define MENU_TEXT_INDENT_V	2

#define MENU_ARROW_WIDTH	14
#define MENU_ARROW_HEIGHT	7

#define MENU_ARROW_SPACE_H	8
#define MENU_ARROW_SPACE_V	7

#define MENU_SHADOW_OFFSET	3

#define MENU_SPACE_V 20

#define	TEXT_COLOR		PF_App_Color_TEXT_DISABLED

typedef enum {
	REGION_NONE=0,
	REGION_PATH,
	REGION_MENU1,
	REGION_MENU2,
	REGION_MENU3
} UIRegion;

static UIRegion
WhichRegion(PF_Point ui_point, bool menus, bool third_menu)
{
	if(ui_point.v >= TOP_MARGIN && ui_point.v <= (TOP_MARGIN + FIELD_HEIGHT))
	{
		if(ui_point.h >= (LEFT_MARGIN + FILE_LABEL_WIDTH))
			return REGION_PATH;
	}
	else if(menus &&
			ui_point.h >= (LEFT_MARGIN + FILE_LABEL_WIDTH + MENU_LABEL_WIDTH) &&
			ui_point.h <= (LEFT_MARGIN + FILE_LABEL_WIDTH + MENU_LABEL_WIDTH + MENU_WIDTH))
	{
		int menu1_top = TOP_MARGIN + FIELD_HEIGHT + MENUS_GAP_V;
		int menu1_bottom = menu1_top + MENU_HEIGHT;
		int menu2_top = menu1_bottom + MENU_SPACE_V;
		int menu2_bottom = menu2_top + MENU_HEIGHT;
		int menu3_top = menu2_bottom + MENU_SPACE_V;
		int menu3_bottom = menu3_top + MENU_HEIGHT;
		
		if(ui_point.v >= menu1_top && ui_point.v <= menu1_bottom)
			return REGION_MENU1;
		else if(ui_point.v >= menu2_top && ui_point.v <= menu2_bottom)
			return REGION_MENU2;
		else if(third_menu && ui_point.v >= menu3_top && ui_point.v <= menu3_bottom)
			return REGION_MENU3;
	}
	
	return REGION_NONE;
}

static void
DrawMenu(DrawbotBot &bot, const char *label, const char *item)
{
	DRAWBOT_PointF32 original = bot.Pos();
	
	float text_height = bot.FontSize();
	
	bot.Move(MENU_LABEL_WIDTH, MENU_TEXT_INDENT_V + text_height);
	
	bot.SetColor(TEXT_COLOR);
	bot.DrawString(label, kDRAWBOT_TextAlignment_Right);
	
	bot.Move(MENU_LABEL_SPACE, -(MENU_TEXT_INDENT_V + text_height));
	
	DRAWBOT_PointF32 menu_corner = bot.Pos();
	
	bot.Move(MENU_SHADOW_OFFSET, MENU_SHADOW_OFFSET);
	bot.SetColor(PF_App_Color_BLACK, 0.3f);
	bot.PaintRect(MENU_WIDTH, MENU_HEIGHT);
	bot.MoveTo(menu_corner);
	
	bot.SetColor(PF_App_Color_SHADOW);
	bot.PaintRect(MENU_WIDTH, MENU_HEIGHT);
	
	bot.SetColor(PF_App_Color_HILITE);
	bot.DrawRect(MENU_WIDTH, MENU_HEIGHT);
	
	bot.Move(MENU_TEXT_INDENT_H, MENU_TEXT_INDENT_V + text_height);
	
	bot.SetColor(TEXT_COLOR);
	bot.DrawString(item,
					kDRAWBOT_TextAlignment_Left,
					kDRAWBOT_TextTruncation_EndEllipsis,
					MENU_WIDTH - MENU_TEXT_INDENT_H - MENU_TEXT_INDENT_H - MENU_ARROW_WIDTH - MENU_ARROW_SPACE_H - MENU_ARROW_SPACE_H);
	
	bot.MoveTo(menu_corner.x + MENU_WIDTH - MENU_ARROW_SPACE_H - MENU_ARROW_WIDTH, menu_corner.y + MENU_ARROW_SPACE_V);
	
	bot.SetColor(PF_App_Color_LIGHT_TINGE);
	bot.PaintTriangle(MENU_ARROW_WIDTH, MENU_ARROW_HEIGHT);
	
	bot.MoveTo(original);
}


static PF_Err
DrawEvent(	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output,
	PF_EventExtra	*event_extra)
{
	PF_Err			err		=	PF_Err_NONE;
	
	//AEGP_SuiteHandler suites(in_data->pica_basicP);
	

	event_extra->evt_out_flags = 0;
	

	if (!(event_extra->evt_in_flags & PF_EI_DONT_DRAW) && params[OCIO_DATA]->u.arb_d.value != NULL)
	{
		if(event_extra->effect_win.area == PF_EA_CONTROL)
		{
			ArbitraryData *arb_data = (ArbitraryData *)PF_LOCK_HANDLE(params[OCIO_DATA]->u.arb_d.value);
			SequenceData *seq_data = (SequenceData *)PF_LOCK_HANDLE(in_data->sequence_data);
		
		
			DrawbotBot bot(in_data->pica_basicP, event_extra->contextH);
			
			
			int panel_width = event_extra->effect_win.current_frame.right;
			float text_height = bot.FontSize();
			
			bool have_file = (arb_data->path[0] != '\0');
			
			
			bot.MoveTo(LEFT_MARGIN + FILE_LABEL_WIDTH, TOP_MARGIN);
			
			DRAWBOT_PointF32 field_corner = bot.Pos();
			
			bot.SetColor(PF_App_Color_SHADOW);
			bot.PaintRect(panel_width - LEFT_MARGIN - RIGHT_MARGIN, FIELD_HEIGHT);
			bot.SetColor(PF_App_Color_BLACK);
			bot.DrawRect(panel_width - LEFT_MARGIN - RIGHT_MARGIN, FIELD_HEIGHT);

			bot.Move(-FIELD_TEXT_INDENT_H, FIELD_TEXT_INDENT_V + text_height);
			bot.SetColor(TEXT_COLOR);
			bot.DrawString("File:", kDRAWBOT_TextAlignment_Right);
			
			bot.Move(2 * FIELD_TEXT_INDENT_H);
			bot.SetColor(TEXT_COLOR);
			
			if(have_file)
			{
				bot.DrawString(arb_data->path,
								kDRAWBOT_TextAlignment_Default,
								kDRAWBOT_TextTruncation_PathEllipsis,
								panel_width - LEFT_MARGIN - RIGHT_MARGIN - FIELD_TEXT_INDENT_H - FIELD_TEXT_INDENT_H);
			}
			else
			{
				bot.DrawString("(none)");
			}
			
			
			bot.MoveTo(field_corner.x, field_corner.y + FIELD_HEIGHT + MENUS_GAP_V);
			
			if(have_file)
			{
				if(arb_data->type == ARB_TYPE_OCIO)
				{
					// color space transformations
					DrawMenu(bot, "Input Space:", arb_data->input);
					
					bot.Move(0, MENU_HEIGHT + MENU_SPACE_V);
					
					DrawMenu(bot, "Transform:", arb_data->transform);
					
					bot.Move(0, MENU_HEIGHT + MENU_SPACE_V);
					
					DrawMenu(bot, "Device:", arb_data->device);
				}
			}
			
			
			event_extra->evt_out_flags = PF_EO_HANDLED_EVENT;

			PF_UNLOCK_HANDLE(params[OCIO_DATA]->u.arb_d.value);
			PF_UNLOCK_HANDLE(in_data->sequence_data);
		}
	}

	return err;
}


template <typename T>
static int
FindInVec(vector<T> &vec, T val)
{
	for(int i=0; i < vec.size(); i++)
	{
		if(vec[i] == val)
			return i;
	}
	
	return 0;
}


static PF_Err
DoClick( 
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output,
	PF_EventExtra	*event_extra )
{
	PF_Err			err		=	PF_Err_NONE;
	
	ArbitraryData *arb_data = (ArbitraryData *)PF_LOCK_HANDLE(params[OCIO_DATA]->u.arb_d.value);
	SequenceData *seq_data = (SequenceData *)PF_LOCK_HANDLE(in_data->sequence_data);
	
	UIRegion reg = WhichRegion(event_extra->u.do_click.screen_point, true, true);
	
	if(reg == REGION_PATH)
	{
		ExtensionMap extensions;
		
		for(int i=0; i < OCIO::FileTransform::getNumFormats(); i++)
		{
			const char *extension = OCIO::FileTransform::getFormatExtensionByIndex(i);
			const char *format = OCIO::FileTransform::getFormatNameByIndex(i);
		
			extensions[ extension ] = format;
		}
		
		extensions[ "ocio" ] = "OCIO Format";
		
		
		char path[ARB_PATH_LEN + 1];
		
		bool result = OpenFile(path, ARB_PATH_LEN, extensions, NULL);
		
		
		if(result)
		{
			try
			{
				OpenColorIO_AE_Context *new_context = new OpenColorIO_AE_Context(path);
				
				if(new_context == NULL)
					throw OCIO::Exception("WTF?");
					
				
				if(seq_data->context)
				{
					delete seq_data->context;
				}
				
				seq_data->context = new_context;
				
				
				strncpy(arb_data->path, path, ARB_PATH_LEN);
				
				if( seq_data->context->isOCIO() )
				{
					arb_data->type = ARB_TYPE_OCIO;
					
					strncpy(arb_data->input, seq_data->context->getInput().c_str(), ARB_SPACE_LEN);
					strncpy(arb_data->transform, seq_data->context->getTransform().c_str(), ARB_SPACE_LEN);
					strncpy(arb_data->device, seq_data->context->getDevice().c_str(), ARB_SPACE_LEN);
				}
				
				
				params[OCIO_DATA]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;
			}
			catch(...)
			{
				PF_SPRINTF(out_data->return_msg, "OCIO Error reading file.");
				
				out_data->out_flags |= PF_OutFlag_DISPLAY_ERROR_MESSAGE;
			}
			
		}
	}
	else if(reg != REGION_NONE && seq_data->context && seq_data->context->isOCIO())
	{
		if(seq_data->context == NULL)
		{
			try
			{
				seq_data->context = new OpenColorIO_AE_Context(arb_data);
			}
			catch(...) {}
		}
				

		if(seq_data->context != NULL && seq_data->context->isOCIO())
		{
			MenuVec menu_items;
			
			int selected_item;
			
			if(arb_data->type == ARB_TYPE_OCIO)
			{
				if(reg == REGION_MENU1)
				{
					menu_items = seq_data->context->getInputs();
					
					selected_item = FindInVec<string>(menu_items, string(arb_data->input));
				}
				else if(reg == REGION_MENU2)
				{
					menu_items = seq_data->context->getTransforms();
					
					selected_item = FindInVec<string>(menu_items, string(arb_data->transform));
				}
				else if(reg == REGION_MENU3)
				{
					menu_items = seq_data->context->getDevices();
					
					selected_item = FindInVec<string>(menu_items, string(arb_data->device));
				}
			}
			
			
			int result = PopUpMenu(menu_items, selected_item);
			
			
			if(result != selected_item)
			{
				if(arb_data->type == ARB_TYPE_OCIO)
				{
					string color_space = menu_items[ result ];
				
					if(reg == REGION_MENU1)
					{
						strncpy(arb_data->input, color_space.c_str(), ARB_SPACE_LEN);
					}
					else if(reg == REGION_MENU2)
					{
						strncpy(arb_data->transform, color_space.c_str(), ARB_SPACE_LEN);
					}
					else if(reg == REGION_MENU3)
					{
						strncpy(arb_data->device, color_space.c_str(), ARB_SPACE_LEN);
					}
					
					try
					{
						seq_data->context->setupOCIO(arb_data->input, arb_data->transform, arb_data->device);
					}
					catch(...) {}
				}
				
				params[OCIO_DATA]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;
			}
		}
	}
	
	
	PF_UNLOCK_HANDLE(params[OCIO_DATA]->u.arb_d.value);
	PF_UNLOCK_HANDLE(in_data->sequence_data);
	
	event_extra->evt_out_flags = PF_EO_HANDLED_EVENT;
	
	return err;
}


static PF_Err
DoAdjustCursor( 
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output,
	PF_EventExtra	*event_extra )
{
	UIRegion reg = WhichRegion(event_extra->u.adjust_cursor.screen_point, false, false);
	
	if(reg == REGION_PATH)
	{
	#ifdef MAC_ENV
		#if PF_AE_PLUG_IN_VERSION >= PF_AE100_PLUG_IN_VERSION
		SetMickeyCursor(); // the cute mickey mouse hand
		#else
		SetThemeCursor(kThemePointingHandCursor);
		#endif
		event_extra->u.adjust_cursor.set_cursor = PF_Cursor_CUSTOM;
	#else
		event_extra->u.adjust_cursor.set_cursor = PF_Cursor_FINGER_POINTER;
	#endif
	}
	
	event_extra->evt_out_flags = PF_EO_HANDLED_EVENT;
}


PF_Err
HandleEvent ( 
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output,
	PF_EventExtra	*extra )
{
	PF_Err		err		= PF_Err_NONE;
	
	if (!err) 
	{
		switch (extra->e_type) 
		{
			case PF_Event_DRAW:
				err =	DrawEvent(in_data, out_data, params, output, extra);
				break;
			
			case PF_Event_DO_CLICK:
				err = DoClick(in_data, out_data, params, output, extra);
				break;
				
			case PF_Event_ADJUST_CURSOR:
				DoAdjustCursor(in_data, out_data, params, output, extra);
				break;
		}
	}
	
	return err;
}
