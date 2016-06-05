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
    pattern_grid.C
    
    PatternGrid
       
    -------------------
    Simon Breslav
    Fall 2004
 ***************************************************************************/

#if (defined (WIN32) && defined(_MSC_VER) && (_MSC_VER <=1300)) /*VS 6.0*/
#pragma warning(disable: 4786)
#endif

#include <map>
#include <set>

using namespace std;

#include "std/config.H"
#include "std/run_avg.H"
#include "mlib/points.H"

using namespace mlib;

#include "gtex/ref_image.H"
#include "npr/hatching_group_base.H"

#include "pattern/pattern_texture.H"
#include "pattern/pattern_stroke.H"

#include "pattern_grid.H"

float PatternGrid::MAX_DIFF_THRESHOLD = 0.3f;

static bool debug_grid = Config::get_var_bool("DEBUG_GRID",false,true);


PatternGrid::PatternGrid(Patch *p) : 
                             _grid_lines(new GridTexture(p)),                            
                             _show_grid_lines(true), 
                             _visible(true),
                             _alpha_press(false),
                             _width_press(false)
{   
   set_patch(p);  
   _light_on = false;
   _light_width = true;
   _light_alpha = true;
   _light_num = 0;
   _light_type = 0;
   _light_cam_frame=0;
   _light_a = 0.3;
   _light_b = 0.7;     
 
   _lod_on = false;
   _lod_alpha = true;
   _lod_width = false;
   _lod_high = 0.2;
   _lod_low = 0.4; 
   
}

void
PatternGrid::set_patch(Patch *p)
{  _patch = p;     
   if(_grid_lines){
      _grid_lines->set_patch(p);
      _grid_lines->set_grid(this);
   }   
}

PatternGrid::~PatternGrid()
{   
   delete _grid_lines;
   get_cells();      
   for (vector<QuadCell*>::size_type k=0; k < _cells.size(); k++) {
      delete _cells[k];
   } 
}

bool
PatternGrid::is_ref_group(int group)
{
   get_ref_groups();
   for (vector<int>::size_type i=0; i < _ref_groups.size(); ++i) {
      if (group == _ref_groups[i])
         return true;
   }
   return false;   
}

void 
PatternGrid::get_ref_groups()
{
   get_ref_cells();
   set<int> unique;
   for (vector<QuadCell*>::size_type i =0; i < _ref_cells.size(); ++i) {
       pair<set<int>::iterator,bool> it;
       it = unique.insert(_ref_cells[i]->get_group_id());
       if (it.second)
          _ref_groups.push_back(_ref_cells[i]->get_group_id());
   }      
}

void
PatternGrid::clear_cell_markings()
{
    get_cells();
    for (vector<QuadCell*>::size_type k=0; k < _cells.size(); k++) {
        if (_cells[k]->is_empty())
           _cells[k]->set_unmarked();
    }
}

void
PatternGrid::set_all_cell_markings()
{
    get_cells();
    for (vector<QuadCell*>::size_type k=0; k < _cells.size(); k++) {
       _cells[k]->set_marked();
    }
}

void
PatternGrid::set_unmarked(vector<QuadCell*> list)
{
    for (vector<QuadCell*>::size_type k=0; k < list.size(); k++) {
        if(list[k]->is_empty())
           list[k]->set_unmarked();
    }
}

const vector<QuadCell*>
PatternGrid::get_cells(Bface_list list)
{
   vector<QuadCell*> cells;
   for (int k=0; k < list.num();k++){
      CellData* cd = CellData::lookup(list[k], this);
      if(cd)
         cells.add_uniquely(cd->get_cell());
   }
   return cells;
}

const vector<QuadCell*>
PatternGrid::get_cells(int group, const vector<QuadCell*>& source)
{
    vector<QuadCell*> cells;

    for (vector<QuadCell*>::size_type k=0; k < source.size(); k++) {
      if(source[k]->get_group_id() == group)
         cells.add_uniquely(source[k]);
    }   

    return cells;
}

const vector<QuadCell*>&
PatternGrid::get_cells()
{
    if (_cells.size() > 0)
       _cells.clear();
    Bface_list fs = _patch->faces().primary_faces();

    set<QuadCell*> unique;
    for (int k=0; k < fs.num(); k++) {
      CellData* cd = CellData::lookup(fs[k], this);
      if (cd) {
         pair<set<QuadCell*>::iterator,bool> it;
         it = unique.insert(cd->get_cell());
         if (it.second)
            _cells.push_back(cd->get_cell());
      }
    }

    return _cells; 
}

const vector<QuadCell*>&
PatternGrid::get_visible_cells()
{
    if (_cells.size() > 0)
       _cells.clear();
    Bface_list fs = _patch->faces().primary_faces();

    set<QuadCell*> unique;
    for (int k=0; k < fs.num(); k++) {
       CellData* cd = CellData::lookup(fs[k], this);
       if (cd && cd->get_cell()->is_visible()) {
          pair<set<QuadCell*>::iterator,bool> it;
          it = unique.insert(cd->get_cell());
          if (it.second)
             _cells.push_back(cd->get_cell());
       }
    }

    return _cells; 
}

const vector<QuadCell*>&
PatternGrid::get_ref_cells()
{
    get_cells();
   
    if (_ref_cells.size() > 0)
       _ref_cells.clear();

    set<QuadCell*> unique;
    for (vector<QuadCell*>::size_type k=0; k < _cells.size(); k++) {
       if (_cells[k]->is_ref()) {
          pair<set<QuadCell*>::iterator,bool> it;
          it = unique.insert(_cells[k]);
          if (it.second)
             _ref_cells.push_back(_cells[k]);
       }
    }   

    return _ref_cells;
}

const vector<Cell_List>&
PatternGrid::get_sorted_cells()
{
   if (_sorted_cells.size() > 0)
       _sorted_cells.clear();   
   get_cells();
  
   //cerr << "cells has " << _cells.size() << endl;
   for (vector<QuadCell*>::size_type k=0; k < _cells.size(); ++k){
      bool added = false;
      for (vector<Cell_List>::size_type m = 0; m < _sorted_cells.size(); ++m) {
         // Find a group of cells that we can add the cell to
         if (_sorted_cells[m].add_cell(_cells[k])) {
            _cells[k]->set_group_id(m); //tell the cell what type it is            
            added = true;
            break;     
         }            
      }
      if(!added){
         // Make a new Group
         Cell_List new_list;
         new_list.add_cell(_cells[k]);         
         _sorted_cells.push_back(new_list);
         _cells[k]->set_group_id(_sorted_cells.size()-1);
      }
      
   }
   return _sorted_cells; 
}

CBface*
PatternGrid::get_ref_cell_face(QuadCell* current_cell,CBedge* edge)
{  
   get_sorted_cells();  //separate into groups and assign group_id's to cells
  
   int group_id = current_cell->get_group_id();
   CBface* ref_face;
   UVpt uv, uv2;
   if(!current_cell->vert_uv(edge->v1(), uv) || !current_cell->vert_uv(edge->v2(), uv2)){
      cerr << "could not get the vert uv" << endl;
      return 0;
   }
   
   // cerr << "there are " << _sorted_cells.size() << "groups, id is " << group_id << endl;
   std::map<int, CBface*, std::greater<double> > candidates;
   
   for (int i =0; i < _sorted_cells[group_id].num(); ++i){
      // same_config means there is no configuration controdictions     
    
      int compatability = 0;
      if(_sorted_cells[group_id][i]->is_ref() && _sorted_cells[group_id][i]->same_config(current_cell, compatability)){
         CBedge* ref_edge = _sorted_cells[group_id][i]->get_edge(uv, uv2);
         //cerr << "comp is " << compatability << endl;
         if(!ref_edge)
            cerr << "NO ref_edge" << endl;
         if((ref_face = _sorted_cells[group_id][i]->outside_cell_face(ref_edge))){
            //cerr << "found a conditate" << endl;
            //int compatability = get_num_of_neighbors(ref_face);
            candidates[compatability] = ref_face;
         }            
      }      
   }

   if (!candidates.empty()) {
      return candidates.begin()->second;
   } 
   return 0; 
}

CBface*
PatternGrid::get_ref_cell_face_random(QuadCell* current_cell,CBedge* edge)
{  
   get_sorted_cells();  //separate into groups and assign group_id's to cells
  
   int group_id = current_cell->get_group_id();
   CBface* ref_face;
   UVpt uv, uv2;
   if(!current_cell->vert_uv(edge->v1(), uv) || !current_cell->vert_uv(edge->v2(), uv2))
      return 0;
   
   //cerr << "there are " << _sorted_cells.size() << "groups, id is " << group_id << endl;
   vector<CBface*> candidates;
   for (vector<Cell_List>::size_type i =0; i < _sorted_cells[group_id].num(); ++i) {
      // same_config means there is no configuration controdictions     
      if(_sorted_cells[group_id][i]->is_ref()){
         CBedge* ref_edge = _sorted_cells[group_id][i]->get_edge(uv, uv2);
         if((ref_face = _sorted_cells[group_id][i]->outside_cell_face(ref_edge)))
            candidates.push_back(ref_face);
      }      
   }
   if (!candidates.empty()) {
      vector<CBface*>::size_type n = candidates.size()-1;
      return candidates[min((size_t)round(drand48()*n), n)];
   }
   return 0; 
}

QuadCell*
PatternGrid::get_ref_1(QuadCell* cell)
{
   get_ref_cells(); // make sure _ref_cells is to date and strokes are 
                    // divided into groups
  
   if (_ref_cells.empty()) {
      return 0;      
   } else { 
      vector<QuadCell*> the_cells = get_cells(cell->get_group_id(), _ref_cells);
      if (the_cells.empty()) {
        vector<QuadCell*>::size_type n = _ref_cells.size()-1;
        return _ref_cells[min((size_t)(drand48()*n), n)];
      } else {
        vector<QuadCell*>::size_type n = the_cells.size()-1;
        return the_cells[min((size_t)(drand48()*n), n)];
      }
    }
}


QuadCell*
PatternGrid::get_ref_2(QuadCell* cell)
{
    get_ref_cells(); // make sure _ref_cells is to date 
    //Get all the cells that have some similar neighbors
   vector<QuadCell*> candidates;
   for (vector<QuadCell*>::size_type i=0; i < _ref_cells.size(); ++i) {
      if ((cell->difference_neighbors(_ref_cells[i])) < MAX_DIFF_THRESHOLD) {
         candidates.push_back(_ref_cells[i]);
      }
   }

   if (candidates.empty()) {
      vector<QuadCell*>::size_type n = _ref_cells.size()-1;
      return _ref_cells[min((size_t)round(drand48()*n), n)];
   } else {
       vector<QuadCell*>::size_type n = candidates.size()-1;
       return candidates[min((size_t)round(drand48()*n), n)];
   }
}  

/*
Gets a random group from the row with group_id given
*/
Stroke_List*
PatternGrid::get_similar_group(int group_id)
{
  vector<Group_List>::size_type n = _group_list[group_id].size()-1;
  return _group_list[group_id][min((size_t)round(drand48()*n), n)];
}

Stroke_List
PatternGrid::get_strokes(const vector<QuadCell*>& cells)
{
   Stroke_List list;
   for (vector<QuadCell*>::size_type m=0; m < cells.size(); ++m) {
       list += cells[m]->get_strokes();
       // Get strokes from the group       
       for(int i=0; i < cells[m]->groups_num(); ++i){
           list += *(cells[m]->get_groups_strokes(i));
       }    
   }
   return list;    
    
}

// Adds a new group to the group of groups
void           
PatternGrid::add_to_group_of_groups(Stroke_List* list)
{
  // Go through all the groups and find the group that produced the
  // smallest group diff, add it to that group
  bool added = false;
  if(!_group_list.empty()){     
     double sd=0;
     std::map<double, int, std::less<double> > group_diffs;
     for (vector<Group_List>::size_type i=0; i < _group_list.size(); ++i) {
        // XXX Group_List should add up all the diffs to all cells and give an 
        // avarage of the diffs
        sd = _group_list[i][0]->difference_to_group(list);
        group_diffs.insert(std::make_pair(sd, i));
     }
     err_msg("group diff is %f", group_diffs.begin()->first);
     if(group_diffs.begin()->first < MAX_DIFF_THRESHOLD){  
        err_msg("add_to_group_of_groups: adding to group %d", group_diffs.begin()->second);
        _group_list[group_diffs.begin()->second] += list;
        list->set_group_id(group_diffs.begin()->second);
        added = true;        
     } 
     //make a new row in the list
  }
  if(!added){  
     Group_List new_list;
     new_list += list;
     _group_list.push_back(new_list);
     err_msg("add_to_group_of_groups: creating new group");
     list->set_group_id(_group_list.size()-1);
  }      
}

vector<Stroke_List*>
PatternGrid::get_ref_3(QuadCell* new_cell)
{
   QuadCell* ref_cell = get_ref_2(new_cell);
   vector<Stroke_List*> list;
   if (!ref_cell) {
      err_msg("SORRY could not find ref cell");
      return list;
   }       
   // Put all the strokes from ref into a new cell
   if (new_cell && ref_cell){   
        for (int m=0; m < ref_cell->groups_num(); ++m){ 
           Stroke_List* group = get_similar_group(ref_cell->get_groups_strokes(m)->get_group_id()); 
           list.push_back(group);
        }
   }
   return list;
}  

void
PatternGrid::clip_to_patch(
                     CNDCpt_list &pts,        NDCpt_list &cpts,
                     const vector<double>&prl, vector<double>&cprl )
{
   size_t k;
   bool started = false;
   Bface *f;
   Wpt foo;

   for (k=0; k<pts.size(); k++) {
      f = find_face_vis(pts[k], foo);
      if (f && f->patch() == _patch) {
         started = true;
         cpts.push_back(pts[k]);
         cprl.push_back(prl[k]);
      } else {
         if (started) {
            k=pts.size();
         }
      }
   }
}

Bface *
PatternGrid::find_face_vis(CNDCpt& pt,Wpt &p)
{
   static VisRefImage *vis_ref = VisRefImage::lookup(VIEW::peek());

   if (!vis_ref) {
      err_mesg(ERR_LEV_ERROR, "PatternGroup::find_face_vis() - Error: Can't get visibility reference image!");
      return 0;
   }
   Bsimplex *s = vis_ref->intersect_sim(pt, p);
   if (is_face(s))
      return (Bface*)s;
   else
      return 0;
}

bool
PatternGrid::project_list(Wpt_list& wlProjList,const NDCpt_list& ndcpts){
   Wpt wloc;
   Bface* f;
   for (NDCpt_list::size_type k=0; k<ndcpts.size(); k++) {
      f = find_face_vis(NDCpt(ndcpts[k]),wloc);
      if (f && f->patch() == _patch && f->front_facing()) {
            wlProjList.push_back(wloc);
      } else {
            if (!f)
            err_adv(debug_grid, "PatternGroup::add() - Missed while projecting: No hit on a mesh!");
            else if (!(f->patch() == _patch))
            err_adv(debug_grid, "PatternGroup::add() - Missed while projecting: Hit wrong patch.");
            else if (!f->front_facing())
            err_adv(debug_grid, "PatternGroup::add() - Missed while projecting: Hit backfacing tri.");
            else
            err_adv(debug_grid, "PatternGroup::add() - Missed while projecting: WHAT?!?!?!?!");
      }
   }
   if (wlProjList.size()<2) {
      err_adv(debug_grid, "PatternGroup:add() - Nothing left after projection failures. Punting...");
      return false;
   }
   return true;
}

QuadCell *
PatternGrid::get_control_cell(CNDCZpt_list& ndczlScaledList){
   Wpt wl;
   Bface* f;
   std::map<QuadCell*, int> cell_count;
   CellData* tmp_cd;
   QuadCell* tmp_cell;
   int max_count = 0;
   
   // Find the control cell for the stroke
   for (NDCZpt_list::size_type k=0; k < ndczlScaledList.size(); k++) {
       f = find_face_vis(NDCpt(ndczlScaledList[k]), wl);
       tmp_cd = CellData::lookup(f, this);
       if(!tmp_cd)          
          continue;
      
       tmp_cell   = tmp_cd->get_cell();
       
      //initialize count to 0
      if(cell_count.find(tmp_cell) == cell_count.end())
         cell_count[tmp_cell] = 0;
       
       cell_count[tmp_cell] = cell_count[tmp_cell] + 1; 
       if (cell_count[tmp_cell] > max_count) max_count = cell_count[tmp_cell];      
   }     

   for(std::map<QuadCell*, int>::iterator it = cell_count.begin(); it != cell_count.end(); ++it){
      if(it->second == max_count)
        return it->first; 
   } 
   return 0;  
   
}

bool
PatternGrid::add(CNDCpt_list &pl,
             const vector<double>&prl,
             BaseStroke * proto,
                 double winding,
                 double straightness)
{    
   int k;
   Bface *f;
    
   // It happens:
   if (pl.empty()){
      err_msg("PatternGroup:add() - Error: point list is empty!");
      return false;
   }
   if (prl.empty()){
      err_msg("PatternGroup:add() - Error: pressure list is empty!");
      return false;
   }
   if (pl.size() != prl.size()){
      err_msg("PatternGroup:add() - gesture pixel list and pressure list are not same length.");
      return false;
   }

  
   NDCpt_list              smoothpts;
   vector<double>          smoothprl;
   if (!(HatchingGroupBase::smooth_gesture(pl, smoothpts, prl, smoothprl, 99)))
      return false;
   
   // Clip to patch
   NDCpt_list              ndcpts;
   vector<double>          finalprl;
   clip_to_patch(smoothpts,ndcpts,smoothprl,finalprl);
   ndcpts.update_length();
 
   // Project to surface
   err_adv(debug_grid, "PatternGroup:add() - Projecting points");
   Wpt_list wlProjList;
   if(!project_list(wlProjList, ndcpts)) return false;
    wlProjList.update_length();

   // Resample Points
   err_adv(debug_grid, "PatternGroup:add() - Resampling points");
   Wpt_list wlScaledList;

   for (k=0 ; k<wlProjList.size(); k++)
         wlScaledList.push_back(wlProjList[k]);

   // Convert back to 2D
   err_adv(debug_grid, "PatternGroup:add() - converting to 2D.");
   NDCZpt_list ndczlScaledList;
   for (k=0;k<wlScaledList.size();k++) ndczlScaledList.push_back(NDCZpt(_patch->xform()*wlScaledList[k]));
   ndczlScaledList.update_length();

   // Calculate pixel length of a stroke
   double pix_len = ndczlScaledList.length() * VIEW::peek()->ndc2pix_scale();
   if (pix_len < 8.0)   {
      err_adv(debug_grid, "PatternGroup::add() - Stroke only %f pixels. Probably an accident. Punting...", pix_len);
      return false;
   }
    
   // Find the control cell for the stroke
    QuadCell* control_cell = get_control_cell(ndczlScaledList); 
    if(!control_cell) {
        WORLD::message("Need Cells to draw on");
        return false;
    } 
   
   vector<CBface*>               faces;
   Wpt_list                      pts;
   vector<Wvec>                  norms;
   vector<Wvec>                  bar;
   vector<double>                alpha;
   vector<double>                width;
   UVpt_list                     uv_p; 

   cerr << "we have " << ndczlScaledList.size() << " and pressur " << finalprl.size() << endl;
   for (k=0; k<ndczlScaledList.size(); k++) {
      Wpt wloc(ndczlScaledList[k]);
      f = control_cell->quad(0,0);

      if (f && f->patch() == _patch && f->front_facing()) {
         Wvec bc;
         Wvec norm;
         UVpt uv;
         
         f->project_barycentric_ndc(NDCpt(ndczlScaledList[k]),bc);

         uv = control_cell->quad_bc_to_cellUV(bc, f);
           
         double w_p = (_width_press) ? finalprl[k] : 1.0;
         double a_p = (_alpha_press) ? finalprl[k] : 1.0;
         Wpt foo;
         CBface* f_tmp = find_face_vis(NDCpt(ndczlScaledList[k]), foo);
         faces.push_back(f_tmp);

         alpha.push_back(a_p);
         width.push_back(w_p);
         uv_p .push_back(uv);

         Wvec bc_t;
         f_tmp->project_barycentric(wloc,bc_t);

         f_tmp->bc2norm_blend(bc_t,norm);

         bar.push_back(bc_t);
         pts.push_back(wloc);
         norms.push_back(norm);
      } else {
         if (!f)
            err_adv(debug_grid, "PatternGroup::add() - Missed in final lookup: No hit on a mesh!");
         else if (!(f->patch() == _patch))
            err_adv(debug_grid, "PatternGroup::add() - Missed in final lookup: Hit wrong patch.");
         else if (!(f->front_facing()))
            err_adv(debug_grid, "PatternGroup::add() - Missed in final lookup: Hit backfracing tri.");
         else
            err_adv(debug_grid, "PatternGroup::add() - Missed in final lookup: WHAT?!?!?!?!");
      }
   }

   if (pts.size()>1) {
      //XXX  - Okay, using the gesture pressure, but no offsets.
      //Need to go back and add offset generation...

      BaseStrokeOffsetLISTptr ol = make_shared<BaseStrokeOffsetLIST>();

      ol->set_replicate(0);
      ol->set_hangover(1);
      ol->set_pix_len(pix_len);

      ol->add(BaseStrokeOffset( 0.0, 0.0, finalprl[0], BaseStrokeOffset::OFFSET_TYPE_BEGIN));
      for (k=1; k< finalprl.size(); k++)
            ol->add(BaseStrokeOffset( (double)k/(double)(finalprl.size()-1), 0.0, finalprl[k], BaseStrokeOffset::OFFSET_TYPE_MIDDLE));

      ol->add(BaseStrokeOffset( 1.0, 0.0, finalprl[finalprl.size()-1],   BaseStrokeOffset::OFFSET_TYPE_END));

      DRAW_STROKE_CMDptr cmd = make_shared<DRAW_STROKE_CMD>( control_cell, new Pattern3dStroke( control_cell,
                                                     faces,
                                                     pts,
                                                     norms,
                                                     bar,
                                                     alpha, 
                                                     width, 
                                                     uv_p, 
                                                     ol, 
                                                     proto, 
                                                     winding,
                                                     straightness));
      WORLD::add_command(cmd);  
      //control_cell->add_stroke();
               
      return true;
   }
   else
   {
      err_adv(debug_grid, "PatternGroup:add() - All lookups are bad. Punting...");
      return false;
   }
   
   return true;
}

void
PatternGrid::set_mask(Bvert_list list, double i)
{
    for (int k=0; k < list.num(); ++k){
      MaskData* md = MaskData::lookup(list[k], this);
      if(md)
         md->set_mask(i);
      else
         new MaskData(list[k], this, i);       
    }    
}

void
PatternGrid::avarage_mask(Bvert_list list, int n)
{
    for (int i = 0; i < n; ++i) {
       for (int k=0; k < list.num();k++) {
          MaskData* md = MaskData::lookup(list[k], this);
          if (md) {
            RunningAvg<double> mask_vals(0);
            mask_vals.push_back(md->get_mask());

            vector<Bvert*> nbrs;
            list[k]->get_p_nbrs(nbrs);
            for (vector<Bvert*>::size_type m=0; m < nbrs.size(); ++m) {
               MaskData* md2 = MaskData::lookup(nbrs[m], this);
               if (md2)
                  mask_vals.push_back(md2->get_mask());
            }
            md->set_mask(min(mask_vals.val(), 1.0));
          }
       }
    }    
}

double
PatternGrid::mask_value(CBface* face,UVpt uv)
{
    Bvert *a, *b, *c, *d;                
    face->get_quad_verts(a, b, c, d);
    MaskData* md  = MaskData::lookup(a, this);
    MaskData* md2 = MaskData::lookup(b, this);
    MaskData* md3 = MaskData::lookup(c, this);
    MaskData* md4 = MaskData::lookup(d, this);
    assert(md && md2 && md3 && md4);
    double m_a = md->get_mask();
    double m_b = md2->get_mask();
    double m_c = md3->get_mask();
    double m_d = md4->get_mask();                                          
    return interp(interp(m_a, m_b, uv[0]), interp(m_d, m_c, uv[0]), uv[1]);   
    
}

bool
DRAW_STROKE_CMD::doit()
{
   if (is_done())
      return true;

   if (!(_cell && _stroke)) {
      return true;
   }
   _cell->add_stroke(_stroke);
   return COMMAND::doit();
}

bool
DRAW_STROKE_CMD::undoit()
{
   if (!is_done())
      return true;

   if (!(_cell && _stroke)) {
      return true;
   }
 
   _cell->remove_stroke(_stroke);
   return COMMAND::undoit();
}
bool
DRAW_STROKE_CMD::clear()
{
   if (is_clear())
      return true;
   if (is_done())
      return false;

   if (!COMMAND::clear()) 
      return false;

   if ( _stroke ) {
      delete _stroke;
      _stroke = 0;
   }
  
   return true;
}
//////////////////////////////////////////////////////
bool
MAKE_CELL_CMD::doit()
{
   if (is_done())
      return true;

   if (!(_cell)) {
      return true;
   }
   if(_face){
      _cell->add_quad(_face);
      _cell->pg()->get_grid_lines()->set_modify_stamp();
   }
   return COMMAND::doit();
}

bool
MAKE_CELL_CMD::undoit()
{
   if (!is_done())
      return true;

   if (!(_cell)) {
      return true;
   }
   QuadCell* c = _cell;
   _cell->delete_cell();
   _cell = 0;
   c->pg()->get_grid_lines()->set_modify_stamp();
   return COMMAND::undoit();
}
//////////////////////////////////////////////////////
bool
EXPEND_CELL_CMD::doit()
{
   if (is_done())
      return true;

   if (!(_cell)) {
      return true;
   }
   if(_ref_face){
      _cell->expand(_ref_face);
      _cell->pg()->get_grid_lines()->set_modify_stamp();      
   }
   return COMMAND::doit();
}

bool
EXPEND_CELL_CMD::undoit()
{
   if (!is_done())
      return true;

   if (!(_cell)) {
      return true;
   }
 
   QuadCell* c = _cell;
   _cell->delete_cell();
   _cell = 0;
   c->pg()->get_grid_lines()->set_modify_stamp();
   return COMMAND::undoit();
}
/* end of file pattern_grid.C */
