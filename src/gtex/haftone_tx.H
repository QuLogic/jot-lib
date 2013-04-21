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
 * dots_tx_pattern.H:
 **********************************************************************/
#ifndef DOTS_TX_H_IS_INCLUDED
#define DOTS_TX_H_IS_INCLUDED

#include "glsl_shader.H"
#include "gtex/tone_shader.H"
#include "glsl_layer_base.H"
#include "hatching_tx.H"

/*****************************************************************
 * halftone_layer_t
 *****************************************************************/
class halftone_layer_t : public layer_base_t {
 public:

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3(
      "halftone_layer_t", halftone_layer_t*, layer_base_t, layer_base_t*
      );

   //******** DATA_ITEM VIRTUAL METHODS ********
   virtual DATA_ITEM* dup() const { return new halftone_layer_t(); };
   virtual CTAGlist&  tags() const;

   halftone_layer_t(): layer_base_t() {

      _use_tone_correction = false;
      _use_lod_only = false;
      _tone_correction_tex_stage = 0;
      _r_function_tex_stage = 0;

      _use_tone_correction_loc = -1;
      _use_lod_only_loc = -1;
      _tone_correction_tex_loc = -1;
      _r_function_tex_loc = -1;
   }

   bool _use_tone_correction;
   GLint  _use_tone_correction_loc;

   bool _use_lod_only;
   GLint _use_lod_only_loc;

   GLint _tone_correction_tex_stage;
   GLint _tone_correction_tex_loc;

   GLint _r_function_tex_stage;
   GLint _r_function_tex_loc;

   //******** I/O Methods ********
   // used by TAG_val for I/O:
   bool& use_tone_correction() { return _use_tone_correction; };
   bool& use_lod_only()        { return _use_lod_only; };

   virtual void get_var_locs(int i, GLuint& program);
   virtual void send_to_glsl()  const;
 
 private: 
   /******** STATIC MEMBER VARIABLES ********/
   static TAGlist*     _lh_tags;
};

/**********************************************************************
 * Halftone_TX:
 **********************************************************************/
class Halftone_TX : public GLSLShader_Layer_Base {
 public:
   //******** MANAGERS ********
   Halftone_TX(Patch* patch = 0);
   virtual ~Halftone_TX();

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("Halftone_TX", Halftone_TX*, GLSLShader, CDATA_ITEM*);
   
   //******** DATA_ITEM VIRTUAL METHODS ********
   virtual DATA_ITEM* dup() const { return new Halftone_TX(); };
   virtual CTAGlist&  tags() const;

   //******** ACCESSORS ********
   void set_style(int s) ;

   // returns the absolute scale: relative scale times pattern size:
   float get_abs_scale(int l);

   halftone_layer_t* get_layer(int l) const;

   //******** UTILITIES ********

   //this is disabled due to GLSL problems
   //the shader exceeded the maximum instruction count
   //so this functionality is commented out in the fragment program
   void toggle_alpha_transitions() {
      _use_alpha_transitions=!_use_alpha_transitions;
   }

   void set_texture_pattern(int layer, str_ptr file_name);
   void set_procedural_pattern(int layer, int pattern_id);

   //tone correction control
   void toggle_tone_correction(int layer);
   void toggle_lod_only_correction(int layer);

   void enable_tone_correction(int layer);
   void disable_tone_correction(int layer);

   void enable_lod_only_correction(int layer);
   void disable_lod_only_correction(int layer);

   //******** I/O ********

   virtual void get_layer(TAGformat &d);

   //******** GLSLShader_Layer_Base VIRTUAL METHODS ********

   virtual void activate_layer_textures();

   //******** GLSLShader VIRTUAL METHODS ********
   virtual int draw(CVIEWptr& v);
   virtual int draw_final(CVIEWptr& v);

   // base classes can over-ride this to maintain their own program
   // variable, e.g. to share a single program among all instances of
   // the derived class:
   virtual GLuint& program() { return _program; }
   virtual bool&  did_init() { return _did_init; }

   // lookup uniform and attribute variable "locations":
   virtual bool get_variable_locs();

   // send values to uniform variables:
   virtual bool set_uniform_variables() const;

   // Calls glPushAttrib() and sets some OpenGL state:
   virtual void set_gl_state(GLbitfield mask=0) const;

   virtual void activate_textures();

   // shader source file names:
   virtual str_ptr vp_filename() { return vp_name("haftone_tx"); }
   virtual str_ptr fp_filename() { return fp_name("haftone_tx"); }

   //******** GTexture VIRTUAL METHODS ********
   virtual void request_ref_imgs();
   virtual int  draw_color_ref(int i);
   virtual void draw_triangles();

 protected:
   //******** Member Variables ********

   static GLuint _program;  // shared by all Halftone_TX instances
   static bool   _did_init; // tells whether initialization attempt was made

   // dynamic 2D pattern variables:
   static GLint _origin_loc;
   static GLint _u_vec_loc;
   static GLint _v_vec_loc;
   static GLint _st_loc;
   static GLint _halo_tex_loc;
   static GLint _timed_lod_hi;
   static GLint _timed_lod_lo;

   bool         _use_alpha_transitions;
   static GLint _use_alpha_loc;

   static GLint _opacity_remap_loc;

   // tone correction

   map <GLint,TEXTUREglptr> tone_correction_maps;
   map <GLint,TEXTUREglptr> r_function_maps;

   // tone map variables:
 
   static GLint _tone_tex_loc;
   static GLint _dims_loc;

   //******** UTILITIES ********

   str_ptr default_falftone() {return ("pattern5.png"); }

   void build_correction_tex(
      TEXTUREglptr correction_map,TEXTUREglptr source
      );
   void build_r_function_tex(
      TEXTUREglptr r_function_map,TEXTUREglptr source
      );
   void build_pattern_histogram(
      uint* histogram, uint &sample_count, TEXTUREglptr source
      );

   // computes one row :
   void compute_inverse_R(
      uint i_lod, uint sample_count, uint* histogram_array,  Image* image
      );
   void compute_forward_R(
      uint sample_count, uint* histogram_array,  Image* image
      );

   double smooth_step(double edge0, double edge1, double x);
   double procedural_dots(double x, double y);

 private:
   static TAGlist* _halftone_tags;
};

#endif // DOTS_TX_H_IS_INCLUDED

// end of file dots_tx_pattern.H
