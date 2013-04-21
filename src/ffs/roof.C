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
 *  \file roof.C
 *  \brief Contains the definition of the ROOF widget.
 *
 *  \ingroup group_FFS
 *  \sa roof.H
 *
 */
#include "disp/colors.H"
#include "geom/gl_util.H"       // for GL_COL()
#include "geom/gl_view.H"       // for GL_VIEW::init_line_smooth()
#include "geom/world.H"         // for WORLD::undisplay()
#include "gtex/basic_texture.H" // for GLStripCB
#include "gtex/ref_image.H"     // for VisRefImage
#include "tess/primitive.H"
#include "tess/ti.H"

#include "ffs/floor.H"

#include "roof.H"

using namespace mlib;
using namespace tess;

static bool debug = Config::get_var_bool("DEBUG_ROOF",false);
const double SEARCH_RAD = 10;

/*****************************************************************
 * ROOF
 *****************************************************************/
ROOFptr ROOF::_instance;

ROOFptr
ROOF::get_instance()
{
   if (!_instance)
      _instance = new ROOF();
   return _instance;
}

ROOF::ROOF() :
   DrawWidget()
{
   //*******************************************************
   // FSA states for handling GESTUREs:
   //
   //   Note: the order matters. First matched will be executed.
   //*******************************************************

   _draw_start += DrawArc(new TapGuard,         drawCB(&ROOF::cancel_cb));
   _draw_start += DrawArc(new ScribbleGuard,    drawCB(&ROOF::cancel_cb));
   _draw_start += DrawArc(new StrokeGuard,      drawCB(&ROOF::stroke_cb));

   // Set up the clean up routine
   atexit(clean_on_exit);
}

void 
ROOF::clean_on_exit() 
{ 
   _instance = 0; 
}

// get connected sub regions
inline ARRAY<Bface_list> 
get_regions(CBface_list& cregion)
{
   ARRAY<Bface_list> ret;
   Bface_list region = cregion;

   while(!region.empty()) {
      region.get_verts().clear_flag02();
      region.set_flags(1);
      Bface_list comp(region.num());
      comp.grow_connected(region.first());
      ret += comp;
      region -= comp;
   }

   if (debug) cerr << "num of regions:" << ret.num() << endl;

   return ret;
}

// XXX - the impl of this doesn't consider edge swaps
inline bool
is_rect(CBface_list& region)
{
   // need to consist only of quads
   for (int i = 0; i < region.num(); i++) {
      if (!region[i]->is_quad())
         return false;
   }

   region.exterior_faces().clear_flags();
   region.set_flags(1);

   // face degree of boundary verts should not exceed 3
   Bvert_list verts = region.get_boundary().verts();
   for (int i = 0; i < verts.num(); i++) {
      if (verts[i]->face_degree(SimplexFlagFilter(1)) > 3)
         return false;
   }

   region.clear_flags();
   return true;
}

// this method assumes that the input region is a rect area
// plz refer to the method is_rect(), and it also doesn't take into
// account of edge swaps
inline EdgeStrip
extract_rect_side(CBface_list& rect, Bedge* edge)
{
   EdgeStrip ret;
   rect.exterior_faces().clear_flags();
   rect.set_flags(1);
   rect.boundary_edges().set_flags(2);

   // go one way until reach corner
   Bvert* v = edge->v2();
   Bedge* e = edge;
   while (v->face_degree(SimplexFlagFilter(1)) > 2) {
      Bedge_list star = v->get_manifold_edges().filter(SimplexFlagFilter(2));
      assert(star.num() == 2);
      e = (star[0]==e)?star[1]:star[0];
      v = e->other_vertex(v);
   }

   // go the other way to extract the side
   ret.add(v, e);
   v = e->other_vertex(v);
   while (v->face_degree(SimplexFlagFilter(1)) > 2) {
      Bedge_list star = v->get_manifold_edges().filter(SimplexFlagFilter(2));
      assert(star.num() == 2);
      e = (star[0]==e)?star[1]:star[0];
      ret.add(v, e);
      v = e->other_vertex(v);
   }

   rect.clear_flags();
   rect.boundary_edges().clear_flags();

   Bvert_list bound_verts = rect.get_boundary().verts();
   int loc = bound_verts.get_index(ret.first());
   bound_verts.shift(-loc);
   assert(bound_verts[0] == ret.first());
   if (bound_verts[1] != ret.vert(1) && bound_verts[1] != ret.last())
      return ret.get_reverse();
   return ret;
}

// if the input region has all and only the subdivision faces of
// a ctrl region, return this ctrl region
// this is a recursive process
inline Bface_list
ctrl_faces(Bface_list& region)
{
   if (!LMESH::isa(region.mesh()))
      return Bface_list();
   Bface_list ret(region.num()/4);
   for (int i = 0; i < region.num(); i++) {
      Lface* f = ((Lface*)region[i])->parent();
      if (!f)
         return region;
      if (!ret.contains(f)) {
         ARRAY<Bface*> faces;
         f->append_subdiv_faces(1, faces);
         if (region.contains_all(faces))
            ret.add_uniquely(f);
         else
            return region;
      }
   }

   return ctrl_faces(ret);
}

// return the parent edgestrip (if it exists) at the given level up
// from the input strip
inline EdgeStrip
parent_edges(EdgeStrip& strip, int rel_lev)
{
   EdgeStrip ret;
   Bvert* v = ((Lvert*)(strip.first()))->parent_vert(rel_lev);
   if (!v)
      return ret;

   for (int i = 0; i < strip.num(); i++) {
      Bedge* e = ((Ledge*)(strip.edge(i)))->parent_edge(rel_lev);
      if (!e) 
         return EdgeStrip();
      if (ret.empty() || e != ret.edges().last()) {
         ret.add(v, e);
         v = e->other_vertex(v);
      }
   }

   return ret;
}

bool 
ROOF::init(CGESTUREptr& g)
{
   if (!(g && g->is_dslash()))
      return false;

   Bedge* e = 0;
   BMESH* m = 0;
   CPIXEL p = g->start();
   ROOFptr me = get_instance();

   // get the mesh that roof is built on
   if ((e = VisRefImage::get_edge(p, SEARCH_RAD))) {
      m = e->mesh();
   } else if (!(m = VisRefImage::get_mesh(p)) || m==FLOOR::lookup_mesh())
      return false;

   // need to have selected faces
   Bface_list faces = MeshGlobal::selected_faces_all_levels(m);
   if (faces.empty())
      return false;

   // region has to be rectangular and consisted of quads
   ARRAY<Bface_list> regions = get_regions(faces);
   ARRAY<Bface_list> rect_regions;
   for (int i = 0; i < regions.num(); i++) {
      if (is_rect(regions[i]))
         rect_regions += regions[i];
   }
   if (rect_regions.empty())
      return false;

   // dslash has to start at the boundary of a rect area
   EdgeStrip bound_strip;
   int seg_index = 0, j = 0;
   for (j = 0; j < rect_regions.num(); j++) {
      bound_strip = rect_regions[j].get_boundary();
      PIXEL_list bound_wpts = bound_strip.verts().wpts();
      bound_wpts += bound_wpts.first();
      PIXEL nearpt;
      double neardist;
      bound_wpts.closest(p, nearpt, neardist, seg_index);
      if (neardist < SEARCH_RAD) 
         break;
   }
   if (j == rect_regions.num())
      return false;

   // setup protected values
   me->_edges = extract_rect_side(rect_regions[j], bound_strip.edge(seg_index));
   me->_bases = ctrl_faces(rect_regions[j]);
   if (debug)
      cerr << "has " << rect_regions[j].num() << " selected faces and "
         << me->_bases.num() << " ctrl faces" << endl;
   int rel_lev = rect_regions[j].mesh()->subdiv_level() - me->_bases.mesh()->subdiv_level();
   me->_edges = parent_edges(me->_edges, rel_lev);
   if (debug)
      cerr << "ctrl side has " << me->_edges.num() << " edges" << endl;

   me->activate();
   return true;
}

void 
ROOF::reset() 
{
   _bases.clear();
   _edges.reset();
}

//! \brief After widget is initiated,
//! define the plane for drawing in.
void
ROOF::compute_plane()
{

   // Need a strip
   if (_edges.empty()) {
      err_adv(debug, "ROOF::compute_plane: no strip found");
      return;
   }

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
   //  (point o above, near the surface) and two
   //  vectors, x and y.  y is derived from the strip
   //  normal, and x is chosen based on criteria below.
   //  The value 'a' is a small amount, like 0.2.
   // 
   //  See ROOF::draw() for how this is all used.

   Wpt o = (_edges.first()->loc() + _edges.last()->loc()) * 0.5;
   Wvec x = (_edges.last()->loc() - _edges.first()->loc()).normalized();
   Bedge* e = _edges.edge(0);
   Wvec y = e->f1()->norm();
   if (!_bases.contains(e->f1())) {
      assert(_bases.contains(e->f2()));
      y = e->f2()->norm();
   }
   
   for (int i = 1; i < _edges.num(); i++)
      y += _edges.edge(i)->norm();
   y.normalized();
   double width = (_edges.last()->loc() - _edges.first()->loc()).length();

   // Set up transform for the draw plane from its uv-coords.
   _plane_xf = 
      Wtransf(o, x*width, y*(width*0.62), cross(x, y).normalized()) *
      Wtransf::scaling(Wvec(width, width*0.62, 1.0));
}

int
ROOF::draw(CVIEWptr& v)
{
   // define the draw plane location
   compute_plane();
		
   if (_edges.mesh()) {

      // Coordinates are in mesh object space -- 
      // setup modelview matrix
      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();
      glMultMatrixd(_edges.mesh()->xform().transpose().matrix());

      // set line smoothing, set line width, and push attributes:
      GL_VIEW::init_line_smooth(GLfloat(v->line_scale()*3.0), GL_CURRENT_BIT);

      // no face culling
      glDisable(GL_CULL_FACE);

      // no lighting
      glDisable(GL_LIGHTING);           // GL_ENABLE_BIT

      // Draw rect boundaries      
      GL_COL(Color::red, alpha());   // GL_CURRENT_BIT
      GLStripCB cb;
      _bases.get_boundary().draw(&cb);

      // draw the drawing plane:    
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
ROOF::cancel_cb(CGESTUREptr& gest, DrawState*& s)
{
   err_adv(debug, "ROOF::cancel_cb");

   // Turn off widget:
   deactivate();

   // Reset state to start:
   s = &_draw_start;

   return 1; // This means we did use up the gesture
}

//! \brief Process the gesture... hopefully the result will be to
//! successfully construct a roof
//!
int  
ROOF::stroke_cb(CGESTUREptr& gest, DrawState*& s)
{
   
   bool debug = ::debug || Config::get_var_bool("DEBUG_ROOF_STROKE_CB",true);

   err_adv(debug, "ROOF::stroke_cb");

   // Need a selected strip to extend a new roof:
   if (_edges.empty()) {
      err_adv(debug, "ROOF::stroke_cb: no strip -- canceling");
      return cancel_cb(gest, s);
   }

   // Check for various gesture types we can't use:
   if (gest->is_tap() ||
       gest->is_scribble() ||
       gest->is_closed()) {
      err_adv(debug, "ROOF::stroke_cb: bad stroke type (%s) -- canceling",
              gest->is_tap() ? "tap" :
              gest->is_scribble() ? "scribble" : "closed");
      return cancel_cb(gest, s);
   }

   // check for valid start and end position
   PIXEL gs = gest->start(), ge = gest->end(),
      ef = _edges.first()->wloc(), el = _edges.last()->wloc();
   if (!(gs.dist(ef)<SEARCH_RAD && ge.dist(el)<SEARCH_RAD) &&
      !(gs.dist(el)<SEARCH_RAD && ge.dist(ef)<SEARCH_RAD))
      return cancel_cb(gest, s);

   // Haven't ruled it out yet. Call Primitive::build_roof() to
   // check further conditions and do the operation if it's okay.
   MULTI_CMDptr cmd = new MULTI_CMD;
   int orig_corner_angle = Config::get_var_int("JOT_CORNER_ANGLE", 45);
   Config::set_var_int("JOT_CORNER_ANGLE",30);
   Primitive* roof =
      Primitive::build_roof(gest->pts(), gest->corners(), _plane_xf.Z(), _bases, _edges, cmd);
   Config::set_var_int("JOT_CORNER_ANGLE",orig_corner_angle);
   if (!roof)
      return cancel_cb(gest, s);
   WORLD::add_command(cmd);
   err_adv(debug || Config::get_var_bool("DEBUG_JOT_CMD",false),
           "added undo primitive (%d commands)",
           cmd->commands().num());

   deactivate();

   return 1;  // this means we did use up the gesture
}

/* end of file roof.C */
