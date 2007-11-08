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
#include "std/config.H"
#include "disp/colors.H"

// Color constants
CCOLOR COLOR::black(0.0,0.0,0.0);
CCOLOR COLOR::white(1.0,1.0,1.0);

CCOLOR COLOR::red  (1.0,0.0,0.0);
CCOLOR COLOR::green(0.0,1.0,0.0);
CCOLOR COLOR::blue (0.0,0.0,1.0);

/*****************************************************************
 * COLOR
 *****************************************************************/
COLOR::COLOR(CHSVCOLOR &c)
{
   double h  = (c[0] == 1.0) ? 0.0 : (c[0] * 6.0);
   double hf = h - (int)h;

   double p1 = c[2] * (1.0 - (c[1])           );
   double p2 = c[2] * (1.0 - (c[1] * (    hf) ));
   double p3 = c[2] * (1.0 - (c[1] * (1.0-hf) ));

   switch((int)h){
      case 0 : 
         _x = c[2] ;    _y = p3;       _z = p1;    
      break;
      case 1 : 
         _x = p2;       _y = c[2] ;    _z = p1;    
      break;
      case 2 : 
         _x = p1;       _y = c[2] ;    _z = p3; 
      break;
      case 3 : 
         _x = p1;       _y = p2;       _z = c[2] ; 
      break;
      case 4 : 
         _x = p3;       _y = p1;       _z = c[2] ; 
      break;
      case 5 : 
         _x = c[2] ;    _y = p1;       _z = p2; 
      break;
      default:
         cerr << "Bad HSVCOLOR used in COLOR constructor!!!" << endl;
         _x =        _y =        _z = -1.0;
   }

}

HSVCOLOR::HSVCOLOR(CCOLOR &c)
{


   double m = min(min(c[0],c[1]),c[2]);
   _z = max(max(c[0],c[1]),c[2]);
   _y = (_z!=0.0)?(_z-m)/_z:0.0;

   if ((m < 0.0) || (_z > 1.0))
   {
      cerr << "Bad COLOR used in HSVCOLOR constructor!!!" << endl;
      _x = _y = _z = -1.0;
      return;
   }

   if (_y != 0.0) {
      
      double r1 = (_z - c[0]) / (_z - m);
      double g1 = (_z - c[1]) / (_z - m);
      double b1 = (_z - c[2]) / (_z - m);

      if      (_z == c[0])
      {
         _x = (m == c[1]) ? 5.0 + b1 : 1.0 - g1;
      }
      else if (_z == c[1])
      {
         _x = (m == c[2]) ? 1.0 + r1 : 3.0 - b1;
      }
      else
      {
         _x = (m == c[0]) ? 3.0 + g1 : 5.0 - r1;
      }

      _x /= 6.0;

   } 
   else 
   {
      _x = 0.0;
   }

}

/* end of file color.C */
