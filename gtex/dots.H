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
 * dots.H:
 **********************************************************************/
#ifndef DOTS_H_IS_INCLUDED
#define DOTS_H_IS_INCLUDED

#include "glsl_shader.H"

/**********************************************************************
 * DotsShader:
 *
 *  Halftone shader based on dots
 *
 **********************************************************************/
class DotsShader : public GLSLShader {
 public:
   //******** MANAGERS ********
   DotsShader(Patch* patch = 0);

   virtual ~DotsShader();

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("Dots", DotsShader*, GLSLShader, CDATA_ITEM*);

   //******** ACCESSORS ********
   void set_style(int s) { _style = s; }

   void set_tone_shader(GTexture* g);

   //******** GLSLShader VIRTUAL METHODS ********

   // base classes can over-ride this to maintain their own program
   // variable, e.g. to share a single program among all instances of
   // the derived class:
   virtual GLuint& program() { return _program; }
   virtual bool&  did_init() { return _did_init; }

   // lookup uniform and attribute variable "locations":
   virtual bool get_variable_locs();

   // send values to uniform variables:
   virtual bool set_uniform_variables() const;

   // Calls glPushAttrib() and sets some OpenGL state:
   virtual void set_gl_state(GLbitfield mask=0) const;

   // Returns a list of the slave gtextures of this class
   virtual GTexture_list gtextures() const;

  // ******** GTexture VIRTUAL METHODS ********
   virtual void request_ref_imgs();
   virtual int  draw_color_ref(int i);

   //******** DATA_ITEM VIRTUAL METHODS ********
   virtual DATA_ITEM  *dup() const { return new DotsShader; }

 protected:
   //******** Member Variables ********
   // shader used to draw the tone reference image:
   GTexture*    _tone_shader;

   // integer encoding what style parameters to use:
   int          _style;

   static GLuint _program;  // shared by all DotsShader instances
   static bool   _did_init; // tells whether initialization attempt was made

   // dynamic 2D pattern variables:
   static GLint _origin_loc;
   static GLint _u_vec_loc;
   static GLint _v_vec_loc;
   static GLint _st_loc;

   // tone map variables:
   static GLint _tone_tex_loc;
   static GLint _width_loc;
   static GLint _height_loc;

   static GLint _style_loc;  

   //******** VIRTUAL METHODS ********

   // use the same vertex program as hatching shader:
   virtual str_ptr vp_filename() { return vp_name("halftone-dots"); }

   // GLSL fragment shader name
   virtual str_ptr fp_filename() { return fp_name("halftone-dots"); }
};

#endif // DOTS_H_IS_INCLUDED

// end of file dots.H
