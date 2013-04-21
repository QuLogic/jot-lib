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
 *  \file trace.C
 *  \brief Contains the definition of the TRACE widget.
 *
 *  \ingroup group_FFS
 *  \sa trace.H
 *
 */
#include "disp/colors.H"
#include "geom/gl_util.H"       // for GL_COL()
#include "geom/world.H"         // for WORLD::undisplay()
#include "geom/texturegl.H"
#include "gtex/ref_image.H"     // for VisRefImage
#include "widgets/file_select.H"

#include "trace.H"

using mlib::XYpt;
using mlib::Point2i;

/*****************************************************************
 * TRACE
 *****************************************************************/
TRACEptr TRACE::_instance;

TRACEptr
TRACE::get_instance(VIEWptr v)
{
   if (!_instance || !_instance->valid() )
      _instance = new TRACE(v);
   return _instance;
}

void
TRACE::clear_instance()
{
   _instance = NULL;
}

bool
TRACE::valid(){
   return is_valid;
}

bool
TRACE::calibrating(){
   return _calib_mode_flag;
}

TRACE::TRACE(VIEWptr v) : DrawWidget(), _view(v), _init_stamp(0)
{
   //*******************************************************
   // FSA states for handling GESTUREs:
   //
   //   Note: the order matters. First matched will be executed.
   //*******************************************************
   _draw_start += DrawArc(new TapGuard,     drawCB(&TRACE::tap_cb));
   _draw_start += DrawArc(new StrokeGuard,  drawCB(&TRACE::stroke_cb));

   _texture = NULL;

   _calibrated = false;
   _calib_mode_flag = true;
   _cur_calib_pt_index = 0;

   // file selection
   {
      is_valid=false;
      load_dia();
   }
   cerr << "trace widget created" << endl;
}

TRACE::~TRACE() 
{
   _texture = NULL;
}

// note... might want to have a method to just load a general
// image instead if more than one method wants to load an image -alexni
void TRACE::load_dia(){
   FileSelect *sel = _view->win()->file_select();

   sel->set_title("Load Image");
   sel->set_action("Load");
   sel->set_icon(FileSelect::LOAD_ICON);
   sel->set_path(".");
   sel->set_file("");
   sel->set_filter("*");
   

   if (sel->display(true, file_cbs, this, FILE_LOAD_TRACE_CB))
      cerr << "TRACE::load_dia() - FileSelect displayed.\n";
   else
      cerr << "TRACE::load_dia() - FileSelect **FAILED** to display!!\n";

   return;
}

// note... may need to change ptr to get_instance in the case that
// it's possible to delete the current instance while a dialog is up.
void 
TRACE::file_cbs(void *ptr, int idx, int action, str_ptr path, str_ptr file)
{
   if(idx == FILE_LOAD_TRACE_CB && (action == FileSelect::OK_ACTION)){
      TRACE* t = (TRACE*)ptr;
      t->_filename = path+file;

      t->_texture = new TEXTUREgl(((TRACE*)ptr)->_filename);
      if(!(t->is_valid = t->_texture->load_texture())){
         t->_texture = NULL;
         return; 
      }
      t->_texture->set_tex_fn(GL_REPLACE);
   }
}

void
TRACE::reset()
{
   // Clear cached data.
   _init_stamp = 0;
}

int
TRACE::draw(CVIEWptr&)
{
   if (!valid()) {
      return 0;
   }

   // load identity for model matrix
   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
   glLoadIdentity();

   // set up to draw in PIXEL coords:
   glMatrixMode(GL_PROJECTION);
   glPushMatrix();
   glLoadMatrixd(VIEW::peek()->xypt_proj().transpose().matrix());
   
   // Set up line drawing attributes
   glPushAttrib(GL_ENABLE_BIT);

   glDisable(GL_LIGHTING);              // GL_ENABLE_BIT
   glEnable(GL_TEXTURE_2D);             // GL_ENABLE_BIT
   glDisable(GL_CULL_FACE);             // GL_ENABLE_BIT
   glDisable(GL_DEPTH_TEST);            // GL_ENABLE_BIT

   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   _texture->apply_texture();           // GL_ENABLE_BIT

   if (_calib_mode_flag) {
      // no transparency if we are in calibration
      glColor4f(1.0,1.0,1.0,0.5);
      if(_cur_calib_pt_index==0){
         WORLD::message("Calibrate trace: click upper left");
      } else if(_cur_calib_pt_index==1){
         WORLD::message("Calibrate trace: click lower right");
      }
   } else {
      glColor4f(1.0, 1.0, 1.0, 0.5);
   }

   // begin drawing
   glBegin(GL_QUAD_STRIP);
   
   XYpt topleft, topright, bottomleft, bottomright;
   
   if (_calibrated) {
      // if we are calibrated, use the corners we got from
      // calibration
                
      if(_cur_calib_pt_index == 4){ 
         topleft     = _samples[_TOP_LEFT];
         topright    = _samples[_TOP_RIGHT];
         bottomleft  = _samples[_BOTTOM_RIGHT];
         bottomright = _samples[_BOTTOM_LEFT];
      } else if(_cur_calib_pt_index == 2){
         topleft     = _samples[0];
         topright    = XYpt( _samples[1][0], _samples[0][1]);
         bottomright = _samples[1];
         bottomleft  = XYpt(_samples[0][0], _samples[1][1]);
      }
   } else {
      // if we aren't calibrated, then draw the image over
      // the whole window
      double tex_dim[2];
      Point2i ts = _texture->get_size();
      tex_dim[0] = ts[0]; tex_dim[1] = ts[1];
      if(tex_dim[1] >= tex_dim[0]){
         tex_dim[0] = (1.0*tex_dim[0])/tex_dim[1];
         tex_dim[1] = 1.0;
      } else {
         tex_dim[1] = (1.0*tex_dim[1])/tex_dim[0];
         tex_dim[0] = 1.0;
      }
          
      topleft     = XYpt(-tex_dim[0],  tex_dim[1]);
      topright    = XYpt( tex_dim[0],  tex_dim[1]);
      bottomleft  = XYpt(-tex_dim[0], -tex_dim[1]);
      bottomright = XYpt( tex_dim[0], -tex_dim[1]);
   }
   
   glTexCoord2f(0.0, 1.0); glVertex2dv(topleft.data());
   glTexCoord2f(0.0, 0.0); glVertex2dv(bottomleft.data());
   glTexCoord2f(1.0, 1.0); glVertex2dv(topright.data());
   glTexCoord2f(1.0, 0.0); glVertex2dv(bottomright.data());
   
   glEnd();
   
   glPopAttrib();
   
   if (_calib_mode_flag) {
      // if we are in calibration mode, draw point or else indicate
      // where the user is supposed to tap next
   }

   glMatrixMode(GL_PROJECTION);
   glPopMatrix(); // unloads our projection matrix
   glMatrixMode(GL_MODELVIEW);
   glPopMatrix(); // unloads model matrix

   return 1;
}

int  
TRACE::cancel_cb(CGESTUREptr&, DrawState*& s)
{
   // Turn off widget:
   deactivate();

   // Reset state to start:
   s = &_draw_start;

   return 1; // This means we DID use up the gesture
}

int  
TRACE::stroke_cb(CGESTUREptr&, DrawState*&) 
{
   return 0;
}

int
TRACE::tap_cb(CGESTUREptr& gest, DrawState*& s) 
{
   assert (gest->is_tap());
   
   if (_calib_mode_flag) {
      // we are in calibration point
      XYpt sample = gest->end();
      _samples[_cur_calib_pt_index] = sample;
      cerr << "recorded sample " << _cur_calib_pt_index << endl;
      _cur_calib_pt_index++;
      /* seems impractical to distort image, leaving this at 2 - alexni
         if (_cur_calib_pt_index == 4) {
      */
      if (_cur_calib_pt_index == 2) {
         // if we have sampled four points, we are done with calibration
         _calib_mode_flag = false;
         _calibrated = true;

         WORLD::message("Calibration complete");
      }
      return 1;
   } else return 0;

}


bool
TRACE::init(CGESTUREptr&)
{
   /* old code
      cerr << "TRACE::init" << endl;

      TRACEptr me = get_instance();

      // for now, hardcode 2D image name
      me->_filename = str_ptr("testfile.pnm");

      me->_texture = new TEXTUREgl(me->_filename);

      if (!me->_texture->load_image() ) {
      cerr << "TRACE::init: could not load file" << me->_filename << endl;
      return false;
      }

      me->_texture->set_tex_fn(GL_REPLACE);

      me->_init_stamp = VIEW::stamp();
   */
   assert(0);
   return true;
}

bool
TRACE::go(double dur)
{
   /* old code
   // After a successful call to init(), this turns on a TRACE
   // to handle the trace session:

   cerr << "TRACE::go" << endl;

   TRACEptr me = get_instance();

   // Should be good to go: 
   if (me->_init_stamp != VIEW::stamp())
   return 0;

   // Set the timeout duration
   me->set_timeout(dur);

   // Become the active widget and get in the world's DRAWN list:
   me->activate();
   */
   assert(0);
   return true;
}

void
TRACE::do_calibrate()
{
   // set flags so we will be in calibration mode
   _calibrated = false;
   _calib_mode_flag = true;
   _cur_calib_pt_index = 0;
}

// end of file trace.C
