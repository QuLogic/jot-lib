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
#ifndef GL_EXTENSIONS_H_IS_INCLUDED
#define GL_EXTENSIONS_H_IS_INCLUDED

/*!
 *  \file gl_extensions.H
 *  \brief To use OpenGL extensions, just include this header.
 *
 *  \sa gl_extensions.C
 *
 */

#include "std/support.H"
#include "glew/glew.H"

/*!
 *  \brief Jot interface for loading OpenGL extensions.
 *
 *  Statically constructed at run-time and initializes
 *  a set of extensions to GL.  The public queries will
 *  return the availability of the extensions, but
 *  the calls to extensions must still be placed in
 *  #ifdef's. e.g.
 *
 *      if (gl_arb_multitexture_suported())
 *      { 
 *      #ifdef GL_ARB_multitexture
 *           do_stuff(); 
 *      #endif
 *      }
 *
 *  \note The general is_extension_supported() is
 *  not exposed, since win32 requires that extension
 *  functions be declared as pointers, and fetched from
 *  the opengl icd.  The baked in extension queries in this
 *  class indicate if an extension is supported AND that
 *  the functions are ready to call.
 *
 */
class GLExtensions {
 private:
   /******** STATIC MEMBER VARIABLES ********/   
 
   static bool _gl_arb_multitexture_supported;
   static bool _gl_nv_register_combiners_supported;
   static bool _gl_nv_vertex_program_supported;
   static bool _gl_ext_compiled_vertex_array_supported;
   static bool _gl_arb_vertex_program_supported;
   static bool _gl_arb_fragment_program_supported;
   static bool _gl_ati_fragment_shader_supported;
   static bool _gl_nv_fragment_program_option_supported;
   static bool _gl_arb_shader_objects_supported;
   static bool _gl_arb_vertex_shader_supported;
   static bool _gl_arb_fragment_shader_supported;

   static bool _init;

   static bool _debug;

   /******** CONSTRUCTOR/DECONSTRUCTOR *******/
   GLExtensions();

 public:
   ~GLExtensions();
 private:

   /******** STATIC MEMBER METHODS ********/

   static void internal_init();

   static void init_extensions();

   static bool init_gl_arb_multitexture();
   static bool init_gl_nv_register_combiners();
   static bool init_gl_nv_vertex_program();
   static bool init_gl_ext_compiled_vertex_array();
   static bool init_gl_arb_vertex_program();
   static bool init_gl_arb_fragment_program();
   static bool init_gl_ati_fragment_shader();
   static bool init_gl_nv_fragment_program_option();
   static bool init_gl_arb_shader_objects();
   static bool init_gl_arb_vertex_shader();
   static bool init_gl_arb_fragment_shader();

   static bool is_extension_supported(const char *extension);

 public:
   static void set_debug(bool d) { _debug = d;     }
   static bool get_debug()       { return _debug;  }

   static void init() { if (!_init) internal_init(); }; 

   static bool gl_arb_multitexture_supported() 
                  { init(); return _gl_arb_multitexture_supported; }
   static bool gl_nv_register_combiners_supported() 
                  { init(); return _gl_nv_register_combiners_supported; }
   static bool gl_nv_vertex_program_supported() 
                  { init(); return _gl_nv_vertex_program_supported; }
   static bool gl_ext_compiled_vertex_array_supported() 
                  { init(); return _gl_ext_compiled_vertex_array_supported; }
   static bool gl_arb_vertex_program_supported() 
                  { init(); return _gl_arb_vertex_program_supported; }
   static bool gl_arb_fragment_program_supported() 
                  { init(); return _gl_arb_fragment_program_supported; }
   static bool gl_ati_fragment_shader_supported() 
                  { init(); return _gl_ati_fragment_shader_supported; }
   static bool gl_nv_fragment_program_option_supported()
                  { init(); return _gl_nv_fragment_program_option_supported; }
   static bool gl_arb_shader_objects_supported()
                  { init(); return _gl_arb_shader_objects_supported; }
   static bool gl_arb_vertex_shader_supported()
                  { init(); return _gl_arb_vertex_shader_supported; }
   static bool gl_arb_fragment_shader_supported()
                  { init(); return _gl_arb_fragment_shader_supported; }

   
   static bool gl_arb_vertex_program_loaded(Cstr_ptr &,bool &, const unsigned char *);
   static bool gl_arb_fragment_program_loaded(Cstr_ptr &,bool &, const unsigned char *);
};

#endif // GL_EXTENSIONS_H_IS_INCLUDED
