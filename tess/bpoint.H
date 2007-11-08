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
 * bpoint.H
 *****************************************************************/
#ifndef BPOINT_H_IS_INCLUDED
#define BPOINT_H_IS_INCLUDED

#include "map3d/map0d3d.H"
#include "map3d/map2d3d.H"
#include "mesh/vert_frame.H"

#include "tess/bbase.H"

class Bpoint;
class Bcurve;
class Bsurface;

class Bpoint_list;
class Bcurve_list;
class Bsurface_list;

/*****************************************************************
 * PointMeme:
 *
 *   Data stored on a Bvert by a Bpoint.
 *   Can be used to look up the Bpoint, 
 *   given the Bvert.
 *****************************************************************/
class PointMeme : public VertMeme {
 public:

   //******** MANAGERS ********

   PointMeme(Bpoint* b, Lvert* v);

   virtual ~PointMeme() {}

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("PointMeme", PointMeme*, VertMeme, CSimplexData*);

   //******** ACCESSORS ********

   Bpoint*  point()     const   { return (Bpoint*) bbase(); }
   Map0D3D* map()       const;

   //******** VertMeme METHODS ********

   virtual CWpt& compute_update();

   // Called when the vertex location changed but this meme
   // didn't cause it:
   virtual void vert_changed_externally();

   //******** SimplexData NOTIFICATION METHODS ********

   virtual bool handle_subdiv_calc();
};

/*****************************************************************
 * Bpoint:
 *
 *      Implementation of a "point" object that can be connected
 *      to a network of curves (Bcurves) embedded in
 *      surfaces. Points can be directly moved by the user;
 *      curves can be over-sketched.  Moving a point affects
 *      adjacent curves and the surfaces they are embedded
 *      in. The resulting chain of dependencies is managed by
 *      deriving points, curves and surfaces from Bnodes.
 *
 *      The actual location of the point and its connectivity is
 *      recorded in the Bvert (a vertex of a mesh). Curves are
 *      represented via chains of mesh edges and vertices, with
 *      subidivision providing the smoothing.
 *****************************************************************/
typedef const Bpoint CBpoint;
class Bpoint : public Bbase{
 public:
   //******** MANAGERS ********

   // preferred method:
   Bpoint(CLMESHptr& mesh, Map0D3D* map,                     int res_lev=0);

   // to be phased out
   Bpoint(CLMESHptr& mesh, CWpt& o,      CWvec& n=Wvec::Y(), int res_lev=0);
   Bpoint(CLMESHptr& mesh, Map2D3D* map, CUVpt& uv,          int res_lev=0);   
   Bpoint(Lvert* vert,     CUVpt& uv,    Map2D3D* map,       int res_lev=0);
   Bpoint(Lvert* vert, double& t, Map1D3D* map, int res_lev=0);
   Bpoint(Lvert* vert, int res_lev=0);

   virtual ~Bpoint();

   DEFINE_RTTI_METHODS3("Bpoint", Bpoint*, Bbase, CBnode*);

   //******** ACCESSORS ********

   Lvert* vert() const {
      return _strip.empty() ? 0 : (Lvert*) _strip.vert(0);
   }
   CWpt&  loc()  const { return vert()->loc(); }
   NDCZpt ndc()  const { return vert()->ndc(); }

   Map0D3D*  map()      const { return _map; }
   Wvec      norm()     const;
   Wvec      tan()      const;
   Wvec      binorm()   const;
   Wtransf   frame()    const;
   Wplane    plane()    const { return Wplane(loc(), norm()); }
   Wline     line()     const { return Wline (loc(), norm()); }

   Wpt      o() { return map()->o(); }
   Wvec     t() { return map()->t(); }
   Wvec     b() { return map()->b(); }
   Wvec     n() { return map()->n(); }

   bool has_shadow()    const { return _shadow_plane.is_valid(); }

   //******** UTILITY METHODS ********

   // Point size normally used in rendering:
   static double standard_size();

   //******** SUBDIVISION BPOINTS ********

   // XXX - not actually used yet.

   // Bpoint at top of hierarchy:
   Bpoint* ctrl_point()         const   { return (Bpoint*) control(); }

   // The subdiv parent and child points:
   Bpoint* parent_point()        const   { return (Bpoint*) _parent; }
   Bpoint* child_point()         const   { return (Bpoint*) _child; }

   // Set the "resolution level":
   virtual void set_res_level(int r);

   //******** ADJACENCY ********

   // for each edge adjacent to the vertex of the Bpoint,
   // looks for a Bcurve (maybe at another subdivision level)
   // that controls the edge; returns all such Bcurves in a list:
   // XXX - policy re: subdivision levels may change.
   Bcurve_list    adjacent_curves()     const;

   Bcurve*        other_curve(Bcurve*)  const;
   Bcurve*        lookup_curve(CBpoint*)const;

   //******** MOVING ********

   bool move_to(CWpt& p);       // sets the location
   bool move_to(CXYpt& x);
   bool move_by(CWvec& delt) { return move_to(loc() + delt); }

   //******** Shadow Creation ********
   void set_shadow_plane(CWplane& P) { _shadow_plane = P; }
   void remove_shadow()              { set_shadow_plane(Wplane()); }

   // If this point is constrained by a surface,
   // return the map2D3D for the surface, (otherwise NULL):
   Map2D3D* constraining_surface() const {
      CSurfacePtMap* sm = SurfacePtMap::upcast(_map);
      return sm ? sm->surface() : 0;
   }
   // Like above, but upcast to PlaneMap if it is one:
   PlaneMap* constraining_plane() const {
      return PlaneMap::upcast(constraining_surface());
   }

   void set_map(Map0D3D* map, bool update_curves=true);
   void remove_constraining_surface();

   //******** NOTIFICATION ********

   void notify_vert_xformed(CWtransf& xf);
   void notify_vert_deleted(PointMeme* vd);

   //******** LOOKUP/INTERSECTION ********

   // Returns the Bpoint that owns the boss meme on the simplex:
   static Bpoint* find_owner(CBsimplex* s) {
      return upcast(Bbase::find_owner(s));
   }

   // Find the Bpoint owner of this simplex, or if there is none,
   // find the Bpoint owner of its subdivision parent (if any),
   // continuing up the subdivision hierarchy until we find a
   // Bpoint owner or hit the top trying.
   static Bpoint* find_controller(CBsimplex* s) {
     return upcast(Bbase::find_controller(s));
   }

   // XXX -- obsolete?
   static Bpoint* lookup(CBvert* v) { return find_controller(v); }

   static Bpoint_list get_points(CBvert_list& verts);

   // given screen-space point p and search radius (in pixels),
   // return the Bpoint closest to p within the search radius (if any).
   // in case of success, also fills in the Wpt that was hit, and
   // (if non-null) the hit edge:
   static Bpoint* hit_point(CNDCpt& p, double radius=8);
   static Bpoint* hit_point(CNDCpt& p, double radius, PIXEL &hit) {
      Bpoint* ret = hit_point(p, radius);
      if (ret)
         hit = ret->loc();
      return ret;
   }
   static Bpoint* hit_ctrl_point(CNDCpt& p, double rad=8) {
      Bpoint* ret = hit_point(p, rad);
      return ret ? ret->ctrl_point() : ret;
   }

   //******** Bbase METHODS ********

   // Delete the vertex
   virtual void delete_elements();

   // Bpoints do not currently do subdivision
   virtual void notify_subdiv_gen(BMESH*) {}

   // The vertex moved:
   virtual void notify_vert_changed(VertMeme*);
   
   virtual void draw_debug();

   virtual bool apply_xf(CWtransf& xf, CMOD& mod);

   //******** SELECTION ********

   static Bpoint_list   selected_points();
   static Bpoint*       selected_point();

   virtual void set_selected();
   virtual void unselect();

   //******** BMESHobs METHODS ********

   virtual void notify_xform(BMESH*, CWtransf&, CMOD&);

   //******** Bnode METHODS ********
   virtual Bnode_list inputs() const;

   //******** SELECTION / TIMEOUT / COLOR ********
   virtual CCOLOR& selection_color() const;
   virtual CCOLOR& regular_color()   const;

   //******** RefImageClient METHODS
   // draw to the screen:
   virtual int draw(CVIEWptr&);

   //******** RefImageClient METHODS ********
   // draw to the visibility reference image (for picking)
   virtual int draw_vis_ref();
	
 //*******************************************************
 // PROTECTED
 //*******************************************************
 protected:
   Wplane       _shadow_plane;
   LvertStrip   _strip;         // for rendering the vertex
   Map0D3D*     _map;           // mapping describing location
 
   //******** PROTECTED METHODS ********

   // Common setup functionality used by both constructors:
   void setup(int res_lev);

   PointMeme* get_meme() const { return (PointMeme*)find_vert_meme(vert());}

   //******** Bnode METHODS ********
   virtual void recompute();	

   // ******** I/O  ********
   static TAGlist*      _bpoint_tags;

   //******** UTILITIES ********

   bool should_draw() const;
   void draw_axes();
};

inline Map0D3D*
PointMeme::map() const { return point()->map(); }

/*******************************************************
 * Bpoint_list
 *******************************************************/
class Bpoint_list : public _Bbase_list<Bpoint_list,Bpoint> {
 public:
   //******** MANAGERS ********
   Bpoint_list(int n=0) :
      _Bbase_list<Bpoint_list,Bpoint>(n)    {}
   Bpoint_list(const Bpoint_list& list) :
      _Bbase_list<Bpoint_list,Bpoint>(list) {}
   Bpoint_list(Bpoint* p) :
      _Bbase_list<Bpoint_list,Bpoint>(p)    {}

   Bpoint_list(CBbase_list& bbases) :
      _Bbase_list<Bpoint_list,Bpoint>(bbases.num()) {
      for (int i=0; i<bbases.num(); i++) {
         Bpoint* p = Bpoint::upcast(bbases[i]);
         if (p)
            *this += p;
      }
   }
   
   //******** CONVENIENCE ********

   Bpoint_list child_points() const {
      Bpoint_list ret;
      for (int i=0; i<_num; i++) {
         Bpoint* c = _array[i]->child_point();
         if (c)
            ret += c;
      }
      return ret;
   }

   Bvert_list get_verts() const {
      Bvert_list ret(num());
      for (int i=0; i<num(); i++)
         ret += _array[i]->vert();
      return ret;
   }

   //******** GRAPH SEARCHES ********
   static Bpoint_list reachable_points(Bpoint* b);

 protected:

   //******** GRAPH SEARCHES ********
   void grow_connected(Bpoint* b, Bpoint_list& ret);
};
typedef const Bpoint_list CBpoint_list;


/*****************************************************************
 * Bpoint inline methods
 *****************************************************************/
inline Bpoint_list
Bpoint::get_points(CBvert_list& verts)
{
   Bpoint_list ret;
   Bpoint* bp = 0;
   for (int i=0; i<verts.num(); i++)
      if ((bp = lookup(verts[i])))
         ret += bp;
   return ret;
}

/************************************************************
 * picking
 ************************************************************/
class BpointFilter : public SimplexFilter {
 public:
   //******** SOLE JOB ********
   virtual bool accept(CBsimplex* s) const {
      return is_vert(s) && Bpoint::lookup((Bvert*)s) != 0;
   }
};

/*****************************************************************
 * BpointFrame:
 *
 *    Used in primitive.C to let a skeleton Bpoint control
 *    a Primitive surface. We want the coordinate frame of
 *    the skeleton point to influence the surface.
 *
 *****************************************************************/
class BpointFrame : public VertFrame {
 public:

   //******** MANAGERS ********

   // The Bpoint or its map cannot be NULL:
   BpointFrame(uint key, Bpoint* p) :
      VertFrame(key, p->vert()), _bp(p) {}

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("BpointFrame", BpointFrame*, VertFrame, CSimplexData*);

   //******** CoordFrame VIRTUAL METHODS ******** 

   virtual Wpt      o() { return _bp->map()->o(); }
   virtual Wvec     t() { return _bp->map()->t(); }
   virtual Wvec     n() { return _bp->map()->n(); }

   virtual Wvec      b() { return CoordFrame::b(); }
   virtual Wtransf  xf() { return CoordFrame::xf(); }
   virtual Wtransf inv() { return CoordFrame::inv(); }

 protected:
   Bpoint*      _bp;
};
typedef const BpointFrame CBpointFrame;

/*****************************************************************
 * BsimplexMap (limited usage):
 *
 *   maps to a simplex in R^3.
 *****************************************************************/
class BsimplexMap : public Map0D3D {
 public:

   //******** MANAGERS ********

   BsimplexMap(Bsimplex* s, mlib::CWvec& bc=mlib::Wvec::null(), 
      mlib::CWvec& n=mlib::Wvec::null(), mlib::CWvec& t=mlib::Wvec::null()) :
      Map0D3D(n,t),
      _s(s), _bc(bc) {}

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("BsimplexMap", BsimplexMap*, Map0D3D, CBnode*);

   //******** VIRTUAL METHODS ********

   virtual mlib::Wpt  map() const   { return _s->bc2pos(_bc); }
   
   virtual bool set_pt(Bsimplex* s, mlib::CWvec& bc);
   virtual bool set_pt(mlib::CWpt& p);

   //******** MAP VIRTUAL METHODS ********

   virtual bool can_transform()         const    { return false; }
   virtual void transform(mlib::CWtransf& xf, CMOD& m) {}

   //******** Bnode VIRTUAL METHODS ********

   virtual Bnode_list inputs()  const { return Bnode_list(); }

 protected:
   
   Bsimplex*  _s;     // the simplex
   Wvec       _bc;    // the barycentric coordinate
};

#endif // BPOINT_H_IS_INCLUDED

// end of file bpoint.H
