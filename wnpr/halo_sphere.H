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
//Halo sphere object
//by Karol Szerszen

#ifndef HALO_SPHERE_H_IS_INCLUDED
#define HALO_SPHERE_H_IS_INCLUDED

#include "tess/tex_body.H"

//default scale for the bounding box size
#define DEFAULT_HALO_SCALE 1.0

class HaloSphere : public HaloBase {
 public:

   HaloSphere();
   void draw_halo(CVIEWptr &v, CBBOX& box, double scale = DEFAULT_HALO_SCALE);

 protected:

   BMESHptr sphere_mesh; 
   DLhandler    display_list;

   void delete_dl() {
      display_list.delete_all_dl();
   }
};

#endif // SKY_BOX_H_IS_INCLUDED
