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
/*! \file light.H
 */

#ifndef LIGHT_H_IS_INCLUDED
#define LIGHT_H_IS_INCLUDED

#include "disp/color.H"
#include "mlib/points.H"

/*! Class for managing lights */

class Light {
 public:
   //******** MANAGERS ********

   /// Default constructor
   Light() :
      _is_enabled(false),
      _is_in_cam_space(true),
      _is_positional(false),
      _ambient_color (COLOR::black),
      _diffuse_color (COLOR::white),
      _specular_color(COLOR::white),
      _coords(mlib::Wvec(0,0,1)),
      _spot_direction(0,0,-1),
      _spot_exponent(0),
      _spot_cutoff(180),
      _k0(1),
      _k1(0),
      _k2(0) {}

   /// Constructor for a directional light
   Light(mlib::CWvec& dir,
         bool is_enabled           = false,
         bool is_in_cam_space      = true,
         CCOLOR& ambient           = COLOR::black,
         CCOLOR& diffuse           = COLOR::white,
         CCOLOR& specular          = COLOR::white
         ) :
      _is_enabled(is_enabled),
      _is_in_cam_space(is_in_cam_space),
      _is_positional(false),
      _ambient_color (ambient),
      _diffuse_color (diffuse),
      _specular_color(specular),
      _coords(dir),
      _spot_direction(0,0,-1),
      _spot_exponent(0),
      _spot_cutoff(180),
      _k0(1),
      _k1(0),
      _k2(0) {}

   /// Constructor for a positional light
   Light(mlib::CWpt& loc,
         bool is_enabled           = false,
         bool is_in_cam_space      = true,
         CCOLOR& ambient           = COLOR::black,
         CCOLOR& diffuse           = COLOR::white,
         CCOLOR& specular          = COLOR::white,
         mlib::CWvec& spot_dir     = mlib::Wvec(0,0,-1),
         float spot_exponent       = 0,  
         float spot_cutoff         = 180,
         float k0                  = 1,
         float k1                  = 0,
         float k2                  = 0) :
      _is_enabled(is_enabled),
      _is_in_cam_space(is_in_cam_space),
      _is_positional(true),
      _ambient_color (ambient),
      _diffuse_color (diffuse),
      _specular_color(specular),
      _coords(loc[0],loc[1],loc[2]),
      _spot_direction(spot_dir),
      _spot_exponent(spot_exponent),
      _spot_cutoff(spot_cutoff),
      _k0(k0),
      _k1(k1),
      _k2(k2) {}

   /// For a directional light, returns the direction:
   mlib::Wvec get_direction() const { return _coords.normalized(); }
   /// For a positional light, returns the position:
   mlib::Wpt  get_position() const {
      return mlib::Wpt(_coords[0],_coords[1],_coords[2]);
   }

   void set_position(mlib::CWpt& p) {
      _coords = mlib::Wvec(p[0],p[1],p[2]);
      _is_positional = true;
   }
   void set_direction(mlib::CWvec& v) {
      _coords = v;
      _is_positional = false;
   }

   //******** MEMBER DATA ********

   // It's all public for now.
   // This is really just plain old data.

   bool       _is_enabled;      ///< is enabled?
   bool       _is_in_cam_space; ///< is defined in camera space?
   bool       _is_positional;   ///< is positional?
   COLOR      _ambient_color;   ///< ambient color
   COLOR      _diffuse_color;   ///< diffuse color
   COLOR      _specular_color;  ///< specular color
   mlib::Wvec _coords;          ///< "coordinates": direction or position
   mlib::Wvec _spot_direction;  ///< GL_SPOT_DIRECTION
   float      _spot_exponent;   ///< GL_SPOT_EXPONENT
   float      _spot_cutoff;     ///< GL_SPOT_CUTOFF
   float      _k0;              ///< GL_CONSTANT_ATTENUATION
   float      _k1;              ///< GL_LINEAR_ATTENUATION
   float      _k2;              ///< GL_QUADRATIC_ATTENUATION
};

#endif // LIGHT_H_IS_INCLUDED

// end of file light.H
