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
 * msld.H:
 **********************************************************************/
#ifndef MSLD_H_IS_INCLUDED
#define MSLD_H_IS_INCLUDED

#include "gtex/glsl_shader.H"

/**********************************************************************
 * MSLDShader:
 *
 *  ...

 *  Note: 
 *       
 *
 **********************************************************************/
class MSLDShader : public GLSLShader {
 public:
   //******** MANAGERS ********
   MSLDShader(Patch* patch = 0);

   virtual ~MSLDShader();

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("MSLD",
                        MSLDShader*, GLSLShader, CDATA_ITEM*);

   //******** ACCESSORS ********
   void set_tone_shader(GTexture* g);

   //******** STATICS ********

   static MSLDShader* get_instance();

   //******** MSLDShader VIRTUAL METHODS ********

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

   // Returns a list of the slave gtextures of this class
   virtual GTexture_list gtextures() const;

  // ******** GTexture VIRTUAL METHODS ********
   virtual void request_ref_imgs();
   virtual int  draw_color_ref(int i);

   //******** DATA_ITEM VIRTUAL METHODS ********
   virtual DATA_ITEM  *dup() const { return new MSLDShader; }

 protected:
   //******** Member Variables ********
   // shader used to draw the tone reference image:
   GTexture*    _tone_shader;

   static GLuint _program;  // MSLD program shared by all toon instances
   static bool   _did_init; // tells whether initialization attempt was made

   static MSLDShader* _instance;

//   GLint scthresh_uniform_loc;
 //  GLint feature_size_uniform_loc;

   // tone map variables:
   GLint _tone_tex_loc;
   GLint _width_loc;
   GLint _height_loc;

   //******** VIRTUAL METHODS ********

   // Return the names of the toon MSLD shader programs:
   virtual str_ptr vp_filename() { return vp_name("msld"); }

   virtual str_ptr fp_filename() { return fp_name("msld"); }

};

#endif // MSLD_H_IS_INCLUDED

// end of file msld.H
