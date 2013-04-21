/*****************************************************************
 * This file is part of jot-lib (or "jot" for short):
 *   <http://code.google.com/p/jot-lib/>
 * 
 * jot-lib is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 * 
 * jot-lib is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with jot-lib.  If not, see <http://www.gnu.org/licenses/>.`
 *****************************************************************/
#include "npr/image_proc.H"

#ifdef USE_IMAGEPROC

// Convolution

/* convolve a single line (row or column) in the image with the filter
 */
void ProcImage::convolveLine(ProcImageLine simg,
                             ProcKernel1D& filter,
                             ProcImageLine dimg)
{
    size_t len = simg.size();
    size_t rad = filter.rad();
    int i, j;

    for (i = 0; i < len; i++) {
        // Zero out result
        double val = 0.0;
        int left = i - filter.rad(), right = i + filter.rad();
        if (left < 0) {
            // Part off left side
            for (j = left; j < 0; j++) {
                // Repeat boundary
                val += simg[0] * filter[i-j];
            }
            left = 0;
        }
        if (right >= len) {
            // Part off right side
            for (j = len; j <= right; j++) {
                // Repeat boundary
                val += simg[len-1] * filter[i-j];
            }
            right = len-1;
        }
        
        // Inside (away from boundaries)
        for (j = left; j <= right; j++) {
            val += simg[j] * filter[i-j];
        }
        
        // Store result in destination
        dimg[i] = val;
    }
}

/* perform a 1D convolution on entire image using horizontal filter
 */
void ProcImage::convolveX(ProcKernel1D& filter, ProcImage& dest)
{
    assert(width() == dest.width() && height() == dest.height());

    for (int i = 0; i < height(); i++) {
        convolveLine(row(i), filter, dest.row(i));
    }
}

/* perform a 1D convolution on entire image using vertical filter
 */
void ProcImage::convolveY(ProcKernel1D& filter, ProcImage& dest)
{
    assert(width() == dest.width() && height() == dest.height());
    
    for (int i = 0; i < width(); i++) {
        convolveLine(column(i), filter, dest.column(i));
    }
}

// --------------------------------------------------------------------

/* normalize kernel so it sums to one
 */
void ProcKernel1D::normalize()
{
    // Find sum of kernel
    double sum = kernel->sum();
    
    // Make sure not zero
    assert(sum != 0.0);
    
    // Normalize
    *kernel /= sum;
}

// --- Gaussian and DOG filters

ProcKernel1D* gaussianKernel1D(double sigma, double wid)
{
    assert(sigma > 0.0);

    // create kernel with radius including 3 stddev out
    ProcKernel1D *k = new ProcKernel1D((int)(wid*sigma + 0.5));

    // compute unnormalized Gaussian
    double denom = 2*sigma*sigma;
    (*k)[0] = 1.0;
    for (int i = 1; i <= k->rad(); i++) {
        (*k)[i] = (*k)[-i] = exp(-(double)i*i/denom);
    }
    
    // normalize kernel
    k->normalize();

//    cout << "Gauss " << sigma << endl;
    double sum = 0;
    for (i = -k->rad(); i <= k->rad(); i++) {
        sum += (*k)[i];
//        cout << (*k)[i] << " ";
    }
//    cout << "     " << sum << endl;

    return k;
}

ProcKernel1D* gaussianKernel1D(double sigma)
{
    return gaussianKernel1D(sigma, 3.0);
}

ProcKernel1D* diffGaussianKernel1D(double sigma, int d)
{
    // Start with normalized Gaussian
    ProcKernel1D *k = gaussianKernel1D(sigma, 3.0 + d * 0.5);
    double sigma2 = sigma*sigma, sigma4;
    int i;

    switch (d) {
      case 0:
        break;
      case 1:
        // First derivative of Gaussian
        for (i = -k->rad(); i <= k->rad(); i++)
          (*k)[i] *= -i/sigma2;
        break;
      case 2:
        // Second derivative of Gaussian
        sigma4 = sigma2 * sigma2;
        for (i = -k->rad(); i <= k->rad(); i++)
          (*k)[i] *= i*i/sigma4 - 1/sigma2;
        break;
      default:
        // Other derivatives undefined
        assert(0);
    }

    // Push rest of tail into end so diff filters sum to zero
    // XXX this is a hack, not sure what right way is, but it seemed
    //     necessary so filter doesn't respond to a constant image
    if (d >= 1) {
        double halfsum = 0.0;
        // Find sum of kernel
        for (i = -k->rad(); i <= k->rad(); i++) halfsum += (*k)[i];
        halfsum /= 2.0;

        (*k)[-k->rad()] -= halfsum;
        (*k)[ k->rad()] -= halfsum;
    }

//    cout << "Diff" << d << " " << sigma << endl;
    double sum = 0;
    for (i = -k->rad(); i <= k->rad(); i++) {
        sum += (*k)[i];
//        cout << (*k)[i] << " ";
    }
//    cout << "     " << sum << endl;
    
    return k;
}

// ------------------------------------------------------------------

// imageGradient
// Given blurring and derivative filters (blur and diff), compute gradient
// images for X and Y directions (dgx and dgy)
// (side effect: also computes temporary images blurred with 1D Gaussian
//  kernel (gx and gy) -- note: gx and gy can be the same image, in which
//  case gy is stored in the temporary)
// 
//              | Gx * I |   | dgx |
//    grad(I) = |        | = |     |    (where Gi is deriv of Gaussian in i)
//              | Gy * I |   | dgy |
//
void imageGradient(ProcKernel1D& blur, ProcKernel1D& diff,
                   ProcImage& src,
                   ProcImage& gx, ProcImage& gy,
                   ProcImage& dgx, ProcImage& dgy)
{
    assert(src.width()  == gx.width()   && src.width()  == gy.width()   &&
           src.width()  == dgx.width()  && src.width()  == dgy.width()  &&
           src.height() == gx.height()  && src.height() == gy.height()  &&
           src.height() == dgx.height() && src.height() == dgy.height());

    // blur with Gaussian in x direction
    src.convolveX(blur, gx);
    // y derivative (Gy)
    gx.convolveY(diff, dgy);

    // blur with Gaussian in y direction
    src.convolveY(blur, gy);
    // x derivative (Gx)
    gy.convolveX(diff, dgx);
}

// imageGradientHessian
// Given blurring and derivative filters (blur, diff and diff2), compute
// gradient (dgx and dgy) and Hessian images (Hxx, Hxy, Hyy)
// (side effect: also computes temporary images blurred with 1D Gaussian
//  kernel (gx and gy) and with 1D Gaussian derivative in x direction
//  -- note: gx, gy and dx can be the same image, in which case dx is
//  stored in the temporary)
// 
//              | Gxx * I    Gxy * I |    | Hxx  Hxy |
// Hessian(I) = |                    | =  |          |
//              | Gxy * I    Gyy * I |    | Hxy  Hyy |
//
void imageGradientHessian(ProcKernel1D& blur,
                          ProcKernel1D& diff, ProcKernel1D& diff2,
                          ProcImage& src,
                          ProcImage& gx,  ProcImage& gy, ProcImage& dx,
                          ProcImage& dgx, ProcImage& dgy,
                          ProcImage& Hxx, ProcImage& Hyy, ProcImage& Hxy)
{
    assert(src.width()  == gx.width()   && src.width()  == dgx.width()  &&
           src.width()  == gy.width()   && src.width()  == dgy.width()  &&
           src.width()  == dx.width()   && src.width()  == Hxy.width()  &&
           src.width()  == Hxx.width()  && src.width()  == Hyy.width()  &&
           src.height() == gx.height()  && src.height() == dgx.height() &&
           src.height() == gy.height()  && src.height() == dgy.height() &&
           src.height() == dx.height()  && src.height() == Hxy.height() &&
           src.height() == Hxx.height() && src.height() == Hyy.height());
    
    // blur with Gaussian in x direction
    src.convolveX(blur, gx);
    // y derivative (Gy)
    gx.convolveY(diff, dgy);
    // y second derivative (Hyy)
    gx.convolveY(diff2, Hyy);

    // blur with Gaussian in y direction
    src.convolveY(blur, gy);
    // x derivative (Gx)
    gy.convolveX(diff, dgx);
    // x second derivative (Hxx)
    gy.convolveX(diff2, Hxx);

    // x diff (no blur)
    src.convolveX(diff, dx);
    // xy mixed derivative (Hxy)
    dx.convolveY(diff, Hxy);
}

#endif
