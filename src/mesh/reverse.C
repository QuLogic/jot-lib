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
/**********************************************************************
 * reverse.C:
 *
 *   Usage:
 *      % reverse [ -c ] < input.sm > output.sm
 *
 *   With -c option, just reverses the normals of each connected
 *   component with negative "volume." Otherwise reverses all the
 *   normals for the whole mesh.
 *
 **********************************************************************/
#include "std/config.H"
#include "mi.H"

int 
main(int argc, char *argv[])
{
   // See note above about -c option:
   bool do_components = false;
   if (argc == 2 && str_ptr(argv[1]) == str_ptr("-c")) {
      do_components = true;
   } else if (argc != 1) {
      err_msg("Usage: %s [ -c ] < input.sm > output.sm", argv[0]);
      return 1;
   }

   BMESHptr mesh = BMESH::read_jot_stream(cin);
   if (!mesh || mesh->empty())
      return 1; // didn't work

   if (do_components) {
      // Reverse separate components as needed:
      bool changed = false;
      ARRAY<Bface_list> components = mesh->get_components();
      for (int i=0; i<components.num(); i++) {
         if (components[i].volume() < 0) {
            err_msg("reversing component: %d faces", components[i].num());
            reverse_faces(components[i]);
            changed = true;
         }
      }
      if (changed) {
         mesh->changed();
      } else {
         err_msg("%s: nothing changed", argv[0]);
      }
   } else {
      // Do the whole thing
      mesh->reverse();
   }

   mesh->write_stream(cout);

   return 0;
}
