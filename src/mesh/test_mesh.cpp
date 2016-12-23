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
 *  used to test whatever mesh functionality is lately being developed.
 *  change the code as needed...
 *
 *  currently testing indexing in SimplexArrays
 *     ... and they do seem to work
 **********************************************************************/
#include "std/config.hpp"
#include "mi.hpp"

inline uint
num_data(CBsimplex* s)
{
   if (!(s && s->data_list()))
      return 0;
   return (uint)s->data_list()->size();
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
   cerr << "data counts for " << verts.size() << " vertices" << endl;
   for (size_t total=0, i=0, t=0; total < verts.size(); total += t, i++) {
      t = verts.filter(NumSimplexDataFilter(i)).size();
      cerr << "  " << i << ": " << t << endl;
   }
}

inline void
check_indices(CBvert_list& verts)
{
   for (size_t i=0; i<verts.size(); i++) {
      if (verts.get_index(verts[i]) != i) {
         cerr << "  index error: " << i << "/" << verts.size()
              << " recorded as " << verts.get_index(verts[i]) << endl;
         return;
      }
   }
   cerr << verts.size() << " indices okay" << endl;
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

   BMESHptr bmesh = make_shared<BMESH>();
   BMESHptr lmesh = make_shared<LMESH>();

   cerr << "dynamic cast from BMESHptr to BMESHptr does ";
   if (dynamic_pointer_cast<BMESH>(bmesh) == nullptr)
      cerr << "NOT ";
   cerr << "work" << endl;

   cerr << "dynamic cast from LMESHptr to LMESHptr does ";
   if (dynamic_pointer_cast<LMESH>(lmesh) == nullptr)
      cerr << "NOT ";
   cerr << "work" << endl;
   return 0;

   BMESHptr mesh = BMESH::read_jot_stream(cin);
   if (!mesh || mesh->empty())
      return 1; // didn't work


   Bvert_list verts = mesh->verts();
   Bvert_list bak   = verts;

   cerr << "loaded mesh" << endl;
   mesh->print();

   print_data_counts(verts);

   cerr << "verts reverse" << endl;
   std::reverse(verts.begin(), verts.end());

   cerr << "check indices" << endl;
   check_indices(verts);

   size_t n = verts.size();
   cerr << "removing " << n/2 << " verts" << endl;
   for (size_t i=0; i<n/2; i++) {
      Bvert* v = verts[i];
      uint b = num_data(v);     // before
      Bvert_list::iterator it = std::find(verts.begin(), verts.end(), v);
      verts.erase(it);
      uint a = num_data(v);     // after
      if (!(b == 1) && (a == 0)) {
         err_msg("before: %d, after: %d", b, a);
         break;
      }
   }

   cerr << "bak:" << endl;
   print_data_counts(bak);

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
