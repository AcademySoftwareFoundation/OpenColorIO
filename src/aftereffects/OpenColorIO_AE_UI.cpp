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


#include "OpenColorIO_AE.h"

#include "OpenColorIO_AE_Context.h"
#include "OpenColorIO_AE_Dialogs.h"

#include "DrawbotBot.h"



// UI drawing constants

#define LEFT_MARGIN         5
#define TOP_MARGIN          5
#define RIGHT_MARGIN        50

#define FILE_LABEL_WIDTH    35

#define FIELD_HEIGHT        22

#define FIELD_TEXT_INDENT_H 10
#define FIELD_TEXT_INDENT_V 4

#define BUTTONS_INDENT_H    (LEFT_MARGIN + 70)

#define BUTTONS_GAP_V       20
#define BUTTONS_GAP_H       30

#define BUTTON_HEIGHT       20
#define BUTTON_WIDTH        80

#define BUTTON_TEXT_INDENT_V 2

#define MENUS_INDENT_H      0

#define MENUS_GAP_V         20

#define MENU_LABEL_WIDTH    100
#define MENU_LABEL_SPACE    5

#define MENU_WIDTH          150
#define MENU_HEIGHT         20

#define MENU_TEXT_INDENT_H  10
#define MENU_TEXT_INDENT_V  2

#define MENU_ARROW_WIDTH    14
#define MENU_ARROW_HEIGHT   7

#define MENU_ARROW_SPACE_H  8
#define MENU_ARROW_SPACE_V  7

#define MENU_SHADOW_OFFSET  3

#define MENU_SPACE_V        20

#define TEXT_COLOR      PF_App_Color_TEXT_DISABLED



typedef enum {
    REGION_NONE=0,
    REGION_CONFIG_MENU,
    REGION_PATH,
    REGION_CONVERT_BUTTON,
    REGION_DISPLAY_BUTTON,
    REGION_EXPORT_BUTTON,
    REGION_MENU1,
    REGION_MENU2,
    REGION_MENU3
} UIRegion;


static UIRegion WhichRegion(PF_Point ui_point, bool menus, bool third_menu)
{
    int field_top = TOP_MARGIN;
    int field_bottom = field_top + FIELD_HEIGHT;
    
    if(ui_point.v >= field_top && ui_point.v <= field_bottom)
    {
        int menu_left = LEFT_MARGIN + MENUS_INDENT_H + MENU_LABEL_WIDTH;
        int menu_right = menu_left + MENU_WIDTH;
        
        if(ui_point.h >= menu_left && ui_point.h <= menu_right)
        {
            return REGION_CONFIG_MENU;
        }
        else
        {
            int field_left = MENUS_INDENT_H + MENU_LABEL_WIDTH + MENU_LABEL_SPACE +
                                MENU_WIDTH + FIELD_TEXT_INDENT_H;
            
            if(ui_point.h >= field_left)
                return REGION_PATH;
        }
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


static void DrawMenu(DrawbotBot &bot, const char *label, const char *item)
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
                    MENU_WIDTH - MENU_TEXT_INDENT_H - MENU_TEXT_INDENT_H -
                        MENU_ARROW_WIDTH - MENU_ARROW_SPACE_H - MENU_ARROW_SPACE_H);
    
    bot.MoveTo(menu_corner.x + MENU_WIDTH - MENU_ARROW_SPACE_H - MENU_ARROW_WIDTH,
                menu_corner.y + MENU_ARROW_SPACE_V);
    
    bot.SetColor(PF_App_Color_LIGHT_TINGE);
    bot.PaintTriangle(MENU_ARROW_WIDTH, MENU_ARROW_HEIGHT);
    
    bot.MoveTo(original);
}


static void DrawButton(DrawbotBot &bot, const char *label, int width, bool pressed)
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
        bot.Move(2, 2);
    
    bot.SetColor(TEXT_COLOR);
    bot.DrawString(label, kDRAWBOT_TextAlignment_Center);
    
    bot.MoveTo(original);
}


static PF_Err DrawEvent(  
    PF_InData       *in_data,
    PF_OutData      *out_data,
    PF_ParamDef     *params[],
    PF_LayerDef     *output,
    PF_EventExtra   *event_extra)
{
    PF_Err          err     =   PF_Err_NONE;
    

    if(!(event_extra->evt_in_flags & PF_EI_DONT_DRAW) &&
        params[OCIO_DATA]->u.arb_d.value != NULL)
    {
        if(event_extra->effect_win.area == PF_EA_CONTROL)
        {
            ArbitraryData *arb_data = (ArbitraryData *)PF_LOCK_HANDLE(params[OCIO_DATA]->u.arb_d.value);
            SequenceData *seq_data = (SequenceData *)PF_LOCK_HANDLE(in_data->sequence_data);
        
            DrawbotBot bot(in_data->pica_basicP, event_extra->contextH, in_data->appl_id);
            
            
            int panel_left = event_extra->effect_win.current_frame.left;
            int panel_top = event_extra->effect_win.current_frame.top;
            int panel_width = event_extra->effect_win.current_frame.right;
            int panel_height = event_extra->effect_win.current_frame.bottom;
            float text_height = bot.FontSize();
            
            if(in_data->appl_id != 'FXTC')
            {
                // for Premiere we need to paint the background first
                bot.SetColor(PF_App_Color_PANEL_BACKGROUND);
                bot.MoveTo(panel_left, panel_top);
                bot.PaintRect(panel_width, panel_height);
            }
            
            
            // configuration menu
            bot.MoveTo(panel_left + MENUS_INDENT_H, panel_top + TOP_MARGIN);
            
            std::string config_menu_text = arb_data->path;
            
            if(arb_data->source == OCIO_SOURCE_NONE)
            {
                config_menu_text = "(none)";
            }
            if(arb_data->source == OCIO_SOURCE_ENVIRONMENT)
            {
                config_menu_text = "$OCIO";
            }
            else if(arb_data->source == OCIO_SOURCE_CUSTOM)
            {
                config_menu_text = (arb_data->action == OCIO_ACTION_LUT ? "LUT" : "Custom");
            }
            
            DrawMenu(bot, "Configuration:", config_menu_text.c_str());
            
            
            if(arb_data->source == OCIO_SOURCE_CUSTOM ||
                arb_data->source == OCIO_SOURCE_ENVIRONMENT)
            {
                // path text field
                int field_left = panel_left + MENUS_INDENT_H + MENU_LABEL_WIDTH +
                                    MENU_LABEL_SPACE + MENU_WIDTH + FIELD_TEXT_INDENT_H;
                
                bot.MoveTo(field_left, panel_top + TOP_MARGIN);
                
                int field_width = MAX(panel_width - field_left + panel_left - RIGHT_MARGIN, 60);
                
                bot.SetColor(PF_App_Color_SHADOW);
                bot.PaintRect(field_width, FIELD_HEIGHT);
                bot.SetColor(PF_App_Color_BLACK);
                bot.DrawRect(field_width, FIELD_HEIGHT);

                bot.Move(FIELD_TEXT_INDENT_H, FIELD_TEXT_INDENT_V + text_height);
                
                bot.SetColor(TEXT_COLOR);
                
                std::string file_string = "(none)";
                
                if(arb_data->source == OCIO_SOURCE_ENVIRONMENT)
                {
                    char *file = std::getenv("OCIO");
                    
                    if(file)
                        file_string = file;
                }
                else
                {
                    file_string = (seq_data->status == STATUS_USING_RELATIVE ?
                                        arb_data->relative_path : arb_data->path);
                }
                
                                    
                bot.DrawString(file_string.c_str(),
                                kDRAWBOT_TextAlignment_Default,
                                kDRAWBOT_TextTruncation_PathEllipsis,
                                field_width - (2 * FIELD_TEXT_INDENT_H));
            }
            
            
            if(seq_data->status == STATUS_FILE_MISSING)
            {
                bot.MoveTo(panel_left + MENU_LABEL_WIDTH + MENU_LABEL_SPACE,
                            panel_top + MENU_HEIGHT + BUTTONS_GAP_V + BUTTON_HEIGHT + BUTTONS_GAP_V);
                
                bot.SetColor(PF_App_Color_RED);
                bot.PaintRect(200, 50);
                
                bot.Move(100, 25 + (bot.FontSize() / 2));
                bot.SetColor(PF_App_Color_WHITE);
                
                if(arb_data->source == OCIO_SOURCE_ENVIRONMENT)
                    bot.DrawString("$OCIO NOT SET", kDRAWBOT_TextAlignment_Center);
                else
                    bot.DrawString("FILE MISSING", kDRAWBOT_TextAlignment_Center);
            }
            else
            {
                // buttons
                int field_bottom = panel_top + TOP_MARGIN + FIELD_HEIGHT;
                int buttons_top = field_bottom + BUTTONS_GAP_V;
                
                // GPU alert
                if(seq_data->gpu_err != GPU_ERR_NONE)
                {
                    bot.MoveTo(panel_left + MENU_LABEL_WIDTH + MENU_LABEL_SPACE,
                                field_bottom + bot.FontSize() + BUTTON_TEXT_INDENT_V);
                    
                    if(seq_data->gpu_err == GPU_ERR_INSUFFICIENT)
                    {
                        bot.DrawString("GPU Insufficient");
                    }
                    else if(seq_data->gpu_err == GPU_ERR_RENDER_ERR)
                    {
                        bot.DrawString("GPU Render Error");
                    }
                }
                
            #ifndef NDEBUG
                // Premiere color space (only for debugging purposes)
                if(in_data->appl_id == 'PrMr' && seq_data->prem_status != PREMIERE_UNKNOWN)
                {
                    bot.MoveTo(panel_left + MENU_LABEL_WIDTH + MENU_LABEL_SPACE + 200,
                                field_bottom + bot.FontSize() + BUTTON_TEXT_INDENT_V);
                    
                    bot.SetColor(PF_App_Color_WHITE);
                    
                    if(seq_data->prem_status == PREMIERE_LINEAR)
                    {
                        bot.DrawString("Linear Float");
                    }
                    else if(seq_data->prem_status == PREMIERE_NON_LINEAR)
                    {
                        bot.DrawString("Non-Linear Float");
                    }
                }
            #endif
                
                // Export button
                if(arb_data->action != OCIO_ACTION_NONE)
                {
                    bot.MoveTo(panel_left + BUTTONS_INDENT_H + (2 * (BUTTON_WIDTH + BUTTONS_GAP_H)), buttons_top);
                    
                    DrawButton(bot, "Export...", BUTTON_WIDTH, false);
                }
                
                if(arb_data->action == OCIO_ACTION_LUT)
                {
                    // Invert button
                    bot.MoveTo(panel_left + BUTTONS_INDENT_H, buttons_top);
                    
                    DrawButton(bot, "Invert", BUTTON_WIDTH, arb_data->invert);
                    
                    // interpolation menu
                    int buttons_bottom = buttons_top + BUTTON_HEIGHT;
                    
                    bot.MoveTo(panel_left + MENUS_INDENT_H, buttons_bottom + MENUS_GAP_V);
                    
                    const char *txt =   arb_data->interpolation == OCIO_INTERP_NEAREST ? "Nearest Neighbor" :
                                        arb_data->interpolation == OCIO_INTERP_LINEAR ? "Linear" :
                                        arb_data->interpolation == OCIO_INTERP_TETRAHEDRAL ? "Tetrahedral" :
                                        arb_data->interpolation == OCIO_INTERP_BEST ? "Best" :
                                        "Unknown";
                    
                    DrawMenu(bot, "Interpolation:", txt);
                }
                else if(arb_data->action == OCIO_ACTION_CONVERT ||
                        arb_data->action == OCIO_ACTION_DISPLAY)
                {
                    // Convert/Display buttons
                    bot.MoveTo(panel_left + BUTTONS_INDENT_H, buttons_top);
                    
                    DrawButton(bot, "Convert", BUTTON_WIDTH,
                                arb_data->action == OCIO_ACTION_CONVERT);
                    
                    bot.Move(BUTTON_WIDTH + BUTTONS_GAP_H);
                    
                    DrawButton(bot, "Display", BUTTON_WIDTH,
                                arb_data->action == OCIO_ACTION_DISPLAY);
                    
                    
                    // menus
                    int buttons_bottom = buttons_top + BUTTON_HEIGHT;
                    
                    bot.MoveTo(panel_left + MENUS_INDENT_H, buttons_bottom + MENUS_GAP_V);
                    
                    if(arb_data->action == OCIO_ACTION_CONVERT)
                    {
                        DrawMenu(bot, "Input Space:", arb_data->input);
                        
                        bot.Move(0, MENU_HEIGHT + MENU_SPACE_V);
                        
                        DrawMenu(bot, "Output Space:", arb_data->output);
                    }
                    else if(arb_data->action == OCIO_ACTION_DISPLAY)
                    {
                        // color space transformations
                        DrawMenu(bot, "Input Space:", arb_data->input);
                        
                        bot.Move(0, MENU_HEIGHT + MENU_SPACE_V);
                        
                        DrawMenu(bot, "Device:", arb_data->device);
                        
                        bot.Move(0, MENU_HEIGHT + MENU_SPACE_V);
                        
                        DrawMenu(bot, "Transform:", arb_data->transform);
                    }
                }
            }
            
            
            event_extra->evt_out_flags = PF_EO_HANDLED_EVENT;

            PF_UNLOCK_HANDLE(params[OCIO_DATA]->u.arb_d.value);
            PF_UNLOCK_HANDLE(in_data->sequence_data);
        }
    }

    return err;
}


std::string GetProjectDir(PF_InData *in_data)
{
    if(in_data->appl_id == 'PrMr')
        return std::string("");
    
    AEGP_SuiteHandler suites(in_data->pica_basicP);

    AEGP_ProjectH projH = NULL;
    suites.ProjSuite5()->AEGP_GetProjectByIndex(0, &projH);

    AEGP_MemHandle pathH = NULL;
    suites.ProjSuite5()->AEGP_GetProjectPath(projH, &pathH);

    if(pathH)
    {
        std::string proj_dir;
        
        A_UTF16Char *path = NULL;
        suites.MemorySuite1()->AEGP_LockMemHandle(pathH, (void **)&path);

        if(path)
        {
            // poor-man's unicode copy
            char c_path[AEGP_MAX_PATH_SIZE];

            char *c = c_path;
            A_UTF16Char *s = path;

            do{
                *c++ = *s;
            }while(*s++ != '\0');

#ifdef WIN_ENV
#define PATH_DELIMITER  '\\'
#else
#define PATH_DELIMITER  '/'
#endif

            std::string proj_path(c_path);
            
            if(proj_path.find_last_of(PATH_DELIMITER) != std::string::npos)
            {
                proj_dir = proj_path.substr(0, proj_path.find_last_of(PATH_DELIMITER));
            }
        }
        
        suites.MemorySuite1()->AEGP_FreeMemHandle(pathH);
        
        return proj_dir;
    }

    return std::string("");
}


static int FindInVec(const std::vector<std::string> &vec, const std::string val)
{
    for(int i=0; i < vec.size(); i++)
    {
        if(vec[i] == val)
            return i;
    }
    
    return -1;
}


static void DoClickPath(
    PF_InData       *in_data,
    PF_OutData      *out_data,
    PF_ParamDef     *params[],
    PF_LayerDef     *output,
    PF_EventExtra   *event_extra,
    ArbitraryData   *arb_data,
    SequenceData    *seq_data)
{
    ExtensionMap extensions;
    
    for(int i=0; i < OCIO::FileTransform::getNumFormats(); i++)
    {
        const char *extension = OCIO::FileTransform::getFormatExtensionByIndex(i);
        const char *format = OCIO::FileTransform::getFormatNameByIndex(i);
    
        if(extension != std::string("ccc")) // .ccc files require an ID parameter
            extensions[ extension ] = format;
    }
    
    extensions[ "ocio" ] = "OCIO Format";
    

    void *hwndOwner = NULL;

#ifdef WIN_ENV
    PF_GET_PLATFORM_DATA(PF_PlatData_MAIN_WND, &hwndOwner);
#endif

    char c_path[ARB_PATH_LEN + 1] = { '\0' };
    
    bool result = OpenFile(c_path, ARB_PATH_LEN, extensions, hwndOwner);
    
    
    if(result)
    {
        Path path(c_path, GetProjectDir(in_data));
        
        OpenColorIO_AE_Context *new_context = new OpenColorIO_AE_Context(path.full_path(),
                                                                    OCIO_SOURCE_CUSTOM);
        
        if(new_context == NULL)
            throw OCIO::Exception("WTF?");
            
        
        if(seq_data->context)
        {
            delete seq_data->context;
        }
        
        seq_data->context = new_context;
        
        arb_data->source = seq_data->source = OCIO_SOURCE_CUSTOM;
        
        strncpy(arb_data->path, path.full_path().c_str(), ARB_PATH_LEN);
        strncpy(arb_data->relative_path, path.relative_path(false).c_str(), ARB_PATH_LEN);
        
        strncpy(seq_data->path, arb_data->path, ARB_PATH_LEN);
        strncpy(seq_data->relative_path, arb_data->relative_path, ARB_PATH_LEN);
        
        
        // try to retain settings if it looks like the same situation,
        // possibly fixing a moved path
        if(OCIO_ACTION_NONE == arb_data->action ||
            (OCIO_ACTION_LUT == new_context->getAction() && OCIO_ACTION_LUT != arb_data->action) ||
            (OCIO_ACTION_LUT != new_context->getAction() && OCIO_ACTION_LUT == arb_data->action) ||
            (OCIO_ACTION_LUT != new_context->getAction() &&
               (-1 == FindInVec(new_context->getInputs(), arb_data->input) ||
                -1 == FindInVec(new_context->getInputs(), arb_data->output) ||
                -1 == FindInVec(new_context->getTransforms(), arb_data->transform) ||
                -1 == FindInVec(new_context->getDevices(), arb_data->device) ) ) )
        {
            // Configuration is different, so initialize defaults
            arb_data->action = seq_data->context->getAction();
            
            if(arb_data->action == OCIO_ACTION_LUT)
            {
                arb_data->invert = FALSE;
                arb_data->interpolation = OCIO_INTERP_LINEAR;
            }
            else
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
            if(arb_data->action == OCIO_ACTION_LUT)
            {
                seq_data->context->setupLUT(arb_data->invert, arb_data->interpolation);
            }
            else if(arb_data->action == OCIO_ACTION_CONVERT)
            {
                seq_data->context->setupConvert(arb_data->input, arb_data->output);
            }
            else if(arb_data->action == OCIO_ACTION_DISPLAY)
            {
                seq_data->context->setupDisplay(arb_data->input, arb_data->device, arb_data->transform);
                
                // transform may have changed
                strncpy(arb_data->transform, seq_data->context->getTransform().c_str(), ARB_SPACE_LEN);
            }
        }
        
        params[OCIO_DATA]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;
        
        seq_data->status = STATUS_USING_ABSOLUTE;
    }
}


static void DoClickConfig(
    PF_InData       *in_data,
    PF_OutData      *out_data,
    PF_ParamDef     *params[],
    PF_LayerDef     *output,
    PF_EventExtra   *event_extra,
    ArbitraryData   *arb_data,
    SequenceData    *seq_data)
{
    void *hwndOwner = NULL;

#ifdef WIN_ENV
    PF_GET_PLATFORM_DATA(PF_PlatData_MAIN_WND, &hwndOwner);
#endif

    ConfigVec configs;
    GetStdConfigs(configs);
    
    if(configs.size() == 0)
        configs.push_back("(nada)"); // menu makes a gray entry that says "No configs in $PATH"
    
    
    MenuVec menu_items;
    int selected = 0;
    
    menu_items.push_back("$OCIO"); // menu will gray this out if $OCIO is not defined
    
    menu_items.push_back("(-"); // this tells the menu to make a seperator
    
    for(ConfigVec::const_iterator i = configs.begin(); i != configs.end(); ++i)
    {
        menu_items.push_back( *i );
        
        if(arb_data->source == OCIO_SOURCE_STANDARD && arb_data->path == *i)
        {
            selected = menu_items.size() - 1;
        }
    }
    
    menu_items.push_back("(-");
    
    menu_items.push_back("Custom...");
    
    int custom_choice = menu_items.size() - 1;
    
    if(arb_data->source == OCIO_SOURCE_CUSTOM)
    {
        selected = -1;
    }
    
    
    int choice = PopUpMenu(menu_items, selected, hwndOwner);
    
    
    if(choice == custom_choice)
    {
        // custom is the same as clicking the path
        DoClickPath(in_data, out_data, params, output, event_extra, arb_data, seq_data);
    }
    else if(choice != selected)
    {
        OpenColorIO_AE_Context *new_context = NULL;
        
        if(choice == 0)
        {
            // $OCIO
            char *file = std::getenv("OCIO");
            
            if(file)
            {
                Path path(file, GetProjectDir(in_data));
                
                new_context = new OpenColorIO_AE_Context(path.full_path(),
                                                            OCIO_SOURCE_ENVIRONMENT);
                
                arb_data->source = seq_data->source = OCIO_SOURCE_ENVIRONMENT;
                
                strncpy(arb_data->path, path.full_path().c_str(), ARB_PATH_LEN);
                strncpy(arb_data->relative_path, path.relative_path(false).c_str(), ARB_PATH_LEN);
            }
            else
                throw OCIO::Exception("No $OCIO environment variable defined."); 
        }
        else
        {
            // standard configs
            std::string config = configs[choice - 2];
            
            std::string path = GetStdConfigPath(config);
            
            if( !path.empty() )
            {
                new_context = new OpenColorIO_AE_Context(config, OCIO_SOURCE_STANDARD);
                
                arb_data->source = seq_data->source = OCIO_SOURCE_STANDARD;
                
                strncpy(arb_data->path, config.c_str(), ARB_PATH_LEN);
                strncpy(arb_data->relative_path, path.c_str(), ARB_PATH_LEN);
            }
            else
                throw OCIO::Exception("Problem loading OCIO configuration."); 
        }
        
        
        if(new_context)
        {
            if(seq_data->context)
            {
                delete seq_data->context;
            }
            
            seq_data->context = new_context;
            
            
            strncpy(seq_data->path, arb_data->path, ARB_PATH_LEN);
            strncpy(seq_data->relative_path, arb_data->relative_path, ARB_PATH_LEN);
            
            // try to retain settings if it looks like the same situation,
            // possibly fixing a moved path
            if(OCIO_ACTION_NONE == arb_data->action ||
                (OCIO_ACTION_LUT == new_context->getAction() && OCIO_ACTION_LUT != arb_data->action) ||
                (OCIO_ACTION_LUT != new_context->getAction() && OCIO_ACTION_LUT == arb_data->action) ||
                (OCIO_ACTION_LUT != new_context->getAction() &&
                   (-1 == FindInVec(new_context->getInputs(), arb_data->input) ||
                    -1 == FindInVec(new_context->getInputs(), arb_data->output) ||
                    -1 == FindInVec(new_context->getTransforms(), arb_data->transform) ||
                    -1 == FindInVec(new_context->getDevices(), arb_data->device) ) ) )
            {
                // Configuration is different, so initialize defaults
                arb_data->action = seq_data->context->getAction();
                
                if(arb_data->action == OCIO_ACTION_LUT)
                {
                    arb_data->invert = FALSE;
                    arb_data->interpolation = OCIO_INTERP_LINEAR;
                }
                else
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
                if(arb_data->action == OCIO_ACTION_LUT)
                {
                    seq_data->context->setupLUT(arb_data->invert, arb_data->interpolation);
                }
                else if(arb_data->action == OCIO_ACTION_CONVERT)
                {
                    seq_data->context->setupConvert(arb_data->input, arb_data->output);
                }
                else if(arb_data->action == OCIO_ACTION_DISPLAY)
                {
                    seq_data->context->setupDisplay(arb_data->input, arb_data->device, arb_data->transform);
                    
                    // transform may have changed
                    strncpy(arb_data->transform, seq_data->context->getTransform().c_str(), ARB_SPACE_LEN);
                }
            }
            
            params[OCIO_DATA]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;
            
            seq_data->status = STATUS_OK;
        }
    }
}


static void DoClickConvertDisplay(
    PF_InData       *in_data,
    PF_OutData      *out_data,
    PF_ParamDef     *params[],
    PF_LayerDef     *output,
    PF_EventExtra   *event_extra,
    ArbitraryData   *arb_data,
    SequenceData    *seq_data,
    UIRegion        reg )
{
    if(arb_data->action == OCIO_ACTION_LUT)
    {
        if(reg == REGION_CONVERT_BUTTON) // i.e. Invert
        {
            // doing it this way so that any exceptions thrown by setupLUT
            // because the LUT can't be inverted are thrown before
            // I actually chenge the ArbData setting
            seq_data->context->setupLUT(!arb_data->invert, arb_data->interpolation);
            
            arb_data->invert = !arb_data->invert;
        
            params[OCIO_DATA]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;
        }
    }
    else if(arb_data->action == OCIO_ACTION_CONVERT || arb_data->action == OCIO_ACTION_DISPLAY)
    {
        if(reg == REGION_CONVERT_BUTTON && arb_data->action != OCIO_ACTION_CONVERT)
        {
            arb_data->action = OCIO_ACTION_CONVERT;
            
            seq_data->context->setupConvert(arb_data->input, arb_data->output);
        
            params[OCIO_DATA]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;
        }
        else if(reg == REGION_DISPLAY_BUTTON && arb_data->action != OCIO_ACTION_DISPLAY)
        {
            arb_data->action = OCIO_ACTION_DISPLAY;
            
            seq_data->context->setupDisplay(arb_data->input, arb_data->device, arb_data->transform);
            
            // transform may have changed
            strncpy(arb_data->transform, seq_data->context->getTransform().c_str(), ARB_SPACE_LEN);
            
            params[OCIO_DATA]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;
        }
    }
}


static void DoClickExport(
    PF_InData       *in_data,
    PF_OutData      *out_data,
    PF_ParamDef     *params[],
    PF_LayerDef     *output,
    PF_EventExtra   *event_extra,
    ArbitraryData   *arb_data,
    SequenceData    *seq_data,
    UIRegion        reg )
{
    ExtensionMap extensions;
    
    for(int i=0; i < OCIO::Baker::getNumFormats(); ++i)
    {
        const char *extension = OCIO::Baker::getFormatExtensionByIndex(i);
        const char *format = OCIO::Baker::getFormatNameByIndex(i);
        
        extensions[ extension ] = format;
    }
    
    extensions[ "icc" ] = "ICC Profile";
    
    
    void *hwndOwner = NULL;

#ifdef WIN_ENV
    PF_GET_PLATFORM_DATA(PF_PlatData_MAIN_WND, &hwndOwner);
#endif

    char path[256] = { '\0' };
    
    bool result = SaveFile(path, 255, extensions, hwndOwner);
    
    
    if(result)
    {
        std::string the_path(path);
        std::string the_extension = the_path.substr( the_path.find_last_of('.') + 1 );
        
        bool do_export = true;
        
        std::string monitor_icc_path;
        
        if(the_extension == "icc")
        {
            char monitor_path[256] = {'\0'};
        
            do_export = GetMonitorProfile(monitor_path, 255, hwndOwner);
        
            if(monitor_path[0] != '\0')
                monitor_icc_path = monitor_path;
        }
        
        if(do_export)
            seq_data->context->ExportLUT(the_path, monitor_icc_path);
    }
}


static void DoClickMenus(
    PF_InData       *in_data,
    PF_OutData      *out_data,
    PF_ParamDef     *params[],
    PF_LayerDef     *output,
    PF_EventExtra   *event_extra,
    ArbitraryData   *arb_data,
    SequenceData    *seq_data,
    UIRegion        reg )
{
    if(seq_data->context != NULL && arb_data->action == seq_data->context->getAction())
    {
        if(arb_data->action == OCIO_ACTION_CONVERT ||
            (arb_data->action == OCIO_ACTION_DISPLAY && (reg == REGION_MENU1)))
        {
            // colorSpace menus
            std::string selected_item;
        
            if(reg == REGION_MENU1)
            {
                selected_item = arb_data->input;
            }
            else
            {
                selected_item = arb_data->output;
            }
            
            
            void *hwndOwner = NULL;

        #ifdef WIN_ENV
            PF_GET_PLATFORM_DATA(PF_PlatData_MAIN_WND, &hwndOwner);
        #endif
        
            const bool changed = ColorSpacePopUpMenu(seq_data->context->config(), selected_item, true, hwndOwner);
            
            
            if(changed)
            {
                if(reg == REGION_MENU1)
                {
                    strncpy(arb_data->input, selected_item.c_str(), ARB_SPACE_LEN);
                }
                else
                {
                    strncpy(arb_data->output, selected_item.c_str(), ARB_SPACE_LEN);
                }
                
                params[OCIO_DATA]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;
            }
        }
        else
        {
            // standard menus
            MenuVec menu_items;
            int selected_item;
            
            if(arb_data->action == OCIO_ACTION_LUT)
            {
                if(reg == REGION_MENU1)
                {
                    menu_items.push_back("Nearest Neighbor");
                    menu_items.push_back("Linear");
                    menu_items.push_back("Tetrahedral");
                    menu_items.push_back("(-");
                    menu_items.push_back("Best");
                    
                    selected_item = arb_data->interpolation == OCIO_INTERP_NEAREST ? 0 :
                                    arb_data->interpolation == OCIO_INTERP_LINEAR ? 1 :
                                    arb_data->interpolation == OCIO_INTERP_TETRAHEDRAL ? 2 :
                                    arb_data->interpolation == OCIO_INTERP_BEST ? 4 :
                                    -1;
                }
            }
            else if(arb_data->action == OCIO_ACTION_CONVERT)
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
            else if(arb_data->action == OCIO_ACTION_DISPLAY)
            {
                if(reg == REGION_MENU1)
                {
                    menu_items = seq_data->context->getInputs();
                    
                    selected_item = FindInVec(menu_items, arb_data->input);
                }
                else if(reg == REGION_MENU2)
                {
                    menu_items = seq_data->context->getDevices();
                    
                    selected_item = FindInVec(menu_items, arb_data->device);
                }
                else if(reg == REGION_MENU3)
                {
                    menu_items = seq_data->context->getTransforms();
                    
                    selected_item = FindInVec(menu_items, arb_data->transform);
                }
            }
            
            
            
            void *hwndOwner = NULL;

        #ifdef WIN_ENV
            PF_GET_PLATFORM_DATA(PF_PlatData_MAIN_WND, &hwndOwner);
        #endif

            int result = PopUpMenu(menu_items, selected_item, hwndOwner);
            
            
            if(result != selected_item)
            {
                std::string color_space = menu_items[ result ];
                
                if(arb_data->action == OCIO_ACTION_LUT)
                {
                    if(reg == REGION_MENU1)
                    {
                        arb_data->interpolation =   result == 0 ? OCIO_INTERP_NEAREST :
                                                    result == 2 ? OCIO_INTERP_TETRAHEDRAL :
                                                    result == 4 ? OCIO_INTERP_BEST :
                                                    OCIO_INTERP_LINEAR;
                                                
                        seq_data->context->setupLUT(arb_data->invert, arb_data->interpolation);
                    }
                }
                else if(arb_data->action == OCIO_ACTION_CONVERT)
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
                }
                else if(arb_data->action == OCIO_ACTION_DISPLAY)
                {
                    if(reg == REGION_MENU1)
                    {
                        strncpy(arb_data->input, color_space.c_str(), ARB_SPACE_LEN);
                    }
                    else if(reg == REGION_MENU2)
                    {
                        strncpy(arb_data->device, color_space.c_str(), ARB_SPACE_LEN);
                    }
                    else if(reg == REGION_MENU3)
                    {
                        strncpy(arb_data->transform, color_space.c_str(), ARB_SPACE_LEN);
                    }
                    
                    seq_data->context->setupDisplay(arb_data->input, arb_data->device, arb_data->transform);
                    
                    // transform may have changed
                    strncpy(arb_data->transform, seq_data->context->getTransform().c_str(), ARB_SPACE_LEN);
                }
                        
                params[OCIO_DATA]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;
            }
        }
    }
}


static PF_Err DoClick( 
    PF_InData       *in_data,
    PF_OutData      *out_data,
    PF_ParamDef     *params[],
    PF_LayerDef     *output,
    PF_EventExtra   *event_extra )
{
    PF_Err          err     =   PF_Err_NONE;
    
    ArbitraryData *arb_data = (ArbitraryData *)PF_LOCK_HANDLE(params[OCIO_DATA]->u.arb_d.value);
    SequenceData *seq_data = (SequenceData *)PF_LOCK_HANDLE(in_data->sequence_data);
    
    
    if(event_extra->effect_win.area == PF_EA_CONTROL)
    {
        bool menus_visible = (arb_data->action != OCIO_ACTION_NONE);
        bool third_menu = (arb_data->action == OCIO_ACTION_DISPLAY);
        
        PF_Point local_point;
        
        local_point.h = event_extra->u.do_click.screen_point.h - event_extra->effect_win.current_frame.left;
        local_point.v = event_extra->u.do_click.screen_point.v - event_extra->effect_win.current_frame.top;
        
        UIRegion reg = WhichRegion(local_point, menus_visible, third_menu);
        
        if(reg != REGION_NONE)
        {
            try
            {
                if(reg == REGION_CONFIG_MENU)
                {
                    DoClickConfig(in_data, out_data, params, output,
                                    event_extra, arb_data, seq_data);
                }
                else if(reg == REGION_PATH)
                {
                    if(arb_data->source == OCIO_SOURCE_CUSTOM)
                    {
                        DoClickPath(in_data, out_data, params, output,
                                        event_extra, arb_data, seq_data);
                    }
                }
                else if(arb_data->action != OCIO_ACTION_NONE &&
                        seq_data->status != STATUS_FILE_MISSING)
                {
                    if(seq_data->context == NULL)
                    {
                        seq_data->context = new OpenColorIO_AE_Context(arb_data,
                                                                GetProjectDir(in_data) );
                    }
                        
                    if(reg == REGION_CONVERT_BUTTON || reg == REGION_DISPLAY_BUTTON)
                    {
                        DoClickConvertDisplay(in_data, out_data, params, output,
                                                event_extra, arb_data, seq_data, reg);
                    }
                    else if(reg == REGION_EXPORT_BUTTON)
                    {
                        DoClickExport(in_data, out_data, params, output,
                                        event_extra, arb_data, seq_data, reg);
                    }
                    else // must be a menu then
                    {
                        DoClickMenus(in_data, out_data, params, output,
                                        event_extra, arb_data, seq_data, reg);
                    }
                }
            }
            catch(std::exception &e)
            {
                if(in_data->appl_id == 'FXTC')
                {
                    PF_SPRINTF(out_data->return_msg, e.what());
                    
                    out_data->out_flags |= PF_OutFlag_DISPLAY_ERROR_MESSAGE;
                }
                else
                {
                    void *hwndOwner = NULL;

                #ifdef WIN_ENV
                    PF_GET_PLATFORM_DATA(PF_PlatData_MAIN_WND, &hwndOwner);
                #endif

                    ErrorMessage(e.what(), hwndOwner);
                }
            }
            catch(...)
            {
                PF_SPRINTF(out_data->return_msg, "Unknown error");
                
                out_data->out_flags |= PF_OutFlag_DISPLAY_ERROR_MESSAGE;
            }
        }
    }
    
    
    PF_UNLOCK_HANDLE(params[OCIO_DATA]->u.arb_d.value);
    PF_UNLOCK_HANDLE(in_data->sequence_data);
    
    event_extra->evt_out_flags = PF_EO_HANDLED_EVENT;
    
    return err;
}


PF_Err HandleEvent ( 
    PF_InData       *in_data,
    PF_OutData      *out_data,
    PF_ParamDef     *params[],
    PF_LayerDef     *output,
    PF_EventExtra   *extra )
{
    PF_Err      err     = PF_Err_NONE;
    
    extra->evt_out_flags = 0;
    
    if(!err) 
    {
        switch(extra->e_type) 
        {
            case PF_Event_DRAW:
                err = DrawEvent(in_data, out_data, params, output, extra);
                break;
            
            case PF_Event_DO_CLICK:
                err = DoClick(in_data, out_data, params, output, extra);
                break;
        }
    }
    
    return err;
}
