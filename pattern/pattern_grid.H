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
    pattern_grid.H
    
    PatternGrid 
        -Recieves Strokes from Pattern Pen and distrubites them to
         the appropriate cell
        -At any given time can give cells that are within it by looking
         on all the faces and
        -Is held by the texture    
    -------------------
    Simon Breslav
    Fall 2004
 ***************************************************************************/
#ifndef _PATTERN_GRID_H_IS_INCLUDED_
#define _PATTERN_GRID_H_IS_INCLUDED_

#include "manip/manip.H"
#include "quad_cell.H"
#include "pattern_stroke.H"
#include "geom/command.H"
#include "mesh/uv_data.H"

class GridTexture;
class LightParams;   
   
#define CPatternGrid const PatternGrid
class PatternGrid :  public DATA_ITEM {
 private:
   //******** MEMBERS VARIABLES ********
   Patch *           _patch;
   ARRAY<QuadCell*>  _cells;       // Every time we asked to get cells
                                   // we look on a the patch and get the cells
   ARRAY<QuadCell*>  _ref_cells;   // Works similarly to _cells only stores
                                   // cells that indicate cells with reference
                                   // pattern also puts all the strokes in the 
                                   // cell into the right group
   ARRAY<Cell_List>  _sorted_cells;// Each row has a cell with identical row/col
    
   ARRAY<int>        _ref_groups;  // group numbers which are reference
   
   ARRAY<Group_List> _group_list;  // Each row is a list of stroke lists
   
   GridTexture*      _grid_lines;  // Grid border lines, cnage color as meaning
                                   // of the cell changes: no cell, cell, ref cell
 
   // ***** UI Related ****** // 
   bool   _show_grid_lines;        // Should pattern_texture draw the grid lines
   int    _visible;
   
   bool              _alpha_press; 
   bool              _width_press;
  
   //UI Params Lights
   bool              _light_on;
   int               _light_num;
   int               _light_type;
   bool              _light_cam_frame;
   bool              _light_width;
   bool              _light_alpha;
   double            _light_a;
   double            _light_b;     
   
   //current values
   bool                 _lod_on;
   bool                 _lod_alpha;
   bool                 _lod_width;
   double               _lod_high;
   double               _lod_low; 
 public:   
   
   //***** UI Related*****//
   void   set_visible(int i) { _visible = i; }       
   int    get_visible() const{ return _visible; } 
  
   void   taggle_grid_line() { _show_grid_lines = !_show_grid_lines; }    
   bool   show_grid_lines() const {return _show_grid_lines;}
   
   void   set_alpha_press(int a) { _alpha_press = (a==1); }
   bool   get_alpha_press()  const{ return _alpha_press; }
   
   void   set_width_press(int a) { _width_press = (a==1); }
   bool   get_width_press()  const{ return _width_press; }
        
   //Lighing
   void   set_light_on(int i) { _light_on = (i==1); }
   int    get_light_on() const{ return _light_on; }
   
   void   set_light_num(int i) { _light_num = i; }
   int    get_light_num() const{ return _light_num; }
   
   void   set_light_type(int i) { _light_type = i; }
   int    get_light_type() const{ return _light_type; }
   
   void   set_light_alpha(int a) { _light_alpha = (a==1); }
   bool   get_light_alpha() const{ return _light_alpha; }
   
   void   set_light_width(int a) { _light_width = (a==1); }
   bool   get_light_width() const{ return _light_width; }

   void   set_light_a(float a) { _light_a = (double)a; }
   float  get_light_a() const{ return (float)_light_a; }
   
   void   set_light_b(float a) { _light_b = (double)a; }
   float  get_light_b() const{ return (float)_light_b; }
 
   // LOD UI stuff...
   bool   get_lod_on()         const { return _lod_on; }
   void   set_lod_on(bool i)         { _lod_on = i; }
   
   bool   get_lod_alpha()      const { return _lod_alpha;}
   void   set_lod_alpha(bool i)      { _lod_alpha = i; }
   
   bool   get_lod_width()      const { return _lod_width; }
   void   set_lod_width(bool i)      { _lod_width = i; } 
   
   double get_lod_high()       const { return _lod_high; }
   void   set_lod_high(double i)     { _lod_high = i;    }
   
   double get_lod_low()        const { return _lod_low;  }
   void   set_lod_low(double i)      { _lod_low = i;     }
   
   GridTexture* get_grid_lines() const{ return _grid_lines; }  
   
   //******** CONSTRUCTOR/DECONSTRUCTOR *******
   PatternGrid(Patch *p=0);
   ~PatternGrid();
   
   static float MAX_DIFF_THRESHOLD;
   //********* CELL STUFF *********
   bool                 is_ref_group(int);
   void                 get_ref_groups();
   
   void                 clear_cell_markings();     //ignores cells with strokes
   void                 set_all_cell_markings();
   void                 set_unmarked(ARRAY<QuadCell*> list);

   
   CARRAY<QuadCell*>    get_cells(Bface_list list);
   CARRAY<QuadCell*>    get_cells(int group, CARRAY<QuadCell*>&);    
   CARRAY<QuadCell*>&   get_cells();     // Looks on the mesh for current cells
   CARRAY<QuadCell*>&   get_visible_cells();     // Looks on the mesh for current cells
   CARRAY<QuadCell*>&   get_ref_cells(); // Looks on the mesh for current ref cells
     
   CARRAY<Cell_List>&   get_sorted_cells();
   CBface*              get_ref_cell_face(QuadCell* current_cell,CBedge* edge);
   CBface*              get_ref_cell_face_random(QuadCell* current_cell,CBedge* edge);
   
   //pick a random cell out of the refs that has compatable configuration
   QuadCell* get_ref_1(QuadCell* cell);  
 
   // Have a thershold for the SSD and picks a random one that is
   // below the threshold
   QuadCell* get_ref_2(QuadCell* cell);  
   
   // Uses get_ref_2 to get the ref cell and then for every group gets a random
   // kind of the group if more then one exist   
   ARRAY<Stroke_List*> get_ref_3(QuadCell* cell);  
   
   //********** STROKE STUFF **************
   Stroke_List*   get_similar_group(int group_id);
   Stroke_List    get_strokes(CARRAY<QuadCell*>& cells);  
   // simplified version of QuadCell::difference_to_cell,
   // picking a group of groups that produce smallest diff
   void           add_to_group_of_groups(Stroke_List* list);     
   
   bool       project_list(mlib::Wpt_list& wlProjList,const mlib::NDCpt_list& ndcpts);
   QuadCell * get_control_cell(mlib::CNDCZpt_list& ndczlScaledList);
   bool     add(mlib::CNDCpt_list &, 
                const ARRAY<double>&nt, 
                BaseStroke * proto,
                double winding,
                double straightness);
   void     clip_to_patch(mlib::CNDCpt_list &pts, mlib::NDCpt_list &cpts, const ARRAY<double>&prl, ARRAY<double>&cprl );
   //******** Ref. image convenience methods ********
   static Bface *  find_face_vis(mlib::CNDCpt& pt, mlib::Wpt &p);

   
   void     set_patch(Patch *p);
   Patch *  patch() { return _patch; }

   //********* Mask Related *********
   void set_empty_mask() { set_mask(_patch->verts(), 0.0);  }
   //void set_full_mask()  { set_mask(_patch->verts(), 1.0);  }  
   void set_mask(Bvert_list, double i);      
   void avarage_mask(Bvert_list list, int n);
   double mask_value(CBface* face,mlib::UVpt uv);   
     
   /******** DATA_ITEM METHODS ********/
   DEFINE_RTTI_METHODS2("PatternGrid", DATA_ITEM, CDATA_ITEM *);
   virtual DATA_ITEM *dup() const { return new PatternGrid; }
  
};

/**********************************************************************
* MaskData:
**********************************************************************/
#define CMaskData const MaskData
class MaskData : public SimplexData {
public:
    MaskData(Bsimplex* s, PatternGrid* p, double v)
             : SimplexData(uint(p), s),
             _mask_val(v) {}

    //******** RUN TIME TYPE ID ********
    DEFINE_RTTI_METHODS2("MaskData", SimplexData, CSimplexData*);

    //******** LOOKUP ********

         // Lookup a PatternGridData* from a Bsimplex:
    static MaskData* lookup(CBsimplex* s, PatternGrid* p) {
        MaskData *one;
        if(s) {
          one = (MaskData*)(s->find_data(uint(p)));
          if(one && !one->is_of_type(MaskData::static_name())) return 0;
          return (one) ? one : 0;
        }
        return 0;

    }
    void     set_mask(double num) { _mask_val = num;  }
    double   get_mask()     const { return _mask_val; }
    //******** SimplexData VIRTUAL METHODS ********
    // These are here to prevent warnings:
    // store it on the given simplex:
    void set(uint id, Bsimplex* s)       { SimplexData::set(id,s); }
    void set(Cstr_ptr& str, Bsimplex* s) { SimplexData::set(str,s); }
protected:
    double _mask_val;

    static uint key() {
      uint ret = (uint)**static_name();
      return ret;
    }


};
/*****************************************************************
 * DRAW_STROKE_CMD
 *
 *      Perform an DRAW_STROKE operation.
 *
 *****************************************************************/
MAKE_PTR_SUBC(DRAW_STROKE_CMD,COMMAND);
class DRAW_STROKE_CMD : public COMMAND {
 public:
   //******** MANAGERS ********
   DRAW_STROKE_CMD(QuadCell *cell, Pattern3dStroke* stroke) :
       _cell(cell),
       _stroke(stroke)
   {}

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS2("DRAW_STROKE_CMD", COMMAND, CCOMMAND*);

   //******** COMMAND VIRTUAL METHODS ********
   virtual bool doit();
   virtual bool undoit();
   virtual bool clear();

   //******** DIAGNOSTIC ********

   virtual void print() const {
      cerr << class_name();
   }

 protected:
   QuadCell*        _cell;
   Pattern3dStroke* _stroke;
};
/*****************************************************************
 * MAKE_CELL_CMD
 *
 *      Perform an  MAKE_CELL_CMD operation.
 *
 *****************************************************************/
MAKE_PTR_SUBC(MAKE_CELL_CMD,COMMAND);
class MAKE_CELL_CMD : public COMMAND {
 public:
   //******** MANAGERS ********
   MAKE_CELL_CMD(QuadCell* cell, CBface* face=0) :
       _cell(cell),
       _face(face)
   {}

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS2("MAKE_CELL_CMD", COMMAND, CCOMMAND*);

   //******** COMMAND VIRTUAL METHODS ********
   virtual bool doit();
   virtual bool undoit();
 
   //******** DIAGNOSTIC ********
   virtual void print() const {
      cerr << class_name();
   }

 protected:
   QuadCell*        _cell;
   CBface*          _face;
};

/*****************************************************************
 * EXPEND_CELL_CMD
 *
 *      Perform an EXPEND_CELL_CMD operation.
 *
 *****************************************************************/
MAKE_PTR_SUBC(EXPEND_CELL_CMD,COMMAND);
class EXPEND_CELL_CMD : public COMMAND {
 public:
   //******** MANAGERS ********
   EXPEND_CELL_CMD(QuadCell* cell, CBface* ref_face) :
       _cell(cell),
       _ref_face(ref_face)
   {}

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS2("EXPEND_CELL_CMD", COMMAND, CCOMMAND*);

   //******** COMMAND VIRTUAL METHODS ********
   virtual bool doit();
   virtual bool undoit();
   
   //******** DIAGNOSTIC ********
   virtual void print() const {
      cerr << class_name();
   }

 protected:
   QuadCell*        _cell;
   CBface*          _ref_face;
};

class UVdiscontinuousEdgeFilter : public SimplexFilter {
 public:
   virtual bool accept(CBsimplex* s) const {
      return is_edge(s) && !UVdata::is_continuous((Bedge*)s);
   }
};

#endif // _PATTERN_GRID_H_IS_INCLUDED_

/* end of file pattern_grid.H */
