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
#ifndef DOTS_EX_H_IS_INCLUDED
#define DOTS_EX_H_IS_INCLUDED

#include "glsl_shader.hpp"

/**********************************************************************
 * DotsShader_EX:
 *
 *  Halftone shader based on dots
 *  Working in object space
 **********************************************************************/
class DotsShader_EX : public GLSLShader {
 public:
   //******** MANAGERS ********
   DotsShader_EX(Patch* patch = nullptr);

   virtual ~DotsShader_EX();

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("Dots_EX", DotsShader_EX*, GLSLShader, CDATA_ITEM*);

   //******** ACCESSORS ********
   void set_style(int s) { _style = s; }

   void set_tone_shader(GTexture* g);

   //******** GLSLShader VIRTUAL METHODS ********

   // base classes can over-ride this to maintain their own program
   // variable, e.g. to share a single program among all instances of
   // the derived class:
   virtual GLuint& program() { return _program; }
   virtual bool&  did_init() { return _did_init; }

   //uv gradients stuff
   virtual void set_patch(Patch* p);
   virtual void changed();

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
   virtual DATA_ITEM  *dup() const { return new DotsShader_EX; }

 protected:
   //******** Member Variables ********
   // shader used to draw the tone reference image:
   GTexture*    _tone_shader;

   // integer encoding what style parameters to use:
   int          _style;

   static GLuint _program;  // shared by all DotsShader_EX instances
   static bool   _did_init; // tells whether initialization attempt was made
   
  
   GLint			 _lod_loc;
   GLint        _proj_der_loc;
   
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
   virtual string vp_filename() { return vp_name("halftone_ex"); }

   // GLSL fragment shader name
   virtual string fp_filename() { return fp_name("dots_ex"); }
};

#endif // DOTS_EX_H_IS_INCLUDED
