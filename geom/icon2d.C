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
/*************************************************************************
 *    FILE: ICON2d.C
 *************************************************************************/

#include "std/support.H"
#include "glew/glew.H"

#include "disp/colors.H"
#include "geom/gl_util.H"       // float4
#include "geom/icon2d.H"

using namespace mlib;

static int tm=DECODER_ADD(ICON2D);

static bool debug = Config::get_var_bool("DEBUG_ICON2D",false);

DLhandler ICON2D::_dl;

void 
ICON2D::recompute_xform() 
{
   // If this ICON is attached to a 3D point, project it to screen space
   if (!_is2d) 
      _pt2d = xform().origin();
   else _pt2d = XYpt(xform().origin()[0], xform().origin()[1]);
}

ICON2D::ICON2D(Cstr_ptr &n,
               Cstr_ptr &filename,
               int num,
               bool tog,
               const mlib::PIXEL &p) :
   GEOM(n),
   _is2d(0),
   _center(0),
   _can_intersect(1),
   _show_boxes(0),
   _filename(Config::JOT_ROOT() + filename),
   _cam(num),
   _suppress_draw(false),
   _active(false),
   _toggle(false),
   _hide(false)
{  

   if (debug)
      cerr << "ICON2D::ICON2D: loading file: " << _filename << endl;
   
   //load "normal" state texture  
   _texture += new TEXTUREgl(_filename + ".png", GL_TEXTURE_2D, GL_TEXTURE0);
   assert(!_texture.empty() && _texture.last());;
   _texture.last()->set_tex_fn(GL_REPLACE);

   if (_texture.last()->load_image()) {
      _suppress_draw = false;
   } else {
      cerr << "ICON2D error : texture not loaded" << endl;
      _suppress_draw = true;
   }

   // load "active" state texture
   if (tog) {
      _act_tex = new TEXTUREgl(_filename + "_active.png",
                               GL_TEXTURE_2D, GL_TEXTURE0); 
      assert(_act_tex);
      _act_tex->set_tex_fn(GL_REPLACE);
   
      if (_act_tex->load_image()) {
         _suppress_draw = false;
      } else {
         cerr << "ICON2D error : active texture not loaded" << endl;
         _suppress_draw = true;
      }
   }
   //icon location
   _name = n;
   _toggle = tog;
   _pix = p;
   _currentTex = 0;
   _skins.push(_filename);      //add the first skin to the array
}

inline bool
load_texture(TEXTUREglptr& tex)
{
   // Load the texture (should be already allocated TEXTUREgl
   // with filename set as member variable).
   //
   // Note: Does not activate the texture.
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

/*************************************************************************
 * Function Name: ICON2D::draw
 * Parameters: 
 * Returns: int
 * Effects: 
 *************************************************************************/
int
ICON2D::draw(
   CVIEWptr &view
   )
{
   // leave the ID reference image alone,
   // and don't draw ICON into screen grabs:
   if (_suppress_draw || view->grabbing_screen() || _hide)
      return 0;

   TEXTUREglptr tex = _texture[_currentTex];

   if (!load_texture(tex)) {
      cerr << "ICON2D::draw: can't load texture, skipping..." << endl;
      return 0;
   }

   //set up needed attributes
   glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);

   // Load identity for model view
   glMatrixMode(GL_MODELVIEW);  // GL_TRANSFORM_BIT
   glPushMatrix();
   glLoadIdentity();

   // Setup projection for drawing in PIXEL coords
   glMatrixMode(GL_PROJECTION); // GL_TRANSFORM_BIT
   glPushMatrix();
   glLoadMatrixd(view->pix_proj().transpose().matrix());
   
   glDisable(GL_LIGHTING);      // GL_ENABLE_BIT
   glDisable(GL_CULL_FACE);     // GL_ENABLE_BIT
   glDisable(GL_DEPTH_TEST);    // GL_ENABLE_BIT

   // XXX - probably don't need alpha blending, but it's on:
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   // activate texture
   if (_toggle && _active)
      _act_tex->apply_texture();                // texture for active icon
   else
      _texture[_currentTex]->apply_texture();   // texture for normal icon

   // XXX - debug: color should be covered by the texture:
   glColor4fv(float4(Color::yellow,0.5));

   // draw a textured quad in pixel space for the icon:
   glBegin(GL_QUAD_STRIP);
   glTexCoord2f(0.0, 1.0); glVertex2dv((_pix + VEXEL( 0,32)).data());
   glTexCoord2f(0.0, 0.0); glVertex2dv((_pix + VEXEL( 0, 0)).data());
   glTexCoord2f(1.0, 1.0); glVertex2dv((_pix + VEXEL(32,32)).data());
   glTexCoord2f(1.0, 0.0); glVertex2dv((_pix + VEXEL(32, 0)).data());
   glEnd();

   // restore the matrix stacks:
   glMatrixMode(GL_PROJECTION);
   glPopMatrix();
   glMatrixMode(GL_MODELVIEW);
   glPopMatrix();
   
   //restore attributes
   glPopAttrib();

   return 0; 
}

RAYhit&
ICON2D::intersect(
   RAYhit   &r,
   CWtransf &m,
   int       //uv
   ) const
{
        
   // Only pick ICON when it is associated w/ a 3D point and we know
   // the window size
   if (_can_intersect && !_suppress_draw) {
           
      //((ICON2D *)this)->recompute_xform(); // Update ICON location
      BBOX2D bbox(bbox2d(0,0));            // Get bounding box 

      //CXYpt pick_pt(r.point() + r.vec());
      PIXEL pick_pt(r.point() + r.vec());
      if (bbox.contains(pick_pt)) {  // picked!
         Wvec ray(r.point() - xform().origin());
         // assume ICON is always close to the camera
         // for now, that means 0.1 distance along viewing vector
         r.check(0, 0, 0, (ICON2D *)this, ray.normalized(), 
                 r.point() + r.vec()*0.1, Wpt::Origin(),
                 (APPEAR *) this, PIXEL());

         return r;
      }
   }

   GEOM::intersect(r,m); // Check if children were intersected
   return r;
}

/*************************************************************************
 * Function Name: icon2d::bbox
 * Effects: 
 *************************************************************************/
BBOX2D
ICON2D::bbox2d(
   int       border, 
   char     *s,
   int       force
   ) const
{
   PIXEL start, endpt;

   start = _pix;
   endpt = PIXEL(_pix[0] + 32.0,_pix[1] + 32.0);

   return BBOX2D(start, endpt);
}

void 
ICON2D::add_skin(Cstr_ptr &n)
{
   _texture.push(new TEXTUREgl(_filename + ".png")); 
   assert(!_texture.empty() && _texture.first());
   _texture.first()->set_tex_fn(GL_REPLACE);

   if (!_texture.first()->load_image())
      cerr << "ICON2D error : texture not loaded" << endl;

   _currentTex++;
   //_skins.push(n);
}

void 
ICON2D::update_skin()
{
   _currentTex++;
   if(_currentTex >= _texture.num())
      _currentTex = 0;
}

// icon2d.C
