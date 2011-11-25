
//
//	mmm...our custom UI
//
//

#include "OpenColorIO_AE.h"

#include "OpenColorIO_AE_Context.h"

#include "OpenColorIO_AE_Dialogs.h"

#include "DrawbotBot.h"


using namespace std;


// UI drawing constants

#define LEFT_MARGIN			5
#define TOP_MARGIN			5
#define RIGHT_MARGIN		50

#define FILE_LABEL_WIDTH	35

#define FIELD_HEIGHT		22

#define FIELD_TEXT_INDENT_H	10
#define FIELD_TEXT_INDENT_V	4

#define BUTTONS_INDENT_H	(LEFT_MARGIN + 70)

#define BUTTONS_GAP_V		20
#define BUTTONS_GAP_H		30

#define BUTTON_HEIGHT		20
#define BUTTON_WIDTH		80

#define BUTTON_TEXT_INDENT_V 3

#define MENUS_INDENT_H		20

#define MENUS_GAP_V			20

#define MENU_LABEL_WIDTH	100
#define MENU_LABEL_SPACE	5

#define MENU_WIDTH			150
#define MENU_HEIGHT			20

#define MENU_TEXT_INDENT_H	10
#define MENU_TEXT_INDENT_V	2

#define MENU_ARROW_WIDTH	14
#define MENU_ARROW_HEIGHT	7

#define MENU_ARROW_SPACE_H	8
#define MENU_ARROW_SPACE_V	7

#define MENU_SHADOW_OFFSET	3

#define MENU_SPACE_V		20

#define	TEXT_COLOR		PF_App_Color_TEXT_DISABLED



typedef enum {
	REGION_NONE=0,
	REGION_PATH,
	REGION_CONVERT_BUTTON,
	REGION_DISPLAY_BUTTON,
	REGION_EXPORT_BUTTON,
	REGION_MENU1,
	REGION_MENU2,
	REGION_MENU3
} UIRegion;


static UIRegion
WhichRegion(PF_Point ui_point, bool menus, bool third_menu)
{
	int field_top = TOP_MARGIN;
	int field_bottom = field_top + FIELD_HEIGHT;
	
	if(ui_point.v >= field_top && ui_point.v <= field_bottom)
	{
		int field_left = LEFT_MARGIN + FILE_LABEL_WIDTH;
		
		if(ui_point.h >= field_left)
			return REGION_PATH;
	}
	else
	{
		int buttons_top = field_bottom + BUTTONS_GAP_V;
		int buttons_bottom = buttons_top + BUTTON_HEIGHT;
		
		if(ui_point.v >= buttons_top && ui_point.v <= buttons_bottom)
		{
			int convert_left = BUTTONS_INDENT_H;
			int convert_right = convert_left + BUTTON_WIDTH;
			int display_left = convert_right + BUTTONS_GAP_H;
			int display_right = display_left + BUTTON_WIDTH;
			int export_left = display_right + BUTTONS_GAP_H;
			int export_right = export_left + BUTTON_WIDTH;
			
			if(ui_point.h >= convert_left && ui_point.h <= convert_right)
				return REGION_CONVERT_BUTTON;
			else if(ui_point.h >= display_left && ui_point.h <= display_right)
				return REGION_DISPLAY_BUTTON;
			else if(ui_point.h >= export_left && ui_point.h <= export_right)
				return REGION_EXPORT_BUTTON;
		}
		else if(menus)
		{
			int menu_left = LEFT_MARGIN + MENUS_INDENT_H + MENU_LABEL_WIDTH;
			int menu_right = menu_left + MENU_WIDTH;
		
			if(ui_point.h >= menu_left && ui_point.h <= menu_right)
			{
				int menu1_top = buttons_bottom + MENUS_GAP_V;
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
		}
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


static void
DrawButton(DrawbotBot &bot, const char *label, int width, bool pressed)
{
	DRAWBOT_PointF32 original = bot.Pos();
	
	float text_height = bot.FontSize();
	
	
	PF_App_ColorType button_color = (pressed ? PF_App_Color_BUTTON_PRESSED_FILL : PF_App_Color_BUTTON_FILL);
	PF_App_ColorType button_hilite = (pressed ? PF_App_Color_BLACK : PF_App_Color_HILITE);
	PF_App_ColorType button_lowlite = (pressed ? PF_App_Color_HILITE : PF_App_Color_BLACK);
	
	
	bot.SetColor(button_color);
	bot.PaintRect(width, BUTTON_HEIGHT);
	
	float brush_size = (pressed ? 1.f : 0.5f);
	
	bot.SetColor(button_hilite);
	bot.MoveTo(original.x + 1, original.y + BUTTON_HEIGHT - 1);
	
	bot.DrawLineTo(original.x + 1, original.y + 1, brush_size);
	bot.DrawLineTo(original.x + width - 1, original.y + 1, brush_size);
	
	bot.MoveTo(original); // annoying corner pixel
	bot.SetColor(button_hilite, 0.3f);
	bot.PaintRect(1, 1);


	brush_size = (pressed ? 0.5f : 1.f);
	
	bot.SetColor(button_lowlite);
	bot.MoveTo(original.x + 1, original.y + BUTTON_HEIGHT - 1);
	
	bot.DrawLineTo(original.x + width - 1, original.y + BUTTON_HEIGHT - 1, brush_size);
	bot.DrawLineTo(original.x + width - 1, original.y + 2, brush_size);
	
	bot.MoveTo(original.x + width - 1, original.y + BUTTON_HEIGHT - 1); // corner
	bot.SetColor(button_lowlite, 0.3f);
	bot.PaintRect(1, 1);
	
	
	bot.MoveTo(original.x + (width / 2), original.y + text_height + BUTTON_TEXT_INDENT_V);
	
	if(pressed)
		bot.Move(1, 1);
	
	bot.SetColor(TEXT_COLOR);
	bot.DrawString(label, kDRAWBOT_TextAlignment_Center);
	
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
	

	if (!(event_extra->evt_in_flags & PF_EI_DONT_DRAW) && params[OCIO_DATA]->u.arb_d.value != NULL)
	{
		if(event_extra->effect_win.area == PF_EA_CONTROL)
		{
			ArbitraryData *arb_data = (ArbitraryData *)PF_LOCK_HANDLE(params[OCIO_DATA]->u.arb_d.value);
			SequenceData *seq_data = (SequenceData *)PF_LOCK_HANDLE(in_data->sequence_data);
		
			DrawbotBot bot(in_data->pica_basicP, event_extra->contextH);
			
			
			int panel_top = event_extra->effect_win.current_frame.top;
			int panel_left = event_extra->effect_win.current_frame.left;
			int panel_width = event_extra->effect_win.current_frame.right;
			float text_height = bot.FontSize();
			
			bool have_file = (arb_data->path[0] != '\0');
			
			// path text field
			bot.MoveTo(panel_left + LEFT_MARGIN + FILE_LABEL_WIDTH, panel_top + TOP_MARGIN);
			
			DRAWBOT_PointF32 field_corner = bot.Pos();
			
			int field_width = panel_width - panel_left - LEFT_MARGIN - RIGHT_MARGIN;
			
			bot.SetColor(PF_App_Color_SHADOW);
			bot.PaintRect(field_width, FIELD_HEIGHT);
			bot.SetColor(PF_App_Color_BLACK);
			bot.DrawRect(field_width, FIELD_HEIGHT);

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
								field_width - (2 * FIELD_TEXT_INDENT_H));
			}
			else
			{
				bot.DrawString("(none)");
			}
			
			
			// buttons
			int field_bottom = panel_top + TOP_MARGIN + FIELD_HEIGHT;
			int buttons_top = field_bottom + BUTTONS_GAP_V;
			
			// Export button
			if(arb_data->type != OCIO_TYPE_NONE)
			{
				bot.MoveTo(panel_left + BUTTONS_INDENT_H + (2 * (BUTTON_WIDTH + BUTTONS_GAP_H)), buttons_top);
				
				DrawButton(bot, "Export...", BUTTON_WIDTH, false);
			}
			
			if(arb_data->type == OCIO_TYPE_LUT)
			{
				// Invert button
				bot.MoveTo(panel_left + BUTTONS_INDENT_H, buttons_top);
				
				DrawButton(bot, "Invert", BUTTON_WIDTH, arb_data->invert);
			}
			else if(arb_data->type == OCIO_TYPE_CONVERT || arb_data->type == OCIO_TYPE_DISPLAY)
			{
				// Convert/Display buttons
				bot.MoveTo(panel_left + BUTTONS_INDENT_H, buttons_top);
				
				DrawButton(bot, "Convert", BUTTON_WIDTH, arb_data->type == OCIO_TYPE_CONVERT);
				
				bot.Move(BUTTON_WIDTH + BUTTONS_GAP_H);
				
				DrawButton(bot, "Display", BUTTON_WIDTH, arb_data->type == OCIO_TYPE_DISPLAY);
				
				
				// menus
				int buttons_bottom = buttons_top + BUTTON_HEIGHT;
				
				bot.MoveTo(panel_left + MENUS_INDENT_H, buttons_bottom + MENUS_GAP_V);
				
				if(arb_data->type == OCIO_TYPE_CONVERT)
				{
					DrawMenu(bot, "Input Space:", arb_data->input);
					
					bot.Move(0, MENU_HEIGHT + MENU_SPACE_V);
					
					DrawMenu(bot, "Output Space:", arb_data->output);
				}
				else if(arb_data->type == OCIO_TYPE_DISPLAY)
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


static int
FindInVec(const vector<string> &vec, const string val)
{
	for(int i=0; i < vec.size(); i++)
	{
		if(vec[i] == val)
			return i;
	}
	
	return -1;
}


static void
DoClickPath(
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output,
	PF_EventExtra	*event_extra,
	ArbitraryData	*arb_data,
	SequenceData	*seq_data,
	UIRegion		reg )
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
			
			
			// try to retain settings if it looks like the same situation,
			// possibly fixing a moved path
			if(	OCIO_TYPE_LUT == new_context->getType() ||
				arb_data->type != new_context->getType() ||
				-1 == FindInVec(new_context->getInputs(), arb_data->input) ||
				-1 == FindInVec(new_context->getInputs(), arb_data->output) ||
				-1 == FindInVec(new_context->getTransforms(), arb_data->transform) ||
				-1 == FindInVec(new_context->getDevices(), arb_data->device) )
			{
				// Configuration is different, so initialize defaults
				arb_data->type = seq_data->context->getType();
				
				if(arb_data->type != OCIO_TYPE_LUT)
				{
					strncpy(arb_data->input, seq_data->context->getInput().c_str(), ARB_SPACE_LEN);
					strncpy(arb_data->output, seq_data->context->getOutput().c_str(), ARB_SPACE_LEN);
					strncpy(arb_data->transform, seq_data->context->getTransform().c_str(), ARB_SPACE_LEN);
					strncpy(arb_data->device, seq_data->context->getDevice().c_str(), ARB_SPACE_LEN);
				}
			}
			else
			{
				// Configuration is the same, retain current settings
				if(arb_data->type == OCIO_TYPE_LUT)
				{
					if(arb_data->invert)
					{
						seq_data->context->setupLUT(arb_data->invert);
					}
				}
				else if(arb_data->type == OCIO_TYPE_CONVERT)
				{
					seq_data->context->setupConvert(arb_data->input, arb_data->output);
				}
				else if(arb_data->type == OCIO_TYPE_DISPLAY)
				{
					seq_data->context->setupDisplay(arb_data->input, arb_data->transform, arb_data->device);
				}
			}
			
			params[OCIO_DATA]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;
		}
		catch(std::exception &e)
		{
			PF_SPRINTF(out_data->return_msg, e.what());
			
			out_data->out_flags |= PF_OutFlag_DISPLAY_ERROR_MESSAGE;
		}
		catch(...)
		{
			PF_SPRINTF(out_data->return_msg, "OCIO Error reading file.");
			
			out_data->out_flags |= PF_OutFlag_DISPLAY_ERROR_MESSAGE;
		}
	}
}

static void
DoClickConvertDisplay(
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output,
	PF_EventExtra	*event_extra,
	ArbitraryData	*arb_data,
	SequenceData	*seq_data,
	UIRegion		reg )
{
	if(arb_data->type == OCIO_TYPE_LUT)
	{
		if(reg == REGION_CONVERT_BUTTON) // i.e. Invert
		{
			try
			{
				seq_data->context->setupLUT(!arb_data->invert);
				
				arb_data->invert = !arb_data->invert;
			
				params[OCIO_DATA]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;
			}
			catch(std::exception &e)
			{
				PF_SPRINTF(out_data->return_msg, e.what());
				
				out_data->out_flags |= PF_OutFlag_DISPLAY_ERROR_MESSAGE;
			}
			catch(...)
			{
				PF_SPRINTF(out_data->return_msg, "LUT can not be inverted.");
				
				out_data->out_flags |= PF_OutFlag_DISPLAY_ERROR_MESSAGE;
			}
		}
	}
	else if(arb_data->type == OCIO_TYPE_CONVERT || arb_data->type == OCIO_TYPE_DISPLAY)
	{
		if(reg == REGION_CONVERT_BUTTON && arb_data->type != OCIO_TYPE_CONVERT)
		{
			arb_data->type = OCIO_TYPE_CONVERT;
			
			seq_data->context->setupConvert(arb_data->input, arb_data->output);
		
			params[OCIO_DATA]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;
		}
		else if(reg == REGION_DISPLAY_BUTTON && arb_data->type != OCIO_TYPE_DISPLAY)
		{
			arb_data->type = OCIO_TYPE_DISPLAY;
			
			seq_data->context->setupDisplay(arb_data->input, arb_data->transform, arb_data->device);
			
			params[OCIO_DATA]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;
		}
	}
}


static void
DoClickExport(
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output,
	PF_EventExtra	*event_extra,
	ArbitraryData	*arb_data,
	SequenceData	*seq_data,
	UIRegion		reg )
{
	ExtensionMap extensions;
	
	for(int i=0; i < OCIO::Baker::getNumFormats(); ++i)
	{
		const char *extension = OCIO::Baker::getFormatExtensionByIndex(i);
		const char *format = OCIO::Baker::getFormatNameByIndex(i);
		
		extensions[ extension ] = format;
	}
	
	extensions[ "icc" ] = "ICC Profile";
	
	
	char path[256];
	
	bool result = SaveFile(path, 255, extensions, NULL);
	
	
	if(result)
	{
		string the_path(path);
		string the_extension = the_path.substr( the_path.find_last_of('.') + 1 );
		
		string monitor_icc_path;
		
		if(the_extension == "icc")
		{
			// TODO: monitor ICC dialog
		}
		
		try
		{
			seq_data->context->ExportLUT(the_path, monitor_icc_path);
		}
		catch(std::exception &e)
		{
			PF_SPRINTF(out_data->return_msg, e.what());
			
			out_data->out_flags |= PF_OutFlag_DISPLAY_ERROR_MESSAGE;
		}
		catch(...)
		{
			PF_SPRINTF(out_data->return_msg, "Export failed.");
			
			out_data->out_flags |= PF_OutFlag_DISPLAY_ERROR_MESSAGE;
		}
	}
}


static void
DoClickMenus(
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output,
	PF_EventExtra	*event_extra,
	ArbitraryData	*arb_data,
	SequenceData	*seq_data,
	UIRegion		reg )
{
	if(seq_data->context != NULL && arb_data->type == seq_data->context->getType())
	{
		MenuVec menu_items;
		int selected_item;
		
		if(arb_data->type == OCIO_TYPE_CONVERT)
		{
			menu_items = seq_data->context->getInputs();
			
			if(reg == REGION_MENU1)
			{
				selected_item = FindInVec(menu_items, arb_data->input);
			}
			else
			{
				selected_item = FindInVec(menu_items, arb_data->output);
			}
		}
		else if(arb_data->type == OCIO_TYPE_DISPLAY)
		{
			if(reg == REGION_MENU1)
			{
				menu_items = seq_data->context->getInputs();
				
				selected_item = FindInVec(menu_items, arb_data->input);
			}
			else if(reg == REGION_MENU2)
			{
				menu_items = seq_data->context->getTransforms();
				
				selected_item = FindInVec(menu_items, arb_data->transform);
			}
			else if(reg == REGION_MENU3)
			{
				menu_items = seq_data->context->getDevices();
				
				selected_item = FindInVec(menu_items, arb_data->device);
			}
		}
		
		
		if(selected_item < 0)
			selected_item = 0;
		
		
		int result = PopUpMenu(menu_items, selected_item);
		
		
		if(result != selected_item)
		{
			string color_space = menu_items[ result ];
			
			if(arb_data->type == OCIO_TYPE_CONVERT)
			{
				if(reg == REGION_MENU1)
				{
					strncpy(arb_data->input, color_space.c_str(), ARB_SPACE_LEN);
				}
				else if(reg == REGION_MENU2)
				{
					strncpy(arb_data->output, color_space.c_str(), ARB_SPACE_LEN);
				}
				
				seq_data->context->setupConvert(arb_data->input, arb_data->output);
				
				params[OCIO_DATA]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;
			}
			else if(arb_data->type == OCIO_TYPE_DISPLAY)
			{
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
				
				seq_data->context->setupDisplay(arb_data->input, arb_data->transform, arb_data->device);
				
				params[OCIO_DATA]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;
			}
		}
	}
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
	
	
	if(event_extra->effect_win.area == PF_EA_CONTROL)
	{
		bool menus_visible = (arb_data->type == OCIO_TYPE_CONVERT || arb_data->type == OCIO_TYPE_DISPLAY);
		bool third_menu = (arb_data->type == OCIO_TYPE_DISPLAY);
		
		PF_Point local_point;
		
		local_point.h = event_extra->u.do_click.screen_point.h - event_extra->effect_win.current_frame.left;
		local_point.v = event_extra->u.do_click.screen_point.v - event_extra->effect_win.current_frame.top;
		
		UIRegion reg = WhichRegion(local_point, menus_visible, third_menu);
		
		if(reg != REGION_NONE)
		{
			try
			{
				if(reg == REGION_PATH)
				{
					DoClickPath(in_data, out_data, params, output, event_extra, arb_data, seq_data, reg);
				}
				else if(arb_data->type != OCIO_TYPE_NONE)
				{
					if(seq_data->context == NULL)
					{
						seq_data->context = new OpenColorIO_AE_Context(arb_data);
					}
						
					if(reg == REGION_CONVERT_BUTTON || reg == REGION_DISPLAY_BUTTON)
					{
						DoClickConvertDisplay(in_data, out_data, params, output, event_extra, arb_data, seq_data, reg);
					}
					else if(reg == REGION_EXPORT_BUTTON)
					{
						DoClickExport(in_data, out_data, params, output, event_extra, arb_data, seq_data, reg);
					}
					else // must be a menu then
					{
						DoClickMenus(in_data, out_data, params, output, event_extra, arb_data, seq_data, reg);
					}
				}
			}
			catch(std::exception &e)
			{
				PF_SPRINTF(out_data->return_msg, e.what());
				
				out_data->out_flags |= PF_OutFlag_DISPLAY_ERROR_MESSAGE;
			}
			catch(...) { }
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
	PF_Err		err		= PF_Err_NONE;
	
	if(event_extra->effect_win.area == PF_EA_CONTROL)
	{
		PF_Point local_point;
		
		local_point.h = event_extra->u.adjust_cursor.screen_point.h - event_extra->effect_win.current_frame.left;
		local_point.v = event_extra->u.adjust_cursor.screen_point.v - event_extra->effect_win.current_frame.top;

		UIRegion reg = WhichRegion(local_point, false, false);
		
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
	}
	
	event_extra->evt_out_flags = PF_EO_HANDLED_EVENT;
	
	return err;
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
	
	extra->evt_out_flags = 0;
	
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
