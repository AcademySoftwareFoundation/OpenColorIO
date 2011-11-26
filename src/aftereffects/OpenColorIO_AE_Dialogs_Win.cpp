

#include "OpenColorIO_AE_Dialogs.h"

#include <Windows.h>

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

	path[0] = '\0';

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


bool GetMonitorProfile(char *path, int buf_len, const void *hwnd)
{
	return false;
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
