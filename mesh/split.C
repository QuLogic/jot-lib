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
 * split.C:
 *
 *      Takes an .sm file as input, splits the mesh into
 *      disconnected pieces, then writes each piece to file
 *      as a separate mesh.
 *
 *      If the input is named bar.sm, and there are found to
 *      be 3 separate pieces, then they will be written to
 *      the files bar0.sm, bar1.sm, and bar2.sm.
 *
 **********************************************************************/
#include "std/config.H"
#include "mi.H"

int 
main(int argc, char *argv[])
{
   if (argc != 2)
   {
      err_msg("Usage: %s input-mesh.sm", argv[0]);
      return 1;
   }

   ifstream fin;
   fin.open(argv[1]);
   if (!fin) {
      err_ret( "%s: Error: Could not open file %s", argv[0], argv[1]);
      return 0;
   }

   // Get the name without the .sm:
   // Probably there is a better way than what follows...
   char mesh_name[1024];
   strcpy(mesh_name, argv[1]);
   int n = strlen(mesh_name);
   if (n < 4)
   {
      err_msg("%s: Can't decipher name %s", argv[0], mesh_name);
      return 1;
   }
   mesh_name[n - 3] = 0;        // stomp the extension

   str_ptr mesh_path = str_ptr(mesh_name);

   BMESHptr mesh = BMESH::read_jot_stream(fin);
   if (!mesh || mesh->empty())
      return 1; // didn't work
   fin.close();

   // Remove duplicate vertices while we're at it
   mesh->remove_duplicate_vertices(false); // don't keep the bastards

   // Split it:
   ARRAY<BMESH*> meshes = mesh->split_components();

   err_msg("got %d meshes", meshes.num());

   str_ptr out_mesh = mesh_path + str_ptr(0) + str_ptr(".sm");
   cerr << "\nwriting " << **out_mesh << endl;
   if (Config::get_var_bool("JOT_RECENTER"))
      mesh->recenter();
   mesh->write_file(**out_mesh);

   for (int i=0; i<meshes.num(); i++) {
      out_mesh = mesh_path + str_ptr(i + 1) + str_ptr(".sm");
      cerr << "\nwriting " << **out_mesh << endl;
      if (Config::get_var_bool("JOT_RECENTER"))
         meshes[i]->recenter();
      meshes[i]->write_file(**out_mesh);
   }

   return 0;
}

/* end of file split.C */
