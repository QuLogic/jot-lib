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
 * merge.C:
 *
 *      Takes multiple .sm files as input, merges them into
 *      a single mesh, then writes the result to file.
 *
 *      Input parameters on the command line should be the
 *      list of input meshes, followed by the name of the
 *      output mesh.
 *
 **********************************************************************/
#include "std/fstream.H"
#include "mesh/lmesh.H"
#include "std/config.H"

inline BMESHptr
read_mesh(char* infile)
{
   BMESHptr ret = new BMESH;
   if (!ret->read_file(infile))
      return 0;
   return ret;
}

int 
main(int argc, char *argv[])
{
   if (argc < 4)
   {
      err_msg("Usage: %s input1.sm input2.sm [ etc. ] output.sm", argv[0]);
      return 1;
   }

   BMESHptr ret;
   for (int i=1; i < argc - 1; i++) {
      BMESHptr mesh = BMESH::read_jot_file(argv[i]);
      if (mesh) {
         // Remove duplicate vertices while we're at it
         mesh->remove_duplicate_vertices(false); // don't keep the bastards
         ret = (ret ? BMESH::merge(ret, mesh) : mesh);
      }
   }

   if (Config::get_var_bool("JOT_RECENTER"))
      ret->recenter();

   ret->write_file(argv[argc-1]);

   return 0;
}

/* end of file merge.C */
