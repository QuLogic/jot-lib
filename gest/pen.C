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
/*!
 *  \file pen.C
 *  \brief Contains the implementation of the Pen class.
 *
 *  \sa pen.C
 *
 */

#include "mesh/bmesh.H"
#include "mesh/base_ref_image.H"
#include "std/config.H"
#include "geom/winsys.H"

#include "pen.H"

/**********************************************************************
 * Pen:
 **********************************************************************/
Pen::Pen(Cstr_ptr& pen_name,
         CGEST_INTptr &gest_int,
         CEvent &d, CEvent &m, CEvent &u,
         CEvent &shift_d, CEvent &shift_u,
         CEvent &ctrl_d,  CEvent &ctrl_u) :
   Simple_int(d, m, u),
   _gest_int(gest_int),
   _shift_fsa(0),
   _ctrl_fsa(0),
   _pen_name(pen_name),
   _view(VIEW::peek()),
   _fsa(&_draw_start)
{
   if (!(shift_d == Event())) {
      _shift_fsa = create_fsa(
         shift_d, m, shift_u,
         &Pen::erase_down, &Pen::erase_move, &Pen::erase_up);
   }
   if (!(ctrl_d == Event()))  {
      _ctrl_fsa = create_fsa(ctrl_d, m, ctrl_u,
                             &Pen::ctrl_down, &Pen::ctrl_move,
                             &Pen::ctrl_up);
   }

   _entry.set_name(str_ptr("Pen Entry - ") + pen_name);
}

/**********************************************************************
 * FSA stuff 
 **********************************************************************/
State *
Pen::create_fsa(
   CEvent               &d,
   CEvent               &m,
   CEvent               &u,
   callback_meth_t down_cb,
   callback_meth_t move_cb,
   callback_meth_t   up_cb
  )
{
   State *beg_state = new State;
   State *mid_state = new State;
   
   // This is a subclass of Simple_int, so the callback paramater to
   // Cb must be a CallMeth_t<Simple_int,Event>::_method), so we do a
   // cast
   *beg_state += Arc(d, Cb((_callb::_method) down_cb, mid_state ));
   *mid_state += Arc(m, Cb((_callb::_method) move_cb            ));
   *mid_state += Arc(u, Cb((_callb::_method)   up_cb, (State*)-1));
   
   return beg_state;
}

int
Pen::check_interactive(CEvent &e, State *&s)
{
   DEVice_buttons *btns=(DEVice_buttons *)e._d;
   DEVice_2d      *ptr=btns->ptr2d();
   RAYhit r(ptr->cur());

   return (e.view()->intersect(r).success() &&
           ray_geom(r,GEOM::null) &&
           ray_geom(r,GEOM::null)->interactive(e,s));
}

void  
Pen::notify_gesture(GEST_INT* gi, CGESTUREptr& gest)
{
   if (Config::get_var_bool("DEBUG_GESTURES",false)) {
      gest->print_stats();
      gest->print_types();
   }

   _fsa.handle_event(gest);

   if (_fsa.is_reset())
      gi->reset();
}

int
Pen::down(CEvent &e, State *&s)
{
   return check_interactive(e, s);
}

int
Pen::move(CEvent &,State *&)
{
   return 0;
}

int
Pen::up(CEvent &,State *&)
{
   return 0;
}

int
Pen::erase_down(CEvent &e, State *&s)
{
   return check_interactive(e, s);
}

int
Pen::erase_move(CEvent &,State *&)
{
   return 0;
}

int
Pen::erase_up(CEvent &, State *&)
{
   return 0;
}

void
Pen::activate(State *start) 
{
   if (_gest_int) {
      _gest_int->reset();
      _gest_int->add_obs(this);
   } else {
      *start += _entry;
   }

   if (_shift_fsa)
      *start += *_shift_fsa;
   if (_ctrl_fsa)
      *start += *_ctrl_fsa;
   ModeName::push_name(_pen_name);
}

bool
Pen::deactivate(State *start) 
{
   ModeName::pop_name();

   if (_ctrl_fsa)
      *start -= *_ctrl_fsa;
   if (_shift_fsa)
      *start -= *_shift_fsa;

   if (_gest_int) {
      _gest_int->rem_obs(this);
      _gest_int->reset();
   } else {
      *start -= _entry;
   }
   return true;
}

/* end of file pen.C */
