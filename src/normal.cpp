/*  Calculate the normal distribution; part of detect-lines.
    Copyright (C) 1996-1998 Carsten Steger

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */

#include "normal.h"

/* Compute the integral of the Gaussian, i.e., the normal distribution. */

#define SQRT2 1.41421356237309504880

#define SQRTPI 1.772453850905516027

#define UPPERLIMIT 20.0 

#define P10 242.66795523053175
#define P11 21.979261618294152
#define P12 6.9963834886191355
#define P13 -.035609843701815385
#define Q10 215.05887586986120
#define Q11 91.164905404514901
#define Q12 15.082797630407787
#define Q13 1.0

#define P20 300.4592610201616005
#define P21 451.9189537118729422
#define P22 339.3208167343436870
#define P23 152.9892850469404039
#define P24 43.16222722205673530
#define P25 7.211758250883093659
#define P26 .5641955174789739711
#define P27 -.0000001368648573827167067
#define Q20 300.4592609569832933
#define Q21 790.9509253278980272
#define Q22 931.3540948506096211
#define Q23 638.9802644656311665
#define Q24 277.5854447439876434
#define Q25 77.00015293522947295
#define Q26 12.78272731962942351
#define Q27 1.0

#define P30 -.00299610707703542174
#define P31 -.0494730910623250734
#define P32 -.226956593539686930
#define P33 -.278661308609647788
#define P34 -.0223192459734184686
#define Q30 .0106209230528467918
#define Q31 .191308926107829841
#define Q32 1.05167510706793207
#define Q33 1.98733201817135256
#define Q34 1.0


double normal(double x)
{
	int    sn;
	double R1, R2, y, y2, y3, y4, y5, y6, y7;
	double erf, erfc, z, z2, z3, z4;
	double phi;

	if (x < -UPPERLIMIT) return 0.0;
	if (x > UPPERLIMIT) return 1.0;

	y = x / SQRT2;
	if (y < 0) {
		y = -y;
		sn = -1;
	} else
		sn = 1;

	y2 = y * y;
	y4 = y2 * y2;
	y6 = y4 * y2;

	if (y < 0.46875) {
		R1 = P10 + P11 * y2 + P12 * y4 + P13 * y6;
		R2 = Q10 + Q11 * y2 + Q12 * y4 + Q13 * y6;
		erf = y * R1 / R2;
		if (sn == 1)
			phi = 0.5 + 0.5*erf;
		else 
			phi = 0.5 - 0.5*erf;
	} else if (y < 4.0) {
		y3 = y2 * y;
		y5 = y4 * y;
		y7 = y6 * y;
		R1 = P20 + P21 * y + P22 * y2 + P23 * y3 + 
			P24 * y4 + P25 * y5 + P26 * y6 + P27 * y7;
		R2 = Q20 + Q21 * y + Q22 * y2 + Q23 * y3 + 
			Q24 * y4 + Q25 * y5 + Q26 * y6 + Q27 * y7;
		erfc = exp(-y2) * R1 / R2;
		if (sn == 1)
			phi = 1.0 - 0.5*erfc;
		else
			phi = 0.5*erfc;
	} else {
		z = y4;
		z2 = z * z;
		z3 = z2 * z;
		z4 = z2 * z2;
		R1 = P30 + P31 * z + P32 * z2 + P33 * z3 + P34 * z4;
		R2 = Q30 + Q31 * z + Q32 * z2 + Q33 * z3 + Q34 * z4;
		erfc = (exp(-y2)/y) * (1.0 / SQRTPI + R1 / (R2 * y2));
		if (sn == 1)
			phi = 1.0 - 0.5*erfc;
		else 
			phi = 0.5*erfc;
	} 

	return phi;
}