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
#ifndef GLSL_PAPER_H_IS_INCLUDED
#define GLSL_PAPER_H_IS_INCLUDED

#include "glsl_shader.H"

/**********************************************************************
 * GLSLPaperShader:
 *
 **********************************************************************/
class GLSLPaperShader : public GLSLShader {
 public:
   //******** MANAGERS ********
   GLSLPaperShader(Patch* patch = 0);

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("GLSL paper",
                        GLSLPaperShader*, BasicTexture, CDATA_ITEM*);

   //******** Static ********

   static GLSLPaperShader* instance();

   // Use this to get the paper texture
   static TEXTUREglptr get_texture();
   static str_ptr get_paper_filename() { return _paper_tex_filename; }; 
   static void    set_contrast(double c) { _contrast = c; }

   // Set the name of the paper texture to use (full path):
   static void set_paper(Cstr_ptr& full_path_name);

   // name of stroke texture (will be bound to tex unit 3):
   static void set_tex(Cstr_ptr& full_path_name);

   static void    begin_glsl_paper(Patch* p);
   static void    end_glsl_paper();

   static void    set_do_texture(bool v) { _do_texture = v; };
   static bool    get_do_texture()       { return _do_texture; };

   //******** GLSLShader VIRTUAL METHODS ********

   // using a static variable for program, so all GLSLPaperShader
   // instances share the same program:
   virtual GLuint& program() { return _program; }
   virtual bool&  did_init() { return _did_init; }

   // called in init() to query and store the "locations" of
   // uniform and attribute variables:
   virtual bool get_variable_locs();

   // Send values of uniform variables to the shader:
   virtual bool set_uniform_variables() const;

   virtual void init_textures();

   virtual void activate_textures();
   
   // ******** GTexture VIRTUAL METHODS ********
   virtual void request_ref_imgs();

   //******** DATA_ITEM VIRTUAL METHODS ********
   virtual DATA_ITEM  *dup() const { return new GLSLPaperShader; }

 protected:
   //******** Member Variables ********
   static GLSLPaperShader* _instance;
   static TEXTUREglptr     _paper_tex;            // the paper texture
   static str_ptr          _paper_tex_name;       // paper texture name
   static str_ptr          _paper_tex_filename;   // paper texture filename
   static TEXTUREglptr     _tex;                  // the stroke texture
   static str_ptr          _tex_name;             // stroke texture name
   static double           _contrast;
   static GLint            _paper_tex_loc;
   static GLint            _tex_loc;
   static GLint            _sample_origin;
   static GLint            _sample_u_vec;
   static GLint            _sample_v_vec;
   static GLint            _st_loc;
   static GLint            _tex_width_loc;
   static GLint            _tex_height_loc;
   static GLint            _contrast_loc;
   
   static bool             _do_texture;

   static GLuint _program;  // GLSL program shared by all instances
   static bool   _did_init; // tells whether initialization attempt was made


   //******** VIRTUAL METHODS ********

   // Return the names of the toon GLSL shader programs:
   virtual str_ptr vp_filename() { return vp_name("paper"); }

   // XXX - temporary, until we can figure out how to not use the
   //       fragment shader:
   virtual str_ptr fp_filename() { return fp_name("paper"); }
};

#endif // GLSL_PAPER_H_IS_INCLUDED

// end of file glsl_paper.H
