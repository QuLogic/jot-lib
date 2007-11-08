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
#ifndef GLSL_NORMAL_H_IS_INCLUDED
#define GLSL_NORMAL_H_IS_INCLUDED

#include "glsl_shader.H"

/**********************************************************************
 * GLSLNormalShader:
 *
 *  ...

 *  Note: 
 *       
 *
 **********************************************************************/
class GLSLNormalShader : public GLSLShader {
 public:
   //******** MANAGERS ********
   GLSLNormalShader(Patch* patch = 0);

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("GLSL_NORMAL",
                        GLSLNormalShader*, GLSLShader, CDATA_ITEM*);

   //******** STATICS ********

   static GLSLNormalShader* get_instance();

   //******** GLSLNormalShader VIRTUAL METHODS ********

   // using a static variable for program, so all toon shader
   // instances share the same program:
   virtual GLuint& program() { return _program; }
   virtual bool&  did_init() { return _did_init; }

   // Called in init(); query and store the "locations" of
   // uniform and attribute variables here:
   virtual bool get_variable_locs();

   // Send values of uniform variables to the shader:
   virtual bool set_uniform_variables() const;

   //******** DATA_ITEM VIRTUAL METHODS ********
   virtual DATA_ITEM  *dup() const { return new GLSLNormalShader; }

 protected:
   //******** Member Variables ********
   // shader used to draw the tone reference image:

   static GLuint _program;  // MSLD program shared by all toon instances
   static bool   _did_init; // tells whether initialization attempt was made

   static GLSLNormalShader* _instance;

   //******** VIRTUAL METHODS ********

   // Return the names of the toon MSLD shader programs:
   virtual str_ptr vp_filename() { return vp_name("normal"); }

   virtual str_ptr fp_filename() { return fp_name("normal"); }

};

#endif // GLSL_NORMAL_H_IS_INCLUDED

// end of file glsl_normal.H
