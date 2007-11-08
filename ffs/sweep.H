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
 * sweep.H
 *****************************************************************/
#ifndef SWEEP_H_IS_INCLUDED
#define SWEEP_H_IS_INCLUDED

/*!
 *  \file sweep.H
 *  \brief Contains the declarations of SWEEP_BASE, SWEEP_POINT, SWEEP_LINE, and SWEP_DISK.
 *
 *  \ingroup group_FFS
 *  \sa sweep.C
 *
 */

#include "npr/stylized_line3d.H"
#include "gest/draw_widget.H"
#include "tess/panel.H"                 // Panel

/*****************************************************************
 * SWEEP_BASE:
 *
 *
 *****************************************************************/
MAKE_PTR_SUBC(SWEEP_BASE,DrawWidget);
typedef const SWEEP_BASE    CSWEEP_BASE;
typedef const SWEEP_BASEptr CSWEEP_BASEptr;

//!\brief Base class for widgets (SWEEP_POINT, SWEEP_LINE, SWEEP_DISK)
//! that handles sweeping out a shape to inflate a higher-
//! dimensional shape from a lower-dimensional one.
class SWEEP_BASE : public DrawWidget {
 public:

   // no public constructor

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("SWEEP_BASE", SWEEP_BASE*, DrawWidget, CDATA_ITEM*);

   //******** DrawWidget METHODS ********

   virtual BMESHptr bmesh() const { return _mesh; }
   virtual LMESH*   lmesh() const { return _mesh; }

   //******** GEL METHODS ******** 

   virtual int draw(CVIEWptr& v);
   virtual int draw_final(CVIEWptr& v); 
   virtual int draw_id_ref_pre1();
   virtual int draw_id_ref_pre2(); 
   virtual int draw_id_ref_pre3(); 
   virtual int draw_id_ref_pre4();
    
   virtual void request_ref_imgs();

 protected:

   //******** MEMBER DATA ********

   LMESHptr             _mesh;  //!< mesh of the curve
   StylizedLine3Dptr    _line;  //!< the guideline

   //******** MANAGERS ********

   SWEEP_BASE();
   virtual ~SWEEP_BASE() {}

   //******** DRAW FSA METHODS ********

   //! For defining callbacks to use with GESTUREs:
   typedef CallMeth_t<SWEEP_BASE, GESTUREptr> draw_cb_t;
   draw_cb_t* drawCB(draw_cb_t::_method m) {
      return new draw_cb_t(this, m);
   }

   //! Turn off the widget:
   virtual int  cancel_cb     (CGESTUREptr& gest, DrawState*&);

   //! Tap callback (usually cancel):
   virtual int  tap_cb        (CGESTUREptr& gest, DrawState*&);

   //! User drew a straight line. Decides appropriate callback:
   virtual int  line_cb       (CGESTUREptr& gest, DrawState*&);

   //! The straight line means extend the guideline, so do it:
   virtual int  extend_line_cb(CGESTUREptr& gest, DrawState*&);

   //! The straight line means trim the guideline, so do it:
   virtual int  trim_line_cb  (CGESTUREptr& gest, DrawState*&);

   //! Do a uniform sweep operation based on the given
   //! displacement along the guideline (from its origin):
   virtual bool do_uniform_sweep(mlib::CWvec& v) = 0;

   //! general stroke: ignored in base class
   virtual int  stroke_cb(CGESTUREptr&, DrawState*&) {
      return 1; // used up the gesture
   }

   //******** UTILITY METHODS ********

   /*!
		if mesh is null, returns false.
		otherwise records mesh, initializes guideline
		does other initialization, and returns true.
		'a' is start of guideline, 'b' is end;
		dur is duration of widget before timeout
    */
   bool setup(LMESHptr mesh, mlib::CWpt& a, mlib::CWpt& b, double dur);

   //! Convenience method for building the guideline:
   void build_line(mlib::CWpt& a, mlib::CWpt& b);

   //! Reset the endpoint of the guideline to match the given
   //! screen location:
   void reset_endpoint(mlib::CPIXEL& pix);

   //! Convenience method for converting the guideline to a Wline:
   mlib::Wline wline() const {
      assert(_line && _line->num() == 2);
      return mlib::Wline(_line->point(0), _line->point(1));
   }

   //! Convenience method for converting the guideline to a mlib::PIXELline:
   mlib::PIXELline pix_line() const {
      assert(_line && _line->num() == 2);
      return mlib::PIXELline(_line->point(0), _line->point(1));
   }

   //! Returns vector of the guideline
   mlib::Wvec sweep_vec() const {
      assert(_line && _line->num() == 2);
      return _line->point(1) - _line->point(0);
   }

   //! Returns origin of the guideline
   mlib::Wpt sweep_origin() const {
      assert(_line && _line->num() == 2);
      return _line->point(0);
   }

   //! Returns end point of the guideline
   mlib::Wpt sweep_end() const {
      assert(_line && _line->num() == 2);
      return _line->point(1);
   }

   //! project given mlib::PIXEL location onto guideline in 3D:
   mlib::Wpt project_to_guideline(mlib::CPIXEL& p) const {
      return wline().intersect(mlib::Wline(p));
   }

   //! Returns true if the gesture starts from the sweep origin
   bool from_center(CGESTUREptr& g) const;

   //! Returns true if p is on the axis line
   bool hits_line(mlib::CPIXEL& p) const;

   //******** DrawWidget METHODS ********

   //! Clear cached info when deactivated:
   virtual void reset();

   //******** MODE NAME ********

   // displayed in jot window
   virtual str_ptr      mode_name()     const { return "revolve"; }
};

/*****************************************************************
 * SWEEP_DISK:
 *****************************************************************/
MAKE_PTR_SUBC(SWEEP_DISK,SWEEP_BASE);
typedef const SWEEP_DISK    CSWEEP_DISK;
typedef const SWEEP_DISKptr CSWEEP_DISKptr;

class UVsurface;

//! \brief Widget that handles sweeping out a disk to create a surface
//! of revolution.
class SWEEP_DISK : public SWEEP_BASE {
 public:

   //******** MANAGERS ********

   // no public constructor
   static SWEEP_DISKptr get_instance();

   static SWEEP_DISKptr get_active_instance() { return upcast(_active); }

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("SWEEP_DISK", SWEEP_DISK*, SWEEP_BASE, CDATA_ITEM*);

   //******** SETTING UP ********

   //! If it is a delayed slash from the center of a disk,
   //! activate the widget and return true.
   static bool init(CGESTUREptr& gest) {
      return get_instance()->setup(gest, default_timeout());
   }

   //! Being re-activated
   static bool init(Panel* p, Bpoint_list points, Bcurve_list curves, Bsurface_list surfs, Wpt_list profile) {
      return get_instance()->setup(p, points, curves, surfs, profile);
   }

   //******** GEL METHODS ******** 

   virtual int draw(CVIEWptr& v);

   //******** FOR RETURNING TO THIS MODE ****
   static ARRAY<Panel*> panels;
   static ARRAY<Bpoint_list> bpoints;
   static ARRAY<Bcurve_list> bcurves;
   static ARRAY<Bsurface_list> bsurfaces;
   static ARRAY<Wpt_list> profiles;

 protected:

   //******** MEMBER DATA ********

   Bface_list   _enclosed_faces;        //!< faces "inside" the boundary
   LedgeStrip   _boundary;              //!< boundary around faces
   mlib::Wplane       _plane;            //!< best fit plane computed from boundary

   // data for deactivation
   Bpoint_list _points;
   Bcurve_list _curves;
   Bsurface_list _surfs;
   Wpt_list _profile;

   static SWEEP_DISKptr _instance;      //!< the only one we need

   static void clean_on_exit();

   //******** MANAGERS ********

   SWEEP_DISK();
   virtual ~SWEEP_DISK() {}

   //******** DRAW FSA METHODS ********

   //! For defining callbacks to use with GESTUREs:
   typedef CallMeth_t<SWEEP_DISK, GESTUREptr> draw_cb_t;
   draw_cb_t* drawCB(draw_cb_t::_method m) {
      return new draw_cb_t(this, m);
   }

   //! Generic stroke. Could be a few things, e.g. non-uniform sweep:
   virtual int  stroke_cb(CGESTUREptr& gest, DrawState*&);

   //! Do a uniform sweep operation based on the given
   //! displacement along the guideline (from its origin):
   virtual bool do_uniform_sweep(mlib::CWvec& v);

   //******** UTILITY METHODS ********

   //! Check conditions, cache info, activate:
   bool setup(CGESTUREptr& gest, double dur);

   //! Being re-activated
   bool setup(Panel* p, Bpoint_list points, Bcurve_list curves, Bsurface_list surfs, Wpt_list profile);

   //! Returns true if the given pixel location is near a "fold point"
   bool hit_boundary_at_sil(mlib::CPIXEL& p, mlib::Wpt& ret) const;

   mlib::Wpt_list get_fold_pts() const;

   //******** GEOMETRY CREATION ********

   //! Perform the sweep operation
   bool do_sweep(CPIXEL_list& g, mlib::CWpt &sil_hit, bool is_line, bool from_center);
   bool do_sweep(mlib::CWpt& o, mlib::Wvec t, mlib::Wvec n, mlib::Wvec b, mlib::Wpt_list spts);

   //! base region has corners; build a "box"
   bool build_box(mlib::CWpt& o, mlib::CWvec& t, mlib::CWpt_list& spts, MULTI_CMDptr cmd=0);

   //! base region has no corners; build a "revolve"
   UVsurface* build_revolve(
      mlib::CWpt_list&        apts,           //!< axis points
      mlib::CWvec&            n,              //!< normal to axis points
      mlib::CWpt_list&        spts,           //!< sweep points
      MULTI_CMDptr      cmd=0           //!< holder for commands generated
      );

   //******** DrawWidget METHODS ********

   //! Clear cached info when deactivated:
   virtual void reset();
};

/*****************************************************************
 * SWEEP_LINE:
 *****************************************************************/
MAKE_PTR_SUBC(SWEEP_LINE,SWEEP_BASE);
typedef const SWEEP_LINE    CSWEEP_LINE;
typedef const SWEEP_LINEptr CSWEEP_LINEptr;

class Bcurve;

//! \brief Widget that handles sweeping out a line to create a
//! rectangle, or a ribbon-like flat surface.
class SWEEP_LINE : public SWEEP_BASE {
 public:

   //******** MANAGERS ********

   // no public constructor
   static SWEEP_LINEptr get_instance();

   static SWEEP_LINEptr get_active_instance() { return upcast(_active); }

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("SWEEP_LINE", SWEEP_LINE*, SWEEP_BASE, CDATA_ITEM*);

   //******** SETTING UP ********

   //! Given an initial slash gesture (or delayed slash) near the
   //! center of an existing straight Bcurve, set up the widget to
   //! do a sweep cross-ways to the Bcurve:
   static bool init(CGESTUREptr& gest) {
      return get_instance()->setup(gest, default_timeout());
   }

   //******** GEL METHODS ******** 

   virtual int draw(CVIEWptr& v);

 protected:

   //******** MEMBER DATA ********

   Bcurve*      _curve;                 //!< straight "curve" to sweep sideways
   mlib::Wplane       _plane;                 //!< plane to sweep in (contains curve)

   static SWEEP_LINEptr _instance;      //!< only one instance

   static void clean_on_exit();

   //******** MANAGERS ********

   SWEEP_LINE();
   virtual ~SWEEP_LINE() {}

   //******** DRAW FSA METHODS ********

   //! For defining callbacks to use with GESTUREs:
   typedef CallMeth_t<SWEEP_LINE, GESTUREptr> draw_cb_t;
   draw_cb_t* drawCB(draw_cb_t::_method m) {
      return new draw_cb_t(this, m);
   }

   //! Do a uniform sweep operation based on the given
   //! displacement along the guideline (from its origin):
   virtual bool do_uniform_sweep(mlib::CWvec& v);

   //! Generic stroke. Sweep out a riboon-like surface
   virtual int stroke_cb(CGESTUREptr& gest, DrawState*&);

   //******** UTILITY METHODS ********

   //! Check conditions, cache info, activate:
   bool setup(CGESTUREptr& gest, double dur);

   //******** GEOMETRY CREATION ********

   //! sweep a straight line crossways in a straight line to create a rectangle
   bool create_rect(CGESTUREptr& g);

   //! create a rectangular Panel based on given vector along the guideline
   bool create_rect(mlib::CWvec& v);

   //! sweep a straight line crossways along a curve to create a "ribbon"
   bool create_ribbon(CGESTUREptr& g);

   //******** DrawWidget METHODS ********

   //! Clear cached info when deactivated:
   virtual void reset();
};

/*****************************************************************
 * SWEEP_POINT:
 *****************************************************************/
MAKE_PTR_SUBC(SWEEP_POINT,SWEEP_BASE);
typedef const SWEEP_POINT    CSWEEP_POINT;
typedef const SWEEP_POINTptr CSWEEP_POINTptr;

//! \brief Widget that handles sweeping out a point to create a
//! straight line (Bcurve)
class SWEEP_POINT : public SWEEP_BASE {
 public:

   //******** MANAGERS ********

   // no public constructor
   static SWEEP_POINTptr get_instance();

   static SWEEP_POINTptr get_active_instance() { return upcast(_active); }

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("SWEEP_POINT", SWEEP_POINT*, SWEEP_BASE, CDATA_ITEM*);

   //******** SETTING UP ********

   //! Given an initial slash gesture (or delayed slash) near the
   //! center of an existing straight Bcurve, set up the widget to
   //! do a sweep cross-ways to the Bcurve:
   static bool turn_on(CGESTUREptr& gest, double dur = default_timeout()) {
      return get_instance()->setup(gest, dur);
   }

   //******** GEL METHODS ******** 

   virtual int draw(CVIEWptr& v);

 protected:

   //******** MEMBER DATA ********

   Bpoint*      _point;                 //!< optional point to sweep into a line
   mlib::Wplane       _plane;                 //!< plane to sweep in (contains point)

   static SWEEP_POINTptr _instance;      //!< only one instance

   static void clean_on_exit();

   //******** MANAGERS ********

   SWEEP_POINT();
   virtual ~SWEEP_POINT() {}

   //******** DRAW FSA METHODS ********

   //! For defining callbacks to use with GESTUREs:
   typedef CallMeth_t<SWEEP_POINT, GESTUREptr> draw_cb_t;
   draw_cb_t* drawCB(draw_cb_t::_method m) {
      return new draw_cb_t(this, m);
   }

   //! Do a uniform sweep operation based on the given
   //! displacement along the guideline (from its origin):
   virtual bool do_uniform_sweep(mlib::CWvec& v);

   //******** UTILITY METHODS ********

   //! Check conditions, cache info, activate:
   bool setup(CGESTUREptr& gest, double dur);

   //******** GEOMETRY CREATION ********

   //! create a rectangular Panel based on given vector along the guideline
   bool create_line(mlib::CWvec& v);

   //******** DrawWidget METHODS ********

   //! Clear cached info when deactivated:
   virtual void reset();
};

#endif // SWEEP_H_IS_INCLUDED

/* end of file sweep.H */
