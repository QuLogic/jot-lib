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
 * wireframe.C
 **********************************************************************/
#include "disp/colors.H"
#include "std/config.H"
#include "geom/gl_view.H"
#include "mesh/bfilters.H"
#include "wireframe.H"
#include "ref_image.H" // for debugging d2d samples

static bool debug = Config::get_var_bool("DEBUG_WIREFRAME", false);

static const bool debug_samples =
   Config::get_var_bool("DEBUG_D2D_SAMPLES",false);

/**********************************************************************
 * WireframeTexture:
 **********************************************************************/
bool  WireframeTexture::_show_quad_diagonals = false;
COLOR WireframeTexture::_default_line_color = Color::black;

void
WireframeTexture::set_default_line_color(CCOLOR& color) 
{
   _default_line_color = color;
}

bool 
WireframeTexture::draws_filled() const 
{
   // for testing d2d samples we need to draw filled triangles
   return debug_samples;
}

int
WireframeTexture::draw(CVIEWptr& v)
{
   if (_ctrl)
      return _ctrl->draw(v);

   // 'a' is opacity (alpha) determined by GL_VIEW::init_line_smooth()
   // needed to simulate line widths below the minimum supported value:
   GLfloat a = 1;
   static bool line_smooth = Config::get_var_bool("ANTIALIAS_WIREFRAME",false);
   if (line_smooth) {
      // set line smoothing, set line width, and push attributes:
      GLfloat w = GLfloat(v->line_scale()*_width);
      GL_VIEW::init_line_smooth(w, GL_CURRENT_BIT);//, &a);
   } else {
      glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT);
      glLineWidth(_width);
   }

   // set state
   GL_COL(_color, a*alpha());   // GL_CURRENT_BIT
   glDisable(GL_LIGHTING);      // GL_ENABLE_BIT

   if (!(_quad_diagonals_in_dl == _show_quad_diagonals &&
         BasicTexture::draw(v))) {

      _quad_diagonals_in_dl = _show_quad_diagonals;

      int dl = _dl.get_dl(v, 1, _patch->stamp());
      if (dl)
         glNewList(dl, GL_COMPILE);

      // Build the edge strip at the current subdivision level
      Bedge_list edges = _patch->cur_edges();
      edges.clear_flags();

      // If secondary edges shouldn't be drawn, set their flags
      // so they won't be drawn:
      if (!BMESH::show_secondary_faces())
         edges.secondary_edges().set_flags(1);

      err_adv(debug, "wireframe: %d edges", edges.num());

      // Construct filter that accepts unreached strong edges of
      // this patch:
      UnreachedSimplexFilter    unreached;
      StrongEdgeFilter          strong;
      PatchEdgeFilter           mine(_patch->cur_patch());
      if (_show_quad_diagonals)
         EdgeStrip(edges, unreached + mine).draw(_cb);
      else
         EdgeStrip(edges, unreached + strong + mine).draw(_cb);

      // end the display list here
      if (_dl.dl(v)) {
         _dl.close_dl(v);

         // the display list is built; now execute it
         BasicTexture::draw(v);
      }
   }

   glPopAttrib();

   if (debug_samples) {
      draw_d2d_samples();
   }

   return _patch->num_faces();
}

void
WireframeTexture::request_ref_imgs()
{
   if (debug_samples) {
      IDRefImage::schedule_update();
   }
}

void
WireframeTexture::draw_d2d_samples()
{
   assert(_patch);
   _patch->update_dynamic_samples();

   CWtransf& xf = _patch->xform();
   Wtransf xfn = _patch->inv_xform().transpose();

   Wpt eye = VIEW::peek()->eye(); // camera location in world space
   Wvec v = VIEW::peek_cam()->data()->right_v(); // world space right vector
   double scale = xf.X().length();               // assumes uniform scaling
   double r = scale*_patch->get_sps_spacing()/2; // spacing, world space

   GL_VIEW::init_point_smooth(6.0f, GL_CURRENT_BIT);
   glDisable(GL_LIGHTING);
   glColor3fv(float3(Color::red_pencil));

   const vector<DynamicSample>& samples = _patch->get_samples();
   for (uint i=0; i<samples.size(); i++) {
      // we want to set the size of each point separately:
      Wpt p = samples[i].get_pos(); // object space
      Wpt w = xf*p; // world space
      Wvec n = (xfn*samples[i].get_norm()).normalized(); // normal, world space
      double nv = (eye - w).normalized() * n;

      if (1) { //_patch->get_use_visibility_test()) {
         nv *= IDVisibilityTest().visibility(w, r);
      }
      if (nv <= 0)
         continue;

      // diameter in pixels:
      double d = nv*PIXEL(w + v*r).dist(PIXEL(w - v*r));
      if (d > 1e-2) {
         glPointSize(d);
         glBegin(GL_POINTS);
         glVertex3fv(float3(p));
         glEnd();
      }
   }
   GL_VIEW::end_point_smooth();
}

// end of file wireframe.C
