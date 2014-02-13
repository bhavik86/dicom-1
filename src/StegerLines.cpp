#include "StegerLines.h"
#include <iostream>

using namespace std;

#define PIXEL_BOUNDARY 0.6f

void compute_eigenvals(
  double dfdrr,
  double dfdrc,
  double dfdcc,
  double eigval[2],
  double eigvec[2][2]
  )
{
  double theta, t, c, s, e1, e2, n1, n2; /* , phi; */

  /* Compute the eigenvalues and eigenvectors of the Hessian matrix. */
  if (dfdrc != 0.0) {
    theta = 0.5*(dfdcc-dfdrr)/dfdrc;
    t = 1.0/(fabs(theta)+sqrt(theta*theta+1.0));
    if (theta < 0.0) t = -t;
    c = 1.0/sqrt(t*t+1.0);
    s = t*c;
    e1 = dfdrr-t*dfdrc;
    e2 = dfdcc+t*dfdrc;
  } else {
    c = 1.0;
    s = 0.0;
    e1 = dfdrr;
    e2 = dfdcc;
  }
  n1 = c;
  n2 = -s;

  /* If the absolute value of an eigenvalue is larger than the other, put that
     eigenvalue into first position.  If both are of equal absolute value, put
     the negative one first. */
  if (fabs(e1) > fabs(e2)) {
    eigval[0] = e1;
    eigval[1] = e2;
    eigvec[0][0] = n1;
    eigvec[0][1] = n2;
    eigvec[1][0] = -n2;
    eigvec[1][1] = n1;
  } else if (fabs(e1) < fabs(e2)) {
    eigval[0] = e2;
    eigval[1] = e1;
    eigvec[0][0] = -n2;
    eigvec[0][1] = n1;
    eigvec[1][0] = n1;
    eigvec[1][1] = n2;
  } else {
    if (e1 < e2) {
      eigval[0] = e1;
      eigval[1] = e2;
      eigvec[0][0] = n1;
      eigvec[0][1] = n2;
      eigvec[1][0] = -n2;
      eigvec[1][1] = n1;
    } else {
      eigval[0] = e2;
      eigval[1] = e1;
      eigvec[0][0] = -n2;
      eigvec[0][1] = n1;
      eigvec[1][0] = n1;
      eigvec[1][1] = n2;
    }
  }
}

void stegerEdges(const Mat & img, Mat & edges, double sigma, double low, double high)
{
	edges.create(img.size(), CV_8U);
	edges = Scalar(0);

	Mat G[3];
	gaussDerivs(G, sigma);

	Mat I[5];
	sepFilter2D(img, I[0], CV_64F, G[1], G[0]);
	sepFilter2D(img, I[1], CV_64F, G[0], G[1]);
	sepFilter2D(img, I[2], CV_64F, G[2], G[0]);
	sepFilter2D(img, I[3], CV_64F, G[1], G[1]);
	sepFilter2D(img, I[4], CV_64F, G[0], G[2]);

	double rng = (high - low) / 255.0;

    for (int y = 0; y < I[0].rows; y++)
	{
        int offs = y*I[0].cols;
		for (int x = 0; x < I[0].cols; x++, offs++)
		{
			double Ip[5];
			Ip[0] = ((double*)I[0].data)[offs];
			Ip[1] = ((double*)I[1].data)[offs];
			Ip[2] = ((double*)I[2].data)[offs];
			Ip[3] = ((double*)I[3].data)[offs];
			Ip[4] = ((double*)I[4].data)[offs];

			double eigval[2], eigvec[2][2];
			compute_eigenvals(Ip[2], Ip[3], Ip[4], eigval, eigvec);

            double val = eigval[0];
            //if (val > 0.0)
            //{
				double n1 = eigvec[0][0];
				double n2 = eigvec[0][1];
				double a = Ip[2]*n1*n1 + 2.0*Ip[3]*n1*n2 + Ip[4]*n2*n2;
                double b = Ip[0]*n1 + Ip[1]*n2;

                if (a != 0.0)
                {
					double t = -b / a;
					double p1 = t*n1;
					double p2 = t*n2;
					if (abs(p1) <= PIXEL_BOUNDARY && abs(p2) <= PIXEL_BOUNDARY) {
                //		if (val >= low)
                            //std::cout << (val - low) / rng << std::endl;
                            edges.at<uchar>(y, x) = saturate_cast<uchar>((val - low) / rng * 100);
                    //}
				}
			}
        }
	}
}
