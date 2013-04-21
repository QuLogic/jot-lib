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
 *  \file extender.C
 *  \brief Contains the definition of the EXTENDER widget.
 *
 *  \ingroup group_FFS
 *  \sa extender.H
 *
 */
#include "disp/colors.H"
#include "geom/gl_util.H"       // for GL_COL()
#include "geom/gl_view.H"       // for GL_VIEW::init_line_smooth()
#include "geom/world.H"         // for WORLD::undisplay()
#include "gtex/basic_texture.H" // for GLStripCB
#include "gtex/ref_image.H"     // for VisRefImage
#include "npr/ffstexture.H"
#include "tess/primitive.H"
#include "tess/uv_surface.H"
#include "tess/tex_body.H"
#include "tess/ti.H"
#include "std/config.H"

#include "ffs/draw_pen.H"
#include "ffs/floor.H"

#include "extender.H"

using namespace mlib;
using namespace tess;

static bool debug = Config::get_var_bool("DEBUG_EXTENDER",false);

/*****************************************************************
 * UTILITIES
 *****************************************************************/
inline bool 
is_stylized() 
{
   return VIEW::peek()->rendering() == FFSTexture::static_name(); 
}

inline BMESHptr
other_mesh(const TEXBODY* t, CBMESHptr& m)
{
   if (!t || !t->contains(m) || t->num_meshes() != 2)
      return 0;
   return (t->mesh(0) == m) ? t->mesh(1) : t->mesh(0);
}

inline bool
merge_surfaces(BMESHptr m1, BMESHptr m2)
{
   // Preconditions:
   //   m1 and m2 are "surface" meshes, controlled by
   //   Bsurfaces.  Each is contained in a TEXBODY, that
   //   contains 1 other mesh as well -- a "skeleton" mesh,
   //   controlled by Bpoints and Bcurves.

   // Now we want to merge the two surface meshes, which means we
   // also merge the two skeleton meshes.

   //
   // XXX - not finished: assumes meshes are at the control level
   //       should fix to work w/ level > 0

   if (!(m1 && m2)) {
      err_adv(debug, "merge_surfaces: Error: One or both meshes is NULL");
      return false;
   }
   if (m1 == m2) {
      err_adv(debug, "merge_surfaces: Warning: It's the same mesh twice!!");
      return false;
   }
   if (get_ctrl_mesh(m1) == get_ctrl_mesh(m2)) {
      err_adv(debug, "merge_surfaces: Error: meshes share same control mesh");
      return false;
   }

   // Verify two closed surfaces
//    if (!m1->is_closed_surface() || !m2->is_closed_surface()) {
//       err_adv(debug, "merge_surfaces: Error: Both meshes not closed surfaces");
//       return false;
//    }

   // Verify two TEXBODYs
   TEXBODY* t1 = TEXBODY::upcast(m1->geom());
   TEXBODY* t2 = TEXBODY::upcast(m2->geom());
   if (!(t1 && t2)) {
      err_adv(debug, "merge_surfaces: Error: can't get TEXBODYs");
      return false;
   }

   // Can't think why this would happen:
   if (t1 == t2) {
      err_adv(debug, "merge_surfaces: both meshes contained in same TEXBODY");
      return false;     // might be over-reaction
   }

   if (t1->num_meshes() != t2->num_meshes()) {
      err_adv(debug, "merge_surfaces: Error: TEXBODY num meshes mis-match");
      return false;
   }

   for (int i=0; i<t1->num_meshes(); i++) {
      BMESH::merge(t1->mesh(i), t2->mesh(i));
   }

//    // Get skeleton meshes
//    BMESHptr s1 = other_mesh(t1, m1);
//    BMESHptr s2 = other_mesh(t2, m2);

//    // The skeleton meshes can be any mix of points, polylines,
//    // and open surfaces. They just can't have closed surfaces:
//    if (s1 && s2) {
//       if (s1->is_closed_surface() || s2->is_closed_surface()) {
//          err_adv(debug, "merge_surfaces: both meshes not closed surfaces");
//          return false;
//       }
//    }

//    err_adv(debug, "merge_surfaces: merging two pairs of meshes");
//    BMESH::merge(m1,m2);
//    if (s1 && s2)
//       BMESH::merge(s1,s2);

//    // Rename if needed so m1 is the merged mesh, m2 the emptied one.
//    if (m1->empty()) {
//       swap(m1,m2);
//       swap(s1,s2);
//       swap(t1,t2);
//    }

//    // If the skeleton meshes merged the other way, we have to
//    // pull s2 out of t2 and put it in t1, replacing s1. Follow?
//    if (s1 && s1->empty() && s2 && !s2->empty()) {
//       t1->remove(s1);
//       t1->add(s2);      // also takes it from t2
//    }

   // Poor t2 is left with two withered husks of meshes,
   // remove it from the WORLD's lists:
   WORLD::destroy(GELptr(t2));

   return true;
}

/*****************************************************************
 * EXTENDER
 *****************************************************************/
EXTENDERptr EXTENDER::_instance;

EXTENDERptr
EXTENDER::get_instance()
{
   if (!_instance)
      _instance = new EXTENDER();
   return _instance;
}

EXTENDER::EXTENDER() :
   DrawWidget()
{
   //*******************************************************
   // FSA states for handling GESTUREs:
   //
   //   Note: the order matters. First matched will be executed.
   //*******************************************************

   _draw_start += DrawArc(new DoubleTapGuard,   drawCB(&EXTENDER::cancel_cb));
   _draw_start += DrawArc(new TapGuard,         drawCB(&EXTENDER::cancel_cb));
   _draw_start += DrawArc(new ScribbleGuard,    drawCB(&EXTENDER::cancel_cb));
   _draw_start += DrawArc(new CircleGuard(1.35),drawCB(&EXTENDER::circle_cb));
   _draw_start += DrawArc(new EllipseGuard,     drawCB(&EXTENDER::ellipse_cb));
   _draw_start += DrawArc(new LineGuard,        drawCB(&EXTENDER::line_cb));
   _draw_start += DrawArc(new StrokeGuard,      drawCB(&EXTENDER::stroke_cb));

   // Set up the clean up routine
   atexit(clean_on_exit);
}

void 
EXTENDER::clean_on_exit() 
{ 
   _instance = 0; 
}

bool 
EXTENDER::init(CGESTUREptr& g)
{
   if (get_instance()->sweep_ball(g))
      return true;

   if (!(g && g->is_double_tap()))
      return false;

   // lookup the Bface from the VisRefImage:
   Bface* f = VisRefImage::get_edit_face();
   if (!f)
      return false;

   double ndc_area = f->ndc_area();
   double pix_area = ndc_area * sqr(VIEW::peek()->ndc2pix_scale());
   double MIN_PIXEL_AREA = 5; // about half of a 3x3 pixel quad
   if (pix_area < MIN_PIXEL_AREA) {
      err_adv(debug, "EXTENDER::init: Bface too small (area: %f sq pix)",
              pix_area);
      return false;
   }

   Bface_list base = f->is_selected() ? 
      Bface_list::reachable_faces(f, !SelectedFaceBoundaryEdgeFilter()) : Bface_list(f);
   return get_instance()->add(base);
}

void 
EXTENDER::reset() 
{
   _base1.clear();
   _base2.clear();
   _strip.reset();
   _plane_xf = Identity;
}


inline bool
was_tapped(Bbase* b, CGESTUREptr& g)
{
   if (!(b && g && g->prev() && g->prev()->is_tap()))
      return false;

   return (
      b == Bbase::find_controller(VisRefImage::get_simplex(g->prev()->center()))
      );
}

inline Wpt
face_center(Bface* f)
{
   assert(f);
   if (f->is_quad())
      return f->quad_centroid();
   return f->centroid();
}

inline Wvec
face_norm(Bface* f)
{
   assert(f);
   if (f->is_quad())
      return f->quad_norm();
   return f->norm();
}

//! \brief choose x vector according to eye vector
inline Wvec
choose_x(CEdgeStrip& strip, Wpt o)
{
   Wvec e = VIEW::eye_vec(o); // unit vector from eye to o
   Bedge_list edges = strip.edges();
   assert(!edges.empty());

   Wvec ret = edges[0]->vec().normalized();
   double min_dot = fabs(ret * e);
   for (int i = 1; i < edges.num(); i++) {
      if (fabs(edges[i]->vec().normalized() * e) < min_dot) {
         ret = edges[i]->vec().normalized();
         min_dot = fabs(ret * e);
      }
   }
   return ret;
}

bool
EXTENDER::sweep_ball(CGESTUREptr& g)
{
   static bool debug = ::debug ||
      Config::get_var_bool("DEBUG_EXTENDER_SWEEP_BALL",false);

   err_adv(debug, "EXTENDER::sweep_ball");

   if (!(g && g->is_stroke())) {
      err_adv(debug, "  non-stroke");
      return false;
   }

   if (g->length() < 40) {
      err_adv(debug, "  stroke too short (%f pixels)", g->length());
      return false;
   }

   Bface* f = VisRefImage::get_ctrl_face(g->start());
   if (!f) {
      err_adv(debug, "  no face");
      return false;
   }

   Primitive* p = Primitive::find_controller(f);

   if (!(p && p->is_ball())) {
      err_adv(debug, "  non-ball");
      return false;
   }

   if (!(p->is_selected() || was_tapped(p,g) || p->is_last())) {
      err_adv(debug, "  ball, but not special");
      return false;
   }

   if (!p->outputs().empty()) {
      err_adv(debug, "  ball is already used");
      return false;
   }

   // XXX - ignoring mesh transform
   Wpt  o = face_center(f); // center of hit face
   Wvec n = cross(choose_x(Bface_list(f).get_boundary(), o), face_norm(f));

   err_adv(debug, " can sweep ball");

   MULTI_CMDptr cmd = new MULTI_CMD;
   push(p->bfaces(), cmd);

   Primitive * tube = Primitive::build_simple_tube(
      p->mesh(),
      p->skel_points().first(),
      radius(p->bverts().pts()),
      n,
      g->pts(),
      cmd
      );
   err_adv(debug, "  %s", tube ? "succeeded" : "failed");
   return tube != 0;
}

bool
EXTENDER::add(Bface_list faces, double)
{
   err_adv(debug, "EXTENDER::add");

   // shouldn't happen but check anyway:
   if (faces.empty()) {
      err_adv(debug, "EXTENDER::add: faces is empty");
      return false;
   }

   // should be a disk
   if (!faces.is_disk()) {
      err_adv(debug, "EXTENDER::add: faces should form a disk");
      return false;
   }

   if (!LMESH::isa(faces.mesh())) {
      return false;
   }

   if (faces.contains_any(_base1) || faces.contains_any(_base2)) {
      return false;
   }

   if (has_secondary_any_level(faces)) {
      err_adv(debug, "EXTENDER::add: faces has subdiv edits");
      return false;
   }

   // If there are already 2 bases,
   // get ready to replace oldest:
   if (!_base1.empty() && !_base2.empty()) {
      err_adv(debug, "EXTENDER::add: replacing old base");
      _base1 = _base2;
      _base2.clear();
   }

   // Base areas should match
   if (!_base1.empty() && _base1.boundary_edges().num() != faces.boundary_edges().num()) {
      err_adv(debug, "EXTENDER::add: boundaries dont match");
      return false;
   }

   // If adding a 2nd base, and it's from a different mesh, then
   // try to merge the meshes (see merge_surfaces(), above). If
   // they can't be merged then forget the 1st base and make like
   // we're starting over with the new base.
   if (!_base1.empty() && _base1.mesh() != faces.mesh()) {
      // merge two different meshes
      // XXX - should be undoable
      if (merge_surfaces(_base1.mesh(), faces.mesh())) {
         err_adv(debug, "EXTENDER::add: merged two meshes");
      } else {
         reset();
         err_adv(debug, "EXTENDER::add: could not merge meshes");
      }
   }

   // add it:
   if (!_base1.empty())  _base2 = faces;
   else         _base1 = faces;

   // make sure it wasn't a duplicate:
   if (_base1.same_elements(_base2))
      _base2.clear();

   // build the strip for drawing selected base(s)
   build_strip();

   // set the timeout duration
   reset_timeout();

   err_adv(debug, "   bases: %d", (_base1.empty()?0:1) + (_base2.empty()?0:1));

   // become the active widget and
   // get in the world's DRAWN list:
   activate();

   return true;
}

//! \brief rebuild the edge strip that runs around the boundary of the base
//! regions.
void
EXTENDER::build_strip()
{

   Bface_list base_regions = _base1 + _base2;

   _strip = base_regions.get_boundary();
}

// ! \brief return the approximate dimension of a pt list
inline double
pts_dim(CWpt_list& pts)
{
   double ret = 0;
   Wpt c = pts.average();
   for (int i = 0; i < pts.num(); i++) {
      if (c.dist(pts[i]) > ret)
         ret = c.dist(pts[i]);
   }
   return 1.414 * ret;
}

//! \brief After 1 or 2 bases are selected,
//! define the plane for drawing in.
void
EXTENDER::compute_plane()
{

   // Need a base
   if (_base1.empty()) {
      err_adv(debug, "EXTENDER::compute_plane: no bases defined");
      return;
   }

   // get best fit planes
   Wplane P1, P2;
   if (!_base1.get_boundary().verts().pts().get_plane(P1, 0.1)) {
      err_adv(debug, "EXTENDER::compute_plane: base1 not planar enough");
      return;
   }
   if (_base1[0]->norm() * P1.normal() < 0)
      P1 = -P1;
   if (!_base2.empty() && !_base2.get_boundary().verts().pts().get_plane(P2, 0.1)) {
      err_adv(debug, "EXTENDER::compute_plane: base2 not planar enough");
      return;
   }
   if (!_base2.empty() && (_base2[0]->norm()*P2.normal()<0))
      P2 = -P2;

   //  (-1,1)                         (1,1)           
   //     . . . . . . . y . . . . . . . .              
   //     .             |               .              
   //     .             |               .              
   //     .             |               .              
   //     .             |               .              
   //     < - - - - - - o - - - - - - - > x            
   //     .    inside   |   surface     .              
   //     . . . . . . . . . . . . . . . .              
   //  (-1,-a)                        (1,-a)
   //
   //  The draw plane will be defined via an "origin"
   //  (point o above, center of verts) and two
   //  vectors, x and y.  y is derived from the normal
   //  of best fit plane, and x is chosen based on criteria below.
   //  The value 'a' is a small amount, like 0.2.
   // 
   //  See EXTENDER::draw() for how this is all used.

   Wvec y;
   Wvec x;
   Wpt  o;
   double width = 0;    // length of x

   Wpt o1 = _base1.get_verts().center();
   if (!_base2.empty()) {
      Wpt o2 = _base2.get_verts().center();

      // origin is midpoint between base centroids:
      o = (o1 + o2)*0.5;

      // Choose x pointing from center of 1st base to the other:
      x = (o2 - o1).normalized();

      // y is average of best fit plane normals, minus the
      // component along x, normalized:
      y = cross(x, cross(P1.normal() + P2.normal(),x))
         .normalized();

      // Fix up if needed:
      // XXX - should fix the fix if needed
      if (y.is_null())
         y = cross(VIEW::eye_vec(o),x).normalized();

      // Choose a width:
      width = (pts_dim(_base1.get_verts().pts()) +
               pts_dim(_base2.get_verts().pts()) +
               o1.dist(o2))/2;

   } else {
      // origin is base centroid
      o = o1;

      // Use best fit plane normal for y:
      y = P1.normal();

      // For x, choose boundary-edge-aligned vector most
      // perpendicular to view vector.
      x = choose_x(_base1.get_boundary(), o);

      // Choose a width:
      width = pts_dim(_base1.get_verts().pts());
   }

   // Set up transform for the draw plane from its uv-coords.
   _plane_xf = 
      Wtransf(o, x*width, y*(width*0.62), cross(x, y).normalized()) *
      Wtransf::scaling(Wvec(width, width*0.62, 1.0));
}

int
EXTENDER::draw(CVIEWptr& v)
{
   // define the draw plane location
   compute_plane();
		
   // draw orange boundary around bases

   if (_strip.mesh()) {

      // Coordinates are in mesh object space -- 
      // setup modelview matrix
      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();
      glMultMatrixd(_strip.mesh()->xform().transpose().matrix());

      // set line smoothing, set line width, and push attributes:
      GL_VIEW::init_line_smooth(GLfloat(v->line_scale()*3.0), GL_CURRENT_BIT);

      // no face culling
      glDisable(GL_CULL_FACE);

      // no lighting
      glDisable(GL_LIGHTING);           // GL_ENABLE_BIT

      // Draw base boundaries      
      if(is_stylized())
         GL_COL(COLOR(.52,.05,.286), 0.7*alpha());
      else
         GL_COL(Color::orange, alpha());   // GL_CURRENT_BIT
      GLStripCB cb;
      _strip.draw(&cb);

      // draw the drawing plane:    
      if(is_stylized())
         GL_COL(COLOR(.407,0.0,.105), 0.3*alpha());
      else
         GL_COL(Color::orange, 0.3*alpha());
             
      double a = -0.2;
      glBegin(GL_TRIANGLE_STRIP);
      //                                         
      //    (-1,1)--------(1,1)                  
      //       |         /  |                    
      //       |       /    |                    
      //       |     /      |                    
      //       |   /        |                    
      //       | /          |                    
      //    (-1,a)--------(1,a)                 
      //       
      glVertex3dv((_plane_xf * Wpt(-1, 1, 0)).data());
      glVertex3dv((_plane_xf * Wpt(-1, a, 0)).data());
      glVertex3dv((_plane_xf * Wpt( 1, 1, 0)).data());
      glVertex3dv((_plane_xf * Wpt( 1, a, 0)).data());
      glEnd();

      // restore GL state
      GL_VIEW::end_line_smooth();

      // restore matrix
      glPopMatrix();
   }  

   // when all faded away, undraw
   if (alpha() == 0)
      WORLD::undisplay(this, false);

   return 0;
}

int  
EXTENDER::cancel_cb(CGESTUREptr& gest, DrawState*& s)
{
   err_adv(debug, "EXTENDER::cancel_cb");

   if (gest->is_tap()) {
      // If gesture is a tap on a mesh, ignore it
      // because it could be the 1st part of a double tap
      if (VisRefImage::get_edit_face())
         return( 0 );
   }

   // Turn off widget:
   deactivate();

   // Reset state to start:
   s = &_draw_start;


   return 1; // This means we did use up the gesture
}

int  
EXTENDER::ellipse_cb(CGESTUREptr& gest, DrawState*& s)
{
   err_adv(debug, "EXTENDER::ellipse_cb");

   cancel_cb(gest, s);

   return 1; // This means we used up the gesture
}

int  
EXTENDER::circle_cb(CGESTUREptr& gest, DrawState*& s)
{
   // this callback only applies to base area consisting of a single quad

   // Plan:
   //   * do ray intersect at center of circle
   //   * get corresponding top-level Bface
   //   * ensure it's quad1 or quad2
   //   * map the circle to a rectangle in UV-space of quad
   //   * create 4 new verts at rectangle corners
   //   * blow out the quad
   //   * stitch it all back together
   //   * later - put a proper Bsurface on the stitched region

   // Diagnostic:
   bool debug = ::debug ||
      Config::get_var_bool("DEBUG_EXTENDER_CIRCLE_CB",false);

   err_adv(debug, "EXTENDER::circle_cb");

   XYpt    c = gest->center();            // get center of circle
   Bface* bf = VisRefImage::Intersect(c); // do exact ray intersection

   if (!bf) {
      err_adv(debug, "EXTENDER::circle_cb: intersect failed");
      cancel_cb(gest, s);
      return 1; // This means we used up the gesture
   }

   // We only deal with LMESHes
   if (!LMESH::isa(bf->mesh())) {
      err_adv(debug, "EXTENDER::circle_cb: intersected non-LMESH");
      return 1; // This means we used up the gesture
   }

   // Up-cast
   Lface* f = (Lface*)bf;

   Wvec bc; // Barycentric coords of hit point on triangle
   if (!f->ray_intersect(c, bc)) {
      err_adv(debug, "EXTENDER::circle_cb: 2nd intersect failed!?");
      return 1; // This means we used up the gesture
   }
   
   // Leaving open the option of this being nonzero later:
   int top_level = 0;

   // Get to corresponding face at the top level mesh, and
   // map the given barycentric coords of the hit point to
   // corresponding barycentric coords on the top-level
   // face:
   f = f->bc_to_level(top_level, bc, bc);
   if (!f) {
      err_adv(debug, "EXTENDER::circle_cb: can't get top-level face");
      return 1; // This means we used up the gesture
   }

   // Should never fail:
   if (!f->is_quad()) {
      err_adv(debug, "EXTENDER::circle_cb: circle not drawn on a quad");
      return 1; // This means we used up the gesture
   }

   // Need to draw the circle on a selected quad:
   if (_base1.empty()) {
      err_adv(debug, "EXTENDER::circle_cb: no quad selected");
      return 1; // This means we used up the gesture
   } 

   if (!_base1.contains_all(Bface_list(f)) &&
       (_base2.empty() || !_base2.contains_all(Bface_list(f)))) {
      err_adv(debug, "EXTENDER::circle_cb: circle not on selected base");
      return 1; // This means we used up the gesture
   }

   // Remember which selected quad we're actually dealing with:
   if ((_base1.contains(f) && _base1.num() != 2) ||
      (_base2.contains(f) && _base2.num() != 2)) {
      err_adv(debug, "EXTENDER::circle_cb: base is not a quad");
      return 1; // This means we used up the gesture
   }
   bool from_base1 = _base1.contains(f);
   Bface*& quad = from_base1 ? _base1[0] : _base2[0];

   // Convert barycentric coords on the triangle to
   // UV-coords on the quad:
   UVpt uv_center = f->quad_bc_to_uv(bc);
   
   // Get world-space radius for circle gesture:
   double r = world_length(f->quad_uv2loc(uv_center), gest->radius());

   // Get lengths in u and v dimensions for circle:
   double urad = r / f->quad_dim1();
   double vrad = r / f->quad_dim2();

   // Don't let the edges of the proposed hole get too close to
   // the edge of the quad
   const double MARGIN= 0.01; // amount is relative to quad size
   if ((uv_center[0] - urad < MARGIN) ||
       (uv_center[1] - vrad < MARGIN) ||
       (uv_center[0] + urad > 1 - MARGIN) ||
       (uv_center[1] + vrad > 1 - MARGIN)) {
      err_adv(debug, "EXTENDER::circle_cb: circle not sufficiently inside quad");
      return 1; // This means we used up the gesture
   }

   // Get quad vertices in standard order:
   Bvert *v1, *v2, *v3, *v4;
   if (!f->get_quad_verts(v1, v2, v3, v4)) {
      err_adv(debug, "EXTENDER::circle_cb: can't get quad vertices");
      return 1; // This means we used up the gesture
   }

   // Preserve UV-coordinates while we're at it (if they exist):
   UVpt uv1, uv2, uv3, uv4, uva, uvb, uvc, uvd;
   bool do_uv = (
      UVdata::get_quad_uvs(f, uv1, uv2, uv3, uv4)                           &&
      UVdata::quad_interp_texcoord(f, uv_center + UVvec(-urad, -vrad), uva) &&
      UVdata::quad_interp_texcoord(f, uv_center + UVvec( urad, -vrad), uvb) &&
      UVdata::quad_interp_texcoord(f, uv_center + UVvec( urad,  vrad), uvc) &&
      UVdata::quad_interp_texcoord(f, uv_center + UVvec(-urad,  vrad), uvd)
      );
   
   UVsurface* surf = UVsurface::find_owner(f);

   Lface* inset = 0;
   if (do_uv && surf && (inset = surf->inset_quad( v1,  v2,  v3,  v4,
                                                   uva, uvb, uvc, uvd))) {
      quad = inset;

   } else {

      // Have to do everything ourselves...

      // We'll need these...
      BMESH*    bmesh = f->mesh();
      Patch*        p = f->patch();     // XXX - needs work

      // Get the 4 points of the propsed hole:
      Wpt A = f->quad_uv2loc(uv_center + UVvec(-urad, -vrad));
      Wpt B = f->quad_uv2loc(uv_center + UVvec( urad, -vrad));
      Wpt C = f->quad_uv2loc(uv_center + UVvec( urad,  vrad));
      Wpt D = f->quad_uv2loc(uv_center + UVvec(-urad,  vrad));

      // Create the new vertices:
      Lvert* va = (Lvert*)bmesh->add_vertex(A);
      Lvert* vb = (Lvert*)bmesh->add_vertex(B);
      Lvert* vc = (Lvert*)bmesh->add_vertex(C);
      Lvert* vd = (Lvert*)bmesh->add_vertex(D);
   
      // Blow out the quad (this deletes f)
      bmesh->remove_edge(f->weak_edge());


      // Stitch it up
      //
      //   v4 ---------- v3                              
      //    |            |                                
      //    |   vd---vc  |                                
      //    |    |   |   |                                
      //    |    |   |   |                                
      //    |   va---vb  |                                
      //    |            |                                
      //   v1 ---------- v2                               
      //
      //
      // XXX - TODO: put a Bsurface on it
      bmesh->add_quad(v1, v2, vb, va, p);
      bmesh->add_quad(v2, v3, vc, vb, p);
      bmesh->add_quad(v3, v4, vd, vc, p);
      bmesh->add_quad(v4, v1, va, vd, p);
      bmesh->add_quad(va, vb, vc, vd, p);

      if (do_uv) {
         if (!(UVdata::set(v1,v2,vb,va,uv1,uv2,uvb,uva) &&
               UVdata::set(v2,v3,vc,vb,uv2,uv3,uvc,uvb) &&
               UVdata::set(v3,v4,vd,vc,uv3,uv4,uvd,uvc) &&
               UVdata::set(v4,v1,va,vd,uv4,uv1,uva,uvd) &&
               UVdata::set(va,vb,vc,vd,uva,uvb,uvc,uvd)))
            err_adv(debug, "EXTENDER::circle_cb: couldn't propagate UV-coords");
      }

      bmesh->changed(BMESH::TRIANGULATION_CHANGED);

      // Replace the blown quad with the new one
      quad = lookup_face(va, vb, vc);
   }

   // XXX - fix this, lines copied from EXTENDER::add():
   // define the draw plane
   if (from_base1)
      _base1 = Bface_list(quad);
   else
      _base2 = Bface_list(quad);
   compute_plane();

   // build the strip for drawing selected quad(s)
   build_strip();

   // set the timeout duration
   reset_timeout();

   // become the active widget and
   // get in the world's DRAWN list:
   activate();

   return 1; // This means we used up the gesture
}

int  
EXTENDER::line_cb(CGESTUREptr& gest, DrawState*& s)
{
   err_adv(debug, "EXTENDER::line_cb");

   return stroke_cb(gest, s);
}

//! \brief Process the gesture... hopefully the result will be to
//! sweep out a new branch of the Primitive
//!
//! This function checks some conditions and possibly rejects the
//! stroke. Otherwise it calls the static function
//! Primitive::extend_branch(), which checks further conditions and
//! possibly rejects the stroke. Otherwise it creates a Primitive
//! instance and calls the member function Primitive::extend() to
//! generate the new geometry. For more info see comments in
//! tess/primitive.H above the declaration of Primitive::extend().
int  
EXTENDER::stroke_cb(CGESTUREptr& gest, DrawState*& s)
{

   bool debug = ::debug || Config::get_var_bool("DEBUG_EXTENDER_STROKE_CB",false);

   err_adv(debug, "EXTENDER::stroke_cb");

   // Need a selected base to extend a new primitive branch:
   if (_base1.empty()) {
      err_adv(debug, "EXTENDER::stroke_cb: no base -- canceling");
      return cancel_cb(gest, s);
   }

   // Check for various gesture types we can't use:
   if (gest->is_tap() ||
       gest->is_scribble() ||
       gest->is_closed()) {
      err_adv(debug, "EXTENDER::stroke_cb: bad stroke type (%s) -- canceling",
              gest->is_tap() ? "tap" :
              gest->is_scribble() ? "scribble" : "closed");
      return cancel_cb(gest, s);
   }

   // Haven't ruled it out yet. Call Primitive::extend_branch() to
   // check further conditions and do the operation if it's okay.
   MULTI_CMDptr cmd = new MULTI_CMD;
   Primitive* branch =
      Primitive::extend_branch(gest->pts(), _plane_xf.Z(), _base1, _base2, cmd);
   if (!branch)
      return cancel_cb(gest, s);
   //FLOOR::realign(branch->cur_mesh(), cmd);
   WORLD::add_command(cmd);
   err_adv(debug || Config::get_var_bool("DEBUG_JOT_CMD",false),
           "added undo primitive (%d commands)",
           cmd->commands().num());

   deactivate();

   return 1;  // this means we did use up the gesture
}

/* end of file extender.C */
