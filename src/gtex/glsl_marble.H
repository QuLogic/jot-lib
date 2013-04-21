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
* glsl_marble.H:
**********************************************************************/
#ifndef GLSL_MARBLE_H_IS_INCLUDED
#define GLSL_MARBLE_H_IS_INCLUDED

#include "glsl_shader.H"

/**********************************************************************
* GLSLMarbleShader:
*
* GLSL Marble shader, it is meant to 
* fully test the perlin noise functionality
*
**********************************************************************/
class GLSLMarbleShader : public GLSLShader {
public:
   //******** MANAGERS ********
   GLSLMarbleShader(Patch* patch = 0);

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("GLSL Marble",
      GLSLMarbleShader*, BasicTexture, CDATA_ITEM*);

   // Set the name of the texture to use (full path):
   void set_tex(Cstr_ptr& full_path_name);

   //******** GLSLShader VIRTUAL METHODS ********

   // Called in init(); query and store the "locations" of
   // uniform and attribute variables here:
   virtual bool get_variable_locs();

   // Send values of uniform variables to the shader:
   virtual bool set_uniform_variables() const;

   // Init the 1D toon texture by loading from file:
   virtual void init_textures();

   // Activate the 1D toon texture for drawing:
   virtual void activate_textures();

   //******** DATA_ITEM VIRTUAL METHODS ********
   virtual DATA_ITEM  *dup() const { return new GLSLMarbleShader; }

protected:
   //******** Member Variables ********
   TEXTUREglptr _tex; 
   TEXTUREglptr _perlin;                          // the texture
   GLint        _tex_loc;       // "location" of sampler2D in the program
   GLint        _perlin_loc;    // "location" of the 
   Perlin* _perlin_generator; //pointer to an instance of the 
                                     //perlin texture generator

   //******** VIRTUAL METHODS ********

   // Return the names of the toon GLSL shader programs:
   virtual str_ptr vp_filename() { return vp_name("marble"); }

   // XXX - temporary, until we can figure out how to not use the
   //       fragment shader:
   virtual str_ptr fp_filename() { return fp_name("marble"); }

   // we're not using any fragment shader:
   // XXX - we have to figure out how to not use the fragment shader
   //   virtual str_list fp_filenames() { return str_list(); }
};

#endif // GLSL_MARBLE_H_IS_INCLUDED

// end of file glsl_marble.H
