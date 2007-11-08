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
/**********************************
Vertex Attribute 
function object 

by Karol Szerszen
***********************************/
#ifndef VERT_ATTRIB_H_IS_INCLUDED
#define VERT_ATTRIB_H_IS_INCLUDED

#include "bface.H"


template <typename AttType, typename RetType>
class VertAttrib {
 public :

   virtual ~VertAttrib() {}

   //Chuck Norris can instantiate a pure virtual templated class.
   virtual AttType get_attrib(CBvert *v, CBface* f) = 0;





   virtual RetType dFd(CBface* f, int dir) 
   {

   //literal C++ translation 
   //will get simplified with built in functions..

      AttType att1 = get_attrib(f->v1(),f);
      AttType att2 = get_attrib(f->v2(),f);
      AttType att3 = get_attrib(f->v3(),f);

//might want to try loc instead of wloc

     Wvec A = f->v2()->wloc() - f->v1()->wloc();
     Wvec B = f->v3()->wloc() - f->v1()->wloc();
      
     RetType delta1 = att2 - att1;
     RetType delta2 = att3 - att1;

      
     double a_T,b_T,c_T,d_T;
/*
      { a_T, c_T }      { Ax, Ay, Az }    { Ax, Bx } 
      { b_T, d_T }  =   { Bx, By, Bz } *  { Ay, By }
                                          { Az, Bz }
 */ 
      a_T = A[0]*A[0]+A[1]*A[1]+A[2]*A[2];
      b_T = A[0]*B[0]+A[1]*B[1]+A[2]*B[2];
      c_T = A[0]*B[0]+A[1]*B[1]+A[2]*B[2];
      d_T = B[0]*B[0]+B[1]*B[1]+B[2]*B[2];

      double a_S,b_S,c_S,d_S,e_S,f_S;
      double det = 1.0 /(a_T * d_T - b_T * c_T);

/*
      { a_S, c_S, e_S }    { a_T, c_T }^-1     { Ax, Ay, Az }
      { b_S, d_S, f_S } =  { b_T, d_T }     *  { Bx, By, Bz }

*/


      a_S = ((A[0] *  d_T) + (B[0] * -c_T)) * det;
      b_S = ((A[0] * -b_T) + (B[0] *  a_T)) * det; 
     
      c_S = ((A[1] *  d_T) + (B[1] * -c_T)) * det;
      d_S = ((A[1] * -b_T) + (B[1] *  a_T)) * det; 

      e_S = ((A[2] *  d_T) + (B[2] * -c_T)) * det;
      f_S = ((A[2] * -b_T) + (B[2] *  a_T)) * det; 


      switch (dir)
      {
      case 0 : // X
         {
            return delta1 * a_S + delta2 * b_S;
         }

      case 1 : // Y
         {
            return delta1 * c_S + delta2 * d_S;
         }

      case 2 : // Z
         {
            return delta1 * e_S + delta2 * f_S;
         }
      }

     //default average of all 3 principal directions

     return((delta1 * a_S + delta2 * b_S) +
            (delta1 * c_S + delta2 * d_S) +
            (delta1 * e_S + delta2 * f_S)) / 3.0;
   
   }

        
   virtual RetType dFdx(CBface* f) {
      
      return dFd(f,0);

   }

   virtual RetType dFdy(CBface* f) {
      
      return dFd(f,1);
     
   }

   virtual RetType dFdz(CBface* f) {
      
      return dFd(f,2);
      
   }
};

#endif // VERT_ATTRIB_H_IS_INCLUDED

// end of file vert_attrib.H
