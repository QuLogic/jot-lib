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
#include "gl_sphir_tex_coord_gen.H"
#include "mesh/bmesh.H"



void
GLSphirTexCoordGen ::setup()
{
}

UVpt
GLSphirTexCoordGen ::compute_uv(CBvert *v)
{
  //computes spherical 2d texture coordinates
  //in object coordinates
   Wvec n = v->loc() - v->mesh()->get_bb().center(); //Wpt(0,0,0);  
   n = n.normalized(); 

   double alpha = atan2(n[2],n[0]); 
   double theta = atan2(sqrt(n[0]*n[0]+n[2]*n[2]), -n[1]); //acos(n[1]);

   UVpt tex_coord = UVpt((alpha / (TWO_PI) ) + 0.5, (theta / TWO_PI) + 0.5);
   
   return tex_coord;
}


UVpt
GLSphirTexCoordGen ::uv_from_vert(CBvert *v, CBface *f)
{
   //in object coordinates

   UVpt tex_coord = compute_uv(v);

   Bvert* t1 = f->v1();
   Bvert* t2 = f->v2();
   Bvert* t3 = f->v3();

   UVpt ts1;
   UVpt ts2;
   
   //grab the two other UV coordinates for this face
   
   if (t1 == v)
   {
      ts1 = compute_uv(t2);
      ts2 = compute_uv(t3);
   }
   if (t2 == v)
   {
      ts1 = compute_uv(t1);
      ts2 = compute_uv(t3);
   }

   if (t3 == v)
   {
      ts1 = compute_uv(t1);
      ts2 = compute_uv(t2);
   }
 
  
   return fix_seems(tex_coord,ts1,ts2);
}

// end of file gl_sphir_tex_coord_gen.C
