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
#ifndef COLORS_H_IS_INCLUDED
#define COLORS_H_IS_INCLUDED

/*****************************************************************
 * colors.H
 *
 *      Definitions of additional named colors.  New named colors
 *      can be added here without changing color.H. The reason to
 *      avoid changing color.H is that many jot files depend on
 *      it, so changes to that file lead to a long recompile.
 *      Relatively few files depend on this header, so new colors
 *      can be defined here without causing excessively long
 *      recompilation times.
 *
 *****************************************************************/
#include "disp/rgba.H"  // should be moved to disp
#include "std/config.H"
#include "color.H"

namespace Color {

   //! \brief create a COLOR from unsigned bytes r, g, b
   inline COLOR color_ub(unsigned char r, unsigned char g, unsigned char b) {
      return COLOR(r/255.0, g/255.0, b/255.0);
   }
   //! \brief create a COLOR from an rgba unsigned int
   //  (ignores alpha)
   inline COLOR rgba_to_color(uint rgba) {
      return COLOR(rgba_to_r(rgba)/255.0,
                   rgba_to_g(rgba)/255.0,
                   rgba_to_b(rgba)/255.0);
   }
   //! \brief return alpha (in [0,1]) from an rgba unsigned int
   //  (ignores rgb)
   inline double rgba_to_alpha(uint rgba) {
      return rgba_to_a(rgba)/255.0;
   }

   //! \brief create an rgba unsigned int from a COLOR and alpha
   inline uint color_to_rgba(CCOLOR& col, double alpha=1) {
      return build_rgba(uchar(255*col[0]),
                        uchar(255*col[1]),
                        uchar(255*col[2]),
                        uchar(255*alpha ));
   }

   //******** NAMED COLORS ********

   CCOLOR black         (0.0,0.0,0.0);
   CCOLOR white         (1.0,1.0,1.0);

   CCOLOR grey1         (0.1,0.1,0.1);
   CCOLOR grey2         (0.2,0.2,0.2);
   CCOLOR grey3         (0.3,0.3,0.3);
   CCOLOR grey4         (0.4,0.4,0.4);
   CCOLOR grey5         (0.5,0.5,0.5);
   CCOLOR grey6         (0.6,0.6,0.6);
   CCOLOR grey7         (0.7,0.7,0.7);
   CCOLOR grey8         (0.8,0.8,0.8);
   CCOLOR grey9         (0.9,0.9,0.9);

   inline COLOR grey(double g) { return COLOR(g,g,g); }

   CCOLOR red           (1.0,0.0,0.0);
   CCOLOR green         (0.0,1.0,0.0);
   CCOLOR blue          (0.0,0.0,1.0);

   CCOLOR yellow        (1.0,1.0,0.0);
   CCOLOR magenta       (1.0,0.0,1.0);
   CCOLOR cyan          (0.0,1.0,1.0);

   CCOLOR pink          (1.0,0.5,0.5);

   CCOLOR orange        (1.0,0.5,0.0);

   CCOLOR brown         (0.5,.37,.25);
   CCOLOR tan           (0.9,0.8,0.7);

   CCOLOR firebrick     (0.698, 0.133, 0.133);

   CCOLOR blue_pencil_l         = color_ub(166,201,243); // light
   CCOLOR blue_pencil_m         = color_ub(104,158,222); // medium
   CCOLOR blue_pencil_d         = color_ub( 79,135,211); // dark

   CCOLOR red_pencil            = color_ub(215, 50, 20);

   CCOLOR orange_pencil_l       = color_ub(255, 204, 51);
   CCOLOR orange_pencil_m       = color_ub(255, 153, 11);
   CCOLOR orange_pencil_d       = color_ub(243, 102, 0);

   CCOLOR blue1   = color_ub( 63,  64,  69);
   CCOLOR blue2   = color_ub(  6,  90, 126);
   CCOLOR blue3   = color_ub(100, 117, 135);
   CCOLOR blue4   = color_ub(133, 156, 162);  
   CCOLOR green2  = color_ub(223, 217, 185);
   CCOLOR green1  = color_ub(193, 186, 118);
   CCOLOR orange1 = color_ub(234, 137,  33);     
   CCOLOR orange2 = color_ub(254, 171,  69);  
   CCOLOR orange3 = color_ub(255, 216, 178);
   CCOLOR red1    = color_ub(115,   0,  24);
   CCOLOR red2    = color_ub(149,  47,   7);
   CCOLOR red3    = color_ub(204, 136,  99);  

   CCOLOR brown5  = color_ub(180, 150,  98);
   CCOLOR brown4  = color_ub(207, 164, 101);  
   CCOLOR brown3  = color_ub(170, 120,  63);     
   CCOLOR brown2  = color_ub(137,  91,  46);  
   CCOLOR brown1  = color_ub( 93,  50,  23); 

   CCOLOR gray1   = color_ub(140, 141, 146);     
   CCOLOR gray2   = color_ub(183, 172, 166);  
   CCOLOR gray3   = color_ub(236, 225, 219);

   CCOLOR s_green = color_ub(177 , 217,  60); //selection green

   //******** UTILITIES ********

   // Convert a color to a string, e.g. "0.25 0.7 0.15":
   inline str_ptr color_to_str(CCOLOR& col) {
      return str_ptr(col[0]) + " " + col[1] + " " + col[2];
   }

   // Given a Config variable name and a default COLOR value,
   // return the corresponding COLOR if the variable is defined,
   // otherwise return the default value:
   inline CCOLOR get_var_color(Cstr_ptr& var_name, CCOLOR& default_val) {
      str_ptr col_str = Config::get_var_str(var_name, color_to_str(default_val));
      double r, g, b;
      sscanf(**col_str, "%lf%lf%lf", &r, &g, &b);
      return COLOR(r,g,b);
   }

   // component-wise min and max for COLORs:
   // WIN32 asks that you do not name a function "min" or "max":
   inline COLOR
   get_min(CCOLOR& a, CCOLOR& b) {
      return COLOR(min(a[0],b[0]),
                   min(a[1],b[1]),
                   min(a[2],b[2]));
   }

   inline COLOR
   get_max(CCOLOR& a, CCOLOR& b) {
      return COLOR(max(a[0],b[0]),
                   max(a[1],b[1]),
                   max(a[2],b[2]));
   }
}

#endif // COLORS_H_IS_INCLUDED

// end of file colors.H
