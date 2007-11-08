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
 * qinv.C:
 *
 *   Do quasi-interlopolation using Loop subdivision
 **********************************************************************/
#include "std/config.H"
#include "mi.H"
#include "lmesh.H"

inline Wpt
qinv_loc(CBvert* v)
{
   return v->loc() + (v->loc() - LoopLoc().limit_val(v));
}

inline Wpt_list
qinv_locs(LMESHptr mesh)
{
   assert(mesh);
   Wpt_list ret(mesh->nverts());
   for (int i=0; i<mesh->nverts(); i++) {
      ret += qinv_loc(mesh->bv(i));
   }
   return ret;
}

inline Wpt_list
limit_locs(LMESHptr mesh)
{
   assert(mesh);
   Wpt_list ret(mesh->nverts());
   for (int i=0; i<mesh->nverts(); i++) {
      ret += LoopLoc().limit_val(mesh->bv(i));
   }
   return ret;
}

inline void
set_locs(LMESHptr mesh, CWpt_list& pts)
{
   assert(mesh && mesh->nverts() == pts.num());

   for (int i=0; i<mesh->nverts(); i++)
      mesh->bv(i)->set_loc(pts[i]);
   mesh->changed(BMESH::VERT_POSITIONS_CHANGED);
}

int 
main(int argc, char *argv[])
{
   // if -l flag is used, output limit locations
   bool do_limit = false;
   if (argc == 2 && str_ptr("-l") == argv[1]) {
      do_limit = true;
   } else if (argc != 1) {
      err_msg("Usage: %s < mesh.sm > mesh-qinv.sm", argv[0]);
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
   if (do_limit) {
      cerr << "outputting limit positions" << endl;
      set_locs(mesh, limit_locs(mesh));
   } else {
      set_locs(mesh, qinv_locs(mesh));
   }

   mesh->write_stream(cout);

   return 0;
}

// end of file qinv.C
