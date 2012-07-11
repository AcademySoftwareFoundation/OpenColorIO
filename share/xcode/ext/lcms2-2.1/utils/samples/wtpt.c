//
//  Little cms
//  Copyright (C) 1998-2000 Marti Maria
//
// THIS SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
// EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
// WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
//
// IN NO EVENT SHALL MARTI MARIA BE LIABLE FOR ANY SPECIAL, INCIDENTAL,
// INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
// OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
// WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF
// LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
// OF THIS SOFTWARE.
//
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

// Example: how to show white points of profiles


#include "lcms.h"



static
void ShowWhitePoint(LPcmsCIEXYZ WtPt)
{       
	   cmsCIELab Lab;
	   cmsCIELCh LCh;
	   cmsCIExyY xyY;
       char Buffer[1024];

		
	   _cmsIdentifyWhitePoint(Buffer, WtPt);
       printf("%s\n", Buffer);
       
       cmsXYZ2Lab(NULL, &Lab, WtPt);
	   cmsLab2LCh(&LCh, &Lab);
	   cmsXYZ2xyY(&xyY, WtPt);

	   printf("XYZ=(%3.1f, %3.1f, %3.1f)\n", WtPt->X, WtPt->Y, WtPt->Z);
       printf("Lab=(%3.3f, %3.3f, %3.3f)\n", Lab.L, Lab.a, Lab.b);
	   printf("(x,y)=(%3.3f, %3.3f)\n", xyY.x, xyY.y);
	   printf("Hue=%3.2f, Chroma=%3.2f\n", LCh.h, LCh.C);
       printf("\n");
       
}


int main (int argc, char *argv[])
{
       printf("Show media white of profiles, identifying black body locus. v2\n\n");


       if (argc == 2) {
				  cmsCIEXYZ WtPt;
		          cmsHPROFILE hProfile = cmsOpenProfileFromFile(argv[1], "r");

				  printf("%s\n", cmsTakeProductName(hProfile));
				  cmsTakeMediaWhitePoint(&WtPt, hProfile);
				  ShowWhitePoint(&WtPt);
				  cmsCloseProfile(hProfile);
              }
       else
              {
              cmsCIEXYZ xyz;
              
              printf("usage:\n\nIf no parameters are given, then this program will\n");
              printf("ask for XYZ value of media white. If parameter given, it must be\n");
              printf("the profile to inspect.\n\n");

              printf("X? "); scanf("%lf", &xyz.X);
              printf("Y? "); scanf("%lf", &xyz.Y);
              printf("Z? "); scanf("%lf", &xyz.Z);

			  ShowWhitePoint(&xyz);
              }

	   return 0;
}

