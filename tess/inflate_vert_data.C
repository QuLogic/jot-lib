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
#include "geom/world.H"
#include "mesh/mi.H"
#include "std/config.H"

#include "bcurve.H"
#include "inflate_vert_data.H"

using Wpt;
using Wvec;

static bool debug=Config::get_var_bool("DEBUG_EXTRUDE",false);

/*****************************************************************
 * InflateVertData
 *****************************************************************/
InflateVertData::InflateVertData(Bvert* v) :
   SimplexData(key(), v)
{
   // The constructor is called (internally) only when the given
   // simplex does not already have a InflateVertData associated
   // with it.
   //
   // InflateVertData gets looked-up by its classname.

   assert(v != NULL);
}

bool 
InflateVertData::gen_twins(Bvert* v, double h, CSimplexFilter& filter)
{
   // generate all the inflate vertices for the given vertex
   // (one for each separate normal field in the 1-ring):

   // check on the mesh
   assert(v && v->mesh());
   BMESH* mesh = v->mesh();

   // get the data
   InflateVertData* vd = get_data(v);
   assert(vd);
   if (!vd->_verts.empty()) {
      err_msg("InflateVertData::gen_twins: called twice?! bailing...");
      return false;
   }

   // get representative faces, one per normal field:
   Bface_list faces = leading_faces(v, filter);
   if (faces.num() == 0) {
      err_adv(debug, "InflateVertData::gen_twins: error: no faces at vertex");
      return 0;
   }

   // offset amount (relative to local edge size):
   double t = h * avg_strong_edge_len(v->get_adj());

   // offset position and direction:
   Wpt  p = v->loc();
   Wvec n = v->norm();

   // no creases: do a single offset:
   if (faces.num() == 1) {
      vd->_verts += OffsetVert(mesh->add_vertex(p + n*t), v);
      return true;
   }

   // if 2 normals, test angles, maybe insert 1
   //               otherwise do both

   if (faces.num() == 2) {
      // all normals are angle weighted averages
      Wvec   n0 = vert_normal(v, faces[0], filter);
      Wvec   n1 = vert_normal(v, faces[1], filter);
      double a0 = n0.angle(n);
      double a1 = n1.angle(n);

      // if the angle between each normal and the average is
      // less than MIN_ANGLE, generate a single vertex
      const double MIN_ANGLE = 25.0; // degrees
      if (rad2deg(a0) < MIN_ANGLE && rad2deg(a1) < MIN_ANGLE) {
         // rather than generate 2 vertices, we generate 1
         // "collapsed" vertex. 
         //
         // positioning is a bit different...
         // offset more ... same adjustment for concave or convex:
         // use average of offsets for 2 sides
         t *= (1/cos(a0) + 1/cos(a1))/2;

         vd->_verts += OffsetVert(mesh->add_vertex(p + n*t), v);
         return true;
      }

      // generate 2 separate vertices,
      // later have to deal with sewing
      vd->_verts += OffsetVert(mesh->add_vertex(p + n0*t), faces[0]);
      vd->_verts += OffsetVert(mesh->add_vertex(p + n1*t), faces[1]);
      return true;
   }

   // If > 2 normals, generate a vertex for each normal field?
   // (this is a stop-gap until we figure out what to do):
   for (int i=0; i<faces.num(); i++) {
      Wvec ni = vert_normal(v, faces[i], filter);
      vd->_verts += OffsetVert(mesh->add_vertex(p + ni*t), faces[i]);
   }
   return true;

//     if (debug)
//        WORLD::show(v->loc(), n);

   return true;
}

Bvert* 
InflateVertData::get_twin(CBvert* v, CBsimplex* s)
{
   // find the inflate "twin" for the given face containing v

   InflateVertData* vd = lookup(v);
   if (!vd)
      return 0;

   Bvert* ret = 0;
   for (int i=0; i<vd->_verts.num(); i++) {
      if (vd->_verts[i]._s == s)
         return vd->_verts[i]._v;
      if (vd->_verts[i]._s == (CBsimplex*)v)
         ret = vd->_verts[i]._v;
   }
   return ret;
}

/* end of file inflate_vert_data.C */
