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
 * HaloBlurShader.H:
 **********************************************************************/
#ifndef HALO_BLUR_SHADER_H_IS_INCLUDED
#define HALO_BLUR_SHADER_H_IS_INCLUDED

#include "glsl_shader.H"
#include "gtex/tone_shader.H"

/**********************************************************************
 * HaloBlurShader:
 *
 *  

 *  Note: 
 *  The reason why I duplicated this code is that I have an idea to do a 
 *  more radical blur effect
 *
 *  TODO,  use this to do blur for the HaloRefImage assembly      
 *  
 **********************************************************************/
class HaloBlurShader : public GLSLShader {
 public:
   //******** MANAGERS ********
   HaloBlurShader(Patch* patch = 0);

   virtual ~HaloBlurShader();

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("HaloBlurShader",
                        HaloBlurShader*, GLSLShader, CDATA_ITEM*);

   //******** ACCESSORS ********

   ToneShader*  get_tone_shader()   { return (ToneShader*)_patch->get_tex("ToneShader"); } 

   //******** STATICS ********

   static HaloBlurShader* get_instance();

   //******** BlurShader VIRTUAL METHODS ********

   // using a static variable for program, so all toon shader
   // instances share the same program:
   virtual GLuint& program() { return _program; }
   virtual bool&  did_init() { return _did_init; }

   // Called in init(); query and store the "locations" of
   // uniform and attribute variables here:
   virtual bool get_variable_locs();

   // Send values of uniform variables to the shader:
   virtual bool set_uniform_variables() const;

   // Calls glPushAttrib() and sets some OpenGL state:
   virtual void set_gl_state(GLbitfield mask=0) const;

  // ******** GTexture VIRTUAL METHODS ********

  //********** SPECIAL STUFF FOR FILTER TEXTURE ******

   virtual int  draw(CVIEWptr& v); //draws a quad instead
   void set_input_tex(TEXTUREglptr input) {_input_tex = input; };
   void set_kernel_size(int filter_size) {_kernel_size = filter_size; };
   int  get_kernel_size() { return _kernel_size;};

   //******** DATA_ITEM VIRTUAL METHODS ********
   virtual DATA_ITEM  *dup() const { return new HaloBlurShader; };

 

 protected:
   //******** Member Variables ********
   // shader used to draw the tone reference image:

   


   static GLuint _program;  // program shared by all toon instances
   static bool   _did_init; // tells whether initialization attempt was made

   static HaloBlurShader* _instance;

   //input image variables:
   GLint _input_tex_loc;
   GLint _width_loc;
   GLint _height_loc;
   GLint _direction_loc;
   GLint _kernel_size_loc;
   GLint _kernel_size;

   TEXTUREglptr _input_tex;
  

   //******** VIRTUAL METHODS ********

   // Return the names of the shader programs:
   virtual str_ptr vp_filename() { return vp_name("halo_blur"); }

   virtual str_ptr fp_filename() { return fp_name("halo_blur"); }

};

#endif // 

// end of file halo_blur_shader.H
