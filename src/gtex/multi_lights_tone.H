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
 * tone_shader.H:
 **********************************************************************/
#ifndef MULTILIGHTSTONE_H_IS_INCLUDED
#define MULTILIGHTSTONE_H_IS_INCLUDED

#include "tone_shader.H"

/**********************************************************************
 * MLToneShader:
 *
 *
 *  Based on GLSLToonShader.
 *
 **********************************************************************/
class MLToneShader : public ToneShader {
 public:
   //******** MANAGERS ********
   MLToneShader(Patch* patch = 0);

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("MLToneShader", MLToneShader*, ToneShader, CDATA_ITEM*);

   //******** UTILITIES ********

   void set_show_channel(const int c) { _show_channel = c; }
   int get_show_channel() const { return _show_channel; }

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
   virtual DATA_ITEM  *dup() const { return new MLToneShader; }
 protected:
   //******** Member Variables ********
   static GLint  _tex_loc;  // "location" of sampler2D in the program  
   static GLint  _tex_2d_loc;  // "location" of sampler2D in the program  
   static GLint  _is_tex_2d_loc;  // "location" of sampler2D in the program  

 public:

 protected:
   int   _show_channel;

   static GLint  _is_enabled_loc;
   static GLint  _remap_nl_loc;
   static GLint  _remap_loc;
   static GLint  _backlight_loc;
   static GLint  _e0_loc;
   static GLint  _e1_loc;
   static GLint  _s0_loc;
   static GLint  _s1_loc;
   static GLint  _blend_normal_loc;
   static GLint  _global_edge_len_loc;
   static GLint  _proj_der_loc;
   static GLint  _unit_len_loc, _edge_len_scale_loc; 
   static GLint  _ratio_scale_loc, _user_depth_loc;
   static GLint  _show_channel_loc;

   static GLuint _program;  // GLSL program shared by all toon instances
   static bool   _did_init; // tells whether initialization attempt was made

   static MLToneShader* _instance;
   static TAGlist* _tags;

   //******** VIRTUAL METHODS ********

   virtual str_ptr vp_filename() { return vp_name("multi_lights_tone"); }
   virtual str_ptr fp_filename() { return fp_name("multi_lights_tone"); }
};

#endif // MULTILIGHTSTONE_H

// end of file multi_lights_tone.H
