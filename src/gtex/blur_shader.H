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
 * blur_shader.H:
 **********************************************************************/
#ifndef BLUR_SHADER_H_IS_INCLUDED
#define BLUR_SHADER_H_IS_INCLUDED

#include "glsl_shader.H"
#include "gtex/tone_shader.H"

/**********************************************************************
 * BlurShader:
 *
 *  ...

 *  Note: 
 *       
 *
 **********************************************************************/
class BlurShader : public GLSLShader {
 public:
   //******** MANAGERS ********
   BlurShader(Patch* patch = 0);

   virtual ~BlurShader();

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("BlurShader",
                        BlurShader*, GLSLShader, CDATA_ITEM*);

   //******** ACCESSORS ********

   void set_detail_func(const int a)        { _detail_func = a; }
   void set_unit_len(const float f)         { _unit_len = f; }
   void set_edge_len_scale(const float l)      { _edge_len_scale = l; }
   void set_ratio_scale(const float l)      { _ratio_scale = l; }
   void set_user_depth(const float l)      { _user_depth = l; }
   void set_blur_size(const float o) { _blur_size = o; }

   int          get_detail_func() const     { return _detail_func; }
   float        get_unit_len() const      { return _unit_len; }
   float        get_edge_len_scale() const   { return _edge_len_scale; }
   float        get_ratio_scale() const   { return _ratio_scale; }
   float        get_user_depth() const   { return _user_depth; }
   float get_blur_size() { return _blur_size; }
   ToneShader*  get_tone_shader()   { return (ToneShader*)_patch->get_tex("ToneShader"); } 

   //******** STATICS ********

   static BlurShader* get_instance();

   //******** BlurShader VIRTUAL METHODS ********

   // using a static variable for program, so all toon shader
   // instances share the same program:
   virtual GLuint& program() { return _program; }
   virtual bool&  did_init() { return _did_init; }

   // Initialize textures, if any:
   virtual void init_textures();

   // Called in init(); query and store the "locations" of
   // uniform and attribute variables here:
   virtual bool get_variable_locs();

   // Send values of uniform variables to the shader:
   virtual bool set_uniform_variables() const;

   // Calls glPushAttrib() and sets some OpenGL state:
   virtual void set_gl_state(GLbitfield mask=0) const;


   // ******** GTexture VIRTUAL METHODS ********
   virtual void request_ref_imgs();
   virtual int  draw_color_ref(int i);

   //******** DATA_ITEM VIRTUAL METHODS ********
   virtual DATA_ITEM  *dup() const { return new BlurShader; }

   void set_tex(Cstr_ptr& full_path_name){

      ((ToneShader*)_patch->get_tex("ToneShader"))->set_tex(full_path_name);
   }

 protected:
   //******** Member Variables ********
   // shader used to draw the tone reference image:

   
   float   _blur_size;
   int    _detail_func;
   float  _unit_len;
   float  _edge_len_scale;
   float  _user_depth;
   float  _ratio_scale;
   float  _global_edge_len;

   static GLuint _program;  // program shared by all toon instances
   static bool   _did_init; // tells whether initialization attempt was made

   static BlurShader* _instance;

   // tone map variables:
   GLint _tone_tex_loc;
   GLint _width_loc;
   GLint _height_loc;
   GLint _blur_size_loc;
   GLint _unit_len_loc, _edge_len_scale_loc; 
   GLint _ratio_scale_loc, _user_depth_loc;
   GLint _global_edge_len_loc;
   GLint _proj_der_loc;
   GLint _detail_func_loc;

   //******** VIRTUAL METHODS ********

   // Return the names of the shader programs:
   virtual str_ptr vp_filename() { return vp_name("blur"); }

   virtual str_ptr fp_filename() { return fp_name("blur"); }
};

#endif // RIDGE_H_IS_INCLUDED

// end of file blur_shader.H
