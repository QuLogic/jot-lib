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
#ifndef _GUI_H_IS_INCLUDED_
#define _GUI_H_IS_INCLUDED_

#include "mesh/patch.H"
#include "mesh/bmesh.H"

namespace GUI
{

// If no patch is given, gives a texture of the
// Focused patch, otherwise of the patch supplied
template <class T>
inline T* get_current_texture(Patch* p=Patch::focus())
{
   return p ? T::upcast(p->cur_tex()) : 0;
}

template <class T>
inline void get_all_my_textures(
   str_list& list,              // names displayed in GUI for my_patches
   int& index,                  // index of my_p in list
   Patch*& my_p,                // "focus" patch if its cur tex is T
   Patch_list& my_patches       // all patches whose cur tex is T
   )
{
   index = -1;
   my_p = 0;
   my_patches.clear();
   BMESH_list meshes = BMESH_list(DRAWN);
   for (int i=0; i < meshes.num(); ++i) {
      Patch_list patch = meshes[i]->patches();
      for (int j=0; j < patch.num(); ++j) {
         if (T::upcast(patch[j]->cur_tex())) {
            my_patches.add(patch[j]);
            list.add(str_ptr(" m_") + str_ptr(i) +
                     str_ptr("_p_") + str_ptr(j));
            if (patch[j] == Patch::focus()) {
               my_p = patch[j];
               index = list.num()-1;
            }
         }
      }
   }
}

template <class T>
inline bool exists_in_the_world()
{
   str_list list;
   Patch_list my_patches;
   Patch* my_p;
   int index;
   get_all_my_textures<T>(list, index, my_p, my_patches);
   return (!my_patches.empty());
}

};

#endif // _GUI_H_IS_INCLUDED_

// end of file gui.H
