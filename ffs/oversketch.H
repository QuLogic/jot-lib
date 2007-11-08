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
 * oversketch.H
 *****************************************************************/
#ifndef OVERSKETCH_H_IS_INCLUDED
#define OVERSKETCH_H_IS_INCLUDED

/*!
 *  \file oversketch.H
 *  \brief Contains the declaration of the OVERSKETCH widget.
 *
 *  \ingroup group_FFS
 *  \sa oversketch.C
 *
 */

#include "mesh/lmesh.H"
#include "gest/draw_widget.H"

/*****************************************************************
 * OVERSKETCH:
 *****************************************************************/
MAKE_PTR_SUBC(OVERSKETCH,DrawWidget);
typedef const OVERSKETCH    COVERSKETCH;
typedef const OVERSKETCHptr COVERSKETCHptr;

//! \brief Widget that handles oversketching shapes, generally at
//! the silhouette.
class OVERSKETCH : public DrawWidget {
 public:

   //******** MANAGERS ********

   // no public constructor
   static OVERSKETCHptr get_instance();

   static OVERSKETCHptr get_active_instance() { return upcast(_active); }

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("OVERSKETCH", OVERSKETCH*, DrawWidget, CDATA_ITEM*);

   //******** SETTING UP ********

   //! If the gesture is valid for activating the widget,
   //! activates it and returns true.
   static bool init(CGESTUREptr& g);

   //******** DrawWidget METHODS ********

   virtual BMESHptr bmesh() const { return _mesh; }
   virtual LMESH*   lmesh() const { return _mesh; }

   //******** GEL METHODS ******** 

   virtual int draw(CVIEWptr& v);

 protected:

   //******** MEMBER DATA ********

   // under construction...

   EdgeStrip    _selected_sils;
   Bface_list   _selected_region;
   LMESHptr     _mesh;

   static OVERSKETCHptr _instance;

   static void clean_on_exit();

   //******** MANAGERS ********

   OVERSKETCH();
   virtual ~OVERSKETCH() {}

   //******** MODE NAME ********

   //! displayed in jot window
   virtual str_ptr      mode_name()     const { return "oversketch"; }

   //******** DRAW FSA METHODS ********

   //! For defining callbacks to use with GESTUREs:
   typedef CallMeth_t<OVERSKETCH, GESTUREptr> draw_cb_t;
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

   bool find_matching_sil(CGESTUREptr& gest);
   bool find_matching_sil(mlib::CPIXEL_list& pts, CEdgeStrip& sils);

   bool try_oversketch(mlib::CPIXEL_list& pts);
   bool match_substrip(mlib::CPIXEL_list& pts, CBvert_list& chain, EdgeStrip& strip);
   bool get_sil_substrip(mlib::CPIXEL_list& pts, EdgeStrip& strip);
   bool compute_offsets(mlib::CPIXEL_list& pts, CEdgeStrip& sils);
   bool apply_offsets(CBvert_list& sil_verts, CARRAY<double>& offsets);

   void select_faces();

   bool check_primitive();

   //******** DrawWidget METHODS ********

   //! Clear cached info when deactivated:
   virtual void reset();
};

#endif // OVERSKETCH_H_IS_INCLUDED

/* end of file oversketch.H */
