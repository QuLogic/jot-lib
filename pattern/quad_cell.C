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
    quad_cell.C
    
    QuadCell
   -------------------
    Simon Breslav
    Fall 2004
***************************************************************************/
#if (defined (WIN32) && defined(_MSC_VER) && (_MSC_VER <=1300)) /*VS 6.0*/
#pragma warning(disable: 4786)
#endif

#include "pattern/pattern_pen.H"
#include "std/config.H"
#include "stroke/base_stroke.H"
#include "mesh/mi.H"
#include "mesh/uv_data.H"

#include <vector>
#include <map>
#include <set>
#include <algorithm>


#include "quad_cell.H"
#include "pattern_texture.H"
#include "pattern_stroke.H"

using namespace mlib;

#define GROUP_THRESHOLD 0.1
static bool debug_cell = Config::get_var_bool("DEBUG_CELL",false,true);


 //******** MANAGERS ******** 
QuadCell::QuadCell(PatternGrid* pg_, CBface* quad) 
    : _RowsCache(0),_ColsCache(0), _du(0), _dv(0), _is_ref(false), _pg(pg_), _marked(false)
{     
   assert(quad->is_quad());
  
   Bvert *a, *b, *c, *d;
   quad->get_quad_verts(a, b, c, d);
   
   Bvert_list row_1(2);
   row_1 += a;
   row_1 += b;
   Bvert_list row_2(2);
   row_2 += d;
   row_2 += c;
   
   _grid += row_1;
   _grid += row_2;
   
   cache();   
   orient_to_uv();  
   fix_up_cell_data(); 
}
QuadCell::QuadCell(QuadCell* old_cell, PatternGrid* pg)
   : _RowsCache(0),_ColsCache(0), _du(0), _dv(0), _is_ref(false), _pg(pg), _marked(false)
{  
   _grid = old_cell->_grid;
   cache();  
   fix_up_cell_data();    
}

bool
QuadCell::expand(CBface* ref_quad) 
{   
   
//    if(ncols() > 2 || nrows() > 2)
//       return 0;
   
   CellData* cd = CellData::lookup(ref_quad, _pg);
   if(!cd)
      return 0;
   
   int col = cd->get_cell_col();
   int row = cd->get_cell_row();
   QuadCell* ref_cell = cd->get_cell();
   
   int i; 
   // Expand to the right
   for(i = col; i < ref_cell->_ColsCache-1; ++i){
      CBedge* edge = lookup_edge(vert(_ColsCache, 0), vert(_ColsCache, 1));
      if(edge){
         CBface* new_face = outside_face(edge);
         if(new_face)
            add_quad(new_face);
         else
            return 0;
      }
   }
   
   // Expand to the left
   for(i = col; i > 0; --i){
      CBedge* edge = lookup_edge(vert(0, 0), vert(0, 1));
      if(edge){
         CBface* new_face = outside_face(edge);
         if(new_face)
            add_quad(new_face);
         else
            return 0;
      }
   }
   // Expand up
   for(i = row; i < ref_cell->_RowsCache-1; ++i){
      CBedge* edge = lookup_edge(vert(0, _RowsCache), vert(1, _RowsCache));
      if(edge){
         CBface* new_face = outside_face(edge);
         if(new_face)
            add_quad(new_face);
         else
            return 0;
      }
   }
   // Expand down
    for(i = row; i > 0; --i){
      CBedge* edge = lookup_edge(vert(0, 0), vert(1, 0));
      if(edge){
         CBface* new_face = outside_face(edge);
         if(new_face)
            add_quad(new_face);
         else
            return 0;
      }
    }
    return 1;
}

bool
QuadCell::add_quad(CBface* quad)
{
   int i;
   if(!quad->is_quad())
      return 0;
   
   // Get a shared edge
   CBface* this_quad;   
   CBedge* e = shared_cell_quad_edge(this_quad, quad);
     
   //Decide which way to extend
   try {
      if (list_contains_edge(bottom(),e)) {           
         Bvert_list new_row;
         opposite_edges(bottom(), new_row);
         ARRAY<Bvert_list> tmp_grid(_grid);
         clear();
         _grid += new_row;
         for(i=0; i < tmp_grid.num(); ++i){
            _grid += tmp_grid[i];      
         } 
      } else if (list_contains_edge(top(),e)) {       
         Bvert_list new_row;                 
         opposite_edges(top(), new_row);
         _grid += new_row; 
        
      } else if (list_contains_edge(col(_ColsCache),e)) {             
         Bvert_list new_col;                 
         opposite_edges(col(_ColsCache), new_col);
         for(i=0; i <= _RowsCache; ++i){
           _grid[i] += new_col[i];       
         } 
      } else if (list_contains_edge(col(0),e)) {      
         Bvert_list new_col;                 
         opposite_edges(col(0), new_col);
         ARRAY<Bvert_list> tmp_grid(_grid);
         //clear();
         for(i=0; i <= _RowsCache; ++i){
            _grid[i][0] = new_col[i];             
         }
         for(i=0; i <= _RowsCache; ++i){
            for(int j=1; j <= _ColsCache; ++j){
              _grid[i][j] = tmp_grid[i][j-1];
            }    
         }
         for(i=0; i <= _RowsCache; ++i){
            _grid[i] += tmp_grid[i][_ColsCache];           
         }       
      } else {
         err_msg("Can not extend");  
         return false;
      } 
   } catch (str_ptr str){
      WORLD::message(str);
      return false;
   }   
   cache();  
   //orient_to_uv();
   fix_up_cell_data(); 
   return true;         
}
bool 
QuadCell::delete_cell()
{
   if(groups_num() > 0 || stroke_num() > 0){
      return false;
   }
   for(int j=0; j < _RowsCache; ++j){
      for(int i=0; i < _ColsCache; ++i){        
         Bface* f = quad(i, j);
         if(f){              
           CellData *cd = CellData::lookup(f, _pg);
           if(cd){
             delete cd;
           }
         }         
      }
   }
   clear();
   delete this;
   return true;
}

//******* Neighbor Structure **********
QuadCell*
QuadCell::find_neighbor_cell(CBedge* e)
{
   assert(e);
   
   if(edge_location(e) == 0)
       return 0;

   CellData *cd  = CellData::lookup(e->f1(), _pg);
   CellData *cd2 = CellData::lookup(e->f2(), _pg);
   
   if(!cd || !cd2)
      return 0;
   
   //just in case make sure two cells are differnt
   if(cd->get_cell() == cd2->get_cell())
      return 0;
   
   return (cd->get_cell() == this) ? cd2->get_cell() : cd->get_cell();
       
}

void      
QuadCell::get_neighbors(ARRAY<Cell_List>& neighbor_list)
{
   neighbor_list.clear();
   QuadCell* neighbor;
   ARRAY<Bedge_list> list(4);
   get_cell_boundery(list); 
   
    //*** Get my neighbors ***//
   for(int i=0; i < 4; ++i){        
      Cell_List neighbors;
	   for(int j=0; j < list[i].num(); ++j){
          Bface* f = list[i][j]->f1();
          Bface* f2 = list[i][j]->f2();
          CellData *cd = CellData::lookup(f, _pg);
          CellData *cd2 = CellData::lookup(f2, _pg); 
         
          if(cd && cd2){            
            neighbor = (cd->get_cell() == this) ? cd2->get_cell() : cd->get_cell();
            neighbors.add_uniquely(neighbor);            
          }           
       }
       neighbor_list += neighbors;      
   }
}

void 
QuadCell::get_cell_boundery(ARRAY<Bedge_list>& list)
{
   list += top().get_chain();
   list += bottom().get_chain();
   list += col(0).get_chain();
   list += col(_ColsCache).get_chain();
   assert(list.num() == 4);   
}

bool         
QuadCell::has_unmarked_neighbor()
{
   ARRAY<Bedge_list> list;
   get_cell_boundery(list);
   for(int i=0; i < list.num(); ++i){
       for(int j=0; j < list[i].num(); ++j){
          if(!PatternPen::AUTO_GRID_SYNTH){ 
             if(UVdata::is_continuous(list[i][j])){
                if(outside_face(list[i][j]))
                   return true;
             }
          }else{
             if(outside_face(list[i][j]))
                return true;
          }
       }
   }
   return false;
   
}
void         
QuadCell::get_unmarked_neighbor_faces(ARRAY<CBface*>& list)
{
   ARRAY<Bedge_list> edge_list;
   get_cell_boundery(edge_list);
   CBface* f;
   for(int i=0; i < edge_list.num(); ++i){
       for(int j=0; j < edge_list[i].num(); ++j){
          if(!PatternPen::AUTO_GRID_SYNTH){ 
             if(UVdata::is_continuous(edge_list[i][j])){
                if((f = outside_face(edge_list[i][j])))
                   list += f;
             }
          }else{
             if((f = outside_face(edge_list[i][j])))
                list += f;
          }         
       }
   }
}
bool         
QuadCell::same_config(QuadCell* cell, int& number_same)
{
   ARRAY<Bedge_list> my_edge_list(4);
   ARRAY<Bedge_list> ref_edge_list(4);
   get_cell_boundery(my_edge_list);
   cell->get_cell_boundery(ref_edge_list);
   for(int i=0; i < 4; ++i){
      int my_num = my_edge_list[i].num();      
      int ref_num = ref_edge_list[i].num();
      //cerr << "my_num " << my_num << endl;
      //cerr << "ref_num " << ref_num << endl;
      
      if(my_num != ref_num){
         cerr << "wrong num" << endl;
         return false; //they are wrong type
      }
      //int my_unique_cells = 0;
      //int ref_unique_cells = 0;
      QuadCell* prev_my_n;
      QuadCell* prev_ref_n;  
      
      for(int j = 0; j < my_num; ++j){                
         QuadCell* my_n;
         QuadCell* ref_n;
                  
         if((my_n = find_neighbor_cell(my_edge_list[i][j]))){                   
            if((ref_n = cell->find_neighbor_cell(ref_edge_list[i][j]))){
               if(my_n->get_group_id() != ref_n->get_group_id()){
                  cerr << "wrong config " << my_n->get_group_id() << " " << my_n->get_group_id() << endl;
                  return false;         
               
               }else {
                  number_same++;
               }
            }
            //else{
            //   cerr << "not enouth info" << endl;
            //   return false;
            //}
         }
        
         if(j != 0 && my_n && ref_n){
            if(my_n != prev_my_n){
               if(ref_n == prev_ref_n){
                  cerr << "wrong number of cells" << endl;
                  return false;  
               }
            } else {
               if(ref_n != prev_ref_n){
                  cerr << "wrong number of cells" << endl;
                  return false;  
               }
            }               
         }
         prev_my_n = my_n;
         prev_ref_n = ref_n;
      }         
   }
   
   return true;   
}
//*********** MESH OPERATIONS********* 
CBedge* 
QuadCell::shared_cell_quad_edge(CBface*& face_in, CBface* const face_out)
{
   CBedge* e;
   for(int j=0; j < nrows()-1; ++j){
      for(int i=0; i < ncols()-1; ++i){        
         face_in = quad(i, j);
         if(!face_in) return 0;         
         CBface* f = face_out;
         if((e = shared_quad_quad_edge(face_in, f)))        
            return e;
      }
   }
   return 0;   
}

CBedge* 
QuadCell::shared_quad_quad_edge(CBface*& f, CBface*& f2) 
{
   if(!f->is_quad() || !f->is_quad())
      return 0;
   
   CBedge * e = (f->shared_edge(f2));
   if(e){ 
      return e;
   }
   CBedge * e1 = (f->shared_edge(f2->quad_partner()));
   if(e1){ 
      f2 = f2->quad_partner(); 
      return e1;
   }
   CBedge * e2 = (f->quad_partner()->shared_edge(f2));
   if(e2){ 
      f = f->quad_partner(); 
      return e2;
   }
   CBedge * e3 = (f->quad_partner()->shared_edge(f2->quad_partner()));
   if(e3){ 
      f = f->quad_partner(); 
      f2 = f2->quad_partner(); 
      return e3;
   } else {  
      return 0;
   }
}     

CBface* 
QuadCell::inside_face(CBedge* e)
{
   if(CellData::was_marked(e->f1(), _pg) && CellData::was_marked(e->f2(), _pg))
      return 0;   
   CBface* f = (CellData::was_marked(e->f1(), _pg)) ? e->f1() : e->f2();
   return (f->is_quad()) ? f : 0;   
}

CBface* 
QuadCell::outside_face(CBedge* e)
{  
   if(CellData::was_marked(e->f1(), _pg) && CellData::was_marked(e->f2(), _pg))
      return 0;
   
   CBface* f = (CellData::was_marked(e->f1(), _pg)) ? e->f2() : e->f1();
   return (f->is_quad()) ? f : 0;
}

CBface* 
QuadCell::outside_cell_face(CBedge* e)
{ 
  CellData* cd;
  CellData* cd2;
  if(!e || !e->f1() || !e->f2())
     return 0;
  
  if(!(cd = CellData::lookup(e->f1(), _pg)) || !(cd2 = CellData::lookup(e->f2(), _pg)))
      return 0;
  if(CellData::same_cells(e->f1(), e->f2(), _pg))
      return 0;     
  QuadCell* cell_1 = cd->get_cell();
  
  CBface* f = (cell_1 == this) ? e->f1() : e->f2(); 
  
  CBface* f_r =  (CellData::same_cells(f, e->f1(), _pg)) ? e->f2() : e->f1();
  return (f_r->is_quad()) ? f_r : 0;
}

bool 
QuadCell::list_contains_edge(CBvert_list& list, CBedge* e)
{
    Bedge_list l = list.get_chain();
    for(int i=0; i < l.num(); ++i){
      if(l[i] == e)
         return true;   
   }
   return false;

}

//******** UV Translation *************//
bool
QuadCell::uv_out_of_range(UVpt p)
{
   if (p[0] > 1.0 || p[0] < 0.0 || p[1] > 1.0 || p[1] < 0.0)
      return true;
   else
      return false;
}


UVpt
QuadCell::bvert_to_uv(Bvert* v)   const
{
   for(int j=0; j <= _RowsCache; ++j){
      for(int i=0; i <= _ColsCache; ++i){
         if(v == vert(i, j))
            return uv(i, j);
      }      
   }
   assert(0);
   return UVpt(0,0); 
}

 
UVpt 
QuadCell::quad_bc_to_cellUV(CWvec& bc, Bface* f)
{ 
   CellData* cd = CellData::lookup(f, _pg);
   assert(cd);

   int col = cd->get_cell_col();
   int row = cd->get_cell_row();

   UVpt uv_p = quad_bc_to_uv(bc,f);
   double u = col * _du;
   double v = row * _dv;   
   UVpt point(u + ((uv_p[0]) * _du), v + ((uv_p[1]) * _dv));
   return point;
}

UVpt
QuadCell::quad_bc_to_uv(CWvec& bc, Bface * f)
{ 
   // We got some barycentric coords WRT this face (a quad),
   // and now we want to convert them to uv-coords WRT to
   // the 4 quad vertices in standard order.
   CellData* cd = CellData::lookup(f, _pg);
   assert(cd);

   int col = cd->get_cell_col();
   int row = cd->get_cell_row();

   UVpt p;
   Bvert *a=0, *b=0, *c=0, *d=0;
   if (!f->get_quad_verts(a, b, c, d)) {
      err_msg("Bface::quad_bc_to_uv: Error: can't get quad verts");
      return UVpt();
   }  
   // Barycentric coords are given WRT to vertices v1, v2, v3.
   // We want them WRT:
   //   a, b, c (lower face), or
   //   a, c, d (upper face).
   //
   // k = 0, 1, or 2 according to whether a = v1, v2, or v3.
    
   int k = f->vindex(a) - 1;
   if (k < 0 || k > 2) {
      err_msg("Bface::quad_bc_to_uv: Error: can't get oriented");
      return UVpt();
   }
   
   if (f->contains(b)) {
      // We are the lower face.
      p = UVpt(1 - bc[k], bc[(k + 2)%3]);
   } else if (f->contains(d)) {
      // We are the upper face.
      p = UVpt(bc[(k + 1)%3], 1 - bc[k]);
   } else {
      // Should not happen
      err_msg("Bface::quad_bc_to_uv: Error: can't get oriented");
      return UVpt();
   }
   
   //Now let reverse if needed to orienate to the grid  
   if(a == vert(col, row)){
      return p;
   } else if(a == vert(col+1, row+1)) {
      // Reversed both
     return UVpt(1-p[0], 1-p[1]);
   } else if(a ==  vert(col+1, row)) {
     return UVpt(p[1], 1-p[0]);      
   } else if(a ==  vert(col, row+1)) {
     return UVpt(1-p[0], p[0]);      
   } else {
      err_msg("You gave me col and row wrong, :(");
      return UVpt(0,0);
   }
   
}

Wpt   
QuadCell::cellUV_to_loc(CUVpt& uv_p)
{
   
   //tmp = interp(interp(a, b, u),  interp(d, c, u), v);
   double u = uv_p[0];
   double v = uv_p[1];
   
   double i_d = (u / _du);
   double j_d = (v / _dv);
   
   int i = (int)(i_d);
   int j = (int)(j_d);  
   //clamp the coords to the
   i = clamp(i, 0, _ColsCache-1);
   j = clamp(j, 0, _RowsCache-1);   
 
   Bface* f = quad(i, j);
   
   // Now we have to make the uv compatable with quad's uv...
   UVpt quad_uv; 
   double q_u = i_d - i;
   double q_v = j_d - j;
   
   Bvert *a=0, *b=0, *c=0, *d=0;
   
   if(!f || !f->is_quad())
      throw str_ptr("QuadCell::cellUV_to_loc : face no good or not a quad"); 
   
   f->get_quad_verts(a, b, c, d);   
   
   if(a == vert(i, j)){
      //cerr << "a is 0,0" << endl;
      quad_uv = UVpt(q_u, q_v);
   } else if(a == vert(i+1, j+1)) {
     //cerr << "a is 1,1" << endl;      
     quad_uv = UVpt(1-q_u, 1-q_v);
   } else if(a ==  vert(i+1, j)) {
     //cerr << "a is 1,0" << endl; 
     quad_uv = UVpt(q_v, 1-q_u);      
   } else if(a ==  vert(i, j+1)) {
     //cerr << "a is 0,1" << endl;
     quad_uv = UVpt(1-q_v, q_u);  
   } else {
     throw str_ptr("QuadCell::cellUV_to_loc : could not convert uv to quad uv");  
   }
   
   if(!f){    
     cerr << "j i is" << j << " " << i << " with " << u << " " << v << endl;
     throw str_ptr("QuadCell::cellUV_to_loc : could not find new face");                           
   } 
   
   return f->quad_uv2loc(quad_uv);
  
}

CBface*      
QuadCell::cellUV_to_face(UVpt uv_p)
{  
   /*Wpt pt = cellUV_to_loc(p);   
   for(int i=0; i < _faces.num(); ++i){
      Wvec bc;
      _faces[i]->project_barycentric(pt,bc);
      //err_msg("bc here is %f %f %f",bc[0],bc[1],bc[2]);
      if(bc[0] >= 0.0 && bc[1] >= 0.0 && bc[2] >= 0.0)
         return _faces[i];     
   }*/
   double u = uv_p[0];
   double v = uv_p[1];
   
   int i = (int)(u / _du);
   int j = (int)(v / _dv); 
   i = clamp(i, 0, _ColsCache-1);
   j = clamp(j, 0, _RowsCache-1);      
 
   Bface* f = quad(i, j);
   //does not matter which face of the quad, visibility is happy with ether...
   return f; 

}


//   v
//   ^
//   |
// l |
//   ------>u
//      b
// This requires that all the cells have same orientation
UVpt
QuadCell::my_UV_to_controlCell_UV(UVpt uv_in, QuadCell* c_cell, CBedge* e)
{         
     UVpt new_uv;

     if(!e)
       throw str_ptr("QuadCell::my_UV_to_controlCell_UV : no edge");;
     
     UVpt a_c = c_cell->bvert_to_uv(e->v1());
     UVpt b_c = c_cell->bvert_to_uv(e->v2());
     UVpt a = bvert_to_uv(e->v1());
     UVpt b = bvert_to_uv(e->v2());   

    // cerr << "points are " << a_c << "to  "<< a << " " << b_c << "  to " << b << endl;
     double cell_slide;       
    
     // if edge is  bottom
     if (c_cell->list_contains_edge(c_cell->bottom(), e)){           
          cell_slide = a_c[0] - (( (b_c[0]- a_c[0]) / (b[0] - a[0]) ) * a[0]);
          new_uv[0] = ( ( (b_c[0]- a_c[0]) / (b[0] - a[0]) ) * uv_in[0] ) + cell_slide;
          new_uv[1] = uv_in[1]-1;
     // if edge is top
     } else if (c_cell->list_contains_edge(c_cell->top(), e)) {
          cell_slide = a_c[0] - (( (b_c[0]- a_c[0]) / (b[0] - a[0]) ) * a[0]);
          new_uv[0] = ( ( (b_c[0]- a_c[0]) / (b[0] - a[0]) ) * uv_in[0] ) + cell_slide;
          new_uv[1] = uv_in[1]+1;
     // if it's left
     } else if (c_cell->list_contains_edge(c_cell->col(0), e)) {
          
        cell_slide = a_c[1] - (( (b_c[1]- a_c[1]) / (b[1] - a[1]) ) * a[1]);
          new_uv[0] = uv_in[0]-1;
          new_uv[1] = ( ( (b_c[1]- a_c[1]) / (b[1] - a[1]) ) * uv_in[1] ) + cell_slide;
     //if it's right
     } else if (c_cell->list_contains_edge(c_cell->col(c_cell->ncols()-1), e)) {
       
        cell_slide = a_c[1] - (( (b_c[1]- a_c[1]) / (b[1] - a[1]) ) * a[1]);   
          new_uv[0] = uv_in[0]+1;
          new_uv[1] = ( ( (b_c[1]- a_c[1]) / (b[1] - a[1]) ) * uv_in[1] ) + cell_slide;
     }
     //cerr << "OUT UVpoint " << new_uv << "\n";
     return new_uv;
    
}

bool  
QuadCell::my_UV_to_controlCell_UV(UVpt& ret_uv,
                                  QuadCell*& neighbor_cell, 
                                  int i, 
                                  CUVpt_list& uv_stroke)
{
   //int k=i;
   //UVpt the_uv = uv_stroke[k];
   
   QuadCell* current_cell = this;
   QuadCell* n_cell;
   UVpt_list uv_list = uv_stroke;
   
   if(!current_cell)
      return false;
      //throw str_ptr("QuadCell::my_UV_to_controlCell_UV : could no current_cell");                              
          
   while(current_cell->uv_out_of_range(uv_list[i])){
      //get point inside the current cell        
      Bedge* edge = current_cell->find_in_edge(i, uv_list);
      if(!edge){
         return false;
         //throw str_ptr("QuadCell::my_UV_to_controlCell_UV : could not find edge");                              
      }  
      n_cell =  current_cell->find_neighbor_cell(edge);
      if(!n_cell){
         return false;
         //throw str_ptr("QuadCell::my_UV_to_controlCell_UV : could not find neighbor_cell");                              
      }         
      // make the uv list in turms of the new cell
      UVpt_list tmp = uv_list; uv_list.clear();
      for(int k = 0; k < tmp.num(); ++k){
         uv_list += current_cell->my_UV_to_controlCell_UV(tmp[k], n_cell, edge);
        // cerr << "new uv Pt " << current_cell->my_UV_to_controlCell_UV(tmp[k], n_cell, edge) << endl; 
      }
      current_cell = n_cell;    
      
   }
   
   neighbor_cell = n_cell;
   ret_uv = uv_list[i];
   return true;
}

//******** Synthesis ************//
double 
QuadCell::difference_to_cell(QuadCell * cell)
{
  
   double ssd=0;
   for(int m=0; m < 4; m++){
      std::set<double, std::less<double> > group_diffs;
      std::vector<Stroke_List*> list_1;
      std::vector<Stroke_List*> list_2;
	  
	  populate_list(list_1, m, this);
      populate_list(list_2, m, cell);
      if(list_1.empty() && list_2.empty())
         continue;
     	  
      //cerr << "List 1 is " << list_1.size() << endl;
      //cerr << "List 2 is " << list_2.size() << endl;   
      
      // The reason we swap is b/c it's easier if we know that blanks will be 
      // only in list_1...
      if(list_1.size() > list_2.size())
         list_1.swap(list_2);
      
      int n_d = (list_2.size() - list_1.size()); 
      // Populate smaller list with blanks
	  for(int i =0; i < n_d; ++i){        
         list_1.push_back(new NullStrokes);
      }   
      assert(list_1.size() == list_2.size());
      
      //sort the list by pointers so that next_permutation will work right...
      std::sort(list_2.begin(), list_2.end());
      
      int c=0;
      double sd;
	  do{        
        sd = 0;
        for(uint k=0; k < list_1.size(); ++k){
           sd += (list_1[k]->difference_to_group(list_2[k]));
             //cerr << c << " permutation is " << list_1[k]->difference_to_group(list_2[k]) << endl;   
        }
        group_diffs.insert(sd);
        c++;      
      }while(std::next_permutation(list_2.begin(), list_2.end()));
      
      assert(!group_diffs.empty());
      ssd += *(group_diffs.begin());
   }  
   
   err_adv(debug_cell,"Cell to cell diff: %f", ssd);
   return ssd; 
}

double      
QuadCell::difference_neighbors(QuadCell * cell)
{
   double ssd = 0;
   int weight = 0;
   
   int my_num=0, ref_num=0;
   ARRAY<Cell_List>  my_neighor_list(4);
   ARRAY<Cell_List>  ref_neighor_list(4);
   
    //*** Get neighbors ***//
   get_neighbors(my_neighor_list);
   cell->get_neighbors(ref_neighor_list);
   
   //*** Compare neighbors ***//
   for(int i=0; i < 4; ++i){
      my_num = my_neighor_list[i].num();      
      ref_num = ref_neighor_list[i].num();
      
      //err_msg("%d Neighbor: %d and %d",i, my_num, ref_num);
      if(my_num > 0 && my_num == ref_num){
         for(int j = 0; j < my_num; ++j){
               if((my_neighor_list[i][j])->groups_num() > 0 && (ref_neighor_list[i][j])->groups_num() > 0){
               ssd += my_neighor_list[i][j]->difference_to_cell(ref_neighor_list[i][j]);              
               weight++;
            }
         }        
      } 
   }
   //err_adv(debug_cell,"2 cells are differnt by %f, weight is %f", ssd,weight);
   // Return normalised ssd
   //return (weight > 0) ? (ssd / weight) : 0;
   return (weight > 0) ? (ssd / weight) : PatternGrid::MAX_DIFF_THRESHOLD + 1;         
}


//******** Stroke Related ********//
double     
QuadCell::current_width(double start_width, double pix_size)
{
   double x = at_length(center(), 1) / pix_size;
   double c = 0.25;
   
   double desired_frac = max(((sqrt(4*c*x + (1-2*c)*(1-2*c)-4*c*c))/(2*c)) - ((1-2*c)/(2*c)), 0.0);
   cerr << "x " << x << " frac " << desired_frac << endl;
     
   double new_width = start_width * x;//desired_frac;
   
   return new_width;
   /*
   double new_width;
   double ratio_adjust = at_length(center(), 1) / pix_size;
   double desired_frac;
   
   double lo_width      = .05;
   double hi_width      = .01;
  
   if(ratio_adjust < 1){  //we are  farther away from the object
      desired_frac = lo_width + (1 - lo_width) *  ratio_adjust;
   } else {
      desired_frac = hi_width + (1 - hi_width) *  ratio_adjust;
   }
   //cerr << "ratio is " << ratio_adjust << "desired " << desired_frac <<  endl;
   new_width = start_width * desired_frac;
   
   if(pg()->get_light_on() && pg()->get_light_width()){      
      VIEWptr view = VIEW::peek();
      Wvec light_c;
      if (view->light_get_in_cam_space(light_num))
         light_c = view->cam()->data()->at_v();//view->cam()->xform().inverse();  // * view->light_get_coordinates_v(light_num);
      else
         light_c = view->light_get_coordinates_v(light_num);

      double angle = light.angle((_inv_xf * light_c)); 
      angle /= M_PI_2;  //normalize the angle to 0..1
      double end = pg()->get_light_radius();
      double start = end * (2/3);
     
      if(angle > start){
         //double val = 1- (angle - start);
         double t = (angle - start);
         double val = t*(1/(start-end)) + 1;       
         new_width *= clamp(val, 0.0, 1.0);  
      }              
   }
   
   return new_width;*/
   
}
double                    
QuadCell::current_alpha_frac(double pix_size, Wvec light, int light_num)
{ 
   
   //double a = 1.0;
   /*
   if(pg()->get_lod_on() && pg()->get_lod_alpha()){   
      static double view_pix_size = at_length(center(), 1);
      static uint update_length = VIEW::stamp();
      if(update_length != VIEW::stamp()){
         view_pix_size = at_length(center(), 1);
         update_length = VIEW::stamp();
      }
      
      double ratio_adjust = view_pix_size / pix_size;
      //cerr << "ratio is " << ratio_adjust << endl;
      double lo_alpha = pg()->get_lod_low(); 
      double hi_alpha = pg()->get_lod_high(); 
     
      double lo_slope = 1.8;
      double hi_slope = 2.5;
      
      
      if(ratio_adjust <= lo_alpha){
         double new_ratio_adjust = lo_alpha - ratio_adjust;
         a = (max(1.0 - new_ratio_adjust * lo_slope, 0.0));
      }else if(ratio_adjust > hi_alpha) {
         double new_ratio_adjust = ratio_adjust - hi_alpha;
         a = (max(1.0 - new_ratio_adjust / hi_slope, 0.0));
      }      
   }*/
   //Lighitng
   /*
   if(pg()->get_light_on() && pg()->get_light_alpha()){      
      VIEWptr view = VIEW::peek();
      
      Wvec light_c;
      //if (view->light_get_in_cam_space(light_num))
      //   light_c = view->cam()->xform().inverse() * view->light_get_coordinates_v(light_num);
      if (view->light_get_in_cam_space(light_num))
         light_c = view->cam()->data()->at_v();
      else
         light_c = view->light_get_coordinates_v(light_num);
      
      double angle = light.angle((_inv_xf * light_c)); 
      angle /= M_PI_2;  //normalize the angle to 0..1
      
      double end = pg()->get_light_radius();
      double start = end * (2/3);
     
      if(angle > start){
         //double val = 1- (angle - start);
         double t = (angle - start);
         double val = t*(1/(start-end)) + 1;       
         a *= clamp(val, 0.0, 1.0);  
      }         
   }
   
   return a;
   */
   return 1.0;
}

Bedge*
QuadCell::find_in_edge(int i, Pattern3dStroke* stroke)
{
   int j, k=i;
   //err_msg("i is %d", k);
   UVpt in_uv = stroke->get_uv_p(k);
   bool forward = false; 
   
   while(uv_out_of_range(in_uv)){
     //if(k >= (stroke->get_verts_num()-1))
     if(k <= 0)    
       forward = true;
     
     if(forward)
         k++;
     else
         k--;
      
     in_uv = stroke->get_uv_p(k);     
   }
   UVpt out_uv;
   //if(forward)
     out_uv = stroke->get_uv_p(k-1);
   //else
   //  out_uv = stroke->get_uv_p(k+1); 
   out_uv = (uv_out_of_range(out_uv)) ? out_uv : stroke->get_uv_p(k+1);
 
   if(!(uv_out_of_range(out_uv))){
      cerr << "old uv is " << out_uv << endl;
      throw str_ptr("QuadCell::find_in_edge: is not out of fange...what ta hell...");
   }
   UVline stroke_seg(in_uv, out_uv);
   
   ARRAY<Bedge_list> list(4);
   get_cell_boundery(list);  

   for(i=0; i < list.num(); ++i){
       for(j=0; j < list[i].num(); ++j){
         UVline edge_seg(bvert_to_uv(list[i][j]->v1()), bvert_to_uv(list[i][j]->v2())); 
         if(edge_seg.intersect_segs(stroke_seg)){
            //cerr << "the two lines are " <<   edge_seg << " and " << stroke_seg << endl;
            /*if(forward){
              WORLD::show(list[i][j]->v1()->loc(), 10,COLOR(1,1,0)); 
              WORLD::show(list[i][j]->mid_pt(), 10,COLOR(1,1,0)); 
              WORLD::show(list[i][j]->v2()->loc(), 10,COLOR(1,1,0));     
            } else {
                WORLD::show(list[i][j]->v1()->loc(), 10,COLOR(1,0,0)); 
              WORLD::show(list[i][j]->mid_pt(), 10,COLOR(1,0,0)); 
              WORLD::show(list[i][j]->v2()->loc(), 10,COLOR(1,0,0));     
            }
            */            
               return list[i][j];
         }
       }
   }
   return 0;
   
   
}

Bedge*
QuadCell::find_in_edge(int i, CUVpt_list& uv_stroke)
{
 int j, k=i;
   //err_msg("i is %d", k);
   UVpt in_uv = uv_stroke[k];
   bool forward = false; 
   
   while(uv_out_of_range(in_uv)){
     //if(k >= (uv_stroke.num()-1))
     if(k <= 0)   
       forward = true;
     
     if(forward)
         k++;
     else
         k--;
      
     in_uv = uv_stroke[k];     
   }
   UVpt out_uv;
   //if(forward)
     out_uv = uv_stroke[k-1];
   //else
   //  out_uv = stroke->get_uv_p(k+1); 
   out_uv = (uv_out_of_range(out_uv)) ? out_uv : uv_stroke[k+1];
 
   if(!(uv_out_of_range(out_uv))){
      cerr << "old uv is " << out_uv << endl;
      throw str_ptr("QuadCell::find_in_edge: is not out of fange...what ta hell...");
   }
   UVline stroke_seg(in_uv, out_uv);
   
   ARRAY<Bedge_list> list(4);
   get_cell_boundery(list);  

   for(i=0; i < list.num(); ++i){
       for(j=0; j < list[i].num(); ++j){
         UVline edge_seg(bvert_to_uv(list[i][j]->v1()), bvert_to_uv(list[i][j]->v2())); 
         if(edge_seg.intersect_segs(stroke_seg)){
              return list[i][j];
         }
       }
   }
   return 0;  
   
}


void                      
QuadCell::remove_stroke(Pattern3dStroke * s)
{
   for(int i=0; i < _3d_strokes.num(); ++i){
      if(_3d_strokes[i] == s)
         _3d_strokes.remove(i);
   }
}     

int         
QuadCell::add_group(Stroke_List* old_group)
{
   return add_group_var(old_group);
   /*
    
   //QuadCell* tmp_cell; 
   //QuadCell* cerrent_cell;
   
   CBface* f;
   Stroke_List* new_group;  
   //QuadCell* neighbor_cell;  
   
   // Pick wich group we are getting
     
   if(old_group->class_name() == "Structured_Hatching")
          new_group = new Structured_Hatching;   
    else if(old_group->class_name() == "Structured_Curves")
          new_group = new Structured_Curves;
   else if(old_group->class_name() == "Stippling")
          new_group = new Stippling;
   else if(old_group->class_name() == "OtherStrokes")
          new_group = new OtherStrokes;   
   else{
       err_msg("Failed Type ");  
       return 0;
   }  
   //cerr << "___________________________ "  << endl;       
   
     
  for(int i=0; i < old_group->num(); ++i){         
      UVpt            uv_tmp;
      Wpt_list        new_pts;
      ARRAY<Wvec>     new_norms;   
      ARRAY<CBface*>  new_f;
      ARRAY<Wvec>     new_bar;
      ARRAY<double>   new_alpha;
      ARRAY<double>   new_width;
      UVpt_list       adjusted_uv_pts;     
      //Wpt border_pix;
      try {
           int n = (*old_group)[i]->get_verts_num();             
           for(int k=0; k < n; ++k) {                         
              UVpt old_uv = (*old_group)[i]->get_uv_p(k);
              // If UV are > 1 or < 0 then the point is outside the cell
              //cerr << "OLD UV was " << old_uv << endl;  
              if(uv_out_of_range(old_uv)){ 
                  //cerr << "OLD UV was " << old_uv << endl;  
                  //Find a edge closest to the prev point
*/                  
                 /* 
                 Bedge* e_temp = find_in_edge(k, (*old_group)[i]);  
                  if(!e_temp){                        
                     if(new_pts.empty()) 
                        continue;
                     else
                        break;
                  }                     
                      //throw str_ptr("QuadCell::add_group : could not find edge");
                  
                  neighbor_cell = find_neighbor_cell(e_temp);
                    
                  if(!neighbor_cell){                        
                     if(new_pts.empty()) 
                        continue;
                     else
                        break;
                  }*/   
/*                  
                  //throw str_ptr("QuadCell::add_group : could not find neighbor_cell");                              
                  QuadCell* neighbor_cell;  
                  uv_tmp = my_UV_to_controlCell_UV(neighbor_cell, k, adjusted_uv_pts);  
                  
                  //uv_tmp = my_UV_to_controlCell_UV(old_uv, neighbor_cell, e_temp);  
                  //cerr << "uv in turms of neig " << uv_tmp << endl;
                  new_pts += neighbor_cell->cellUV_to_loc(uv_tmp);                 
                  f = neighbor_cell->cellUV_to_face(uv_tmp);
                  if(!f){                        
                     if(new_pts.empty()) 
                        continue;
                     else
                        break;
                  }
                     //throw str_ptr("QuadCell::add_group : could not find a face");
                  
               } else {                  
                  new_pts += cellUV_to_loc(old_uv);                  
                  f = cellUV_to_face(old_uv);
                  if(!f)
                     throw str_ptr("QuadCell::add_group : could not find a face2");                  
               }                  
               new_f += f;    
               new_norms += f->norm();  
               new_bar   += (*old_group)[i]->get_bar()[k];
               new_alpha += (*old_group)[i]->get_alpha()[k];
               new_width += (*old_group)[i]->get_width()[k];
               adjusted_uv_pts +=  (*old_group)[i]->get_uv_p(k);             
           }             
           Pattern3dStroke* new_stroke = new Pattern3dStroke((QuadCell*)this,                                                
                                         (*old_group)[i]->get_pix_size(),
                                         (*old_group)[i]->get_light(),
                                         (*old_group)[i]->get_light_num(),                                         
                                         new_f,
                                         new_pts,
                                         new_norms,
                                         new_bar,
                                         new_alpha,
                                         new_width,
                                         adjusted_uv_pts,           
                                         (*old_group)[i]->get_offsets(),
                                         (*old_group)[i]->get_stroke(),
                                         (*old_group)[i]->winding(),
                                         (*old_group)[i]->straightness());
           
            new_group->add(new_stroke);              
         } //try
         catch(str_ptr error){
               cerr << error << endl; 
         }
			//make sure avarage info is up to date
            
     } //Per stroke loop      
     if(!new_group->empty()){
		   new_group->update();
		   _groups += new_group;
     }
     err_msg("QuadCell::add_group, %d ",groups_num());   
     return (_groups.num()-1);
     */
}


int         
QuadCell::add_group_var(Stroke_List* old_group)
{
   int ff;       
   //QuadCell* tmp_cell; 
   //QuadCell* cerrent_cell;
   
   CBface* cbf;
   
   Stroke_List* new_group;  
   
   
   // Pick wich group we are getting
     
   if(old_group->class_name() == "Structured_Hatching")
          new_group = new Structured_Hatching;  
   else if(old_group->class_name() == "Stippling")
          new_group = new Stippling;
   else if(old_group->class_name() == "OtherStrokes")
          new_group = new OtherStrokes;   
   else{
       err_msg("Failed Type ");  
       return 0;
   }  
   //cerr << "___________________________ "  << endl;       
   ARRAY<UVpt>             avg_pts;
   // Get the  Backbone points
   if(old_group->class_name() == "Stippling"){
      for(int k=0; k < old_group->num(); ++k){
         avg_pts += (*old_group)[k]->get_avg_uv_pt();
      }  
   } else if(old_group->class_name() == "Structured_Hatching"){
      Stroke_List* tmp_group = _pg->get_similar_group(old_group->get_group_id()); 
      for(int k=0; k < old_group->num(); ++k){
         if (k < tmp_group->num()){           
            avg_pts += (*tmp_group)[k]->get_avg_uv_pt();
         } else {
            avg_pts += (*old_group)[k]->get_avg_uv_pt();
         }
      }  
   }
   ARRAY<Pattern3dStroke*> strokes;
   strokes.clear();
   ARRAY<double> scale;
   if(PatternPen::VARIATION){
      // Get The strokes
      if(old_group->class_name() == "Stippling"){
         for(ff =0; ff < old_group->num(); ++ff){
            //cerr << "QuadCell::add_group_var: group id " << old_group->get_group_id() << endl;
            Stroke_List* tmp_group = _pg->get_similar_group(old_group->get_group_id()); 
            int n = tmp_group->num()-1;      
            strokes += (*tmp_group)[(int)round(drand48()*n)];            
         }
         assert(strokes.num() == avg_pts.num());      
      } else if (old_group->class_name() == "Structured_Hatching"){ 
         // if structured hatching then pick another group and scale strokes to that
         Stroke_List* tmp_group = _pg->get_similar_group(old_group->get_group_id());
         for(ff =0; ff < old_group->num(); ++ff){
            if (ff < tmp_group->num()){
               double int_1 = (*tmp_group)[ff]->get_avg_uv_pt().dist((*tmp_group)[ff]->get_uv_p(0));
               double int_2 = (*old_group)[ff]->get_avg_uv_pt().dist((*old_group)[ff]->get_uv_p(0));
               scale +=  int_1 / int_2;
            } else {   
               scale += 1;
            }
         }
         for(ff =0; ff < old_group->num(); ++ff){   
            strokes += (*old_group)[ff];            
         }
      } else {   
         for(ff =0; ff < old_group->num(); ++ff){   
            strokes += (*old_group)[ff];            
         }
      }
   } else {
      // NO VARIATION
      for(ff =0; ff < old_group->num(); ++ff){   
            strokes += (*old_group)[ff];            
      }
   }
  for(int i=0; i < strokes.num(); ++i){         
      
     UVpt_list       adjusted_uv_pts;        
     
     if(PatternPen::VARIATION){
        if (old_group->class_name() == "Stippling") {
         adjusted_uv_pts = Pattern3dStroke::offsetUV_2_uv(avg_pts[i],strokes[i]->get_offset_uv());
        } else if(old_group->class_name() == "Structured_Hatching"){
           for(int h=0; h < strokes[i]->get_verts_num(); ++h){
              adjusted_uv_pts += Pattern3dStroke::offsetUV_2_uv(avg_pts[i],strokes[i]->get_offset_uv()[h]*scale[i]);
           }
        } else {   
         adjusted_uv_pts = strokes[i]->get_uv_p();
        }  
     }else {
        adjusted_uv_pts = strokes[i]->get_uv_p();
     }
      UVpt            uv_tmp;
      Wpt_list        new_pts;
      ARRAY<Wvec>     new_norms;   
      ARRAY<CBface*>  new_f;
      ARRAY<Wvec>     new_bar;
      ARRAY<double>   new_alpha;
      ARRAY<double>   new_width;
      //BaseStrokeOffsetLISTptr new_offsets = new BaseStrokeOffsetLIST;       
      //Wpt border_pix;
      try {
           int n = strokes[i]->get_verts_num();             
           for(int k=0; k < n; ++k) {                         
              UVpt old_uv = adjusted_uv_pts[k]; //(*old_group)[i]->get_uv_p(k);
              // If UV are > 1 or < 0 then the point is outside the cell
              //cerr << "OLD UV was " << old_uv << endl;  
              if(uv_out_of_range(old_uv)){ 
                  //Find a edge closest to the prev point                           
                  /*
                  Bedge* e_temp = find_in_edge(k, adjusted_uv_pts);  
                  if(!e_temp){ 
                     //if remainder of stroke is inside keep at it otherwise stop with the stroke
                     if(new_pts.empty()) 
                        continue;
                     else
                        break;
                  }
                  
                      //throw str_ptr("QuadCell::add_group : could not find edge");
                  
                  neighbor_cell = find_neighbor_cell(e_temp);
                  
                  if(!neighbor_cell){                        
                     if(new_pts.empty()) 
                        continue;
                     else
                        break;
                        //throw str_ptr("QuadCell::add_group : could not find neighbor_cell");                              
                  }
                  */                  
                  //uv_tmp = my_UV_to_controlCell_UV(old_uv, neighbor_cell, e_temp);  
                  //cerr << "uv in turms of neig " << uv_tmp << endl;
                  QuadCell* neighbor_cell;  
                  if(!my_UV_to_controlCell_UV(uv_tmp, neighbor_cell, k, adjusted_uv_pts)){
                     if(new_pts.empty()) 
                        continue;
                     else
                        break;                     
                  }                     
                  //cerr << "uv in turms of neig " << uv_tmp << endl;
                  new_pts += neighbor_cell->cellUV_to_loc(uv_tmp);                 
                  cbf = neighbor_cell->cellUV_to_face(uv_tmp);
                  if(!cbf){
                     if(new_pts.empty()) 
                        continue;
                     else
                        break;
                     //throw str_ptr("QuadCell::add_group : could not find a face");
                  }
               } else {                  
                  new_pts += cellUV_to_loc(old_uv);                  
                  cbf = cellUV_to_face(old_uv);
                  if(!cbf)
                     throw str_ptr("QuadCell::add_group : could not find a face2");                  
               }                  
               
               new_f += cbf;    
               Wvec bc_t; 
               cbf->project_barycentric(new_pts.last(),bc_t);  
               //XXX should update barycentric coords and norms
               new_bar   += bc_t;//strokes[i]->get_bar()[k];
               
               Wvec tmp_norm;
               cbf->bc2norm_blend(bc_t,tmp_norm);   
               
               new_norms += tmp_norm;  
               //strokes[i]->get_norms()[k];
               
               new_alpha += strokes[i]->get_alpha()[k];
               new_width += strokes[i]->get_width()[k];
               //new_offsets.add();               
           }
                 
           Pattern3dStroke* new_stroke = new Pattern3dStroke((QuadCell*)this,                                                
                                         strokes[i]->get_pix_size(),
                                         strokes[i]->get_start_width(), 
                                         //strokes[i]->get_light(),
                                         //strokes[i]->get_light_num(),                                         
                                         new_f,
                                         new_pts,
                                         new_norms,
                                         new_bar,
                                         new_alpha,
                                         new_width,
                                         adjusted_uv_pts,           
                                         strokes[i]->get_offsets(),
                                         strokes[i]->get_stroke(),
                                         strokes[i]->winding(),
                                         strokes[i]->straightness());
           
            new_group->add(new_stroke);              
         } //try
         catch(str_ptr error){
               cerr << error << endl; 
         }
			//make sure avarage info is up to date
            
     } //Per stroke loop      
     if(!new_group->empty()){
		   new_group->update();
		   _groups += new_group;
     }
     //err_msg("QuadCell::add_group_var, %d ",groups_num());   
     return (_groups.num()-1);
}

void                      
QuadCell::remove_group(int i)
{
   _groups.remove(i);
}     
void                       
QuadCell::make_groups()
{
   
    if(_3d_strokes.empty())
       return;   
    //ARRAY<Pattern3dStroke*> tmp_list(_3d_strokes);
    bool group_found;
    Stroke_List* tmp;            
    // For evry type of strokes  
    for(int m=0; m < 3; ++m){     
       
       ARRAY<Pattern3dStroke*> wrong_group;
       group_found = true;
       do{
          // Pick wich group we are getting
          if(m == 0)
             tmp = new Structured_Hatching;           
          else if(m == 1)
             tmp = new Stippling;
          else if(m == 2)
             tmp = new OtherStrokes;
             
          // Go throught all the strokes and pull out the onece that qualify as your type
          // and delete them from 3d_strokes (will be put back if does not really qualify)
          tmp->add_strokes(_3d_strokes);
          
          // If not found anything, dich this type of strokes
          if(tmp->empty()){
             group_found = false;
             delete tmp;
          } else {             
             // get rid all the stroke that were added by mistake
             // This is done as a separate step to allow for arbitray
             // order of stroke drawing
             //XXX - does not do much    
             tmp->refine_list(wrong_group);         
             //Copy back the strokes that were picked incorectly
             for(int i=0; i < wrong_group.num(); ++i){
                _3d_strokes.add(wrong_group[i]);
             }       
           
             if (!tmp->empty()) {
                _groups += tmp;
                //cerr << tmp->class_name() << " group added " << endl;    
             } else {
                group_found = false;
             }        
         
           }
           
           
           
        } while (group_found); 

            
 } //end of one type of strokes
 
 
 err_msg("Unsorted Strokes: %d Groups: %d", _3d_strokes.num(), _groups.num());
 err_msg("----------------------------------");
 for(int i=0; i < _groups.num(); ++i){
     cerr << "Group " <<  _groups[i]->class_name()<< " "<< _groups[i]->num() << " strokes" << endl;
 }
 err_msg("----------------------------------");
}

void 
QuadCell::populate_list(std::vector<Stroke_List*>& list, int type, QuadCell *cell)
{
   str_ptr group_name;
   if(type == 0)
       group_name = "Structured_Hatching";
    else if(type == 1)  
      group_name = "Structured_Curves";
    else if(type == 2)  
      group_name = "Stippling";
    else if(type == 3)  
      group_name = "OtherStrokes";  
      
   for(int i=0; i < cell->_groups.num(); ++i){
      if(cell->_groups[i]->class_name() == group_name)
       list.push_back(cell->_groups[i]);  
   }       

}

//******** ACCESSORS ********//

bool         
QuadCell::is_visible()
{
   for(int j=0; j < _RowsCache; ++j){
      for(int i=0; i < _ColsCache; ++i){        
         if(quad(i, j) && quad(i, j)->front_facing())
            return true;         
      }
   }
   return false;
}

Bvert_list 
QuadCell::col(int i)
{   
   Bvert_list list;

   for (int j=0; j <= _RowsCache; j++) {
      list += row(j)[i];  
   } 
   return list;
}

Bface* 
QuadCell::quad(int i, int j) const    
{ 

      //just in case we got vertex that is on the very edge
      if(i == _ColsCache)
         i--;
      if(j == _RowsCache)
         j--;
           
      if(i < _ColsCache && j < _RowsCache){     
           return  lookup_quad(vert(i, j), vert(i+1,j), vert(i+1, j+1), vert(i, j+1));
      } else { 
           return 0;
      }
} 
bool         
QuadCell::vert_uv(Bvert* v, UVpt& uv_loc)
{
   for(int j=0; j < nrows(); ++j){
      for(int i=0; i < ncols(); ++i){        
         
         if(v == vert(i, j)){
            //cerr << "Found vert at " << i << " " << j << endl;
            uv_loc = UVpt(i, j);
            return true;
         }            
      }
   }
   return false;
}

CBedge*      
QuadCell::get_edge(UVpt p1,UVpt p2)
{
   Bvert* v1 = vert(int(p1[0]), int(p1[1]));
   Bvert* v2 = vert(int(p2[0]), int(p2[1]));
   return lookup_edge(v1, v2);
}

//******** Helper Functions ********  


/*
Bedge*
QuadCell::find_edge(Wpt& p)
{
   double dist = 0;
   ARRAY<Bedge_list> list(4);
   list += top().get_chain();
   list += bottom().get_chain();
   list += col(0).get_chain();
   list += col(_ColsCache).get_chain();   
   assert(list.num() == 4);
   map<double, Bedge*, less<double> > edge_dist;
   
   for(int i=0; i < 4; ++i){        
       for(int j=0; j < list[i].num(); ++j){
           
         dist = (list[i][j])->line().project_to_seg(p).dist(p);
         edge_dist[dist] = list[i][j];    
       }
   }   
   return edge_dist.begin()->second;
   
}
*/


void
QuadCell::fix_up_cell_data()
{
   for(int j=0; j < _RowsCache; ++j){
      for(int i=0; i < _ColsCache; ++i){        
         Bface* f = quad(i, j);
         if(!f)
            return;            
         CellData *cd = CellData::lookup(f, _pg);
         
         if(cd){
             delete cd;
         }
         cd = new CellData(f, _pg,this, i, j);            
         
      }
   }


}

int
QuadCell::edge_location(CBedge* e)
{

     if (list_contains_edge(top(), e))
          return 2;
     else if (list_contains_edge(bottom(), e))
          return 1;
     else if (list_contains_edge(col(0), e))
          return 4;
     else if(list_contains_edge(col(ncols()-1), e))
          return 5;
     else 
          return 0;
}

CBedge*
QuadCell::joint_edge(QuadCell* start_cell)
{
     ARRAY<Bedge_list> list(4);
     get_cell_boundery(list);
     assert(list.num() == 4);  
     for(int i=0; i < list.num(); ++i){
       for(int j=0; j < list[i].num(); ++j){
           if(start_cell->edge_location(list[i][j]) != 0)
              return list[i][j];
       }
     }
     return 0;
    
}

Bvert_list& 
QuadCell::opposite_edges(CBvert_list& list , Bvert_list& new_list)
{
   CBedge* e; 
   CBface* f;
   CBedge* new_e;
   Bvert * new_v1;
   Bvert * new_v2;

   
   for(int i=0; i < list.num()-1; ++i){
      e = lookup_edge(list[i], list[i+1]);
      f = outside_face(e);
      if(!f)
         throw str_ptr("Cell in a way, cannot expand");
      
      new_e = f->opposite_quad_edge(e);
      new_v1 = new_e->v1();
      new_v2 = new_e->v2();
       
      new_list += (makes_strong_chain( list[i], new_v1)) ? new_v1 : new_v2;   
   } 
   new_list += (makes_strong_chain(list.last(), new_v1)) ? new_v1 : new_v2;
   
   return new_list;
}

bool 
QuadCell::makes_strong_chain(Bvert* a, Bvert* b)
{
   Bedge * tmp = lookup_edge(a,b);
   return (tmp && tmp->is_strong()) ? true : false;
}

//******** BASIC OPS ********  
bool 
QuadCell::is_good() const {
      if (nrows() < 2 || ncols() < 2){
         err_msg("nrows() < 2 || ncols() < 2");
       return false;
     }
      // check that each row is the same size
      for (int i=1; i<nrows(); i++)
         if (row(i).num() != row(i-1).num()){
          err_msg("row(%d).num() (%d) != row(%d-1).num() (%d)", i, row(i).num() ,i,row(i-1).num());
            return false;
       }
      return true;
}
void
QuadCell::cache()
{ 
   int i,j,k;

   assert(is_good());
   
   _RowsCache = nrows() - 1;
   _ColsCache = ncols() - 1;   

   assert(_RowsCache > 0 && _ColsCache > 0);

   _du = 1.0/_ColsCache;
   _dv = 1.0/_RowsCache;
   
   
   _faces.clear();
   for(j=0; j < _RowsCache; ++j){
      for(i=0; i < _ColsCache; ++i){        
         
         Bface* f = quad(i, j);
         if(f){
           _faces += f;
           _faces += f->quad_partner();
         }            
      }
   }
   _is_uv_continuous = true;
   _is_edge_uv_continuous =  true;
   Bedge_list b_b = _faces.boundary_edges();
   Bedge_list b_i = _faces.interior_edges();
   for(k=0; k < b_b.num(); ++k){
      if(!UVdata::is_continuous(b_b[k])){
         //err_msg("Cell is uv discontinuous");
         _is_edge_uv_continuous = false;
         break;
      }
   }
   for(k=0; k < b_i.num(); ++k){
      if(!UVdata::is_continuous(b_i[k])){
         //err_msg("Cell is uv discontinuous");
         _is_uv_continuous = false;         
         break;
      }
   }
     
   Wpt_list plane;   
   plane += loc_l_b();
   plane += loc_r_b();
   plane += loc_l_t();
   plane += loc_r_t();
   
   Wline tan1(plane[0],plane[1]);
   Wline tan2(plane[0],plane[2]);
   
   Wplane pl;
   plane.get_best_fit_plane(pl);
         
   Wvec t = tan1.vector().normalized();
   Wvec b = tan2.vector().normalized();
   Wpt  o = pl.origin();
   Wvec n = cross(t,b).normalized(); 
   
   _xf     =  Wtransf(o, t, b, n);
   _inv_xf = _xf.inverse(); 

   _center = o;
}

double    
QuadCell::area() const
{  
   double area=0;
   for(int j=0; j < _RowsCache; ++j){
      for(int i=0; i < _ColsCache; ++i){        
         Bface* f = quad(i, j);
         assert(f);   
         area += f->quad_area();    
      }
   }
   return area;  
}

//******* Private method
void
QuadCell::rotate_cell()
{
    ARRAY<Bvert_list>  tmp_grid(_grid);
    Bvert_list         tmp_list;
    clear();
    //err_msg("Cell is %d by %d", tmp_grid[0].num(), tmp_grid.num());
    for(int i=0; i < tmp_grid[0].num(); ++i){
      for(int j = tmp_grid.num()-1; j >=0 ; --j){
         //err_msg("Looking at %d , %d", j, i);
         assert(tmp_grid[j][i]);
         tmp_list += tmp_grid[j][i];
          }
      _grid += tmp_list;
      tmp_list.clear();
        }
    cache();
    fix_up_cell_data(); 
}
        
void
QuadCell::flip_hor()
{
   ARRAY<Bvert_list>  tmp_grid(_grid);
   Bvert_list         tmp_list;
   clear();
   for(int j=tmp_grid.num()-1; j >= 0; --j){
      for(int i=0; i < tmp_grid[j].num() ; ++i){
         assert(tmp_grid[j][i]);
         tmp_list += tmp_grid[j][i];
     }
     _grid += tmp_list;
     tmp_list.clear();
   }
   cache();
   fix_up_cell_data();
}

void
QuadCell::flip_vert()
{
   ARRAY<Bvert_list>  tmp_grid(_grid);
   Bvert_list         tmp_list;
   clear();
   for(int j=0; j < tmp_grid.num(); ++j){
     for(int i=tmp_grid[j].num()-1; i >= 0 ; --i){
        assert(tmp_grid[j][i]);
        tmp_list += tmp_grid[j][i];
     }
     _grid += tmp_list;
     tmp_list.clear();
   }
   cache();
   fix_up_cell_data();
}


// orient the cells to the uv coords
void
QuadCell::orient_to_uv()
{
   CBface* f= quad(0,0);
   if(!f || !f->is_quad() || !UVdata::has_uv(f))
         return;
    
   Bvert* bv1 = vert(0, 0);
   Bvert* bv2 = vert(1, 0);   
   Bvert* bv3 = vert(1, 1);
   Bvert* bv4 = vert(0, 1);
   
   UVpt a, b, c, d;
   UVdata::get_quad_uvs(bv1,bv2,bv3,bv4,a,b,c,d);
   //cerr << "a is " << a << endl;
   //cerr << "b is " << b << endl;
   //cerr << "c is " << c << endl;
   //cerr << "d is " << d << endl;

   
   UVline tan1(a,b);
   UVline tan2(a,d);   
   
   UVvec one = tan1.vector().normalized();
   UVvec two = tan2.vector().normalized();

   //cerr << "UV VECS " << one << " " << two << endl;  
   if(one[0] == 0){
       rotate_cell();
       bv1 = vert(0, 0);
       bv2 = vert(1, 0);   
       bv3 = vert(1, 1);
       bv4 = vert(0, 1);
       UVdata::get_quad_uvs(bv1,bv2,bv3,bv4,a,b,c,d);
       //cerr << "a is " << a << endl;
       //cerr << "b is " << b << endl;
       //cerr << "c is " << c << endl;
       //cerr << "d is " << d << endl;
       UVline tan12(a,b);
       UVline tan22(a,d);   
       one = tan12.vector().normalized();
       two = tan22.vector().normalized();
   }
   
   if(one[0] < 0)
      flip_vert();  
   if(one[1] < 0)
      flip_hor();
  
   if(two[1] < 0)
      flip_hor();  
   if(two[0] < 0)
      flip_vert();
   
     
   
}
/* end of file quad_cell.C */
