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
 * test_mesh.C:
 *
 *  used to test whatever mesh functionality is lately being developed.
 *  change the code as needed...
 *
 *  currently testing indexing in SimplexArrays
 *     ... and they do seem to work
 *
 **********************************************************************/
#include "std/config.H"
#include "mi.H"

inline uint
num_data(CBsimplex* s)
{
   if (!(s && s->data_list()))
      return 0;
   return (uint)s->data_list()->num();
}

/*****************************************************************
 * NumSimplexDataFilter:
 *
 *   Accept a simplex if it contains a specific number
 *   of SimplexData pointers.
 *
 *****************************************************************/
class NumSimplexDataFilter : public SimplexFilter {
 public:
   NumSimplexDataFilter(uint num) : _num(num) {}

   virtual bool accept(CBsimplex* s) const {
      return (s && num_data(s) == _num);
   }

 protected:
   uint _num;
};
typedef const NumSimplexDataFilter CNumSimplexDataFilter;

inline void
print_data_counts(CBvert_list& verts)
{
   cerr << "data counts for " << verts.num() << " vertices" << endl;
   for (uint total=0, i=0, t=0; total < (uint)verts.num(); total += t, i++) {
      t = verts.filter(NumSimplexDataFilter(i)).num();
      cerr << "  " << i << ": " << t << endl;
   }
}

inline void
check_indices(CBvert_list& verts)
{
   for (int i=0; i<verts.num(); i++) {
      if (verts.get_index(verts[i]) != i) {
         cerr << "  index error: " << i << "/" << verts.num()
              << " recorded as " << verts.get_index(verts[i]) << endl;
         return;
      }
   }
   cerr << verts.num() << " indices okay" << endl;
}

/*****************************************************************
 * main
 *****************************************************************/
int 
main(int argc, char *argv[])
{
   if (argc != 1) {
      err_msg("Usage: %s < mesh.sm", argv[0]);
      return 1;
   }

   BMESH* bmesh = new BMESH;
   BMESH* lmesh = new LMESH;

   cerr << "dynamic cast from BMESH* to BMESH* does ";
   if (dynamic_cast<BMESH*>(bmesh) == NULL)
      cerr << "NOT ";
   cerr << "work" << endl;

   cerr << "dynamic cast from LMESH* to LMESH* does ";
   if (dynamic_cast<LMESH*>(lmesh) == NULL)
      cerr << "NOT ";
   cerr << "work" << endl;
   return 0;

   BMESHptr mesh = BMESH::read_jot_stream(cin);
   if (!mesh || mesh->empty())
      return 1; // didn't work


   Bvert_list verts = mesh->vert_list();
   Bvert_list bak   = verts;

   cerr << "loaded mesh" << endl;
   mesh->print();

   print_data_counts(verts);

   cerr << "begin index" << endl;
   verts.begin_index();

   print_data_counts(verts);

   cerr << "verts reverse" << endl;
   verts.reverse();

   cerr << "check indices" << endl;
   check_indices(verts);

   int n = verts.num();
   cerr << "removing " << n/2 << " verts" << endl;
   for (int i=0; i<n/2; i++) {
      Bvert* v = verts[i];
      uint b = num_data(v);     // before
      verts -= v;
      uint a = num_data(v);     // after
      if (!(b == 1) && (a == 0)) {
         err_msg("before: %d, after: %d", b, a);
         break;
      }
   }

   print_data_counts(verts);

   cerr << "bak:" << endl;
   print_data_counts(bak);

   cerr << "bak with indexing:" << endl;
   bak.begin_index();
   print_data_counts(bak);

   cerr << "turning off indexing" << endl;
   verts.end_index();

   cerr << "verts: " << endl;
   print_data_counts(verts);

   cerr << "verts = bak" << endl;
   verts = bak; 
   print_data_counts(verts);

   cerr << "bak.clear()" << endl;
   bak.clear();
   print_data_counts(verts);
   
   return 0;
}

// end of file test_mesh.C
