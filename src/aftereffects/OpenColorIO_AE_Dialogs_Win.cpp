
//
// OpenColorIO AE
//
// After Effects implementation of OpenColorIO
//
// OpenColorIO.org
//

#include "OpenColorIO_AE_Dialogs.h"

#include <Windows.h>
#include <Icm.h>

#include <list>

#include "lcms2.h"


using namespace std;

HINSTANCE hDllInstance = NULL;

static void AppendString(char *text, int &length, const char *str, int len = -1)
{
	if(len < 0)
		len = strlen(str);

	const char *in = str;
	char *out = &text[length];

	for(int i=0; i < len; i++)
	{
		*out++ = *in++;

		length++;
	}
}

static void AppendNull(char *text, int &length)
{
	AppendString(text, length, "\0\0", 1);
}

static void MakeFilterText(char *filter_text, const ExtensionMap &extensions, bool do_combined)
{
	// Construct the Windows file dialog filter string, which looks like this:
	//
	//	"All OCIO files\0"
	//		"*.ocio;*.cube;*.vf;*.mga\0"
	//	"OpenColorIO (*.ocio)\0"
	//		"*.ocio\0"
	//	"Iridas (*.cube)\0"
	//		"*.cube\0"
	//	"Nuke Vectorfield (*.vf)\0"
	//		"*.vf\0"
	//	"Apple Color (*.mga)\0"
	//		"*.mga\0"
	//	"\0";
	//
	// Note the inline nulls and final double-null, which foil regular string functions.

	char combined_entry[512];;
	int combined_length = 0;

	char seperate_entries[512];
	int seperate_length = 0;

	AppendString(combined_entry, combined_length, "All OCIO files");
	AppendNull(combined_entry, combined_length);

	for(ExtensionMap::const_iterator i = extensions.begin(); i != extensions.end(); i++)
	{
		string extension = i->first;
		string format = i->second;

		string format_part = format + " (*." + extension + ")";
		string extension_part = "*." + extension;
		string combined_part = extension_part + ";";

		AppendString(seperate_entries, seperate_length, format_part.c_str(), format_part.size());
		AppendNull(seperate_entries, seperate_length);
		AppendString(seperate_entries, seperate_length, extension_part.c_str(), extension_part.size());
		AppendNull(seperate_entries, seperate_length);

		AppendString(combined_entry, combined_length, combined_part.c_str(), combined_part.size());
	}

	AppendNull(seperate_entries, seperate_length);
	AppendNull(combined_entry, combined_length);


	char *in = combined_entry;
	char *out = filter_text;

	if(do_combined)
	{
		for(int i=0; i < combined_length; i++)
			*out++ = *in++;
	}

	in = seperate_entries;

	for(int i=0; i < seperate_length; i++)
		*out++ = *in++;
}


bool OpenFile(char *path, int buf_len, const ExtensionMap &extensions, const void *hwnd)
{
	const char *my_lpstrTitle = "Import OCIO";
	const char *my_lpstrDefExt = "ocio";

	char my_lpstrFilter[512];
	MakeFilterText(my_lpstrFilter, extensions, true);


	OPENFILENAME lpofn;

	lpofn.lStructSize = sizeof(lpofn);
	lpofn.hwndOwner = (HWND)hwnd;
	lpofn.hInstance = hDllInstance;
	lpofn.lpstrFilter = my_lpstrFilter;
	lpofn.lpstrCustomFilter = NULL;
	lpofn.nMaxCustFilter = 0;
	lpofn.nFilterIndex = 0;
	lpofn.lpstrFile = path;
	lpofn.nMaxFile = buf_len;
	lpofn.lpstrFileTitle = path;
	lpofn.nMaxFileTitle = buf_len;
	lpofn.lpstrInitialDir = NULL;
	lpofn.lpstrTitle = my_lpstrTitle;
	lpofn.Flags = OFN_LONGNAMES |
					OFN_HIDEREADONLY | 
					OFN_PATHMUSTEXIST |
					OFN_OVERWRITEPROMPT;
	lpofn.nFileOffset = 0;
	lpofn.nFileExtension = 0;
	lpofn.lpstrDefExt = my_lpstrDefExt;
	lpofn.lCustData = 0;
	lpofn.lpfnHook = NULL;
	lpofn.lpTemplateName = NULL;
	lpofn.lStructSize = sizeof(lpofn);

	return GetOpenFileName(&lpofn);
}


bool SaveFile(char *path, int buf_len, const ExtensionMap &extensions, const void *hwnd)
{
	const char *my_lpstrTitle = "Export OCIO";
	const char *my_lpstrDefExt = "icc";

	char my_lpstrFilter[256];
	MakeFilterText(my_lpstrFilter, extensions, false);


	OPENFILENAME lpofn;

	lpofn.lStructSize = sizeof(lpofn);
	lpofn.hwndOwner = (HWND)hwnd;
	lpofn.hInstance = hDllInstance;
	lpofn.lpstrFilter = my_lpstrFilter;
	lpofn.lpstrCustomFilter = NULL;
	lpofn.nMaxCustFilter = 0;
	lpofn.nFilterIndex = 0;
	lpofn.lpstrFile = path;
	lpofn.nMaxFile = buf_len;
	lpofn.lpstrFileTitle = path;
	lpofn.nMaxFileTitle = buf_len;
	lpofn.lpstrInitialDir = NULL;
	lpofn.lpstrTitle = my_lpstrTitle;
	lpofn.Flags = OFN_LONGNAMES |
					OFN_HIDEREADONLY | 
					OFN_PATHMUSTEXIST |
					OFN_OVERWRITEPROMPT;
	lpofn.nFileOffset = 0;
	lpofn.nFileExtension = 0;
	lpofn.lpstrDefExt = my_lpstrDefExt;
	lpofn.lCustData = 0;
	lpofn.lpfnHook = NULL;
	lpofn.lpTemplateName = NULL;
	lpofn.lStructSize = sizeof(lpofn);

	return GetSaveFileName(&lpofn);
}

// dialog item IDs
enum {
	DLOG_noUI = -1,
	DLOG_OK = IDOK, // was 1
	DLOG_Cancel = IDCANCEL, // was 2
	DLOG_Profile_Menu = 3
};


static vector<string> *g_profile_vec = NULL;
static int g_selected_item = DLOG_noUI;

static WORD	g_item_clicked = 0;

static BOOL CALLBACK DialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{ 
    BOOL fError; 
 
    switch (message) 
    { 
      case WM_INITDIALOG:
		do{
			// add profile list to combo boxe
			HWND menu = GetDlgItem(hwndDlg, DLOG_Profile_Menu);

			for(int i=0; i < g_profile_vec->size(); i++)
			{
				SendMessage(menu,(UINT)CB_ADDSTRING,(WPARAM)wParam,(LPARAM)(LPCTSTR)g_profile_vec->at(i).c_str() );
				SendMessage(menu,(UINT)CB_SETITEMDATA, (WPARAM)i, (LPARAM)(DWORD)i); // this is the channel index number

				if( g_selected_item == i )
					SendMessage(menu, CB_SETCURSEL, (WPARAM)i, (LPARAM)0);
			}
		}while(0);
		return FALSE;
 
        case WM_COMMAND: 
			g_item_clicked = LOWORD(wParam);

            switch(LOWORD(wParam)) 
            { 
                case DLOG_OK: 
				case DLOG_Cancel:  // do the same thing, but g_item_clicked will be different
					do{
						HWND menu = GetDlgItem(hwndDlg, DLOG_Profile_Menu);

						LRESULT cur_sel = SendMessage(menu,(UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);

						g_selected_item = SendMessage(menu,(UINT)CB_GETITEMDATA, (WPARAM)cur_sel, (LPARAM)0);

					}while(0);

					EndDialog(hwndDlg, 0);
                    return TRUE; 
            } 
    }

    return FALSE; 
} 

bool GetMonitorProfile(char *path, int buf_len, const void *hwnd)
{
	list<string> profile_descriptions;
	map<string, string> profile_paths;

	// path to the monitor's profile
	char monitor_profile_path[256] = { '\0' };
	DWORD path_size = 256;
	BOOL get_icm_result = GetICMProfile(GetDC((HWND)hwnd), &path_size, monitor_profile_path);

	// directory where Windows stores its profiles
	char profile_directory[256] = { '\0' };
	DWORD dir_name_size = 256;
	BOOL get_color_dir_result = GetColorDirectory(NULL, profile_directory, &dir_name_size);

	// Get the profile file names from Windows
	ENUMTYPE enum_type;
	enum_type.dwSize = sizeof(ENUMTYPE);
	enum_type.dwVersion = ENUM_TYPE_VERSION;
	enum_type.dwFields = ET_DEVICECLASS;  // alternately could use ET_CLASS
	enum_type.dwDeviceClass = CLASS_MONITOR;

	BYTE *buf = NULL;
	DWORD buf_size = 0;
	DWORD num_profiles = 0;

	BOOL other_enum_result = EnumColorProfiles(NULL, &enum_type, buf, &buf_size, &num_profiles);

	if(buf_size > 0 && num_profiles > 0) // expect other_enum_result == FALSE and err == ERROR_INSUFFICIENT_BUFFER
	{
		buf = (BYTE *)malloc(buf_size);

		other_enum_result = EnumColorProfiles(NULL, &enum_type, buf, &buf_size, &num_profiles);

		if(other_enum_result)
		{
			// build a list of the profile descriptions
			// and a map to return the paths
			char *prof_name = (char *)buf;

			for(int i=0; i < num_profiles; i++)
			{
				string prof = prof_name;
				string prof_path = string(profile_directory) + "\\" + prof_name;

				cmsHPROFILE hProfile = cmsOpenProfileFromFile(prof_path.c_str(), "r");

				// Note: Windows will give us profiles that aren't ICC (.cdmp for example).
				// Don't worry, LittleCMS will just return NULL for those.
				if(hProfile)
				{
					char profile_description[256];

					cmsUInt32Number got_desc = cmsGetProfileInfoASCII(hProfile, cmsInfoDescription, "en", "US", profile_description, 256);

					if(got_desc)
					{
						profile_descriptions.push_back(profile_description);

						profile_paths[ profile_description ] = prof_path;
					}

					cmsCloseProfile(hProfile);
				}

				prof_name += strlen(prof_name) + 1;
			}
		}

		free(buf);
	}


	if(profile_descriptions.size() > 0)
	{
		// set a vector and selected index for building the profile menu
		profile_descriptions.sort();
		profile_descriptions.unique();

		vector<string> profile_vec;
		int selected = 0;

		for(list<string>::const_iterator i = profile_descriptions.begin(); i != profile_descriptions.end(); i++)
		{
			profile_vec.push_back( *i );

			if( profile_paths[ *i ] == monitor_profile_path)
			{
				selected = profile_vec.size() - 1;
			}
		}

		// run the dialog
		g_profile_vec = &profile_vec;
		g_selected_item = selected;

		int status = DialogBox(hDllInstance, (LPSTR)"PROFILEDIALOG", (HWND)hwnd, (DLGPROC)DialogProc);


		if(status == -1)
		{
			// dialog didn't open, my bad
			return true;
		}
		else if(g_item_clicked == DLOG_Cancel)
		{
			return false;
		}
		else
		{
			strncpy(path, profile_paths[ profile_vec[ g_selected_item ] ].c_str(), buf_len);

			return true;
		}
	}
	else
		return true;
}


int PopUpMenu(const MenuVec &menu_items, int selected_index, const void *hwnd)
{
	HMENU menu = CreatePopupMenu();

	if(menu)
	{
		for(int i=0; i < menu_items.size(); i++)
		{
			UINT flags = (i == selected_index ? (MF_STRING | MF_CHECKED) : MF_STRING);

			AppendMenu(menu, flags, i + 1, menu_items[i].c_str());
		}

		POINT pos;
		GetCursorPos(&pos);

		int result = TrackPopupMenuEx(menu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD, pos.x, pos.y, (HWND)hwnd, NULL);

		DestroyMenu(menu);

		if(result == 0)
		{
			// means the user clicked off the menu
			return selected_index;
		}
		else
			return result - 1;
	}
	else
		return selected_index;
}


BOOL WINAPI DllMain(HANDLE hInstance, DWORD fdwReason, LPVOID lpReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
		hDllInstance = (HINSTANCE)hInstance;

	return TRUE;
}
