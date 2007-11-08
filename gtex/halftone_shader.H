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
/**************************************************************************
 *	halftone_shader.H
 *************************************************************************/

#ifndef HALFTONE_SHADER_IS_INCLUDED
#define HALFTONE_SHADER_IS_INCLUDED

#include "glsl_shader.H"

// --compile_time;
class GLSLToonShader;

class HalftoneShader : public GLSLShader {
 public:
   // ******** MANAGERS ********
   HalftoneShader(Patch* patch = 0);
   
   void   set_style(int s) { _style = s; }
   // ******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("Halftone Shader",
                        HalftoneShader*, BasicTexture, CDATA_ITEM*);

   // ******** GLSLShader VIRTUAL METHODS ********

   // Called in init(), subclasses can query and store the
   // "locations" of uniform and attribute variables here. 
   // Base class doesn't use any variables, so it's a no-op.
   virtual bool get_variable_locs();

   // The following are called in draw():

   // Send values of uniform variables to the shader.
   virtual bool set_uniform_variables() const;

   // Initialize textures, if any:
   virtual void init_textures();

   virtual void set_gl_state(GLbitfield mask=0) const;

   // Activate textures, if any:
   virtual void activate_textures();

   // Calls glPopAttrib():
   //virtual void restore_gl_state() const;

   /** Returns a list of the slave gtextures of this class */
   virtual GTexture_list gtextures() const;

   // ******** GTexture VIRTUAL METHODS ********
   virtual void request_ref_imgs();
   virtual int  draw_color_ref(int i);

   // ******** DATA_ITEM VIRTUAL METHODS ********
   virtual DATA_ITEM  *dup() const { return new HalftoneShader; }

 private:

   // ******** Member Variables ********
   GLSLToonShader*   m_toon_tone_shader;
   TEXTUREglptr      m_texture;
   TEXTUREglptr		 _perlin;
   GLint			       _lod_loc;
   GLint             m_origin_loc;
   GLint             m_u_vec_loc;
   GLint             m_v_vec_loc;
   GLint             m_tex_loc;
   GLint             m_width;
   GLint             m_height;
   GLint             _style_loc;  
   int               _style;
   GLint        _perlin_loc;    // "location" of the 
   Perlin* _perlin_generator; //pointer to an instance of the 
                                     //perlin texture generator

   // ******** VIRTUAL METHODS ********

   // Return the names of the halftone GLSL shader programs:
   virtual str_ptr vp_filename() { return vp_name("halftone"); }
   virtual str_ptr fp_filename() { return fp_name("halftone"); }
};

#endif // HALFTONE_SHADER_IS_INCLUDED

// End of file halftone_shader.H
