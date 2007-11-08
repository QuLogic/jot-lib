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
#ifndef _COLOR_H_INC
#define _COLOR_H_INC
#include "mlib/points.H"


class COLOR;
class HSVCOLOR;

typedef const class COLOR     CCOLOR;
typedef const class HSVCOLOR  CHSVCOLOR;

/* ------------------ class definitions --------------------- */

class COLVEC : public mlib::Vec3<COLVEC> {
 public :
   COLVEC() { }
   COLVEC(double x, double y, double z)
      : mlib::Vec3<COLVEC>(x,y,z) { }
};

class COLOR : public mlib::Point3<COLOR, COLVEC> {
 protected:

 public :
   COLOR() { }
   COLOR(double x, double y, double z)
      : mlib::Point3<COLOR,COLVEC>(x,y,z) { }
   COLOR(const double col[4])
      : mlib::Point3<COLOR,COLVEC>(col[0],col[1],col[2]) {}
   COLOR(CHSVCOLOR &c);
   static inline COLOR any()  { return COLOR(.5 + drand48()/2, 
                                             .5 + drand48()/2, 
                                             .5 + drand48()/2); }
   static COLOR random() { return COLOR(drand48(), drand48(), drand48()); }

   double luminance()     const { return .30*_x + .59*_y + .11*_z; }

   // returns linear blend between this color and dest. amount should
   // be between 0 and 1
   COLOR blend(CCOLOR& dest, double amount) const { 
      return COLOR(dest[0]*amount + (1.0-amount)*_x, 
                   dest[1]*amount + (1.0-amount)*_y, 
                   dest[2]*amount + (1.0-amount)*_z);
   }

   // basic color constants:
   // (more are defined in colors.H):
   static CCOLOR     black;     // 0 0 0
   static CCOLOR     white;     // 1 1 1

   static CCOLOR       red;     // 1 0 0
   static CCOLOR     green;     // 0 1 0
   static CCOLOR      blue;     // 0 0 1
};


#ifdef JOT_NEEDS_DOUBLE_STAR_EXPLICIT
// Should be able to define this once for Point3, but that doesn't work
// (then again, we have to do this because of template problems in the first
// place)
inline COLOR operator *(double s, const COLOR& p) { return p * s;}
#endif


#define CHSVCOLOR const HSVCOLOR
class HSVCOLVEC : public mlib::Vec3<HSVCOLVEC> {
 public :
   HSVCOLVEC() { }
   HSVCOLVEC(double h, double s, double v)
      : mlib::Vec3<HSVCOLVEC>(h,s,v) { }
};

class HSVCOLOR : public mlib::Point3<HSVCOLOR, HSVCOLVEC> {
 public :
   HSVCOLOR() { }
   HSVCOLOR(double h, double s, double v)
      : mlib::Point3<HSVCOLOR,HSVCOLVEC>(h,s,v) { }
   HSVCOLOR(const double col[4])
      : mlib::Point3<HSVCOLOR,HSVCOLVEC>(col[0],col[1],col[2]) { }
   HSVCOLOR(CCOLOR &c);
};

#ifdef JOT_NEEDS_DOUBLE_STAR_EXPLICIT
// Should be able to define this once for Point3, but that doesn't work
// (then again, we have to do this because of template problems in the first
// place)
inline HSVCOLOR operator *(double s, const HSVCOLOR& p) { return p * s;}
#endif

#endif // _COLOR_H_INC

// end of file color.H

