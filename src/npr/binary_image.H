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
 * binary_image.H:
 **********************************************************************/
#ifndef BINARY_IMAGE_H_IS_INCLUDED
#define BINARY_IMAGE_H_IS_INCLUDED

#include "gtex/glsl_shader.H"

/**********************************************************************
 * BinaryImageShader:
 *

 *  Note: Current version uses GL_TEXTURE2D, but works fine
 *        if you load an image of size 1 x n.
 *
 **********************************************************************/
class BinaryImageShader : public GLSLShader {
 public:
   //******** MANAGERS ********
   BinaryImageShader(Patch* patch = 0);

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("Binary Image",
                        BinaryImageShader*, BasicTexture, CDATA_ITEM*);

   // Set the name of the texture to use (full path):
   void set_tex(Cstr_ptr& full_path_name);

   // set the TEXTUREgl to use:
   void set_tex(CTEXTUREglptr& tex);

   //******** STATICS ********

   static BinaryImageShader* get_instance();

   // static methods for running the toon shader program
   // without necessarily having a Patch of a BMESH;
   static void draw_start(
      TEXTUREglptr toon_tex,     // the 1D toon texture
      CCOLOR& base_color,        // base color for triangles
      double  base_alpha = 1,    // base alpha for triangles
      bool    do_culling = false // enable backface culling
      );

   static void draw_end();

   //******** GLSLShader VIRTUAL METHODS ********

   // using a static variable for program, so all toon shader
   // instances share the same program:
   virtual GLuint& program() { return _program; }
   virtual bool&  did_init() { return _did_init; }

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
   virtual DATA_ITEM  *dup() const { return new BinaryImageShader; }

   void  set_binary_threshold(const float t) { _binary_threshold = t; }
   float get_binary_threshold() const { return _binary_threshold; }

 protected:
   //******** Member Variables ********
   TEXTUREglptr  _tex;      // the 1D toon texture

   float         _binary_threshold;

   static GLint  _tex_loc;  // "location" of sampler2D in the program
   static GLint  _thresh_loc;  // "location" of sampler2D in the program
   static GLuint _program;  // GLSL program shared by all toon instances
   static bool   _did_init; // tells whether initialization attempt was made

   static BinaryImageShader* _instance;

   //******** VIRTUAL METHODS ********

   // Return the names of the toon GLSL shader programs:
   virtual str_ptr vp_filename() { return vp_name("binary"); }

   // XXX - temporary, until we can figure out how to not use the
   //       fragment shader:
   virtual str_ptr fp_filename() { return fp_name("binary"); }

   // we're not using any fragment shader:
   // XXX - we have to figure out how to not use the fragment shader
//   virtual str_list fp_filenames() { return str_list(); }
};

#endif // BINARY_IMAGE_H_IS_INCLUDED

// end of file binary_image.H
