

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
/*
	char *my_lpstrFilter =
		"All LUT files\0"
			"*.alut;*.cube;*.ocio;*.vf;*.a3d;*.shklut;*.amp;*.mga\0"
		"Fusion (*.alut)\0"
			"*.alut\0"
		"Iridas (*.cube)\0"
			"*.cube\0"
		"OpenColorIO (*.ocio)\0"
			"*.ocio\0"
		//"Nuke Lookup Project (*.nk)\0"
		//	"*.nk\0"
		"Nuke Vectorfield (*.vf)\0"
			"*.vf\0"
		"Panavision (*.a3d)\0"
			"*.a3d\0"
		"Shake (*.shklut)\0"
			"*.shklut\0"
		"Photoshop (*.amp)\0"
			"*.amp\0"
		"Apple Color (*.mga)\0"
			"*.mga\0"
		"\0";
*/

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

	BOOL result = GetOpenFileName(&lpofn);

	if(!result)
	{
		DWORD err = CommDlgExtendedError();

		if(err == PDERR_SETUPFAILURE)
		{
			err = PDERR_PARSEFAILURE;
		}
	}

	return result;
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


int PopUpMenu(const MenuVec &menu_items, int selected_index)
{
	return 0;
}


BOOL WINAPI DllMain(HANDLE hInstance, DWORD fdwReason, LPVOID lpReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
		hDllInstance = (HINSTANCE)hInstance;

	return TRUE;   // Indicate that the DLL was initialized successfully.
}
