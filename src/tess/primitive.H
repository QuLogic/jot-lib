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
 * primitive.H
 *****************************************************************/
#ifndef PRIMITIVE_H_IS_INCLUDED
#define PRIMITIVE_H_IS_INCLUDED

/*!
 *  \file primitive.H
 *  \brief Contains the declaration of the Primitive class.
 *
 *  Contains the Primitive class, as well as the static methods
 *  for creating new Primitives.
 *  \sa primitive.C
 *
 */

#include "geom/command.H"
#include "mesh/simplex_frame.H"

#include "tess/bsurface.H"

class XFMeme;
class Pcalc;
/*******************************************************
 * Primitive
 *
 *   inflatable primitive
 *******************************************************/
class Primitive;
typedef const Primitive CPrimitive;
class Primitive :  public Bsurface {
 public:

   //******** PUBLIC INTERFACE FOR CREATING NEW PRIMITIVES ********

   //! Define a new branch of the Primitive from the given pixel
   //! trail, starting at base1 and ending at base2. The screen
   //! points will be projected to 3D in the plane defined by the
   //! given normal vector n, and containing the centroid of the
   //! first base, and possibly the second base, if it is not null.
   static Primitive* extend_branch(
      PIXEL_list,
      CWvec&,
      Bface_list,
      Bface_list,
      MULTI_CMDptr cmd=0
      );

   //! to create a roof based on the input parameters
   static Primitive* build_roof(
      PIXEL_list,
      ARRAY<int>,
      CWvec&,
      CBface_list&,
      CEdgeStrip&,
      MULTI_CMDptr cmd=0
      );

   //! If conditions are favorable, create a ball primitive with given
   //! screen-space radius (in PIXELs) around an isolated skeleton point:
   static Primitive* create_ball(
      Bpoint* skel, double pix_rad, MULTI_CMDptr cmd=0
      );

   static Primitive* build_simple_tube(
      LMESHptr     mesh,
      Bpoint*      b1,
      double       rad,
      CWvec&       n,
      CPIXEL_list& stroke,
      MULTI_CMDptr cmd     // command list for undo/redo
      );

   static Primitive* init(
      LMESH*       skel_mesh,
      Wpt_list     pts,
      CWvec&       n,
      MULTI_CMDptr cmd
      );

   virtual ~Primitive();

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("Primitive", Primitive*, Bsurface, CBnode*);

   //******** SKELETON POINTS AND CURVES ********

   CBpoint_list& skel_points() const { return _skel_points; }
   CBcurve_list& skel_curves() const { return _skel_curves; }

   //! Return the existing skeleton point that is closest to
   //! the given screen point p, and within radius r:
   Bpoint* find_skel_point(CPIXEL& p, double rad) const;

   //! Return the existing skeleton curve that most closely
   //! matches the given screen-space curve, provided the average
   //! distance between them does not exceed the given error
   //! threshold.  The distance is measured between the given
   //! curve and its "projection" on the skeleton curve.
   Bcurve* find_skel_curve(CPIXEL_list& crv, double err_thresh) const;

   bool is_ball() const;
   bool is_tube() const;
   bool is_roof() const;

   // create xfmemes for verts
   XFMeme* create_xf_meme(Lvert* vert, CoordFrame* frame);

   // absorb, XXX - protected?
   void absorb_skel(Bsurface* s);
   
   // XXX - protected?
   void finish_build(MULTI_CMDptr cmd);
   void add_input(Bnode* n) { _other_inputs.add_uniquely(n); }

   //******** STATICS ********

   static Primitive* find_owner(Bsimplex* s) {
      return upcast(Bbase::find_owner(s));
   }
   static Primitive* find_controller(CBsimplex* s) {
      return upcast(Bbase::find_controller(s));
   }
   //! Returns the set of Primitives owning any of the given faces:
   static Bsurface_list get_primitives(CBface_list& faces) {
      Bsurface_list ret = Bsurface::get_surfaces(faces);
      for (int i=ret.num()-1; i>=0; i--)
         if (!isa(ret[i]))
            ret.remove(i);
      return ret;
   }


   //******** Bsurface METHODS ********

   virtual void hide();
   virtual void show();

   //******** Bnode METHODS ********
   virtual Bnode_list inputs() const;

   //******** RefImageClient METHODS
   virtual int draw(CVIEWptr&);
   virtual int draw_vis_ref();

 protected:
   //******** INTERNAL DATA ********
   LMESHptr     _skel_mesh;
   Bpoint_list  _skel_points;
   Bcurve_list  _skel_curves;
   Bsurface_list _skel_surfaces;
   Bnode_list   _other_inputs;
   Bface_list   _base1; // optional attachment ...
   Bface_list   _base2; //              ... regions

   //******** MANAGERS ********

 public:
   //! Creates a blank Primitve for given mesh and skeleton mesh:
   Primitive(LMESH* mesh, LMESH* skel);
 protected:

   //******** BUILDING ********

   // Protected methods used by the public create methods
   // for building Primitives

   bool build_ball(Bpoint* skel, double pix_rad);

   bool build_simple_tube(
      Bpoint*           b1,
      double            w,
      CWvec&            n,
      CWpt_list&        pts,
      MULTI_CMDptr      cmd
      );

   bool extend(
      CPIXEL_list&      pixels,
      CBface_list&      b1,
      CBface_list&      b2,
      CWvec&            n,
      MULTI_CMDptr      cmd
      );

   bool extend(
      CPIXEL_list&      pixels,
      ARRAY<int>&       corners,
      CBface_list&      bases,
      CEdgeStrip&       side,
      CWvec&            n,
      MULTI_CMDptr      cmd
      );

   void build_ring(CoordFrame* frame, CWpt_list& locals, Bvert_list& ret);
   void build_band(CBvert_list& p, CBvert_list& c,
                   double du, double vp, double vc);

   void absorb_skel(Bcurve* c);
   void absorb_skel(Bpoint* p);

   static Primitive* get_primitive(BMESH*);

   //! The following came from splitting up a large function in a big
   //! hurry in order to be able to use parts of it in a new context...
   //! it sux, should be a lot cleaner:

   void create_skel_curve(
      const Pcalc& pcalc,
      param_list_t& tvals,
      CWpt_list& pts,
      Bpoint* bp1,
      Bpoint* bp2,
      CWvec& n,
      Bcurve*& skel_curve,
      Bedge_list& edges,
      MULTI_CMDptr cmd
      );

   Bface_list build_cap(Bvert* a, Bvert* b, Bvert* c, Bvert* d, double du=0.25);

   void build_tube(
      CoordFrame* f1,      //!< frame at 1st end
      CoordFrame* f2,      //!< frame at last end
      Bvert_list& p1,      //!< 1st ring
      Bvert_list& p2,      //!< last ring
      CWpt_list&  u1,      //!< local coords at 1st end
      CWpt_list&  u2,      //!< local coords at last end
      CBvert_list& v1,
      CBvert_list& v2,
      double du,
      const Pcalc& pcalc,  //
      double L,
      CBedge_list& edges,  //!< edges of skel curve
      CWvec& n,
      Bcurve* skel_curve,
      Bpoint* skel_point,
      Bface_list& cap1,
      Bface_list& cap2,
      bool sleeve_needed,
      MULTI_CMDptr cmd
      );

   bool create_wafer(CWpt_list& pts, Bpoint* skel);
   void create_xf_memes(CBvert_list& verts, CoordFrame* f);
};

#endif // PRIMITIVE_H_IS_INCLUDED

// end of file primitive.H
