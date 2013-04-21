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
#ifndef CURVATURE_UI_H_IS_INCLUDED
#define CURVATURE_UI_H_IS_INCLUDED

/*!
 *  \file curvature_ui.H
 *  \brief Contains the definitions of classes for the UI for manipulating
 *  curvature related gTextures (i.e. LineDrawingTexture, CurvatureTexture, etc.).
 *
 *  \sa curvature_ui.C
 *
 */

#include <vector>
#include <map>

#include "std/hash.H"
#include "disp/view.H"

class CurvatureUI;

/*!
 *  \brief Singleton class that provides a single, global access point for all
 *  CurvatureUI's.
 *
 */
class CurvatureUISingleton {
   
   public:
   
      //! \brief Get the instance of the singleton.
      inline static CurvatureUISingleton &Instance();
      
      //! \brief Is the CurvatureUI for the given VIEW visible?
      bool is_vis(CVIEWptr& v);
      
      //! \brief Show the CurvatureUI for the given VIEW if it is not already
      //! shown.
      bool show(CVIEWptr& v);
      //! \brief Hide the CurvatureUI for the given VIEW if it is currently shown.
      bool hide(CVIEWptr& v);
      
      //! \brief Update the CurvatureUI for the given VIEW.
      bool update(CVIEWptr& v);
      
      float sc_thresh()
         { return _sc_thresh; }
      
      bool line_drawing_draw_contours()
         { return _line_drawing_draw_contours != 0; }
      bool line_drawing_draw_sugcontours()
         { return _line_drawing_draw_sugcontours != 0; }
      bool line_drawing_draw_color()
          { return _line_drawing_draw_color != 0; }
      
      bool curvature_draw_gaussian_curv()
         { return _curvature_draw_gaussian_curv != 0; }
      int curvature_gaussian_filter()
         { return _curvature_gaussian_filter; }
      bool curvature_draw_mean_curv()
         { return _curvature_draw_mean_curv != 0; }
      int curvature_mean_filter()
         { return _curvature_mean_filter; }
      bool curvature_draw_radial_curv()
         { return _curvature_draw_radial_curv != 0; }
      int curvature_radial_filter()
         { return _curvature_radial_filter; }
   
   private:
   
      float _sc_thresh;
      int _line_drawing_draw_contours;
      int _line_drawing_draw_sugcontours;
      int _line_drawing_draw_color;
      int _curvature_draw_gaussian_curv;
      int _curvature_draw_mean_curv;
      int _curvature_draw_radial_curv;
      int _curvature_gaussian_filter;
      int _curvature_mean_filter;
      int _curvature_radial_filter;
   
      //! \brief Get the CurvatureUI for the given VIEW.
      CurvatureUI *fetch(CVIEWptr& v);
      
      typedef std::map<VIEWimpl*, CurvatureUI*> view2ui_map_t;
      view2ui_map_t view2ui_map;
   
      CurvatureUISingleton();
      CurvatureUISingleton(const CurvatureUISingleton &);
      
      ~CurvatureUISingleton();
      
      CurvatureUISingleton &operator=(const CurvatureUISingleton &);
      
      friend class CurvatureUI;
   
};

inline CurvatureUISingleton&
CurvatureUISingleton::Instance()
{
   
   static CurvatureUISingleton instance;
   return instance;
   
}

#endif // CURVATURE_UI_H_IS_INCLUDED
