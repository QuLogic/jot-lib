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
/*****************************************************************
 * image.H
 *****************************************************************/
#include "std/config.H"
#include "geom/image.H"

/*****************************************************************
 * utilities
 *****************************************************************/
static
inline
void
eat_whitespace(istream& in)
{
   while (isspace(in.peek())) {
      in.get();
   }
}

/******************************************************************
 * Image
 ******************************************************************/
Image::Image() :
   _width(0),
   _height(0),
   _bpp(0),
   _data(0),
   _no_delete(false) 
{
}

Image::Image(Cstr_ptr& file) :
   _width(0),
   _height(0),
   _bpp(0),
   _data(0),
   _no_delete(false) 
{
   if (file)
      load_file(**file);
}

Image::Image(uint w, uint h, uint bpp, uchar* data, bool nd) :
   _width(0),
   _height(0),
   _bpp(0),
   _data(0),
   _no_delete(false) 
{
   if (data)
      set(w,h,bpp,data,nd);
   else
      resize(w,h,bpp);
}

Image::Image(const Image& img) :
   _width(0),
   _height(0),
   _bpp(0),
   _data(0),
   _no_delete(false) 
{
   *this = img;
}

Image&
Image::operator=(const Image& img)
{
   if (&img != this)
      set(img.width(), img.height(), img.bpp(), img.data(), false);
   return *this;
}

void 
Image::set(int w, int h, uint bpp, uchar* data, bool nd) 
{
   if (bpp < 1 || bpp > 4) {
      err_msg("Image::Image: %d bytes/pixel not supported", bpp);
   } else {
      clear();
      _width = w;
      _height = h;
      _bpp = bpp;
      _data = data;
      _no_delete = nd;
   }
}

void 
Image::freedata() 
{
   if (!_no_delete) {
      delete [] _data;
   }
   _data = 0;
}
   
void 
Image::clear() 
{
   freedata();
   _width = _height = _bpp = 0;
   _no_delete = 0;
}

bool 
Image::resize(uint w, uint h, uint b) 
{
   // if current size is different from
   // desired size:
   //   let go current data (if any)
   //   set new width, height and bpp
   //   allocate new data
   //   but don't initialize it
   if (_width != w || _height != h || _bpp != b) {
      _width = w;
      _height = h;
      _bpp = b;
      if (!_no_delete)
         delete [] _data;
      _data = 0;
      _no_delete = 0;
      if ((_data = new uchar [ size() ]) == 0) {
         err_ret("Image::resize: can't allocate data");
         return 0;
      }
   }
   return 1;
}

uchar*
Image::copy()
{
   if (empty())
      return 0;

   uchar* ret = new uchar [ size() ];

   if (ret)     memcpy(ret, _data, size());
   else         err_ret("Image::copy: can't allocate data");

   return ret;
}

int
Image::resize_rows_mult_4()
{
   if (empty() || (row_size()%4) == 0)
      return 1;

   uint   new_w = _width + 4 - _width%4;
   uchar* new_d = 0;
   if ((new_d = new uchar [ new_w * _height * _bpp ]) == 0) {
      err_ret("Image::resize_rows_mult_4: can't allocate data");
      return 0;
   }
   int new_row_size = new_w*_bpp;
   for (uint r=0; r<_height; r++)
      memcpy(new_d + new_row_size*r, row(r), row_size());

   set(new_w, _height, _bpp, new_d);

   return 1;
}

int 
Image::copy_tile(const Image& tile, uint i, uint j)
{
   // scenario:
   //   this is a big image.
   //   tile is small enough that lots of copies of 
   //   it could be arranged to "tile" into this one.
   //   we'll copy data from tile into a slot of this image. 
   //   the slot is over i and up j copies of the tile image.
   //
   // how come why:
   //   used to perform hi-res screen grabs.
   //   the screen is rendered in separate tiles which
   //   are then arranged into a single hi-res image.

   // precondition: both images have the same bytes
   //   per pixel, and the designated slot fits in
   //   this image.

   if (tile._width*(i+1) > _width ||
       tile._height*(j+1) > _height ||
       tile._bpp != _bpp) {
      err_msg("Image::copy_tile: bad size in tiled image");
      return 0;
   }

   // copy each row into correct slot in this image:
   int row_offset = tile.row_size()*i;
   for (uint r=0; r<tile._height; r++) {
      memcpy(row(tile._height*j + r) + row_offset,      // destination
             tile.row(r),                               // source
             tile.row_size());                          // number of bytes
   }

   return 1;
}

/***********************************************************************
 * Method : Image::load_file
 * Params : const char* file
 * Returns: int (success/failure)
 * Effects: tries to load data from file
 ***********************************************************************/
int
Image::load_file(const char* file)
{
   if (!file || *file == '\0')
      return 0;

   // see if file can be opened before running off parsing...
   {
#if (defined (WIN32) && defined(_MSC_VER) && (_MSC_VER <=1300)) /*VS 6.0*/
      ifstream in(file, ios::in | ios::nocreate );
#else
      ifstream in(file);
#endif
      if (!in.good()) 
         {
            err_mesg(ERR_LEV_INFO | ERR_INCL_ERRNO, "Image::load_file() - ERROR! Can't open file %s", file);
            return 0;
         }
   }

   // try to read it as a png file
   if (read_png(file))
      return 1;

   // try to read it as a pnm file:
   if (read_pnm(file))
      return 1;

   // add other formats here...
   return 0;
}

int
Image::read_pnm(const char* file)
{
#if (defined (WIN32) && defined(_MSC_VER) && (_MSC_VER <=1200)) /*VS 6.0*/
   ifstream in(file, ios::in | ios::binary | ios::nocreate );
#else
   ifstream in(file);
#endif

   if (!in.good()) 
      {
         err_ret("Image::read_pnm() - ERROR! Can't open file %s", file);
         return 0;
      }

   char head[128];
   in >> head;

   if      (!strcmp(head, "P2"))        return read_pgm(in, 1);
   else if (!strcmp(head, "P5"))        return read_pgm(in, 0);
   else if (!strcmp(head, "P3"))        return read_ppm(in, 1);
   else if (!strcmp(head, "P6"))        return read_ppm(in, 0);
   else                                 return 0;
}

inline int
pnm_read_int(istream& in, uint* d)
{
   while (!in.eof() && !in.bad()) {
      char buf[128];
      in >> buf;
      if (buf[0] == '#')
         in.getline(buf, 128);
      else if (sscanf(buf, "%d", d) == 1)
         return 1;
   }
   return 0;
}


int
Image::read_pgm(istream& in, bool ascii)
{
   // reset and free up memory:
   clear();

   // read dimensions (skipping comments)
   pnm_read_int(in, &_width);
   pnm_read_int(in, &_height);
   uint max_val;
   pnm_read_int(in, &max_val);
   if (!ascii) eat_whitespace(in);

   // Some machines can't handle luminance (greyscale) textures,
   // so we load the greyscale image into RGB triplets:
   _bpp = 3;

   if (in.bad() || in.eof()) {
      err_ret("Image::read_pgm: error reading stream");
      clear();
      return 0;
   } else if ((_data = new uchar [ _width*_height*_bpp ]) == 0) {
      err_ret("Image::read_pgm: can't allocate data");
      clear();
      return 0;
   }
   _no_delete = 0;

   // read the image (invert vertically):
   uint row_bytes = row_size(), val=0;
   for (int y=_height-1; y>=0; y--) {
      uchar* row = _data + y*row_bytes;
      for (unsigned int x=0; x<_width; x++) {

         if (in.bad() || in.eof()) {
            err_ret("Image::read_pgm: error reading stream");
            clear();
            return 0;
         }

         if (ascii)     in >> val;
         else           val = in.get();

         *row++ = val;
         *row++ = val;
         *row++ = val;
      }
   }

   return 1;
}

/***********************************************************************
 * Method : Image::read_ppm
 * Params : istream& in, bool ascii
 * Returns: int (success/failure)
 * Effects: reads ppm (pixmap) file into memory
 ***********************************************************************/
int
Image::read_ppm(istream& in, bool ascii)
{
   // reset and free up memory:
   clear();

   // read dimensions (skipping comments)
   pnm_read_int(in, &_width);
   pnm_read_int(in, &_height);
   uint max_val;
   pnm_read_int(in, &max_val);
   if (!ascii) eat_whitespace(in);

   _bpp = 3;

   if (in.bad() || in.eof()) {
      err_ret("Image::read_ppm: error reading from stream");
      clear();
      return 0;
   } else if ((_data = new uchar [ size() ]) == 0) {
      err_ret("Image::read_ppm: can't allocate data");
      clear();
      return 0;
   }
   _no_delete = 0;

   // read the image (invert vertically):
   uint row_bytes = row_size(), val=0;
   for (int y=_height-1; y>=0; y--) {
      uchar* row = _data + y*row_bytes;

      if (in.bad() || in.eof()) {
         err_ret("Image::read_ppm: error reading from stream");
         clear();
         return 0;
      }

      if (!ascii) {
         in.read((char *) row, _width*3);
      } else {
         for (unsigned int x=0; x<_width; x++) {
            in >> val; *row++ = val;
            in >> val; *row++ = val;
            in >> val; *row++ = val;
         }
      }
   }

   return 1;
}

/***********************************************************************
 * Method : Image::write_pnm
 * Params : const char* file_name
 * Returns: int (success/failure)
 * Effects: writes pnm file to disk (not implemented)
 ***********************************************************************/
int
Image::write_pnm(const char* file) const
{
   if (_bpp != 3) {
      err_msg("Image::write_pnm: writing files w/o 3 BPP is unimplemented");
      return 0;
   }

   cerr << "WARNING: writing ppm file, but these colors appear to be off from"
        << endl;
   cerr << "         the ones shown in the JOT window.  Somehow there's error"
        << endl;
   cerr << "         being introduced, it appears.  This needs debugging."
        << endl;

   FILE* fp;
   fp = fopen(file, "wb");

   // write ppm RGB header
   fprintf(fp, "P6\n");

   // read dimensions (skipping comments)
   fprintf(fp, "%d %d\n", _width, _height);
   fprintf(fp, "255\n"); // (max_val always seems to be 255)

   // write data
   uint row_bytes = row_size();
   for(int y=_height-1;y>=0;y--)
      fwrite(_data + y * row_bytes, _bpp, _width, fp);

   fclose(fp);

   return 1;
}

/***********************************************************************
 * Method : Image::expand_power2
 * Params :
 * Returns: bool (true if anything changed)
 * Effects: resizes image so width and height are powers of 2.
 *          pixels outside old dimensions are assigned 0 value
 ***********************************************************************/
bool
Image::expand_power2()
{
   if (empty())
      return false;

   unsigned int w = getPower2Size(_width);
   unsigned int h = getPower2Size(_height);
   if (w == _width && h == _height)
      return false;

   uchar *data;
   if ((data = new uchar [ (w*h*_bpp) ]) == 0) {
      err_ret("Image::expand_power2: can't allocate data");
      return false;
   }
   memset(data, 0, w*h*_bpp);
   int old_row_size = row_size();
   int new_row_size = w * _bpp;
   for (uint y=0; y<_height; y++)
      memcpy(data  + y*new_row_size,
             _data + y*old_row_size,
             old_row_size);
   set(w,h,_bpp,data);
   return true;
}

#include "libpng/png.h"
static const unsigned int PNG_BYTES_TO_CHECK = 8;

/***********************************************************************
 * Method : Image::open_png
 * Params : const char* file_name
 * Returns: FILE* (null on failure)
 * Effects: opens given file and verifies it is a png file
 ***********************************************************************/
FILE*
Image::open_png(const char* file_name)
{
   // open the file and verify it is a PNG file:
   // Windows needs the following to be "rb", the b turns off
   // translating from DOS text to UNIX text
   FILE* fp = fopen(file_name, "rb");
   if (!fp) {
      err_ret("Image::open_png() - ERROR! Can't open file %s", file_name);
      return 0;
   }

   // Read in the signature bytes:
   unsigned char buf[PNG_BYTES_TO_CHECK];
   if (fread(buf, 1, PNG_BYTES_TO_CHECK, fp) != PNG_BYTES_TO_CHECK) {
      err_ret("Image::open_png: can't read file %s", file_name);
      return 0;
   }

   // Compare the first PNG_BYTES_TO_CHECK bytes of the signature:
   if (!png_sig_cmp(buf, 0, PNG_BYTES_TO_CHECK)) {
      // OK: it is a PNG file
      return fp;
   } else {
      // it is not a PNG file
      // close the file, return NULL:
      fclose(fp);
      return 0;
   }
}

/***********************************************************************
 * Method : Image::read_png
 * Params : const char* file_name
 * Returns: int (success/failure)
 * Effects: opens file, reads data into memory (if it is a png file)
 ***********************************************************************/
int
Image::read_png(const char* file)
{
   FILE *fp = open_png(file);
   if (!fp)
      return 0;

   int success = read_png(fp);
   fclose(fp);
   if (success)
      return 1;
   err_msg("Image::read_png: error reading file: %s", file);
   return 0;
}

int
Image::read_png(FILE* fp)
{
   bool debug=true;

   // reset and free up memory:
   clear();

   png_structp  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,0,0,0);
   if (!png_ptr) {
      err_msg("Image::read_png: png_create_read_struct() failed");
      return 0;
   }

   // Allocate/initialize the memory for image information
   png_infop info_ptr = png_create_info_struct(png_ptr);
   if (!info_ptr) {
      png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
      err_msg("Image::read_png: png_create_info_struct() failed");
      return 0;
   }

   // Set error handling
   if (setjmp(png_ptr->jmpbuf)) {
      // jump here from error encountered inside PNG code...
      // free all memory associated with the png_ptr and info_ptr
      png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
      err_adv(debug, "Image::read_png: error in PNG code, bailing out");
      return 0;
   }

   // Set up the input control (using standard C streams)
   //
   //   NB:
   //     aside from this file, all of jot uses iostreams.
   //     mixing the two I/O methods causes performance
   //     problems for low-level I/O buffering.
   //     we should look into setting up PNG to use
   //     iostreams.
   //
   png_init_io(png_ptr, fp);

   // indicate how much of the file has been read:
   png_set_sig_bytes(png_ptr, PNG_BYTES_TO_CHECK);

   // read information from the file before the first image data chunk
   png_read_info(png_ptr, info_ptr);

   // extract _width, _height, and other info from header:
   int bit_depth, color_type, interlace_type;
   png_get_IHDR(png_ptr, info_ptr,
                (png_uint_32*)&_width,
                (png_uint_32*)&_height,
                &bit_depth,
                &color_type,
                &interlace_type,
                NULL, NULL);

   // tell libpng to strip 16 bit/color files down to 8 bits/channel
   png_set_strip_16(png_ptr);

   // Extract multiple pixels with bit depths of 1, 2, and 4 from a single
   // byte into separate bytes (useful for paletted and grayscale images).
   png_set_packing(png_ptr);

   // Expand paletted colors into true RGB triplets
   if (color_type == PNG_COLOR_TYPE_PALETTE)
      png_set_expand(png_ptr);

   // Expand grayscale images to the full 8 bits from 1, 2, or 4 bits/pixel
   if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
      png_set_expand(png_ptr);

   // Expand paletted or RGB images with transparency to full alpha channels
   // so the data will be available as RGBA quartets.
   if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
      png_set_expand(png_ptr);

   // update the header to reflect transformations applied:
   png_read_update_info(png_ptr, info_ptr);

   // now it's safe to read the size of a row:
   unsigned long row_bytes = png_get_rowbytes(png_ptr, info_ptr);
   _bpp = row_bytes / _width;

   // make sure bytes per pixel is a valid number:
   if (_bpp < 1 || _bpp > 4) {
      err_msg("Image::read_png: %d bytes/pixel not supported", _bpp);
   } else if (interlace_type != PNG_INTERLACE_NONE) {
      err_msg("Image::read_png: unsupported interlace type (%d)",
              interlace_type);
   } else if ((_data = new uchar [ size() ]) == 0) {
      err_ret("Image::read_png: can't allocate data");
   } else {
      _no_delete = 0;

      // no more excuses: read the image (inverted vertically):
      for (int y=_height-1; y>=0; y--)
         png_read_row(png_ptr, _data + y*row_bytes, 0);

      // read rest of file, and get additional
      // chunks in info_ptr - REQUIRED
      png_read_end(png_ptr, info_ptr);
   }

   // clean up after the read, and free any memory allocated - REQUIRED
   png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);

   // return pixel data:
   if (_data) {
      return 1;
   } else {
      err_adv(debug, "Image::read_png: failed (data is null)");
      clear();
      return 0;
   }
}

/***********************************************************************
 * Method : Image::write_png
 * Params : const char* file_name
 * Returns: int (success/failure)
 * Effects: opens file, writes data to disk
 ***********************************************************************/
int
Image::write_png(const char* file) const
{
   if (_width == 0 || _height == 0 || _data == 0) {
      err_msg("Image::write_png: image has no data");
      return 0;
   } else if (_bpp < 1 || _bpp > 4) {
      err_msg("Image::write_png: unsupported number of bytes/pixel (%d)",
              _bpp);
      return 0;
   }

   FILE* fp;
   if ((fp = fopen(file, "wb")) == 0) {
      err_ret("Image::write_png: can't open file %s", file);
      return 0;
   }

   // Create and initialize the png_struct with the desired error handler
   // functions.  If you want to use the default stderr and longjump method,
   // you can supply NULL for the last three parameters.  We also check that
   // the library version is compatible with the one used at compile time,
   // in case we are using dynamically linked libraries.  REQUIRED.
   png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,0,0,0);
   if (!png_ptr) {
      fclose(fp);
      err_msg("Image::write_png: png_create_write_struct() failed");
      return 0;
   }

   // Allocate/initialize the image information data.  REQUIRED
   png_infop info_ptr = png_create_info_struct(png_ptr);
   if (!info_ptr) {
      fclose(fp);
      png_destroy_write_struct(&png_ptr,  (png_infopp)NULL);
      err_msg("Image::write_png: png_create_info_struct() failed");
      return 0;
   }

   // Set error handling
   if (setjmp(png_ptr->jmpbuf)) {
      // jump here from error encountered inside PNG code...
      // free all memory associated with the png_ptr and info_ptr
      fclose(fp);
      png_destroy_write_struct(&png_ptr,  (png_infopp)NULL);
      err_msg("Image::write_png: error writing file %s", file);
      return 0;
   }

   // Set up the input control (using standard C streams)
   //
   //   see note re: C streams in read_png() above
   //
   png_init_io(png_ptr, fp);

   // set the image information:
   png_set_IHDR(png_ptr,
                info_ptr,
                _width,
                _height,
                8,                              // bit depth
                ((_bpp==4) ? PNG_COLOR_TYPE_RGB_ALPHA :
                 (_bpp==3) ? PNG_COLOR_TYPE_RGB :
                 (_bpp==2) ? PNG_COLOR_TYPE_GRAY_ALPHA :
                 PNG_COLOR_TYPE_GRAY),
                PNG_INTERLACE_NONE,
                PNG_COMPRESSION_TYPE_BASE,
                PNG_FILTER_TYPE_BASE);

   // set gamma
   double gamma = Config::get_var_dbl("JOT_GAMMA",0.45,true);
   png_set_gAMA(png_ptr, info_ptr, gamma);
                
   // write the file header information.  REQUIRED
   png_write_info(png_ptr, info_ptr);

   // write the image data (inverted vertically):
   for (int y=_height-1; y>=0; y--)
      png_write_row(png_ptr, row(y));

   // It is REQUIRED to call this to finish writing
   png_write_end(png_ptr, info_ptr);

   // clean up after the write, and free any memory allocated
   png_destroy_write_struct(&png_ptr, (png_infopp)NULL);

   // close the file
   fclose(fp);

   return 1;
}

bool
Image::set_rgba(uint x, uint y, uint rgba)
{
   if (x >= _width || y >= _height) {
      return false;
   }

   uchar* a = addr(x,y);
   switch (_bpp) {
    case 4: a[3] = uchar(rgba_to_a(rgba));
    case 3: a[2] = uchar(rgba_to_b(rgba));
    case 2: a[1] = uchar(rgba_to_g(rgba));
    case 1: a[0] = uchar(rgba_to_r(rgba)); break;
    default: return false;
   }
   return true;
}
        
bool
Image::set_grey(uint x, uint y, uchar grey)
{
   if (x >= _width || y >= _height) {
      return false;
   }

   uchar* a = addr(x,y);
   switch (_bpp) {
    case 4: a[3] = 255;
    case 3: a[2] = grey;
    case 2: a[1] = grey;
    case 1: a[0] = grey; break;
    default: return false;
   }
   return true;
}
        
uint 
Image::pixel_rgba(uint x, uint y) const
{
   if (x >= _width || y >= _height)
      return 0;
   uchar* val = row(y);
   uint ret=0;
   switch(_bpp) {
    case 4:
      // R,G,B,A unsigned char's
      ret = build_rgba(val[x*_bpp], val[x*_bpp+1], val[x*_bpp+2], val[x*_bpp+3]);
      break;
    case 3:
      // R,G,B unsigned char's
      ret = build_rgba(val[x*_bpp], val[x*_bpp+1], val[x*_bpp+2]);
      break;
    case 2:
      // luminance/alpha unsigned char's
      ret = build_rgba(val[x*_bpp], val[x*_bpp], val[x*_bpp], val[x*_bpp+1]);
      break;
    case 1:
      // luminance unsigned char's
      ret = build_rgba(val[x*_bpp], val[x*_bpp], val[x*_bpp]);
      break;
    default:
      // it's not in there
      err_msg( "Image::pixel_rgba: unknown bits per pixel (%d) in image", _bpp);
      return 0;
   }    
   return ret;
}

double
Image::average_tone_dbl() const
{
   // Returns the average tone of the image RGB channels
   // as a double in the range [0..1]

   double ret = 0;
   for (uint y=0; y<_height; y++) {
      for (uint x=0; x<_width; x++) {
         ret += pixel_grey_dbl(x,y);
      }
   }
   if (_width > 0 && _height > 0)
      ret /= num_pixels();
   return ret;
}

void
Image::mult_by(double s)
{
   uchar* d = data();
   uint   n = num_pixels();
   for (uint i=0; i<n; i++) {
      d[i] = uchar(round(clamp(d[i]*s, 0.0, 255.0)));
   }
}

/**********************************************************************
 * PixelOP:
 *
 *  function object that applies some operation to an Image pixel.
 *    
 **********************************************************************/
/*
class PixelOP {
 public:
   virtual ~PixelOP() {}

   // apply the operation to the pixel for the given channel.
   // data is the address of the channel value, and channel
   // tells what channel
   virtual void apply(uchar* data, uint channel) = 0;
};

void
Image::apply_op(PixelOP& op)
{
   uchar* d = data();
   uint   n = num_pixels();
   for (uint i=0; i<n; i++) {
      op.apply(d[i], i%_bpp);
   }
}
//*/

// end of file image.C
