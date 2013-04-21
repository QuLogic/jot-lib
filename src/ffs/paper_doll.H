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
 * paper_doll.H
 *****************************************************************/
#ifndef PAPER_DOLL_H_IS_INCLUDED
#define PAPER_DOLL_H_IS_INCLUDED

#include "mesh/lmesh.H"
#include "tess/bcurve.H"
#include "tess/primitive.H"
#include "gest/draw_widget.H"

/*****************************************************************
 * PAPER_DOLL:
 *
 *   Widget that handles "paper doll" editing .
 *
 *****************************************************************/
MAKE_PTR_SUBC(PAPER_DOLL,DrawWidget);
typedef const PAPER_DOLL    CPAPER_DOLL;
typedef const PAPER_DOLLptr CPAPER_DOLLptr;
class PAPER_DOLL : public DrawWidget {
 public:

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("PAPER_DOLL",
                        PAPER_DOLL*, DrawWidget, CDATA_ITEM*);

   //******** SETTING UP ********

   // If the gesture is valid for activating the widget,
   // activates it and returns true.
   static bool init(CGESTUREptr& g);

   static bool init(CBcurve_list& boundary);

   //******** DrawWidget METHODS ********

   virtual BMESHptr bmesh() const { return _prim ? _prim->mesh() : 0; }
   virtual LMESH* lmesh()   const { return _prim ? _prim->mesh() : 0; }

   //******** GEL METHODS ******** 

   virtual int draw(CVIEWptr& v);

 protected:

   //******** MEMBER DATA ********

   // under construction...

   Primitive*   _prim;

   static PAPER_DOLLptr _instance;

   static void clean_on_exit();

   //******** MANAGERS ********

   PAPER_DOLL();
   virtual ~PAPER_DOLL() {}

   static PAPER_DOLLptr get_instance();

   static PAPER_DOLLptr get_active_instance() { return upcast(_active); }

   //******** DRAW FSA METHODS ********

   // For defining callbacks to use with GESTUREs:
   typedef CallMeth_t<PAPER_DOLL, GESTUREptr> draw_cb_t;
   draw_cb_t* drawCB(draw_cb_t::_method m) {
      return new draw_cb_t(this, m);
   }

   // Turn off the widget:
   virtual int  cancel_cb(CGESTUREptr& gest, DrawState*&);

   // Tap callback (usually cancel):
   virtual int  tap_cb(CGESTUREptr& gest, DrawState*&);

   // general stroke
   virtual int  stroke_cb(CGESTUREptr&, DrawState*&);

   //******** UTILITY METHODS ********

   LMESHptr mesh() const { return _prim ? _prim->mesh() : 0; }

   Primitive* build_primitive(CBface_list& o_faces);

   bool init(Primitive* p);

   //******** DrawWidget METHODS ********

   // Clear cached info when deactivated:
   virtual void reset();
};

#endif // PAPER_DOLL_H_IS_INCLUDED

// end of file paper_doll.H
