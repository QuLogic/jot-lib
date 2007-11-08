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
 * sm2obj.C:
 *
 *   Convert from .sm format to .obj
 *
 *   Usage: sm2obj < input.sm > output.obj
 *
 **********************************************************************/
#include "std/config.H"
#include "mesh/mi.H"

#include <string>

void
write_verts(const BMESH &mesh, ostream &os)
{
   const int num = mesh.nverts();

   int i;
   for (i = 0; i < num; i++) {
      CWpt &loc = mesh.bv(i)->loc();
      os << "v " << loc[0] << " " << loc[1] << " " << loc[2] << endl;
   }
   for (i = 0; i < num; i++) {
      Wvec n = mesh.bv(i)->norm();
      os << "vn " << n[0] << " " << n[1] << " " << n[2] << endl;
   }
}

void
write_faces(const BMESH &mesh, ostream &os, bool do_simple=false)
{
   const int num = mesh.nfaces();

   for (int i = 0; i < num; i++) {
      os << "f ";
      for (int v = 1; v <= 3; v++) {
         int k = mesh.bf(i)->v(v)->index()+1;
         os << k;
         if (!do_simple)
            os << "//" << k;
         os << ((v==3) ? "" : " ");
      }
      os << endl;
   }
}

int 
main(int argc, char *argv[])
{
   // -s option writes "simple" format, meaning faces are written as:
   //
   //   f 1 3 2
   //
   // instead of:
   //
   //   f 1//1 3//3 2//2

   bool do_simple = false;
   if (argc == 2 && string(argv[1]) == "-s") {
      do_simple = true;
   } else if (argc != 1) {
      err_msg("Usage: %s [ -s ] < input.sm > output.sm", argv[0]);
      return 1;
   }

   BMESHptr mesh = BMESH::read_jot_stream(cin);
   if (!mesh || mesh->empty())
      return 1; // didn't work

   if (Config::get_var_bool("JOT_RECENTER"))
      mesh->recenter();

   if (Config::get_var_bool("JOT_PRINT_MESH"))
      mesh->print();

   // write verts
   write_verts(*mesh, cout);

   // write faces
   write_faces(*mesh, cout, do_simple);

   return 0;
}
