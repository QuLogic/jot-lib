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
 * panel.C:
 **********************************************************************/
#include "mesh/mi.H"
#include "std/config.H"
#include "mlib/polyline.H"
#include "bvert_grid.H"
#include "panel.H"
#include "ti.H"

using namespace mlib;
using namespace tess;

static bool debug = Config::get_var_bool("DEBUG_PANEL",false);

inline Bface_list
cur_subdiv_faces(Bface_list in)
{
   Bface_list out;
   if (in.empty()) return out;
   int level = (in[0]->mesh())->rel_cur_level();

   for (int i = 0; i < in.num(); i++)
      ((Lface*)(in[i]))->append_subdiv_faces(level, out);
   return out;
}

/************************************************************
 * PCellCornerVertFilter:
 *
 *  Accepts a vertex whose if it lies on a "corner" of
 *  a PCell. I.e., either a Bpoint controls the vertex
 *  or more than one PCell is adjacent to the vertex.
 *
 ************************************************************/
bool 
PCellCornerVertFilter::accept(CBsimplex* s) const
{
   if (!is_vert(s))
      return false;
   Bvert* v = (Bvert*)s;

   return (Bpoint::find_controller(v) != NULL ||
           Panel::find_cells(v->get_faces()).num() > 1);
}

/************************************************************
 * PCell:
 ************************************************************/
PCell_list 
PCell::nbrs() const 
{
   // return list of neighboring cells that share a boundary
   // edge with this cell
   //
   // find exterior faces that lie across a boundary edge from
   // this cell
   Bface_list ext = _faces.exterior_faces();
   ext.clear_flags();
   _faces.get_boundary().edges().get_faces().set_flags(1);
   return Panel::find_cells(ext.filter(SimplexFlagFilter(1)));
}

Wpt
PCell::center() const
{
   return cur_subdiv_faces(_faces).get_verts().center();
}

PCell_list 
PCell::internal_nbrs() const 
{
   PCell_list ret = nbrs();
   for (int i=ret.num()-1; i >= 0; --i) {
      if (!same_panel(ret[i]))
         ret.remove(i);
   }
   return ret;
}

PCell_list 
PCell::external_nbrs() const 
{
   PCell_list ret = nbrs();
   for (int i=ret.num()-1; i >= 0; --i) {
      if (same_panel(ret[i]))
         ret.remove(i);
   }
   return ret;
}

inline ARRAY<Bedge_list>
get_boundaries(const PCell_list& cells)
{
   ARRAY<Bedge_list> ret(cells.num());
   for (int i=0; i<cells.num(); i++)
      ret += cells[i]->boundary_edges();
   return ret;
}

Bedge_list
PCell::shared_edges(const PCell_list& cells)
{
   ARRAY<Bedge_list> boundaries = get_boundaries(cells);

   {
      // clear all flags
      for (int i=0; i<boundaries.num(); i++)
         boundaries[i].clear_flags();
   }
   {
      // increment all edge flags
      for (int i=0; i<boundaries.num(); i++)
         boundaries[i].inc_flags();
   }
   // the shared ones will have flag value = 2
   Bedge_list ret;
   {
      for (int i=0; i<boundaries.num(); i++)
         ret += boundaries[i].filter(SimplexFlagFilter(2));
   }
   return ret;
}

/************************************************************
 * Panel:
 ************************************************************/
bool    Panel::_show_skel = false;

Panel::Panel(Panel* parent)
{
   // Creates a child Panel of the given parent.
   set_parent(parent);

   // XXX - need a correct way to identify the list of inputs.
   //       for now we're using the child points and curves
   //       of our parent's points and curves lists.
   hookup();

   // Make sure we'll be recomputed
   invalidate();
}

Panel::~Panel()
{
   destructor();
}

void
Panel::add_cell(CBface_list& faces)
{
   Bface_list new_faces;
   for (int i = 0; i < faces.num(); i++) {
      if (!find_cell(faces[i]))
         new_faces += faces[i];
   }
   _cells += new PCell(this, new_faces);
}

void
Panel::add_cell(FaceMeme* fm) 
{
   static bool debug = ::debug || Config::get_var_bool("DEBUG_PANEL_CELLS",false);
   if (!(fm && fm->face())) {
      err_adv(debug, "Panell::add_cell: error: null face meme");
      return;
   }
   add_cell(Bface_list(fm->face())); // adds both halves of a quad
}

void
Panel::draw_skeleton() const
{
   if (_cells.empty())
      return;

   {
      // draw centers of cells as dots
      //
      // Enable point smoothing and push gl state:
      GL_VIEW::init_point_smooth(10, GL_CURRENT_BIT);
      glDisable(GL_LIGHTING);                              // GL_ENABLE_BIT
      glBegin(GL_POINTS);
      glColor4fv(float4(Color::blue_pencil_d, 0.8));       // GL_CURRENT_BIT
      for (int i=0; i<_cells.num(); i++) {
         if (!_cells[i]->is_empty()) {
            glVertex3dv(_cells[i]->center().data());
         }
      }
      glEnd();
      GL_VIEW::end_point_smooth();      // pop state
   }

   {
      // draw lines connecting adjacent internal cells
      GL_VIEW::init_line_smooth(8, GL_CURRENT_BIT);
      glDisable(GL_LIGHTING);                           // GL_ENABLE_BIT
      glBegin(GL_LINES);
      // use medium blue for internal edges
      glColor4fv(float4(Color::blue_pencil_m, 0.8));    // GL_CURRENT_BIT
      for (int i=0; i<_cells.num(); i++) {
         PCell_list nbrs = _cells[i]->internal_nbrs();
         for (int j=0; j<nbrs.num(); j++) {
            glVertex3dv(_cells[i]->center().data());
            glVertex3dv(  nbrs[j]->center().data());
         }
      }
      glEnd();
      GL_VIEW::end_line_smooth();
   }

   {
      // draw lines connecting adjacent external cells
      GL_VIEW::init_line_smooth(8, GL_CURRENT_BIT);
      glDisable(GL_LIGHTING);                           // GL_ENABLE_BIT
      glBegin(GL_LINES);
      // use light orange for internal edges
      glColor4fv(float4(Color::orange_pencil_l, 0.8));  // GL_CURRENT_BIT
      for (int i=0; i<_cells.num(); i++) {
         PCell_list nbrs = _cells[i]->external_nbrs();
         for (int j=0; j<nbrs.num(); j++) {
            glVertex3dv(_cells[i]->center().data());
            glVertex3dv(  nbrs[j]->center().data());
         }
      }
      glEnd();
      GL_VIEW::end_line_smooth();      // pop state 
   }
}

int 
Panel::draw(CVIEWptr& v) 
{
   int ret = Bsurface::draw(v);
   if (ret && _show_skel) {
      assert(control()); // haig-like
      control()->draw_skeleton();
   }
   return ret;
}

Bnode_list 
Panel::inputs()  const 
{ 
   return Bsurface::inputs() + _bcurves + _bpoints;
}

Panel*
Panel::create(CBcurve_list& contour, CARRAY<int> ns)
{
   // Create a Panel to fill a simple closed boundary:

   if (contour.empty()) {
      return 0;
   }

   Panel* ret = new Panel();
   if (ret->tessellate_tri  (contour, ns) ||        // try for triangle
       ret->tessellate_quad (contour, ns) ||        // try for quad
       ret->tessellate_disk (contour, ns))          // go for disk
      return ret;

   ARRAY<Bcurve_list> multi;
   multi += contour;
   if (ret->tessellate_multi(multi))            // fall back on basics
      return ret;

   err_adv(debug, "Panel::create: failed");

   // have a hissy fit
   delete ret;
   return 0;
}

Panel*
Panel::create(CBvert_list& verts)
{
   // Create a Panel to fill a simple closed boundary:

   Panel* ret = new Panel();
   if (ret->tessellate_disk(verts)) // go for disk
      return ret;

   // give up
   err_adv(debug, "Panel::create: failed");
   delete ret;
   return 0;
}

Panel*
Panel::create(CARRAY<Bcurve_list>& contours)
{
   if (contours.empty()) {
      err_adv(debug, "Panel::create: empty contour list");
      return 0;
   }
   if (contours.num() == 1)
      return create(contours.first());

   // Multiple contours, e.g. enclosed surface has holes or
   // consists of disconnected pieces.
   Panel* ret = new Panel;
   if (ret->tessellate_multi(contours))
      return ret;

   err_adv(debug, "Panel::create: failed");
   delete ret;
   return 0;
}

static bool debug_tess = Config::get_var_bool("DEBUG_TESSELLATE",false);

bool
Panel::prepare_tessellation(LMESH* m)
{
   if (!(is_control() && m)) {
      err_adv(debug && !is_control(),
              "Panel::prepare_tessellation: error: non-control panel");
      err_adv(debug && !m, "Panel::prepare_tessellation: error: null mesh");
      return false;
   }
   
   // Delete all mesh elements controlled by this Panel
   if (debug && !bfaces().empty())
      err_msg("Panel::prepare_tessellation: warning: deleting %d faces",
              bfaces().num());
   delete_elements();
   _cells.clear(); 

   // Undo dependencies in case panel was previously built
   unhook();

   // clear old info:
   if (!(_bcurves.empty() && _bpoints.empty())) {
      err_msg("Panel::prepare_tessellation: clearing old curve/point lists");
      _bcurves.clear();
      _bpoints.clear();
   }

   // associate with this mesh:
   set_mesh(m);

   return true;
}

bool
Panel::prepare_tessellation(CBcurve_list& contour)
{
   if (!prepare_tessellation(contour.mesh())) {
      err_adv(debug, "Panel::prepare_tessellation: failed");
      return false;
   }

   // record new control curves:
   absorb(contour);

   return true;
}

bool
Panel::finish_tessellation()
{
   err_adv(debug, "Panel::finish_tessellation: %d faces", bfaces().num());

   // Done building, tell mesh:
   _mesh->changed();

   // Set rest length of edges to the average initial length
//   _ememes.set_rest_length(bedges().avg_len());

   // register as an ouput with each control curve:
   hookup();

   invalidate();

   // Choose a res level:
   int res_lev = _bcurves.max_res_level();
   res_lev = min(res_lev, 2);
   if (debug) {
      cerr << "  setting res level " << res_lev << " for: " << endl;
      cerr << "    points: ";
      _bpoints.print_identifiers();
      cerr << "    curves: ";
      _bcurves.print_identifiers();
   }
   set_res_level(res_lev);
   _bcurves.set_res_level(res_lev);
   _bpoints.set_res_level(res_lev);

   // update subdivision to chosen level
   _mesh->update_subdivision(res_lev);

   if (0 && debug) {
      for (Panel* p = this; p; p = upcast(p->child())) {
         cerr << p->identifier() << ": "
              << p->vmemes().num() << " memes / "
              << p->bverts().num() << " verts" << endl;
      }
   }

   // Now check for the damage.
   // Should have more sophisticated check of whether things are ok:
   if (_patch->edges().any_satisfy(StressedEdgeFilter())) {
      // Ugh. Tessellation is effed up
      err_adv(debug, "Panel::finish_tessellation: found stressed edges");
   } else {
      err_adv(debug, "Panel::finish_tessellation: succeeded");
   }

   _bpoints.activate();
   _bcurves.activate();
   activate();

   return true;
}

bool
Panel::tessellate_disk(CBcurve_list& contour, CARRAY<int> ns)
{
   if (ns.num()!=0 && ns.num()!=contour.num())
      return false;
   for (int i = 0; i < ns.num(); i++)
      if (ns[i]<=0) return false;
   bool create_mode = (ns.num()==0);

   // For "disk" tessellation we accept a closed chain of
   // curves that hopefully winds around its center

   if (!create_mode)
      prepare_tessellation(contour);
   if (create_mode && contour.num() > 2) {
      resample(contour, 1);
   } else if (!create_mode) {
      for (int i = 0; i < contour.num(); i++)
         resample(contour[i], ns[i]);
   }
   Bvert_list verts;
   if (!contour.extract_boundary_ccw(verts)) {
      err_adv(debug, "Panel::tessellate_disk: can't extract CCW boundary");
      return false;
   }

   return tessellate_disk(verts, create_mode);
}

bool
Panel::tessellate_disk(CBvert_list& verts, bool create_mode)
{
   // For "disk" tessellation we accept a closed chain of
   // vertices that hopefully winds around its center

   if (!verts.forms_closed_chain()) {
      err_adv(debug, "Panel::tessellate_disk: bad vertex list");
      return false;
   }

   // Do common setup for tessellation
   if (create_mode && !prepare_tessellation(LMESH::upcast(verts.mesh())))
      return false;

   // get "center of mass"
   Wpt center = verts.center();

   Lvert* cv = (Lvert*)_mesh->add_vertex(center);
   add_vert_meme(cv);
   add_vert_memes(verts);
   invalidate(); // memes are hot, sign up for updates

   // Build faces
   int n = verts.num();
   for (int k=0; k<n; k++)
      add_face(verts[k], cv, verts[(k+n-1)%n]);

   // create a single cell for the disk:
   add_cell(bfaces());

   if (create_mode) {
      // Keep a record of the curves and points:
      Bcurve_list curves = Bcurve::get_curves(verts.get_chain());
      curves.add_uniquely(Bcurve::find_controller(lookup_edge(verts.last(), verts.first())));
      curves.reverse();
      absorb(curves);
      absorb(Bpoint::get_points(verts));
   }

   // Do common clean-up after tessellation
   return finish_tessellation();
}

bool
Panel::tessellate_banded_disk(CBcurve_list& contour)
{
   // For "disk" tessellation we accept a closed chain of
   // curves that hopefully winds around its center

   if (!contour.forms_closed_chain())
      return false;

   // Do common setup for tessellation
   if (!prepare_tessellation(contour))
      return false;

   Bvert_list verts;
   if (!contour.extract_boundary_ccw(verts))
      return false;

   // create center vert:
   Wpt center = contour.center();
   Lvert* cv = (Lvert*)_mesh->add_vertex(center);

   // make interior band of vertices:
   Bvert_list inner;
   int n = verts.num();
   for (int i=0; i<n; i++)
      inner += _mesh->add_vertex(interp(center, verts[i]->loc(), 0.55));

   // create memes before we forget
   add_vert_meme(cv);
   add_vert_memes(verts);
   add_vert_memes(inner);

   // Build faces
   for (int k=0; k<n; k++) {
      int j = (k+n-1)%n;
      add_quad(verts[k], inner[k], inner[j], verts[j]);
      add_face(inner[j], inner[k], cv);
   }

   // create a single cell for the disk:
   add_cell(bfaces());

   // Do common clean-up after tessellation
   return finish_tessellation();
}

bool
Panel::split_tri(Bpoint* bp, Bcurve* bc, CPIXEL_list& pts, bool scribble)
{
   assert(_bcurves.num() == 3);
   assert(Bpoint::hit_ctrl_point(pts.first(), 8) == bp);
   assert(Bcurve::hit_ctrl_curve(pts.last(), 8) == bc);
   static bool debug = ::debug || Config::get_var_bool("DEBUG_SPLIT_TRI",false);
   err_adv(debug, "Panel::split_tri");

   if (!_bpoints.contains(bp) || !_bcurves.contains(bc))
      return false;
   if (bc->contains(bp))
      return false;
   if (scribble && bc->num_edges()!=2)
      return false;

   // XXX - can add code here to restrict the endpoint of the stroke to be near
   //     - center of bc.
   //     - I choose to let the condition be more relax

   Bcurve_list c = _bcurves;
   if (c[0] == bc)
      c.shift(1);
   ARRAY<int> ns;
   for (int i = 0; i < 3; i++) {
      if (c[i]!=bc || scribble)
         ns += 1;
      else
         ns += 2;
   }

   PNL_RETESS_CMDptr cmd = new PNL_RETESS_CMD(this, c, ns);
   WORLD::add_command(cmd);
   return true;
}

bool
Panel::inc_sec_disk(Bcurve* bc1, Bcurve* bc2, CPIXEL_list& pts, bool scribble)
{
   assert((bc1&&!bc2) || (!bc1&&bc2));
   if (bc1) assert(Bcurve::hit_ctrl_curve(pts.first(), 8) == bc1);
   if (bc2) assert(Bcurve::hit_ctrl_curve(pts.last(), 8) == bc2);
   static bool debug = ::debug || Config::get_var_bool("DEBUG_INC_SEC_DISK",false);
   err_adv(debug, "Panel::inc_sec_disk");

   Bcurve* bc = bc1 ? bc1 : bc2;
   if (!_bcurves.contains(bc))
      return false;
   if (scribble && bc->num_edges()==1)
      return false;
   
   ARRAY<int> ns;
   for (int i = 0; i < _bcurves.num(); i++)
      if (_bcurves[i] != bc)
         ns += _bcurves[i]->num_edges();
      else
         ns += bc->num_edges() + (scribble ? -1 : 1);
   PNL_RETESS_CMDptr cmd = new PNL_RETESS_CMD(this, _bcurves, ns);
   WORLD::add_command(cmd);
   return true;
}

bool
Panel::inc_sec_tri(Bcurve* bc1, Bcurve* bc2, CPIXEL_list& pts, bool scribble)
{
   assert(_bcurves.num() == 3);
   assert(Bcurve::hit_ctrl_curve(pts.first(), 8) == bc1);
   assert(Bcurve::hit_ctrl_curve(pts.last(), 8) == bc2);
   static bool debug = ::debug || Config::get_var_bool("DEBUG_INC_SEC_TRI",false);
   err_adv(debug, "Panel::inc_sec_tri");

   if (!_bcurves.contains(bc1) || !_bcurves.contains(bc2))
      return false;
   if (bc1==bc2 || bc1->num_edges()!=bc2->num_edges())
      return false;
   if (scribble && bc1->num_edges()==1)
      return false;

   Bcurve_list c = _bcurves;
   c -= bc1;
   c -= bc2;
   c += bc1;
   c += bc2;

   // XXX - can add code here to restrict the stroke to be parallel to bc0
   //     - I choose to let the condition be more relax

   ARRAY<int> ns;
   ns += 1;
   ns += bc1->num_edges()+(scribble ? -1 : 1);
   ns += bc2->num_edges()+(scribble ? -1 : 1);
   PNL_RETESS_CMDptr cmd = new PNL_RETESS_CMD(this, c, ns);
   WORLD::add_command(cmd);
   return true;
}

bool
Panel::inc_sec_quad(Bcurve* bc1, Bcurve* bc2, CPIXEL_list& pts, bool scribble)
{
   assert(_bcurves.num() == 4);
   assert(Bcurve::hit_ctrl_curve(pts.first(), 8) == bc1);
   assert(Bcurve::hit_ctrl_curve(pts.last(), 8) == bc2);
   static bool debug = ::debug || Config::get_var_bool("DEBUG_INC_SEC_QUAD",false);
   err_adv(debug, "Panel::inc_sec_quad");

   if (!_bcurves.contains(bc1) || !_bcurves.contains(bc2))
      return false;
   if (scribble && (bc1->num_edges()==1 || bc2->num_edges()==1))
      return false;
   int loc1 = _bcurves.get_index(bc1);
   int loc2 = _bcurves.get_index(bc2);
   if (loc2==((loc1+1)%4) || loc2==((loc1-1)%4))
      return false;

   // XXX - can add code here to restrict the stroke to be perpendicular to bc1
   //     - I choose to let the condition be more relax

   ARRAY<int> ns;
   for (int i = 0; i < 4; i++)
      if (_bcurves[i]!=bc1 && _bcurves[i]!=bc2)
         ns += _bcurves[i]->num_edges();
      else
         ns += _bcurves[i]->num_edges()+(scribble ? -1 : 1);
   PNL_RETESS_CMDptr cmd = new PNL_RETESS_CMD(this, _bcurves, ns);
   WORLD::add_command(cmd);
   return true;
}

bool
Panel::oversketch(CPIXEL_list& sketch_pixels)
{
   static bool debug = ::debug || Config::get_var_bool("DEBUG_OVERSKETCH",false);
   err_adv(debug, "Panel::oversketch");

   // First, check parameters 

   if (sketch_pixels.num() < 2) {
      cerr << "Panel::oversketch(): too few sketch pixels" << endl;
      return false;
   }

   // Attempt to slice the sketch pixels into the current screen path of the skeleton
   PIXEL_list skel_pixels = (PIXEL_list)(skel_pts());

   // XXX - in practice, we could loosen this condition to 
   // allow oversketch of a single point
   if (skel_pixels.num()<2) {
      cerr << "Panel::oversketch(): insufficient number of skel pixel points"
           << endl;
      return false;
   }

   double DIST_THRESH = 15.0;
   PIXEL_list new_skel;

   // use the static splice method in Bcurve
   // XXX - should extract this static method as a global utility function
   if (!Bcurve::splice_curves(sketch_pixels, skel_pixels,
                             DIST_THRESH, new_skel)) {
      err_adv(debug, "Panel::oversketch: can't splice in oversketch stroke");
      return false;
   }


   Wplane P = bfaces()[0]->plane(); // XXX- this assumes that the panel is in 2D
   Wpt_list pts;
   new_skel.project_to_plane(P, pts);
   pts.resample(_cells.num()-1); // XXX- uniform sampling
   
   // Now reshape the skel to the new desired pixel path.
   ARRAY<Panel*> visited;
   map<Bvert*, Wpt> map_locs;
   MULTI_CMDptr cmd = new MULTI_CMD();
   if(apply_skel(pts, visited, map_locs, cmd, Wvec::X(), Wvec::X())) {
      WORLD::add_command(cmd);
      return true;
   }
   delete cmd;
   return false;
}

// returns a Bvert_list that only contain verts controlled
// by Bcurves
inline Bvert_list
trim(Bvert_list in)
{
   Bvert_list out;
   for (int i = 0; i < in.num(); i++)
      if (Bcurve::find_controller(in[i]))
         out += in[i];
   return out;
}

// when mode is false, we are using pts in the list;
// when mode is true, we are using midpoints of segments;
// i is index of the input list
inline Wpt
get_o(CWpt_list& pts, int i, bool mode)
{
   if (i<0 || i>=pts.num()) return Wpt();
   if (mode) {
      if (i==0) return pts[0];
      return (pts[i]+pts[i-1])/2;
   }
   return pts[i];
}

// when mode is false, we are computing tangent with regard to pts
// in the list; when mode is true, we are computing tangent with
// regard to midpoints of segments.
// i is index of the input list
inline Wvec
get_t(CWpt_list& pts, int i, bool mode)
{
   if (i<0 || i>=pts.num()) return Wvec();
   if (i==0) {
      if (pts.num()==1)
         return Wvec::X();
      return pts[1]-pts[0];
   }
   if (mode)
      return pts[i]-pts[i-1];
   if (i == pts.num()-1)
      return pts[i]-pts[i-1];
   return pts[i+1]-pts[i-1];
}

// transform pts in the old coordinate system to pts in the
// new coordinate system
inline Wpt_list
transform(CWpt_list& pts, Wpt o, Wpt new_o, Wvec t, Wvec new_t, Wvec n)
{
   Wvec b = cross(n, t).normalized();
   Wvec new_b = cross(n, new_t).normalized();
   Wtransf xf = Wtransf(o, t, b, n);
   Wtransf new_xf = Wtransf(new_o, new_t, new_b, n);
   Wpt_list ret;
   for (int i = 0; i < pts.num(); i++)
      ret += new_xf * (xf.inverse() * pts[i]);
   return ret;
}

Wpt_list
Panel::skel_pts()
{
   Wpt_list ret;
   for (int i = 0; i < _cells.num(); i++)
      ret += _cells[i]->center();
   return ret;
}

bool
Panel::apply_skel(CWpt_list& new_skel, ARRAY<Panel*>& visited, 
                  map<Bvert*, Wpt>& map_locs, MULTI_CMDptr cmd, 
                  Wvec default_old_t, Wvec default_new_t)
{
   static bool debug = ::debug || Config::get_var_bool("DEBUG_APPLY_SKEL",false);
   err_adv(debug, "Panel::apply_skel");

   // get old skeleton
   Wpt_list old_skel = skel_pts();

   Bvert_list shared_verts_up, shared_verts_down, others;

   // coordinate transform variables
   Wvec t, new_t, n = bfaces()[0]->plane().normal();
   Wpt o, new_o; 
   Wpt_list pts;
   
   // find the new positions for boundary vertices
   for (int i = 0; i < _cells.num(); i++) {
      // identify different kinds of verts in a cell
      shared_verts_up = (i==_cells.num()-1) ? Bvert_list() :
          _cells[i]->shared_boundary(_cells[i+1]).get_verts();
      shared_verts_down = (i==0) ? Bvert_list() :
          _cells[i]->shared_boundary(_cells[i-1]).get_verts();
      others = _cells[i]->boundary_edges().get_verts();
      others -= shared_verts_up;
      others -= shared_verts_down;
      shared_verts_down = trim(shared_verts_down);
      
      // find the new positions for shared verts
      o = get_o(old_skel, i, true);
      new_o = get_o(new_skel, i, true);
      t = get_t(old_skel, i, true); 
      new_t = get_t(new_skel, i, true);
      pts = transform(shared_verts_down.pts(), o, new_o, t, new_t, n);
      for (int j = 0; j < shared_verts_down.num(); j++) {
         if (map_locs.find(shared_verts_down[j]) == map_locs.end())
            map_locs[shared_verts_down[j]] = pts[j];
      }

      // find the new positions for verts that are not shared by cells
      o = get_o(old_skel, i, false);
      new_o = get_o(new_skel, i, false);
      t = (old_skel.num()==1) ? default_old_t : get_t(old_skel, i, false);
      new_t = (new_skel.num()==1) ? default_new_t : get_t(new_skel, i, false);
      pts = transform(others.pts(), o, new_o, t, new_t, n);
      for (int j = 0; j < others.num(); j++) {
         if (map_locs.find(others[j]) == map_locs.end())
            map_locs[others[j]] = pts[j];
      }
   }

   if (debug) cerr << "   stored vertices number: " << map_locs.size() << endl;

   // apply the new positions to boundary curves
   for (int i = 0; i < _bcurves.num(); i++) {
      Bvert_list verts = _bcurves[i]->verts();
      Wpt_list pts;
      for (int j = 0; j < verts.num(); j++) {
         assert(map_locs.find(verts[j]) != map_locs.end());
         pts += map_locs[verts[j]];
      }

      // use a finer level sampling of the Wpt_list
      pts = refine_polyline_interp(pts, _bcurves[i]->res_level());

      // use WPT_LIST_RESHAPE_CMD to reshape
      pts.update_length();
      Wpt_listMap* m = Wpt_listMap::upcast(_bcurves[i]->map());
      WPT_LIST_RESHAPE_CMDptr  w_cmd= new WPT_LIST_RESHAPE_CMD(m,pts);
      cmd->add(w_cmd);
   }

   // mark this panel as visited
   visited += this;

   // recursively call the apply_skel method of neighboring panels
   for (int i = 0; i < _bcurves.num(); i++) {      
      Panel* nbr = nbr_panel(_bcurves[i]);
      if (nbr && !visited.contains(nbr)) {
         Bvert* first = _bcurves[i]->verts().first();
         Bvert* last = _bcurves[i]->verts().last();
         o =  (first->loc() + last->loc()) / 2;
         new_o = (map_locs[last] + map_locs[first]) / 2;
         t = last->loc() - first->loc();
         new_t = map_locs[last] - map_locs[first];
         pts = transform(nbr->skel_pts(), o, new_o, t, new_t, n);         
         nbr->apply_skel(pts, visited, map_locs, cmd, t, new_t);
      }
   }

   return true;
}

bool
Panel::re_tess(PIXEL_list pts, bool scribble) // pass by copy
{
   static bool debug = ::debug || Config::get_var_bool("DEBUG_RE_TESS",false);
   err_adv(debug, "Panel::re_tess");

   // preliminary test to eliminate some of the incorrect strokes
   // before calling specific re-tessellation methods
   Bpoint* bp1 = Bpoint::hit_ctrl_point(pts.first(), 8);
   Bpoint* bp2 = Bpoint::hit_ctrl_point(pts.last(), 8);
   Bcurve* bc1 = Bcurve::hit_ctrl_curve(pts.first(), 8);
   Bcurve* bc2 = Bcurve::hit_ctrl_curve(pts.last(), 8);

   // is it trying to retess a disk?
   bool disk = false;
   if ((!bp1 && !bp2) && ((bc1&&!bc2) || (!bc1&&bc2))) {
      if ((bc1 && hit_ctrl_surface(pts.last())==this) ||
         (bc2 && hit_ctrl_surface(pts.first())==this))
         disk = true;
   }

   if (!bp1) {
      swap(bp1, bp2);
      swap(bc1, bc2);
      pts.reverse();
   }
   if (bp1 && !bp2) { // a point and a curve
      if (!bc2) return false;
      if (curves().num() == 3)
         return split_tri(bp1, bc2, pts, scribble);
      else;
         // XXX - to fill in

   } else if (!bp1 && !bp2){ // 2 curves or a disk
      if ((!bc1 || !bc2) && !disk) return false;  
      if (curves().num() == 3)
         return inc_sec_tri(bc1, bc2, pts, scribble);
      else if (curves().num() == 4)
         return inc_sec_quad(bc1, bc2, pts, scribble);
      else
         return inc_sec_disk(bc1, bc2, pts, scribble);
   } else { // 2 points
      // XXX - to fill in
   }

   return false;
}

bool
Panel::re_tess(CBcurve_list& cs, CARRAY<int>& ns)
{
   if (cs.num() != ns.num())
      return false;

   bool result = false;
   if (cs.num() == 3)
      result = tessellate_tri(cs, ns);
   else if (cs.num() == 4)
      result = tessellate_quad(cs, ns);
   if (result)
      return true;
   return tessellate_disk(cs, ns);
}

Panel*
Panel::nbr_panel(Bcurve* c)
{
   if (!_bcurves.contains(c))
      return NULL;
   Bnode_list out = c->outputs();
   if (out.num() < 2)
      return NULL;
   
   return (out[0]==this) ? Panel::upcast(out[1]) : Panel::upcast(out[0]);
}

bool
Panel::try_re_tess_adj(Bcurve* bc, int n)
{
   if (!_bcurves.contains(bc) || n<=0)
      return false;
   Bnode_list out = bc->outputs();
   assert(out.num() < 2);
   if (out.num() != 1)
      return false;
   Panel* adj = Panel::upcast(out[0]);
   if (!adj)
      return false;

   Bcurve_list cs = adj->curves();
   ARRAY<int> ns;
   for (int i = 0; i < cs.num(); i++) {
      if (cs[i] == bc)
         ns += n;
      else
         ns += cs[i]->num_edges();
   }

   // special treatment for quad panels
   if (cs.num() == 4) {
      int loc = cs.get_index(bc);
      loc = (loc+2)%4;
      ns[loc] = n;
   }

   return adj->re_tess(cs, ns);
}

bool
Panel::tessellate_tri(Bcurve_list c, ARRAY<int> ns)
{
   if(ns.num()!=0 && ns.num()!=3)
      return false;
   for (int i = 0; i < ns.num(); i++)
      if (ns[i]<=0) return false;
   bool create_mode = ns.empty();
   if (!create_mode && !ns.contains(1))
      return false;

   if (c.num() != 3) {
      return false;
   }

   static bool debug = ::debug ||
      Config::get_var_bool("DEBUG_TRI_PANEL",false);
   err_adv(debug, "Panel::tessellate_tri");

   // find length of shortest curve (will be covered by 1 edge):
   double s = min(min(c[0]->length(),c[1]->length()),c[2]->length());
   assert(s > 0);

   if (create_mode) {
      // cycle the curves until the first one is the shortest
      if (s < c[0]->length())
         c.shift(1);
      if (s < c[0]->length())
         c.shift(1);
      assert(s == c[0]->length()); // first must be shortest now
   } else {
      // cycle the curves until ns[0] is 1
      if (ns[0] != 1) {
         c.shift(1);
         ns.shift(1);
      }
      if (ns[0] != 1) {
         c.shift(1);
         ns.shift(1);
      }
      assert(ns[0] == 1);
   }

   //                    /|                        
   //                  /  |                         
   //                /    |                         
   //         c[2] /      | c[1]                    
   //            /        |                         
   //          /          |                         
   //        /            |                         
   //        ------------->                         
   //             c[0]
   //
   // Ensure the above layout: when c0 is viewed running left to right, 
   // then c1 attaches on the right and c2 attaches on the left.
   // and the outward surface normals point to us.
   if (!c[1]->contains(c[0]->b2())) {
      err_adv(debug, "  swapping c1 and c2...");
      swap(c[1],c[2]);
      if (!create_mode)
         swap(ns[1], ns[2]);
   }
   if (!(c[1]->contains(c[0]->b2()) && c[2]->contains(c[0]->b1()))) {
      if (debug) {
         err_msg("  problem with orientation:");
         cerr << "    c0: b1 and b2: " << c[0]->b1() << ", " << c[0]->b2() << endl;
         cerr << "    c1: b1 and b2: " << c[1]->b1() << ", " << c[1]->b2() << endl;
         cerr << "    c2: b1 and b2: " << c[2]->b1() << ", " << c[2]->b2() << endl;
      }
      return false;
   }

   if (!prepare_tessellation(c)) {
      err_adv(debug, "  prepare_tessellation failed");
      return false;
   }

   // we need to resample the curves before proceeding.
   //
   // in the end we want the number of edges for (c0,c1,c2) to fit one
   // of the patterns: (1,n,n), (1,1,2), (1,2,1).

   // number of edges for each:
   int n0, n1, n2;
   if (create_mode) {
      n0 = 1;
      n1 = int(round(c[1]->length()/s));
      n2 = int(round(c[2]->length()/s));
   } else {
      n0 = ns[0];
      n1 = ns[1];
      n2 = ns[2];
   }

   if (!resample(c[0],n0)) {
      if (create_mode || !try_re_tess_adj(c[0], n0)) {
         err_adv(debug, "  can't resample c0");
         return false;
      }
   }

   // do corrections on n1, n2
   if (create_mode) {
      if (n1 == n2) {
         // no problem
      } else if (n1 == 1) {
         if (can_resample(c[2],2))
            n2 = 2;
         else if (can_resample(c[2],1))
            n2 = 1;
         else
            return false;
      } else if (n2 == 1) {
         if (can_resample(c[1],2))
            n1 = 2;
         else if (can_resample(c[1],1))
            n1 = 1;
         else
            return false;
      } else {
         // force them to agree
         int n = int(ceil((n1 + n2)/2.0));
         if (can_resample(c[1],n) && can_resample(c[2],n)) {
            n1 = n2 = n;
         } else if (can_resample(c[1],c[2]->num_edges())) {
            n1 = n2 = c[2]->num_edges();
         } else if (can_resample(c[2],c[1]->num_edges())) {
            n1 = n2 = c[1]->num_edges();
         } else {
            err_adv(debug, "  can't resample c1 and c2");
            return false;
         }
      }
   }
   if (!resample(c[1],n1)) {
      if (create_mode || !try_re_tess_adj(c[1], n1)) {
         err_adv(debug, "  can't resample c1 to %d edges",n1);
         return false;
      }
   }
   if (!resample(c[2],n2)) {
      if (create_mode || !try_re_tess_adj(c[2], n2)) {
         err_adv(debug, "  can't resample c2 to %d edges",n2);
         return false;
      }
   }
   assert(c[0]->num_edges() == n0 &&
          c[1]->num_edges() == n1 &&
          c[2]->num_edges() == n2);

   err_adv(debug, "  processing %d,%d,%d triangular region",
           n0, n1, n2);

   // Now extract vertices in 3 lists:
   ARRAY<Bvert_list> v(3);
   if (!c.extract_boundary_ccw(v)) {
      err_adv(debug, "  can't extract boundary w/ CCW orientation");
      return false;
   }
   assert(v.num() == 3);

   // Now reverse direction of c2 so
   // c2 and c1 both run bottom to top:
   //                                            
   //                    /|                        
   //                  /  |                         
   //                /    |                         
   //         c[2] /      | c[1]                    
   //            /        |                         
   //          /          |                         
   //        /            |                         
   //        ------ > -----                         
   //             c[0]
   v[2].reverse();

   // create memes at vertices:
   // (all vertices are on the boundary):
   add_vert_memes(v[0] + v[1] + v[2]);

   // create faces:
   // the only valid cases (as of now) are:
   //   1,1,1
   //   1,n,n  (n > 1)
   //   1,1,2
   //   1,2,1
   n0 = v[0].num() - 1;
   n1 = v[1].num() - 1;
   n2 = v[2].num() - 1;
   if (n1 == n2) {
      // use quads all the way up until the last one, a triangle:
      for (int k=0; k<n2-1; k++) {
         // quads
         add_cell(add_quad(v[2][k],v[1][k],v[1][k+1],v[2][k+1])); 
      }
      // triangle
      add_cell(add_face(v[2][n2-1],v[1][n2-1],v[1].last()));
   } else if (n1 == 1) {
      //                    /|                        
      //                  /  |                         
      //                /    |                         
      //         c[2] /   t2 | c[1]                    
      //            /  \     |                         
      //          /      \   |                         
      //        /    t1    \ |                         
      //        --------------                         
      //             c[0]
      if(n2 != 2) return false;
      add_face(v[0][0],v[0][1],v[2][1]); // t1
      add_face(v[1][0],v[1][1],v[2][1]); // t2
      add_cell(bfaces());
   } else if (n2 == 1) {
      /*
      //       
      //       |\                                     
      //       |  \                                    
      //       |    \   c[1]                           
      //  c[2] | t2   \                                
      //       |    /   \                              
      //       |  /       \                            
      //       |/    t1     \                          
      //       ---------------                         
      //            c[0]
      */
      if(n1 != 2) return false;
      add_face(v[1][0],v[1][1],v[2][0]); // t1
      add_face(v[1][1],v[1][2],v[2][0]); // t2
      add_cell(bfaces());
   } else {
      assert(0);
   }

   // Add face and edge memes:
   add_face_memes(_patch->faces());

   invalidate(); // memes are hot, sign up for updates

   // XXX - create skeleton for 1-n-n case?

   err_adv(debug, "Panel::tessellate_tri: done");

   return finish_tessellation();
}

bool
Panel::choose_quad_resolution(
   Bcurve* h1,  // horizontal curve 1
   Bcurve* h2,  // horizontal curve 2
   Bcurve* v1,  // vertical curve 1
   Bcurve* v2   // vertical curve 2
   )
{
   // Internal helper method for tessellating a quadrilateral
   // region. Chooses the number of edges in each dimension,
   // aiming for good aspect ratios. Assumes the curves have been
   // screened so the average "horizontal" length is more than
   // the average "vertical" length:
   //
   //              h2                              
   //        ----------------                       
   //      |                 |                      
   //      |                 |                      
   //   v1 |                 | v2                   
   //      |                 |                      
   //      |                 |                      
   //       -----------------                       
   //              h1  

   assert(h1 && h2 && v1 && v2);

   // First ensure v1 and v2 use the same numbe of edges.
   // (If there is a choice, try for 1 edge):
   if (!ensure_same_sampling(v1, v2, 1))
      return false;

   // Average edge length of vertical curves:
   double v_edge_len = (v1->avg_edge_length() + v2->avg_edge_length())/2;

   // Average total length of horizontal curves:
   double h_total_len = (h1->length() + h2->length())/2;
   int preferred_h_num = (int)round(h_total_len / v_edge_len);
   return ensure_same_sampling(h1, h2, preferred_h_num);
}

bool
Panel::tessellate_quad(CBcurve_list& c, CARRAY<int> ns)
{
   if(ns.num()!=0 && ns.num()!=4)
      return false;
   for (int i = 0; i < ns.num(); i++)
      if (ns[i]<=0) return false;
   bool debug = Config::get_var_bool("DEBUG_QUAD_PANEL",false);
   err_adv(debug, "Panel::tessellate_quad: starting...");

   if (c.num() != 4) {
      err_adv(debug, "  rejecting non-quad");
      return false;
   }
   if (!prepare_tessellation(c)) {
      err_adv(debug, "  prepare_tessellation failed");
      return false;
   }

   //             c[2]                             
   //        ----------------                       
   //      |                 |                      
   //      |                 |                      
   // c[3] |                 | c[1]                 
   //      |                 |                      
   //      |                 |                      
   //       -----------------                       
   //             c[0]
   //
   // The 4 curves are ordered CCW around the region

   if (ns.num() == 0)   {
      // Resample curves to get regular grid with good aspect ratios:
      bool ok=false;
      if ((c[0]->length() + c[2]->length()) > (c[1]->length() + c[3]->length()))
         ok = choose_quad_resolution(c[0], c[2], c[1], c[3]);
      else
         ok = choose_quad_resolution(c[1], c[3], c[0], c[2]);
      if (!ok) {
         err_adv(debug, "  can't get uniform grid");
         return false;
      }
   } else {
      for (int i = 0; i < 4; i++)
         if (!resample(c[i],ns[i]) && !try_re_tess_adj(c[i], ns[i])) {
            err_adv(debug, "  can't resample c[%d] to %d edges", i, ns[i]);
            return false;
         }
   }

   err_adv(debug, "quad dimensions: %d x %d",
           c[0]->num_edges(), c[1]->num_edges());
   
   // Now extract vertices in 4 lists:
   ARRAY<Bvert_list> v(4);
   if (!c.extract_boundary_ccw(v)) {
      err_adv(debug, "  can't extract CCW boundary");
      return false;
   }
   assert(v.num() == 4);

   // Now we have to reverse the direction of the top and
   // left vertex lists, so they look like the following,
   // instead of running around CCW:
   //                                              
   //             v[2]                             
   //        --------------->                       
   //      ^                 ^                      
   //      |                 |                      
   // v[3] |                 | v[1]                 
   //      |                 |                      
   //      |                 |                      
   //       ---------------->                       
   //             v[0]                             
   //                                            
   v[2].reverse();
   v[3].reverse();

   // Try to build the grid. It checks further conditions
   // like same numbers of verts on top and bottom and on
   // left and right, and if okay it builds the grid and
   // generates internal vertices:
   BvertGrid grid;
   if (c[0]->num_edges() <= c[1]->num_edges()) {
      if (!grid.build(v[0], v[2], v[3], v[1])) {
         err_adv(debug, "  can't build grid");
         return 0;
      }
   } else {
      v[1].reverse();
      v[3].reverse();
      if (!grid.build(v[3], v[1], v[2], v[0])) {
         err_adv(debug, "  can't build grid");
         return 0;
      }
   }

   // We still have to set interior vertex locations.
   // Use Coons patch definition (see bvert_grid.C):
   if (!grid.interp_boundary()) {
      err_adv(debug, "  can't interp boundary");
      return 0;
   }

   // Generate quads inside the grid and add them to the patch:
   if (!grid.add_quads(_patch)) {
      err_adv(debug, "  can't add quads");
      return 0;
   }

   // create cells
   if (grid.ncols() >= grid.nrows()) {
      //if (grid.nrows() > 3) {
      //   add_cell(bfaces()); // one big pile
      //} else {
         // one or two horizontal bands...
         // add a cell for each column
         for (int i=0; i<grid.ncols()-1; i++)
            add_cell(grid.vband(i));
      //}
   } else {
      //if (grid.ncols() > 3) {
      //   add_cell(bfaces()); // one big pile
      //} else {
         // one or two vertical bands...
         // add a cell for each row
         for (int j=0; j<grid.nrows()-1; j++)
            add_cell(grid.hband(j));
      //}
   }

   // Create blending memes, including inactive ones along
   // the boundary:
   for (int j=0; j<grid.nrows(); j++)
      add_vert_memes(grid.row(j));

   invalidate(); // memes are hot, sign up for updates

   // Add face and edge memes:
   add_face_memes(_patch->faces());

   // XXX -
   //  When the "grid" is just a chain of quads, generate a skeleton?

   err_adv(debug, "  done");

   return finish_tessellation();
}

bool
Panel::tessellate_multi(CARRAY<Bcurve_list>& contours)
{
   // Curves in a single contour are assumed to be in one mesh already

   // Uses GLU tessellator routines to triangulate a set of
   // boundary vertices. Does not introduce internal vertices.

   if (debug_tess)
      err_msg("Panel::tessellate_multi: using %d contours", contours.num());

   if (contours.empty())
      return false;

   if (!prepare_tessellation(contours.first()))
      return false;

   // need this to pass to Bsurface::tessellate():
   ARRAY<Bvert_list> contour_vert_lists;

   for(int i=0; i<contours.num(); i++) {
      Bcurve_list& contour = contours[i];

      if (debug_tess)
         err_msg("Panel::tessellate_multi: %d curves in contour %d",
                 contour.num(), i);

      if (!contour.forms_closed_chain())
         return false;

      if (i > 0) {
         // first pass was dealt with in prepare_tessellation()
         // ensure it's all one mesh:
         //
         set_mesh(LMESH::merge(_mesh, contour.mesh()));

         // stick these curves in the big list:
         absorb(contour);
      }

      contour_vert_lists += contour.boundary_verts();
   }

   // Reset mesh to subdivision level 0 (control mesh):
   _mesh->update_subdivision(0);

   // Now do it:
   tessellate(contour_vert_lists);

   return finish_tessellation();
}

void
Panel::produce_child()
{
   bool debug = ::debug || Config::get_var_bool("DEBUG_PANEL",false);
   err_adv(debug, "Panel::produce_child: level %d, res level %d",
           _mesh->subdiv_level(), _res_level);

   if (_child) {
      err_adv(debug, "  child exists");
      return;
   }
   if (!_mesh->subdiv_mesh()) {
      err_msg("Panel::produce_child: Error: no subdiv mesh");
      return;
   }

   // It hooks itself up and takes care of everything...
   // even setting the child pointer is not necessary here.
   _child = new Panel(this);
}

VertMeme*
Panel::add_vert_meme(Lvert* v)
{
   // Register our face meme on the given face.

   // Screen out the wackos
   if (!v)
      return 0;

   // Don't create a duplicate meme:
   if (find_vert_meme(v))
      return find_vert_meme(v);

   return new BlendingMeme(this, v); 
}

void
Panel::add_vert_memes(CBvert_list& verts)
{
   if (!(_mesh && _mesh == verts.mesh())) {
      err_msg("Panel::add_vert_memes: Error: bad mesh");
      return;
   }

   for (int i=0; i<verts.num(); i++)
      add_vert_meme((Lvert*)verts[i]);
}

/*****************************************************************
 * PNL_RETESS_CMD
 *****************************************************************/
PNL_RETESS_CMD::PNL_RETESS_CMD(Panel* p, CBcurve_list& cs, CARRAY<int>& ns)
{
   if (cs.num() != ns.num() || p->curves().num() != ns.num()) {
      err_msg("PNL_RETESS_CMD::PNL_RETESS_CMD: error: lists not equal");
      return;
   }
   for (int i = 0; i < ns.num(); i++) {
      if (ns[i] <= 0) {
         err_msg("PNL_RETESS_CMD:PNL_RETESS_CMD: error: list contains non-positive number");
         return;
      }
      if (!cs.contains(p->curves()[i])) {
         err_msg("PNL_RETESS_CMD:PNL_RETESS_CMD: error: lists not the same");
         return;
      }
   }

   _p    = p;
   _old_cs = p->curves(); 
   _new_cs = cs;
   _new_ns = ns;
   _old_ns.clear();
   for (int i = 0; i < ns.num(); i++)
      _old_ns += p->curves()[i]->num_edges();
}

bool
PNL_RETESS_CMD::doit()
{
   if (is_done())
      return true;

   _p->re_tess(_new_cs, _new_ns);

   return COMMAND::doit();
}

bool
PNL_RETESS_CMD::undoit()
{
   if (!is_done())
      return true;

   _p->re_tess(_old_cs, _old_ns);

   return COMMAND::undoit();
}

/* end of file panel.C */
