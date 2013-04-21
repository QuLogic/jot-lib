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
 *  \file vieweasel.C
 *  \brief Contains the implementation of the VIEW_EASEL class.
 *  \author Jonathan M. Cohen (jmc)
 *  \date Fri Jun 12 16:06:40 US/Eastern 1998
 *
 *  \sa vieweasel.H
 *
 */

#include "geom/world.H"

#include "vieweasel.H"

using mlib::XYpt;

/*************************************************************************
 * Function Name: VIEW_EASEL::VIEW_EASEL
 * Parameters: CVIEWptr &v
 * Effects: 
 *************************************************************************/
VIEW_EASEL::VIEW_EASEL(
   CVIEWptr &v) :
   _view(v) /* , _trace(NULL) */
{
   _camera = CAMptr( new CAM( str_ptr("easel cam") ) ); 
   saveCam( _view->cam() );
}

VIEW_EASEL::~VIEW_EASEL()
{
}

void 
VIEW_EASEL::saveCam(const CAMptr &c) 
{
   if (c && _camera)
      *_camera = *c; 
}

// void
// VIEW_EASEL::bindTrace(TRACEptr t)
// {
//    if(_trace){
//       WORLD::undisplay(_trace, false);
//    }

//    _trace = t;
//    if (!DRAWN.contains(_trace))
//       WORLD::display(_trace, false);
// }

/*************************************************************************
 * Function Name: VIEW_EASEL::restoreEasel
 * Parameters: 
 * Returns: void
 * Effects: 
 *************************************************************************/
void
VIEW_EASEL::restoreEasel()
{
   _view->copy_cam(_camera);
   for ( int i = 0; i < _lines.num(); i++ ) {
      if (!DRAWN.contains(_lines[i]))
         WORLD::display(_lines[i], false);
   }
//    if (_trace && !DRAWN.contains(_trace))
//       WORLD::display(_trace, false);
}


/*************************************************************************
 * Function Name: VIEW_EASEL::removeEasel
 * Parameters: 
 * Returns: void
 * Effects: 
 *************************************************************************/
void
VIEW_EASEL::removeEasel()
{
   WORLD::undisplay_gels(_lines, false);
//    if (_trace )
//       WORLD::undisplay(_trace, false);
}

void
VIEW_EASEL::add_line(const GELptr &p)
{ 
   _lines.add_uniquely(p);
   if (!DRAWN.contains(p))
      WORLD::display(p, false);
} 

void
VIEW_EASEL::rem_line(const GELptr &p)
{ 
   if (p) {
      WORLD::undisplay(p, false);
      _lines -= p; 
   }
}

void
VIEW_EASEL::clear_lines()
{
   WORLD::undisplay_gels(_lines, false); // not undoable
   _lines.clear();
}

GELptr 
VIEW_EASEL::extract_closest(
   Cstr_ptr&            /* type */,
   const XYpt&          /* pt */,
   double               /* max_dist */, 
   const GELptr&        /* except */
   ) 
{
   cerr << "VIEW_EASEL::extract_closest: not implemented" << endl;
   return 0;
}

GELptr 
VIEW_EASEL::extract_closest_pix(
   Cstr_ptr&            /* type */,
   const XYpt&          /* pt */,
   double               /* max_dist */, 
   const GELptr&        /* except */
   ) 
{
   cerr << "VIEW_EASEL::extract_closest_pix: not implemented" << endl;
   return 0;
}

/* end of file vieweasel.C */
