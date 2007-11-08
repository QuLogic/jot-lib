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
#include "disp/colors.H"                // Color::blue_pencil_d
#include "gtex/util.H"                  // for draw_strip
#include "gtex/ref_image.H"             // for VisRefImage
#include "gtex/basic_texture.H"         // GLStripCB
#include "tess/panel.H"                 // Panel
#include "tess/vert_mapper.H"
#include "tess/tex_body.H"
#include "tess/tess_cmd.H"
#include "tess/mesh_op.H"               // FIT_VERTS_CMD
#include "tess/ti.H"                    // create_disk_map

#include "paper_doll.H"

static bool debug = Config::get_var_bool("DEBUG_PAPER_DOLL",false);

using mlib::Wplane;
using mlib::Wvec;
using mlib::Wpt;
using mlib::Wpt_list;
using namespace GtexUtil;
using namespace tess;

/*****************************************************************
 * UTILITIES
 *****************************************************************/
template <class T>
inline bool
are_all_bsurfaces(const Bsurface_list& surfs)
{
   for (int i=0; i<surfs.num(); i++)
      if (!T::isa(surfs[i]))
         return false;
   return true;
}

inline Bvert_list
copy_verts(CBvert_list& verts, LMESHptr mesh)
{
   Bvert_list ret(verts.num());
   for (int i=0; i<verts.num(); i++) {
      ret += mesh->add_vertex(verts[i]->loc());
   }
   return ret;
}

inline Bface*
copy_face(Bface* f, CVertMapper& vmap, Primitive* p, bool reverse)
{
   assert(f && p && vmap.is_valid());

   Bvert* v1 = vmap.a_to_b(f->v1());
   Bvert* v2 = vmap.a_to_b(f->v2());
   Bvert* v3 = vmap.a_to_b(f->v3());
   assert(v1 && v2 && v3);

   FaceMeme* fm = 0;
   UVpt a, b, c;
   if (UVdata::get_uvs(f, a, b, c)) {
      Panel* r = Panel::upcast(Bsurface::find_controller(f));
      assert(r && r->curves().num()==4);
      int n = min(r->curves()[0]->num_edges(), r->curves()[1]->num_edges());
      if (reverse) {
         fm = p->add_face(v1, v3, v2, a, c, b);
      } else {
         a = UVpt(2+1.0/n-a[0], a[1]);
         b = UVpt(2+1.0/n-b[0], b[1]);
         c = UVpt(2+1.0/n-c[0], c[1]);
         fm = p->add_face(v1, v2, v3, a, b, c);
      }
   } else {
      if (reverse)
         fm = p->add_face(v1, v3, v2);
      else
         fm = p->add_face(v1, v2, v3);
   }
   return fm ? fm->face() : 0;
   
}

inline bool 
copy_edge(Bedge* a, CVertMapper& vmap)
{
   // copy edge attributes, e.g. from skel to skin

   Bedge* b = vmap.a_to_b(a);
   if (!(a && b))
      return false;

   if (a->is_weak())
      b->set_bit(Bedge::WEAK_BIT);

   // more?

   return true;
}

inline bool 
copy_edges(CBedge_list& edges, CVertMapper& vmap)
{
   bool ret = true;
   for (int i=0; i<edges.num(); i++)
      if (!copy_edge(edges[i], vmap))
         ret = false;
   return ret;
}

inline Bface_list
copy_faces(CBface_list& faces, CVertMapper& vmap, Primitive* p, bool reverse=false)
{
   assert(p && vmap.is_valid());
   Bface_list ret;

   for (int i=0; i<faces.num(); i++) {
      ret += copy_face(faces[i], vmap, p, reverse);
   }
   copy_edges(faces.get_edges(), vmap);
   return ret;
}

inline LMESHptr
get_inflate_mesh(LMESHptr skel_mesh)
{
   if (!skel_mesh)
      return 0;
   TEXBODY* tex = TEXBODY::upcast(skel_mesh->geom());
   if (!tex)
      return 0;
   return LMESH::upcast(tex->get_inflate_mesh(skel_mesh));
}

inline bool
create_sides(
   CBface_list& faces,
   CVertMapper& tmap,
   CVertMapper& bmap,
   Primitive* p
   )
{
   assert(p && tmap.is_valid() && bmap.is_valid());
   Bface_list ret;

   EdgeStrip boundary = faces.get_boundary();
   assert(!boundary.empty());

   EdgeStrip top_strip = tmap.a_to_b(boundary);
   EdgeStrip bot_strip = bmap.a_to_b(boundary);
   assert(!(top_strip.empty() || bot_strip.empty()));

   UVpt a, b, c, d;
   Bedge *top_e, *bot_e;
   Bface *top_f, *bot_f;
   Bvert *v1, *v2, *v3, *v4;
   for (int i = 0; i < top_strip.num(); i++) {
      top_e = top_strip.edge(i);
      bot_e = bot_strip.edge(i);
      top_f = top_e->get_face();
      bot_f = bot_e->get_face();
      v1 = top_strip.vert(i);
      v2 = top_e->other_vertex(v1);
      v3 = bot_strip.vert(i);
      v4 = bot_e->other_vertex(v3);
      bool has_uv = UVdata::get_uvs(top_f, a, b, c);
      if (has_uv) {
         assert(top_f->is_quad() && bot_f->is_quad());
         a = UVdata::get_uv(v1, top_f);
         b = UVdata::get_uv(v2, top_f);
         c = UVdata::get_uv(v3, bot_f);
         d = UVdata::get_uv(v4, bot_f);
         if (c[0]==0 && d[0]==0) 
            c[0] = d[0] = a[0]+abs((UVdata::get_uv(top_f->other_vertex(v1, v2), top_f))[0]-a[0]);
         if (top_f->other_vertex(top_f->weak_edge()) == v1)
            p->add_quad(v1, v3, v4, v2, a, c, d, b);
         else {
            assert(top_f->other_vertex(top_f->weak_edge()) == v2);
            p->add_quad(v3, v4, v2, v1, c, d, b, a);
         }
      } else {
         p->add_quad(v1, v3, v4, v2);
      }
   }

   return true;
}

inline Bedge_list
quad_cell_end_edges(PCell* cell)
{
   // if the cell is a quad (4 sides) and has one neigbhor,
   // return the side opposite from the neighbor.

   assert(cell && cell->num_corners() == 4);

   PCell_list nbrs = cell->nbrs();
   if (nbrs.num() != 1) {
      err_adv(debug, "quad_cell_end_edges: neighbors: %d != 1", nbrs.num());
      return Bedge_list();
   }
   // find an edge of the shared boundary.
   // do it now before messing with flags...
   assert(!cell->shared_boundary(nbrs[0]).empty());
   Bedge* e = cell->shared_boundary(nbrs[0]).first();
   assert(e);

   EdgeStrip boundary = cell->get_boundary();
   assert(boundary.num_line_strips() == 1);

   // iterate around the boundary, setting edge flags to value
   // k that is incremented whenever we pass a cell corner.
   int k = 0;
   PCellCornerVertFilter filter;
   for (int i=0; i<boundary.num(); i++) {
      if (filter.accept(boundary.vert(i)))
         k = (k + 1)%4;
      boundary.edge(i)->set_flag(k);
   }

   // we want the edges with flag == k + 2 mod 4
   return boundary.edges().filter(
      SimplexFlagFilter((e->flag() + 2)%4)
      );
}

inline Bvert_list
reorder(CBedge_list& edges)
{
   Bvert_list ret;

   Bvert_list verts = edges.get_verts();
   for (int i = 0; i < verts.num(); i++)
      if (Bpoint::find_controller(verts[i]) || Bcurve::find_controller(verts[i])) {
         ret += verts[i];
         break;
      }
   if (ret.empty())
         return ret;

   for (int i = 1; i < verts.num(); i++) {
      for (int j = 0; j < edges.num(); j++) {
         Bvert* v = edges[j]->other_vertex(ret.last());
         if (v && !ret.contains(v)) {
            ret += v;
            break;
         }
      }
   }

   err_adv(debug, "   reorder: num of verts: %d", ret.num());
   return ret;
}

inline double
compute_h(Bvert* v)
{
   assert(v);
   PCell_list cells = Panel::find_cells(v->get_faces());

   if (cells.num() == 2) {
      // common case: vertex in a tube ring (2 neighboring cells)
      if (Bpoint::find_controller(v) || Bcurve::find_controller(v))
         return cells[0]->shared_boundary(cells[1]).avg_len()/2;

      Bvert_list verts = reorder(cells[0]->shared_boundary(cells[1]));
      double num = verts.num()-1;
      assert(num > 1);
      int index = verts.get_index(v);
      assert(index != -1);
      double h = (compute_h(verts.first()) + compute_h(verts.last())) / 2;
      return h * (1 + min(index/num, 1-index/num));

   } else if (cells.num() > 2) {
      // multiple adjacent cells
      return PCell::shared_edges(cells).avg_len()/2;
   } else if (cells.num() == 1) {
      // just one cell, e.g. tip of a tube, or part of a disk
      if (cells[0]->num_corners() == 3) {
         if (cells[0]->nbrs().num() != 1) {
            return cells[0]->boundary_edges().avg_len()/2;
         }
         assert(cells[0]->nbrs().num() == 1);
         return 0; // it's the tip of the triangle
      } else if (cells[0]->num_corners() == 4) {
         if (cells[0]->nbrs().num() == 0) {
            return cells[0]->boundary_edges().avg_len()/2;
         } else if (cells[0]->nbrs().num() == 1) {
            // or maybe should do same rule as next case
            Bedge_list end_edges = quad_cell_end_edges(cells[0]);
            err_adv(debug, "found %d end edges", end_edges.num());
            return end_edges.avg_len()/2;
         }
         return v->strong_edges().avg_len()/2;
      }
      if (Bpoint::find_controller(v) || Bcurve::find_controller(v))
         return cells[0]->boundary_edges().filter(BorderEdgeFilter()||BcurveFilter()).avg_len()/2;

      Bvert_list nbrs;
      v->get_nbrs(nbrs);
      assert(!nbrs.empty());
      return 1.5 * compute_h(nbrs[0]);
   } 

   err_adv(debug, "compute_h: unexpected number of cells: %d", cells.num());
   return -1;
}

static void
define_offset(
   Bvert* v,
   CVertMapper& tmap,
   CVertMapper& bmap,
   CWvec& n,
   Primitive* p
   )
{
   assert(v);
   double h = compute_h(v);

   // apply the offset to top and bottom verts:
   Bvert* tv = tmap.a_to_b(v);
   assert(tv);
   tv->offset_loc(n*h);
   Bvert* bv = bmap.a_to_b(v);
   assert(bv);
   bv->offset_loc(-n*h);

   // create xf memes
   Bface_list one_ring;
   v->get_all_faces(one_ring);
   DiskMap* frame = DiskMap::create(one_ring, false, v);
   p->create_xf_meme((Lvert*)tv, frame);
   p->create_xf_meme((Lvert*)bv, frame);
   p->add_input(frame);
}

inline void
define_offsets(
   CBvert_list& verts,
   CVertMapper& tmap,
   CVertMapper& bmap,
   CWvec& n,
   Primitive* p
   )
{
   assert(tmap.is_valid() && bmap.is_valid());

   for (int i=0; i<verts.num(); i++) {
      define_offset(verts[i], tmap, bmap, n, p);
   }
}

inline MULTI_CMDptr
hide_surfs_cmd(CBsurface_list& surfs)
{
   MULTI_CMDptr hide_all = new MULTI_CMD;
   for (int i=0; i<surfs.num(); i++)
      hide_all->add(new HIDE_BBASE_CMD(surfs[i],false));
   return hide_all;
}

inline void
delete_all(Bcurve_list curves)
{
   while (!curves.empty())
      delete curves.pop();
}

/*****************************************************************
 * PAPER_DOLL
 *****************************************************************/
PAPER_DOLLptr PAPER_DOLL::_instance;

PAPER_DOLL::PAPER_DOLL() :
   DrawWidget()
{
   _draw_start += DrawArc(new TapGuard,    drawCB(&PAPER_DOLL::cancel_cb));
   _draw_start += DrawArc(new StrokeGuard, drawCB(&PAPER_DOLL::stroke_cb));

   // Set up the clean up routine
   atexit(clean_on_exit);
}

void 
PAPER_DOLL::clean_on_exit() 
{ 
   _instance = 0; 
}

PAPER_DOLLptr
PAPER_DOLL::get_instance()
{
   if (!_instance)
      _instance = new PAPER_DOLL();
   return _instance;
}

bool 
PAPER_DOLL::init(CGESTUREptr& g)
{
   if (!(g && g->is_stroke()))
      return false;

   if (g->below_min_length() || g->below_min_spread())
      return false;

   if (!g->is_ellipse()) {
      err_adv(debug, "PAPER_DOLL::init: non-ellipse");
      return false;
   }
   Panel* p = Panel::upcast(
      Bsurface::get_surface(get_top_level(VisRefImage::get_faces(g->pts())))
      );
   err_adv(debug, "PAPER_DOLL::init: %s panel", p?"found":"could not find");
   if (!(p && p->is_selected())) {
      err_adv(debug, "PAPER_DOLL::init: ellipse not over selected panel");
      return false;
   }
   assert(p && p->bfaces().num() > 0);
   Bface_list faces = Bface_list::reachable_faces(p->bfaces().first());
   assert(!faces.empty());
   if (!faces.is_planar(deg2rad(1.0))) {
      err_adv(debug, "PAPER_DOLL::init: region is not planar");
      return false;
   }

   EdgeStrip boundary = faces.get_boundary();
   if (boundary.empty()) {
      err_adv(debug, "PAPER_DOLL::init: region has no boundary");
      return false;
   }

   Bsurface_list surfs = Bsurface::get_surfaces(faces);
   if (!are_all_bsurfaces<Panel>(surfs)) {
      err_adv(debug, "PAPER_DOLL::init: region not all panels");
      return 0;
   }

   err_adv(debug, "PAPER_DOLL::init: proceeding...");

   err_adv(debug, "  boundary edges: %d, components: %d, panels: %d",
           boundary.edges().num(), boundary.num_line_strips(), surfs.num()
      );

   if (get_instance()->build_primitive(faces)) {
      err_adv(debug, "  ... succeeded");
      return true;
   }
   err_adv(debug, "  ... failed");
   return false;
}

bool 
PAPER_DOLL::init(CBcurve_list& contour)
{
   if (contour.empty()) {
      err_adv(debug, "PAPER_DOLL::init: empty contour");
      return false;
   }
   LMESH* skel_mesh = contour.mesh();
   if (!skel_mesh) {
      err_adv(debug, "PAPER_DOLL::init: curves don't share a mesh");
      return false;
   }
   if (!contour.is_each_straight()) {
      err_adv(debug, "PAPER_DOLL::init: curves not straight");
      return false;
   }
   if (!contour.forms_closed_chain()) {
      err_adv(debug, "PAPER_DOLL::init: curves don't form a closed chain");
      return false;
   }
   if (!contour.is_planar()) {
      err_adv(debug, "PAPER_DOLL::init: curves not planar");
      return false;
   }
   Wpt_list pts = contour.get_chain().get_verts().pts();
   if (!(pts.num() == 4 || pts.num() == 5)) {
      err_adv(debug, "PAPER_DOLL::init: can't do %d-gon", pts.num());
      return false;
   }

   Wplane P;
   if (!pts.get_plane(P)) {
      err_adv(debug, "PAPER_DOLL::init: can't get plane from contour");
      return false;
   }
   assert(P.is_valid());
   Wpt  o = pts.average();
   Wvec n = P.normal();

   // make plane normal point toward camera
   if (VIEW::eye_vec(o) * n > 0)
      n = -n;

   // reverse order of points if needed so they go CCW
   // around plane normal:
   err_adv(debug, "contour winding number: %f", pts.winding_number(o, n));
   if (pts.winding_number(o, n) > 1) {
      pts.reverse();
   }

   // create the primitive
   MULTI_CMDptr cmd = new MULTI_CMD;
   Primitive* p = Primitive::init(skel_mesh, pts, P.normal(), cmd);
   if (!p) {
      err_adv(debug, "PAPER_DOLL::init: Primitive::init() failed");
      return false;
   }

   // hide the curves
   // take over drawing the primitive (?)
   //   or just augment
   // activate

   delete_all(contour);

   get_instance()->init(p);

   err_adv(debug, "PAPER_DOLL::init: curves okay");
   return true;
}

bool
PAPER_DOLL::init(Primitive* p)
{
   _prim = p;
   activate();
   return true;
}

Primitive*
PAPER_DOLL::build_primitive(CBface_list& o_faces)
{
   LMESHptr skel_mesh = LMESH::upcast(o_faces.mesh());
   assert(skel_mesh);
   LMESHptr mesh = get_inflate_mesh(skel_mesh);
   assert(mesh);

   // create vertices for top and bottom parts:
   Bvert_list o_verts = o_faces.get_verts();            // original verts
   Bvert_list t_verts = copy_verts(o_verts, mesh);      // top verts
   Bvert_list b_verts = copy_verts(o_verts, mesh);      // bottom verts

   // set up mappings:
   //   original --> top
   //   original --> bottom
   VertMapper t_map(o_verts, t_verts, true);
   VertMapper b_map(o_verts, b_verts, true);

   Primitive* ret = new Primitive(mesh, skel_mesh);

   // build dependencies
   Bsurface_list surfs = Bsurface::get_surfaces(o_faces);
   for (int i = 0; i < surfs.num(); i++)
      ret->absorb_skel(surfs[i]);

   // generate top faces and bottom faces
   Bface_list t_faces = copy_faces(o_faces, t_map, ret, false);
   Bface_list b_faces = copy_faces(o_faces, b_map, ret, true);

   // create the sides:
   create_sides(o_faces, t_map, b_map, ret);

   Wvec n = o_faces.avg_normal();
   if (1) {
      define_offsets(o_verts, t_map, b_map, n, ret);
   } else {
      // under construction...
      //  for now just offset the top and bottom uniformly for testing...
      const double k = Config::get_var_dbl("PAPER_DOLL_OFFSET_SCALE",1.0);
      double       h = o_faces.get_edges().strong_edges().avg_len();
      t_verts.transform(Wtransf::translation( 0.5*k*h*n));
      b_verts.transform(Wtransf::translation(-0.5*k*h*n));
   }


   // make it all undoable:
   MULTI_CMDptr cmd = new MULTI_CMD;

   // finish build
   ret->finish_build(cmd);

   //cmd->add(hide_surfs_cmd(Bsurface::get_surfaces(o_faces)));
   cmd->add(new SHOW_BBASE_CMD(ret));
   WORLD::add_command(cmd);

   return _prim = ret;
}

int 
PAPER_DOLL::draw(CVIEWptr& v)
{
   return 0;
}

void
PAPER_DOLL::reset()
{
   _prim = 0;
}

int  
PAPER_DOLL::cancel_cb(CGESTUREptr& gest, DrawState*& s)
{
   err_adv(debug, "PAPER_DOLL::cancel_cb");

   // Turn off widget:
   deactivate();

   // Reset state to start:
   s = &_draw_start;

   return 1; // This means we did use up the gesture
}

int  
PAPER_DOLL::tap_cb(CGESTUREptr& gest, DrawState*& s)
{
   err_adv(debug, "PAPER_DOLL::tap_cb");

   return cancel_cb(gest, s);
}

int  
PAPER_DOLL::stroke_cb(CGESTUREptr& gest, DrawState*& s)
{
   err_adv(debug, "PAPER_DOLL::stroke_cb");
   assert(gest);

   return 1; // This means we did use up the gesture
}

// end of file paper_doll.C
