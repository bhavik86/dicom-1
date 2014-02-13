#include "GaussDeriv.h"

/* Functions to compute the integral, and the 0th and 1st derivative of the
   Gaussian function 1/(sqrt(2*PI)*sigma)*exp(-0.5*x^2/sigma^2) */

/* Integral of the Gaussian function */
double phi0(double x, double sigma)
{
	return normal(x/sigma);
}

/* The Gaussian function */
double phi1(double x, double sigma)
{
	double t = x/sigma;
	return SQRT_2_PI_INV/sigma*exp(-0.5*t*t);
}

/* First derivative of the Gaussian function */
double phi2(double x, double sigma)
{
	double t = x/sigma;
	return -x*SQRT_2_PI_INV/pow(sigma,3.0)*exp(-0.5*t*t);
}

/* Gaussian smoothing mask */
Mat gaussDeriv0(double sigma)
{
	int n = (int)MASK_SIZE(MAX_SIZE_MASK_0, sigma); /* Error < 0.001 on each side */
	Mat res(1, 2*n+1, CV_64F);
	double * mask = (double*)res.data + n;
	for (int i = -n + 1; i <= n - 1; i++)
		mask[i] = phi0(-i+0.5, sigma) - phi0(-i-0.5, sigma);
	mask[-n] = 1.0 - phi0(n-0.5, sigma);
	mask[n] = phi0(-n+0.5, sigma);
	return res;
}


/* First derivative of Gaussian smoothing mask */
Mat gaussDeriv1(double sigma)
{
	int n = (int)MASK_SIZE(MAX_SIZE_MASK_1, sigma); /* Error < 0.001 on each side */
	Mat res(1, 2*n+1, CV_64F);
	double * mask = (double*)res.data + n;
	for (int i = -n + 1; i <= n - 1; i++)
		mask[i] = phi1(-i+0.5, sigma) - phi1(-i-0.5, sigma);
	mask[-n] = -phi1(n-0.5, sigma);
	mask[n] = phi1(-n+0.5, sigma);
	return res;
}

/* Second derivative of Gaussian smoothing mask */
Mat gaussDeriv2(double sigma)
{
	int n = (int)MASK_SIZE(MAX_SIZE_MASK_2, sigma); /* Error < 0.001 on each side */
	Mat res(1, 2*n+1, CV_64F);
	double * mask = (double*)res.data + n;
	for (int i = -n + 1; i <= n - 1; i++)
		mask[i] = phi2(-i+0.5, sigma) - phi2(-i-0.5, sigma);
	mask[-n] = -phi2(n-0.5, sigma);
	mask[n] = phi2(-n+0.5, sigma);
	return res;
}

void gaussDerivs(Mat * G, double sigma)
{
	G[0] = gaussDeriv0(sigma);
	G[1] = gaussDeriv1(sigma);
	G[2] = gaussDeriv2(sigma);
}