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
 * glsl_hatching.H:
 **********************************************************************/
#ifndef GLSL_HATCHING_H_IS_INCLUDED
#define GLSL_HATCHING_H_IS_INCLUDED

#include "glsl_paper.H"

class ToneShader;

struct HatchingStyle
{
   const static int LAYER_NUMBER = 4;
   HatchingStyle(int l1, int l2, int l3, int l4);
   void set_style(int l1, int l2, int l3, int l4);   
   int  visible[LAYER_NUMBER];       //is this layer visible
   float color[LAYER_NUMBER*3];
   float line_spacing[LAYER_NUMBER];
   float line_width[LAYER_NUMBER];
   int  do_paper[LAYER_NUMBER];
   float angle[LAYER_NUMBER];
   float perlin_amp[LAYER_NUMBER];
   float perlin_freq[LAYER_NUMBER];
   float min_tone[LAYER_NUMBER];
   int highlight[LAYER_NUMBER];
   int channel[LAYER_NUMBER];
};

/**********************************************************************
 * GLSLHatching:
 *
 *  Simple image-space hatching via GLSL fragment program
 *
 **********************************************************************/
class GLSLHatching : public GLSLPaperShader {
 public:
   //******** MANAGERS ********
   GLSLHatching(Patch* patch = 0);
   
   virtual ~GLSLHatching();

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("GLSL Hatching", GLSLHatching*, BasicTexture, CDATA_ITEM*);

   //******** ACCESSORS ********
   void  set_style(int l1, int l2, int l3, int l4); 

   void set_tone_shader(ToneShader* g);

   //******** GLSLShader VIRTUAL METHODS ********

   // using a static variable for program, so all GLSLHatching
   // instances share the same program:
   virtual GLuint& program() { return _program; }
   virtual bool&  did_init() { return _did_init; }

   virtual bool get_variable_locs();

   virtual bool set_uniform_variables() const;

    // Initialize textures, if any:
   virtual void init_textures();
   virtual void activate_textures();

   virtual void set_gl_state(GLbitfield mask=0) const;

   // ******** GTexture VIRTUAL METHODS ********
   virtual void request_ref_imgs();
   virtual int  draw_color_ref(int i);

   // Returns a list of the slave gtextures of this class
   virtual GTexture_list gtextures() const;

   //******** DATA_ITEM VIRTUAL METHODS ********
   virtual DATA_ITEM  *dup() const { return new GLSLHatching; }

 protected:
   // shader used to draw the tone reference image:
   ToneShader*     _tone_shader;
   
   //perlin noise stuff
   TEXTUREglptr    _perlin;           // 2D perlin noise texture
   static GLint    _perlin_loc;       // "location" of the 
   Perlin*         _perlin_generator; // pointer to an instance of the 
                                      // perlin texture generator
   
   HatchingStyle* _style;

   static GLuint _program;  // GLSL program shared by all instances
   static bool   _did_init; // tells whether initialization attempt was made

   // tone map variables:
   static GLint  _tone_tex_loc;
   static GLint  _width_loc;
   static GLint  _height_loc;

   static GLint _visible_loc;       //is this layer visible
   static GLint _color_loc;
   static GLint _line_spacing_loc;
   static GLint _line_width_loc;
   static GLint _do_paper_loc;
   static GLint _angle_loc;
   static GLint _perlin_amp_loc;
   static GLint _perlin_freq_loc;
   static GLint _min_tone_loc;
   static GLint _highlight_loc;
   static GLint _channel_loc;

   //******** VIRTUAL METHODS ********

   // GLSL vertex shader name
   virtual str_ptr vp_filename() { return vp_name("hatching"); }

   // GLSL fragment shader name
   virtual str_ptr fp_filename() { return fp_name("hatching"); }
};

#endif // GLSL_HATCHING_H_IS_INCLUDED

// end of file glsl_hatching.H
