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
#ifndef INCLUDE_TESS_INLINE_H
#define INCLUDE_TESS_INLINE_H

/*****************************************************************
 * ti.H: (tess inline include file):
 *
 *      Convenient inlined methods for the tess library.
 *****************************************************************/
#include "disp/view.H"
#include "gtex/basic_texture.H"
#include "mesh/mi.H"
#include "tess/tex_body.H"
#include "tess/bsurface.H"
#include "tess/tess_cmd.H"
#include "tess/disk_map.H"

namespace tess {

   const int MIN_EDGES_FOR_CLOSED_CURVE = 3;
   const int MIN_EDGES_FOR_OPEN_CURVE = 1;

inline LMESHptr
get_common_mesh(Bpoint* b1, Bpoint* b2, MULTI_CMDptr cmd)
{
   // If both points are from the same mesh, return that mesh.
   // If both points are null, return a FFS skeleton mesh.
   // If one is null, return the mesh of the other.
   if (!(b1 || b2)) {
      // neither point exists: use new skel mesh
      return TEXBODY::get_skel_mesh(cmd);
   } else if (b1 && b2) {
      // both exist: same mesh that is not null?
      return (b1->mesh() && b1->mesh() == b2->mesh()) ? b1->mesh() : 0;
   } else if (b1) {
      return b1->mesh();
   } else {
      assert(b2);
      return b2->mesh();
   }
}

/*
// Convenience: make a Bcurve, with support for undo:
inline Bcurve*
create_curve(LMESHptr m, CWpt_list& wpts, mlib::CWvec& n,
             int num_edges, int res_level,
             Bpoint* b1, Bpoint* b2, MULTI_CMDptr cmd)
{
   if (!(m || (m = get_common_mesh(b1, b2, cmd)))) {
      cerr << "create_curve: can't get a mesh" << endl;
      return 0;
   }

   Bcurve* ret = new Bcurve(m, wpts, n, num_edges, res_level, b1, b2);
   if (cmd)
      cmd->add(new SHOW_BBASE_CMD(ret));
	
   return ret;
}
//*/

inline void
draw_edge_strip(EdgeStrip& strip, double width, CCOLOR& color, double alpha)
{
   // correct width for hi-res, multi-pass renderings
   width *= VIEW::peek()->line_scale();

   // init antialiasing
   GL_VIEW::init_line_smooth(GLfloat(width), GL_CURRENT_BIT);

   glDisable(GL_LIGHTING);      // GL_ENABLE_BIT
   GL_COL(color, alpha);        // GL_CURRENT_BIT

   GLStripCB cb;
   strip.draw(&cb);

   // end antialiasing
   GL_VIEW::end_line_smooth();
}

inline void
push(CBface_list& faces, MULTI_CMDptr& cmd, bool push_boundary=true)
{
   // push the faces, create a command so it is undoable:

   if (faces.empty())
      return;

   // if the set of faces exactly matches a Bsurface,
   // use Bbase::hide instead of just pushing the faces:
   //Bsurface* surf = Bsurface::get_surface(faces);
   //if (surf && surf->bfaces().same_elements(faces)) {
   //   if (cmd)
   //      cmd->add(new HIDE_BBASE_CMD(surf,false));
   //   else
   //      surf->hide();
   //} else {
      faces.push_layer(push_boundary);
      if (cmd)
         cmd->add(new PUSH_FACES_CMD(faces, push_boundary));
   //}
}

inline double
uv_delt(Bedge* e, Bface* f)
{
   // f null, return 1
   if (!UVdata::has_uv(f))
      return 1;
   assert(f->contains(e));

   UVvec delt = UVdata::get_uv(e->v1(), f) - UVdata::get_uv(e->v2(), f);
   double ret = max(fabs(delt[0]),fabs(delt[1]));
   return ret;
}

inline double
min_uv_delt(Bedge* e)
{
   if (!e) return 1;

   return min(uv_delt(e, e->f1()),uv_delt(e, e->f2()));
}

inline double
min_uv_delt(CBedge_list& edges)
{
   double ret = 1.0;

   for (int i=0; i<edges.num(); i++) {
      ret = min(ret, min_uv_delt(edges[i]));
   }
   return ret;
}

// XXX - should move to mlib
inline double
avg_dist(CWpt_list& p, mlib::CWpt& o)
{
   if (p.empty()) return 0;
   double ret = 0;
   for (int i=0; i<p.num(); i++)
      ret += o.dist(p[i]);
   return ret/p.num();
}

inline double 
radius(CWpt_list& pts)
{
   return avg_dist(pts, pts.average());
}

/*****************************************************************
 * InflateCreaseFilter:
 *
 *      Filter for identifying "creases" where the inflate
 *      operation has to act carefully
 *
 *****************************************************************/
class InflateCreaseFilter : public SimplexFilter {
 public:
   virtual bool accept(CBsimplex* s) const;
};

//
// returns the distinct normals separated by edges of the
// selected type (or by border edges)
//
// XXX - move to bvert.H
//
inline ARRAY<Wvec>
get_norms(CBvert* v, CSimplexFilter& filter)
{
   ARRAY<Wvec> ret;
   Bface_list leaders = leading_faces(v, filter); // empty if v == 0
   for (int i=0; i<leaders.num(); i++)
      ret += vert_normal(v, leaders[i], filter);
   return ret;
}

//
// how much to scale the offset at creases
//
inline double
offset_scale(CBvert* v, CSimplexFilter& filter)
{
   // The filter accepts edges that serve as boundaries
   // between distinct portions of surface.

   if (!(v && v->degree(filter) > 0))
      return 1;

   // get average normal:
   Wvec n = v->norm();

   // get distinct normals separated by edges
   // accepted by the filter
   ARRAY<Wvec> norms = get_norms(v, filter);
   assert(norms.num() > 0);

   // return the average of 1/cos(theta_i), where theta_i is
   // the angle between n and norms[i]. but don't let the
   // cosine term get smaller than 0.5:
   RunningAvg<double> ret(0);
   for (int i=0; i<norms.num(); i++)
      ret.add(1/max(n * norms[i], 0.5));
   return ret.val();
}

inline void
show_surface(Bsurface* s, MULTI_CMDptr cmd)
{
   assert(s && cmd);
   // old way: show/hide the surface:
//    cmd->add(new SHOW_BBASE_CMD(s));

   // new way: unpush/push the faces:
   // need this to restore the surface when multiple undo ops
   // are happening in sequence:
   cmd->add(new UNPUSH_FACES_CMD(s->bfaces()));;
}

inline bool
resample(Bcurve* c, int num_edges)
{
   if (!c)
      return false;
   if (c->num_edges() == num_edges)
      return true;
   if (!c->can_resample())
      return false;
   c->resample(num_edges);
   return true;
}

inline bool
resample(const Bcurve_list& curves, int num_edges)
{
   bool ret = true;
   for (int i=0; i<curves.num(); i++) {
      ret = resample(curves[i],num_edges) && ret;
   }
   return ret;
}

inline bool
can_resample(Bcurve* c, int num_edges)
{
   return (c && (c->num_edges() == num_edges || c->can_resample()));
}

inline DiskMap*
create_disk_map(CBface_list& disk, CWvec& n, bool flip_tan)
{
   DiskMap* ret = DiskMap::create(disk, flip_tan);
   if (ret)
      ret->set_norm(n);
   return ret;
}

inline DiskMap*
create_disk_map(CBface_list& disk, CWvec& n, CWpt& p, bool flip_tan)
{
   DiskMap* ret = create_disk_map(disk, n, flip_tan);
   if (ret)
      ret->set_pt(p);
   return ret;
}

} // namespace tess

#endif // INCLUDE_TESS_INLINE_H
