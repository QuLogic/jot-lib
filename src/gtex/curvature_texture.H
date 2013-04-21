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
#ifndef CURVATURE_TEXTURE_H_IS_INCLUDED
#define CURVATURE_TEXTURE_H_IS_INCLUDED

/*!
 *  \file curvature_texture.H
 *  \brief Contains the definitions of the classes that implement the curvature
 *  visualization gTexture.
 *
 */

#include "gtex/basic_texture.H"

/*!
 *  \brief gTexture that implements curvature visualization for a mesh.
 *
 */
class CurvatureTexture : public BasicTexture {
   
   public:
   
      //! \name Constructors
      //@{
      
      CurvatureTexture(Patch* patch = 0, StripCB* cb=0);
      
      //@}
      
      //! \name RTTI Related
      //@{
      
      DEFINE_RTTI_METHODS3("Curvature", CurvatureTexture*,
                           BasicTexture, CDATA_ITEM *);
      
      //@}
      
      //! \name GTexture Virtual Methods
      //@{
      
      virtual int draw(CVIEWptr& v); 
      
      //@}
      
      //! \name DATA_ITEM Virtual Methods
      //@{
      
      virtual DATA_ITEM  *dup() const { return new CurvatureTexture; }
      
      //@}
      
      //! \brief Numerical constants for different curvature filters.
      enum curvature_filter_t {
         FILTER_NONE = 0,
         FILTER_GAUSSIAN = 1,
         FILTER_MEAN = 2,
         FILTER_RADIAL = 3
      };
      
      //! \name Style Parameter Accessors
      //@{
      
      static bool get_draw_radial_curv()
         { return draw_radial_curv; }
      static void set_draw_radial_curv(bool draw_radial_curv_in)
         { draw_radial_curv = draw_radial_curv_in; }
      static bool get_draw_mean_curv()
         { return draw_mean_curv; }
      static void set_draw_mean_curv(bool draw_mean_curv_in)
         { draw_mean_curv = draw_mean_curv_in; }
      static bool get_draw_gaussian_curv()
         { return draw_gaussian_curv; }
      static void set_draw_gaussian_curv(bool draw_gaussian_curv_in)
         { draw_gaussian_curv = draw_gaussian_curv_in; }
      
      static curvature_filter_t get_radial_filter()
         { return radial_filter; }
      static void set_radial_filter(curvature_filter_t radial_filter_in)
         { radial_filter = radial_filter_in; }
      static curvature_filter_t get_mean_filter()
         { return mean_filter; }
      static void set_mean_filter(curvature_filter_t mean_filter_in)
         { mean_filter = mean_filter_in; }
      static curvature_filter_t get_gaussian_filter()
         { return gaussian_filter; }
      static void set_gaussian_filter(curvature_filter_t gaussian_filter_in)
         { gaussian_filter = gaussian_filter_in; }
      
      static float get_sugcontour_thresh()
         { return sugcontour_thresh; }
      static void set_sugcontour_thresh(float sugcontour_thresh_in)
         { sugcontour_thresh = sugcontour_thresh_in; }
      
      //@}
   
   private:
   
      static bool draw_radial_curv;    //!< Draw radial curvature?
      static bool draw_mean_curv;      //!< Draw mean curvature?
      static bool draw_gaussian_curv;  //!< Draw gaussian curvature?
      
      static curvature_filter_t radial_filter;     //!< Radial curvature filter
      static curvature_filter_t mean_filter;       //!< Mean curvature filter
      static curvature_filter_t gaussian_filter;   //!< Gaussian curvature filter
      
      static float sugcontour_thresh;  //!< Suggestive contour threshold
   
};

#endif // CURVATURE_TEXTURE_H_IS_INCLUDED
