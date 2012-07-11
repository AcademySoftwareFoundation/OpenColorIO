
#include <windows.h>
#include "lcms.h"

static cmsHPROFILE prof_xyz,prof_rgb;
static cmsHTRANSFORM trans_xyz_to_rgb,trans_rgb_to_xyz;

static DWORD WINAPI make_trans_xyz_to_rgb(LPVOID lpParameter)
{
 trans_xyz_to_rgb = cmsCreateTransform(
  prof_xyz,TYPE_XYZ_DBL,
  prof_rgb,TYPE_RGB_DBL,
  INTENT_ABSOLUTE_COLORIMETRIC,cmsFLAGS_NOTPRECALC);
 return 0;
}

static DWORD WINAPI make_trans_rgb_to_xyz(LPVOID lpParameter)
{
 trans_rgb_to_xyz = cmsCreateTransform(
  prof_rgb,TYPE_RGB_DBL,
  prof_xyz,TYPE_XYZ_DBL,
  INTENT_ABSOLUTE_COLORIMETRIC,cmsFLAGS_NOTPRECALC);
 return 0;
}

int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR
lpCmdLine,int nCmdShow)
{
 prof_xyz = cmsCreateXYZProfile();
 prof_rgb = cmsOpenProfileFromFile("AdobeRGB1998.icc","rb");
//cmsCreate_sRGBProfile();
 for (int i=0;i<10;++i)
 {
#define try_threads
#ifdef try_threads
  DWORD threadid;
  HANDLE workers[2];
  workers[0] = CreateThread(NULL,0,make_trans_xyz_to_rgb,NULL,0,&threadid);
  workers[1] = CreateThread(NULL,0,make_trans_rgb_to_xyz,NULL,0,&threadid);
  WaitForMultipleObjects(2,workers,TRUE,INFINITE);
  for (unsigned i=0;i<2;++i)
   CloseHandle(workers[i]);
#else
  make_trans_xyz_to_rgb(0);
  make_trans_rgb_to_xyz(0);
#endif
  cmsDeleteTransform(trans_xyz_to_rgb);
  cmsDeleteTransform(trans_rgb_to_xyz);
 }
 cmsCloseProfile(prof_rgb);
 cmsCloseProfile(prof_xyz);
}
