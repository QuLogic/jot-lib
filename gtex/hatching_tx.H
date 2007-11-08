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
 * hatching_tx_pattern.H:
 **********************************************************************/
#ifndef HATCHING_TX_H_IS_INCLUDED
#define HATCHING_TX_H_IS_INCLUDED

#include "glsl_layer_base.H"

/**********************************************************************
 * layer_hatching_t
 **********************************************************************/
class layer_hatching_t : public layer_base_t {
 private: 
   /******** STATIC MEMBER VARIABLES ********/
   static TAGlist*     _lh_tags;
 public:
   layer_hatching_t(); 
   
   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3(
      "layer_hatching_t", layer_hatching_t*, layer_base_t, layer_base_t*
      );

    //******** DATA_ITEM VIRTUAL METHODS ********
   virtual DATA_ITEM* dup() const { return new layer_hatching_t(); }
   virtual CTAGlist&  tags() const;

   str_ptr      _paper_name;

   GLfloat      _angle; // initial rotation applied to pattern, in degrees
   GLint        _angle_loc;
   GLfloat      _paper_contrast;  
   GLint        _paper_contrast_loc;
   GLint        _paper_tex; 
   GLint        _paper_tex_loc; 
   GLfloat      _paper_scale;
   GLint        _paper_scale_loc;
   GLfloat      _tone_push;
   GLint        _tone_push_loc;
  
   virtual void get_var_locs(int i, GLuint& program);
   virtual void send_to_glsl() const;

   //******** I/O Methods ********
   // used by TAG_val for I/O:
   virtual void         get_paper_name (TAGformat &d);
   virtual void         put_paper_name (TAGformat &d) const;
   float&      angle()           { return _angle; }
   float&      paper_contrast()  { return _paper_contrast; }
   float&      paper_scale()     { return _paper_scale; }   
   float&      tone_push()       { return _tone_push; }   
};

/**********************************************************************
 * Hatching_TX:
 **********************************************************************/
class HatchingTX : public GLSLShader_Layer_Base {
 public:
   //******** MANAGERS ********
   HatchingTX(Patch* patch = 0);
   virtual ~HatchingTX();

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("HatchingTX", HatchingTX*, GLSLShader, CDATA_ITEM*);

   //******** ACCESSORS ********
  
   // sets the hatching and paper textures for the given layer,
   // but only if they are currently not assigned textures.
   void init_default(int layer);

   void init_layer(int l, Cstr_ptr& pattern, Cstr_ptr& paper);

   layer_hatching_t* get_layer(int i) const {
      return dynamic_cast<layer_hatching_t*>(_layers[i]);
   }

   //******** I/O Methods ********
   virtual void get_layer(TAGformat &d);

   //******** GLSLShader VIRTUAL METHODS ********

   virtual GLuint& program() { return _program; }
   virtual bool&  did_init() { return _did_init; }

   // Initialize textures, if any:
   virtual void init_textures();

   // lookup uniform and attribute variable "locations":
   virtual bool get_variable_locs();

   // send values to uniform variables:
   virtual bool set_uniform_variables() const;

   // Calls glPushAttrib() and sets some OpenGL state:
   virtual void set_gl_state(GLbitfield mask=0) const;

   virtual void activate_textures();
   virtual void activate_layer_textures();

   virtual void draw_triangles();

   // shader source file names:
   virtual str_ptr vp_filename() { return vp_name("hatching_tx"); }
   virtual str_ptr fp_filename() { return fp_name("hatching_tx"); }  

   //******** GTexture VIRTUAL METHODS ********
   virtual void request_ref_imgs();
   virtual int  draw_color_ref(int i);
   
   virtual int draw(CVIEWptr& v);
   virtual int draw_final(CVIEWptr& v);

   //******** DATA_ITEM VIRTUAL METHODS ********
   virtual DATA_ITEM* dup() const { return new HatchingTX(); }
   virtual CTAGlist&  tags() const;

   void set_texture_pattern(int layer, str_ptr file_name);
   void set_paper_texture(int layer, str_ptr file_name);
   void cleanup_unused_paper_textures(int tex_stage); 

 protected:
   //******** Member Variables ********
   static TAGlist* _hatching_tags;
   static GLuint _program;  // shared by all HatchingTX instances
   static bool   _init; // tells whether initialization attempt was made
  
   // dynamic 2D pattern variables:
   static GLint _origin_loc;
   static GLint _u_vec_loc;
   static GLint _v_vec_loc;
   static GLint _st_loc;

   // tone map variables:
   static GLint _tone_tex_loc;
   static GLint _dims_loc; 
   static GLint _halo_tex_loc;

   static GLint _opacity_remap_loc;

   //keeps track of used paper texturss
   map<GLint, TEXTUREglptr> _paper_textures;
};

#endif // HATCHING_TX_H_IS_INCLUDED

// end of file hatching_tx_pattern.H
