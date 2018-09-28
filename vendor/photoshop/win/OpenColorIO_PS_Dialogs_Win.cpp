
#include "OpenColorIO_PS_Dialog.h"

#include <Windows.h>
#include <Shlobj.h>
//#include <commctrl.h>
//#include <stdio.h>

#include <fstream>
#include <map>
#include <cstdlib>

#include "OpenColorIO_PS_Context.h"
#include "OpenColorIO_PS_Version.h"

#include "OpenColorIO_AE_Dialogs.h"

#include "ocioicc.h"


static HINSTANCE hDllInstance = NULL;

enum {
    DLOG_noUI = -1,
	DLOG_OK = IDOK,
	DLOG_Cancel = IDCANCEL,
	DLOG_Export = 3,
	DLOG_Configuration_Label,
	DLOG_Configuration_Menu,
	DLOG_Convert_Radio,
	DLOG_Display_Radio,
	DLOG_Menu1_Label,
	DLOG_Menu1_Menu,
	DLOG_Menu1_Button,
	DLOG_Menu2_Label,
	DLOG_Menu2_Menu,
	DLOG_Menu2_Button,
	DLOG_Menu3_Label,
	DLOG_Menu3_Menu,
	DLOG_Invert_Check
};

static DialogSource    g_source;
static std::string     g_config; // path when source == SOURCE_CUSTOM
static DialogAction    g_action;
static bool            g_invert;
static DialogInterp    g_interpolation;
static std::string     g_inputSpace;
static std::string     g_outputSpace;
static std::string     g_device;
static std::string     g_transform;

static WORD	g_item_clicked = 0;

static OpenColorIO_PS_Context *g_context = NULL;

static HWND g_configutationToolTip = NULL;

// sensible Win macros
#define GET_ITEM(ITEM)	GetDlgItem(hwndDlg, (ITEM))

#define SET_LABEL_STRING(ITEM, STRING)	SetDlgItemText(hwndDlg, (ITEM), (STRING));

#define SET_CHECK(ITEM, VAL)	SendMessage(GET_ITEM(ITEM), BM_SETCHECK, (WPARAM)(VAL), (LPARAM)0)
#define GET_CHECK(ITEM)			SendMessage(GET_ITEM(ITEM), BM_GETCHECK, (WPARAM)0, (LPARAM)0)

#define ADD_MENU_ITEM(MENU, INDEX, STRING, VALUE, SELECTED) \
				SendMessage(GET_ITEM(MENU),( UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)(LPCTSTR)STRING ); \
				SendMessage(GET_ITEM(MENU),(UINT)CB_SETITEMDATA, (WPARAM)INDEX, (LPARAM)(DWORD)VALUE); \
				if(SELECTED) \
					SendMessage(GET_ITEM(MENU), CB_SETCURSEL, (WPARAM)INDEX, (LPARAM)0);

#define GET_NUMBER_MENU_ITEMS(MENU)	SendMessage(GET_ITEM(MENU), CB_GETCOUNT , (WPARAM)0, (LPARAM)0);
//#define GET_VALUE_OF_MENU_ITEM(MENU, INDEX)	SendMessage(GET_ITEM(MENU), CB_GETITEMDATA, (WPARAM)(INDEX), (LPARAM)0);
#define GET_STRING_OF_MENU_ITEM(MENU, INDEX, BUF)	SendMessage(GET_ITEM(MENU), (UINT)CB_GETLBTEXT, (WPARAM)(INDEX), (LPARAM)(BUF))

#define GET_MENU_VALUE(MENU)		SendMessage(GET_ITEM(MENU), (UINT)CB_GETITEMDATA, (WPARAM)SendMessage(GET_ITEM(MENU),(UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0), (LPARAM)0)

#define GET_MENU_VALUE_STRING(MENU, BUF)	SendMessage(GET_ITEM(MENU), (UINT)CB_GETLBTEXT, (WPARAM)SendMessage(GET_ITEM(MENU),(UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0), (LPARAM)(BUF))

#define SELECT_STRING_ITEM(MENU, STRING)	SendMessage(GET_ITEM(MENU), CB_SELECTSTRING, (WPARAM)-1, (LPARAM)(STRING));

#define REMOVE_ALL_MENU_ITEMS(MENU)	SendMessage(GET_ITEM(MENU), CB_RESETCONTENT, (WPARAM)0, (LPARAM)0);

#define ENABLE_ITEM(ITEM, ENABLE)	EnableWindow(GetDlgItem(hwndDlg, (ITEM)), (ENABLE));

#define SHOW_ITEM(ITEM, SHOW)		ShowWindow(GetDlgItem(hwndDlg, (ITEM)), (SHOW) ? SW_SHOW : SW_HIDE)

static void TrackInvert(HWND hwndDlg, bool readFromControl)
{
	assert(readFromControl); // should always be from the control

	g_invert = GET_CHECK(DLOG_Invert_Check);
}

static void TrackMenu3(HWND hwndDlg, bool readFromControl)
{
    assert(g_action == ACTION_DISPLAY);
    
    const int transformMenu = DLOG_Menu3_Menu;

    if(readFromControl)
    {
		char buf[256];

		GET_MENU_VALUE_STRING(transformMenu, buf);

		g_transform = buf;
    }
	else
    {
        // set menu from value
		bool match = false;

		const int numberOfItems = GET_NUMBER_MENU_ITEMS(transformMenu);
		
		for(int i=0; i < numberOfItems && !match; i++)
		{
			char buf[256];

			GET_STRING_OF_MENU_ITEM(transformMenu, i, buf);

			if(g_transform == buf)
			{
				match = true;

				SendMessage(GET_ITEM(transformMenu), CB_SETCURSEL, (WPARAM)i, (LPARAM)0);
			}
		}
		
		
		if(!match)
		{
			const std::string defaultTransform = g_context->getDefaultTransform(g_device);
			
			for(int i=0; i < numberOfItems && !match; i++)
			{
				char buf[256];

				GET_STRING_OF_MENU_ITEM(transformMenu, i, buf);

				if(defaultTransform == buf)
				{
					SendMessage(GET_ITEM(transformMenu), CB_SETCURSEL, (WPARAM)i, (LPARAM)0);
				}
			}

			g_transform = defaultTransform;
		}
    }
}

static void TrackMenu2(HWND hwndDlg, bool readFromControl)
{
    if(g_action == ACTION_DISPLAY)
    {
        const int deviceMenu = DLOG_Menu2_Menu;
        
		if(readFromControl)
		{
			char buf[256];

			GET_MENU_VALUE_STRING(deviceMenu, buf);

			g_device = buf;
		}
		else
		{
			SELECT_STRING_ITEM(deviceMenu, g_device.c_str());
		}
    }
    else
    {
        assert(g_action == ACTION_CONVERT);
        
		const int outputMenu = DLOG_Menu2_Menu;
        
		if(readFromControl)
		{
			char buf[256];

			GET_MENU_VALUE_STRING(outputMenu, buf);

			g_outputSpace = buf;
		}
		else
		{
			SELECT_STRING_ITEM(outputMenu, g_outputSpace.c_str());
		}
    }
    
    
    if(g_action == ACTION_DISPLAY)
    {
        const SpaceVec transforms = g_context->getTransforms(g_device);
        
		const int transformMenu = DLOG_Menu3_Menu;
        
		REMOVE_ALL_MENU_ITEMS(transformMenu);
        
		int index = 0;

        for(SpaceVec::const_iterator i = transforms.begin(); i != transforms.end(); ++i)
        {
			ADD_MENU_ITEM(transformMenu, index, i->c_str(), index, (g_transform == *i));
			index++;
        }
        
        TrackMenu3(hwndDlg, false);
    }
}

static void TrackMenu1(HWND hwndDlg, bool readFromControl)
{
	assert(readFromControl); // should always be from the control

    if(g_action == ACTION_LUT)
    {
        g_interpolation = (DialogInterp)GET_MENU_VALUE(DLOG_Menu1_Menu);
    }
    else
    {
		char buf[256] = { '\0' };

		GET_MENU_VALUE_STRING(DLOG_Menu1_Menu, buf);

        g_inputSpace = buf;
    }
}

static void TrackMenu1Button(HWND hwndDlg)
{
    if(g_context != NULL)
    {
        const bool chosen = ColorSpacePopUpMenu(g_context->getConfig(), g_inputSpace, false, hwndDlg);
        
        if(chosen)
        {
			const int inputSpaceMenu = DLOG_Menu1_Menu;

			const int numberOfItems = GET_NUMBER_MENU_ITEMS(inputSpaceMenu);
			
			for(int i=0; i < numberOfItems; i++)
			{
				char buf[256];

				GET_STRING_OF_MENU_ITEM(inputSpaceMenu, i, buf);

				if(g_inputSpace == buf)
				{
					SendMessage(GET_ITEM(inputSpaceMenu), CB_SETCURSEL, (WPARAM)i, (LPARAM)0);
				}
			}
        }
    }
}

static void TrackMenu2Button(HWND hwndDlg)
{
    if(g_context != NULL)
    {
        const bool chosen = ColorSpacePopUpMenu(g_context->getConfig(), g_outputSpace, false, hwndDlg);
        
        if(chosen)
        {
			const int outputSpaceMenu = DLOG_Menu2_Menu;

			const int numberOfItems = GET_NUMBER_MENU_ITEMS(outputSpaceMenu);
			
			for(int i=0; i < numberOfItems; i++)
			{
				char buf[256];

				GET_STRING_OF_MENU_ITEM(outputSpaceMenu, i, buf);

				if(g_outputSpace == buf)
				{
					SendMessage(GET_ITEM(outputSpaceMenu), CB_SETCURSEL, (WPARAM)i, (LPARAM)0);
				}
			}
        }
    }
}

static void TrackActionRadios(HWND hwndDlg, bool readFromControl)
{
	if(readFromControl)
	{
		if( GET_CHECK(DLOG_Display_Radio) )
		{
			g_action = ACTION_DISPLAY;
		}
		else
		{
			g_action = ACTION_CONVERT;
		}
	}
	else
	{
		assert(g_action != ACTION_LUT);

		if(g_action == ACTION_DISPLAY)
		{
			SET_CHECK(DLOG_Convert_Radio, FALSE);
			SET_CHECK(DLOG_Display_Radio, TRUE);
		}
		else
		{
			SET_CHECK(DLOG_Convert_Radio, TRUE);
			SET_CHECK(DLOG_Display_Radio, FALSE);
		}
	}
   
    
    assert(g_context != NULL);
    
    const int inputLabel = DLOG_Menu1_Label;
	const int inputMenu = DLOG_Menu1_Menu;

	SET_LABEL_STRING(inputLabel, "Input Space:");
    
	REMOVE_ALL_MENU_ITEMS(inputMenu);
    
    const SpaceVec &colorSpaces = g_context->getColorSpaces();
    
	int index = 0;

    for(SpaceVec::const_iterator i = colorSpaces.begin(); i != colorSpaces.end(); ++i)
    {
		ADD_MENU_ITEM(inputMenu, index, i->c_str(), index, (g_inputSpace == *i));
		index++;
    }
    
	SHOW_ITEM(DLOG_Menu1_Button, TRUE);
    
    
    if(g_action == ACTION_DISPLAY)
    {
        const int deviceLabel = DLOG_Menu2_Label;
        const int deviceMenu = DLOG_Menu2_Menu;
        
        const int transformLabel = DLOG_Menu3_Label;
        const int transformMenu = DLOG_Menu3_Menu;
        
		SET_LABEL_STRING(deviceLabel, "Device:");
		SHOW_ITEM(deviceLabel, TRUE);

		SHOW_ITEM(deviceMenu, TRUE);
        
		REMOVE_ALL_MENU_ITEMS(deviceMenu);
        
        const SpaceVec &devices = g_context->getDevices();
        
		index = 0;

        for(SpaceVec::const_iterator i = devices.begin(); i != devices.end(); ++i)
        {
			ADD_MENU_ITEM(deviceMenu, index, i->c_str(), index, (g_device == *i));
			index++;
        }
        
        SHOW_ITEM(DLOG_Menu2_Button, FALSE);
                
        
		SET_LABEL_STRING(transformLabel, "Device:");
		SHOW_ITEM(transformLabel, TRUE);

		SHOW_ITEM(transformMenu, TRUE);
        
		REMOVE_ALL_MENU_ITEMS(transformMenu);
        

        TrackMenu2(hwndDlg, false);
    }
    else
    {
        assert(g_action == ACTION_CONVERT);
        
        const int outputLabel = DLOG_Menu2_Label;
        const int outputMenu = DLOG_Menu2_Menu;
        
		SET_LABEL_STRING(outputLabel, "Output Space:");
		SHOW_ITEM(outputLabel, TRUE);

		SHOW_ITEM(outputMenu, TRUE);
        
		REMOVE_ALL_MENU_ITEMS(outputMenu);
        
		index = 0;

        for(SpaceVec::const_iterator i = colorSpaces.begin(); i != colorSpaces.end(); ++i)
        {
			ADD_MENU_ITEM(outputMenu, index, i->c_str(), index, (g_outputSpace == *i));
			index++;
        }
        
        SHOW_ITEM(DLOG_Menu2_Button, TRUE);
        
        
		SHOW_ITEM(DLOG_Menu3_Label, FALSE);
		SHOW_ITEM(DLOG_Menu3_Menu, FALSE);
    }
}

enum {
	CONFIG_ENVIRONMENT = 0,
	CONFIG_SEPERATOR,
	CONFIG_STANDARD,
	CONFIG_CUSTOM
};

static void TrackConfigMenu(HWND hwndDlg, bool readFromControl)
{
	std::string configPath;

	if(readFromControl)
	{
		const LRESULT source = GET_MENU_VALUE(DLOG_Configuration_Menu);

		if(source == CONFIG_SEPERATOR)
		{
			// invalid, read from params
			TrackConfigMenu(hwndDlg, false);
			return;
		}
		else if(source == CONFIG_ENVIRONMENT)
		{
			char *envFile = std::getenv("OCIO");

			g_source = SOURCE_ENVIRONMENT;
			
			if(envFile != NULL)
			{
				configPath = envFile;
			}
		}
		else if(source == CONFIG_CUSTOM)
		{
			ExtensionMap extensions;

            for(int i=0; i < OCIO::FileTransform::getNumFormats(); ++i)
            {
				const std::string extension = OCIO::FileTransform::getFormatExtensionByIndex(i);
				const std::string format = OCIO::FileTransform::getFormatNameByIndex(i);
                
				if(extension != "ccc") // .ccc files require an ID parameter
					extensions[ extension ] = format;
            }

			extensions[ "ocio" ] = "OCIO Format";

			char temp_path[256] = { '\0' };
			
			const bool clickedOK = OpenFile(temp_path, 255, extensions, hwndDlg);

			if(clickedOK)
			{
				g_source = SOURCE_CUSTOM;
				
				g_config = temp_path;

				configPath = g_config;
			}
			else
			{
				// go back to whatever we had
				TrackConfigMenu(hwndDlg, false);
				return;
			}
		}
		else
		{
			assert(source == CONFIG_STANDARD);

			char menu_string[256] = { '\0' };

			GET_MENU_VALUE_STRING(DLOG_Configuration_Menu, menu_string);
			
			g_source = SOURCE_STANDARD;

			g_config = menu_string;

			configPath = GetStdConfigPath(g_config);
		}
	}
	else
	{
		if(g_source == SOURCE_ENVIRONMENT)
		{
			char *envFile = std::getenv("OCIO");

			SELECT_STRING_ITEM(DLOG_Configuration_Menu, "$OCIO");
			
			if(envFile != NULL)
			{
				configPath = envFile;
			}
		}
		else if(g_source == SOURCE_CUSTOM)
		{
			SELECT_STRING_ITEM(DLOG_Configuration_Menu, "Custom...");

			configPath = g_config;
		}
		else
		{
			assert(g_source == SOURCE_STANDARD);

			SELECT_STRING_ITEM(DLOG_Configuration_Menu, g_config.c_str());

			configPath = GetStdConfigPath(g_config);
		}
	}
	

    if( !configPath.empty() )
    {
        try
        {
            delete g_context;
        
            g_context = new OpenColorIO_PS_Context(configPath);
            
            
            if( g_context->isLUT() )
            {
                g_action = ACTION_LUT;
                
				SHOW_ITEM(DLOG_Invert_Check, TRUE);
                
                if( g_context->canInvertLUT() )
                {
					ENABLE_ITEM(DLOG_Invert_Check, TRUE);
                }
                else
                {
                    ENABLE_ITEM(DLOG_Invert_Check, FALSE);
                
                    g_invert = false;
                }
                
				SET_CHECK(DLOG_Invert_Check, g_invert);
                
				SHOW_ITEM(DLOG_Convert_Radio, FALSE);
				SHOW_ITEM(DLOG_Display_Radio, FALSE);

                
                const int interpLabel = DLOG_Menu1_Label;
				const int interpMenu = DLOG_Menu1_Menu;

				const bool canTetrahedral = !g_context->canInvertLUT();

				SET_LABEL_STRING(interpLabel, "Interpolation:");
                
                // interpolation menu
				REMOVE_ALL_MENU_ITEMS(interpMenu);

				ADD_MENU_ITEM(interpMenu, 0, "Nearest Neighbor", INTERPO_NEAREST, (g_interpolation == INTERPO_NEAREST));
				ADD_MENU_ITEM(interpMenu, 1, "Linear", INTERPO_LINEAR, (g_interpolation == INTERPO_LINEAR));

				if(canTetrahedral)
					ADD_MENU_ITEM(interpMenu, 2, "Tetrahedral", INTERPO_TETRAHEDRAL, (g_interpolation == INTERPO_TETRAHEDRAL));

				ADD_MENU_ITEM(interpMenu, 3, "Best", INTERPO_BEST, (g_interpolation == INTERPO_BEST));
                

                SHOW_ITEM(DLOG_Menu1_Button, FALSE);
                
				SHOW_ITEM(DLOG_Menu2_Label, FALSE);
				SHOW_ITEM(DLOG_Menu2_Menu, FALSE);
                SHOW_ITEM(DLOG_Menu2_Button, FALSE);
                
				SHOW_ITEM(DLOG_Menu3_Label, FALSE);
				SHOW_ITEM(DLOG_Menu3_Menu, FALSE);
            }
            else
            {
                SHOW_ITEM(DLOG_Invert_Check, FALSE);
            
                if(g_action == ACTION_LUT)
                {
                    g_action = ACTION_CONVERT;
                }
                
				SHOW_ITEM(DLOG_Convert_Radio, TRUE);
				SHOW_ITEM(DLOG_Display_Radio, TRUE);
                                
                
                const SpaceVec &colorSpaces = g_context->getColorSpaces();
                
                if(-1 == FindSpace(colorSpaces, g_inputSpace))
                {
                    g_inputSpace = g_context->getDefaultColorSpace();
                }
                
                if(-1 == FindSpace(colorSpaces, g_outputSpace))
                {
                    g_outputSpace = g_context->getDefaultColorSpace();
                }
                
                
                const SpaceVec &devices = g_context->getDevices();
                
                if(-1 == FindSpace(devices, g_device))
                {
                    g_device = g_context->getDefaultDevice();
                }
                
                
                const SpaceVec transforms = g_context->getTransforms(g_device);
                
                if(-1 == FindSpace(transforms, g_transform))
                {
                    g_transform = g_context->getDefaultTransform(g_device);
                }
                
                
                TrackActionRadios(hwndDlg, false);
            }
        }
        catch(const std::exception &e)
        {
			MessageBox(hwndDlg, e.what(), "OpenColorIO error", MB_OK);

            if(g_source != SOURCE_ENVIRONMENT)
            {
                g_source = SOURCE_ENVIRONMENT;
                
                TrackConfigMenu(hwndDlg, false);
            }
        }
        catch(...)
        {
			MessageBox(hwndDlg, "Some unknown error", "OpenColorIO error", MB_OK);

            if(g_source != SOURCE_ENVIRONMENT)
            {
                g_source = SOURCE_ENVIRONMENT;
                
                TrackConfigMenu(hwndDlg, false);
            }
        }
    }
	else
	{
		delete g_context;

		g_context = NULL;

		REMOVE_ALL_MENU_ITEMS(DLOG_Menu1_Menu);
		REMOVE_ALL_MENU_ITEMS(DLOG_Menu2_Menu);
		REMOVE_ALL_MENU_ITEMS(DLOG_Menu3_Menu);
	}
            
    // set the tooltip
	char toolTipString[256];

	strncpy(toolTipString, configPath.c_str(), 255);

	
	if(g_configutationToolTip != NULL)
	{
		DestroyWindow(g_configutationToolTip);
		g_configutationToolTip = NULL;
	}

	g_configutationToolTip = CreateWindowEx(NULL, TOOLTIPS_CLASS, NULL,
											  WS_POPUP | TTS_ALWAYSTIP, // TTS_BALLOON
											  CW_USEDEFAULT, CW_USEDEFAULT,
											  CW_USEDEFAULT, CW_USEDEFAULT,
											  hwndDlg, NULL, 
											  hDllInstance, NULL);
	assert(g_configutationToolTip != NULL);

	TOOLINFO toolInfo = { 0 };
	toolInfo.cbSize = sizeof(toolInfo);
	toolInfo.hwnd = hwndDlg;
	toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
	toolInfo.uId = (UINT_PTR)GET_ITEM(DLOG_Configuration_Menu);
	toolInfo.lpszText = toolTipString;

	const LRESULT setTooltip = SendMessage(g_configutationToolTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);

	assert(setTooltip);
}

static void DoExport(HWND hwndDlg)
{
	if(g_context != NULL)
	{
		try
		{
			ExtensionMap extensions;
		    
			for(int i=0; i < OCIO::Baker::getNumFormats(); ++i)
			{
				const char *extension = OCIO::Baker::getFormatExtensionByIndex(i);
				const char *format = OCIO::Baker::getFormatNameByIndex(i);
		        
				extensions[ extension ] = format;
			}
		    
			extensions[ "icc" ] = "ICC Profile";
		    

			char path[256] = { '\0' };
		    
			const bool result = SaveFile(path, 255, extensions, hwndDlg);
		    
		    
			if(result)
			{
				std::string the_path(path);
				std::string the_extension = the_path.substr( the_path.find_last_of('.') + 1 );
		        
				bool do_export = true;
		        
				std::string monitor_icc_path;
		        
				if(the_extension == "icc")
				{
					char monitor_path[256] = {'\0'};
		        
					do_export = GetMonitorProfile(monitor_path, 255, hwndDlg);
		        
					if(do_export)
					{
						OCIO::ConstProcessorRcPtr processor;
	                    
						if(g_action == ACTION_CONVERT)
						{
							processor = g_context->getConvertProcessor(g_inputSpace, g_outputSpace);
						}
						else if(g_action == ACTION_DISPLAY)
						{
							processor = g_context->getDisplayProcessor(g_inputSpace, g_device, g_transform);
						}
						else
						{
							assert(g_action == ACTION_LUT);
	                        
							const OCIO::Interpolation interp = (g_interpolation == INTERPO_NEAREST ? OCIO::INTERP_NEAREST :
																g_interpolation == INTERPO_LINEAR ? OCIO::INTERP_LINEAR :
																g_interpolation == INTERPO_TETRAHEDRAL ? OCIO::INTERP_TETRAHEDRAL :
																OCIO::INTERP_BEST);
	                        
							const OCIO::TransformDirection direction = (g_invert ? OCIO::TRANSFORM_DIR_INVERSE : OCIO::TRANSFORM_DIR_FORWARD);
	                        
							processor = g_context->getLUTProcessor(interp, direction);
						}
	                    
	                    
						int cubesize = 32;
						int whitepointtemp = 6505;
						std::string copyright = "OpenColorIO, Sony Imageworks";
	                    
						// create a description tag from the filename
						const char delimiter = '\\';
						const size_t filename_start = the_path.find_last_of(delimiter) + 1;
						const size_t filename_end = the_path.find_last_of('.') - 1;
				        
						std::string description = the_path.substr(the_path.find_last_of(delimiter) + 1,
															1 + filename_end - filename_start);

						SaveICCProfileToFile(the_path, processor, cubesize, whitepointtemp,
												monitor_path, description, copyright, false);
					}
				}
				else
				{
					// need an extension->format map
					std::map<std::string, std::string> extensions;
	                
					for(int i=0; i < OCIO::Baker::getNumFormats(); ++i)
					{
						const char *extension = OCIO::Baker::getFormatExtensionByIndex(i);
						const char *format = OCIO::Baker::getFormatNameByIndex(i);
	                    
						extensions[ extension ] = format;
					}
	                
					std::string format = extensions[ the_extension ];
	                
	                
					OCIO::BakerRcPtr baker;
	                
					if(g_action == ACTION_CONVERT)
					{
						baker = g_context->getConvertBaker(g_inputSpace, g_outputSpace);
					}
					else if(g_action == ACTION_DISPLAY)
					{
						baker = g_context->getDisplayBaker(g_inputSpace, g_device, g_transform);
					}
					else
					{
						assert(g_action == ACTION_LUT);
	                    
						const OCIO::Interpolation interp = (g_interpolation == INTERPO_NEAREST ? OCIO::INTERP_NEAREST :
															g_interpolation == INTERPO_LINEAR ? OCIO::INTERP_LINEAR :
															g_interpolation == INTERPO_TETRAHEDRAL ? OCIO::INTERP_TETRAHEDRAL :
															OCIO::INTERP_BEST);
	                    
						const OCIO::TransformDirection direction = (g_invert ? OCIO::TRANSFORM_DIR_INVERSE : OCIO::TRANSFORM_DIR_FORWARD);
	                    
						baker = g_context->getLUTBaker(interp, direction);
					}
	                
					baker->setFormat( format.c_str() );
	                
					std::ofstream f(the_path.c_str());
					baker->bake(f);
				}
			}
		}
        catch(const std::exception &e)
        {
			MessageBox(hwndDlg, e.what(), "OpenColorIO error", MB_OK);

            if(g_source != SOURCE_ENVIRONMENT)
            {
                g_source = SOURCE_ENVIRONMENT;
                
                TrackConfigMenu(hwndDlg, false);
            }
        }
        catch(...)
        {
			MessageBox(hwndDlg, "Some unknown error", "OpenColorIO error", MB_OK);

            if(g_source != SOURCE_ENVIRONMENT)
            {
                g_source = SOURCE_ENVIRONMENT;
                
                TrackConfigMenu(hwndDlg, false);
            }
        }
	}
}

static BOOL CALLBACK DialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{ 
    BOOL fError; 
 
    switch (message) 
    { 
		case WM_INITDIALOG:
			do{
				int index = 0;
				
				char *envFile = std::getenv("OCIO");

				const DWORD envType = (envFile == NULL ? CONFIG_SEPERATOR : CONFIG_ENVIRONMENT);
				const bool envSelected = (envFile == NULL != NULL && g_source == SOURCE_ENVIRONMENT);

				ADD_MENU_ITEM(DLOG_Configuration_Menu, index, "$OCIO", CONFIG_ENVIRONMENT, envSelected);
				index++;

				ADD_MENU_ITEM(DLOG_Configuration_Menu, index, "-", CONFIG_SEPERATOR, FALSE);
				index++;

				ConfigVec standardConfigs;
				GetStdConfigs(standardConfigs);
				
				if(standardConfigs.size() > 0)
				{
					for(int i=0; i < standardConfigs.size(); i++)
					{
						const std::string &config = standardConfigs[i];
						
						const bool selected = (g_source == SOURCE_STANDARD && config == g_config);

						ADD_MENU_ITEM(DLOG_Configuration_Menu, index, config.c_str(), CONFIG_STANDARD, selected);
						index++;
					}
				}
				else
				{
					std::string label = "No standard configs";

					char appdata_path[MAX_PATH];
					HRESULT result = SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA, NULL,
														SHGFP_TYPE_CURRENT, appdata_path);

					if(result == S_OK)
					{
						label = "(No configs in " + std::string(appdata_path) + "\\OpenColorIO\\)";
					}

					ADD_MENU_ITEM(DLOG_Configuration_Menu, index, label.c_str(), CONFIG_SEPERATOR, FALSE);
					index++;
				}

				ADD_MENU_ITEM(DLOG_Configuration_Menu, index, "-", CONFIG_SEPERATOR, FALSE);
				index++;

				ADD_MENU_ITEM(DLOG_Configuration_Menu, index, "Custom...", CONFIG_CUSTOM, g_source == SOURCE_CUSTOM);
				index++;
				

				TrackConfigMenu(hwndDlg, false);

			}while(0);
		return TRUE;
 
		case WM_NOTIFY:
			return FALSE;

        case WM_COMMAND: 
			g_item_clicked = LOWORD(wParam);

			const int menuParam = HIWORD(wParam);

            switch (LOWORD(wParam)) 
            { 
                case DLOG_OK: 
				case DLOG_Cancel:  // do the same thing, but g_item_clicked will be different

					delete g_context;
					g_context = NULL;
					
					DestroyWindow(g_configutationToolTip);
					g_configutationToolTip = NULL;

					// quit dialog
					//PostMessage((HWND)hwndDlg, WM_QUIT, (WPARAM)WA_ACTIVE, lParam);
					EndDialog(hwndDlg, 0);
                    //DestroyWindow(hwndDlg); 
				return TRUE;


				case DLOG_Export:
					DoExport(hwndDlg);
				return TRUE;


				case DLOG_Configuration_Menu:
					if(menuParam == CBN_SELCHANGE)
						TrackConfigMenu(hwndDlg, true);
				return TRUE;


				case DLOG_Convert_Radio:
				case DLOG_Display_Radio:
					TrackActionRadios(hwndDlg, true);
				return TRUE;


				case DLOG_Menu1_Menu:
					if(menuParam == CBN_SELCHANGE)
						TrackMenu1(hwndDlg, true);
				return TRUE;


				case DLOG_Menu1_Button:
					TrackMenu1Button(hwndDlg);
				return TRUE;


				case DLOG_Menu2_Menu:
					if(menuParam == CBN_SELCHANGE)
						TrackMenu2(hwndDlg, true);
				return TRUE;


				case DLOG_Menu2_Button:
					TrackMenu2Button(hwndDlg);
				return TRUE;

				
				case DLOG_Menu3_Menu:
					if(menuParam == CBN_SELCHANGE)
						TrackMenu3(hwndDlg, true);
				return TRUE;


				case DLOG_Invert_Check:
					TrackInvert(hwndDlg, true);
				return TRUE;
            } 

			return FALSE;
    } 
    return FALSE; 
} 

DialogResult OpenColorIO_PS_Dialog(DialogParams &params, const void *plugHndl, const void *mwnd)
{
	hDllInstance = (HINSTANCE)plugHndl;
	SetHInstance(hDllInstance);

	g_source = params.source;
	g_config = params.config;
	g_action = params.action;
	g_invert = params.invert;
	g_interpolation = params.interpolation;
	g_inputSpace = params.inputSpace;
	g_outputSpace = params.outputSpace;
	g_device = params.device;
	g_transform = params.transform;
	

	const int status = DialogBox((HINSTANCE)plugHndl, (LPSTR)"OCIODIALOG", (HWND)mwnd, (DLGPROC)DialogProc);
	

	if(g_item_clicked == IDOK)
	{
		params.source = g_source;
		params.config = g_config;
		params.action = g_action;
		params.invert = g_invert;
		params.interpolation = g_interpolation;
		params.inputSpace = g_inputSpace;
		params.outputSpace = g_outputSpace;
		params.device = g_device;
		params.transform = g_transform;

		return RESULT_OK;
	}
	else
		return RESULT_CANCEL;
}

void OpenColorIO_PS_About(const void *plugHndl, const void *mwnd)
{
	const std::string ocioVersion = OCIO::GetVersion();
	
	const std::string endl = "\n";

	std::string text = std::string("OpenColorIO") + endl +
						__DATE__ + endl +
						endl +
						"OCIO version " + OCIO::GetVersion();

	MessageBox((HWND)mwnd, text.c_str(), "OpenColorIO", MB_OK);
}
