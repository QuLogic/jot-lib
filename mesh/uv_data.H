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
/*****************************************************************
 * uv_data.H
 *****************************************************************/
#ifndef UV_DATA_H_IS_INCLUDED
#define UV_DATA_H_IS_INCLUDED

#include "lface.H"

class UVMapping;

/*****************************************************************
 * UVdata:
 *
 *    UV coordinates associated with a Bvert on a Bface.
 *
 *    Can store uv-coords explicitly for a vertex, which is an
 *    option when all the faces around the vertex agree on what
 *    the uv-coordinate should be.
 *
 *    If they don't all agree, then each face stores its own
 *    uv-coordinate for that vertex.
 *
 *    XXX - TODO: add specialized sub-classes for handling the
 *          distinct cases that the data is stored on a Bface,
 *          Bedge, or Bvert. Currently, the UVdata class is used
 *          for storing data on all 3 of those cases, which is a
 *          little clunky.
 *****************************************************************/
#define CUVdata const UVdata
class UVdata : public SimplexData {
 public:
   //******** MANAGERS ********
   // No public constructor -- use UVdata::set(...) below

   //******** RUN TIME TYPE ID ********
   DEFINE_RTTI_METHODS2("UVdata", SimplexData, CSimplexData*);

   /*******************************************************
    * Static methods
    *******************************************************/

   //******** CHECKING FOR UV-COORDS ********

   // Determine if the face has uv-coords:
   static bool has_uv(CBface* f) { return lookup(f) != NULL; }

   // Determine if it's a quad with continuous UV-coordinates:
   static bool quad_has_uv(CBface* quad);

   //******** GETTING UV-COORDS ********

   // In case the vertex has continuous uv-coords,
   // assign them to 'uv' and return true:
   static bool get_uv(CBvert* v, UVpt& uv);

   // Extract uv-coords for a Bvert on a Bface, w/ error checking.
   // An error occurs if v or f are nil, or f does not contain
   // v, or there are no uv-coords assigned to v or f:
   static bool get_uv(CBvert* v, CBface* f, UVpt& uv);

   // Same as above, with assertion of success, for confident types:
   static UVpt get_uv(CBvert* v, CBface* f);

   static bool get_uvs(CBface* f, UVpt& uv1, mlib::UVpt& uv2, mlib::UVpt& uv3) {
      if (!f)
         return false;
      return get_uv(f->v1(), f, uv1) &&
         get_uv(f->v2(), f, uv2) && get_uv(f->v3(), f, uv3);
   }

   static bool get_uvs(CBvert* v1, CBvert* v2, CBvert* v3,
                       UVpt& uv1, mlib::UVpt& uv2, mlib::UVpt& uv3) {
      Bface* f = lookup_face(v1,v2,v3);
      if (!f)
         return false;
      return get_uv(v1, f, uv1) && get_uv(v2, f, uv2) && get_uv(v3, f, uv3);
   }

   // XXX - Here for backwards compatibility. Will be removed.
   static UVpt get_uv(CBvert* v, CBface* f, bool& success) {
      UVpt ret;
      if (get_uv(v, f, ret)) {
         success = 1;
         return ret;
      }
      success = false;
      return ret;
   }

   // Return the 4 UV-coordinates at the quad corners:
   static bool get_quad_uvs(CBface* quad,
                            UVpt& uva, mlib::UVpt& uvb, mlib::UVpt& uvc, mlib::UVpt& uvd);

   // Return the 4 UV-coordinates of the given vertices that form a quad:
   static bool get_quad_uvs(CBvert* a, CBvert* b, CBvert* c, CBvert* d, 
                            UVpt& uva, mlib::UVpt& uvb, mlib::UVpt& uvc, mlib::UVpt& uvd);

   // For the given quad, do bilinear interpolation on the UV
   // coordinates at its 4 corners. The input parameter 'uv' can
   // vary from (0,0) at the lower left to (1,1) at the upper
   // right. The computed UV coordinate is returned in
   // 'texcoord', and the boolean return value indicates success
   // (true) or failure.
   static bool quad_interp_texcoord(
      CBface* quad, CUVpt& uv, mlib::UVpt& texcoord);

   //******** SETTING UV-COORDS ********

   // If uv-coords around v are continuous, then assign a new
   // value at v and return true. Otherwise return false.
   static bool set(Bvert* v, CUVpt& uv);

   // Set uv-coords for a single vertex WRT a face
   static bool set(Bvert* v, Bface* f, CUVpt& uv);

   // Set uv-coords on a face for 3 vertices all at once:
   static bool set(Bface* f,
                   Bvert* v1, Bvert* v2, Bvert* v3,
                   CUVpt&  a, mlib::CUVpt&  b, mlib::CUVpt&  c) {
      return (set(v1,f,a) && set(v2,f,b) && set(v3,f,c)); 
   }

   // Set uv-coords on a face, with given coords corresponding to
   // the face's v1, v2, v3 in that order:
   static bool set(Bface* f, CUVpt& a, mlib::CUVpt& b, mlib::CUVpt& c) {
      return set(f, f->v1(), f->v2(), f->v3(), a, b, c);
   }

   // Set uv-coords on a face (provided the 3 CCW verts define one):
   static bool set(Bvert*  v1, Bvert*  v2, Bvert*  v3,
                   CUVpt& uv1, mlib::CUVpt& uv2, mlib::CUVpt& uv3);

   // Set uv-coords on a quad (provided the 4 CCW verts define one):
   static bool set(Bvert*  v1, Bvert*  v2, Bvert*  v3, Bvert*  v4,
                   CUVpt& uv1, mlib::CUVpt& uv2, mlib::CUVpt& uv3, mlib::CUVpt& uv4);

   // Displace the currently assigned uv-coord by the given
   // delta.  Both methods require uv-coords to exist
   // already at the given vertex. First version also
   // requires uv-coords to be continuous:
   static bool offset_uv(Bvert* v, CUVvec& delt);
   static bool offset_uv(Bvert* v, Bface* f, CUVvec& delt);

   // change from per-vert uv-representation to per-face
   // at original subdiv level and all finer ones too
   static void split(CEdgeStrip& stip);

   //******** UV-CONTINUITY ********

   // Returns true if UV-coords are continuous across the edge:
   static bool is_continuous(CBedge* e);

   // Gives the number of UV-discontinous edges adjacent to v:
   static int discontinuity_degree(CBvert* v);

   // Returns true if there are no UV-discontinuous edges around v:
   static bool is_continuous(CBvert* v) {
      return (discontinuity_degree(v) == 0);
   }

   //******** LOOKUP ********
   // Lookup a UVdata* from a Bsimplex:
   static UVdata* lookup(CBsimplex* s) {
      return s ? (UVdata*)s->find_data(key()) : 0;
   }

   // Similar to above, but creates a UVdata if it wasn't found
   static UVdata* get_data(Bsimplex* s) {
      UVdata* ret = lookup(s);
      return ret ? ret : s ? new UVdata(s) : 0;
   }

   /*******************************************************
    * Non-static methods
    *******************************************************/

   //******** ACCESSORS ********
   UVMapping*   mapping()                 const { return _mapping; }
   void         set_mapping(UVMapping *m)       { _mapping = m; }

   // XXX - Temporary, for backward compatibility.
   //       For looking up uv-coords, callers should
   //       switch over to UVdata::get_uv(v,f) as above.
   Bface* face() const { assert(is_face(_simplex)); return (Bface*)_simplex; }

   // These are only safe to call on a UVdata associated w/ a face:
   // (otherwise an assertion fails):
   CUVpt& uv(CBvert* v) const;
   CUVpt& uv(int i)     const   { return uv(face()->v(i)); }
   CUVpt& uv1()         const   { return uv(1); }
   CUVpt& uv2()         const   { return uv(2); }
   CUVpt& uv3()         const   { return uv(3); }

   //******** SETTING SUBDIVISION CALC SCHEME ********

   // XXX - These don't invalidate previously computed subdivision
   //       values (if any), as they should:
   void set_do_simple() { _calc_type = SIMPLE_CALC; }
   void set_do_loop()   { _calc_type = LOOP_CALC; }
   void set_do_hybrid() { _calc_type = HYBRID_CALC; }

   //******** COORDINATE CONVERSIONS ********
   // Convert barycentric coords to UV:
   void bc2uv(CWvec& bc, mlib::UVpt& ret) {
      ret =  (uv1()*bc[0]) + (uv2()*bc[1]) + (uv3()*bc[2]);
   }

   //******** SimplexData VIRTUAL METHODS ********
   // These are here to prevent warnings:
   // store it on the given simplex:
   void set(uint id, Bsimplex* s)       { SimplexData::set(id,s); }
   void set(Cstr_ptr& str, Bsimplex* s) { SimplexData::set(str,s); }
   
   virtual void notify_split(Bsimplex*) {
      // Q: Need to do something?
      // A: Yes. (Not implemented.)
      err_msg("UVdata::notify_split: Warning: not implemented");
   }

   // Called when subdiv elements are generated:
   virtual void notify_subdiv_gen();

   // Callback for when the simplex is deleted:
   virtual void notify_simplex_deleted() {
      // New mapping dies when the hatch groups using it die
      // off.  That should happen before we get here...
      assert(!_mapping);

      // The base class does the right thing (deletes this!):
      SimplexData::notify_simplex_deleted();
   } 

   // Callback for when subdivision values need to be recomputed:
   virtual bool  handle_subdiv_calc();

   //*****************************************************************
 protected:

   // Enum for different schemes for computing UV-subdivision values:
   enum calc_type {
      SIMPLE_CALC = 0,          // The default choice
      LOOP_CALC,                // Use scheme of Charles Loop
      HYBRID_CALC               // Use mixed Catmull-Clark / Loop
   };
      
   UVpt         _uv;            // Just used when the simplex is a Bvert
   bool         _uv_valid;      // Tells whether above is valid
   calc_type    _calc_type;     // Which scheme to use
   bool         _did_subdiv;    // if true, subdiv calc was done once

   UVMapping*   _mapping;       // XXX - need a comment here...

   // The unique ID used to lookup UVdata:
   static uint key() {
      uint ret = (uint)**static_name();
      return ret;
   }

   //******** PROTECTED CONSTRUCTOR ********
   // The constructor is called (internally) only when the given
   // simplex does not already have a UVdata associated with it.
   //
   // UVdata gets looked-up by its classname.
   UVdata(Bsimplex* s);

   // For vertices only -- record a uv-coord for that vertex, to
   // be used regardless of which face containing the vertex is
   // considered.
   void set_uv(CUVpt& uv) {
      assert(is_vert(simplex()));
      _uv = uv;
      _uv_valid = true;
   }

   // Set uv-coords on a vertex, taking precendence over any
   // per-face uv-coords around the vertex.
   //
   // The public method set(v, uv) calls this method after
   // screening to ensure uv-coords are continuous at v:
   static void _set(Bvert* v, CUVpt& uv) {
      UVdata* uvd = get_data(v);
      assert(uvd);
      uvd->set_uv(uv);
   }

   //******** SPLITTING ********

   // Change from continuous uv at a vertex to per-face uv:
   static void split(Bvert* v);

   // split the verts
   static void split(CBvert_list& verts);

   // split a chain of verts, continuing at each finer subdiv level
   static void split_chain(Bvert_list chain);

   //******** SUBDIVISION HELPERS ********

   UVpt subdiv_uv(CBvert* v, CBface* f) const;
   UVpt subdiv_uv(CBedge* e, CBface* f) const;

   void set_subdiv_uv(Ledge* e, Lface* f);
   void set_subdiv_uv(Lvert* v, Lface* f);
};

#endif // UV_DATA_H_IS_INCLUDED

/* end of file uv_data.H */
