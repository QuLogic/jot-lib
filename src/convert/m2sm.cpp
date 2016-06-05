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
 * m2sm.C:
 * 
 *   convert from Hugues Hoppe's .m format to jot .sm format.
 *
 **********************************************************************/
#include "std/config.H"
#include "mesh/mi.H"

void
read_vert(Wpt_list& pts) 
{
  int num; // vert number 
  double x = 0, y = 0, z = 0;
  cin >> num >> x >> y >> z;

  if (num < 1) {
    cerr << "invalid vert num " << num << endl;
  } else if (num > 1e6) {
    cerr << "too scared to handle large vert num " << num << endl;
  } else {
    while ((int)pts.size() <= num) {
      pts.push_back(Wpt());
    }
    pts[num] = Wpt(x,y,z);
  }

  cin.ignore(256, '\n');
}

void
read_face(vector<Point3i>& faces)
{
  int num; // face number (ignored)
  int i = -1, j = -1, k = -1; // vert indices
  cin >> num >> i >> j >> k;
  if ( i>0 && j>0 && k>0 ) {
    faces.push_back(Point3i(i,j,k));
  } else {
    cerr << "error reading face " << num 
         << ": " << i << ", " << j << ", " << k << endl;
  }
  cin.ignore(256, '\n');
}

int 
main(int argc, char *argv[])
{
   if (argc != 1) {
      err_msg("Usage: %s < input.m", argv[0]);
      return 1;
   }

   Wpt_list pts;
   vector<Point3i> faces;

   char keyword[256];

   do {
     cin >> keyword;
     if ( !strcmp(keyword, "Vertex")) {
       read_vert(pts);
     } else if ( !strcmp(keyword, "Face")) {
       read_face(faces);
     } else {
       cin.ignore(256, '\n');
     }
     keyword[0] = 0; // start fresh
   } while (!cin.eof());   
   
   BMESHptr mesh = make_shared<BMESH>();
   
   for (Wpt_list::size_type i=1; i<pts.size(); i++) {
     mesh->add_vertex(pts[i]);
   }

   for (auto & face : faces) {
     mesh->add_face(face[0]-1,
                    face[1]-1,
                    face[2]-1);
   }

   int i;
   // remove any unused vertices
   for (i=mesh->nverts()-1; i>=0; i--) {
     if (mesh->bv(i)->degree() == 0) {
       mesh->remove_vertex(mesh->bv(i));
     }
   }

   mesh->changed();

   // Remove duplicate vertices while we're at it
   mesh->remove_duplicate_vertices(false); // don't keep the bastards

   bool is_bad = false;
   for (i=0; i<mesh->nedges(); i++)
      if (!mesh->be(i)->consistent_orientation())
         is_bad = true;
   if (is_bad)
      err_msg("Warning: inconsistently oriented triangles -- can't fix");

   // What the hell, recenter if they like:
   if (Config::get_var_bool("JOT_RECENTER"))
      mesh->recenter();

   if (Config::get_var_bool("JOT_PRINT_MESH"))
      mesh->print();

   mesh->write_stream(cout);

   return 0;
}
