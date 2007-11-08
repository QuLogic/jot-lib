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
 * subdivide.C:
 **********************************************************************/
#include "std/config.H"
#include "mi.H"
#include "lmesh.H"

int 
main(int argc, char *argv[])
{
   int num_levels = 1;

   if (argc == 2)
      num_levels = max(atoi(argv[1]), 0);
   else if(argc != 1) {
      err_msg("Usage: %s [ num_levels ] < mesh.sm > mesh-sub.sm", argv[0]);
      return 1;
   }

   LMESHptr mesh = LMESH::read_jot_stream(cin);
   if (!mesh || mesh->empty())
      return 1; // didn't work
   mesh->set_subdiv_loc_calc(new LoopLoc());

   if (Config::get_var_bool("JOT_PRINT_MESH")) {
      cerr << "input mesh:" << endl;
      mesh->print();
   }

   if (num_levels > 0)
      mesh->update_subdivision(num_levels);

   if (Config::get_var_bool("JOT_PRINT_MESH")) {
      cerr << "level " << num_levels << " mesh:" << endl;
      mesh->cur_mesh()->print();
   }

   mesh->cur_mesh()->write_stream(cout);

   return 0;
}

/* end of file subdivide.C */
