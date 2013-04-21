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
 * glsl_shader.C
 *
 *****************************************************************/
#include "gtex/gl_extensions.H"
#include "gtex/util.H"
#include "geom/gl_util.H"
#include "mesh/patch.H"
#include "paper_effect.H"
#include "glsl_shader.H"

using namespace mlib;

static bool debug = Config::get_var_bool("DEBUG_GLSL_SHADER", false);

// the following tells whether GLSL is available via extensions
// (e.g. in OpenGL 1.5). It will be initialized in init():
static bool need_arb_ext = false;

// default silhouette parameters:
const GLfloat SIL_WIDTH = 10.0f;
const COLOR   SIL_COLOR = Color::white;
const GLfloat SIL_ALPHA = 0.5f;

GLSLShader::GLSLShader(Patch* p, StripCB* cb) :
   BasicTexture(p, cb ? cb : new VertNormStripCB),
   _program(0),
   _did_init(false),
   _draw_sils(false),
   _sil_width(SIL_WIDTH),
   _sil_color(SIL_COLOR),
   _sil_alpha(SIL_ALPHA)
{
}

GLSLShader::~GLSLShader()
{ 
   delete_program(_program);
}

void
GLSLShader::delete_program(GLuint& prog)
{
   if (prog == 0)
      return;
   if (need_arb_ext) {
      glDeleteObjectARB(prog);
   } else {
      glDeleteProgram(prog);
   }
   prog = 0;
}

void
GLSLShader::print_shader_source(GLuint shader)
{
   GLint src_len = 0;
   glGetObjectParameterivARB(shader, GL_OBJECT_SHADER_SOURCE_LENGTH_ARB,
                             &src_len);
   if (src_len < 1) {
      cerr << "  source length is 0" << endl;
      return;
   }
   GLcharARB* buf = new GLcharARB [ src_len ];
   GLint chars_written = 0;
   glGetShaderSourceARB(shader, src_len, &chars_written, buf);
   cerr << buf << endl;
}

void
GLSLShader::print_info(Cstr_ptr& gtex_name, GLuint obj)
{
   cerr << gtex_name << ": print_info: checking object " << obj << endl;
   GLint log_len = 0;
   GL_VIEW::print_gl_errors(gtex_name + "::print_info:");
   glGetObjectParameterivARB(obj, GL_OBJECT_INFO_LOG_LENGTH_ARB, &log_len);
   if (log_len < 1) {
      cerr << " print_info: log length is 0" << endl;
      return;
   }
   GLcharARB* buf = new GLcharARB [ log_len ];
   GLint chars_written = 0;
   glGetInfoLogARB(obj, log_len, &chars_written, buf);
   cerr << "info log: " << endl
        << buf << endl;
   delete buf;
}

bool
GLSLShader::link_program(Cstr_ptr& gtex_name, GLuint prog)
{
   GLint status = 0;
   if (need_arb_ext) {
      glLinkProgramARB(prog);
      glGetObjectParameterivARB(prog, GL_OBJECT_LINK_STATUS_ARB, &status);
   } else {
      glLinkProgram(prog);
      glGetProgramiv(prog, GL_LINK_STATUS, &status);
   }
   if (status != GL_TRUE)
      cerr << gtex_name << ": link_program: link failed" << endl;

   // print log messages about the error:
   const GLsizei BUF_SIZE = 4096;
   char info_log[BUF_SIZE] = {0};
   GLsizei len=0;
   if (need_arb_ext) {
      // todo...
   } else {
      glGetProgramInfoLog(prog, BUF_SIZE, &len, info_log);
   }
   cerr << gtex_name
        << ": ProgramInfoLog: "
        << endl
        << info_log
        << endl;

   if (debug && need_arb_ext) {
      glValidateProgramARB(prog);
      GLint ret = 0;
      glGetObjectParameterivARB(prog, GL_OBJECT_VALIDATE_STATUS_ARB, &ret);
      cerr << "program " << prog
           << " validation status: " << ret << endl;
      print_info(gtex_name, prog);
   }
   return (status == GL_TRUE);
}

bool
GLSLShader::compile_shader(Cstr_ptr& gtex_name, GLuint shader)
{
   GLint status = 0;
   if (need_arb_ext) {
      glCompileShaderARB(shader);
      glGetObjectParameterivARB(shader, GL_OBJECT_COMPILE_STATUS_ARB, &status);
      if (debug)
         print_info(gtex_name, shader);
   } else {
      glCompileShader(shader);
      glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
   }
   return (status == GL_TRUE);
}

char*
GLSLShader::read_file(
  Cstr_ptr& gtex_name,
  Cstr_ptr& filename,
  GLint& length
)
{
   // Read characters from file into buffer
   char* buf = 0;
   ifstream in(**filename, ifstream::in | ifstream::binary);
   if (in.is_open()) {
      in.seekg(0, ios::end);
      length = in.tellg();
      buf = new char[length + 1];
      memset(buf, 0, length + 1);
      in.seekg(0, ios::beg);
      in.read(buf, length);
      in.close();
   } else {
      cerr << gtex_name << "::read_file: could not read file: "
           << filename
           << endl;
   }
   return buf;
}

void
GLSLShader::delete_shader(GLuint shader)
{
   if (need_arb_ext) {
      glDeleteObjectARB(shader);
   } else {
      glDeleteShader(shader);
   }
}

void
GLSLShader::delete_shaders(CARRAY<GLuint>& shaders)
{
   for (int i=0; i<shaders.num(); i++)
      delete_shader(shaders[i]);
}

GLuint
GLSLShader::load_shader(Cstr_ptr& gtex_name, Cstr_ptr& filename, GLenum type)
{
   // Read characters from file into dynamically allocated buffer:
   GLint length = 0;
   char* buf = read_file(gtex_name, filename, length);
   if (!buf)
      return 0;

   // Allocate shader object, attach the source code, and compile:
   GLuint shader = 0;
   if (need_arb_ext) {
      shader = glCreateShaderObjectARB(type);
      glShaderSourceARB(shader, 1, (const GLcharARB**)&buf, &length);
   } else {
      shader = glCreateShader(type);
      glShaderSource(shader, 1, (const GLchar**)&buf, &length);
   }
   delete [] buf;
   if (compile_shader(gtex_name, shader)) {
      if (debug) {
         const GLsizei BUF_SIZE = 4096;
         char info_log[BUF_SIZE] = {0};
         GLsizei len;
         if (need_arb_ext) {
            glGetInfoLogARB(shader, BUF_SIZE, &len, info_log);
         } else {
            glGetShaderInfoLog(shader, BUF_SIZE, &len, info_log);
         }
         cerr << gtex_name
              << "ShaderInfoLog for shader: "
              << filename
              << endl
              << info_log
              << endl;
      }
      return shader;
   }

   // failed to compile
   cerr << gtex_name
        << "::load_shader: compile failed for file: "
        << filename
        << endl;

   // print log messages about the error:
   const GLsizei BUF_SIZE = 4096;
   char info_log[BUF_SIZE] = {0};
   GLsizei len=0;
   if (need_arb_ext) {
      glGetInfoLogARB(shader, BUF_SIZE, &len, info_log);
   } else {
      glGetShaderInfoLog(shader, BUF_SIZE, &len, info_log);
   }
   cerr << gtex_name << ": ShaderInfoLog: "
        << endl
        << info_log
        << endl;

   delete_shader(shader);
   return 0;
}

bool
GLSLShader::load_shaders(
   Cstr_ptr& gtex_name,
   Cstr_list& filenames,
   ARRAY<GLuint>& shaders,
   GLenum type)
{
   for (int i=0; i<filenames.num(); i++) {
      GLuint shader = load_shader(gtex_name, filenames[i], type);
      if (shader == 0) {
         cerr << gtex_name
              << "::load_shaders: failed to load file: "
              << filenames[i]
              << endl;
         // if one failed, delete them all:
         delete_shaders(shaders);
         return false;
      }
      if (debug && need_arb_ext) {
         cerr << gtex_name
              << ": print_shader_source: checking shader " << shader << endl;

         print_shader_source(shader);
      }
      shaders += shader;
   }
   // returns true even if the list of filenames is empty:
   return true; 
}

bool
GLSLShader::attach_shaders(CARRAY<GLuint>& shaders, GLuint prog)
{
   if (need_arb_ext) {
      for (int i=0; i<shaders.num(); i++)
         glAttachObjectARB(prog, shaders[i]);
   } else {
      for (int i=0; i<shaders.num(); i++)
         glAttachShader(prog, shaders[i]);
   }
   return true;
}

bool
GLSLShader::init()
{
   // Only do things once.
   if (did_init())
      return program() != 0;
   did_init() = true;

   if (GLEW_VERSION_2_0) {
      // In OpenGL 2.0, GLSL is available in the core spec
      if (debug) cerr << class_name() << "::init: OpenGL 2.0" << endl;
   } else if (GLEW_VERSION_1_5) {
      // In OpenGL 1.5, GLSL is available via extensions
      need_arb_ext = true;
      if (debug) cerr << class_name() << "::init: OpenGL 1.5" << endl;
   } else {
      // In earlier OpenGL GLSL is not available
      if (debug) cerr << class_name()
                      << "::init: pre-OpenGL 1.5: can't do GLSL" << endl;
      return false;
   }

   // Create the program object:
   if (need_arb_ext) {
      program() = glCreateProgramObjectARB();
   } else {
      GL_VIEW::print_gl_errors("GLSLShader::init");
      program() = glCreateProgram();
   }

   if (!program()) {
      // should never happen
      cerr << class_name() << "::init: error: glCreateProgram failed" << endl;
      return false;
   }

   // Read shaders from files, compile them, 
   // attach them to the program, and link...
   ARRAY<GLuint> shaders;
   GLenum vp, fp;
   if (need_arb_ext) {
      vp = GL_VERTEX_SHADER_ARB;
      fp = GL_FRAGMENT_SHADER_ARB;
   } else {
      vp = GL_VERTEX_SHADER;
      fp = GL_FRAGMENT_SHADER;
   }

   if (load_shaders(class_name(), vp_filenames(), shaders, vp) &&
       load_shaders(class_name(), fp_filenames(), shaders, fp) &&
       attach_shaders(shaders, program()) && bind_attributes(program()) &&
       link_program(class_name(), program())) {

 
      // report success if debugging:
      if (debug)
         cerr << class_name() << "::init: loaded program" << endl;
      return true;
   }

   // Failure: delete program
   cerr << class_name() << "::init: could not load program" << endl;
   delete_program(program());

   return false;
}

void
GLSLShader::use_program(GLuint prog)
{
   if (need_arb_ext) {
      glUseProgramObjectARB(prog);
   } else {
      glUseProgram(prog);
   }
}

bool
GLSLShader::get_uniform_loc(Cstr_ptr& var_name, GLint& loc) 
{
   // wrapper for glGetUniformLocation, with error reporting

   loc = glGetUniformLocation(program(), **var_name);
   if ((loc < 0)&&(debug)) {
      cerr << class_name() << "::get_uniform_loc: error: variable "
           << "\"" << var_name << "\" not found"
           << endl;
      return false;
   }
   return true;
}

bool
GLSLShader::load_texture(TEXTUREglptr& tex)
{
   // Load the texture (should be already allocated TEXTUREgl
   // with filename set as member variable).
   //
   // Note: Does not activate the texture for use in the current frame.
   //
   // This is a no-op of the texture was previously loaded.

   if (!tex) {
      return false;
   } else if (tex->is_valid()) {
      return true;
   } else if (tex->load_attempt_failed()) {
      // we previously tried to load this texture and failed.
      // no sense trying again with the same filename.
      return false;
   }

   return tex->load_texture();
}

bool
GLSLShader::activate_texture(TEXTUREglptr& tex)
{
   // Load (if needed) and activate the texture.

   if (!load_texture(tex)) {
      bool debug=true;
      if (debug) {
         cerr << "GLSLShader::activate_texture: can't load texture: "
              << (tex ? tex->file() : "null pointer") 
              <<  " at texture unit "
              << (tex ?  tex->get_raw_unit() : -1) << endl;
      }
      return false;
   }

   tex->apply_texture();

   if (debug) 
      GL_VIEW::print_gl_errors(
         str_ptr("glsl_shader::activate_texture: activating textuire ") +
         str_ptr(tex ? tex->file() : "null pointer") +
         str_ptr(" at unit ") +
         str_ptr((int)tex->get_raw_unit()) );

   return true;
}

void
GLSLShader::set_gl_state(GLbitfield mask) const
{
   // default behavior that is probably useful for many GTextures.
   // (otherwise, over-ride this method!)

   // push GL state before changing things

   mask = mask | GL_ENABLE_BIT | GL_CURRENT_BIT;
   if (_draw_sils) {
      // set line width for silhouettes:
      // (line scale could be ignored in most cases, except when rendering
      // high-resolution output images thru multiple passes...):
      GLfloat w = GLfloat(VIEW::peek()->line_scale())*_sil_width;

      // XXX - under construction
      bool antialias = false;
      if (antialias) {
         // push attributes, enable line smoothing, and set line width.
         // default behavior: always use antialiasing.
         GL_VIEW::init_line_smooth(w, mask);
      } else {
         // push attributes and set line width
         glPushAttrib(mask);
         glLineWidth(w); // GL_LINE_BIT
      }
   } else {
      glPushAttrib(mask);
   }

   set_face_culling();                  // GL_ENABLE_BIT

     // set the color from the patch:
   if (_patch)
      GL_COL(_patch->color(), alpha());  // GL_CURRENT_BIT
}

void
GLSLShader::restore_gl_state() const
{
   glPopAttrib();
}

int
GLSLShader::draw(CVIEWptr& v)
{
   GL_VIEW::print_gl_errors(class_name() + "::draw: start");

   // Ensure program is loaded:
   if (!init())
      return 0;
   assert(program());

   // Load textures (if any) and set their parameters.
   // This is a no-op after the first time.
   //
   // Q: Why not do it in the constructor?
   // A: Some textures get created but not drawn, so we
   //    prefer to wait until we're sure we need the textures
   //    before requesting resources from OpenGL.
   init_textures();
   if (debug)
      GL_VIEW::print_gl_errors(class_name() + "::draw: init textures");

   // call glPushAttrib() and set desired state 
   set_gl_state();
   if (debug)
      GL_VIEW::print_gl_errors(class_name() + "::draw: push attrib");

   // activate textures, if any:
   activate_textures(); // GL_ENABLE_BIT
   if (debug)
      GL_VIEW::print_gl_errors(class_name() + "::draw: activate textures");

   // activate program:
   activate_program();
   if (debug)
      GL_VIEW::print_gl_errors(class_name() + "::draw: activate program");

   // query variable locations and store the results:
   get_variable_locs();
  
   // send values to uniform variables:
   set_uniform_variables();
   if (debug) {
      GL_VIEW::print_gl_errors(class_name() + "::draw: set uniform variables");
   }

   // now draw the triangles using a display list.
   // execute display list if it's valid:
   if (BasicTexture::dl_valid(v)) {
      BasicTexture::draw(v);
   } else {
      // try to generate a display list
      int dl = _dl.get_dl(v, 1, _patch->stamp());
      if (dl)
         glNewList(dl, GL_COMPILE);

      // draw the triangle strips
      draw_triangles();
      // end the display list here
      if (_dl.dl(v)) {
         _dl.close_dl(v);
         BasicTexture::draw(v);
      }
   }

   if (debug)
      GL_VIEW::print_gl_errors(class_name() + "::draw: draw triangles");

   // XXX - under construction:
   // optionally draw silhouettes
   // (outside the display list: they are view-dependent):
   draw_sils();  

   // restore gl state:
   restore_gl_state();

   if (debug) {
      GL_VIEW::print_gl_errors(class_name() + "::draw: pop attrib");
   }

   deactivate_program();


   GL_VIEW::print_gl_errors(class_name() + "::draw: end");

   return _patch->num_faces();
}

void
GLSLShader::draw_sils()
{
 if (_draw_sils) {      
      GL_COL(_sil_color, _sil_alpha); // GL_CURRENT_BIT
      _patch->cur_sils().draw(_cb);
      if (debug) {
         GL_VIEW::print_gl_errors(class_name() + "::draw: draw sils");
      }
 }
} 

/**********************************************************************
 * GLSLLightingShader:
 *
 *   Does OpenGL lighting calculations in GLSL.
 **********************************************************************/
GLuint GLSLLightingShader::_program = 0;
bool   GLSLLightingShader::_did_init = false;

GLSLLightingShader::GLSLLightingShader(Patch* p) :
   GLSLShader(p) 
{
}

int
GLSLLightingShader::draw(CVIEWptr& v)
{
   // Set material parameters for OGL:
   GtexUtil::setup_material(_patch);

   // Execute the GLSL program:
   return GLSLShader::draw(v);
}

// end of file glsl_shader.C
