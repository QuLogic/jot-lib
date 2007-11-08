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
 * glsl_halo.H:
 **********************************************************************/
#ifndef GLSL_HALO_H_IS_INCLUDED
#define GLSL_HALO_H_IS_INCLUDED

#include "glsl_toon.H"

/**********************************************************************
 * GLSLHaloShader:
 *
 *  GLSL version of a 1D halo shader.

 *  Note: Current version uses GL_TEXTURE2D, but works fine
 *        if you load an image of size 1 x n.
 *
 **********************************************************************/
class GLSLHaloShader : public GLSLToonShader {
 public:
   //******** MANAGERS ********
   GLSLHaloShader(Patch* patch = 0);

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("GLSL Halo2",
                        GLSLHaloShader*, GLSLToonShader, CDATA_ITEM*);

   //******** STATICS ********

   static GLSLHaloShader* get_instance();

   //******** GLSLShader VIRTUAL METHODS ********

   // using a static variable for program, so all toon shader
   // instances share the same program:
   virtual GLuint& program() { return _program; }
   virtual bool&  did_init() { return _did_init; }

   // Called in init(); query and store the "locations" of
   // uniform and attribute variables here:
   virtual bool get_variable_locs();

   // Send values of uniform variables to the shader; needed for VIEWFORM
   // uniform variable
   virtual bool set_uniform_variables() const;

   virtual void set_gl_state(GLbitfield mask=0) const;


   //******** DATA_ITEM VIRTUAL METHODS ********
   virtual DATA_ITEM  *dup() const { return new GLSLHaloShader; }

 protected:
   //******** Member Variables ********

   
   static GLint  _pixel_width_loc;
   //static GLint  _distance_loc;
   static GLuint _program;  // GLSL program shared by all toon instances
   static bool   _did_init; // tells whether initialization attempt was made

   static GLSLHaloShader* _instance;

   //******** VIRTUAL METHODS ********

   // Return the names of the toon GLSL shader programs:
   virtual str_ptr vp_filename() { return vp_name("halo2"); }

   // XXX - temporary, until we can figure out how to not use the
   //       fragment shader:
   virtual str_ptr fp_filename() { return fp_name("halo2"); }

};

#endif // GLSL_HALO_H_IS_INCLUDED

// end of file glsl_halo.H
