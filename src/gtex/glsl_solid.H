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
 * glsl_toon.H:
 **********************************************************************/
#ifndef GLSL_SOLID_H_IS_INCLUDED
#define GLSL_SOLID_H_IS_INCLUDED

#include "glsl_paper.H"

/**********************************************************************
 * GLSLSolidShader:
 *
 *  GLSL version of a solid shader.
 *
 **********************************************************************/
class GLSLSolidShader : public GLSLPaperShader {
 public:
   //******** MANAGERS ********
   GLSLSolidShader(Patch* patch = 0);

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("GLSL Solid",
                        GLSLSolidShader*, BasicTexture, CDATA_ITEM*);

   //******** GLSLShader VIRTUAL METHODS ********

   // using a static variable for program, so all toon shader
   // instances share the same program:
   virtual GLuint& program() { return _program; }
   virtual bool&  did_init() { return _did_init; }

   //******** DATA_ITEM VIRTUAL METHODS ********
   virtual DATA_ITEM  *dup() const { return new GLSLSolidShader; }

 protected:
  
   static GLuint _program;  // GLSL program shared by all instances
   static bool   _did_init; // tells whether initialization attempt was made

   //******** VIRTUAL METHODS ********
   // Return the names of the toon GLSL shader programs:
   virtual str_ptr vp_filename() { return vp_name("paper"); }

   // XXX - temporary, until we can figure out how to not use the
   //       fragment shader:
   virtual str_ptr fp_filename() { return fp_name("solid"); }
 
};

#endif // GLSL_SOLID_H_IS_INCLUDED

// end of file glsl_solid.H
