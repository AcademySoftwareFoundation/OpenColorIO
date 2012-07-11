
#include "lcms2.h"


static
double VecDist(BYTE bin[3], BYTE bout[3])
{
       double rdist, gdist, bdist;

       rdist = fabs(bout[0] - bin[0]);
       gdist = fabs(bout[1] - bin[1]);
       bdist = fabs(bout[2] - bin[2]);

       return (sqrt((rdist*rdist + gdist*gdist + bdist*bdist)));
}


int main(int  argc, char* argv[])
{

	int r, g, b;
	BYTE RGB[3], RGB_OUT[3];
	cmsHTRANSFORM xform;
	cmsHPROFILE hProfile;
	double err, SumX=0, SumX2=0, Peak = 0, n = 0;


	if (argc != 2) {
		printf("roundtrip <icc profile>\n");
		return 1;
	}

	hProfile = cmsOpenProfileFromFile(argv[1], "r");
	xform = cmsCreateTransform(hProfile,TYPE_RGB_8, hProfile, TYPE_RGB_8, INTENT_RELATIVE_COLORIMETRIC, cmsFLAGS_NOTPRECALC);

	for (r=0; r< 256; r++) {
		printf("%d  \r", r);
		for (g=0; g < 256; g++) {
			for (b=0; b < 256; b++) {

				RGB[0] = r;
				RGB[1] = g;
				RGB[2] = b;

				cmsDoTransform(xform, RGB, RGB_OUT, 1);

				err = VecDist(RGB, RGB_OUT);

				SumX  += err;
                SumX2 += err * err;
				n += 1.0;
				if (err > Peak)
					Peak = err;

			}
		}
	}

	printf("Average %g\n", SumX / n);
	printf("Max %g\n", Peak);
	printf("Std  %g\n", sqrt((n*SumX2 - SumX * SumX) / (n*(n-1))));
	cmsCloseProfile(hProfile);
	cmsDeleteTransform(xform);

		return 0;
}