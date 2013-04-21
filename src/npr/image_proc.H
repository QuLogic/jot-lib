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
#ifndef IMAGEPROC_HEADER

#define IMAGEPROC_HEADER


// Linux has problems using STL stuff in jot
#ifndef linux
// (comment this #define out and recompile before checking in to make sure
//  you #ifdef'd all references to the classes in this file!)

// XXX - Actually, let's just ALWAYS axe this...
// #define USE_IMAGEPROC

#endif



#ifdef USE_IMAGEPROC



// Image processing stuff

// --- Doug DeCarlo 1/03



#include "gtex/ref_image.H"

#include <valarray>

#include <cstddef>



// Enable bounds checking on images and kernels

//#define IMAGEPROC_DEBUG



// -----------------------------------------------------------------------

// -- slice_access class: holds a valarray and a slice

//  Used for accessing rows and columns in a ProcImage

//  (This is necessary because slice_arrays can't be copied)

// Supports size() and [] operations

template<class T> class slice_access {

    std::valarray<T>* v;

    std::slice s;



    T& ref(size_t i) const {

#ifdef IMAGEPROC_DEBUG

        assert(s.start()+i*s.stride() >= 0 &&

               s.start()+i*s.stride() < v->size());

#endif

        return (*v)[s.start()+i*s.stride()];

    }



  public:

    slice_access(std::valarray<T>* vv, std::slice ss) : v(vv), s(ss) { }

    size_t size() const { return s.size(); }

    T& operator[](size_t i) { return ref(i); }

};

// for accessing const ProcImages

template<class T> class Cslice_access {

    std::valarray<T>* v;

    std::slice s;



    T& ref(size_t i) const {

#ifdef IMAGEPROC_DEBUG

        assert(s.start()+i*s.stride() >= 0 &&

               s.start()+i*s.stride() < v->size());

#endif

        return (*v)[s.start()+i*s.stride()];

    }



  public:

    Cslice_access(std::valarray<T>* vv, std::slice ss) :v(vv), s(ss) { }

    size_t size() const { return s.size(); }

    const T& operator[](size_t i) const { return ref(i); }

};



// -----------------------------------------------------------------------

// -- ProcKernel1D class for 1D convolution kernels centered around zero

class ProcKernel1D

{

  public:

    /* Constructor for zero kernel of particular size

     * 

     * Creates kernel of "radius" r:  [-r, .., 0, ..., r]

     * containing all zeros

     */

    ProcKernel1D(int r)

    {

        rad_ = r;

        kernel = new std::valarray<double>(2*r+1);

    }

    ~ProcKernel1D()

    {

        delete kernel;

    }

    

    /* size of kernel

     */

    int rad()  const { return rad_; }

    int size() const { return kernel->size(); }

    

    /* individual element access

     */

    double  operator[](int index) const {

#ifdef IMAGEPROC_DEBUG

        assert(index + rad_ >= 0 && index + rad_ < kernel->size());

#endif

        return (*kernel)[index + rad_];

    }

    double& operator[](int index) {

#ifdef IMAGEPROC_DEBUG

        assert(index + rad_ >= 0 && index + rad_ < kernel->size());

#endif

        return (*kernel)[index + rad_]; 

    }

    

    /* normalize kernel

     */

    void normalize();

    

  private:

    // Radius of kernel

    int rad_;

    // Values of kernel in [0..2*rad] (length is 2*rad+1)

    std::valarray<double> *kernel;

};



// --- One line in the image

typedef slice_access<double>& ProcImageLine;



// --- ProcImage class for image processing single-channel images of doubles

class ProcImage

{

  public:

    // Constructor for empty image

    ProcImage(size_t w, size_t h)

    {

        width_ = w;

        height_ = h;

        image_ = new std::valarray<double>(width_ * height_);

    }

    // Constructor from refimage

    ProcImage(RefImage *refimg)

    {

        width_ = refimg->width();

        height_ = refimg->height();

        image_ = new std::valarray<double>(width_ * height_);



        fromRefImage(refimg);

    }

    ~ProcImage()

    {

        delete image_;

    }



    // -----



    /* Compare sizes of image to a refimage

     */

    bool resizeNeeded(RefImage *i)

    { 

        return width() != i->width() || height() != i->height();

    }



    /* Copy contents from ref image (from red channel)

     */

    void fromRefImage(RefImage *refimg)

    {

        int p = 0;



        for (int x = 0; x < width_; x++) {

            for (int y = 0; y < height_; y++) {

                (*image_)[p++] = rgba_to_r_d(refimg->val(x, y));

            }

        }

    }



    /* Put back in RefImage 

     */

    void toRefImage(RefImage *refimg)

    {

        int p = 0;

        for (int x = 0; x < width_; x++) {

            for (int y = 0; y < height_; y++) {

                double dpix = (*image_)[p++];

                if (dpix < 0) dpix = 0;

                if (dpix > 1) dpix = 1;

                uint pix = (uint)(dpix * 255 + 0.5);

                refimg->val(x,y) = build_rgba(pix, pix, pix);

            }

        }

    }



    /* Resize image (losing contents)

     */

    void resize(size_t w, size_t h)

    {

        width_ = w;

        height_ = h;

        image_->resize(width_ * height_);

    }

    /* Resize and also copy from refimg

     */

    void resize(RefImage *refimg)

    {

        resize(refimg->width(), refimg->height());

        fromRefImage(refimg);

    }



    // -----

    

    // Number of pixels

    size_t size()   const { return width_ * height_; }

    // Number of columns

    size_t width()  const { return width_; }

    // Number of rows

    size_t height() const { return height_; }

    

    /* Access to rows and columns of image

     */

    slice_access<double>  row(size_t i)

    {

        return slice_access<double>(image_, std::slice(i,width_,height_));

    }

    Cslice_access<double> row(size_t i) const

    {

        return Cslice_access<double>(image_, std::slice(i,width_,height_));

    }

    slice_access<double>  column(size_t i)

    {

        return slice_access<double>(image_, std::slice(i*height_,height_,1));

    }

    Cslice_access<double> column(size_t i) const

    {

        return Cslice_access<double>(image_, std::slice(i*height_,height_,1));

    }

    

    slice_access<double>  operator[](size_t c)       { return column(c); }

    Cslice_access<double> operator[](size_t c) const { return column(c); }



    /* Pixel access (column, row)

     */

    double& operator()(size_t c, size_t r)       { return column(c)[r]; }

    double  operator()(size_t c, size_t r) const { return column(c)[r]; }



    /* Accessor for image data

     */

    std::valarray<double>& image() const { return *image_; }



    void convolveLine(ProcImageLine simg, ProcKernel1D& filter,

                      ProcImageLine dimg);

    void convolveX(ProcKernel1D& filter, ProcImage& dest);

    void convolveY(ProcKernel1D& filter, ProcImage& dest);

    

  private:

    // image data (by column)

    std::valarray<double> *image_;

    // width and height of image

    size_t width_, height_;

};



// Typical kernels

ProcKernel1D* gaussianKernel1D(double sigma);

ProcKernel1D* diffGaussianKernel1D(double sigma, int d);



// Derivative images

void imageGradient(ProcKernel1D& blur, ProcKernel1D& diff,

                   ProcImage& src,

                   ProcImage& gx, ProcImage& gy,

                   ProcImage& dgx, ProcImage& dgy);

void imageGradientHessian(ProcKernel1D& blur,

                          ProcKernel1D& diff, ProcKernel1D& diff2,

                          ProcImage& src,

                          ProcImage& gx,  ProcImage& gy, ProcImage& dx,

                          ProcImage& dgx, ProcImage& dgy,

                          ProcImage& Hxx, ProcImage& Hyy, ProcImage& Hxy);



#endif // USE_IMAGEPROC



#endif // IMAGEPROC_HEADER

