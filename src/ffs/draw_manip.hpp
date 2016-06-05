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
/**********************************************************************
 * draw_manip.H
 **********************************************************************/
#ifndef DRAW_MANIP_H_HAS_BEEN_INCLUDED
#define DRAW_MANIP_H_HAS_BEEN_INCLUDED

/*!
 *  \file draw_manip.H
 *  \brief Contains the declaration of the DrawManip class.
 *
 *  \ingroup group_FFS
 *  \sa draw_manip.C
 *
 */

#include "manip/manip.H"
#include "tess/bpoint.H"
#include "tess/bcurve.H"
#include "tess/tess_cmd.H"

/******************************************************************
 * DrawManip:
 ******************************************************************/
//! \brief Manipulates objects in the scene via button 2.
class DrawManip : public Simple_int {
 public:
   //******** MANAGERS ********
   DrawManip(CEvent &d, CEvent &m, CEvent &u);

   //******** CONVENIENCE ********
   XYpt    ptr_cur ()  const { return DEVice_2d::last->cur(); }
   XYpt    ptr_last()  const { return DEVice_2d::last->old(); }
   Wline   ptr_ray()   const { return Wline(ptr_cur()); }

   //******** FSA CALLBACKS ********
   virtual int choose_cb     (CEvent &,State *&);
   virtual int plane_trans_cb(CEvent &,State *&);
   virtual int line_trans_cb (CEvent &,State *&);
   virtual int up_cb         (CEvent &,State *&);

   //******** Simple_int METHODS ********
   virtual int down(CEvent &,State *&);
   virtual int move(CEvent &,State *&);
   virtual int up  (CEvent &,State *&);

 protected:
   
   //******** MEMBERS ********
   State        _choosing;    //!< transitional state
   State        _plane_trans; //!< moving in a given plane
   State        _line_trans;  //!< moving along a given line

   MOVE_CMDptr  _cmd;
   Wpt          _down_pt;
   Wvec         _vec;         //!< constraint vector (line dir or plane normal)

   PIXEL        _down_pix;    //!< pixel location of last mouse event
   bool         _down;
   bool         _first;

   //******** INTERNAL METHODS ********
   typedef int (DrawManip::*callback_meth_t)(CEvent&, State *&);
   State  *create_fsa(CEvent &d, CEvent &m, CEvent &u,
                      callback_meth_t down, callback_meth_t move,
                      callback_meth_t up); // What the heck is this??? -B
   int check_interactive(CEvent &e, State *&s);

   bool init_point(Bpoint* p, State*& s);
   bool init_curve(Bcurve* c, State*& s);

   void apply_translation(CWvec& delt);

   // constraint plane or line:
   Wplane plane() const { return Wplane(_down_pt, _vec); }
   Wline  line()  const { return Wline (_down_pt, _vec); }
};

#endif  // DRAW_MANIP_H_HAS_BEEN_INCLUDED

// end of file draw_manip.H
