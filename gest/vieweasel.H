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
#ifndef VIEW_EASEL_H_IS_INCLUDED
#define VIEW_EASEL_H_IS_INCLUDED

/*!
 *  \file vieweasel.H
 *  \brief Contains the definition of the VIEW_EASEL class.
 *
 *  \sa vieweasel.C
 *
 */

#include "disp/view.H"
// #include "draw/trace.H"

MAKE_PTR_BASEC(VIEW_EASEL);
typedef const VIEW_EASELptr CVIEW_EASELptr;

/*!
 *  \brief Container for GELS associated with a given fixed camera.
 *
 */
class VIEW_EASEL : public REFcounter {
   
   public:
   
      //******** MANAGERS ********
      VIEW_EASEL(const VIEWptr &v);
      virtual ~VIEW_EASEL();
      
      //******** PUBLIC INTERFACE ********
      CAMptr  cam() const         { return _camera; }
      VIEWptr view() const        { return _view; }
      
      void saveCam(const CAMptr& c);    // copy and store the given camera parameters
      
//       void bindTrace(TRACEptr t);
      
      void restoreEasel();
      void removeEasel();
      
      GELlist  &lines()            { return _lines; }
      const GELlist &lines() const { return _lines; }
      void      add_line(const GELptr &p);
      void      rem_line(const GELptr &p);
      
      void clear_lines();
      
      GELptr extract_closest(Cstr_ptr &type, const mlib::XYpt &pt,
                             double max_dist, const GELptr &except = GELptr());
      
      GELptr extract_closest_pix(Cstr_ptr &type, const mlib::XYpt &pt,
                                 double max_dist, const GELptr &except = GELptr());

   protected:

      //**** MEMBERS ****
      GELlist      _lines;  //!< they used to be lines, now more generally GELS 
      CAMptr       _camera; //!< the fixed camera
      VIEWptr      _view;   //!< the associated view
      
//       TRACEptr     _trace;  //!< if not null, the trace bound to the easel

};


#endif // VIEW_EASEL_H_IS_INCLUDED

/* end of file vieweasel.H */
