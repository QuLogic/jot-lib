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
#include "stroke_pattern.H"
#include "stroke_group.H"
#include "stroke_path.H"

using namespace std;
using namespace mlib;;


///////////////////////////////
// constructor / destructor
///////////////////////////////


void 
StrokePattern::delete_groups(){
  unsigned int nb_stroke_groups = _stroke_groups.size();
  for(unsigned int i=0; i<nb_stroke_groups ; i++){
    delete _stroke_groups[i];
  }  
  _stroke_groups.clear();
}


void
StrokePattern::set_epsilon(double eps){
  // first change epsilon
  _epsilon = eps;

  // replace the last group with a new one at the proper epsilon
  if (_stroke_groups.empty()) return;
  const StrokeGroup* last_group = _stroke_groups.back();

  // first get its strokes
  vector<GestureStroke> group_strokes;
  const vector<StrokePaths*>& paths = last_group->paths();
  unsigned int nb_paths = paths.size();
  for (unsigned int j=0 ; j<nb_paths ; j++){
    const vector<GestureStroke>& strokes = paths[j]->strokes();
    unsigned int nb_strokes = strokes.size();
    for (unsigned int k=0 ; k<nb_strokes ; k++){
      GestureStroke old_stroke = strokes[k];
      group_strokes.push_back(old_stroke);
    }
  }
    
  // then create a new group and delete the old one
  _stroke_groups.pop_back();
  add_group(last_group->type(), last_group->reference_frame(), group_strokes);
  delete last_group;
}

void
StrokePattern::set_style_adjust(double sa){
  // first change epsilon
  _style_adjust = sa;

  // replace the last group with a new one at the proper epsilon
  if (_stroke_groups.empty()) return;
  const StrokeGroup* last_group = _stroke_groups.back();

  // first get its strokes
  vector<GestureStroke> group_strokes;
  const vector<StrokePaths*>& paths = last_group->paths();
  unsigned int nb_paths = paths.size();
  for (unsigned int j=0 ; j<nb_paths ; j++){
    const vector<GestureStroke>& strokes = paths[j]->strokes();
    unsigned int nb_strokes = strokes.size();
    for (unsigned int k=0 ; k<nb_strokes ; k++){
      GestureStroke old_stroke = strokes[k];
      group_strokes.push_back(old_stroke);
    }
  }
    
  // then create a new group and delete the old one
  _stroke_groups.pop_back();
  add_group(last_group->type(), last_group->reference_frame(), group_strokes);
  delete last_group;
}



void
StrokePattern::set_analyze_style(bool b){
  // first change var
  _analyze_style = b;

  // replace the last group with a new one at the proper epsilon
  if (_stroke_groups.empty()) return;
  const StrokeGroup* last_group = _stroke_groups.back();

  // first get its strokes
  vector<GestureStroke> group_strokes;
  const vector<StrokePaths*>& paths = last_group->paths();
  unsigned int nb_paths = paths.size();
  for (unsigned int j=0 ; j<nb_paths ; j++){
    const vector<GestureStroke>& strokes = paths[j]->strokes();
    unsigned int nb_strokes = strokes.size();
    for (unsigned int k=0 ; k<nb_strokes ; k++){
      GestureStroke old_stroke = strokes[k];
      group_strokes.push_back(old_stroke);
    }
  }
    
  // then create a new group and delete the old one
  _stroke_groups.pop_back();
  add_group(last_group->type(), last_group->reference_frame(), group_strokes);
  delete last_group;
}



//////////////////
// analysis
//////////////////

void
StrokePattern::add_group(int type, int ref_frame, const vector<GestureStroke>& strokes){
  // first fit singletons elements to strokes 
  vector<StrokePaths*> singletons;
  int path_type;
  if (type==StrokeGroup::HATCHING){
    path_type = StrokePaths::LINE;
  } else if (type==StrokeGroup::STIPPLING){
    path_type = StrokePaths::POINT;
  } else {
    path_type = StrokePaths::BBOX;    
  }
  fit_singletons(strokes, path_type, singletons);

  // then cluster singletons into elements, 
  // compute their statistics and add them to a new group
  StrokeGroup* new_group = new StrokeGroup(type, ref_frame);
  extract_elements(singletons, new_group);

  // analyze the group and add it to the pattern
  new_group->analyze(_analyze_style);
  _stroke_groups.push_back(new_group);

  // update the bbox and indicate it has changed
  unsigned int nb_strokes = strokes.size();
  for (unsigned int i=0 ; i<nb_strokes ; i++){
    _bbox += strokes[i].gesture()->pix_bbox();
  }
  _complete = false;

}


void 
StrokePattern::fit_singletons(const vector<GestureStroke>& strokes, int path_type,
			      vector<StrokePaths*>& singletons){
  unsigned int nb_strokes = strokes.size();
  for (unsigned int i=0 ; i<nb_strokes ; i++){
    StrokePaths* new_path = new StrokePaths(_epsilon, _style_adjust, path_type, _analyze_style, strokes[i]);
    singletons.push_back(new_path);
  }
}

void 
StrokePattern::extract_elements(vector<StrokePaths*>& candidates,
				StrokeGroup* group){
  vector<StrokePaths*> current_candidates = candidates;
  vector<StrokePaths*> next_candidates;
  while(!current_candidates.empty()) {
    bool clustered = false;
    StrokePaths* current_candidate = current_candidates.front();
    unsigned int nb_candidates = current_candidates.size();
    for (unsigned int j=1 ; j<nb_candidates ; j++){
      bool current_clustered;
      if (group->type()==StrokeGroup::STIPPLING) {
	current_clustered = current_candidate->cluster_stippling(current_candidates[j]);
      } else if (group->type()==StrokeGroup::HATCHING){
	current_clustered = current_candidate->cluster_hatching(current_candidates[j]);
      } else {
	current_clustered = current_candidate->cluster_free(current_candidates[j]);	
      }
      if (!current_clustered){
	next_candidates.push_back(current_candidates[j]);
      } else {
	clustered = true;
      }
    }
    if (clustered){
      next_candidates.push_back(current_candidate);
    } else {
      group->add_path(current_candidate);
    }
    current_candidates = next_candidates;
    next_candidates.clear();
  }
}


//////////////////
// synthesis 
//////////////////

void 
StrokePattern::synthesize(int mode, int distrib, bool stretched, GestureCell* target_cell) {
  if (!_bbox.valid() || !target_cell || !target_cell->valid()) {
    return;
  }

  if (!_complete) {
    unsigned int nb_stroke_groups = _stroke_groups.size();
    for(unsigned int i=0; i<nb_stroke_groups ; i++){
      _stroke_groups[i]->set_bbox(_bbox);
    }
    _complete = true;
  }

  if (mode==StrokeGroup::SAMPLE || mode==StrokeGroup::COPY || mode==StrokeGroup::CLONE){ 
    // synthesize strokes to extend the groups and fill the target cell
    // uses nearest neighbors for correction and synthesis in UV (deprecated)
    unsigned int nb_stroke_groups = _stroke_groups.size();
    for(unsigned int i=0; i<nb_stroke_groups ; i++){
      _stroke_groups[i]->set_behavior(mode);
      _stroke_groups[i]->enable_stretching(stretched);
      _stroke_groups[i]->set_correction_amount(_correction_amount);
      _stroke_groups[i]->synthesize_NN(target_cell);
    }    
  } else if (mode==StrokeGroup::MIMIC){
    // synthesize directly "in pixels" using a 1-ring correction
    unsigned int nb_stroke_groups = _stroke_groups.size();
    for(unsigned int i=0; i<nb_stroke_groups ; i++){
      _stroke_groups[i]->set_behavior(mode);
      _stroke_groups[i]->set_distribution(distrib);
      _stroke_groups[i]->enable_stretching(stretched);
      _stroke_groups[i]->set_correction_amount(_correction_amount);
      _stroke_groups[i]->synthesize_1ring(target_cell);
    }    
  } else if (mode==StrokeGroup::EFROS){
    // synthesize directly "in pixels" using a 1-ring correction
    unsigned int nb_stroke_groups = _stroke_groups.size();
    for(unsigned int i=0; i<nb_stroke_groups ; i++){
      _stroke_groups[i]->set_behavior(mode);
      _stroke_groups[i]->set_distribution(distrib);
      _stroke_groups[i]->enable_stretching(stretched);
      _stroke_groups[i]->set_ring_nb(_ring_nb);
      _stroke_groups[i]->set_correction_amount(_correction_amount);
      _stroke_groups[i]->synthesize_1ring(target_cell);
    }    
  }

}

void 
StrokePattern::render_synthesized_strokes(GestureCell* target_cell){
  unsigned int nb_stroke_groups = _stroke_groups.size();
  for(unsigned int i=0; i<nb_stroke_groups ; i++){
    _stroke_groups[i]->set_correction_amount(_correction_amount);
    _stroke_groups[i]->render_synthesized_strokes(target_cell);
  }     
}


/////////////////////
// file management
/////////////////////

void 
StrokePattern::save(string name) const{
  // TODO: save a pattern in XML format
}


void 
StrokePattern::load(string name) const{
  // TODO: load a pattern in XML format
}
