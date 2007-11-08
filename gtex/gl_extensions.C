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
/*!
 *  \file gl_extensions.C
 *  \brief Contains the implementation of the Jot interface for loading OpenGL
 *  extensions.
 *
 *  \sa gl_extensions.H
 *
 */

#include "glew/glew.H"
#include "gl_extensions.H"
#include "std/config.H"

//YYY - Seems newer Redhats come with gluts that want to see these GLX extensions
//      while the latest nvidia drivers fail to provide them.  Likely, they'd
//      just be stubs anyway, so we'll just go ahead a provide them as such.
//      However, what if they do appear?! Mmmm, maybe only define these as needed...

#ifdef STUB_GLX_SGIX_video_resize

int glXBindChannelToWindowSGIX (Display *,  int, int, Window) { cerr << "gl_extensions.h::glXBindChannelToWindowSGIX() ********* STUB!!!!! *********\n"; return 0; }
int glXChannelRectSGIX (Display *, int, int, int, int, int, int) { cerr << "gl_extensions.h::glXChannelRectSGIX() ********* STUB!!!!! *********\n"; return 0; }
int glXQueryChannelRectSGIX (Display *, int, int, int *, int *, int *, int *) { cerr << "gl_extensions.h::glXQueryChannelRectSGIX() ********* STUB!!!!! *********\n"; return 0; }
int glXQueryChannelDeltasSGIX (Display *, int, int, int *, int *, int *, int *) { cerr << "gl_extensions.h::glXQueryChannelDeltasSGIX() ********* STUB!!!!! *********\n"; return 0; }
int glXChannelRectSyncSGIX (Display *, int, int, GLenum) { cerr << "gl_extensions.h::glXChannelRectSyncSGIX() ********* STUB!!!!! *********\n"; return 0; }

#endif // STUB_GLX_SGIX_video_resize

/////////////////////////////////////
// Static Variable Initialization
/////////////////////////////////////

bool           GLExtensions::_gl_arb_multitexture_supported             = false;
bool           GLExtensions::_gl_nv_register_combiners_supported        = false;
bool           GLExtensions::_gl_nv_vertex_program_supported            = false;
bool           GLExtensions::_gl_ext_compiled_vertex_array_supported    = false;
bool           GLExtensions::_gl_arb_vertex_program_supported           = false;
bool           GLExtensions::_gl_arb_fragment_program_supported         = false;
bool           GLExtensions::_gl_ati_fragment_shader_supported          = false;
bool           GLExtensions::_gl_nv_fragment_program_option_supported   = false;
bool           GLExtensions::_gl_arb_shader_objects_supported           = false;
bool           GLExtensions::_gl_arb_vertex_shader_supported            = false;
bool           GLExtensions::_gl_arb_fragment_shader_supported          = false;

bool           GLExtensions::_init = false;

bool           GLExtensions::_debug = Config::get_var_bool("DEBUG_GL_EXTENSIONS",false,true);

/////////////////////////////////////
// Constructor (never used)
/////////////////////////////////////

GLExtensions::GLExtensions()
{
   assert(0);
}

/////////////////////////////////////
// Destructor (never used)
/////////////////////////////////////

GLExtensions::~GLExtensions()
{
   assert(0);
}

/////////////////////////////////////
// init()
/////////////////////////////////////
void
GLExtensions::internal_init()
{
   assert(!_init);
   err_mesg(ERR_LEV_INFO, "GLExtensions::init() - Initializing GL extensions...");
   init_extensions();
   _init = true;
   err_mesg(ERR_LEV_INFO, "GLExtensions::init() - ...done.");
}

/////////////////////////////////////
// init_extensions()
/////////////////////////////////////

void
GLExtensions::init_extensions()
{

   _gl_arb_multitexture_supported =             init_gl_arb_multitexture();
   _gl_nv_register_combiners_supported =        init_gl_nv_register_combiners();
   _gl_nv_vertex_program_supported =            init_gl_nv_vertex_program();
   _gl_ext_compiled_vertex_array_supported =    init_gl_ext_compiled_vertex_array();
   _gl_arb_vertex_program_supported =           init_gl_arb_vertex_program();
   _gl_arb_fragment_program_supported =         init_gl_arb_fragment_program();
   _gl_ati_fragment_shader_supported =          init_gl_ati_fragment_shader();
   _gl_nv_fragment_program_option_supported =   init_gl_nv_fragment_program_option();
   _gl_arb_shader_objects_supported =           init_gl_arb_shader_objects();
   _gl_arb_vertex_shader_supported =            init_gl_arb_vertex_shader();
   _gl_arb_fragment_shader_supported =          init_gl_arb_fragment_shader();

}

/////////////////////////////////////
// init_gl_ext_compiled_vertex_array()
/////////////////////////////////////

bool
GLExtensions::init_gl_ext_compiled_vertex_array()
{
  if (Config::get_var_bool("NO_GL_EXT_compiled_vertex_array",false,true))
  {
    err_mesg(ERR_LEV_WARN, "GLExtensions: GL_EXT_compiled_vertex_array  - Just saying no!");
    return false;
  }

#ifdef GL_EXT_compiled_vertex_array

   err_mesg(ERR_LEV_INFO, "GLExtensions: GL_EXT_compiled_vertex_array is defined in glext.h!");

   if (is_extension_supported("GL_EXT_compiled_vertex_array")) 
   {
      err_mesg(ERR_LEV_INFO, "GLExtensions: GL_EXT_compiled_vertex_array is supported by hardware!");
      return true;
   }
   else 
   {
      err_mesg(ERR_LEV_INFO, "GLExtensions: GL_EXT_compiled_vertex_array is NOT supported by hardware!");
      return false;
   }
#else //GL_EXT_compiled_vertex_array
   err_mesg(ERR_LEV_WARN, "GLExtensions: GL_EXT_compiled_vertex_array is not defined in glext.h!");
   return false;
#endif //GL_EXT_compiled_vertex_array

}

/////////////////////////////////////
// init_gl_nv_vertex_program()
/////////////////////////////////////

bool
GLExtensions::init_gl_nv_vertex_program()
{
  if (Config::get_var_bool("NO_GL_NV_vertex_program",false,true))
  {
    err_mesg(ERR_LEV_WARN, "GLExtensions: GL_NV_vertex_program  - Just saying no!");
    return false;
  }
       
#ifdef GL_NV_vertex_program

   err_mesg(ERR_LEV_INFO, "GLExtensions: GL_NV_vertex_program is defined in glext.h!");

   if (is_extension_supported("GL_NV_vertex_program")) 
   {
      err_mesg(ERR_LEV_INFO, "GLExtensions: GL_NV_vertex_program is supported by hardware!");
      return true;
   }
   else
   {
      err_mesg(ERR_LEV_INFO, "GLExtensions: GL_NV_vertex_program is NOT supported by hardware!");
      return false;
   }
#else
   err_mesg(ERR_LEV_WARN, "GLExtensions: GL_NV_vertex_program is not defined in glext.h!");
   return false;
#endif

}

/////////////////////////////////////
// init_gl_arb_multitexture()
/////////////////////////////////////

bool
GLExtensions::init_gl_arb_multitexture()
{
  if (Config::get_var_bool("NO_GL_ARB_multitexture",false,true))
  {
    err_mesg(ERR_LEV_WARN, "GLExtensions: GL_ARB_multitexture  - Just saying no!");
    return false;
  }

#ifdef GL_ARB_multitexture

   err_mesg(ERR_LEV_INFO, "GLExtensions: GL_ARB_multitexture is defined in glext.h!");

   if (is_extension_supported("GL_ARB_multitexture")) 
   {
      err_mesg(ERR_LEV_INFO, "GLExtensions: GL_ARB_multitexture is supported by hardware!");
      return true;
   }
   else 
   {
      err_mesg(ERR_LEV_INFO, "GLExtensions: GL_ARB_multitexture is NOT supported by hardware!");
      return false;
   }
#else //GL_ARB_multitexture
   err_mesg(ERR_LEV_WARN, "GLExtensions: GL_ARB_multitexture is not defined in glext.h!");
   return false;
#endif //GL_ARB_multitexture

}

/////////////////////////////////////
// init_gl_nv_register_combiners()
/////////////////////////////////////

bool
GLExtensions::init_gl_nv_register_combiners()
{
   if (Config::get_var_bool("NO_GL_NV_register_combiners",false,true))
  {
    err_mesg(ERR_LEV_WARN, "GLExtensions: GL_NV_register_combiners  - Just saying no!");
    return false;
  }

#ifdef GL_NV_register_combiners

   err_mesg(ERR_LEV_INFO, "GLExtensions: GL_NV_register_combiners is defined in glext.h!");

   if (is_extension_supported("GL_NV_register_combiners")) 
   {
      err_mesg(ERR_LEV_INFO, "GLExtensions: GL_NV_register_combiners is supported by hardware!");
      return true;
   }
   else 
   {
      err_mesg(ERR_LEV_INFO, "GLExtensions: GL_NV_register_combiners is NOT supported by hardware!");
      return false;
   }
#else //GL_NV_register_combiners
   err_mesg(ERR_LEV_WARN, "GLExtensions: GL_NV_register_combiners is not defined in glext.h!");
   return false;
#endif //GL_NV_register_combiners

}


/////////////////////////////////////
// init_gl_arb_vertex_program()
/////////////////////////////////////

bool
GLExtensions::init_gl_arb_vertex_program()
{
   if (Config::get_var_bool("NO_GL_ARB_vertex_program",false,true))
  {
    err_mesg(ERR_LEV_WARN, "GLExtensions: GL_ARB_vertex_program  - Just saying no!");
    return false;
  }
       
#ifdef GL_ARB_vertex_program

   err_mesg(ERR_LEV_INFO, "GLExtensions: GL_ARB_vertex_program is defined in glext.h!");

   if (is_extension_supported("GL_ARB_vertex_program")) 
   {
      err_mesg(ERR_LEV_INFO, "GLExtensions: GL_ARB_vertex_program is supported by hardware!");
      return true;
   }
   else
   {
      err_mesg(ERR_LEV_INFO, "GLExtensions: GL_ARB_vertex_program is NOT supported by hardware!");
      return false;
   }
#else
   err_mesg(ERR_LEV_WARN, "GLExtensions: GL_ARB_vertex_program is not defined in glext.h!");
   return false;
#endif

}



/////////////////////////////////////
// init_gl_arb_vertex_program()
/////////////////////////////////////

bool
GLExtensions::init_gl_arb_fragment_program()
{
  if (Config::get_var_bool("NO_GL_ARB_fragment_program",false,true))
  {
    err_mesg(ERR_LEV_WARN, "GLExtensions: GL_ARB_fragment_program  - Just saying no!");
    return false;
  }
       
#ifdef GL_ARB_fragment_program

   err_mesg(ERR_LEV_INFO, "GLExtensions: GL_ARB_fragment_program is defined in glext.h!");

   if (is_extension_supported("GL_ARB_fragment_program")) 
   {
      err_mesg(ERR_LEV_INFO, "GLExtensions: GL_ARB_fragment_program is supported by hardware!");
      return true;
   }
   else
   {
      err_mesg(ERR_LEV_INFO, "GLExtensions: GL_ARB_fragment_program is NOT supported by hardware!");
      return false;
   }
#else
   err_mesg(ERR_LEV_WARN, "GLExtensions: GL_ARB_fragment_program is not defined in glext.h!");
   return false;
#endif

}


/////////////////////////////////////
// init_gl_ati_fragment_shader()
/////////////////////////////////////

bool
GLExtensions::init_gl_ati_fragment_shader()
{
  if (Config::get_var_bool("NO_GL_ATI_fragment_shader",false,true))
  {
    err_mesg(ERR_LEV_WARN, "GLExtensions: GL_ATI_fragment_shader  - Just saying no!");
    return false;
  }
 
#ifdef GL_ATI_fragment_shader

   err_mesg(ERR_LEV_INFO, "GLExtensions: GL_ATI_fragment_shader is defined in glext.h!");

   if (is_extension_supported("GL_ATI_fragment_shader")) 
   {
      err_mesg(ERR_LEV_INFO, "GLExtensions: GL_ATI_fragment_shader is supported by hardware!");
      return true;
   }
   else 
   {
      err_mesg(ERR_LEV_INFO, "GLExtensions: GL_ATI_fragment_shader is NOT supported by hardware!");
      return false;
   }
#else //GL_ATI_fragment_shader
   err_mesg(ERR_LEV_WARN, "GLExtensions: GL_ATI_fragment_shader is not defined in glext.h!");
   return false;
#endif //GL_ATI_fragment_shader

}

//////////////////////////////////////////
// init_gl_nv_fragment_program_option()
//////////////////////////////////////////

bool
GLExtensions::init_gl_nv_fragment_program_option()
{
   
#ifdef GL_NV_fragment_program_option

   if (is_extension_supported("GL_NV_fragment_program_option")) 
   {
      err_mesg(ERR_LEV_INFO, "GLExtensions: GL_NV_fragment_program_option is supported by hardware!");
      return true;
   }
   else 
   {
      err_mesg(ERR_LEV_INFO, "GLExtensions: GL_NV_fragment_program_option is NOT supported by hardware!");
      return false;
   }

#else // GL_NV_fragment_program_option

   err_mesg(ERR_LEV_WARN, "GLExtensions: GL_NV_fragment_program_option is not defined in glext.h!");
   return false;

#endif // GL_NV_fragment_program_option
   
}

//////////////////////////////////////
// init_gl_arb_shader_objects()
//////////////////////////////////////

bool
GLExtensions::init_gl_arb_shader_objects()
{
   
#ifdef GL_ARB_shader_objects

   if (is_extension_supported("GL_ARB_shader_objects")) 
   {
      err_mesg(ERR_LEV_INFO, "GLExtensions: GL_ARB_shader_objects is supported by hardware!");
      return true;
   }
   else 
   {
      err_mesg(ERR_LEV_INFO, "GLExtensions: GL_ARB_shader_objects is NOT supported by hardware!");
      return false;
   }

#else

   err_mesg(ERR_LEV_WARN, "GLExtensions: GL_ARB_shader_objects is not defined in glext.h!");
   return false;

#endif
   
}

//////////////////////////////////////
// init_gl_arb_vertex_shader()
//////////////////////////////////////

bool
GLExtensions::init_gl_arb_vertex_shader()
{
   
#ifdef GL_ARB_vertex_shader

   if (is_extension_supported("GL_ARB_vertex_shader") &&
       init_gl_arb_shader_objects() &&
       init_gl_arb_vertex_program()) 
   {
      err_mesg(ERR_LEV_INFO, "GLExtensions: GL_ARB_vertex_shader is supported by hardware!");
      return true;
   }
   else 
   {
      err_mesg(ERR_LEV_INFO, "GLExtensions: GL_ARB_vertex_shader is NOT supported by hardware!");
      return false;
   }

#else

   err_mesg(ERR_LEV_WARN, "GLExtensions: GL_ARB_vertex_shader is not defined in glext.h!");
   return false;

#endif
   
}

//////////////////////////////////////
// init_gl_arb_fragment_shader()
//////////////////////////////////////

bool
GLExtensions::init_gl_arb_fragment_shader()
{
   
#ifdef GL_ARB_fragment_shader

   if (is_extension_supported("GL_ARB_fragment_shader") && init_gl_arb_shader_objects()) 
   {
      err_mesg(ERR_LEV_INFO, "GLExtensions: GL_ARB_fragment_shader is supported by hardware!");
      return true;
   }
   else 
   {
      err_mesg(ERR_LEV_INFO, "GLExtensions: GL_ARB_fragment_shader is NOT supported by hardware!");
      return false;
   }

#else

   err_mesg(ERR_LEV_WARN, "GLExtensions: GL_ARB_fragment_shader is not defined in glext.h!");
   return false;

#endif
   
}

/////////////////////////////////////
// is_extension_supported
/////////////////////////////////////

bool
GLExtensions::is_extension_supported(const char *extension)
{
   
   return GLEWSingleton::Instance().is_supported(extension);
   
}


/////////////////////////////////////
// gl_arb_vertex_program_loaded()
/////////////////////////////////////
bool
GLExtensions::gl_arb_vertex_program_loaded(
   Cstr_ptr &header,
   bool &native, 
   const unsigned char *prog)
{
   bool success;

   if (!gl_arb_vertex_program_supported())
   {
      err_mesg(ERR_LEV_INFO, "%sGL_ARB_vertex_program not supported.", **header);
      native = false;
      success = false;
   }
   else
   {
#ifdef GL_ARB_vertex_program

      int i;
      unsigned char *err_buf1, *err_buf2;
      const GLubyte *err_str;
      GLint err_pos;
      GLint is_native;
      
      GLenum err = glGetError();

      err_mesg(ERR_LEV_INFO, "%sLoading program...", **header);

      err_str = glGetString( GL_PROGRAM_ERROR_STRING_ARB );
      glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &err_pos );
      glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB, &is_native);      

      native = (is_native != 0);

      if (err_pos != -1)
      {
         int prog_len = strlen((const char *)prog);   assert(err_pos <= prog_len);

         int err_left = err_pos, err_right = err_pos;

         while ((err_left  > 0) && 
                  (prog[err_left-1] != '\n' &&
                   prog[err_left-1] != ';'  && 
                   prog[err_left-1] != '\r'))    err_left--;
         while ((err_right < prog_len) && 
                  (prog[err_right+1] != '\n' && 
                   prog[err_right]   != ';'  && 
                   prog[err_right+1] != '\r' && 
                   prog[err_right+1] !=  0 ))    err_right++;

         err_buf1 = new unsigned char [err_right - err_left + 2]; assert(err_buf1);
         err_buf2 = new unsigned char [err_right - err_left + 2]; assert(err_buf2);

         for (i=0; i<(err_right-err_left+1); i++) 
         {
            err_buf1[i] = prog[err_left+i];
            err_buf2[i] = ' ';
         }
         err_buf2[err_pos-err_left]='^';

         err_buf1[i] = 0;
         err_buf2[i] = 0;

      }

      if ( err == GL_INVALID_OPERATION )
      {
         success = false;

         //assert(!native);

         assert(err_str && (err_str[0] != 0));

         err_mesg(ERR_LEV_ERROR, "%s*** Failed! ***", **header);
         
         if (err_pos != -1)
         {
            err_mesg(ERR_LEV_ERROR, "%sERROR LOCATION: %d",  **header, err_pos); 
            err_mesg(ERR_LEV_ERROR, "%sERROR EXCERPT: [%s]", **header, err_buf1);
            err_mesg(ERR_LEV_ERROR, "%s               [%s]", **header, err_buf2);
         }

         err_mesg(ERR_LEV_ERROR, "%sERROR STRING: '%s'", **header, err_str);
      }
      else
      {
         success = true;

         if (native)
            err_mesg(ERR_LEV_INFO, "%sWill execute natively in hardware.", **header);
         else
            err_mesg(ERR_LEV_WARN, "%sWill execute **BUT** not natively in hardware. Using emulation...", **header);

         if (err_pos != -1)
         {
            err_mesg(ERR_LEV_WARN, "%sWARNING LOCATION: %d",  **header, err_pos); 
            err_mesg(ERR_LEV_WARN, "%sWARNING EXCERPT: [%s]", **header, err_buf1);
            err_mesg(ERR_LEV_WARN, "%s                 [%s]", **header, err_buf2);
         }

         if (err_str && (err_str[0] != 0))
            err_mesg(ERR_LEV_WARN, "%sWARNING STRING: '%s'", **header, err_str);

      }

      if (err_pos != -1)
      {
         delete[] err_buf1;
         delete[] err_buf2;
      }

      GLint ins_num, ins_max, nins_num, nins_max;
      glGetProgramivARB(GL_VERTEX_PROGRAM_ARB,  GL_PROGRAM_INSTRUCTIONS_ARB,              &ins_num);      
      glGetProgramivARB(GL_VERTEX_PROGRAM_ARB,  GL_MAX_PROGRAM_INSTRUCTIONS_ARB,          &ins_max);      
      glGetProgramivARB(GL_VERTEX_PROGRAM_ARB,  GL_PROGRAM_NATIVE_INSTRUCTIONS_ARB,       &nins_num);      
      glGetProgramivARB(GL_VERTEX_PROGRAM_ARB,  GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB,   &nins_max);      

      GLint tmp_num, tmp_max, ntmp_num, ntmp_max;
      glGetProgramivARB(GL_VERTEX_PROGRAM_ARB,  GL_PROGRAM_TEMPORARIES_ARB,              &tmp_num);      
      glGetProgramivARB(GL_VERTEX_PROGRAM_ARB,  GL_MAX_PROGRAM_TEMPORARIES_ARB,          &tmp_max);      
      glGetProgramivARB(GL_VERTEX_PROGRAM_ARB,  GL_PROGRAM_NATIVE_TEMPORARIES_ARB,       &ntmp_num);      
      glGetProgramivARB(GL_VERTEX_PROGRAM_ARB,  GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB,   &ntmp_max);      
    
      GLint par_num, par_max, npar_num, npar_max;
      glGetProgramivARB(GL_VERTEX_PROGRAM_ARB,  GL_PROGRAM_PARAMETERS_ARB,              &par_num);      
      glGetProgramivARB(GL_VERTEX_PROGRAM_ARB,  GL_MAX_PROGRAM_PARAMETERS_ARB,          &par_max);      
      glGetProgramivARB(GL_VERTEX_PROGRAM_ARB,  GL_PROGRAM_NATIVE_PARAMETERS_ARB,       &npar_num);      
      glGetProgramivARB(GL_VERTEX_PROGRAM_ARB,  GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB,   &npar_max);      

      GLint att_num, att_max, natt_num, natt_max;
      glGetProgramivARB(GL_VERTEX_PROGRAM_ARB,  GL_PROGRAM_ATTRIBS_ARB,              &att_num);      
      glGetProgramivARB(GL_VERTEX_PROGRAM_ARB,  GL_MAX_PROGRAM_ATTRIBS_ARB,          &att_max);      
      glGetProgramivARB(GL_VERTEX_PROGRAM_ARB,  GL_PROGRAM_NATIVE_ATTRIBS_ARB,       &natt_num);      
      glGetProgramivARB(GL_VERTEX_PROGRAM_ARB,  GL_MAX_PROGRAM_NATIVE_ATTRIBS_ARB,   &natt_max);      

      GLint add_num, add_max, nadd_num, nadd_max;
      glGetProgramivARB(GL_VERTEX_PROGRAM_ARB,  GL_PROGRAM_ADDRESS_REGISTERS_ARB,              &add_num);      
      glGetProgramivARB(GL_VERTEX_PROGRAM_ARB,  GL_MAX_PROGRAM_ADDRESS_REGISTERS_ARB,          &add_max);      
      glGetProgramivARB(GL_VERTEX_PROGRAM_ARB,  GL_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB,       &nadd_num);      
      glGetProgramivARB(GL_VERTEX_PROGRAM_ARB,  GL_MAX_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB,   &nadd_max);      

      err_mesg(ERR_LEV_SPAM, "%sResource Usage:", **header);
      err_mesg(ERR_LEV_SPAM, "%s  Instructions (ARB): MAX=%d  USED=%d", **header, ins_max,   ins_num);
      err_mesg(ERR_LEV_SPAM, "%s            (NATIVE): MAX=%d  USED=%d", **header, nins_max,  nins_num);
      err_mesg(ERR_LEV_SPAM, "%s  Temporaries  (ARB): MAX=%d  USED=%d", **header, tmp_max,   tmp_num);
      err_mesg(ERR_LEV_SPAM, "%s            (NATIVE): MAX=%d  USED=%d", **header, ntmp_max,  tmp_num);
      err_mesg(ERR_LEV_SPAM, "%s  Parameters   (ARB): MAX=%d  USED=%d", **header, par_max,   par_num);
      err_mesg(ERR_LEV_SPAM, "%s            (NATIVE): MAX=%d  USED=%d", **header, npar_max,  npar_num);
      err_mesg(ERR_LEV_SPAM, "%s  Attributes   (ARB): MAX=%d  USED=%d", **header, att_max,   att_num);
      err_mesg(ERR_LEV_SPAM, "%s            (NATIVE): MAX=%d  USED=%d", **header, natt_max,  natt_num);
      err_mesg(ERR_LEV_SPAM, "%s  Addressors   (ARB): MAX=%d  USED=%d", **header, add_max,   add_num);
      err_mesg(ERR_LEV_SPAM, "%s            (NATIVE): MAX=%d  USED=%d", **header, nadd_max,  nadd_num);
      err_mesg(ERR_LEV_SPAM, "%s...done.", **header);
#endif
   }
   return success;
}


/////////////////////////////////////
// gl_arb_fragment_program_loaded()
/////////////////////////////////////
bool
GLExtensions::gl_arb_fragment_program_loaded(
   Cstr_ptr &header,
   bool &native, 
   const unsigned char *prog)
{
   bool success;

   if (!gl_arb_fragment_program_supported())
   {
      err_mesg(ERR_LEV_INFO, "%GL_ARB_fragment_program not supported.", **header);
      native = false;
      success = false;
   }
   else
   {
#ifdef GL_ARB_fragment_program

      int i;
      unsigned char *err_buf1, *err_buf2;
      const GLubyte *err_str;
      GLint err_pos;
      GLint is_native;
      
      GLenum err = glGetError();

      err_mesg(ERR_LEV_INFO, "%sLoading program...", **header);

      err_str = glGetString( GL_PROGRAM_ERROR_STRING_ARB );
      glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &err_pos );
      glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB, &is_native);      

      native = (is_native != 0);

      if (err_pos != -1)
      {
         int prog_len = strlen((const char *)prog);   assert(err_pos <= prog_len);

         int err_left = err_pos, err_right = err_pos;

         while ((err_left  > 0) && 
                  (prog[err_left-1] != '\n' &&
                   prog[err_left-1] != ';'  && 
                   prog[err_left-1] != '\r'))    err_left--;
         while ((err_right < prog_len) && 
                  (prog[err_right+1] != '\n' && 
                   prog[err_right]   != ';'  && 
                   prog[err_right+1] != '\r' && 
                   prog[err_right+1] !=  0 ))    err_right++;

         err_buf1 = new unsigned char [err_right - err_left + 2]; assert(err_buf1);
         err_buf2 = new unsigned char [err_right - err_left + 2]; assert(err_buf2);

         for (i=0; i<(err_right-err_left+1); i++) 
         {
            err_buf1[i] = prog[err_left+i];
            err_buf2[i] = ' ';
         }
         err_buf2[err_pos-err_left]='^';

         err_buf1[i] = 0;
         err_buf2[i] = 0;

      }

      if ( err == GL_INVALID_OPERATION )
      {
         success = false;

         //assert(!native);

         assert(err_str && (err_str[0] != 0));

         err_mesg(ERR_LEV_ERROR, "%s*** Failed! ***", **header);
         
         if (err_pos != -1)
         {
            err_mesg(ERR_LEV_ERROR, "%sERROR LOCATION: %d",  **header, err_pos); 
            err_mesg(ERR_LEV_ERROR, "%sERROR EXCERPT: [%s]", **header, err_buf1);
            err_mesg(ERR_LEV_ERROR, "%s               [%s]", **header, err_buf2);
         }

         err_mesg(ERR_LEV_ERROR, "%sERROR STRING: '%s'", **header, err_str);

      }
      else
      {
         success = true;

         if (native)
            err_mesg(ERR_LEV_INFO, "%sWill execute natively in hardware.", **header);
         else
            err_mesg(ERR_LEV_WARN, "%sWill execute **BUT** not natively in hardware. Using emulation...", **header);

         if (err_pos != -1)
         {
            err_mesg(ERR_LEV_WARN, "%sWARNING LOCATION: %d",  **header, err_pos); 
            err_mesg(ERR_LEV_WARN, "%sWARNING EXCERPT: [%s]", **header, err_buf1);
            err_mesg(ERR_LEV_WARN, "%s                 [%s]", **header, err_buf2);
         }

         if (err_str && (err_str[0] != 0))
            err_mesg(ERR_LEV_WARN, "%sWARNING STRING: '%s'", **header, err_str);

      }

      if (err_pos != -1)
      {
         delete[] err_buf1;
         delete[] err_buf2;
      }

      GLint ins_num, ins_max, nins_num, nins_max;
      glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,  GL_PROGRAM_INSTRUCTIONS_ARB,              &ins_num);      
      glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,  GL_MAX_PROGRAM_INSTRUCTIONS_ARB,          &ins_max);      
      glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,  GL_PROGRAM_NATIVE_INSTRUCTIONS_ARB,       &nins_num);      
      glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,  GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB,   &nins_max);      

      GLint ains_num, ains_max, nains_num, nains_max;
      glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,  GL_PROGRAM_ALU_INSTRUCTIONS_ARB,              &ains_num);      
      glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,  GL_MAX_PROGRAM_ALU_INSTRUCTIONS_ARB,          &ains_max);      
      glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,  GL_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB,       &nains_num);      
      glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,  GL_MAX_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB,   &nains_max);      

      GLint tins_num, tins_max, ntins_num, ntins_max;
      glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,  GL_PROGRAM_TEX_INSTRUCTIONS_ARB,              &tins_num);      
      glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,  GL_MAX_PROGRAM_TEX_INSTRUCTIONS_ARB,          &tins_max);      
      glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,  GL_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB,       &ntins_num);      
      glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,  GL_MAX_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB,   &ntins_max);      

      GLint txns_num, txns_max, ntxns_num, ntxns_max;
      glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,  GL_PROGRAM_TEX_INDIRECTIONS_ARB,              &txns_num);      
      glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,  GL_MAX_PROGRAM_TEX_INDIRECTIONS_ARB,          &txns_max);      
      glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,  GL_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB,       &ntxns_num);      
      glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,  GL_MAX_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB,   &ntxns_max);      

      GLint tmp_num, tmp_max, ntmp_num, ntmp_max;
      glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,  GL_PROGRAM_TEMPORARIES_ARB,              &tmp_num);      
      glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,  GL_MAX_PROGRAM_TEMPORARIES_ARB,          &tmp_max);      
      glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,  GL_PROGRAM_NATIVE_TEMPORARIES_ARB,       &ntmp_num);      
      glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,  GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB,   &ntmp_max);      
    
      GLint par_num, par_max, npar_num, npar_max;
      glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,  GL_PROGRAM_PARAMETERS_ARB,              &par_num);      
      glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,  GL_MAX_PROGRAM_PARAMETERS_ARB,          &par_max);      
      glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,  GL_PROGRAM_NATIVE_PARAMETERS_ARB,       &npar_num);      
      glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,  GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB,   &npar_max);      

      GLint att_num, att_max, natt_num, natt_max;
      glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,  GL_PROGRAM_ATTRIBS_ARB,              &att_num);      
      glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,  GL_MAX_PROGRAM_ATTRIBS_ARB,          &att_max);      
      glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,  GL_PROGRAM_NATIVE_ATTRIBS_ARB,       &natt_num);      
      glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,  GL_MAX_PROGRAM_NATIVE_ATTRIBS_ARB,   &natt_max);      

      err_mesg(ERR_LEV_SPAM, "%sResource Usage:", **header);
      err_mesg(ERR_LEV_SPAM, "%s  Instructions - ALL (ARB): MAX=%d  USED=%d",    **header, ins_max,   ins_num);
      err_mesg(ERR_LEV_SPAM, "%s                  (NATIVE): MAX=%d  USED=%d",    **header, nins_max,  nins_num);
      err_mesg(ERR_LEV_SPAM, "%s  Instructions - ALU (ARB): MAX=%d  USED=%d",    **header, ains_max,  ains_num);
      err_mesg(ERR_LEV_SPAM, "%s                  (NATIVE): MAX=%d  USED=%d",    **header, nains_max, nains_num);
      err_mesg(ERR_LEV_SPAM, "%s  Instructions - TEX (ARB): MAX=%d  USED=%d",    **header, tins_max,  tins_num);
      err_mesg(ERR_LEV_SPAM, "%s                  (NATIVE): MAX=%d  USED=%d",    **header, ntins_max, ntins_num);
      err_mesg(ERR_LEV_SPAM, "%s  Tex Indirections (ARB): MAX=%d  USED=%d",      **header, txns_max,  txns_num);
      err_mesg(ERR_LEV_SPAM, "%s                (NATIVE): MAX=%d  USED=%d",      **header, ntxns_max, ntxns_num);
      err_mesg(ERR_LEV_SPAM, "%s  Temporaries  (ARB): MAX=%d  USED=%d",          **header, tmp_max,   tmp_num);
      err_mesg(ERR_LEV_SPAM, "%s            (NATIVE): MAX=%d  USED=%d",          **header, ntmp_max,  tmp_num);
      err_mesg(ERR_LEV_SPAM, "%s  Parameters   (ARB): MAX=%d  USED=%d",          **header, par_max,   par_num);
      err_mesg(ERR_LEV_SPAM, "%s            (NATIVE): MAX=%d  USED=%d",          **header, npar_max,  npar_num);
      err_mesg(ERR_LEV_SPAM, "%s  Attributes   (ARB): MAX=%d  USED=%d",          **header, att_max,   att_num);
      err_mesg(ERR_LEV_SPAM, "%s            (NATIVE): MAX=%d  USED=%d",          **header, natt_max,  natt_num);
      err_mesg(ERR_LEV_SPAM, "%s...done.", **header);
#endif
   }
   return success;
}
