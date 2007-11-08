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
/*****************************************************************
 * A base class for GLSL textures using layers of textured patterns
 *
 * by Karol Szerszen  Fall 2006
 *****************************************************************/
#ifndef LAYER_BASE_H_IS_INCLUDED
#define LAYER_BASE_H_IS_INCLUDED

#include "gtex/glsl_shader.H"
#include "gtex/solid_color.H"
#include "gtex/tone_shader.H"
#include "gtex/util.H"

/*****************************************************************
 * layer_base_t:
 *
 * Storage clss for one layer. More specific layers should
 * be derived from this.
 *
 * Each base layer has these properties which need to be
 * reflected in the GLSL code:
 *
 * - an INT variable determining the mode:
 *   0:   disabled layer
 *   1:   using texture pattern
 *   2,3: procedural modes
 *
 * - a SAMPLER2D variable valid if using texture
 *
 * - BOOL highlights on/off
 * - VEC3 containing the ink color
 * - FLOAT containing the scale of the pattern
 *****************************************************************/
class layer_base_t: public DATA_ITEM {
 private: 
   //******** STATIC MEMBER VARIABLES ********
   static TAGlist*     _lb_tags;
 public :
   layer_base_t() :
      _pattern_name(""),
      _mode(0), //disabled layer
      _highlight(false),
      _highlight_loc(-1),
      _pattern_tex_stage(0),
      _pattern_tex_stage_loc(-1),
      _pattern_scale(10.0f),
      _pattern_scale_loc(-1),
      _ink_color(Color::black),
      _ink_color_loc(-1),
      _channel(0),
      _channel_loc(-1)
      {}
   virtual ~layer_base_t() {}

   //******** RUN-TIME TYPE ID ******** 
   DEFINE_RTTI_METHODS3(
      "layer_base_t", layer_base_t*, DATA_ITEM, CDATA_ITEM*
      );

   //get the variable locations for this layer
   virtual void get_var_locs(int i, GLuint& program);
   // if children want to send only subset of the parameters...
   virtual void get_var_locs_some(int mode, int i, GLuint& program);

   //******** GLSL LAYER NAMING CONVENTION ********

   // the above function expects these names 
   // in the fragment program 
   //
   // i=[0..3]
   //
   // layer[i].mode          <- (int)   operation mode
   // layer[i].is_highlight  <- (bool)  highlight enable
   // layer[i].ink_color     <- (vec3)  color of the ink
   // layer[i].pattern_scale <- (float) pattern scale (textured and procedural)
   // layer[i].pattern       <- (sampler2D) pattern texture

   // sends this layer's settings to glsl:
   virtual void send_to_glsl() const;  

   // layer properties
   // these will be a part of presets
   // each property has a coresponding location in the GLSL code
   bool get_uniform_loc(Cstr_ptr& var_name, GLint& loc, GLuint& program);

   //******** DATA_ITEM VIRTUAL METHODS ********
   virtual DATA_ITEM* dup() const { return new layer_base_t(); }
   virtual CTAGlist&  tags() const;

   //******** MEMBER DATA ********
   str_ptr _pattern_name;

   // layer operation mode
   int   _mode;
   GLint _mode_loc;

   // highlight layer mode on/off
   bool  _highlight;
   GLint _highlight_loc;

   // this will point to a texture
   // multiple layers can point to the same texture stage
   int   _pattern_tex_stage;
   GLint _pattern_tex_stage_loc;

   // pattern scale used for both textured and procedural mode
   float _pattern_scale;
   GLint _pattern_scale_loc;

   // ink color
   COLOR _ink_color;
   GLint _ink_color_loc;

   // light channel
   GLint _channel;
   GLint _channel_loc;

   //******** I/O Methods ********
   virtual void get_pattern_name (TAGformat &d);
   virtual void put_pattern_name (TAGformat &d) const;

   int&        mode()         { return _mode; }
   bool&       highlight()    { return _highlight; }
   float&      pattern_scale(){ return _pattern_scale; }
   COLOR&      ink_color()    { return _ink_color; }
   GLint&      channel()      { return _channel; }
};

/*****************************************************************
 * GLSLShader_Layer_Base:
 *
 *  Base class for shaders that implement "Dynamic 2D Patterns"
 *  like Hatching_TX, Painterly, Halftone_TX.
 *****************************************************************/
class GLSLShader_Layer_Base : public GLSLShader {
 public :
   //******** MANAGERS ********
   GLSLShader_Layer_Base(Patch* p, StripCB* cb=0);
   virtual ~GLSLShader_Layer_Base();

   enum max_layer_t { MAX_LAYERS=4 };

   //******** ACCESSORS ********
   ToneShader* get_tone_shader() const  { return get_tex<ToneShader>(_patch); }

   //******** UTILITIES ********

   // fills in the locations for layer variables
   // calls get_var_locs(int i, GLuint& program);
   virtual void get_layers_variable_names();

   // sends all the layer data to glsl
   // layer class may be derived, but it should
   // know how to send its data via send_to_glsl()
   virtual void send_layers_variables() const;

   virtual void activate_layer_textures();

   // layer control functions

   void set_texture_pattern(int layer, str_ptr dir, str_ptr file_name);
   void set_procedural_pattern(int layer, int pattern_id);
   void disable_layer(int layer);
   void cleanup_unused_textures(int tex_stage);

   //******** I/O METHODS ********

   // get_layer should be defined in derived classes
   void put_layer(TAGformat &d) const;

   void get_tone_shader(TAGformat &d); 
   void put_tone_shader(TAGformat &d) const;  

   void get_base_shader(TAGformat &d);
   void put_base_shader(TAGformat &d) const;

   CCOLOR get_base_color()       const  { return _solid->get_color(); }  
   void   set_base_color(CCOLOR& c)     { _solid->set_color(c); }  

   //******** GTexture VIRTUAL METHODS ********
   virtual GTexture_list gtextures() const {
      return GTexture_list(_solid);
   }

   //******** DATA_ITEM VIRTUAL METHODS ********
   virtual CTAGlist&  tags() const;

 protected:
   //******** Member Variables ********

   // base coat:
   SolidColorTexture* _solid;

   ARRAY<layer_base_t*> _layers;

   // keeps track of used texture stages
   // the lowest free stage is determined by TexUnit::PATTERN_TEX
   // defined in /gtex/util.H
   map<GLint, TEXTUREglptr> _patterns;

   vector<int> _used_texture_stages;   //unsorted

   static TAGlist* _tags;

   //******** UTILITIES ********

   // use these functions to keep track of stuff
   // returns the lowest free texture stage, and marks as used
   int get_free_tex_stage(); 
   void free_tex_stage(int tex_stage);  //marks texture stage as unused

   bool is_used_tex_stage(int tex_stage);

   str_ptr get_name(Cstr_ptr& name, Cstr_ptr& default_name) {
      return (name == "") ? default_name : name;
   }
};

/*****************************************************************
 * StripOpCB:
 *   
 *   Sends "opacity" values to GLSL shaders; used for blending
 *   across patch boundaries. "opacity" is a misnomer, should be
 *   "blending weights".
 *****************************************************************/
class StripOpCB : public GLStripCB {
 public:
   StripOpCB() : _opacity_attr_loc(-1), _patch(0) {}
   void set_opacity_attr_loc(GLint loc) { _opacity_attr_loc = loc; }
   void set_patch(Patch* p) { _patch = p; }

   void send_opacity_attr(float opcaity) {
      if (_opacity_attr_loc != -1)
         glVertexAttrib1f(_opacity_attr_loc, opcaity);
   }
   virtual void faceCB(CBvert* v, CBface* f);

 private:
   GLint   _opacity_attr_loc;
   Patch*  _patch;   
};

#endif // LAYER_BASE_H_IS_INCLUDED
