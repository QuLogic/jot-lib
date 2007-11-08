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
 * SimpleImageLineShader.H:
 **********************************************************************/
#ifndef SIMPLE_IMG_LINE_SHADER_HE_IS_INCLUDED
#define SIMPLE_IMG_LINE_SHADER_HE_IS_INCLUDED

#include "gtex/glsl_shader.H"
#include "gtex/multi_lights_tone.H"
#include "gtex/blur_shader.H"

/**********************************************************************
 * SimpleImageLineShader:
 *
 * Line drawings via abstracted shading
 *       
 * Shader renders lines along "ridges" and "valleys" in the tone image.
 *
 **********************************************************************/
class SimpleImageLineShader : public GLSLShader {
 public:
   //******** MANAGERS ********
   SimpleImageLineShader(Patch* patch = 0);
   virtual ~SimpleImageLineShader();

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("SimpleImageLineShader",
                        SimpleImageLineShader*, GLSLShader, CDATA_ITEM*);

   //******** ACCESSORS ********

   void set_curv_threshold(const int i, const float thd) { 
      _curv_threshold[i] = thd; }
   void set_line_width(const float w)         { _line_width = w; }
   void set_curv_opacity_ctrl(const int m)       { _curv_opacity_ctrl = m; }
   void set_dist_opacity_ctrl(const int m)       { _dist_opacity_ctrl = m; }
   void set_moving_factor(const float m)       { _moving_factor = m; }
   void set_tone_opacity_ctrl(const int m)       { _tone_opacity_ctrl = m; }


   float get_curv_threshold(const int i) const { return _curv_threshold[i]; }
   float          get_line_width() const     { return _line_width; }
   ToneShader*  get_tone_shader()          { return (ToneShader*)_tone; } 
   BlurShader*  get_blur_shader()   { return (BlurShader*) _patch->get_tex("BlurShader"); } 
   int          get_curv_opacity_ctrl() const    { return _curv_opacity_ctrl; }
   int          get_dist_opacity_ctrl() const   { return _dist_opacity_ctrl; }
   float        get_moving_factor() const { return _moving_factor; }
   int          get_tone_opacity_ctrl() const   { return _tone_opacity_ctrl; }

   //******** STATICS ********

   static SimpleImageLineShader* get_instance();

   //******** SimpleImageLineShader VIRTUAL METHODS ********

   // using a static variable for program, so all toon shader
   // instances share the same program:
   virtual GLuint& program() { return _program; }
   virtual bool&  did_init() { return _did_init; }

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
   virtual GTexture_list gtextures() const {
      GTexture_list ret;
      ret.add((GTexture*)_tone);
      return ret;
   }
   // Initialize textures, if any:
   virtual void init_textures();

   virtual int draw(CVIEWptr& v); 
   virtual int draw_final(CVIEWptr& v);

   //******** DATA_ITEM VIRTUAL METHODS ********
   virtual DATA_ITEM  *dup() const { return new SimpleImageLineShader; }

   //******** RefImageClient METHODS: ********
   virtual int draw_id_ref();

 protected:
   //******** Member Variables ********
   // shader used to draw the tone reference image:
   
   float  _line_width;
   float  _curv_threshold[2];
   int    _curv_opacity_ctrl;
   int    _dist_opacity_ctrl;
   float  _moving_factor;
   int    _tone_opacity_ctrl;


   static GLuint _program;  // program shared by all toon instances
   static bool   _did_init; // tells whether initialization attempt was made

   static SimpleImageLineShader* _instance;

   // tone map variables:
   GLint _tone_tex_loc;
   GLint _width_loc;
   GLint _height_loc;

   GLint _curv_loc[2];
   GLint _line_width_loc;

   GLint _curv_opacity_ctrl_loc;
   GLint _dist_opacity_ctrl_loc;
   GLint _moving_factor_loc;
   GLint _tone_opacity_ctrl_loc;

   ToneShader *_tone;


   //******** VIRTUAL METHODS ********

   // Return the names of the shader programs:
   virtual str_ptr vp_filename() { return vp_name("simple_img_line"); }
   virtual str_ptr fp_filename() { return fp_name("simple_img_line"); }
};

#endif // IMG_LINE_SHADER_H_IS_INCLUDED

// end of file img_line_shader.H
