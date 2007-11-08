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
 * glsl_shader.H:
 **********************************************************************/
#ifndef GLSL_SHADER_H_IS_INCLUDED
#define GLSL_SHADER_H_IS_INCLUDED

#include "geom/texturegl.H"
#include "geom/gl_util.H"
#include "util.H"               // TexUnit
#include "basic_texture.H"
#include "perlin.H" //header file still included so it can use the namespace

/**********************************************************************
 * GLSLShader:
 *
 *   Base class for GTextures that replace OpenGL's fixed
 *   functionality with GLSL vertex and fragment programs.
 *
 *   Derived classes should fill in two virtual methods that
 *   return the names of the required vertex and fragment
 *   programs; the base class provides functionality to read the
 *   different shader files and link them into a GLSL program.
 *
 *   Derived classes can also override other virtual methods
 *   (see below) to load and activate textures, query
 *   variable locations, and send values of uniform
 *   variables to the program. To send values of attribute
 *   variables to the program, use a custom StripCB.
 *
 *   See glsl_toon.H for an example of using this class.
 *
 **********************************************************************/

class GLSLShader : public BasicTexture { //no longer derived from Perlin
 public:
   //******** MANAGERS ********
   GLSLShader(Patch* patch = 0, StripCB* cb = 0);
   virtual ~GLSLShader();

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("GLSLShader", GLSLShader*, BasicTexture, CDATA_ITEM*);

   //******** ACCESSORS ********

   void    set_draw_sils(bool b)        { _draw_sils = b; }
   bool    get_draw_sils()      const   { return _draw_sils; }

   void    set_sil_width(GLfloat w)     { _sil_width = w; }
   GLfloat get_sil_width()      const   { return _sil_width; }

   void    set_sil_color(CCOLOR& c)     { _sil_color = c; }
   COLOR   get_sil_color()      const   { return _sil_color; }

   void    set_sil_alpha(GLfloat a)     { _sil_alpha = a; }
   GLfloat get_sil_alpha()      const   { return _sil_alpha; }

   //******** UTILITY FUNCTIONS ********
   str_ptr shader_path() const { return Config::JOT_ROOT() + "nprdata/glsl/"; }

   // Given the base name of the shader, add the full path and extension:
   str_ptr vp_name(Cstr_ptr& name) const { return shader_path() + name + ".vp"; }
   str_ptr fp_name(Cstr_ptr& name) const { return shader_path() + name + ".fp"; }

   // Wrapper for glGetUniformLocation, with error reporting:
   bool get_uniform_loc(Cstr_ptr& name, GLint& loc);

   // Optimistic convenience version of above:
   GLint get_uniform_loc(Cstr_ptr& name) {
      GLint ret = -1;
      get_uniform_loc(name, ret);
      return ret;
   }

   // Load the texture (should be already allocated TEXTUREgl
   // with filename set as member variable).
   //
   // Note: Does not activate the texture for use in the current frame.
   //
   // This is a no-op of the texture was previously loaded.
   static bool load_texture(TEXTUREglptr& tex);

   // Load (if needed) and activate the texture.
   static bool activate_texture(TEXTUREglptr& tex);

   //******** VIRTUAL METHODS ********

   // base classes can over-ride this to maintain their own program
   // variable, e.g. to share a single program among all instances of
   // the derived class:
   virtual GLuint& program() { return _program; }
   virtual bool&  did_init() { return _did_init; }


   //gets called between the compile and link of the program
   //it is meant to explicitly bind the vertex attribute locations
   //this operation is ****NOT REQUIRED**** 
   //attributes not explicitly bound will be bound automaticly by the linker
   virtual bool bind_attributes(GLuint prog) { return true; };

   // Called in init(), subclasses can query and store the
   // "locations" of uniform and attribute variables here. 
   // Base class doesn't use any variables, so it's a no-op.
   virtual bool get_variable_locs() { return true; }

   // The following are called in draw():

   // Send values of uniform variables to the shader.
   virtual bool set_uniform_variables() const { return true; }

   // Initialize textures, if any:
   virtual void init_textures() {}

   // Calls glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | mask),
   // enables/disables face culling as needed, and sets the current color:
   virtual void set_gl_state(GLbitfield mask=0) const;

   // Activate textures, if any:
   virtual void activate_textures() {}

   // after everthing has been set, draw the triangles:
   virtual void draw_triangles() {
      _patch->draw_tri_strips(_cb);
   }

   // Calls glPopAttrib():
   virtual void restore_gl_state() const;


   //******** GTexture VIRTUAL METHODS ********

   // Draw the patch. If Base classes over-ride the above virtual
   // methods correctly, they don't need to over-ride draw.
   // See GLSLToonShader for an example.
   virtual int draw(CVIEWptr& v); 
   virtual void draw_sils();

   //******** DATA_ITEM VIRTUAL METHODS ********
   virtual DATA_ITEM  *dup() const { return new GLSLShader; }

 protected:
   GLuint       _program;       // GLSL program
   bool         _did_init;      // tells if init() was called before

   // optional: silhouettes:
   bool         _draw_sils;
   GLfloat      _sil_width;
   COLOR        _sil_color;
   GLfloat      _sil_alpha;

   //******** PROTECTED UTILITY METHODS ********

   // Loads and compiles the GLSL program:
   bool init();

   // Activate/deactivate the program:
   void activate_program()         { use_program(program()); }
   void deactivate_program() const { use_program(0); }

   // Utility to activate a GLSL program (via OpenGL v. 2.0 or 1.5):
   static void use_program(GLuint prog);

   static void delete_program(GLuint& prog);
   static void delete_shader(GLuint shader);
   static void delete_shaders(CARRAY<GLuint>& shaders);
   static void print_shader_source(GLuint shader);
   static void print_info(Cstr_ptr& gtex_name, GLuint obj);
   static bool link_program(Cstr_ptr& gtex_name, GLuint prog);
   static bool compile_shader(Cstr_ptr& gtex_name, GLuint shader);
   static GLuint load_shader(
      Cstr_ptr& gtex_name,
      Cstr_ptr& filename,
      GLenum type);
   static bool load_shaders(
      Cstr_ptr& gtex_name,
      Cstr_list& filenames,
      ARRAY<GLuint>& shaders,
      GLenum type);

   static char* read_file(Cstr_ptr& gtex_name, Cstr_ptr& filename, GLint& len);
   static bool attach_shaders(CARRAY<GLuint>& shaders, GLuint prog);

   //******** VIRTUAL METHODS ********

   // The following methods give the names of the vertex and
   // fragment shader programs.

   // For the usual case that there is only 1 file, subclasses
   // can override the following:
   virtual str_ptr vp_filename() {
      // Base class uses jot/nprdata/glsl/simple.vp by default, but
      // that can be changed using environment variable GLSL_SHADER_VP_NAME
      return Config::get_var_str(
         "GLSL_SHADER_VP_NAME",
	 Config::JOT_ROOT() + "/nprdata/glsl/simple.vp"
	 );
   }
   virtual str_ptr fp_filename() {
      // Base class uses jot/nprdata/glsl/simple.fp by default, but
      // "that can be changed using environment variable GLSL_SHADER_FP_NAME
      return Config::get_var_str(
         "GLSL_SHADER_FP_NAME",
	 Config::JOT_ROOT() + "/nprdata/glsl/simple.fp"
	 );
   }

   // Subclasses can fill in the following two methods to return
   // a list of file names of vertex and fragment program source
   // files; but usually there is only 1 file for each, in which
   // case it's more convenient to override the single-name
   // versions, above.
   virtual str_list vp_filenames() {
      str_list ret;
      ret += vp_filename(); // override this method for single name case
      return ret;
   }
   virtual str_list fp_filenames() {
      str_list ret;
      ret += fp_filename(); // override this method for single name case
      return ret;
   }
};

/**********************************************************************
 * GLSLLightingShader:
 *
 *   Reproduces standard OpenGL lighting via GLSL shaders, in
 *   conjunction with lighting.vp and lighting.fp shaders in
 *   the jot/nprdata/glsl/ directory.
 **********************************************************************/
class GLSLLightingShader : public GLSLShader {
 public:
   //******** MANAGERS ********
   GLSLLightingShader(Patch* patch = 0);

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("GLSL Lighting",
                        GLSLLightingShader*, GLSLShader, CDATA_ITEM*);

   //******** GLSLShader VIRTUAL METHODS ********

   // using a static variable for program, so all toon shader
   // instances share the same program:
   virtual GLuint& program() { return _program; }
   virtual bool&  did_init() { return _did_init; }

   //******** GTexture VIRTUAL METHODS ********

   // Draw the patch:
   virtual int draw(CVIEWptr& v); 

   //******** DATA_ITEM VIRTUAL METHODS ********
   virtual DATA_ITEM  *dup() const { return new GLSLLightingShader; }

 private:

   static GLuint _program;  // shared by all GLSLLightingShader instances
   static bool   _did_init; // tells whether initialization attempt was made

   //******** VIRTUAL METHODS ********

   virtual str_ptr vp_filename() { return vp_name("lighting"); }
   virtual str_ptr fp_filename() { return fp_name("lighting"); }
};

#endif // GLSL_SHADER_H_IS_INCLUDED

// end of file glsl_shader.H
