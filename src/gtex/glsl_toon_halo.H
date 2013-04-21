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
 * glsl_toon_halo.H:
 **********************************************************************/
#ifndef GLSL_TOON_HALO_H_IS_INCLUDED
#define GLSL_TOON_HALO_H_IS_INCLUDED

#include "glsl_shader.H"

/**********************************************************************
 * GLSLToonShaderHalo:
 *
 *  GLSL version of a 1D toon shader.

 *  Note: Current version uses GL_TEXTURE2D, but works fine
 *        if you load an image of size 1 x n.

 *  This version of the toon shader is meant for drawing halos
 *  It does not use display lists
 *
 **********************************************************************/
class GLSLToonShaderHalo : public GLSLShader {
 public:
   //******** MANAGERS ********
   GLSLToonShaderHalo(Patch* patch = 0);

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("GLSL Toon Halo",
                        GLSLToonShaderHalo*, BasicTexture, CDATA_ITEM*);

   // Set the name of the texture to use (full path):
   void set_tex(Cstr_ptr& full_path_name);

   //******** GLSLShader VIRTUAL METHODS ********

   // Called in init(), subclasses can query and store
   // the "locations" of uniform and attribute variables here:
   virtual bool get_variable_locs();

   // Send values of uniform variables to the shader:
   virtual bool set_uniform_variables() const;

   // Init the 1D toon texture by loading from file:
   virtual void init_textures();

   // Activate the 1D toon texture for drawing:
   virtual void activate_textures();

   //******** DATA_ITEM VIRTUAL METHODS ********
   virtual DATA_ITEM  *dup() const { return new GLSLToonShaderHalo; }

 protected:
   //******** Member Variables ********
   TEXTUREglptr _tex;           // the texture
   GLint        _tex_loc;       // "location" of sampler2D in the program

   //******** VIRTUAL METHODS ********

   // Return the names of the toon GLSL shader programs:
   virtual str_ptr vp_filename() { return vp_name("halo"); }

   // XXX - temporary, until we can figure out how to not use the
   //       fragment shader:
   virtual str_ptr fp_filename() { return fp_name("halo"); }

   // we're not using any fragment shader:
   // XXX - we have to figure out how to not use the fragment shader
//   virtual str_list fp_filenames() { return str_list(); }
};

#endif // GLSL_TOON_HALO_H_IS_INCLUDED

// end of file glsl_toon_halo.H
