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
 * ImageLineShader.H:
 **********************************************************************/
#ifndef IMG_LINE_SHADER_HE_IS_INCLUDED
#define IMG_LINE_SHADER_HE_IS_INCLUDED

#include "gtex/glsl_shader.H"
#include "gtex/multi_lights_tone.H"
#include "gtex/basecoat_shader.H"
#include "gtex/blur_shader.H"

/**********************************************************************
 * ImageLineStripCB:
 *
 **********************************************************************/

class ImageLineStripCB : public GLStripCB {
 public:
   ImageLineStripCB() {}

   void set_loc(GLint L)        { _loc = L;        }

   virtual void faceCB(CBvert* v, CBface*);
 private:

   GLint        _loc;

};


/**********************************************************************
 * ImageLineShader:
 *
 *  ...

 *  Note: 
 *       
 *
 **********************************************************************/
class ImageLineShader : public GLSLShader {
 public:
   //******** MANAGERS ********
   ImageLineShader(Patch* patch = 0);

   virtual ~ImageLineShader();

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("ImageLineShader",
                        ImageLineShader*, GLSLShader, CDATA_ITEM*);

   //******** ACCESSORS ********

   void set_curv_threshold(const int i, const float thd) { 
      _curv_threshold[i] = thd; }
   void set_line_width(const float w)         { _line_width = w; }
   void set_dark_color(CCOLOR& c)           { _dark_color = c; }  
   void set_highlight_color(CCOLOR& c)      { _highlight_color = c; }  
   void set_light_color(CCOLOR& c)      { _light_color = c; }  
   void set_detail_func(const int a)        { _detail_func = a; }
   void set_unit_len(const float f)         { _unit_len = f; }
   void set_edge_len_scale(const float l)      { _edge_len_scale = l; }
   void set_ratio_scale(const float l)      { _ratio_scale = l; }
   void set_user_depth(const float l)      { _user_depth = l; }
   void set_debug_shader(const int a)        { _debug_shader = a; }
   void set_line_mode(const int m)       { _line_mode = m; }
   void set_draw_mode(const int m)       { _draw_mode = m; }
   void set_confidence_mode(const int m)       { _confidence_mode = m; }
   void set_alpha_offset(const float l)      { _alpha_offset = l; }
   void set_silhouette_mode(const int m)       { _silhouette_mode = m; }
   void set_draw_silhouette(const int m)       { _draw_silhouette = m; }
   void set_highlight_control(const float m)   { _highlight_control = m; }
   void set_light_control(const float m)   { _light_control = m; }
   void set_tapering_mode(const int m)       { _tapering_mode = m; }
   void set_moving_factor(const float m)       { _moving_factor = m; }
   void set_tone_effect(const int m)       { _tone_effect = m; }
   void set_ht_width_control(const float m)       { _ht_width_control = m; }


   float get_curv_threshold(const int i) const { return _curv_threshold[i]; }
   float          get_line_width() const     { return _line_width; }
   ToneShader*  get_tone_shader()          { return _tone; } 
   BasecoatShader*  get_basecoat_shader()      { return _basecoat; } 
   BlurShader*  get_blur_shader()   { return (BlurShader*) _patch->get_tex("BlurShader"); } 
   COLOR        get_dark_color() const      { return _dark_color; }  
   COLOR        get_highlight_color() const { return _highlight_color; }  
   COLOR        get_light_color() const { return _light_color; }  
   int          get_detail_func() const     { return _detail_func; }
   float        get_unit_len() const      { return _unit_len; }
   float        get_edge_len_scale() const   { return _edge_len_scale; }
   float        get_ratio_scale() const   { return _ratio_scale; }
   float        get_user_depth() const   { return _user_depth; }
   int          get_debug_shader() const     { return _debug_shader; }
   int          get_line_mode() const    { return _line_mode; }
   int          get_draw_mode() const    { return _draw_mode; }
   double       get_ndcz_bounding_box_size();
   int          get_confidence_mode() const    { return _confidence_mode; }
   float        get_alpha_offset() const   { return _alpha_offset; }
   int          get_silhouette_mode() const   { return _silhouette_mode; }
   int          get_draw_silhouette() const   { return _draw_silhouette; }
   float        get_highlight_control() const   { return _highlight_control; }
   float        get_light_control() const   { return _light_control; }
   int          get_tapering_mode() const   { return _tapering_mode; }
   float        get_moving_factor() const { return _moving_factor; }
   int          get_tone_effect() const   { return _tone_effect; }
   float        get_ht_width_control() const { return _ht_width_control; }

   //******** STATICS ********

   static ImageLineShader* get_instance();

   //******** ImageLineShader VIRTUAL METHODS ********

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
      ret.add((GTexture*)_basecoat);
      return ret;
   }
   // Initialize textures, if any:
   virtual void init_textures();

   virtual int draw(CVIEWptr& v); 
   virtual int draw_final(CVIEWptr& v);

   //******** DATA_ITEM VIRTUAL METHODS ********
   virtual DATA_ITEM  *dup() const { return new ImageLineShader; }

   //******** RefImageClient METHODS: ********
   virtual int draw_id_ref();
   virtual int draw_vis_ref();

 protected:
   //******** Member Variables ********
   // shader used to draw the tone reference image:
   
   float  _line_width;
   float  _curv_threshold[2];
   int    _detail_func;
   float  _unit_len;
   float  _edge_len_scale;
   float  _user_depth;
   float  _ratio_scale;
   int    _debug_shader;
   int    _line_mode;
   int    _draw_mode;
   int    _confidence_mode;
   float  _alpha_offset;
   int    _silhouette_mode;
   int    _draw_silhouette;
   float  _global_edge_len;
   float  _highlight_control;
   float  _light_control;
   int    _tapering_mode;
   float  _moving_factor;
   float  _ht_width_control;
   int    _tone_effect;

   COLOR        _dark_color;
   COLOR        _highlight_color;
   COLOR        _light_color;

   TEXTUREglptr  _toon_tex;      // the 1D toon texture

   static GLuint _program;  // program shared by all toon instances
   static bool   _did_init; // tells whether initialization attempt was made

   static ImageLineShader* _instance;

   // tone map variables:
   GLint _tone_tex_loc;
   GLint _id_tex_loc;
   GLint _width_loc;
   GLint _height_loc;

   GLint _curv_loc[2];
   GLint _line_width_loc;

   GLint _dark_color_loc;
   GLint _highlight_color_loc;
   GLint _light_color_loc;
   GLint _detail_func_loc;
   GLint _unit_len_loc, _edge_len_scale_loc; 
   GLint _ratio_scale_loc, _user_depth_loc;
   GLint _debug_shader_loc;
   GLint _line_mode_loc;
   GLint _confidence_mode_loc;
   GLint _silhouette_mode_loc;
   GLint _draw_silhouette_loc;
   GLint _alpha_offset_loc;
   GLint _global_edge_len_loc;
   GLint _proj_der_loc;
   GLint _highlight_control_loc;
   GLint _light_control_loc;
   GLint _tapering_mode_loc;
   GLint _moving_factor_loc;
   GLint _tone_effect_loc;
   GLint _ht_width_control_loc;

   ToneShader *_tone;
   BasecoatShader *_basecoat;


   //******** VIRTUAL METHODS ********

   // Return the names of the shader programs:
   virtual str_ptr vp_filename() { return vp_name("img_line"); }

   virtual str_ptr fp_filename() { return fp_name("img_line"); }
};

#endif // IMG_LINE_SHADER_H_IS_INCLUDED

// end of file img_line_shader.H
