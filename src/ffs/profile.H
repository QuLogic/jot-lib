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
 * profile.H
 *****************************************************************/
#ifndef PROFILE_H_IS_INCLUDED
#define PROFILE_H_IS_INCLUDED

/*!
 *  \file profile.H
 *  \brief Contains the declaration of the PROFILE widget (applicable to primitives).
 *
 *  \ingroup group_FFS
 *  \sa profile.C
 *
 */

#include "mesh/lmesh.H"
#include "gest/draw_widget.H"

/*****************************************************************
 * PROFILE:
 *****************************************************************/
MAKE_PTR_SUBC(PROFILE,DrawWidget);
typedef const PROFILE    CPROFILE;
typedef const PROFILEptr CPROFILEptr;

//! \brief Widget that handles oversketching primitives, generally at
//! the silhouette.
class PROFILE : public DrawWidget {
 public:

   //******** MANAGERS ********

   // no public constructor
   static PROFILEptr get_instance();

   static PROFILEptr get_active_instance() { return upcast(_active); }

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("PROFILE", PROFILE*, DrawWidget, CDATA_ITEM*);

   //******** SETTING UP ********

   //! If the gesture is valid for activating the widget,
   //! activates it and returns true.
   static bool init(CGESTUREptr& g);

   //******** DrawWidget METHODS ********

   virtual BMESHptr bmesh() const { return _mesh; }
   virtual LMESH*   lmesh() const { return _mesh; }

   //******** GEL METHODS ******** 

   virtual int draw(CVIEWptr& v);

   //******** UTILITY METHODS **********

   void inc_aoi();
   void dec_aoi();

 protected:

   //******** MEMBER DATA ********

   // under construction...

   EdgeStrip    _selected_edges;
   Bface_list   _selected_region;
   EdgeStrip    _region_boundary;
   ARRAY<bool>  _boundary_side;
   LMESHptr     _mesh;
   int          _n, _a, _b; // these affect the aoi
   int          _mode; // 0-editing silhouettes; 1-editing cross sections

   static PROFILEptr _instance;

   static void clean_on_exit();

   //******** MANAGERS ********

   PROFILE();
   virtual ~PROFILE() {}

   //******** MODE NAME ********

   //! displayed in jot window
   virtual str_ptr      mode_name()     const { return "profile"; }

   //******** DRAW FSA METHODS ********

   //! For defining callbacks to use with GESTUREs:
   typedef CallMeth_t<PROFILE, GESTUREptr> draw_cb_t;
   draw_cb_t* drawCB(draw_cb_t::_method m) {
      return new draw_cb_t(this, m);
   }

   //! Turn off the widget:
   virtual int  cancel_cb(CGESTUREptr& gest, DrawState*&);

   //! Tap callback (usually cancel):
   virtual int  tap_cb(CGESTUREptr& gest, DrawState*&);

   //! general stroke
   virtual int  stroke_cb(CGESTUREptr&, DrawState*&);

   //******** UTILITY METHODS ********

   bool find_matching_xsec(CGESTUREptr& gest);
   bool do_xsec_match(mlib::PIXEL_list& pts);
   bool find_matching_sil(CGESTUREptr& gest);
   bool find_matching_sil(mlib::CPIXEL_list& pts, CEdgeStrip& sils);

   bool try_oversketch(mlib::CPIXEL_list& pts);
   bool try_extend_boundary(mlib::CPIXEL_list& pts);
   bool match_substrip(mlib::CPIXEL_list& pts, CBvert_list& chain, EdgeStrip& strip);
   bool get_sil_substrip(mlib::CPIXEL_list& pts, EdgeStrip& strip);
   bool compute_offsets(mlib::CPIXEL_list& pts, CEdgeStrip& sils);
   bool apply_offsets(CBvert_list& sil_verts, CARRAY<double>& offsets);
   bool sharp_end_xform(Bvert* v, PIXEL tap);

   void select_faces();
   bool test_bound_dec(int dec, bool side);
   bool test_bound_inc(int inc);
   
   Bvert_list n_next_verts(Bvert* vert, int n, bool dir);
   Bface_list n_next_quads(Bedge* edge, int n, bool dir, bool add_bound);
   //Bface_list n_quad_ring(Bedge* edge, int n, bool dir, bool add_bound);

   //******** DrawWidget METHODS ********

   //! Clear cached info when deactivated:
   virtual void reset();
};

#endif // PROFILE_H_IS_INCLUDED

/* end of file profile.H */
