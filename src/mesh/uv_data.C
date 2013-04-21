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
#include "gtex/util.H"  // XXX - remove after done debugging
#include "std/config.H"
#include "mesh/uv_data.H"

using namespace mlib;

static bool debug = Config::get_var_bool("DEBUG_UV_DATA",false);

/*****************************************************************
 * UVdata
 *****************************************************************/
UVdata::UVdata(Bsimplex* s) :
   SimplexData(key(), s),
   _uv_valid(false),
   _calc_type(SIMPLE_CALC),     // default: simple subdiv scheme
   _did_subdiv(false),
   _mapping(0) 
{
   // The constructor is called (internally) only when the given
   // simplex does not already have a UVdata associated with it.
   //
   // UVdata gets looked-up by its classname.

   // For face data, slather UVdatas all over the
   // vertices and edges:
   if (is_face(s)) {
      Bface* f = (Bface*)s;
      get_data(f->v1());
      get_data(f->v2());
      get_data(f->v3());
      get_data(f->e1());
      get_data(f->e2());
      get_data(f->e3());

      // XXX - 
      //   If the UVdata is created on a pre-existing face
      //   that has already generated its subdivision
      //   elements, then currently the UVdatas won't be
      //   created on those subdivision elements ...
   }
}

bool 
UVdata::quad_has_uv(CBface* f) 
{
   // Simple check to see if the given Bface is a quad and
   // has continous uv-coordinates

   return (f            &&                   // non-null
           f->is_quad() &&                   // is a quad
           lookup(f)    &&                   // has UV-data
           is_continuous(f->weak_edge())     // UV is continuous in quad
           );
}

bool
UVdata::get_uv(CBvert* v, UVpt& uv)
{
   if (!v)
      return 0;

   // Try the lightweight way first:
   UVdata* uvd = lookup(v);
   if (uvd && uvd->_uv_valid) {
      uv = uvd->_uv;
      return 1;
   }

   if (!is_continuous(v))
      return 0;

   return get_uv(v, v->get_face(), uv);
}

bool
UVdata::get_uv(CBvert* v, CBface* f, UVpt& uv) 
{
   // Extract uv-coords for a Bvert on a Bface,
   // checking for errors.

   // An error occurs if v or f are nil, or f does not contain
   // v, or there are no uv-coords assigned to v or f.

   UVdata* uvd = lookup(v);
   if (uvd && uvd->_uv_valid) {
      uv = uvd->_uv;
      return true;
   }
   if (f && f->contains(v) && has_uv(f)) {
      uv =  f->tex_coord(v);
      return true;
   }
   return false;
}

UVpt 
UVdata::get_uv(CBvert* v, CBface* f) 
{
   UVpt ret;
   if (!get_uv(v, f, ret) && debug) {
      if (!f) {
         err_msg("UVdata::get_uv: error: null face");
      } else {
         err_msg("UVdata::get_uv: error, showing triangle");
         GtexUtil::show_tris(Bface_list((Bface*)f));
      }
   }
   return ret;
}

CUVpt& 
UVdata::uv(CBvert* v) const 
{
   // Non-static method that is safe to call on a UVdata
   // associated w/ a face.

   // Prefer a uv-coord stored explicitly on the vertex:
   UVdata* uvd = lookup(v);
   if (uvd && uvd->_uv_valid)
      return uvd->_uv;
   else
      return face()->tex_coord(v);
}

bool
UVdata::set(Bvert* v, CUVpt& uv) 
{
   // If uv-coords around v are continuous, then assign a new
   // value at v and return true. Otherwise return false.

   if (!(v && is_continuous(v)))
      return 0;
   _set(v, uv);
   return 1;
}

void
UVdata::split(Bvert* v)
{
   if (!v) return;

   UVdata* uvd = lookup(v);
   if (uvd && uvd->_uv_valid) {
      // If uvd->_uv_valid is true, we must set it to false
      // and then push the old uv coord onto all the
      // neighboring faces.
      Bface_list faces = v->get_all_faces();
      for (int i=0; i<faces.num(); i++) {
         if (lookup(faces[i]) == NULL) {
            if (debug) {
               err_msg("UVdata::split: continous vert next to non-uv face!");
               GtexUtil::show_tris(Bface_list(faces[i]));
               WORLD::show(v->loc(), 6, Color::red);
            }
            continue;
         }
//         get_data(faces[i]);
         assert(lookup(faces[i]) != NULL); // must be, right?
         faces[i]->tex_coord(v) = uvd->_uv;
      }
      uvd->_uv_valid = false;
   }
}

void
UVdata::split(CBvert_list& verts)
{
   for (int i=0; i<verts.num(); i++)
      UVdata::split(verts[i]);
}

void
UVdata::split_chain(Bvert_list chain)
{
   split(chain);
   for (Bvert_list next; get_subdiv_chain(chain, 1, next); chain = next)
      split(next);
}

void
UVdata::split(CEdgeStrip& strip)
{
   ARRAY<Bvert_list> chains;
   strip.get_chains(chains);
   for (int i=0; i<chains.num(); i++)
      split_chain(chains[i]);
}

bool
UVdata::set(Bvert* v, Bface* f, CUVpt& uv)
{
   // Set uv-coords on a face for a given vertex

   // Doesn't hurt to be careful:
   if (!(f && v))
      return false;

   // Sometimes they toy with us:
   if (!f->contains(v)) {
      err_msg("UVdata::set: Error: face doesn't contain given vertex");
      return false;
   }

   split(v);
   // We are setting the uv-coord per face for this vertex. So if
   // there was previously an assignment of continuous uv at the
   // given vertex, we must clear that away.
//    UVdata* uvd = lookup(v);
//    if (uvd && uvd->_uv_valid) {
//       // If uvd->_uv_valid is true, we must set it to false
//       // and then push the old uv coord onto all the
//       // neighboring faces.
//       Bface_list faces = v->get_all_faces();
//       for (int i=0; i<faces.num(); i++) {
//          get_data(faces[i]);
// //         assert(lookup(faces[i]) != NULL); // must be, right?
//          faces[i]->tex_coord(v) = uvd->_uv;
//       }
//       uvd->_uv_valid = false;
//    }

   // Make sure UVdata is there
   get_data(f);

   // Set the per-face uv-coords:
   f->tex_coord(v) = uv;

   return true;
}

bool 
UVdata::offset_uv(Bvert* v, CUVvec& delt)
{
   UVpt cur;
   if (!get_uv(v, cur))
      return 0;
   return set(v, cur + delt);
}

bool
UVdata::offset_uv(Bvert* v, Bface* f, CUVvec& delt)
{
   UVpt cur;
   if (!get_uv(v, f, cur))
      return 0;
   return set(v, f, cur + delt);
}

bool 
UVdata::set(
   Bvert*   a, Bvert*   b, Bvert*   c,
   CUVpt& uva, CUVpt& uvb, CUVpt& uvc
   )
{
   // Set uv-coords on a face
   // (provided the 3 CCW verts define one):

   //                 c                               
   //                /|                                
   //              /  |                                
   //            /    |                                
   //          /      |                                
   //        /        |                                
   //      /          |                                
   //    a ---------- b                                
   //

   // First try to get the face
   Bface* f = lookup_face(a,b,c);
   return f ? set(f, a, b, c, uva, uvb, uvc) : false;
}

bool 
UVdata::set(
   Bvert*   a, Bvert*   b, Bvert*   c, Bvert*   d,
   CUVpt& uva, CUVpt& uvb, CUVpt& uvc, CUVpt& uvd
   )
{
   // Set uv-coords on a quad
   // (provided the 4 CCW verts define one):

   //    d ---------- c                               
   //    |            |                                
   //    |            |                                
   //    |            |                                
   //    |            |                                
   //    |            |                                
   //    |            |                                
   //    a ---------- b                                
   //
   // We'll try the diagonal both ways before giving up...

   // First try to get the two faces assuming the diagonal
   // runs from a to c:
   Bface* lower = lookup_face(a,b,c);
   Bface* upper = lookup_face(a,c,d);

   if (upper && lower) {
      set(lower, a, b, c, uva, uvb, uvc);
      set(upper, a, c, d, uva, uvc, uvd);
      return true;
   }

   // If that didn't work try it the other way:
   lower = lookup_face(a,b,d);
   upper = lookup_face(b,c,d);

   if (upper && lower) {
      set(lower, a, b, d, uva, uvb, uvd);
      set(upper, b, c, d, uvb, uvc, uvd);
      return true;
   }

   // We didn't want to do it anyway
   return false;
}

bool 
UVdata::get_quad_uvs(
   CBface* f,
   UVpt& uva,
   UVpt& uvb,
   UVpt& uvc,
   UVpt& uvd)
{
   // Pull out the uv-coordinates (in standard quad order)
   // from the given Bface that is supposedly a quad.

   //    d ---------- c                               
   //    |          / |                                
   //    |        /   |                                
   //    |      /     |                                
   //    |    /       |                                
   //    |  /         |                                
   //    |/           |                                
   //    a ---------- b                                
   //
   //   STANDARD QUAD ORDER

   Bvert *a=0, *b=0, *c=0, *d=0;
   return (f && f->get_quad_verts(a, b, c, d) &&
           get_quad_uvs(a,b,c,d,uva,uvb,uvc,uvd));
}

bool 
UVdata::get_quad_uvs(
   CBvert* a,
   CBvert* b,
   CBvert* c,
   CBvert* d,
   UVpt& uva,
   UVpt& uvb,
   UVpt& uvc,
   UVpt& uvd)
{
   // Pull out the uv-coordinates from the 4 given vertices
   // that supposedly form a quad.

   //    d ---------- c                               
   //    |            |                                
   //    |            |                                
   //    |            |                                
   //    |            |                                
   //    |            |                                
   //    |            |                                
   //    a ---------- b                                
   //

   if (!(a && b && c && d))
      return false;

   // Try the diagonal running NE:
   Bedge* e = a->lookup_edge(c);
   if (e) {
      if (!quad_has_uv(e->f1()))
         return 0;

      // Get the two faces, trying the diagonal running NE
      Bface* lower = lookup_face(a,b,c);
      Bface* upper = lookup_face(a,c,d);

      // Should never fail, but check anyway:
      if (!(upper && lower))
         return 0;

      // Get the data and return happy
      uva = get_uv(a, lower);
      uvb = get_uv(b, lower);
      uvc = get_uv(c, lower);
      uvd = get_uv(d, upper);

      return 1;
   }

   // Try the diagonal the other way:
   e = b->lookup_edge(d);
   if (!(e && quad_has_uv(e->f1())))
      return 0;

   // Get the two faces, trying the diagonal running NE
   Bface* lower = lookup_face(a,b,d);
   Bface* upper = lookup_face(b,c,d);

   // Should never fail, but check anyway:
   if (!(upper && lower))
      return 0;

   // Get the data and return happy
   uva = get_uv(a, lower);
   uvb = get_uv(b, lower);
   uvc = get_uv(c, upper);
   uvd = get_uv(d, upper);

   return 1;
}

bool
UVdata::quad_interp_texcoord(CBface* f, CUVpt& uv, UVpt& ret)
{
   // For the given quad, use bilinear interpolation on the
   // UV coordinates at the quad vertices to map the given
   // UV coordinate (WRT the quad) to a UV coordinate. I.e.,
   // the 'UV' coordinate in this case can vary from (0,0)
   // in the lower left corner of the quad (see below) to
   // (1,1) in the upper right.
   //
   // For this to succeed, the given Bface must be a quad,
   // there must be texture coordinates defined at the 4
   // quad vertices, and there can be no UV-discontinuity
   // along the quad diagonal.

   //
   //    d ---------- c                               
   //    |          / |                                
   //    |        /   |                                
   //    |      /     |                                
   //    |    /       |                                
   //    |  /         |                                
   //    |/           |                                
   //    a ---------- b                                
   //
   //   STANDARD QUAD ORDER

   // Get the uv-coords at the four corners:
   UVpt uva, uvb, uvc, uvd;
   if (!get_quad_uvs(f, uva, uvb, uvc, uvd))
      return false;

   // Do bilinear interpolation
   UVvec x0 = uvb - uva;
   UVvec x1 = uvc - uvd;
   UVvec y0 = uvd - uva;
   ret = uva + x0*uv[0] + y0*uv[1] + (x1 - x0)*(uv[0]*uv[1]);

   return true;
}

bool
UVdata::is_continuous(CBedge* e) 
{
   // Check for uv discontinuity across the edge
   if (!e)
      return 1; // no edge -- whatever

   Bvert *v1 = e->v1(), *v2 = e->v2();  // Should always exist
   Bface *f1 = e->f1(), *f2 = e->f2();  // Either might be null
   UVdata* uvd1 = lookup(f1);
   UVdata* uvd2 = lookup(f2);
   if (!(uvd1 || uvd2)) {
      // no uv coords at all -- no discontinuity
      return 1;
   } else if (uvd1 && uvd2) {
      // uv on both sides -- continuous iff they agree:
      return (
         get_uv(v1,f1) == get_uv(v1,f2) &&
         get_uv(v2,f1) == get_uv(v2,f2)
         );
   } else {
      // uv on one side but not the other -- discontinuity
      // (but only if both faces are present)
      return !(f1 && f2);
   }
}

int 
UVdata::discontinuity_degree(CBvert* v) 
{
   // Gives the number of UV-discontinous edges adjacent to v:

   int ret = 0;
   for (int i=0; i<v->degree(); i++)
      if (!is_continuous(v->e(i)))
         ret++;
   return ret;
}

void
UVdata::notify_subdiv_gen() 
{
   // Called right after new subdivision elements have been
   // generated by the simplex upon which this UVdata is
   // stored. Now is the time to generate UVdata on the
   // newly created subdivision elements.

   if (is_face(simplex())) {
      // Propagate data to the 4 subdivision faces.
      // That also puts it on the subdivision verts and edges.
      Lface* f = (Lface*) simplex();
      get_data(f->subdiv_face1());
      get_data(f->subdiv_face2());
      get_data(f->subdiv_face3());
      get_data(f->subdiv_face_center());
   }
}

bool
UVdata::handle_subdiv_calc()
{
   // Some of you may have been wondering why anyone would
   // need UVdata on an edge. The whole reason is to pick up
   // this here callback, which is generated when it is time
   // to recompute subdivision values for an edge or vertex.
   // (I.e., this is also why you need UVdata on a vertex).
   //
   // XXX - Currently we aren't keeping track of when
   //       subdivision uv-coords need to be recomputed. We
   //       just get this callback whenever subdivision
   //       *locations* need to be recomputed. So the wrong
   //       thing happens if you change the uv-coords of the
   //       control mesh but not the vertex locations or
   //       connectivity. And if you change the vertex
   //       locations but not the uv-coords the latter get
   //       recomputed even though they don't need to be.

   // XXX - even worser: working on a deadline, we're changing
   //  mesh connectivity at arbitrary subdivision levels
   //  (not previously done), and that's leading to an assertion
   //  failure in subdiv_uv(Bedge*, Bface*) below...
   //  needs to get fixed (later)
   //   lem - 1/6/2002
//   return 0;

   // Don't compute it more than once.  Current policy is uv
   // coords are assigned but not later edited or changed ever.
   // Why? Because with mesh connectivity edits happening at
   // arbitrary levels of subdivision, we don't want parent
   // elements forcing inappropriate uv values on elements at
   // current level. -- lem 12/29/2004; same deadline :(
   if (_did_subdiv) {
//      return false;
   }
   _did_subdiv = true;

   if (is_edge(simplex())) {
      Ledge* e = (Ledge*) simplex();

      if ( !e->get_face() )
         return false;

      if (is_continuous(e)) {
         // Set a single uv-coord on the subdivision vertex
         // to be used regardless of the face containing it.
         _set(e->subdiv_vertex(), subdiv_uv(e, e->get_face()));
      } else {
         // Compute 2 separate subdiv uv-coords, set each one
         // on the 3 sub-faces on each side of the edge.
         // (If a face is missing or has no uv-coords it's a no-op).
         set_subdiv_uv(e, (Lface*)e->f1());
         set_subdiv_uv(e, (Lface*)e->f2());
      }
   } else if (is_vert(simplex())) {
      Lvert* v = (Lvert*) simplex();

      if ( !v->get_face() )
         return false;

      if (is_continuous(v)) {
         // Set a single uv-coord on the subdivision vertex
         // to be used regardless of the face containing it.
         _set(v->subdiv_vertex(), subdiv_uv(v, v->get_face()));
      } else {
         // Compute and set a separate uv-coord for each face
         // surrounding the subdiv vert:
         Bface_list faces;
         v->get_faces(faces);
         for (int i=0; i<faces.num(); i++)
            set_subdiv_uv(v, (Lface*)faces[i]);
      }
   } else {
      // We get here if this UVdata is on a Bface or a null simplex.
      // Neither should ever happen.
      assert(0);
   }

   // Returning 0 means we are not overriding the normal
   // subdivision calculation.
   return 0;
}

void
UVdata::set_subdiv_uv(Ledge* e, Lface* f)
{
   // Compute the uv-coords for the subdivision vertex of
   // the given edge e and set them on the 3 subdivision
   // faces generated by the given face f, which is adjacent
   // to e. (See ASCII diagram below).

   // Don't deal w/ null pointers
   if (!(e && f))
      return;

   // Do nothing if f has no uv-coords
   if (!lookup(f)) 
      return;

   // Don't put up with silliness
   assert(f->contains(e));

   /*      
   //
   //   e runs vertically from subv1 to subv2,
   //   f is on the right, producing 4 sub-faces. 
   // 
   //       subv1              
   //           | \            
   //           |   \ subva    
   //           | f1 /\        
   //           |  /  | \      
   //      subv |/  f2|   \                 
   //           | \   |  /     
   //           |   \ |/       
   //           | f3 / subvb   
   //           |  /           
   //           |/             
   //       subv2
   //                                                       
   //
   // We want to compute the uv-coords for e's subdivision vertex,
   // then set that uv-coord on the 3 sub-faces shown (f1, f2, f3).
   */   
   
   // Ensure subdiv elements exist:
   f->allocate_subdiv_elements();

   // Get the needed subdivision vertices
   Bvert* subv  = e->subdiv_vertex();
   Bvert* subv1 = e->lv(1)->subdiv_vertex();
   Bvert* subv2 = e->lv(2)->subdiv_vertex();
   Bvert* subva = ((Ledge*)f->other_edge(e->v1(),e))->subdiv_vertex();
   Bvert* subvb = ((Ledge*)f->other_edge(e->v2(),e))->subdiv_vertex();

   // Get the 3 sub-faces (CCW order is not relevant):
   Bface* f1 = lookup_face(subv1,subv,subva);
   Bface* f2 = lookup_face(subva,subv,subvb);
   Bface* f3 = lookup_face(subv2,subv,subvb);

   // Set the same uv-coords on the 3 faces
   CUVpt& uv = subdiv_uv(e, f);
   if (f1)
      set(subv, f1, uv);
   if (f2)
      set(subv, f2, uv);
   if (f3)
      set(subv, f3, uv);
}

void
UVdata::set_subdiv_uv(Lvert* v, Lface* f)
{
   // Compute the subdivision uv-coords for the given vertex
   // which belongs to the given face. Then assign those
   // uv-coords to the subdivison vertex of v on the
   // corresponding subdivision face of f.

   // Don't deal w/ null pointers
   if (!(v && f))
      return;

   // Do nothing if f has no uv-coords
   if (!lookup(f)) 
      return;

   // Don't put up with silliness
   assert(f->contains(v));

   /*******************************************************
   //                                                  
   //                      /\                          
   //                    /   \                         
   //                  /      \                        
   //          subvb /- - - - -\                       
   //              /  \      /  \                      
   //            /     \   /     \                     
   //          /  subf  \/        \                    
   //     subv --------------------                    
   //               subva                              
   //
   *******************************************************/

   // Ensure subdiv elements exist:
   f->allocate_subdiv_elements();

   // Get the needed subdivision vertices
   Bvert* subv  = v->subdiv_vertex();
   Bvert* subva = ((Ledge*)f->edge_from_vert(v))->subdiv_vertex();
   Bvert* subvb = ((Ledge*)f->edge_before_vert(v))->subdiv_vertex();

   // Get the sub-face:
   Bface* subf = lookup_face(subv,subva,subvb);

   // Set the uv-coord for the sub-vert on the sub-face:
   if (subf)
      set(subv, subf, subdiv_uv(v,f));
}

/*****************************************************************
 * Computing subdivision uv-coords
 *****************************************************************/
#include "subdiv_calc.H"

// Need this operator defined to instantiate LoopCalc<UVpt> below:
UVpt
operator+(CUVpt& a, CUVpt& b)
{
   return UVpt(a[0] + b[0], a[1] + b[1]);
}

/*****************************************************************
 * LoopUV
 *
 *      Calculates subdivision values of uv-coordinates
 *****************************************************************/
class LoopUV : public LoopCalc<UVpt> {
 protected:
   CBface*    _face; // Used in get_val() to lookup correct uv

 public:
   LoopUV(CBface* f) : _face(f) { assert(f); }

   //******** VALUE ACCESSORS ********
   virtual UVpt get_val(CBvert* v) const { return UVdata::get_uv(v, _face);}

   virtual UVpt centroid(CLvert* v) const;

   //******** SUBDIVISION CALCULATION ********
   virtual UVpt subdiv_val(CBvert* bv) const;
   virtual UVpt subdiv_val(CBedge* be) const;

   //******** GETTING MORE LIKE IT ********
   // we don't do duping
   virtual SubdivCalc<UVpt> *dup() const { return 0; }
};

UVpt
LoopUV::centroid(CLvert* v) const
{
   assert(v);

   switch (UVdata::discontinuity_degree(v)) {
    case 0:
    {
       // No discontinuity:
       UVpt ret;
       for (int i=0; i<v->degree(); i++)
          ret += UVdata::get_uv(v->nbr(i), v->e(i)->get_face());
       return ret / v->degree();
    }
    case 2:
    {
       // Find the 2 discontinuity edges and get the uv values of
       // their opposite vertices that agree with the current vertex.
       CUVpt& uv = UVdata::get_uv(v,_face);
       UVpt ret;
       for (int i=0; i<v->degree(); i++) {
          Bedge* e = v->e(i);
          if (!UVdata::is_continuous(e)) {
             if (UVdata::lookup(e->f1()) && UVdata::get_uv(v,e->f1()) == uv)
                ret += UVdata::get_uv(v->nbr(i), e->f1());
             else if (UVdata::lookup(e->f2()) &&
                      UVdata::get_uv(v,e->f2()) == uv)
                ret += UVdata::get_uv(v->nbr(i), e->f2());
             else
                assert(0);
          }
       }
       return ret/2;
    }
    default:
       // Treat as a corner vertex
       return UVdata::get_uv(v, _face);
   }
}

UVpt
LoopUV::subdiv_val(CBvert* bv) const 
{
   assert(bv);

   // XXX - has to be an Lvert
   Lvert* v = (Lvert*)bv;
   switch (UVdata::discontinuity_degree(v)) {
    case 0:     return smooth_subdiv_val(v);
    case 2:     return crease_subdiv_val(v);
    default:    return get_val(v);
   }
}

UVpt
LoopUV::subdiv_val(CBedge* e) const 
{
   assert(e);

   if (UVdata::is_continuous(e)) {
      return (
         (get_val(e->v1()) + get_val(e->v2()))*3.0 +
         UVdata::get_uv(e->opposite_vert1(), e->f1()) +
         UVdata::get_uv(e->opposite_vert2(), e->f2())
         )/8.0;
   } else {
      return (get_val(e->v1()) + get_val(e->v2()))/2.0;
   }
}

/*****************************************************************
 * SimpleUVCalc:
 *
 *      Calculates subdivision values of uv-coordinates via simple
 *      scheme.
 *****************************************************************/
class SimpleUVCalc : public SimpleCalc<UVpt> {
 protected:
   CBface*    _face; // Used in get_val() to lookup correct uv

 public:
   //******** MANAGERS ********
   SimpleUVCalc(CBface* f) : _face(f) { assert(f); }

   //******** VALUE ACCESSORS ********
   virtual UVpt get_val(CBvert* v) const { return UVdata::get_uv(v, _face);}

   //******** SUBDIVISION CALCULATION ********
   virtual UVpt subdiv_val(CBvert* bv) const { return get_val(bv); }
   virtual UVpt subdiv_val(CBedge* be) const;

   //******** GETTING MORE LIKE IT ********
   // we don't do duping
   virtual SubdivCalc<UVpt> *dup() const { return 0; }
};

UVpt
SimpleUVCalc::subdiv_val(CBedge* e) const 
{
   assert(e);

   if (e->is_weak()) {

      // For quad diagonals we actually average the 4 quad points:

      Bface* f1 = e->f1();
      Bface* f2 = e->f2();

      if (debug && !UVdata::is_continuous(e)) {
         cerr << "SimpleUVCalc::subdiv_val: found discontinuous weak edge "
              << "at level " << e->mesh()->subdiv_level() << endl;
         WORLD::show(e->v1()->loc(), e->v2()->loc(), 4);
         return UVpt::Origin();
      }

      // Many assumptions should hold:
      //assert(UVdata::is_continuous(e));
      assert(f1 && f1->is_quad() &&
             f2 && f2->is_quad() && f1->quad_partner() == f2);
      assert(_face == f1 || _face == f2);

      return (
         UVdata::get_uv(f1->v1(), f1) +
         UVdata::get_uv(f1->v2(), f1) +
         UVdata::get_uv(f1->v3(), f1) +
         UVdata::get_uv(f1->quad_vert(), f2))/4.0;
   } else {
      return (get_val(e->v1()) + get_val(e->v2()))/2.0;
   }
}

UVpt
UVdata::subdiv_uv(CBvert* v, CBface* f) const
{
   switch(_calc_type) {
    case SIMPLE_CALC:   return SimpleUVCalc(f).subdiv_val(v);
    case HYBRID_CALC:     
      err_msg("UVdata::subdiv_uv: Hybrid scheme not implemented for UV");
      // No break - it drops down to the default clause:
    default:
      return LoopUV(f).subdiv_val(v);
   }
}

UVpt
UVdata::subdiv_uv(CBedge* e, CBface* f) const
{
   switch(_calc_type) {
    case SIMPLE_CALC:   return SimpleUVCalc(f).subdiv_val(e);
    case HYBRID_CALC:     
      err_msg("UVdata::subdiv_uv: Hybrid scheme not implemented for UV");
      // No break - it drops down to the default clause:
    default:
      return LoopUV(f).subdiv_val(e);
   }
}

/* end of file uv_data.C */
