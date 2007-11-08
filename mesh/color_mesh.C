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
 * color_mesh.C:
 **********************************************************************/
#include "disp/colors.H"
#include "std/config.H"
#include "mi.H"

using namespace mlib;

inline void
set_base_colors(CBvert_list& verts, CCOLOR& col)
{
   for (int i=0; i<verts.num(); i++)
      verts[i]->set_color(col, 1);
}

inline void
add_shading(CBvert_list& verts, Wvec l, CCOLOR& col, double s = 1.0)
{
   // normalize the "light" vector:

   l = l.normalized();
   for (int i=0; i<verts.num(); i++) {
      double a = pow(max(l * verts[i]->norm(), 0.0), s);
      if (a > 0)
         verts[i]->set_color(interp(verts[i]->color(), col, a), 1);
   }
}

inline void
color_verts(CBvert_list& verts)
{
   set_base_colors(verts, Color::blue2);

   add_shading(verts, Wvec( 1,  1,  1), Color::orange1, 2.0);
   add_shading(verts, Wvec(-1,  1, -1), Color::blue4,   1.0);
   add_shading(verts, Wvec( 0, -1,  0), Color::blue1,   1.0);
}

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

   if (Config::get_var_bool("JOT_PRINT_MESH"))
      mesh->print();

   color_verts(mesh->verts());

   mesh->write_stream(cout);

   return 0;
}

// end of file color_mesh.C
