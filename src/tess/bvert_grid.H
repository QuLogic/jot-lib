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
/*****************************************************************
 * bvert_grid.H
 *****************************************************************/
#ifndef BVERT_GRID_H_IS_INCLUDED
#define BVERT_GRID_H_IS_INCLUDED

#include "mesh/bmesh.H"

/*****************************************************************
 * BvertGrid
 *
 *      2D array of Bverts, given in rows that run left to right,
 *      from the bottom to the top.
 *****************************************************************/
class BvertGrid {
 public:
   //******** MANAGERS ********
   BvertGrid() : _RowsCache(0), _ColsCache(0), _du(0), _dv(0) {}

   //******** ACCESSORS ********

   // dimensions
   int nrows() const { return _grid.num(); }
   int ncols() const { return _grid.empty() ? 0 : bottom().num(); }

   // Vertex (i,j), where i designates the column and j
   // designates the row:
   Bvert* vert(int i, int j)    const { return _grid[j][i]; }
   CWpt&  loc (int i, int j)    const { return _grid[j][i]->loc(); }
   UVpt   uv  (int i, int j)    const { return mlib::UVpt(i*_du, j*_dv); }

   BMESH* mesh()                const { return _mesh; }

   // rows
   CBvert_list& row(int j) const { return _grid[j]; }
   CBvert_list& bottom()   const { return _grid.first(); }
   CBvert_list& top()      const { return _grid.last(); }

   // columns
   Bvert_list   col(int i) const;
   Bvert_list   left()     const { return col(0); }
   Bvert_list   right()    const { return col(ncols()-1); }

   // horizontal band of faces between row j and j+1:
   Bface_list hband(int j) const;

   // vertical band of faces between col i and i+1:
   Bface_list vband(int i) const;

   //******** BUILDING ********

   // Reset to empty grid:
   void clear() {
      _grid.clear();
      _du = _dv = _RowsCache = _ColsCache = 0;
      _mesh = 0;
   }

   // Vertices of bottom and top run left to right.
   // On the left and right they run bottom to top.
   bool build(CBvert_list& bottom, CBvert_list& top,
              CBvert_list& left,   CBvert_list& right);

   // Assign interior vertex positions by interpolating from
   // boundary positions. (Uses definition of Coons patch):
   bool interp_boundary();

   // Add quad from vert(i-1,j-1) to vert(i,j):
   bool add_quad(int i, int j, Patch* p);

   // Add all quads:
   bool add_quads(Patch* p);
   
   //******** DIAGNOSTIC ********

   bool is_empty() const { return _grid.empty(); }

   // Returns true if vertices make a proper grid:
   bool is_good() const {
      if (nrows() < 2 || ncols() < 2)
         return false;
      // check that each row is the same size
      for (int i=1; i<nrows(); i++)
         if (row(i).num() != row(i-1).num())
            return false;
      return true;
   }

 protected:

   // vertex grid:
   ARRAY<Bvert_list>    _grid;

   // cached values (after grid is built):
   int          _RowsCache;     // numrows - 1
   int          _ColsCache;     // numcols - 1
   double       _du;            // change in u from one column to the next
   double       _dv;            // change in v from one row to the next
   BMESH*       _mesh;

   //******** INTERNAL METHODS ********

   // Compute cached values:
   void cache();
};

#endif // BVERT_GRID_H_IS_INCLUDED

/* end of file bvert_grid.H */
