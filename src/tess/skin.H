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
 * skin.H
 *****************************************************************/
#ifndef SKIN_H_IS_INCLUDED
#define SKIN_H_IS_INCLUDED

#include "geom/command.H"

#include "tess/bsurface.H"
#include "tess/vert_mapper.H"
#include "tess/ti.H"
using namespace tess;

class SkinMeme;
class SubdivUpdater;

class ProblemEdgeFilter : public SimplexFilter {
   InflateCreaseFilter _icf;
   BorderEdgeFilter    _bef;
   CreaseEdgeFilter    _cef;
   BcurveFilter        _bcf;
   MultiEdgeFilter     _mef;
 public:
   virtual bool accept(CBsimplex* s) const {
      return (_icf.accept(s) ||
              _bef.accept(s) ||
              _cef.accept(s) ||
              _bcf.accept(s) ||
              _mef.accept(s));
   }
};

class StrongNPrEdgeFilter : public SimplexFilter {
   ProblemEdgeFilter   _pef;
   StrongEdgeFilter    _sef;
 public:
   virtual bool accept(CBsimplex* s) const {
      return (_sef.accept(s) && !_pef.accept(s));
   }
};

class RestrictTrackerFilter : public SimplexFilter {
   ProblemEdgeFilter    _pef;
 public:
   virtual bool accept(CBsimplex* s) const {
      if (is_edge(s))
         return _pef.accept(s);
      if (is_vert(s))
         return ((Bvert*)s)->degree(_pef) == 2;
      return false;
   }
};

/*******************************************************
 * Skin
 *
 *   Bsurface that clings to one or more other surfaces.
 *******************************************************/
class Skin:  public Bsurface {
 public:

   //******** MANAGERS ********

   // no public constructor, use create methods

   // create a cover skin of a given region
   static Skin* create_cover_skin(Bface_list region, MULTI_CMDptr cmd);

   // create a "sleeve" joining two skeleton regions
   static Skin*  create_multi_sleeve(
      Bface_list interior,      // interior skeleton surface
      VertMapper skel_map,      // tells how some skel verts are identified
      MULTI_CMDptr cmd);

   // inflate a given face set on both sides and sew ribbons together
   // around it:
   static bool create_inflate(CBface_list& skel, double h, MULTI_CMDptr cmd, bool mode);

 public:

   virtual ~Skin();
   
   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("Skin", Skin*, Bsurface, CBnode*);

   //******** LOOKUP ********

   // Returns the Skin that owns the boss meme on the simplex:
   static Skin* find_owner(CBsimplex* s) {
      return upcast(Bbase::find_owner(s));
   }

   // Find the Skin owner of this simplex, or if there is none,
   // find the Skin owner of its subdivision parent (if any),
   // continuing up the subdivision hierarchy until we find a
   // Skin owner or hit the top trying.
   static Skin* find_controller(CBsimplex* s) {
     return upcast(Bbase::find_controller(s));
   }

   Skin* parent()  const { return upcast(Bbase::parent()); }
   Skin* child()   const { return upcast(Bbase::child()); }
   Skin* control() const { return upcast(Bbase::control()); }

   Skin* subdiv_skin(int k) const { return upcast(subdiv_bbase(k)); }

   //******** UTILITIES ********

   CBface_list& skel_faces()    const { return _skel_faces; }
   CBface_list& skin_faces()    const { return _patch->faces(); }

   CBvert_list skel_verts()     const { return skel_faces().get_verts(); }
   CBvert_list skin_verts()     const { return skin_faces().get_verts(); }

   CBedge_list skel_edges()     const { return skel_faces().get_edges(); }
   CBedge_list skin_edges()     const { return skin_faces().get_edges(); }

   VertMapper* get_mapper()     { return &_mapper; }
   VertMapper* get_inf_mapper() { return &_inf_mapper; }
   void set_mapper(VertMapper m){ _mapper = m;     }
   void set_inf_mapper(VertMapper m) { _inf_mapper = m; }
   SubdivUpdater* get_updater() { return _updater; }
   Skin* get_partner()          { return _partner; }
   ARRAY<Skin*> get_covers()    { return _covers;   }
   bool is_inflate()            { return _inflate; }

   void add_offsets(double dist);

   //******** DIAGNOSTIC ********

   static void debug_smoothing(int iters = 1);

   //******** Bbase VIRTUAL METHODS ********

   virtual void produce_child() {}

   virtual void draw_debug();

   //******** RefImageClient METHODS

   virtual int draw(CVIEWptr&);

   //******** Bnode METHODS ********

   virtual Bnode_list inputs() const;

 protected:

   Bface_list           _skel_faces;    // skel faces replicated by skin
   SubdivUpdater*       _updater;       // Bnode for skel faces
   VertMapper           _mapper;        // maps skel verts to skin verts
   VertMapper           _inf_mapper;    // auxilary mapper for inflation
   Skin*                _partner;       // inflate partner
   ARRAY<Skin*>         _covers;        // cover skins
   bool                 _reverse;       // if true, normals are reversed from skels
   bool                 _inflate;       // if true, this is an inflation

   static Skin*         _debug_instance;// used in debugging

   //******** MANAGERS ********

   // Create top level Skin:
   Skin(LMESHptr,
        CBface_list& skel_faces,
        CVertMapper& skel_map,
        int res_level,
        bool reverse,
        Cstr_ptr& name,
        MULTI_CMDptr cmd);

   // Create a child Skin of the given parent:
   Skin(Skin* parent, CBface_list& skel_faces, MULTI_CMDptr cmd);

   // Tasks common to both constructors
   void finish_ctor(MULTI_CMDptr);
   void create_subdiv_updater();

   // Replicate skel verts to create new skin verts
   bool gen_verts(CBvert_list& skel_verts);

   // Like above, but take into account skel verts that are
   // "identified" together. I.e. if s1 and s2 are skel verts that are
   // identified together, we create just a single skin vert
   // corresponding to both s1 and s2.
   bool gen_verts(CBvert_list& skel_verts, CVertMapper& skel_mapper);

   // Replicate skel faces to create new skin faces.
   // skel_mapper tells how some skel vertices are identified
   // (to each other).
   bool gen_faces(CVertMapper& skel_mapper=VertMapper());

   // Replicate skel vert to create new skin vert
   Lvert* gen_vert(Bvert* v);

   // Replicate skel face to create new skin face
   Lface* gen_face(Bface* f);

   // Copy edge attributes (weak/strong) from skel to skin
   bool copy_edges(CBedge_list& edges)  const;
   bool copy_edge(Bedge* a)             const;

   // Do correction at boundary to allow joining
   // to underlying skeleton surfaces:
   bool do_boundary_correction();
   bool correct_face(Bface*, bool& changed);

   // Attach to skel surfaces:
   bool join_to_skel(CBface_list& skel_region, MULTI_CMDptr cmd);

   // Make all skin memes sticky (or not):
   // returns count of number of successful tries
   int set_sticky(CBvert_list& skin_verts, bool sticky=true) const;
   int set_all_sticky(bool sticky=true) const;

   int set_offsets(CBvert_list& skin_verts, double offset) const;
   int set_all_offsets(double offset) const;

   void freeze(CBvert_list& skin_verts) const;
   void freeze_all() const { freeze(skin_verts()); }

   void  adjust_crease_offsets() const;

   void freeze_problem_verts(bool mode=false) const;

   void track_deeper(CBvert_list& verts, int R) const;

   void set_non_penetrate(CBvert_list& verts, bool b) const;
   void set_stay_outside (CBvert_list& verts, bool b) const;

   void restrict(CBvert_list& verts, SimplexFilter* f) const;

   Wpt_list     track_points(CBvert_list& verts) const;
   Bvert_list   sticky_verts(CBvert_list& verts) const;
   Bvert_list   frozen_verts(CBvert_list& verts) const;
   Bvert_list unfrozen_verts(CBvert_list& verts) const;

   void set_partner(Skin* partner);

   // create one side of a two sided inflate
   static Skin* create_inflate(
      CBface_list& skel, double h, int R, bool reverse, bool mode, MULTI_CMDptr cmd
      );
};

/*****************************************************************
 * SkinCurveMap:
 *
 *   Represents a curve embedded in a skin. Defined by an
 *   array of (pair of simplex/barycentric coordinates). 
 *
 *****************************************************************/
class SkinCurveMap : public Map1D3D {
 public:
   //******** MANAGERS ********

   // Constructor only takes an array of simplices and bcs
   // if the curve is not closed, p0 and p1 have to be not null
   SkinCurveMap(Bsimplex_list& simps, ARRAY<Wvec>& bcs, Skin* skin,
      Map0D3D* p0 = 0, Map0D3D* p1 = 0);

   virtual ~SkinCurveMap() { unhook(); } // undo dependencies

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("SkinCurveMap", SkinCurveMap*, Map1D3D, CBnode*);

   //******** ACCESSORS ********
   
   void          set_pts(CBsimplex_list& simps, ARRAY<Wvec>& bcs);

   //******** Map1D3D VIRTUAL METHODS ********

   // Is the curve a closed loop? 
   virtual bool is_closed()  const { 
      return get_wpts().is_closed(); 
   }

   // Total length of curve:
   virtual double length()   const;

   // Point on curve at parameter value t:
   virtual mlib::Wpt map(double t) const;

   // Derivative at parameter value t.  Base class does
   // numerical derivative and treates the parameter as
   // valid outside [0,1].  Since we clamp the parameter to
   // [0,1], the derivative is the null vector when t is
   // outside [0,1] (for non-closed curves).
   virtual mlib::Wvec deriv(double t) const {
      if (!is_closed() && (t < 0 || t > 1))
          return mlib::Wvec::null();
      return Map1D3D::deriv(t);
   }

   // The "planck length" for this curve; i.e., the step size
   // below which you'd just be sampling the noise:
   virtual double hlen() const;

   // Tells how many "samples" are used to describe the curve.
   virtual int nsamples()       const { return _simps.num(); }

   // Return a Wpt_list describing the current shape of the map
   virtual mlib::Wpt_list get_wpts() const;

   //******** LOCAL COORD FRAME ********

   // The "normal" direction at each parameter value is the
   // surface normal.
   virtual bool is_oriented()   const { return true; }

   // The local unit-length normal:
   virtual mlib::Wvec norm(double t) const;

   virtual void set_norm(mlib::CWvec&) {}     // ignore

   //******** Bnode VIRTUAL METHODS ********

   // The endpoints are the surface, and endpoints if any:
   virtual Bnode_list inputs()  const {
      Bnode_list ret = Map1D3D::inputs();
      ret += (Bnode*)_skin->get_updater();
      return ret;
   }

 protected:
   Bsimplex_list  _simps;  // simplices that the curve lives in
   ARRAY<Wvec>    _bcs;   // barycentric coordinates
   Skin*          _skin;

   //******** PROTECTED METHODS ********

   // The base class allows parameters outside the range [0,1],
   // but not us. If the parameter is outside get it back in:
   double fix(double t) const {
      return is_closed() ? WrapCoord::N(t) : clamp(t, 0.0, 1.0);
   }

   //******** Bnode VIRTUAL METHODS ********

   // After endpoints change, re-adjust curve to match endpoints:
   virtual void recompute();
};

#endif // SKIN_H_IS_INCLUDED

// end of file skin.H
