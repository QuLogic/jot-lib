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
/***************************************************************************
    quad_cell.H
    
    QuadCell
      -2D array of Bverts, given in rows that run left to right, 
       from the bottom to the top.
      -Can access a quad(face) at a particular coords
      0,2---------1,2---------2,2
      |            |            |
      |            |            |
      |   Q 0,1    |   Q 1,1    |
      |            |            |
      |            |            |  
      0,1---------1,1---------2,1
      |            |            |
      |            |            |
      |   Q 0,0    |   Q 1,0    |
      |            |            |
      |            |            |  
      0,0---------1,0---------2,0
   -------------------
    Simon Breslav
    Fall 2004
 ***************************************************************************/
#ifndef QUAD_CELL_H_IS_INCLUDED
#define QUAD_CELL_H_IS_INCLUDED

#include "mesh/bmesh.H"
#include <vector>

class BaseStroke;
class Pattern3dStroke;
class PatternGrid;
class Stroke_List;
class Cell_List;

#define CQuadCell const QuadCell   
class QuadCell {
 public:
  
   //******** MANAGERS ********              
   QuadCell(PatternGrid*, CBface* quad);
   QuadCell(QuadCell*, PatternGrid*);
   bool     expand( CBface* ref_quad);
   bool     add_quad(CBface* quad);
   bool     delete_cell();
 
   //******* Neighbor Structure **********
   //Given an edge find a neighbor cell if one exists
   QuadCell*    find_neighbor_cell(CBedge* e);
   void         get_neighbors(ARRAY<Cell_List>&);   
   void         get_cell_boundery(ARRAY<Bedge_list>&);
   bool         has_unmarked_neighbor(); 
   void         get_unmarked_neighbor_faces(ARRAY<CBface*>&);
   bool         same_config(QuadCell*, int& number_same);
   //*********** MESH OPERATIONS*********     
   CBface_list& faces() const { return _faces; }
   // Do two vetexes make a strong edge
   bool   makes_strong_chain(Bvert* a, Bvert* b); 
   //actully modifies the faces to be adjacent
   CBedge* shared_cell_quad_edge(CBface*& face_in, CBface* const face_out);
   CBedge* shared_quad_quad_edge(CBface*& f, CBface*& f2);
   
   CBface* inside_face(CBedge* e);    //gets marked face, 0 if both marked  
   CBface* outside_face(CBedge* e);   //gets unmarked face, 0 if both marked
   
   //gets face from a neighbor cell, 0 if no cell
   // needs 'in' cell as input so that it knows what is in and out...
   CBface* outside_cell_face(CBedge* e);  

   
   // If there is an edge that is joint by two cells it returns the first one it finds
   CBedge*      joint_edge(QuadCell* start_cell);   
   // Given a list, does it have this edge in it?
   bool         list_contains_edge(CBvert_list& list, CBedge* e);  
   Bedge*       find_in_edge(int i, Pattern3dStroke* stroke);
   Bedge*       find_in_edge(int i, mlib::CUVpt_list& uv_stroke);
   
   //******** UV Translations ********  
   bool         uv_out_of_range(mlib::UVpt);
   // Given baracentic coords within quad get UV point
   mlib::UVpt         quad_bc_to_cellUV(mlib::CWvec& bc, Bface* f);
   // Intermediate step to get UV point from the quad
   mlib::UVpt         quad_bc_to_uv(mlib::CWvec& bc, Bface* f);
   CBface*      cellUV_to_face(mlib::UVpt p);
   mlib::Wpt          cellUV_to_loc(mlib::CUVpt& uv_p);
   mlib::Wpt          cellUV_to_loc_signed(mlib::CUVpt& uv_p);

   // Finds Bvert in the cell and gives it's UV location
   mlib::UVpt         bvert_to_uv(Bvert*)   const;
   // Finds UV point relative to some control cell
   mlib::UVpt         my_UV_to_controlCell_UV(mlib::UVpt uv_in, QuadCell* control_cell, CBedge* e);
   
   bool         my_UV_to_controlCell_UV(mlib::UVpt& ret, QuadCell*& neighbor_cell, int i, mlib::CUVpt_list& uv_stroke);  
   //******** Synthesis Related ********  
   double    difference_to_cell(QuadCell *);
   double    difference_neighbors(QuadCell *);
   
   //******** Stroke Related ********  
   bool                      is_empty() { return (groups_num() > 0 || stroke_num() > 0) ? false : true; }
   void                      add_stroke(Pattern3dStroke * s)
                                               { _3d_strokes.add(s); }
   void                      remove_stroke(Pattern3dStroke * s);                                                                                         
   int                       stroke_num()      { return _3d_strokes.num(); }
   Pattern3dStroke*          get_stroke(int i) const{ return _3d_strokes[i]; }
   CARRAY<Pattern3dStroke*>& get_strokes()     const{ return _3d_strokes; }
   // Gives all the strokes in one array, patter_texture askes this
   int                       groups_num()      { return _groups.num(); }
   Stroke_List*              get_groups_strokes(int i)      
                                               const { return _groups[i]; }
   double                    current_width(double start_width, 
                                           double pix_size);
   double                    current_alpha_frac(double pix_size, mlib::Wvec light, int l_num);                                               
   
   enum stroke_type {
      STRUCTURED_HATCHING=0,
      STURCTURED_CURVE,
      STIPPLING,
      OTHER,
      NULLLIST,
      GROUPNUM
   };
   
   // Get a spacific type of a group
   static void populate_list(std::vector<Stroke_List*>& list, int type, QuadCell *cell);
    
   void         make_groups();
   int          add_group(Stroke_List*);
   int          add_group_var(Stroke_List*); //with variation
   //XXX not implemented
   void         remove_group(int i);   
                                               
   //******** ACCESSORS ********
   bool         is_visible();
   bool         is_uv_continuous()const {return _is_uv_continuous; }
   bool         is_edge_uv_continuous()const {return _is_edge_uv_continuous; }
   PatternGrid* pg()       const{ return _pg; }
   
   // Vertex (i,j), where i designates the column and j designates the row:
   CBvert_list& row(int i) const { return _grid[i]; }
   CBvert_list& bottom()   const { return _grid.first(); }
   CBvert_list& top()      const { return _grid.last(); }
   Bvert_list   col(int i);
   
   double       get_du()   const { return _du;}
   double       get_dv()   const { return _dv;} 
   
   // Lower left vertex of the quad, assumes it's inside the cell
   Bface*       quad (int i, int j) const;     
        
   int          nrows() const { return _grid.num(); }
   int          ncols() const { return _grid.empty() ? 0 : bottom().num(); }
   Bvert*       vert(int i, int j)    const { return _grid[j][i]; }//{ return (i <= _ColsCache && j <= _RowsCache) ? _grid[j][i] : 0; }   
   mlib::CWpt&        loc (int i, int j)    const { return _grid[j][i]->loc(); }    
   mlib::UVpt         uv  (int i, int j)    const { return mlib::UVpt(i*_du, j*_dv); }
   bool         vert_uv(Bvert*, mlib::UVpt& uv_loc);
   CBedge*      get_edge(mlib::UVpt,mlib::UVpt);
   
   // XXX maybe I should interpulate the actual point...the center used for LOD
   mlib::CWpt&        center()      const { return _center; }
   mlib::CWpt&        loc_l_b ()    const { return loc(0,0); }
   mlib::CWpt&        loc_r_b ()    const { return loc(_ColsCache,0); }
   mlib::CWpt&        loc_l_t ()    const { return loc(0,_RowsCache); }
   mlib::CWpt&        loc_r_t ()    const { return loc(_ColsCache,_RowsCache); }     
   
   double       area() const;
   //********* Helper Function ******** 
   bool         is_ref()        { return _is_ref;  }
   void         toggle_ref()    { _is_ref = !_is_ref; }
   void         set_ref()       { _is_ref = 1; }   
   
   void         set_marked() { _marked = true; }
   void         set_unmarked() { _marked = false; }
   bool         is_marked() { return _marked; }   
   
   void         set_group_id(int id) { _group_id = id; }
   int          get_group_id() const { return _group_id; }
   
   mlib::Wtransf      get_xf() const { return _xf; }
   mlib::Wtransf      get_inv() const { return _inv_xf; }
 private:  
   ARRAY<Pattern3dStroke*>    _3d_strokes;    // The strokes withing the cell
   ARRAY<Stroke_List*>        _groups;
 
   ARRAY<Bvert_list>          _grid;      
   Bface_list                 _faces;
   int                        _RowsCache;     // numrows - 1
   int                        _ColsCache;     // numcols - 1
   double                     _du;            // change in u from one column to the next
   double                     _dv;            // change in v from one row to the next     
   bool                       _is_ref;        // is a reference cell
   PatternGrid*               _pg;   
   bool                       _marked;        // used for synthesis flood fill
   int                        _group_id;      // ID for when we divide
                                              // cells into groups to synthesize 
                                              // Grid pattern
   mlib::Wtransf                    _xf;
   mlib::Wtransf                    _inv_xf;
   mlib::Wpt                        _center;
   bool                       _is_uv_continuous;
   bool                       _is_edge_uv_continuous;
   //******** INTERNAL METHODS ********
   void                       cache();
   //******** Orientation *******
   //rotates the vertices cw
   void rotate_cell();   
   void flip_hor();  
   void flip_vert();   
   void orient_to_uv();    
   // Goes through all the quads within the cell and
   // makes sure it has correct info about the cell it has (CellData)
   void fix_up_cell_data();   
   
   // Finds an edge on the cell border and returns an int representing it's location
   //     2
   //   -----
   // 4 |   | 5
   //   -----
   //     1
   // numbering so that we can do abs(top-bottom) or abs(right-left) and get 1
   int          edge_location(CBedge* e); 
   
   // Given a Bvert_list on the edge of the cell give a new list not not in a cell
   // This is to extend the cell borders by one strip of quads
   Bvert_list&  opposite_edges(CBvert_list& list, Bvert_list& new_list); 
      
   //******** BASIC OPS ********
   bool         is_empty() const { return _grid.empty(); }
   // Returns true if vertices make a proper grid:
   bool         is_good() const;
   // Reset to empty grid:
   void         clear() {
      _grid.clear();
     _du = _dv = _RowsCache = _ColsCache = 0;
   }    
};

#define CCell_List const Cell_List   
class Cell_List : public ARRAY<QuadCell *>
{
 public:   
   // Do not overwrite the add function of the array so that things can bypass
   // this check...
   bool add_cell(QuadCell* cell){
      if(can_add(cell)){
         if(empty()){
            nrows = cell->nrows();
            ncols = cell->ncols();
         }
         add_uniquely(cell);        
         return true;
      }else{
         return false;
      }
   }
 private:
   bool can_add(QuadCell* cell)
   {
      if(empty()) return true;            
      return (cell && cell->nrows() == nrows && cell->ncols() == ncols) ? true : false;
   }    
   int nrows;
   int ncols;  
  
};
#endif // QUAD_CELL_H_IS_INCLUDED

/* end of file quad_cell.H */
