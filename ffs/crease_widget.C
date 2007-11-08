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
 *  \file crease_widget.C
 *  \brief Contains the definition of the CREASE_WIDGET widget.
 *
 *  \ingroup group_FFS
 *  \sa crease_widget.C
 *
 */
 
#include "disp/colors.H"                // Color::blue_pencil_d
#include "gtex/util.H"                  // for draw_strip
#include "gtex/ref_image.H"             // for VisRefImage
#include "gtex/basic_texture.H"         // GLStripCB
#include "std/run_avg.H"
#include "tess/mesh_op.H"

#include "crease_widget.H"

static bool debug = Config::get_var_bool("DEBUG_CREASE_WIDGET",false);

using namespace mlib;
using namespace GtexUtil;

/*****************************************************************
 * CREASE_WIDGET
 *****************************************************************/
CREASE_WIDGETptr CREASE_WIDGET::_instance;

CREASE_WIDGET::CREASE_WIDGET() :
   DrawWidget(),
   _strip(0)
{
   _draw_start += DrawArc(new TapGuard,      drawCB(&CREASE_WIDGET::cancel_cb));
   _draw_start += DrawArc(new ZipZapGuard,   drawCB(&CREASE_WIDGET::sharpen_cb));
   _draw_start += DrawArc(new SmallArcGuard, drawCB(&CREASE_WIDGET::smooth_cb));
   _draw_start += DrawArc(new StrokeGuard,   drawCB(&CREASE_WIDGET::stroke_cb));

   // Set up the clean up routine
   atexit(clean_on_exit);
}

void 
CREASE_WIDGET::clean_on_exit() 
{ 
   _instance = 0; 
}

CREASE_WIDGETptr
CREASE_WIDGET::get_instance()
{
   if (!_instance)
      _instance = new CREASE_WIDGET();
   return _instance;
}

inline PIXEL_list
subdiv_curve(Bedge* e, int rel_level)
{
   if (e) {
      Bvert_list chain;
      if (get_subdiv_chain(e->v1(), e->v2(), rel_level, chain)) {
         return PIXEL_list(chain.wpts());
      }
   }
   return PIXEL_list();
}

inline double
avg_dist(CPIXEL_list& A, CPIXEL_list& B, double& overlap, double step = 1.0)
{
   int n = max(1,int(round(A.length()/step)));  // number of steps
   double dt = 1.0/n;                           // step size in parameter [0,1]
   n++;                                         // number of samples
   RunningAvg<double> ret(0);
	
   if (A.empty()||B.empty())
	   return 0;

   for (int i=0; i<n; i++) {
      PIXEL sample = A.interpolate(i*dt);       // current sample
      PIXEL nearpt = B.closest(sample);         // near point on B

      // count the sample if the nearpt on B is not an endpoint:
      if (!(nearpt.is_equal(B.first()) || nearpt.is_equal(B.last()))) {
         ret.add(nearpt.dist(sample));
      }
   }

   // overlap is fraction of samples counted
   overlap = ret.num()/n;

   // return average distance to B over samples counted
   return ret.val();
}

inline Bedge*
best_match_align(CBedge_list& edges, CGESTUREptr& g)
{
   if (!(g && g->is_stroke()))
      return 0;

   BMESH* mesh = edges.mesh();
   if (!mesh) {
      err_adv(debug, "best_match_align: bad edge set");
      return 0;
   }

   Bedge* ret = 0;
   double min_dist = 0;
   int k = mesh->rel_cur_level(); // compare edges mapped to this level
   for (int i=0; i<edges.num(); i++) {
      double overlap = 0;
      const double MIN_ANGLE = 20; // minimum acceptable dihedral angle: 20 degrees
      if (rad2deg(edges[i]->dihedral_angle()) < MIN_ANGLE)
         continue;
      double d = avg_dist(g->pts(), subdiv_curve(edges[i], k), overlap);
      if (overlap < 0.8) {
         continue;
      }
      if (!ret || d < min_dist) {
         min_dist = d;
         ret = edges[i];
      }
   }
   err_adv(debug, "best_match_align: min_dist: %f", min_dist);
   const double MAX_AVG_DIST = 5;
   if (ret && min_dist > MAX_AVG_DIST) {
      err_adv(debug, "best_match_align: rejecting edge: bad match");
      ret = 0;
   } else {
      err_adv(debug, "best_match_align: %s edge", ret ? "found" : "could not find");
   }

   return ret;
}

//! Given zip-zap gesture g, return a control-level edge
//! that is well-aligned with g.
inline Bedge*
find_match_align(CGESTUREptr& g)
{

   // find Bfaces lying under screen path of g.
   // map to top level.
   // reject if not single mesh
   // return top level edge that best matches g
   //   measure edge chain in cur subdiv level
   // 
   if (!(g && g->is_zip_zap())) {
      err_adv(debug, "find_match_align: non-zip-zap");
      return 0;
   }

   VisRefImage *vis_ref = VisRefImage::lookup(VIEW::peek());
   if (!vis_ref) {
      err_adv(debug, "find_match_align: can't get vis ref image");
      return false;
   }

   const double RAD = 8;
   Bface_list faces = get_top_level(vis_ref->get_faces(g->pts(), RAD));
   if (!faces.same_mesh()) {
      err_adv(debug, "find_match_align: can't get top-level faces for gesture");
      return false;
   }

   err_adv(debug, "find_match_align: %d top-level faces", faces.num());
   
   return best_match_align(faces.get_edges(), g);
}

inline Bedge*
best_match_cross(CBedge_list& edges, CGESTUREptr& g)
{
   if (!(g && g->straightness() > 0.8))
      return 0;

   PIXELline segment = g->endpt_line();

   BMESH* mesh = edges.mesh();
   if (!mesh) {
      err_adv(debug, "best_match_cross: bad edge set");
      return 0;
   }

   Bedge* ret = 0;
   double min_dist = 0;
   int k = mesh->rel_cur_level(); // compare edges mapped to this level
   for (int i=0; i<edges.num(); i++) {
      const double MIN_ANGLE = 20; // minimum acceptable dihedral angle: 20 degrees
      if (rad2deg(edges[i]->dihedral_angle()) < MIN_ANGLE)
         continue;
      PIXEL_list edge_curve = subdiv_curve(edges[i], k);
      if (!edge_curve.intersects_seg(segment))
         continue;
      double d = edge_curve.dist(segment.midpt());
      if (!ret || d < min_dist) {
         min_dist = d;
         ret = edges[i];
      }
   }
   err_adv(debug, "best_match_cross: %s edge", ret ? "found" : "could not find");

   return ret;
}

//! Given straight gesture g (small arc), return a
//! control-level edge that g crosses.
inline Bedge*
find_match_cross(CGESTUREptr& g)
{

   // find Bfaces lying under screen path of g.
   // map to top level.
   // reject if not single mesh
   // return top level edge that best matches g
   //   measure edge chain in cur subdiv level
   // 
   if (!(g && g->straightness() > 0.8)) {
      err_adv(debug, "find_match_cross: bad straightness: %f", g->straightness());
      return 0;
   }

   VisRefImage *vis_ref = VisRefImage::lookup(VIEW::peek());
   if (!vis_ref) {
      err_adv(debug, "find_match_cross: can't get vis ref image");
      return false;
   }

   const double RAD = 8;
   Bface_list faces = get_top_level(vis_ref->get_faces(g->pts(), RAD));
   if (!faces.same_mesh()) {
      err_adv(debug, "find_match_cross: can't get top-level faces for gesture");
      return false;
   }

   err_adv(debug, "find_match_cross: %d top-level faces", faces.num());
   
   return best_match_cross(faces.get_edges(), g);
}

//! return an edge filter that accepts more edges
//! like the given one
SimplexFilter&
CREASE_WIDGET::get_filter(Bedge* e)
{

   assert(e);
   static SelectedSimplexFilter selected;
   static BoundaryEdgeFilter    sel_bdry(selected);
   static CreaseEdgeFilter      crease;
   static SharpEdgeFilter       sharp;
   
   if (selected.accept(e)) {
      return selected;
   } else if (sel_bdry.accept(e)) {
      return sel_bdry;
   } else if (e->is_crease()) {
      return crease;
   } else {
      sharp.set_angle(min(60.0, rad2deg(e->dihedral_angle())));
      return sharp;
   }
}

bool 
CREASE_WIDGET::init(CGESTUREptr& g)
{
   if (debug) cerr << "CREASE_WIDGET::init: ";

   CREASE_WIDGET* cw = get_instance();
   assert(cw);

   if (cw->is_active()) {
      err_adv(debug, "CREASE_WIDGET is already active");
      return false;
   }

   // check gesture
   if (!(g && g->is_stroke())) {
      err_adv(debug, "non-stroke");
      return false;
   } if (g->below_min_length() || g->below_min_spread()) {
      err_adv(debug, "short/slow stroke");
      return false;
   } if (!g->is_zip_zap()) {
      err_adv(debug, "not zip-zap");
      return false;
   }

   // find matching edge and get its mesh
   Bedge* e = find_match_align(g);
   if (!(e && e->mesh())) {
      err_adv(debug, "no match");
      return false;
   }
   if (!e->mesh()) {
      err_adv(debug, "no mesh");
      return false;
   }

   // activate the widget:
   return cw->init(e);
}

bool
CREASE_WIDGET::init(Bedge* e)
{
   assert(!is_active());
   assert(e && e->mesh());

   _mesh = e->mesh();
   _strip = _mesh->new_edge_strip(get_filter(e));
   activate();
   err_adv(debug, "CREASE_WIDGET::init: activated");
   return true;
}

bool
CREASE_WIDGET::add(Bedge* e)
{
   assert(is_active());
   assert(_strip != NULL);
   assert(e && e->mesh());
   if (e->mesh() != _mesh) {
      err_adv(debug, "CREASE_WIDGET::add: edge belongs to wrong mesh");
      return false;
   }
   _strip->add(e->v1(), e);
   err_adv(debug, "CREASE_WIDGET::add: added edge");
   return true;
}

const double SIL_SEARCH_RAD = 12;

void
CREASE_WIDGET::reset()
{
   delete _strip;
   _strip = 0;
   _mesh = 0;
}

int  
CREASE_WIDGET::cancel_cb(CGESTUREptr& gest, DrawState*& s)
{
   err_adv(debug, "CREASE_WIDGET::cancel_cb");

   // Turn off widget:
   deactivate();

   // Reset state to start:
   s = &_draw_start;

   return 1; // This means we did use up the gesture
}

int  
CREASE_WIDGET::tap_cb(CGESTUREptr& gest, DrawState*& s)
{
   err_adv(debug, "CREASE_WIDGET::tap_cb");

   return cancel_cb(gest, s);
}

inline void
mark_dirty(CBvert_list& verts, int bit = Lvert::SUBDIV_LOC_VALID_BIT)
{
   for (int i=0; i<verts.num(); i++)
      ((Lvert*)verts[i])->mark_dirty(bit);
}

int  
CREASE_WIDGET::sharpen_cb(CGESTUREptr& gest, DrawState*& s)
{
   err_adv(debug, "CREASE_WIDGET::sharpen_cb");
   assert(gest);
   assert(_strip && _mesh);

   Bedge* e = find_match_align(gest);
   if (!e) {
      err_adv(debug, "no matching edge");
      return 1;
   }
   if (e->mesh() != _mesh) {
      err_adv(debug, "found edge, but mesh is wrong");
      return 1;
   }
   if (_strip->edges().contains(e)) {
      LMESH* lm = lmesh();
      if (lm)
         err_adv(debug, "before sharpen: %d dirty verts", lm->dirty_verts().num());
      if (e->crease_val()<USHRT_MAX)
         WORLD::add_command(new CREASE_INC_CMD(_strip->edges(), 
            (ushort)_mesh->rel_cur_level(), true));
      //_strip->edges().inc_crease_vals((ushort)_mesh->rel_cur_level());
      if (lm)
         mark_dirty(_strip->edges().get_verts());
      _mesh->changed(BMESH::CREASES_CHANGED);
      if (lm)
         err_adv(debug, "after  sharpen: %d dirty verts", lm->dirty_verts().num());
   } else {
      add(e);
   }

   return 1; // This means we did use up the gesture
}

int  
CREASE_WIDGET::smooth_cb(CGESTUREptr& gest, DrawState*& s)
{
   err_adv(debug, "CREASE_WIDGET::smooth_cb");
   assert(gest);
   assert(_strip && _mesh);

   Bedge* e = find_match_cross(gest);
   if (!e) {
      err_adv(debug, "no matching edge");
      return 1;
   }
   if (e->mesh() != _mesh) {
      err_adv(debug, "found edge, but mesh is wrong");
      return 1;
   }
   if (_strip->edges().contains(e)) {
      if (e->crease_val()> 0)
         WORLD::add_command(new CREASE_INC_CMD(_strip->edges(), 
            (ushort)_mesh->rel_cur_level(), false));
      //_strip->edges().dec_crease_vals((ushort)_mesh->rel_cur_level());
      if (lmesh())
         mark_dirty(_strip->edges().get_verts());
      _mesh->changed(BMESH::CREASES_CHANGED);
   } else
      err_adv(debug, "found edge, but it isn't in the active set");

   return 1; // This means we did use up the gesture
}

int  
CREASE_WIDGET::stroke_cb(CGESTUREptr& gest, DrawState*& s)
{
   err_adv(debug, "CREASE_WIDGET::stroke_cb");
   assert(gest);

   // START HERE

   return 1; // This means we did use up the gesture
}

inline unsigned short
ctrl_crease_val(CBedge* e)
{
   // Return crease value of the control edge (if any)
   if (!(e && LMESH::isa(e->mesh())))
      return 0;
   Ledge* c = ((Ledge*)e)->ctrl_edge();
   return c ? c->crease_val() : 0;
}

/**********************************************************************
 * CreaseStripCB:
 **********************************************************************/
 //! \brief Draw an EdgeStrip with colors set according to crease
 //! value of edges.
class CreaseStripCB : public GLStripCB {
 public:
   CreaseStripCB(CCOLOR& col, double alpha=1) :
      _color(col), _alpha(alpha) {}

   virtual void edgeCB(CBvert* v, CBedge* e) {
      if (!(v && e))
         return;
      double a = _alpha * (1 - 0.8/(1 << ctrl_crease_val(e)));
      GL_COL(_color, a);
      glVertex3dv(v->loc().data());
   }

 private:
   COLOR        _color; //!< fixed color for strip
   double       _alpha; //!< base alpha
};

int 
CREASE_WIDGET::draw(CVIEWptr& v)
{
   if (!_strip || _strip->empty())
      return 0;

   static int old_num = 0;
   if (_strip->num() != old_num) {
      err_adv(debug, "CREASE_WIDGET::draw: strip has %d edges", _strip->num());
      old_num = _strip->num();
   }
   CreaseStripCB cb(Color::blue_pencil_d, 0.8);
   GtexUtil::draw_strip(*_strip, 6, &cb);

   return 0;
}

// end of file crease_widget.C
