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
 * UV_SURFACE.H
 *****************************************************************/
#ifndef UV_SURFACE_H_IS_INCLUDED
#define UV_SURFACE_H_IS_INCLUDED

#include "map3d/map2d3d.H"
#include "bsurface.H"

class UVmeme;
/*****************************************************************
 * UVsurface
 *****************************************************************/
class UVsurface: public Bsurface {
 public:
   //******** MANAGERS ********

   // Create a UVsurface from the given map, but don't
   // tessellate anything yet:
   UVsurface(Map2D3D* map, CLMESHptr& mesh);

   // Create a UVsurface from the given map and tessellate it:
   UVsurface(Map2D3D* map,
             CLMESHptr& mesh,
             int num_rows,
             int num_cols,
             bool do_cap=false);

   // Create a child UVsurface of the given parent:
   UVsurface(UVsurface* parent);

   virtual ~UVsurface();
   
   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("UVsurface", UVsurface*, Bsurface, CBnode*);

   //******** ACCESSORS ********

   Map2D3D* map()               const { return _map; }
   void set_map(Map2D3D* m);

   UVmeme* uvm(int i) const;

   bool get_uv(CBvert* v, UVpt& uv) const;
   bool get_uv(CBface* f, UVpt& uv) const;

   //******** SUBDIVISION UVsurfaces ********

   // The subdiv parent and child UVsurfaces:
   UVsurface* parent_uv_surf()   const   { return (UVsurface*) _parent; }
   UVsurface* child_uv_surf()    const   { return (UVsurface*) _child; }

   //******** LOOKUP ********

   // Returns the UVsurface that owns the boss meme on the simplex:
   static UVsurface* find_owner(CBsimplex* s) {
      return upcast(Bbase::find_owner(s));
   }

   // Find the UVsurface owner of this simplex, or if there is none,
   // find the UVsurface owner of its subdivision parent (if any),
   // continuing up the subdivision hierarchy until we find a
   // UVsurface owner or hit the top trying.
   static UVsurface* find_controller(CBsimplex* s) {
     return upcast(Bbase::find_controller(s));
   }

   //******** BUILDING ********

   // Create a surface of revolution from the given base curve,
   // central axis, and sweep line:
   static UVsurface* build_revolve(
      Bcurve* bcurve,              // base curve around around enclosed faces
      Map1D3D* axis,               // central axis (should be straight)
      CWpt_list& spts,             // profile of the revolution (sweep curve)
      CBface_list& enclosed_faces, // base region, interior of bcurve
      Bpoint_list& points,         // points created
      Bcurve_list& curves,         // curves created
      Bsurface_list& surfs,        // surfaces created
      MULTI_CMDptr cmd=0);

   //    d ---------- c                               
   //    |            |                                
   //    |            |                                
   //    |            |                                
   //    |            |                                
   //    |            |                                
   //    |            |                                
   //    a ---------- b 
   // 
   // Build a Coons patch from the 4 points.
   // 
   // XXX -- need to explain input requirements
   static UVsurface* build_coons_patch(
      Bpoint* a,                                       
      Bpoint* b,                                       
      Bpoint* c,                                       
      Bpoint* d,
      MULTI_CMDptr cmd=0);

   // Tessellate the surface regularly with the given
   // dimensions.  If 'do_cap' is true and the mapping has
   // cylindrical topology, cap off the ends:
   bool tessellate_rect(int rows, int columns, bool do_cap);

   //   v4 ---------- v3                              
   //    |            |                                
   //    |   ud---uc  |                                
   //    |    |   |   |                                
   //    |    |   |   |                                
   //    |   ua---ub  |                                
   //    |            |                                
   //   v1 ---------- v2
   // 
   //    Stitch it up at given uv-coords, return the inset quad:
   //
   Lface* inset_quad(Bvert* v1, Bvert* v2, Bvert* v3, Bvert* v4,
                     CUVpt& ua, mlib::CUVpt& ub, mlib::CUVpt& uc, mlib::CUVpt& ud);

   //******** Bbase VIRTUAL METHODS ********

   virtual void produce_child();

   virtual bool apply_xf(CWtransf& xf, CMOD& mod);

   //******** BMESHobs METHODS ********

   virtual void notify_xform(BMESH*, CWtransf&, CMOD&);

   //******** Bnode METHODS ********

   virtual Bnode_list inputs() const;

 protected:
   Map2D3D*     _map;   // The function for the surface

   //******** INTERNAL METHODS ********

   // Generate a row of vertices and their uv-coords:
   void build_row(double v, int num_cols, Bvert_list&, UVpt_list&,
                  Bvert* first_v=0, Bvert* last_v=0);

   // Just adds memes to the given vertices
   void add_memes(CBvert_list& verts, CUVpt_list& uvs);

   // Generate quads between 2 rows of vertices with given uv-coords:
   void build_band(CBvert_list&, CBvert_list&, CUVpt_list&, mlib::CUVpt_list&);

   // build a fan around the given vertex (diagram in uv_surface.C)
   void build_fan(Bvert*, CUVpt&, CBvert_list&, mlib::CUVpt_list&);
};
typedef const UVsurface CUVsurface;

/*****************************************************************
 * UVmeme:
 *
 *   Vert meme class that uses a Map2D3D to map uv-coordinates
 *   to 3D world-space points.
 *****************************************************************/
class UVmeme : public VertMeme {
 public:

   enum { REGULAR=0, POLE_U, POLE_V };

   //******** MANAGERS ********

   UVmeme(UVsurface* b, Lvert* v, CUVpt& uv, char pole=REGULAR);

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("UVmeme", UVmeme*, VertMeme, CSimplexData*);

   //******** ACCESSORS ********

   UVsurface*   uv_surf()       const { return (UVsurface*)bbase(); }
   Map2D3D*     map()           const { return uv_surf()->map(); }

   CUVpt&       uv()            const { return _uv; }
   bool         uv_valid()      const { return _uv_valid; }
   void         set_uv(CUVpt& uv);

   // Compute a delta in uv-space that best matches the
   // given world-space displacement.
   bool solve_delt(CWvec& w_delt, mlib::UVvec& uv_delt);

   // Re-set the uv coord to match the current vert loc:
   bool set_uv_from_loc();

   //******** VertMeme VIRTUAL METHODS ********

   // Generation of child memes
   virtual void copy_attribs_v(VertMeme*);
   virtual void copy_attribs_e(VertMeme*, VertMeme*);
   virtual void copy_attribs_q(VertMeme*, VertMeme*, VertMeme*, VertMeme*);

   // Compute where to move to a more "relaxed" position:
   virtual bool compute_delt();

   // Move to the previously computed relaxed position:
   virtual bool apply_delt();

   // Compute 3D vertex location using the surface map
   virtual CWpt& compute_update();

   // Called when the vertex location changed but this VertMeme
   // didn't cause it:
   virtual void vert_changed_externally();

 protected:
   bool         _uv_valid;      // true if _uv is correct
   UVpt         _uv;            // cached uv coord
   UVvec        _delt;          // computed offset to be applied
   double       _disp;          // displacement due to inflation/extrude
   char         _pole;          // status re: pole (FREE, POLE_U or POLE_V)

   //******** COMPUTING UV ********

   // retrieve uv-coords from the mesh triangles:
   bool lookup_uv();

   // Many ops require both a map and a valid uv-coord.
   // this checks for both, caches the uv coord and
   // returns the map:
   Map2D3D* get_set() {
      if (!(_uv_valid || lookup_uv()))
         return 0;
      return map();
   }

   //******** VertMeme VIRTUAL METHODS ********

   // Methods for generating vert memes in the child Bbase.
   // (See meme.H for more info):
   virtual VertMeme* _gen_child(Lvert*)                                  const;
   virtual VertMeme* _gen_child(Lvert*, VertMeme*)                       const;
   virtual VertMeme* _gen_child(Lvert*, VertMeme*, VertMeme*, VertMeme*) const;
};

inline UVmeme*
UVsurface::uvm(int i) const
{
   return (UVmeme*) _vmemes[i];
}

inline bool 
UVsurface::get_uv(CBvert* v, UVpt& uv) const
{
   UVmeme* m = UVmeme::upcast(find_meme(v));
   if (m && m->uv_valid()) {
      uv = m->uv();
      return 1;
   }
   return 0;
}

inline bool 
UVsurface::get_uv(CBface* f, UVpt& uv) const
{
   if (!f)
      return 0;
   if (get_uv(f->v1(), uv))
      return 1;
   if (get_uv(f->v2(), uv))
      return 1;
   if (get_uv(f->v3(), uv))
      return 1;
   return 0;
}

#endif // UV_SURFACE_H_IS_INCLUDED

/* end of file UV_SURFACE.H */
