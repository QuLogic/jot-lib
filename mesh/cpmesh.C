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
 * cpmesh.C:
 **********************************************************************/
#include "std/config.H"
#include "mi.H"

int 
main(int argc, char *argv[])
{
   if (argc != 1) {
      err_msg("Usage: %s < input.sm > output.sm", argv[0]);
      return 1;
   }

   BMESHptr mesh = BMESH::read_jot_stream(cin);
   if (!mesh || mesh->empty())
      return 1; // didn't work

   // Remove duplicate vertices
   mesh->remove_duplicate_vertices(false); // don't keep the bastards

   bool is_bad = false;
   for (int i=0; i<mesh->nedges(); i++)
      if (!mesh->be(i)->consistent_orientation())
         is_bad = true;
   if (is_bad)
      err_msg("Warning: inconsistently oriented triangles -- can't fix");

   if (Config::get_var_bool("JOT_COMPUTE_CREASES"))
      mesh->compute_creases();

   if (Config::get_var_bool("JOT_RECENTER"))
      mesh->recenter();

   if (Config::get_var_bool("JOT_PRINT_MESH"))
      mesh->print();

   mesh->write_stream(cout);

   return 0;
}
