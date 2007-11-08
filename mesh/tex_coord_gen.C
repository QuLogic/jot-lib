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
//base functionality needed by most 
//automatic texture coordinate generators
// by Karol Szerszen


#include "tex_coord_gen.H"


double 
TexCoordGen::fix_range(double main_x, double x2, double x3 )
{

   const double B = 0.5;



   if (abs((main_x - x2)>B) && abs((main_x - x3)>B))
   {
      if (((main_x - x2)>0) && ((main_x - x3)>0))
      {
         main_x-=1.0;

         cout << " A: " << main_x; 
      }
      else
         if (((main_x - x2)<0) && ((main_x - x3)<0))
         {
            main_x+=1.0;
            
            cout << " B: " << main_x; 
         }
         else
            cout << "ERROR, " ;
   
   }


   return main_x;

}




UVpt 
TexCoordGen :: fix_seems(CUVpt& fix_this_coord, CUVpt& coord2, CUVpt& coord3)
{

  UVpt tex_coord = UVpt(fix_range(fix_this_coord[0],coord2[0],coord3[0]), 
                        fix_range(fix_this_coord[1],coord2[1],coord3[1]));

/*

   UVpt tex_coord = fix_this_coord;
   UVpt ts1 = coord2;
   UVpt ts2 = coord3;
   

 
   //get both deltas 
   double delta1U, delta2U; 
   double deltaU = 0.0;
   double delta1V, delta2V;
   double deltaV = 0.0;

   delta1U = ts1[0] - tex_coord[0]; delta2U = ts2[0] - tex_coord[0];
  
   delta1V = ts1[1] - tex_coord[1]; delta2V = ts2[1] - tex_coord[1];

   //check if they are both outside the bounds
   if ( ((delta1U > 0.5) && (delta2U > 0.5)) ||((delta1U < -0.5) && (delta2U < -0.5)))
      deltaU = delta1U;
   
   if ( ((delta1V > 0.5) && (delta2V > 0.5)) ||((delta1V < -0.5) && (delta2V < -0.5)))
      deltaV = delta1V;
   
   
  
  //tweak the texture coordinate to match the others 
  
   if (deltaU >  0.5) 
      tex_coord[0]+= 1.0;
   if (deltaU < -0.5) 
      tex_coord[0]+= -1.0;

   if (deltaV >  0.5) 
      tex_coord[1]+= 1.0;
   if (deltaV < -0.5) 
      tex_coord[1]+= -1.0;

 */
   return tex_coord;


}
