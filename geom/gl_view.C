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
#include "std/support.H"
#include "glew/glew.H"

#include "disp/animator.H"
#include "disp/colors.H"
#include "geom/gl_view.H"
#include "geom/geom.H"
#include "disp/recorder.H"
#include "std/thread_mutex.H" 

using namespace mlib;

static bool multithread = Config::get_var_bool("JOT_MULTITHREAD",false);

static ThreadMutex polyextmutex;
static ThreadMutex gl_version_mutex;
static Cstr_ptr in_swap_buffers("GL_VIEW::swap_buffers");

bool    GL_VIEW::_checked_point_sizes = false;
bool    GL_VIEW::_checked_line_widths = false;
GLfloat GL_VIEW::_min_point_size = 0;
GLfloat GL_VIEW::_max_point_size = 0;
GLfloat GL_VIEW::_min_line_width = 0;
GLfloat GL_VIEW::_max_line_width = 0;

void
GL_VIEW::load_proj_mat(CAMdata::eye e)
{
   glLoadMatrixd(_view->wpt_proj(_view->screen(),e).transpose().matrix());
}

void
GL_VIEW::load_cam_mat(CAMdata::eye e)
{
   CCAMdataptr &camdata = ((CVIEWptr &) _view)->cam()->data();
   glLoadMatrixd(camdata->xform(_view->screen(), e).transpose().matrix());
}

/* ------------------ GL_VIEW static class definitions --------------------- */

/*
 *  paint() - initializes GL for drawing, then draws all the objects
 *            (twice for stereo),  then invokes each stencil callback
 *            which may draw additional objects through a stencil mask.
 *      
 *       draw_setup()
 *          clear_buffer()       // clear display and call clear cb's
 *          setup stencil()      // initialize stencil masks
 *          setup scissor()      // scissor drawing to a sub-region of display
 *       draw_frame()            // draw display frame (twice for stereo, etc)
 *          draw_objects()       // draw all objects
 *          stencil_cbs()        // draw each stencil (call callback)
 *             stencil_draw()    // setup stencil mask
 *                draw_objects() // draw stencil-specific objects
 *       swap_buffers()          // unless synching with other displays
 */

int
GL_VIEW::paint()
{
   int width, height;
   _view->get_size(width, height);
   int tris = 0;

   if ((!multithread && (VIEWS.num()>1)) ||
       _view->win()->needs_context()) 
   {   
      _view->win()->set_context(); // XXX Should only happen if > 1 view
   }

   if (_resizePending) 
   {
      _paintResize = true;
      set_size(_resizeW, _resizeH, _resizeX, _resizeY);
   }

   // Only draw the scene if we're not rendering to disk
   // Otherwise, the post_draw_cb with draw...
   Animator *anim = _view->animator();
   if (!(anim->on() && anim->play_on() && anim->rend_on()))
   {
      draw_setup();

      // Window may need to perform custom-rendering (e.g., to draw a cursor)
      _view->win()->draw();

      switch (_view->stereo()) {
       default              : cerr << "Unknown stereo mode in GL_VIEW.C" << endl;
       brcase NONE          : tris += draw_frame();
       brcase LEFT_EYE_MONO : tris += draw_frame(CAMdata::LEFT);
       brcase RIGHT_EYE_MONO: tris += draw_frame(CAMdata::RIGHT);
       brcase TWO_BUFFERS   : glDrawBuffer(GL_BACK_RIGHT);
                              tris += draw_frame(CAMdata::RIGHT);
                              glClear(GL_DEPTH_BUFFER_BIT);
                              glDrawBuffer(GL_BACK_LEFT);
                              tris += draw_frame(CAMdata::LEFT);
       brcase HMD: case LCD: {
          static const int RADJUST = Config::get_var_int("RADJUST",0,true);
          static const int LADJUST = Config::get_var_int("LADJUST",0,true);
          // used on POSSE w/ the black and yellow HMD
          // Stereo - must draw all the objects twice, once for each eye
          glViewport((GLsizei) (0),                 // top-window (right eye)
                     (GLsizei) (height/2+20+RADJUST),
                     (GLsizei) (width - 1),
                     (GLsizei) (height/2-21));
          tris += draw_frame(CAMdata::RIGHT);
     
          glViewport((GLsizei) (0),                 // bot-window (left eye)
                     (GLsizei) (0 + 20 + LADJUST),
                     (GLsizei) (width - 1),
                     (GLsizei) (height/2-21));
          tris += draw_frame(CAMdata::LEFT);
       }
      }
   }

   if (_focusPending) 
   {
      _view->win()->set_focus();
      _focusPending = false;
   }

// HACK CITY
//   if (!_view->dont_swap() || Config::get_var_bool("ASYNCH",false,true))
//      swap_buffers();
   return tris;
}

/*
 * draw_setup() - initializes GL for the upcoming display update.
 *   initialization includes, setting the drawing buffer,
 *   configuring the Zbuffer, clearing to a background color,
 *   and configuring the stencil buffers and scissor regions.
 */
void
GL_VIEW::draw_setup()
{
   // Initialization
   glDepthMask(GL_TRUE);
   glDepthFunc(GL_LEQUAL);
   glEnable(GL_DEPTH_TEST);

   // setup correct buffer, and clear its color & depth data.
   clear_draw_buffer();

   // initialize stencil mask for each registered stencil cb
   if (_view->stencil_cbs().num())
      setup_stencil();
   else
      glDisable(GL_STENCIL_TEST);

   // setup scissor region if applicable
   if (_view->has_scissor_region())
      setup_scissor();
}

/*
 * draw_frame() - draws all objects and stencils 
 *   this paints a complete display frame by drawing all the active
 * objects and all the active stencils.   The stencil_cbs() will
 * call stencil_draw() to actually paint on the display.
 */
int
GL_VIEW::draw_frame(
   CAMdata::eye  e
   )
{
   int tris = draw_objects(_view->drawn(), e);

   if (_view->stencil_cbs().num())
      for (int i = 0; i < _view->stencil_cbs().num(); i++)
         tris += _view->stencil_cbs()[i]->stencil_cb();

   return tris;
}


//
// Do whatever is needed at the end of drawing
//
void
GL_VIEW::swap_buffers()
{

   if (_view->has_scissor_region())
      glDisable(GL_SCISSOR_TEST);

   if (_view->win()->double_buffered())
      _view->win()->swap_buffers();
   else
      glFlush();

   print_gl_errors(in_swap_buffers);
}

void
GL_VIEW::clear_draw_buffer()
{
   if (_view->win()->double_buffered())
      glDrawBuffer(GL_BACK);
   else
      glDrawBuffer(GL_FRONT);

   // Set background color for clear
   if (_view->rendering() == RCOLOR_ID) {
      glClearColor(0, 0, 0, 0);
   } else {
      COLOR bkg(_view->color());
      //use the view's new _alpha?
      glClearColor((float)bkg[0], (float)bkg[1], (float)bkg[2], 0);
   }

   // Clear the color & depth buffers.
   // NOTE: If using a stereo mode, GL is supposed to clear both the
   // back left & back right buffers if glDrawBuffer(..) is set to GL_BACK.
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   _view->notify_clearobs();
}

/*
 * setup_stencil() - initializes GL stencil planes
 *    the VIEW supports a list of stencil callbacks.  Each stencil 
 * callback is defines a polygon region on the display surface
 * that it alone can paint.  Thus, before rendering anything onto
 * the display, this routine asks each stencil callback for the 
 * display region that it uses.   For each such region, this 
 * routine sets a stencil mask that enables only the corresponding 
 * stencil cb to draw there.
 */
void
GL_VIEW::setup_stencil()
{
   glEnable(GL_STENCIL_TEST);
   glClearStencil(0x0);
   glClear(GL_STENCIL_BUFFER_BIT);

   // Set up stencil buffer

   // First, set up orthogonal projection (-1 to 1 in x and y)
   glMatrixMode(GL_PROJECTION);
   glPushMatrix();
   glLoadMatrixd(_view->xypt_proj().transpose().matrix());

   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
   glLoadIdentity();

   XYpt_list pts;
   for (int i = 0; i < _view->stencil_cbs().num(); i++) {
      // Get bounds of the requested stencil buffer
      _view->stencil_cbs()[i]->stencil_bounds(pts);

      // Now, set up the stencil buffer
      // The stencil buffer will be set to i+1 where the following
      // rectangle(s) are drawn
      glStencilFunc(GL_ALWAYS, (GLint) i + 1, 0x127);
      glStencilOp  (GL_REPLACE, GL_REPLACE, GL_REPLACE);
      glDisable    (GL_LIGHTING);
      glBegin(GL_QUADS);
	 for (int p = 0;  p < pts.num(); p++)
	    glVertex2dv(pts[p].data());
      glEnd();
   }

   // Only draw where stencil buffer *is* 0 (where no rectangles were
   // drawn)
   glStencilFunc(GL_EQUAL, 0x0, (GLuint) 127);
   glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

   glPopMatrix();               // Restore modelview matrix
   glMatrixMode(GL_PROJECTION); // Switch to projection matrix
   glPopMatrix();               // Restore projection matrix
}

/* 
 * setup_scissor() - clips drawing to a sub-region of display
 */
void
GL_VIEW::setup_scissor()
{
   PIXEL lower_left (XYpt(_view->scissor_xmin(),-1));
   PIXEL upper_right(XYpt(_view->scissor_xmax(), 1));

   // IS THIS ACCURATE ENOUGH??  do we have to be more careful about
   // how ensuring that if one xrange is [a,b] and another is [b,c]
   // that they are perfectly adjacent to each other in all cases?
   GLsizei x,y,w,h;
   x = GLsizei(lower_left[0]);
   y = GLsizei(lower_left[1]);
   w = GLsizei(upper_right[0] - lower_left[0]);
   h = GLsizei(upper_right[1] - lower_left[1]);

   glScissor(x, y, w, h);
   glEnable (GL_SCISSOR_TEST);
}

inline void
check(GEOMptr geom)
{
   if (!geom)
      return;
   cerr << geom->name() << " (" << geom->class_name() << ")"
        << " has transp: " << (geom->has_transp() ? "true" : "false")
        << ", transp: " << geom->transp()
        << ", needs blend: " << (geom->needs_blend() ? "true" : "false")
        << endl;
}

/*
 * draw_objects() - draws all objects, but not stencils
 */
int
GL_VIEW::draw_objects(
   CGELlist     &objs, 
   CAMdata::eye  e
   )
{
   static GLfloat SHININESS =
      (GLfloat)Config::get_var_dbl("SHININESS",20.0,true);
   int tris = 0;

   // load projection matrix
   glMatrixMode(GL_PROJECTION);
   load_proj_mat(e);

   // load the camera position matrix
   glMatrixMode(GL_MODELVIEW);
   load_cam_mat(e);

   // set default state
   glLineWidth(float(_view->line_scale()*1.0));
   glDisable(GL_NORMALIZE);      // MODELERs enable this only if needed
   glDisable(GL_BLEND);
   if (_view->lights_on()) 
   {
       GLfloat emission_color[] = {0,0,0,0};
       GLfloat specular_color[] = {0,0,0,0};

       glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emission_color);
       glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
       glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular_color);
       glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, SHININESS);
   } 
   else 
   { 
       GLfloat amb_diff_color[] = {0,0,0,0};
       GLfloat specular_color[] = {0,0,0,0};
       glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);
       glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, amb_diff_color);
       glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular_color);
       glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, SHININESS);
   }
   glEnable(GL_COLOR_MATERIAL);  // glColor will now set components 
                                 // specified by glColorMaterial flags

   glShadeModel(GL_SMOOTH);
   if (_view->is_clipping()) {
      glEnable(GL_CLIP_PLANE0);

      GLdouble equation[4];
      CWvec &vec = _view->clip_plane().normal();
      equation[0] = vec[0];
      equation[1] = vec[1];
      equation[2] = vec[2];
      equation[3] = _view->clip_plane().d();
      glClipPlane(GL_CLIP_PLANE0, equation);
      glDisable(GL_CULL_FACE);
   } else {
      glDisable(GL_CLIP_PLANE0);

      if (_view->rendering() == RWIRE_FRAME) glDisable(GL_CULL_FACE);
      else                                   glEnable (GL_CULL_FACE);
   }

   if (_view->lights_on())
      setup_lights(e);
   else
      glDisable(GL_LIGHTING);

   // sort objects into two lists, with blended
   // objects in order of decreasing depth:
   sort_blended_objects(objs);

   // experimental policy (10/2005):
   init_polygon_offset();

   glMatrixMode(GL_MODELVIEW);
   int i;

   static bool checked = false;
   if (0 && !checked) {
      checked = true;
      for (int i=0; i<objs.num(); i++) {
         check(GEOM::upcast(objs[i]));
      }
   }

   bool debug_blend = false;
   if (debug_blend)
      cerr << "---------- opaque: ----------" << endl;
   for (i = 0; i < _opaque.num(); i++) {
      // XXX - should sort all clip avoiding objects together so we
      // don't have to do this per-object 
      if (_view->is_clipping()) {
         if (DONOT_CLIP_OBJ.get(_opaque[i]))
              glDisable(GL_CLIP_PLANE0);
         else glEnable (GL_CLIP_PLANE0);
      }

      load_cam_mat(e);
      if (debug_blend)
         cerr << _opaque[i]->name() << endl;
      tris += _opaque[i]->draw(_view);
   }

   if (debug_blend)
      cerr << "---------- blended: ----------" << endl;
   if (!_blended.empty()) {
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      for (i = 0; i < _blended.num(); i++) {
         // XXX - should sort all clip avoiding objects together so we
         // don't have to do this
         if (_view->is_clipping()) {
            if (DONOT_CLIP_OBJ.get(_blended[i]))
                 glDisable(GL_CLIP_PLANE0);
            else glEnable (GL_CLIP_PLANE0);
         }
         load_cam_mat(e);
         if (debug_blend)
            cerr << _blended[i]->name() << endl;
         tris += _blended[i]->draw(_view);
      }
   }

   end_polygon_offset();

   // Final pass (for strokes or whatever)
   // XXX - for RICL types, the draw methods really are const,
   //       but the base class RefImageClient does not define
   //       them as const. should find a way to define RICL so
   //       the methods are const but still over-ride base
   //       class versions. for now to const_cast:
   const_cast<GELlist*>(&objs)->draw_final(_view);

   return tris;
}


/*
 * stencil_draw() - paints a specific stencil region
 *    this is called by each stencil region after the stencil cb has
 * setup any region-specific rendering parameters.  In addition, the
 * stencil cb can supply a list of objects to be drawn instead of the
 * default scene objects.
 */
int
GL_VIEW::stencil_draw(
   STENCILCB *cb, 
   GELlist   *objs
   )
{
   const int num = _view->stencil_cbs().get_index(cb);
   if (num != BAD_IND) {
      glStencilFunc(GL_EQUAL, (GLint) num + 1, 0x127);
      glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
      return draw_objects(objs ? *objs : _view->drawn());
   } else {
      cerr << "GL_VIEW::stencil_draw - couldn't find STENCILCB" << endl;
      return 0;
   }
}

bool
GL_VIEW::print_gl_errors(Cstr_ptr &location)
{
   bool ret = false;

   GLenum err;

   while((err = glGetError())) {
      str_ptr errstr;
      switch (err) {
         case GL_INVALID_ENUM:      errstr = "Invalid Enumerator";   break;
         case GL_INVALID_VALUE:     errstr = "Invalid Value";        break;
         case GL_INVALID_OPERATION: errstr = "Invalid Operation";    break;
         case GL_STACK_OVERFLOW:    errstr = "Stack Overflow";       break;
         case GL_STACK_UNDERFLOW:   errstr = "Stack Underflow";      break;
         case GL_OUT_OF_MEMORY:     errstr = "Out of Memory";        break;
         default:                   errstr = "Unknown Error";        break;
      }
      err_msg("%s ***NOTE*** OpenGL Error: [%x] '%s'", **location, err, **errstr);
      ret = true;
   }

   return ret;
}

inline bool
is_valid_3D_object(CGELptr& gel)
{
   return GEOM::isa(gel) && gel->bbox().valid();
}

int
GL_VIEW::depth_compare(
   const void *a,
   const void *b
   )
{
   CAMptr cam = VIEW::peek_cam();

   GELptr gel1 = *(GELptr*)a;
   GELptr gel2 = *(GELptr*)b;

   // XXX - Changed default sorting when a non
   // GEOM show up... This makes gesture strokes
   // appear on top.  Mind you, gel's should have bbox's
   // so I dunno why we can't just forge ahead...

   if (!is_valid_3D_object(gel1))
      return 1;
   
   if (!is_valid_3D_object(gel2))
      return -1;
   
   double x = (gel1)->bbox().center().dist_sqrd(cam->data()->from()) -
              (gel2)->bbox().center().dist_sqrd(cam->data()->from());
   
   return (x<0) ? 1 : (x>0) ? -1 : 0;
}


void       
GL_VIEW::sort_blended_objects(
   CGELlist &list
   )
{
    _blended.clear();
    _opaque.clear();
   
   for (int i=0; i < list.num(); i++ ) {
      if (list[i]->needs_blend())
         _blended += list[i];
      else
         _opaque  += list[i];
   }
   
   _blended.sort(GL_VIEW::depth_compare);
}

void
GL_VIEW::setup_light(int i)
{
   GLenum num = light_i(i);              // GL_LIGHT0, e.g.
   const Light& light = _view->get_light(i);

   // set modelview matrix before passing light coordinates
   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
   if (light._is_in_cam_space) {
      glLoadIdentity();
   } else {
      glLoadMatrixd(_view->world_to_eye().transpose().matrix());
   }

   // If a light is disabled, set its colors to black
   // (GLSL shaders need this since they don't know which lights are enabled)
   COLOR a = Color::black;
   COLOR d = Color::black;
   COLOR s = Color::black;
   if (light._is_enabled) {
      a = light._ambient_color;
      d = light._diffuse_color;
      s = light._specular_color;
      glEnable(num);
   } else {
      glDisable(num);
   }

   // set light parameters:
   glLightfv(num, GL_AMBIENT,  float4(a));
   glLightfv(num, GL_DIFFUSE,  float4(d));
   glLightfv(num, GL_SPECULAR, float4(s));
   if (light._is_positional) {
      glLightfv(num, GL_POSITION, float4(light.get_position()));
      
   } else {
      glLightfv(num, GL_POSITION, float4(light.get_direction()));
   }
   glLightf (num, GL_CONSTANT_ATTENUATION,  light._k0);
   glLightf (num, GL_LINEAR_ATTENUATION,    light._k1);
   glLightf (num, GL_QUADRATIC_ATTENUATION, light._k2);
   glLightfv(num, GL_SPOT_DIRECTION, float4(light._spot_direction));
   glLightf (num, GL_SPOT_EXPONENT,  light._spot_exponent);
   glLightf (num, GL_SPOT_CUTOFF,    light._spot_cutoff);

   glPopMatrix();
}

void
GL_VIEW::setup_lights(CAMdata::eye e)
{
   print_gl_errors("GL_VIEW::setup_lights: starting");

   if (_view->lights_on()) {
      glEnable(GL_LIGHTING);
   } else {
      glDisable(GL_LIGHTING);
   }

   // setup global ambient light:
   glLightModelfv(GL_LIGHT_MODEL_AMBIENT,
                  float4(_view->light_get_global_ambient()));

   // enable two-sided lighting and use local viewer model:
   glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 1);
   glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);

   // setup individual lights:
   for (int i=0; i<VIEW::max_lights(); i++)
      setup_light(i);
   
   print_gl_errors("GL_VIEW::setup_lights: end");
}

void
GL_VIEW::end_buf_read()
{
   // restore default state:
   glPixelStorei(GL_PACK_ALIGNMENT,4);
}

void
GL_VIEW::prepare_buf_read()
{
   glReadBuffer(GL_BACK);
   glPixelStorei(GL_PACK_ALIGNMENT,1);  // needed to avoid seg faults
}

void
GL_VIEW::read_pixels(
   uchar *data,
   bool  alpha
   )
{
   //If alhpa=true, then the caller wants alpha too!!

   draw_setup();

   draw_frame();

   int width, height;
   _view->get_size(width, height);

   // back buffer now contains pixels of
   // the current tile. read the pixels:
   if (alpha)
      glReadPixels(0,0,width,height,GL_RGBA,GL_UNSIGNED_BYTE,data);
   else
      glReadPixels(0,0,width,height,GL_RGB,GL_UNSIGNED_BYTE,data);
}

//
// Callback for when the size of the view changes
//
void
GL_VIEW::set_size(
   int width,
   int height,
   int xpos,  // x position
   int ypos   // y position
   )
{
   if (multithread && !_paintResize) 
   {
      _resizePending = true;
      _resizeW = width; _resizeH = height; _resizeX = xpos; _resizeY = ypos;
      return;
   }
   _paintResize = false;
   _resizePending = false;

   if ((!multithread && (VIEWS.num()>1)) ||
       _view->win()->needs_context()) 
   {   
      _view->win()->set_context();
   }

   // If using Yotta, make sure entire background color is set to
   // bkgnd color before resetting the viewport offset & dimensions so
   // the compositing h/w doesn't do the wrong thing because of any
   // non-bkgnd colored pixels outside the viewport.
   if (_view->stereo() == LEFT_EYE_MONO || _view->stereo() == RIGHT_EYE_MONO) 
   {
      glDrawBuffer(GL_FRONT);
      clear_draw_buffer();
      if (_view->win()->double_buffered()) 
      {
         glDrawBuffer(GL_BACK);
         clear_draw_buffer();
      }
   }

   // set new dimensions & offset for viewport
   glViewport(xpos, ypos, (GLsizei)width, (GLsizei)height);

   // XXX - tracked down the bad stuff, to here:
   //       CAMdata::aspect() is suposedly height/width.
   //       following lines get that ass-backwards.

   // XXX - needs work

   static double asp = Config::get_var_dbl("DISPLAY_ASPECT",-1.0,true);
   if (asp != -1.0)
   {
      _view->cam()->set_aspect(asp);
   }
   else 
   {
      static double pixel_warp_factor = Config::get_var_dbl("PIXEL_WARP_FACTOR",1.0,true);
      _view->cam()->set_aspect(double(width)/double(height)*pixel_warp_factor);
   }
}

//
// Returns OpenGL version as a double
//
double
GL_VIEW::gl_version()
{
   CriticalSection cs(&gl_version_mutex);
   static double ret = glGetString(GL_VERSION) 
      ? atof((const char*)glGetString(GL_VERSION))
      : 0;

   return ret;
}

bool 
GL_VIEW::poly_ext_available() 
{
   CriticalSection cs(&polyextmutex);
   static bool ret = (strstr((const char *)glGetString(GL_EXTENSIONS),
                            "GL_EXT_polygon_offset") != 0);
   return ret;
}

void
GL_VIEW::init_polygon_offset(
   float  factor,
   float  units,
   GLenum mode
   )
{
   glEnable(mode);                              // GL_ENABLE_BIT
   glPolygonOffset(factor, units);              // GL_POLYGON_BIT
}

void
GL_VIEW::end_polygon_offset()
{
   glDisable(GL_POLYGON_OFFSET_FILL);        // GL_ENABLE_BIT
}

void 
GL_VIEW::check_point_sizes()
{
   if (_checked_point_sizes) {
      return;
   }
   _checked_point_sizes = true;

   GLfloat granularity = 0;
   GLfloat point_sizes[2];
   glGetFloatv(GL_POINT_SIZE_GRANULARITY, &granularity);
   glGetFloatv(GL_POINT_SIZE_RANGE, point_sizes);      // gets min and max
   _min_point_size = point_sizes[0];
   _max_point_size = point_sizes[1];
   if (Config::get_var_bool("JOT_PRINT_POINT_SIZES",false)) {
      cerr << "  GL point size: "
           << "min: " << _min_point_size << ", "
           << "max: " << _max_point_size << ", "
           << "granularity: " << granularity << endl;
   }
}

void 
GL_VIEW::check_line_widths()
{
   if (_checked_line_widths) {
      return;
   }
   _checked_line_widths = true;

   GLfloat granularity = 0;
   GLfloat line_widths[2];
   glGetFloatv(GL_LINE_WIDTH_GRANULARITY, &granularity);
   glGetFloatv(GL_LINE_WIDTH_RANGE, line_widths);      // gets min and max
   _min_line_width = line_widths[0];
   _max_line_width = line_widths[1];
   if (Config::get_var_bool("JOT_PRINT_LINE_WIDTHS",false)) {
      cerr << "  GL line width: "
           << "min: " << _min_line_width << ", "
           << "max: " << _max_line_width << ", "
           << "granularity: " << granularity << endl;
   }
}

void
GL_VIEW::init_point_smooth(GLfloat size, GLbitfield mask, GLfloat* a)
{
   // Turns on antialiasing for points and sets the point size to the
   // given amount. Calls glPushAttrib() with GL_ENABLE_BIT,
   // GL_POINT_BIT and GL_COLOR_BUFFER_BIT along with whatever bits
   // are passed in with 'mask'.

   check_point_sizes();

   size = max(GLfloat(0), size);

   glPushAttrib(mask | GL_ENABLE_BIT | GL_POINT_BIT |
                GL_HINT_BIT | GL_COLOR_BUFFER_BIT);
   glEnable(GL_POINT_SMOOTH);                         // GL_ENABLE_BIT
   glEnable(GL_BLEND);                                // GL_ENABLE_BIT
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // GL_COLOR_BUFFER_BIT
   glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);           // GL_HINT_BIT
   if (size > _max_point_size) {
      size = _max_point_size;
   } else if (size < _min_point_size) {
      if (a) {
         *a = (size/_min_point_size);
      } else {
         static bool warned=false;
         if (!warned) {
            warned=true;
            err_msg("GL_VIEW::init_point_smooth: unsupported point size %f", size);
         }
      }
      size = _min_point_size;
   }

   glPointSize(size);                                    // GL_POINT_BIT
}

void
GL_VIEW::end_point_smooth()
{
   glPopAttrib();
}

void
GL_VIEW::end_line_smooth()
{
   glPopAttrib();
}

void
GL_VIEW::draw_pts(
   CWpt_list&   pts,    // points to draw
   CCOLOR&      color,  // color to use
   double       alpha,  // transparency
   double       size    // width of points
   )
{
   // Draws given points as disconnected dots w/ given
   // color, transparency and size. Assumes projection and
   // modelview matrices have been set as needed.

   if (!pts.empty()) {
      // Enable point smoothing and push gl state:
      init_point_smooth(float(size), GL_CURRENT_BIT);
      GL_COL(color, alpha);             // GL_CURRENT_BIT
      glDisable(GL_LIGHTING);           // GL_ENABLE_BIT
      glBegin(GL_POINTS);
      for (int i=0; i<pts.num(); i++)
         glVertex3dv(pts[i].data());
      glEnd();
      GL_VIEW::end_point_smooth();      // pop state
   }
}

void 
GL_VIEW::draw_wpt_list(
   CWpt_list&      pts,        // polyline to draw
   CCOLOR&         color,      // color to use
   double          alpha,      // transparency
   double          width,      // width of points
   bool            do_stipple  // if true, draw tippled
   )
{
   // Draws a Wpt_list w/ given color, transparency and
   // width. If do_stipple is set to true, the Wpt_list is
   // drawn stippled. Assumes projection and modelview
   // matrices have been set as needed.

   if (pts.num() < 2) 
      return;

   glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_CURRENT_BIT);

   GL_COL(color, alpha);             // GL_CURRENT_BIT
   glDisable(GL_LIGHTING);           // GL_ENABLE_BIT
   glLineWidth(float(width));        // GL_LINE_BIT
   if (do_stipple) {
      glLineStipple(1,0x00ff);        // GL_LINE_BIT
      glEnable(GL_LINE_STIPPLE);      // GL_ENABLE_BIT
   }

   // draw the polyline
   glBegin(GL_LINE_STRIP);
   for (int i=0; i<pts.num(); i++) {
      glVertex3dv(pts[i].data());
   }
   glEnd();

   glPopAttrib();
}

void 
GL_VIEW::draw_lines(
   CARRAY<Wline>&  lines,      // lines to draw
   CCOLOR&         color,      // color to use
   double          alpha,      // transparency
   double          width,      // width of points
   bool            do_stipple  // if true, draw tippled
   )
{
   // Draws given lines w/ given color, transparency and width. If
   // do_stipple is set to true, the lines are drawn stippled. Assumes
   // projection and modelview matrices have been set as needed.

   if (lines.empty()) 
      return;

   glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_CURRENT_BIT);

   GL_COL(color, alpha);             // GL_CURRENT_BIT
   glDisable(GL_LIGHTING);           // GL_ENABLE_BIT
   glLineWidth(float(width));        // GL_LINE_BIT
   if (do_stipple) {
      glLineStipple(1,0x00ff);        // GL_LINE_BIT
      glEnable(GL_LINE_STIPPLE);      // GL_ENABLE_BIT
   }

   // draw the lines

   glBegin(GL_LINES);

   for (int i=0; i<lines.num(); i++) {
      glVertex3dv(lines[i].point().data());
      glVertex3dv(lines[i].endpt().data());
   }
  
   glEnd();

   glPopAttrib();
}

void
GL_VIEW::draw_bb(CWpt_list &bb_pts) const
{
   glDisable(GL_LIGHTING);
   glLineWidth(float(_view->line_scale()*3.0));
   glEnable(GL_LINE_SMOOTH);
   glBegin(GL_LINES);
   // Lines from min pt
   glVertex3dv(bb_pts[0].data()); glVertex3dv(bb_pts[1].data());
   glVertex3dv(bb_pts[0].data()); glVertex3dv(bb_pts[2].data());
   glVertex3dv(bb_pts[0].data()); glVertex3dv(bb_pts[3].data());
   // Lines from max point
   glVertex3dv(bb_pts[7].data()); glVertex3dv(bb_pts[6].data());
   glVertex3dv(bb_pts[7].data()); glVertex3dv(bb_pts[5].data());
   glVertex3dv(bb_pts[7].data()); glVertex3dv(bb_pts[4].data());
   // Lines from top face
   glVertex3dv(bb_pts[2].data()); glVertex3dv(bb_pts[4].data());
   glVertex3dv(bb_pts[2].data()); glVertex3dv(bb_pts[6].data());
   // Missing lines from front face
   glVertex3dv(bb_pts[6].data()); glVertex3dv(bb_pts[1].data());
   // Missing line from bottom face
   glVertex3dv(bb_pts[5].data()); glVertex3dv(bb_pts[1].data());
   glVertex3dv(bb_pts[5].data()); glVertex3dv(bb_pts[3].data());

   // Last missing line
   glVertex3dv(bb_pts[4].data()); glVertex3dv(bb_pts[3].data());
   glEnd();
   glDisable(GL_LINE_SMOOTH);
   glLineWidth(float(_view->line_scale()*1.0));
   glEnable(GL_LIGHTING);
}

// end of file gl_view.C
