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
/**********************************************************************
 * texturegl.H
 **********************************************************************/
#ifndef __TEXTUREGL_H
#define __TEXTUREGL_H

#include "std/support.H"
#include "glew/glew.H" // must come first

#include "texture.H"
#include "disp/view.H"

/**********************************************************************
 * TEXTUREgl:
 **********************************************************************/
MAKE_PTR_SUBC(TEXTUREgl, TEXTURE);
typedef const TEXTUREglptr CTEXTUREglptr;
class TEXTUREgl : public TEXTURE {
 public:
   TEXTUREgl(Cstr_ptr& file         = "",
             GLenum    target       = GL_TEXTURE_2D,
             GLenum    texture_unit = GL_TEXTURE0);
   TEXTUREgl(CBBOX2D& bb, CVIEWptr &v);

   virtual ~TEXTUREgl() { free_dl(); }

   mlib::Point2i  get_size() const { return _img.dims(); }

   virtual int  load_image();
   virtual int  set_image(unsigned char *data, int w, int h, uint bpp=3);

   // delete current texture (if any) and store the given filename
   // as the texture file to load next time we need the data:
   void set_texture(Cstr_ptr& filename);

   GLuint     get_tex_id()      const   { return _dl; }   
   bool       is_valid()        const   { return _dl_valid; }

   virtual bool load_texture(unsigned char **copy = 0);
   virtual bool load_cube_map();

   // Activates the texture and calls glEnable() to enable texturing.
   // NOTE: this should be called between glPushAttrib(GL_ENABLE_BIT)
   // and glPopAttrib()
   virtual void apply_texture(const mlib::Wtransf *xf = 0);
        
   void copy_texture(CBBOX2D &bb, CVIEWptr &v);

   void   set_format(GLenum format) { _format = format; }
   GLenum format    () const        { return _format; }

   // set or get texture unit (GL_TEXTURE0, GL_TEXTURE1, etc.):
   void   set_tex_unit(GLenum unit) { _tex_unit = unit; }
   GLenum get_tex_unit() const      { return _tex_unit; }

   // returns '0' for GL_TEXTURE0, '1' for GL_TEXTURE1, etc.:
   GLint  get_raw_unit() const { return get_tex_unit() - GL_TEXTURE0; }

   void    set_tex_fn(GLint   fn)    { _tex_fn = fn; }
   GLint   tex_fn    () const        { return _tex_fn; }
   
   void    set_wrap_r(GLint wrapr)   { _wrap_s = wrapr;}
   GLint   wrap_r    () const        { return _wrap_r;}
   void    set_wrap_s(GLint wraps)   { _wrap_s = wraps;}
   GLint   wrap_s    () const        { return _wrap_s;}
   void    set_wrap_t(GLint wrapt)   { _wrap_t = wrapt;}
   GLint   wrap_t    () const        { return _wrap_t;}

   void    set_min_filter(GLint minfilt) { _min_filter = minfilt;}
   void    set_max_filter(GLint maxfilt) { _max_filter = maxfilt;}

   bool    mipmap    () const        { return _mipmap;}
   void    set_mipmap(bool mipmap);

   void    set_save_img(bool b = 1) { _save_img = b; }
   const   GLenum  target() const {return _target; }
   
   void declare_texture();
 
protected:
   GLuint       _dl;        // texture object (was display list in OGL 1.1)
   bool         _dl_valid;  // _dl is valid
   GLenum       _format;    // GL_RGB, GL_RGBA, etc.
   GLenum       _tex_unit;  // GL_TEXTURE0 by default
   bool         _save_img;  // whether to free image data after loading
   bool         _mipmap;    // Mipmap or not
   GLint        _min_filter;
   GLint        _max_filter;
   GLint        _wrap_r;
   GLint        _wrap_s;
   GLint        _wrap_t;
   GLint        _tex_fn;    // texture function (GL_DECAL etc.)
   const GLenum _target;    // GL_TEXTURE_2D, GL_TEXTURE_3D, or
                            // GL_TEXTURE_CUBE_MAP

   GLenum bpp_to_format() const {
      return ((_img.bpp()==4) ? GL_RGBA :
              (_img.bpp()==3) ? GL_RGB  :
              (_img.bpp()==2) ? GL_LUMINANCE_ALPHA :
              GL_LUMINANCE
         );
   }

   void free_dl();
};

#endif // __TEXTUREGL_H

// end of file texturegl.H
