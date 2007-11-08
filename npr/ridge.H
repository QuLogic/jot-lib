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
 * ridge.H:
 **********************************************************************/
#ifndef RIDGE_H_IS_INCLUDED
#define RIDGE_H_IS_INCLUDED

#include "gtex/glsl_shader.H"
#include "gtex/tone_shader.H"

/**********************************************************************
 * RidgeShader:
 *
 *  ...

 *  Note: 
 *       
 *
 **********************************************************************/
class RidgeShader : public GLSLShader {
 public:
   //******** MANAGERS ********
   RidgeShader(Patch* patch = 0);

   virtual ~RidgeShader();

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("Ridge",
                        RidgeShader*, GLSLShader, CDATA_ITEM*);

   //******** ACCESSORS ********

   void set_curv_threshold(const float thd) { _curv_threshold = thd; }
   void set_dist_threshold(const float thd) { _dist_threshold = thd; }
   void set_vari_threshold(const float thd) { _vari_threshold = thd; }
   void set_start_offset(const int offset)  { _start_offset = offset; }
   void set_end_offset(const int offset)    { _end_offset = offset; }
   void set_dark_threshold(const float thd) { _dark_threshold = thd; }
   void set_bright_threshold(const float thd) { _bright_threshold = thd; }

   float get_curv_threshold() const { return _curv_threshold; }
   float get_dist_threshold() const { return _dist_threshold; }
   float get_vari_threshold() const { return _vari_threshold; }
   int   get_start_offset() const   { return _start_offset; }
   int   get_end_offset() const     { return _end_offset; }
   ToneShader*  get_tone_shader()   { return (ToneShader*)_patch->get_tex("ToneShader"); } 
   float get_dark_threshold() const { return _dark_threshold; }
   float get_bright_threshold() const { return _bright_threshold; }

   //******** STATICS ********

   static RidgeShader* get_instance();

   //******** RidgeShader VIRTUAL METHODS ********

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

   //******** DATA_ITEM VIRTUAL METHODS ********
   virtual DATA_ITEM  *dup() const { return new RidgeShader; }

   void set_tex(Cstr_ptr& full_path_name){

      ((ToneShader*)_patch->get_tex("ToneShader"))->set_tex(full_path_name);
   }

 protected:
   //******** Member Variables ********
   // shader used to draw the tone reference image:

   
   int    _start_offset, _end_offset;
   float  _curv_threshold, _dist_threshold, _vari_threshold;
   float  _dark_threshold, _bright_threshold;

   static GLuint _program;  // program shared by all toon instances
   static bool   _did_init; // tells whether initialization attempt was made

   static RidgeShader* _instance;

   // tone map variables:
   GLint _tone_tex_loc;
   GLint _width_loc;
   GLint _height_loc;

   GLint _curv_loc;
   GLint _dist_loc;
   GLint _vari_loc;
   GLint _start_offset_loc;
   GLint _end_offset_loc;
   GLint _dark_loc, _bright_loc;

   //******** VIRTUAL METHODS ********

   // Return the names of the shader programs:
   virtual str_ptr vp_filename() { return vp_name("ridge"); }

   virtual str_ptr fp_filename() { return fp_name("ridge"); }

};

#endif // RIDGE_H_IS_INCLUDED

// end of file ridge.H
