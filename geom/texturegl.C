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
#include "disp/colors.H"
#include "gtex/gl_extensions.H"

#include "texturegl.H"
#include "appear.H"
#include "gl_view.H"

#include <GL/glu.h>

using mlib::Wvec;
using mlib::Wtransf;
using mlib::CWtransf;

static bool debug = Config::get_var_bool("DEBUG_TEXTURE",false);

TEXTUREgl::TEXTUREgl(Cstr_ptr& file, GLenum target, GLenum texture_unit) :
   TEXTURE(file),
   _dl(0),
   _dl_valid(false),
   _format(GL_RGBA),            // reset in load file
   _tex_unit(texture_unit),
   _save_img(false),
   _mipmap(false),
   _min_filter(GL_LINEAR),
   _max_filter(GL_LINEAR),
   _wrap_r(GL_REPEAT),
   _wrap_s(GL_REPEAT),
   _wrap_t(GL_REPEAT),
   _tex_fn(GL_MODULATE),
   _target(target)
{
}

TEXTUREgl::TEXTUREgl(CBBOX2D& bb, CVIEWptr& v) :
   TEXTURE(""),
   _dl(0),
   _dl_valid(false),
   _format(GL_RGBA),    // reset in load file
   _tex_unit(GL_TEXTURE0),
   _save_img(false),
   _mipmap(false),
   _min_filter(GL_LINEAR),
   _max_filter(GL_LINEAR),
   _wrap_r(GL_REPEAT),
   _wrap_s(GL_REPEAT),
   _wrap_t(GL_REPEAT),
   _tex_fn(GL_MODULATE),
   _target(GL_TEXTURE_2D) 
{
   copy_texture(bb, v);
}

/***********************************************************************
 * Method : TEXTUREgl::free_dl
 * Params : 
 * Returns: void
 * Effects: frees the texture object or display list
 ***********************************************************************/
void
TEXTUREgl::free_dl()
{
   if (_dl) {
      if (debug) {
         cerr << "TEXTUREgl::free_dl (file: \"" << file() << "\")" << endl;
      }
      glDeleteTextures(1, &_dl);
      _dl = 0;
      _dl_valid = false;
   }
}

void
TEXTUREgl::set_mipmap(bool mipmap)   
{
   // In OpenGL 2.0, non-power-of-2 textures are permitted, 
   // so we don't need to expand the image in that case.
   // BUT, this function can be called in static initialization,
   // before the connection to the OpenGL server is established.
   // So we can't query the version number as we would normally
   // do. Instead rely on an environment variable:
   static bool version2 = Config::get_var_bool("OPENGL2",false);

   if (mipmap != _mipmap) {
      // Changing state: free texture object and regenerate:
      free_dl();

      _mipmap = mipmap;

      set_expand_image(!version2);
   }
   if (_mipmap) {
      _min_filter = GL_LINEAR_MIPMAP_LINEAR;
   } else {
      // Using GL_LINEAR_MIPMAP_LINEAR when no mipmaps
      // are in effect results in broken texture maps:
      // AVOID THAT!
      _min_filter = GL_LINEAR;
   }
}

/***********************************************************************P5
 * Method : TEXTUREgl::load_image
 * Params : 
 * Returns: int (success/failure)
 * Effects: clear the texture object
 *          set image from data (not from file)
 ***********************************************************************/
int
TEXTUREgl::load_image()
{
   if (debug) {
      cerr << "TEXTUREgl::load_image: loading file: \""
           << file() << "\"" << endl;
   }
   free_dl();
   int ret = TEXTURE::load_image();
   _format = bpp_to_format();
   return ret;
}

/***********************************************************************P5
 * Method : TEXTUREgl::set_image
 * Params : unsigned char *data, int w, int h, uint bpp
 * Returns: int (success/failure)
 * Effects: clear the texture object
 *          set image from data (not from file)
 ***********************************************************************/
int
TEXTUREgl::set_image(unsigned char *data, int w, int h, uint bpp)
{
   if (debug) {
      cerr << "TEXTUREgl::set_image" << endl;
   }
   free_dl();
   int ret = TEXTURE::set_image(data,w,h,bpp);
   _format = bpp_to_format();
   return ret;
}

void 
TEXTUREgl::set_texture(Cstr_ptr& filename)
{
   // delete current texture (if any) and store the given filename
   // as the texture file to load next time we need the data:

   // do nothing if it's the same filename as before
   if (filename == _file)
      return;

   if (debug) {
      cerr << "TEXTUREgl::set_texture: old name: \""
           << _file
           << "\", new name: \""
           << filename << "\"" << endl;
   }

   // free the old texture
   free_dl();

   // store the filename to load when needed:
   _file = filename;
   _image_not_available = false; // becomes true if attempt to load fails
}

void
TEXTUREgl::declare_texture() 
{
   // allocate texture object
   // (no-op after first time):
   if (!_dl) {
      glGenTextures(1, &_dl);
      if (debug) GL_VIEW::print_gl_errors( " Texture GL :: declare_texture :: glGenTextures ");

      _dl_valid = (_dl > 0);
      if (_dl_valid) {
         glActiveTexture(_tex_unit);
         if (debug) GL_VIEW::print_gl_errors( " Texture GL :: declare_texture :: glActiveTexture ");
         
         glBindTexture(_target, _dl);
         if (debug) GL_VIEW::print_gl_errors( " Texture GL :: declare_texture :: glBindTexture ");
         
         if (debug) {
            cerr << "TEXTUREgl::declare_texture: succeeded (file: \""
                 << file() << "\")" << endl;
         }
      } else if (debug) {
         cerr << "TEXTUREgl::declare_texture: error: "
              << "glGenTextures() failed!" << endl
              << "  (TEXTUREgl using file: \""
              << file() << "\")" << endl;
      }
   }
}

/***********************************************************************
 * Method : TEXTUREgl::load_texture
 * Params : unsigned char **copy = 0
 * Returns: bool (success/failure)
 * Effects: if texture object has not been initialized:
 *              loads the image from file
 *              (makes a copy of the data on request)
 *              creates and binds texture object
 *              assigns image data to texture object
 *              frees the image data.
 ***********************************************************************/
bool
TEXTUREgl::load_texture(unsigned char **copy) 
{
   if (_target == GL_TEXTURE_CUBE_MAP) {
      return load_cube_map();
   }

   // If _dl is set and valid, do nothing
   if (_dl && _dl_valid)
      return 1;
   
   // if image is empty and we can't load it, give up:
   if (_img.empty() && !load_image()) {
      if (debug) {
         cerr << "TEXTUREgl::load_texture: error: can't load file: \""
              << file() << "\"" << endl;
      }
      return 0;
   }

   if (debug) {
      cerr << "TEXTUREgl::load_texture: loaded file: \""
           << file() << "\"" << endl;
   }
   
   // copy the image data on request:
   if (copy) {
      *copy = _img.copy();
   }

   // create and bind texture object:
   declare_texture();

   if (!is_valid()) {
      cerr << "TEXTUREgl::load_texture: error: can't generate texture" << endl;
      return false;
   }

   if (_target == GL_TEXTURE_2D) {
      if (_mipmap) {
         int ret = 
            gluBuild2DMipmaps(_target,
                              _img.bpp(),
                              _img.width(),
                              _img.height(),
                              format(),
                              GL_UNSIGNED_BYTE,
                              _img.data()
               );
         if (ret) {
            // nonzero return signals error:
            cerr << "TEXTUREgl::load_texture: error building mipmaps" << endl
                 << "  (texture file: " << file() << ")" << endl;
            GL_VIEW::print_gl_errors("TEXTUREgl::load_texture");
         }
      } else {
         glTexImage2D(_target,
                      0,                // mipmap level
                      _img.bpp(),
                      _img.width(),     // width
                      _img.height(),    // height
                      0,                // no border
                      format(),         // GL_RGB, GL_RGBA, etc.
                      GL_UNSIGNED_BYTE, // type of each color component
                      _img.data()       // image data
            );
      }
   } else {
      // XXX - assumes GL_TEXTURE1D, not handling GL_TEXTURE3D
      glTexImage1D(_target,
                   0,                // mipmap level
                   _img.bpp(),
                   _img.width(),     // width
                   0,                // no border
                   format(),         // GL_RGB, GL_RGBA, etc.
                   GL_UNSIGNED_BYTE, // type of each color component
                   _img.data()       // image data
         );
   }

   // the image is now saved in the texture object (or display list)
   // we can free the image data
   if (!_save_img)
      _img.freedata();
 
   return is_valid();
}

/***********************************************************************
 * TEXTUREgl::load_cube_map
 ***********************************************************************/
inline bool
load_cube_map_face(Cstr_ptr& fullpath, Image& img)
{
   if (img.load_file(**fullpath))
      return true;
   
   cerr << "load_cube_map_face: can't load " << fullpath << endl;
   return false;
}

inline bool
same_dims(const Image& a, const Image& b)
{
   return (a.width()  == b.width()  &&
           a.height() == b.height() &&
           a.bpp()    == b.bpp());
}

bool
TEXTUREgl::load_cube_map() 
{
   if (_target != GL_TEXTURE_CUBE_MAP) {
      return false;
   }

   // If _dl is set and valid, do nothing
   if (_dl && _dl_valid)
      return 1;

   if (debug) {
      cerr << "TEXTUREgl::load_cube_map: loading from directory: \""
           << file() << "\"" << endl;
   }

   // load the 6 images:
   str_ptr basepath = _file;
   Image right, left, front, back, top, bottom;
   if (!(load_cube_map_face(basepath + "/right.png", right) &&
         load_cube_map_face(basepath + "/left.png",  left)  &&
         load_cube_map_face(basepath + "/front.png", front) &&
         load_cube_map_face(basepath + "/back.png",  back)  &&
         load_cube_map_face(basepath + "/top.png",   top)   &&
         load_cube_map_face(basepath + "/bottom.png", bottom))) {
      _image_not_available = true;
      cerr << "TEXTUREgl::load_cube_map: can't load images" << endl;
      return false;
   }

   // check for compatible dimensions:
   if (!(same_dims(right, left)  &&
         same_dims(right, front) &&
         same_dims(right, back)  &&
         same_dims(right, top)   &&
         same_dims(right, bottom))) {
      cerr << "TEXTUREgl::load_cube_map: error: images have "
           << "different dimensions" << endl;
      _image_not_available = true;
      return false;
   }
   int w = right.width(), h = right.height(), bpp = right.bpp();
   if (!(bpp == 3 || bpp == 4)) {
      cerr << "TEXTUREgl::load_cube_map: error: unsupported format "
           << "(bpp == " << bpp << ")" << endl;
      _image_not_available = true;
      return false;
   }
   GLenum f = (bpp == 3) ? GL_RGB : GL_RGBA; // format

   // create and bind texture object:
   declare_texture();
   if (!is_valid()) {
      cerr << "TEXTUREgl::load_cube_map: error: declare texture failed"
           << endl;
      return false;
   }

   // load the data to texture memory:
   glTexImage2D(
      GL_TEXTURE_CUBE_MAP_POSITIVE_X,   // target
      0,                                // mipmap level
      bpp,                              // bytes per pixel
      w,                                // width
      h,                                // height
      0,                                // no border
      f,                                // GL_RGB, GL_RGBA, etc.
      GL_UNSIGNED_BYTE,                 // type of each color component
      right.data()                      // image data
      );
   glTexImage2D(
      GL_TEXTURE_CUBE_MAP_NEGATIVE_X,   // target
      0,                                // mipmap level
      bpp,                              // bytes per pixel
      w,                                // width
      h,                                // height
      0,                                // no border
      f,                                // GL_RGB, GL_RGBA, etc.
      GL_UNSIGNED_BYTE,                 // type of each color component
      left.data()                       // image data
      );
   glTexImage2D(
      GL_TEXTURE_CUBE_MAP_POSITIVE_Y,   // target
      0,                                // mipmap level
      bpp,                              // bytes per pixel
      w,                                // width
      h,                                // height
      0,                                // no border
      f,                                // GL_RGB, GL_RGBA, etc.
      GL_UNSIGNED_BYTE,                 // type of each color component
      top.data()                        // image data
      );
   glTexImage2D(
      GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,   // target
      0,                                // mipmap level
      bpp,                              // bytes per pixel
      w,                                // width
      h,                                // height
      0,                                // no border
      f,                                // GL_RGB, GL_RGBA, etc.
      GL_UNSIGNED_BYTE,                 // type of each color component
      bottom.data()                     // image data
      );
   glTexImage2D(
      GL_TEXTURE_CUBE_MAP_POSITIVE_Z,   // target
      0,                                // mipmap level
      bpp,                              // bytes per pixel
      w,                                // width
      h,                                // height
      0,                                // no border
      f,                                // GL_RGB, GL_RGBA, etc.
      GL_UNSIGNED_BYTE,                 // type of each color component
      back.data()                       // image data
      );
   glTexImage2D(
      GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,   // target
      0,                                // mipmap level
      bpp,                              // bytes per pixel
      w,                                // width
      h,                                // height
      0,                                // no border
      f,                                // GL_RGB, GL_RGBA, etc.
      GL_UNSIGNED_BYTE,                 // type of each color component
      front.data()                      // image data
      );

   // free the data in RAM:
   right .freedata();
   left  .freedata();
   front .freedata();
   back  .freedata();
   top   .freedata();
   bottom.freedata();

   return is_valid();
}

/***********************************************************************
 * Method : TEXTUREgl::apply_texture
 * Params : 
 * Returns: void
 * Effects: Make my texture the current texture
 ***********************************************************************/
void
TEXTUREgl::apply_texture(CWtransf *xf)
{
   // load image data into texture if needed:
   load_texture();

   // press on even if that failed
   // (some textures don't load from file...)

   // ensure we get a texture object:
   declare_texture(); // no-op if load texture worked; otherwise create one
   if (!is_valid()) { // ensure texture object is valid
      cerr << "TEXTUREgl::apply_texture: error: can't generate texture"
           << endl;
      return;
   }

   // Make the texture the current texture
   glActiveTexture(_tex_unit);

   glBindTexture(_target, _dl);



   // enable the texture:
   glEnable(_target);  // GL_ENABLE_BIT

   // set texture parameters
   glTexParameteri(_target, GL_TEXTURE_MIN_FILTER, _min_filter);
   glTexParameteri(_target, GL_TEXTURE_MAG_FILTER, _max_filter);
   glTexParameteri(_target, GL_TEXTURE_WRAP_R, _wrap_r);
   glTexParameteri(_target, GL_TEXTURE_WRAP_S, _wrap_s);
   glTexParameteri(_target, GL_TEXTURE_WRAP_T, _wrap_t);

   // XXX - should we set a border color?
   //       old code has been using white, alpha = 0
   glTexParameterfv(_target, GL_TEXTURE_BORDER_COLOR, float4(Color::white,0));

   // set texture function (GL_REPLACE, GL_DECAL, etc.)
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, _tex_fn);



  // The texture transfor mis only set when realy needed, this avoids stack overflow

   if ((_target != GL_TEXTURE_CUBE_MAP)&&( xf || (!(_scale==Wtransf::scaling(1.0,1.0,1.0)))  ))
   {
      // Set up the texture matrix
      glPushAttrib(GL_TRANSFORM_BIT); // save current matrix mode
      glMatrixMode(GL_TEXTURE);
      glLoadMatrixd((xf ? (_scale * (*xf)) : _scale).transpose().matrix());
      glPopAttrib();                  // restore previous matrix mode
   }

   
}

/***********************************************************************
 * Method : TEXTUREgl::copy_texture
 * Params : CBBOX2D& bb, CVIEWptr &v
 * Returns: void
 * Effects: Copies part of the framebuffer as a texture
 ***********************************************************************/
void
TEXTUREgl::copy_texture(CBBOX2D& bb, CVIEWptr &v) 
{
   int width, height;
   assert(_target == GL_TEXTURE_2D);
   v->get_size(width, height);
   int orig_width = (int) (bb.dim()[0] * width  / 2.0);
   int orig_height= (int) (bb.dim()[1] * height / 2.0);
   width  = getPower2Size(orig_width);
   height = getPower2Size(orig_height);
   
   _scale = Wtransf::scaling(Wvec((double)width/orig_width,
                                  (double)height/orig_height,
                                  1));
   glReadBuffer(GL_BACK);

   declare_texture();

   int x = int((bb.min()[0] + 1.0) * width  / 2.0); // X start
   int y = int((bb.min()[1] + 1.0) * height / 2.0); // Y start
   cerr << x << " " << y << " " << width << " " << height << "\n";
   glCopyTexImage2D(_target,
                    0,                                  // mip map level
                    GL_RGB,
                    x,
                    y,
                    width,
                    height,
                    0);                                 // no border

   _save_img = false;
   _img.clear();
}

// end of file texturegl.C
