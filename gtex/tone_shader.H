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
 * tone_shader.H:
 *****************************************************************/
#ifndef TONE_SHADER_H_IS_INCLUDED
#define TONE_SHADER_H_IS_INCLUDED

#include "glsl_shader.H"

/*****************************************************************
 * ToneStripCB:
 *
 *   Does ToneShader shader in GLSL.  Primarily created to pass in
 *   abstract normals into the gpu per vertex to blend with the 
 *   meshes normals.
 *
 *   Same as XToonStripCB
 *****************************************************************/
class ToneStripCB : public GLStripCB {
 public:
   ToneStripCB() : _blend_type(1), _update(true) {}

   void set_norm_loc(GLint L)        { _norm_loc = L;        }
   void set_len_loc(GLint L)        { _len_loc = L;        }
   void set_blendType(int d)    { _blend_type = d; }
   void set_update(bool b)      { _update = true; }

   virtual void faceCB(CBvert* v, CBface*);

   // types of normal abstraction:
   enum normal_t { SMOOTH=0, SPHERIC, ELLIPTIC, CYLINDRIC };

 private:

   GLint        _len_loc, _norm_loc;
   int          _blend_type; //Tells which Shape normals should blend to
   bool         _update;
};

/*****************************************************************
 * OccluderData
 *****************************************************************/
class OccluderData {
 public:
   OccluderData() :
      _xf_loc(-1),
      _is_active(false),
      _is_active_loc(-1),
      _softness(0.0),
      _softness_loc(-1) {}

   OccluderData& operator=(const OccluderData& o) {
      if (&o != this) {
         _shadow_xf     = o._shadow_xf;
         _xf_loc        = o._xf_loc;
         _is_active     = o._is_active;
         _is_active_loc = o._is_active_loc;
         _softness      = o._softness;
         _softness_loc  = o._softness_loc;
      }
      return *this;
   }
   bool operator==(const OccluderData& o) {
      return (
         _shadow_xf     == o._shadow_xf         &&
         _xf_loc        == o._xf_loc            &&
         _is_active     == o._is_active         &&
         _is_active_loc == o._is_active_loc     &&
         _softness      == o._softness          &&
         _softness_loc  == o._softness_loc
         );
   }

   Wtransf _shadow_xf;
   GLint   _xf_loc;

   bool    _is_active;
   GLint   _is_active_loc;

   double  _softness;
   GLint   _softness_loc;
};

/*****************************************************************
 * tone_layer_t
 *****************************************************************/
class tone_layer_t : public DATA_ITEM {
 public :
   tone_layer_t(GLint is_enabled  = 1  ,  // enable layer?
                GLint remap_nl    = 0  ,  // map dot(n,l) in [-1,1] to [0,1]?
                GLint remap       = 0  ,  // 0 = none, 1 = toon, 2 = smoothstep
                GLint backlight   = 0  ,  // 0 = none, 1 = dark, 2 = light
                GLfloat e0        = 0  ,  // remap smoothstep edge 0
                GLfloat e1        = 1  ,  // remap smoothstep edge 1
                GLfloat s0        = 0.5,  // backlight smoothstep edge 0
                GLfloat s1        = 1.0) :// backlight smoothstep edge 1
      _is_enabled(is_enabled),
      _remap_nl(remap_nl),
      _remap(remap),
      _backlight(backlight),
      _e0(e0),
      _e1(e1),
      _s0(s0),
      _s1(s1) {}
     
   //******** RUN-TIME TYPE ID ******** 
   DEFINE_RTTI_METHODS3("tone_layer_t", tone_layer_t*, DATA_ITEM, CDATA_ITEM *);

   //******** DATA_ITEM VIRTUAL METHODS ********
   virtual DATA_ITEM* dup() const { return new tone_layer_t(); }
   virtual CTAGlist&  tags() const;

   //******** I/O Methods ********
   // used by TAG_val for I/O:
   GLint&        is_enabled(){ return _is_enabled; }  
   GLint&        remap_nl()  { return _remap_nl; }  
   GLint&        remap()     { return _remap; }  
   GLint&        backlight() { return _backlight; }  
   GLfloat&      e0()        { return _e0; }   
   GLfloat&      e1()        { return _e1; }   
   GLfloat&      s0()        { return _s0; }   
   GLfloat&      s1()        { return _s1; }   

   GLint   _is_enabled;
   GLint   _remap_nl;
   GLint   _remap;
   GLint   _backlight;
   GLfloat _e0;
   GLfloat _e1;
   GLfloat _s0;
   GLfloat _s1;

 private: 
   /******** STATIC MEMBER VARIABLES ********/
   static TAGlist*     _tl_tags;
};

/**********************************************************************
 * ToneShader:
 *
 *  Shader used by some "dynamic 2D patterns" shaders
 *  to render to the tone reference image.
 *
 *  Based on GLSLToonShader.
 *
 **********************************************************************/
class ToneShader : public GLSLShader {
 public:
   //******** MANAGERS ********
   ToneShader(Patch* patch = 0);

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("ToneShader", ToneShader*, GLSLShader, CDATA_ITEM*);

   //******** UTILITIES ********

   // Set the name of the texture to use (full path):
   void set_tex(Cstr_ptr& full_path_name);

   // set the TEXTUREgl to use:
   void set_tex(CTEXTUREglptr& tex);
   // set the TEXTUREgl to use:
   void set_tex_2d(CTEXTUREglptr& tex);

   // set smoothed normals
   void set_normals(int i);

   void set_blend_normal(int v) { _blend_normal = v; }       
   void set_unit_len(const float f)         { _unit_len = f; }
   void set_edge_len_scale(const float l)      { _edge_len_scale = l; }
   void set_ratio_scale(const float l)      { _ratio_scale = l; }
   void set_user_depth(const float l)      { _user_depth = l; }

   // get the TEXTUREgl
   TEXTUREglptr get_tex() { return _tex; }

   bool  normals_smoothed(){ return _normals_smoothed; }
   bool  normals_elliptic(){ return _normals_elliptic; }
   bool  normals_spheric(){ return _normals_spheric; }
   bool  normals_cylindric(){ return _normals_cylindric; }

   int   get_blend_normal() { return _blend_normal; }
   float get_unit_len() const      { return _unit_len; }
   float get_edge_len_scale() const   { return _edge_len_scale; }
   float get_ratio_scale() const   { return _ratio_scale; }
   float get_user_depth() const   { return _user_depth; }

   void populate_occluders();

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

   //******** GTexture VIRTUAL METHODS ********

   // for debugging:
   virtual int draw_final(CVIEWptr&);

   //******** DATA_ITEM VIRTUAL METHODS ********
   virtual DATA_ITEM  *dup() const { return new ToneShader; }
   virtual CTAGlist&  tags() const;

 protected:
   //******** Member Variables ********
   TEXTUREglptr  _tex;      // the 1D toon texture
   TEXTUREglptr  _tex_2d;  // 2D texture
   static GLint  _tex_loc;  // "location" of sampler2D in the program  
   static GLint  _tex_2d_loc;  // "location" of sampler2D in the program  
   static GLint  _is_tex_2d_loc;  // "location" of sampler2D in the program  

   int          _smoothNormal;
   bool         _normals_smoothed;
   bool         _normals_elliptic;
   bool         _normals_spheric;
   bool         _normals_cylindric;

   int          _blend_normal;
   float        _unit_len;
   float        _edge_len_scale;
   float        _user_depth;
   float        _ratio_scale;
   float        _global_edge_len;

   ARRAY<OccluderData> _occluders;

 public:
   tone_layer_t _layer[4];

   void set_layer(uint layer_num,
                  GLint is_enabled,
                  GLint remap_nl,
                  GLint remap,
                  GLint backlight,
                  GLfloat e0    = 0,
                  GLfloat e1    = 1,
                  GLfloat s0    = 0,
                  GLfloat s1    = 1) {
      assert(layer_num < 4);
      _layer[layer_num] = tone_layer_t(
         is_enabled,
         remap_nl,
         remap,
         backlight,
         e0,
         e1,
         s0,
         s1
         );
   }
 protected:

   static GLint  _is_enabled_loc[4];
   static GLint  _remap_nl_loc  [4];
   static GLint  _remap_loc     [4];
   static GLint  _backlight_loc [4];
   static GLint  _e0_loc        [4];
   static GLint  _e1_loc        [4];
   static GLint  _s0_loc        [4];
   static GLint  _s1_loc        [4];
   static GLint  _blend_normal_loc;
   static GLint  _unit_len_loc, _edge_len_scale_loc; 
   static GLint  _ratio_scale_loc, _user_depth_loc;
   static GLint  _global_edge_len_loc;
   static GLint  _proj_der_loc;
   static GLint  _is_reciever_loc;

   static GLuint _program;  // GLSL program shared by all toon instances
   static bool   _did_init; // tells whether initialization attempt was made

   static ToneShader* _instance;
   static TAGlist* _tags;

   //******** I/O Methods ********
   virtual void         get_layer (TAGformat &d);
   virtual void         put_layer (TAGformat &d) const;

   //******** VIRTUAL METHODS ********

   virtual str_ptr vp_filename() { return vp_name("tone"); }
   virtual str_ptr fp_filename() { return fp_name("tone"); }
};

#endif // TONE_SHADER_H_IS_INCLUDED

// end of file tone_shader.H
