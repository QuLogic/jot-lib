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
 * bvert_grid.C:
 **********************************************************************/
#include "bvert_grid.H"

#include <iterator>

bool
BvertGrid::build(
   CBvert_list& bottom,         // bottom row
   CBvert_list& top,            // top row
   CBvert_list& left,           // left column
   CBvert_list& right           // right column
   )
{
   // Vertices of bottom and top run left to right.
   // On the left and right they run bottom to top.

   // Check everything is righteous:
   if (bottom.size() < 2                ||
       bottom.size() != top.size()      ||
       left.size() < 2                  ||
       left.size() != right.size()      ||
       bottom.front() != left.front()   ||
       bottom.back()  != right.front()  ||
       top.front()    != left.back()    ||
       top.back()     != right.back()   ||
       !bottom.same_mesh()              ||
       !top.same_mesh()                 ||
       !left.same_mesh()                ||
       !right.same_mesh()               ||
       bottom.mesh() == nullptr) {
      err_msg("BvertGrid::build: can't deal with CRAP input");

      std::ostream_iterator<Bvert*> err_it (std::cerr, ", ");

      cerr << "bottom: ";
      std::copy(bottom.begin(), bottom.end(), err_it);
      cerr << endl;

      cerr << "top:    ";
      std::copy(top.begin(), top.end(), err_it);
      cerr << endl;

      cerr << "left:   ";
      std::copy(left.begin(), left.end(), err_it);
      cerr << endl;

      cerr << "right:  ";
      std::copy(right.begin(), right.end(), err_it);
      cerr << endl;

      return false;
   }

   // Wipe the old:
   clear();

   // Build the new...
   //   bottom row:
   _grid.push_back(bottom);

   BMESHptr m = bottom.mesh();    assert(m);

   // Internal rows:
   for (Bvert_list::size_type j=1; j<left.size()-1; j++) {
      Bvert_list row;                    // vertices for row j
      row.push_back(left[j]);            // add first vertex for row j
      for (Bvert_list::size_type i=1; i<bottom.size()-1; i++)
         row.push_back(m->add_vertex()); // add internal vertices
      row.push_back(right[j]);           // add last vertex for row j
      _grid.push_back(row);
   }

   // top row:
   _grid.push_back(top);

   // Now compute cached values:
   cache();

   return true;
}

void
BvertGrid::cache()
{
   assert(is_good());

   _RowsCache = nrows() - 1;
   _ColsCache = ncols() - 1;
   _mesh = bottom().mesh();

   assert(_RowsCache > 0 && _ColsCache > 0 && _mesh != nullptr);

   _du = 1.0/_ColsCache;
   _dv = 1.0/_RowsCache;
}

bool
BvertGrid::interp_boundary()
{
   // Assign positions of interior vertices by interpolating from
   // boundary positions. Uses a Coons patch as defined in:
   //
   //     Gerald Farin.
   //     Curves and Surfaces for CAGD, 3rd Ed., Section 20.2.
   //     (pp. 365-368).

   if (!is_good())
      return false;

   // Iterate over interior verts:
   for (int j=1; j<_RowsCache; j++) {
      double v = j*_dv;
      for (int i=1; i<_ColsCache; i++) {
         double u = i*_du;
         vert(i,j)->set_loc(
            interp(loc(i,0), loc(i,_RowsCache), v) +            // Rc +
            (interp(loc(0,j), loc(_ColsCache,j), u) -           // Rd -
             interp(                                    // Rcd
                interp(loc( 0,0), loc( 0,_RowsCache), v),
                interp(loc(_ColsCache,0), loc(_ColsCache,_RowsCache), v), u))
            );
      }
   }
   return true;
}

bool
BvertGrid::add_quad(int i, int j, Patch* p)
{
   Bface* q = 
      _mesh->add_quad(
         vert(i-1, j-1),
         vert(i  , j-1),
         vert(i  , j  ),
         vert(i-1, j  ),
         uv  (i-1, j-1),
         uv  (i  , j-1),
         uv  (i  , j  ),
         uv  (i-1, j  ),
         p);
   return q != nullptr;
}

bool
BvertGrid::add_quads(Patch* p)
{
   // Create quads with uv-coords

   if (!is_good())
      return false;

   bool ret = true;
   for (int j=1; j<=_RowsCache; j++)
      for (int i=1; i<=_ColsCache; i++)
         if (!add_quad(i, j, p))
            ret = false;

   return ret;
}

Bvert_list
BvertGrid::col(int i) const
{
   Bvert_list ret;
   for (int j=0; j<nrows(); j++)
      ret.push_back(vert(i,j));
   return ret;
}

Bface_list
BvertGrid::hband(int j) const
{
   // horizontal band of faces between row j and j+1:

   // not necessarily the most efficient, but easy and correct:
   return row(j).one_ring_faces().intersect(row(j+1).one_ring_faces());
}

Bface_list
BvertGrid::vband(int i) const
{
   // vertical band of faces between col i and i+1:

   // not necessarily the most efficient, but easy and correct:
   return col(i).one_ring_faces().intersect(col(i+1).one_ring_faces());
}

// end of file bvert_grid.C
