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
 * basecoat_shader.H:
 **********************************************************************/
#ifndef BASECOAT_SHADER_H_IS_INCLUDED
#define BASECOAT_SHADER_H_IS_INCLUDED

#include "tone_shader.H"

/**********************************************************************
 * BasecoatShader:
 *
 *
 *  Based on GLSLToonShader.
 *
 **********************************************************************/
class BasecoatShader : public ToneShader {
 public:
   //******** MANAGERS ********
   BasecoatShader(Patch* patch = 0);

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("BasecoatShader", BasecoatShader*, ToneShader, CDATA_ITEM*);

   //******** UTILITIES ********
   void set_base_color(const int i, CCOLOR& c) { _base_color[i] = c; }  
   void set_basecoat_mode(const int i);
   void   set_color_steepness(const float f) { _color_steepness = f; }
   void   set_color_offset(const float f)  { _color_offset = f; }
   void   set_light_separation(const int i)  { _light_separation = i; }

   COLOR   get_base_color(const int i) const      { return _base_color[i]; }  
   int     get_basecoat_mode() const;
   float   get_color_steepness() const { return _color_steepness; }
   float   get_color_offset() const { return _color_offset; }
   int     get_light_separation() const { return _light_separation; }


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

   // Calls glPushAttrib() and sets some OpenGL state:
   virtual void set_gl_state(GLbitfield mask=0) const;

   //******** DATA_ITEM VIRTUAL METHODS ********
   virtual DATA_ITEM  *dup() const { return new BasecoatShader; }
 protected:
   //******** Member Variables ********
   static GLint  _tex_loc;  // "location" of sampler2D in the program  
   static GLint  _tex_2d_loc;  // "location" of sampler2D in the program  
   static GLint  _is_tex_2d_loc;  // "location" of sampler2D in the program  

 public:

 protected:
   COLOR _base_color[2];
   int   _basecoat_mode;
   float _color_offset;
   float _color_steepness;
   int   _light_separation;

   static GLint  _is_enabled_loc;
   static GLint  _remap_nl_loc;
   static GLint  _remap_loc;
   static GLint  _backlight_loc;
   static GLint  _e0_loc;
   static GLint  _e1_loc;
   static GLint  _s0_loc;
   static GLint  _s1_loc;
   static GLint  _blend_normal_loc;
   static GLint  _unit_len_loc, _edge_len_scale_loc; 
   static GLint  _ratio_scale_loc, _user_depth_loc;
   static GLint  _base_color_loc[2];
   static GLint  _color_offset_loc;
   static GLint  _color_steepness_loc;
   static GLint  _global_edge_len_loc;
   static GLint  _proj_der_loc;
   static GLint  _light_separation_loc;

   static GLuint _program;  // GLSL program shared by all toon instances
   static bool   _did_init; // tells whether initialization attempt was made

   static BasecoatShader* _instance;
   static TAGlist* _tags;

   //******** VIRTUAL METHODS ********

   virtual str_ptr vp_filename() { return vp_name("basecoat"); }
   virtual str_ptr fp_filename() { return fp_name("basecoat"); }
};

#endif // BASECOAT_SHADER_H_

// end of file basecoat_shader.H
