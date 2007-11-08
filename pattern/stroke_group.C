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
#include "stroke_group.H"
#include "stroke_path.H"

#include <queue>
#include <vector>
#include "geom/world.H"

using namespace mlib;

int StrokeGroup::HISTORY_DEPTH = 3;
int StrokeGroup::MAX_COUNT = 200;
double StrokeGroup::MAX_ANGLE = 2*M_PI/3.0;
double StrokeGroup::PROX_THRESHOLD_1D = 0.1;
double StrokeGroup::PAR_THRESHOLD_1D = 0.1*M_PI/2.0;
double StrokeGroup::OV_THRESHOLD_1D = 1.5;
double StrokeGroup::PROX_THRESHOLD_2D = 0.2;
double StrokeGroup::PAR_THRESHOLD_2D = 0.2*M_PI/2.0;
double StrokeGroup::OV_THRESHOLD_2D = 2.0;
double StrokeGroup::COLOR_THRESHOLD = 7.0;

////////////////////////////////
// constructor / destructor
////////////////////////////////

StrokeGroup::~StrokeGroup() {
  unsigned int nb_stroke_paths = _stroke_paths.size();
  for(unsigned int i=0; i<nb_stroke_paths ; i++){
    delete _stroke_paths[i];
  } 
}
 

//////////////////////////
// accessors
//////////////////////////

void
StrokeGroup::add_path(StrokePaths* path){
  _stroke_paths.push_back(path);
}
  



void 
StrokeGroup::set_bbox(CBBOXpix& bbox){
  _bbox = bbox;
  unsigned int nb_stroke_paths = _stroke_paths.size();
  for(unsigned int i=0; i<nb_stroke_paths ; i++){
    _stroke_paths[i]->set_bbox(bbox);
  }    
}



/////////////////
// analysis
/////////////////

void 
StrokeGroup::analyze(bool analyze_style){
  unsigned int nb_stroke_paths = _stroke_paths.size();
  _style_analyzed = analyze_style;

  // 1- compute elements statistics
  if (nb_stroke_paths==0) return;
  compute_elements();
  double length_sum = 0.0, width_sum = 0.0, orientation_sum = 0.0, offset_sum = 0.0;
  double length_norm = 0.0, width_norm = 0.0, orientation_norm = 0.0, offset_norm = 0.0;
  _length.min = 1e19; _width.min = 1e19; _orientation.min = 1e19; _offset.min = 1e19;
  _length.max = -1.0, _width.max = -1.0; _orientation.max = -1.0; _offset.max = -1.0;

  unsigned int nb_elements = _elements.size();
  for (unsigned int i=0 ; i<nb_elements ; i++){
    int idx = _elements[i];
    push_stat(_stroke_paths[idx]->axis_a().length(), 1.0, _length.measures, 
	      length_sum, length_norm, _length.min, _length.max);
    push_stat(_stroke_paths[idx]->axis_b().length(), 1.0, _width.measures, 
	      width_sum, width_norm, _width.min, _width.max);
    push_stat(get_angle(_stroke_paths[idx]), 1.0, _orientation.measures, 
	      orientation_sum, orientation_norm, _orientation.min, _orientation.max);
    push_stat(get_offset(_stroke_paths[idx]), 1.0, _offset.measures, 
	      offset_sum, offset_norm, _offset.min, _offset.max);
    if (analyze_style) {
      //     push_stat(get_pressure(_stroke_paths[idx1], _stroke_paths[idx2]), 1.0, _pressure.measures, 
      // 	      pressure_sum, pressure_norm, _pressure.min, _pressure.max);    
    }
  }
  _orientation.min = 0.0; _orientation.max = 1.0;
  compute_avg_std(length_sum, length_norm, _length.measures, _length.avg, _length.std);
  compute_avg_std(width_sum, width_norm, _width.measures, _width.avg, _width.std);
  compute_avg_std(orientation_sum, orientation_norm, _orientation.measures, _orientation.avg, _orientation.std);
  compute_avg_std(offset_sum, offset_norm, _offset.measures, _offset.avg, _offset.std);
  if (analyze_style) {
//     compute_avg_std(pressure_sum, pressure_norm, _pressure.measures, _pressure.avg, _pressure.std);
  }



  // 2- compute element neighbor statistics 
  if (nb_elements<2) {
    _element_neighbors = vector< vector<int> >(nb_elements);
    _element_pairs = vector< pair<int, int> > (nb_elements);
    return;
  }
  compute_element_neighbors();
  _relative_position = vector<Property>(nb_elements);
  _relative_length = vector<Property>(nb_elements);
  _relative_width = vector<Property>(nb_elements);
  _relative_orientation = vector<Property>(nb_elements);

  for (unsigned int i=0 ; i<nb_elements ; i++){
    int idx1 = _elements[i];
    double relative_position_sum = 0.0, relative_length_sum = 0.0, relative_width_sum = 0.0, relative_orientation_sum = 0.0;
    double relative_position_norm = 0.0, relative_length_norm = 0.0, relative_width_norm = 0.0, relative_orientation_norm = 0.0;
    _relative_position[idx1].min = 1e19; _relative_length[idx1].min = 1e19; _relative_width[idx1].min = 1e19; _relative_orientation[idx1].min = 1e19;
    _relative_position[idx1].max = -1.0; _relative_length[idx1].max = -1.0; _relative_width[idx1].max = -1.0; _relative_orientation[idx1].max = -1.0;
    
    unsigned int nb_element_neighbors = _element_neighbors[idx1].size();
    for (unsigned int j=0 ; j<nb_element_neighbors ; j++){
      int idx2 = _element_neighbors[idx1][j];
      push_stat(get_relative_position(_stroke_paths[idx1], _stroke_paths[idx2]), 1.0, _relative_position[idx1].measures, 
		relative_position_sum, relative_position_norm, _relative_position[idx1].min, _relative_position[idx1].max);
      push_stat(get_relative_length(_stroke_paths[idx1], _stroke_paths[idx2]), 1.0, _relative_length[idx1].measures, 
		relative_length_sum, relative_length_norm, _relative_length[idx1].min, _relative_length[idx1].max);
      push_stat(get_relative_width(_stroke_paths[idx1], _stroke_paths[idx2]), 1.0, _relative_width[idx1].measures, 
		relative_width_sum, relative_width_norm, _relative_width[idx1].min, _relative_width[idx1].max);
      push_stat(get_relative_orientation(_stroke_paths[idx1], _stroke_paths[idx2]), 1.0, _relative_orientation[idx1].measures, 
		relative_orientation_sum, relative_orientation_norm, _relative_orientation[idx1].min, _relative_orientation[idx1].max);
      if (analyze_style) {
	//     push_stat(get_relative_pressure(_stroke_paths[idx1], _stroke_paths[idx2]), 1.0, _relative_pressure[idx1].measures, 
	// 	      relative_pressure_sum, relative_pressure_norm, _relative_pressure[idx1].min, _relative_pressure[idx1].max);    
      }
    }
    compute_avg_std(relative_position_sum, relative_position_norm, _relative_position[idx1].measures, _relative_position[idx1].avg, _relative_position[idx1].std);
    compute_avg_std(relative_length_sum, relative_length_norm, _relative_length[idx1].measures, _relative_length[idx1].avg, _relative_length[idx1].std);
    compute_avg_std(relative_width_sum, relative_width_norm, _relative_width[idx1].measures, _relative_width[idx1].avg, _relative_width[idx1].std);
    compute_avg_std(relative_orientation_sum, relative_orientation_norm, _relative_orientation[idx1].measures, _relative_orientation[idx1].avg, _relative_orientation[idx1].std);
    if (analyze_style) {
//       compute_avg_std(relative_pressure_sum, relative_pressure_norm, relative_pressure[idx1].measures, _relative_pressure[idx1].avg, _relative_pressure[idx1].std);
    }

  }


  // 3- compute nearest neighbor statistics 
  // TODO: for "backward compatibility" only - deprecate later
  double proximity_sum = 0.0, parallelism_sum = 0.0, overlapping_sum = 0.0, separation_sum = 0.0;
  double proximity_norm = 0.0, parallelism_norm = 0.0, overlapping_norm = 0.0, separation_norm = 0.0;
  _proximity.min = 1e19; _parallelism.min = 1e19; _overlapping.min = 1e19; _separation.min = 1e19;
  _proximity.max = -1.0; _parallelism.max = -1.0; _overlapping.max = -1.0; _separation.max = -1.0;
  
  unsigned int nb_element_pairs = _element_pairs.size();
  for (unsigned int i=0 ; i<nb_element_pairs ; i++){
    int idx1 = _element_pairs[i].first;
    int idx2 = _element_pairs[i].second;
    push_stat(get_proximity(_stroke_paths[idx1], _stroke_paths[idx2]), 1.0, _proximity.measures, 
	      proximity_sum, proximity_norm, _proximity.min, _proximity.max);
    push_stat(get_parallelism(_stroke_paths[idx1], _stroke_paths[idx2]), 1.0, _parallelism.measures, 
	      parallelism_sum, parallelism_norm, _parallelism.min, _parallelism.max);
    push_stat(get_overlapping(_stroke_paths[idx1], _stroke_paths[idx2]), 1.0, _overlapping.measures, 
	      overlapping_sum, overlapping_norm, _overlapping.min, _overlapping.max);
    push_stat(get_separation(_stroke_paths[idx1], _stroke_paths[idx2]), 1.0, _separation.measures, 
	      separation_sum, separation_norm, _separation.min, _separation.max);
  }
  compute_avg_std(proximity_sum, proximity_norm, _proximity.measures, _proximity.avg, _proximity.std);
  compute_avg_std(parallelism_sum, parallelism_norm, _parallelism.measures, _parallelism.avg, _parallelism.std);
  compute_avg_std(overlapping_sum, overlapping_norm, _overlapping.measures, _overlapping.avg, _overlapping.std);
  compute_avg_std(separation_sum, separation_norm, _separation.measures, _separation.avg, _separation.std);

  // debug
  if (_type==STIPPLING){
    cerr << "Stippling group:" << endl;
  } else if (_type==HATCHING) { 
    cerr << "Hatching group:" << endl;
  } else {
    cerr << "Free group:" << endl;    
  }

  cerr << "*** element statistics ***" << endl;
  cerr << "#elements=" << _elements.size() << endl;
  cerr << "length = (" << _length.avg << "," << _length.std << ") ; [" << _length.min << ".." << _length.max << "]"<< endl;
  cerr << "width = (" << _width.avg << "," << _width.std << ") ; [" << _width.min << ".." << _width.max << "]"<< endl;
  cerr << "orientation = (" << _orientation.avg << "," << _orientation.std << ") ; [" << _orientation.min << ".." << _orientation.max << "]"<< endl;
  if (_reference_frame==AXIS) {
    cerr << "offset = (" << _offset.avg << "," << _offset.std << ") ; [" << _offset.min << ".." << _offset.max << "]"<< endl;
  }
  cerr << endl;

  cerr << "*** element neighbors statistics ***" << endl;
  for (unsigned int i=0 ; i<nb_element_pairs ; i++){
    int nb_neighbors = _element_neighbors[i].size();
    cerr << "element[" << i << "]: #neighbors=" << nb_neighbors << endl;
    cerr << "relative_position = (" << _relative_position[i].avg << "," << _relative_position[i].std 
	 << ") ; [" << _relative_position[i].min << ".." << _relative_position[i].max << "]"<< endl;
    cerr << "relative_length = (" << _relative_length[i].avg << "," << _relative_length[i].std 
	 << ") ; [" << _relative_length[i].min << ".." << _relative_length[i].max << "]"<< endl;
    cerr << "relative_width = (" << _relative_width[i].avg << "," << _relative_width[i].std 
	 << ") ; [" << _relative_width[i].min << ".." << _relative_width[i].max << "]"<< endl;
    cerr << "relative_orientation = (" << _relative_orientation[i].avg << "," << _relative_orientation[i].std 
	 << ") ; [" << _relative_orientation[i].min << ".." << _relative_orientation[i].max << "]"<< endl;
  }
  cerr << endl;

//   cerr << "*** element pairs statistics ***" << endl;
//   cerr << "proximity = (" << _proximity.avg << "," << _proximity.std << ") ; [" << _proximity.min << ".." << _proximity.max << "]"<< endl;
//   cerr << "parallelism = (" << _parallelism.avg << "," << _parallelism.std << ") ; [" << _parallelism.min << ".." << _parallelism.max << "]"<< endl;     
//   cerr << "overlapping = (" << _overlapping.avg << "," << _overlapping.std << ") ; [" << _overlapping.min << ".." << _overlapping.max << "]"<< endl;     
//   cerr << "separation = (" << _separation.avg << "," << _separation.std << ") ; [" << _separation.min << ".." << _separation.max << "]"<< endl;     
//   cerr << endl;

}


void 
StrokeGroup::compute_elements(){
  _elements.clear();
  int path_type; 
  if (_type==HATCHING){
    path_type = StrokePaths::LINE;
  } else if (_type==STIPPLING) {
    path_type = StrokePaths::POINT;
  } else {
    path_type = StrokePaths::BBOX;   
  }
  unsigned int nb_stroke_paths = _stroke_paths.size();
  for (unsigned int i=0 ; i<nb_stroke_paths ; i++){
    if (_stroke_paths[i]->type()==path_type){
      _elements.push_back(i);
    }
  }
}

void 
StrokeGroup::compute_element_neighbors(){
  _element_neighbors.clear();
  _element_pairs.clear();
  if (_reference_frame==AXIS){
    // find the closest path on the right along the main axis
    compute_element_neighbors_1D();
  } else if (_reference_frame==CARTESIAN || _reference_frame==ANGULAR) {
    // compute a Delaunay triangulation without border edges
    compute_element_neighbors_2D();
  }
}
void
StrokeGroup::compute_element_neighbors_1D(){
  unsigned int nb_elements = _elements.size();
  for (unsigned int i=0 ; i<nb_elements ; i++){
    int idx1 = _elements[i];
    int left_neighbor = -1;
    int right_neighbor = -1; 
    double min_left_dist = -1e19;
    double min_right_dist = 1e19;
    CPIXEL& cur_center = _stroke_paths[idx1]->center();
    for (unsigned int j=0 ; j<nb_elements ; j++){
      int idx2 = _elements[j];
      if (idx1!=idx2){
	CPIXEL& cand_center = _stroke_paths[idx2]->center();
	double cand_dist = cand_center[0]-cur_center[0];
	if (cand_dist>0.0 && cand_dist<min_right_dist){
	  right_neighbor = idx2;
	  min_right_dist = cand_dist;
	}
	if (cand_dist<0.0 && cand_dist>min_left_dist){
	  left_neighbor = idx2;
	  min_left_dist = cand_dist;
	}
	
      }
    }

    vector<int> neighbors;
    if (left_neighbor!=-1){
      neighbors.push_back(left_neighbor);      
    }
    if (right_neighbor!=-1){
      neighbors.push_back(right_neighbor);      
    }
    _element_neighbors.push_back(neighbors);

    // TODO: for "backward compatibility" only - deprecate later
    if (left_neighbor!=-1 && right_neighbor!=-1){
      if (min_right_dist<-min_left_dist){
	_element_pairs.push_back( pair<int,int>(idx1, right_neighbor) );
      } else {
	_element_pairs.push_back( pair<int,int>(idx1, left_neighbor) );	
      }

    } else if (left_neighbor!=-1) {
      _element_pairs.push_back( pair<int,int>(idx1, left_neighbor) );

    } else if (right_neighbor!=-1) {
      _element_pairs.push_back( pair<int,int>(idx1, right_neighbor) );      
    }
  }
}


void
StrokeGroup::compute_element_neighbors_2D(){
  triangulateio in, out;
  int nb_elements = _elements.size();
  if (nb_elements<3) return;

  // initialize input data
  in.numberofpoints = nb_elements;
  in.pointlist = (REAL*) malloc(in.numberofpoints*2*sizeof(REAL));
  for (int i=0 ; i<nb_elements ; i++){
    int idx = _elements[i];
    in.pointlist[2*i] = _stroke_paths[idx]->center()[0];
    in.pointlist[2*i+1] = _stroke_paths[idx]->center()[1];
  }
  in.pointmarkerlist = 0;
  in.numberofpointattributes = 0;
  in.pointattributelist = 0;
  in.numberofsegments = 0;
  in.numberofholes = 0;
  in.numberofregions = 0;
  in.regionlist = 0;

  // initialize output data
  out.pointlist = (REAL*) NULL;
  out.pointattributelist = (REAL*) NULL;
  out.pointmarkerlist = (int*) NULL;
  out.trianglelist = (int*) NULL;
  out.triangleattributelist = (REAL*) NULL;
  out.neighborlist = (int*) NULL;
  out.segmentlist = (int*) NULL;
  out.segmentmarkerlist = (int*) NULL;
  out.edgelist = (int*) NULL;
  out.edgemarkerlist = (int*) NULL;
  
  // compute triangulation and write result
  triangulate("zeQ", &in, &out, 0);
  vector<double> distances;
  for (int i=0 ; i<nb_elements ; i++){
    distances.push_back(1e19);
  }
  _element_pairs = vector< pair<int, int> >(nb_elements);
  _element_neighbors = vector< vector<int> >(nb_elements);
  int nb_pairs = out.numberofedges;
  for (int i=0 ; i<nb_pairs ; i++){
    if (!out.edgemarkerlist[i]){
      int idx1 = _elements[out.edgelist[2*i]];
      int idx2 = _elements[out.edgelist[2*i+1]];
      _element_neighbors[idx1].push_back(idx2);
      _element_neighbors[idx2].push_back(idx1);

      // TODO: for "backward compatibility" only - deprecate later
      double d = _stroke_paths[idx1]->center().dist(_stroke_paths[idx2]->center());
      if (d<distances[idx1]){
	_element_pairs[idx1] = pair<int, int>(idx1, idx2);
	distances[idx1] = d;
      }
      if (d<distances[idx2]){
	_element_pairs[idx2] = pair<int, int>(idx1, idx2);
	distances[idx2] = d;
      }
    }
  }

  // and finally free memory
  if (in.pointlist) free(in.pointlist);
  if (in.pointmarkerlist) free(in.pointmarkerlist);
  if (in.pointattributelist) free(in.pointattributelist);
  if (in.regionlist) free(in.regionlist);
  if (out.pointlist) free(out.pointlist);
  if (out.pointattributelist) free(out.pointattributelist);
  if (out.pointmarkerlist) free(out.pointmarkerlist);
  if (out.trianglelist) free(out.trianglelist);
  if (out.triangleattributelist) free(out.triangleattributelist);
  if (out.neighborlist) free(out.neighborlist);
  if (out.segmentlist) free(out.segmentlist);
  if (out.segmentmarkerlist) free(out.segmentmarkerlist);
  if (out.edgelist) free(out.edgelist);
  if (out.edgemarkerlist) free(out.edgemarkerlist);
}


double 
StrokeGroup::get_angle(const StrokePaths* path) const{
  VEXEL axis_a = path->axis_a();
  VEXEL main_dir = (axis_a*VEXEL(0.0, 1.0)>=0) ? axis_a : -axis_a;
  return  main_dir.angle(VEXEL(1.0, 0.0));
}

double 
StrokeGroup::get_orientation(const StrokePaths* path) const{
  // orientation in [0..1]
  if(_reference_frame==ANGULAR) {
    PIXEL origin (VIEW::peek()->width()*0.5, VIEW::peek()->height()*0.5);
    CVEXEL& origin_to_center = (path->center()-origin).normalized();
    CVEXEL& axis = path->axis_a().normalized();
    return abs(axis*origin_to_center);
  } else {
    return abs(path->axis_a().normalized()*VEXEL(1.0, 0.0));
  }
}

double 
StrokeGroup::get_offset(const StrokePaths* path) const{
  if (_reference_frame==AXIS) {
    double offset = path->center()[1] - VIEW::peek()->height()*0.5;
    return offset;
  } else {
    return 0.0;
  }
}


double
StrokeGroup::get_relative_position(const StrokePaths* sp1, const StrokePaths* sp2) const{
  if (_reference_frame==AXIS){
    return abs(sp1->center()[0]-sp2->center()[0]);
  } else {
    return sp1->center().dist(sp2->center());
  }
}

double  
StrokeGroup::get_relative_length(const StrokePaths* sp1, const StrokePaths* sp2) const{
  return sp1->axis_a().length()/sp2->axis_a().length();
}

double  
StrokeGroup::get_relative_width(const StrokePaths* sp1, const StrokePaths* sp2) const{
  return sp1->axis_b().length()/sp2->axis_b().length();
}

double  
StrokeGroup::get_relative_orientation(const StrokePaths* sp1, const StrokePaths* sp2) const{
  double angle = sp1->axis_a().angle(sp2->axis_a());
  return (angle>M_PI*0.5)? (M_PI-angle)*2/M_PI: angle*2/M_PI;
}





void 
StrokeGroup::push_stat(double measure, double confidence, vector<double>& measures, 
		       double& sum, double& norm, double& min, double& max){
  double weighted_measure = measure*confidence;
  measures.push_back(weighted_measure);
  sum += weighted_measure;
  norm += confidence;
  if (weighted_measure<min) min = weighted_measure;
  if (weighted_measure>max) max = weighted_measure;
}


void 
StrokeGroup::compute_avg_std(double sum, double norm, const vector<double>& measures,
			     double& avg, double& std) const{

  // XXX: if using robust estimation, use the average as a first estimate, 
  // then use an influence function to detect outliers and refine the estimate. 
  // Repeat if necessary.
  avg = sum/norm;
  std = 0.0;
  unsigned int nb_measures = measures.size();
  for (unsigned int i=0 ; i<nb_measures ; i++){
    std += (measures[i] - avg)*(measures[i] - avg);
  }
  std = sqrt(std/nb_measures);

}




/***********************************************
 ** TODO: the following measures (deprecated) **
 **********************************************/

double
StrokeGroup::get_proximity(const StrokePaths* sp1, const StrokePaths* sp2) const{
  if (_reference_frame==AXIS){
    return abs(sp1->center()[0]-sp2->center()[0]);
  } else {
    return sp1->center().dist(sp2->center());
  }
}


double  
StrokeGroup::get_parallelism(const StrokePaths* sp1, const StrokePaths* sp2) const{
  double angle1 = sp1->axis_a().angle(VEXEL(1.0, 0.0));
  if (angle1> M_PI/2){
    angle1 = M_PI-angle1;
  }
  double angle2 = sp2->axis_a().angle(VEXEL(1.0, 0.0));
  if (angle2> M_PI/2){
    angle2 = M_PI-angle2;
  }
  return (angle1>angle2) ? 2*(angle1-angle2)/M_PI : 2*(angle2-angle1)/M_PI;
}


double  
StrokeGroup::get_overlapping(const StrokePaths* sp1, const StrokePaths* sp2) const{
  // first compute the virtual line of sp1 and sp2 
  double l1 = sp1->axis_a().length();
  double l2 = sp2->axis_a().length();
  CPIXEL& c1 = sp1->center();
  CPIXEL& c2 = sp2->center();
  PIXEL virtual_center = (l1*c1+l2*c2)/(l1+l2);
  VEXEL virtual_axis = (sp1->axis_a()+sp2->axis_a()).normalized();
  PIXELline virtual_line (virtual_center, virtual_axis);

  // then compute the projected centers vector and the projected line lengths
  VEXEL center_vec = virtual_line.project(c1)-virtual_line.project(c2);
  if (center_vec*VEXEL(1.0, 0.0)<0){
    center_vec = -center_vec;
  }
//   double center_vec_sign = (center_vec*VEXEL(0.0, 1.0)>=0.0) ? 1.0 : -1.0;
  double center_vec_sign = 1.0;
  PIXEL proj_start1 = virtual_line.project(c1-sp1->axis_a()*0.5);
  PIXEL proj_end1 = virtual_line.project(c1+sp1->axis_a()*0.5);
  PIXEL proj_start2 = virtual_line.project(c2-sp2->axis_a()*0.5);
  PIXEL proj_end2 = virtual_line.project(c2+sp2->axis_a()*0.5);
  double l_proj_1 = (proj_end1-proj_start1).length();
  double l_proj_2 = (proj_end2-proj_start2).length();

  // and finally compute overlapping
  return 2*center_vec.length()*center_vec_sign / (l_proj_1+l_proj_2);
}

double  
StrokeGroup::get_separation(const StrokePaths* sp1, const StrokePaths* sp2) const{
  // first compute the perpendicular virtual line of sp1 and sp2 
  double l1 = sp1->axis_a().length();
  double l2 = sp2->axis_a().length();
  CPIXEL& c1 = sp1->center();
  CPIXEL& c2 = sp2->center();
  PIXEL perp_virtual_center = (l1*c1+l2*c2)/(l1+l2);
  VEXEL perp_virtual_axis = (sp1->axis_a()+sp2->axis_a()).perpend().normalized();
  PIXELline perp_virtual_line (perp_virtual_center, perp_virtual_axis);

  // then compute the projected centers vector and the projected line lengths
  VEXEL center_vec = perp_virtual_line.project(c1)-perp_virtual_line.project(c2);
  return center_vec.length();
}



/////////////////
// Synthesis
/////////////////

void 
StrokeGroup::copy(GestureCell* target_cell, CUVpt& offset, bool stretch) const {
  unsigned int nb_stroke_paths = _stroke_paths.size();
  for(unsigned int i=0; i<nb_stroke_paths ; i++){
    _stroke_paths[i]->copy(target_cell, offset, stretch);
  }  
}

void 
StrokeGroup::synthesize_1ring(GestureCell* target_cell) {
  cerr << "synthesize_1ring" << endl;
  // 1- Synthesize elements: position, shape index, extent, orientation 
  // and optionally attributes
  element_synthesis(target_cell);

  // 2- Synthesize group: correct position, extent, orientation and 
  // optionally attributes based on 1-ring neighborhood comparisons
  group_synthesis(target_cell);

  // 3- Render strokes
  render_synthesized_strokes(target_cell);

}

void 
StrokeGroup::synthesize_Efros(GestureCell* target_cell){
  cerr << 0 << endl;
  // 1- Initialize distribution 
  _target_positions.clear();
  if (_reference_frame==AXIS){
    if (_distribution==LLOYD) {
      lloyd_1D(target_cell);
    } else {
      stratified_1D(target_cell);
    }
  } else {
    if (_distribution==LLOYD) {
      lloyd_2D(target_cell);
    } else {
      stratified_2D(target_cell);
    }
  }
  cerr << 1 << endl;

  // 2- Extract connectivity and synthesis order
  vector< vector<int> > synthesized_neighborhoods;
  vector<int> ordered_elements;
  if (_reference_frame==AXIS){
    compute_synthesized_neighborhoods_1D(synthesized_neighborhoods);
    compute_element_order_1D(synthesized_neighborhoods, ordered_elements);
  } else {
    compute_synthesized_neighborhoods_2D(synthesized_neighborhoods);    
    compute_element_order_2D(synthesized_neighborhoods, ordered_elements);
  }
  cerr << 2 << endl;
  
  // debug
  cerr << "ordered elements: (" << ordered_elements.size() << " / " << _target_positions.size() << " ): ";
  for (unsigned int i=0 ; i<ordered_elements.size() ; i++){
    cerr << ordered_elements[i] << " ";
  }
  cerr << endl;

//   // debug: test neighborhood matching
//   unsigned int nb_synthesized_elements = synthesized_neighborhoods.size();
//   for (unsigned int i=0 ; i<nb_synthesized_elements ; i++){
//     unsigned int nb_neighbors = synthesized_neighborhoods[i].size();
//     for (unsigned int j=0 ; j<nb_neighbors ; j++){
//       int idx = synthesized_neighborhoods[i][j];
//       WORLD::show(NDCZpt(2*_target_positions[i][0]-1.0, 2*_target_positions[i][1]-1.0, 0.0), 
// 		  NDCZpt(2*_target_positions[idx][0]-1.0, 2*_target_positions[idx][1]-1.0, 0.0),
// 		  1.0, COLOR::black, 1.0, true);
//     }
//   }
//   double r_val = (double)rand()/(double)RAND_MAX;
//   int r_idx = (int)(r_val * nb_synthesized_elements);
//   int match_idx = find_best_match(r_idx, synthesized_neighborhoods);
//   for (unsigned int i=0 ; i<nb_synthesized_elements ; i++){
//     if ((int)i==r_idx){
//       WORLD::show(NDCZpt(2*_target_positions[i][0]-1.0, 2*_target_positions[i][1]-1.0, 0.0), 4.0, COLOR::blue, 1.0, true);
//     } else if ((int)i==match_idx){
//       WORLD::show(NDCZpt(2*_target_positions[i][0]-1.0, 2*_target_positions[i][1]-1.0, 0.0), 4.0, COLOR::red, 1.0, true);
//     } else {
//       WORLD::show(NDCZpt(2*_target_positions[i][0]-1.0, 2*_target_positions[i][1]-1.0, 0.0), 4.0, COLOR::black, 1.0, true);      
//     }
//   }


  // 3- First clear old data and initialize center seed and element parameters
  unsigned int nb_seeds = _target_positions.size();
  _target_indices = vector<int> (nb_seeds);
  _corrected_indices = vector<int> (nb_seeds);
  for (unsigned int i=0 ; i<nb_seeds ; i++){
    _target_indices[i] = -1;
    _corrected_indices[i] = -1;
  }
  _target_angles = vector<double> (nb_seeds);
  _target_scales = vector<UVvec> (nb_seeds);
  _corrected_positions = vector<UVpt> (nb_seeds);
  _corrected_angles = vector<double> (nb_seeds);
  _corrected_scales = vector<UVvec> (nb_seeds);
  if (nb_seeds==0){
    return;
  }
  int ref_center_idx = get_ref_center();
  vector<double> sorted_lengths, sorted_widths, sorted_angles;
  get_sorted_parameters(sorted_lengths, sorted_widths, sorted_angles);
  synthesize_element(ref_center_idx, ordered_elements[0], synthesized_neighborhoods, target_cell->scale(),
		     sorted_lengths, sorted_widths, sorted_angles);
  cerr << 3 << endl;
  
  // 4- For each seed, synthesize
  for (unsigned int i=1 ; i<nb_seeds ; i++){
    // 1- Find best ref element
    int ref_idx;
    if (_reference_frame==AXIS){
      ref_idx = find_best_match(ordered_elements[i], synthesized_neighborhoods, target_cell->scale(),
				PROX_THRESHOLD_1D, PAR_THRESHOLD_1D, OV_THRESHOLD_1D);
    } else {
      ref_idx = find_best_match(ordered_elements[i], synthesized_neighborhoods, target_cell->scale(),
				PROX_THRESHOLD_2D, PAR_THRESHOLD_2D, OV_THRESHOLD_2D);
    }

    // 2- Synthesize target element from best ref
    synthesize_element(ref_idx, ordered_elements[i], synthesized_neighborhoods, target_cell->scale(),
		       sorted_lengths, sorted_widths, sorted_angles);
  }
  cerr << 4 << endl;

  // 5- Render strokes
  render_synthesized_strokes(target_cell);
  cerr << 5 << endl;

}

int
StrokeGroup::get_ref_center(){
  unsigned int nb_elements = _elements.size();
  double min_d = 1e19;
  int center_idx = -1;
  PIXEL box_center = _bbox.center();
  for (unsigned int i=0 ; i<nb_elements ; i++){
    double d = _stroke_paths[_elements[i]]->center().dist(box_center);
    if (d<min_d){
      min_d = d;
      center_idx = _elements[i];
    }
  }
  return center_idx;
}


void
StrokeGroup::get_sorted_parameters(vector<double>& sorted_lengths, vector<double>& sorted_widths, 
				   vector<double>& sorted_angles){
  sorted_lengths = _length.measures;
  sort(sorted_lengths.begin(), sorted_lengths.end());
  sorted_widths = _width.measures;
  sort(sorted_widths.begin(), sorted_widths.end());
  sorted_angles = _orientation.measures;
  sort(sorted_angles.begin(), sorted_angles.end());
}

void
StrokeGroup::synthesize_element(int ref_idx, int target_idx, const vector< vector<int> >& target_neighbors, double target_scale,
				const vector<double>& sorted_lengths, const vector<double>& sorted_widths, const vector<double>& sorted_angles){
  // 1- copy parameters from ref to target
  // the position is unchanged
  // the shape idx is the one of the best matching element
  _target_indices[target_idx] = ref_idx;
  // the scale is the one of the best matching element
  _target_scales[target_idx] = UVvec(1.0, 1.0);
  // the angle is the one of the best matching element
  double ref_angle =  get_angle(_stroke_paths[ref_idx]);
  _target_angles[target_idx] = ref_angle;


  // 2- vary ref parameters
  // the shape index is the one of the best matching element
  _corrected_indices[target_idx] = ref_idx;
  // vary position relative to neighbors.
  unsigned int nb_ref_neighbors = _element_neighbors[ref_idx].size();
  unsigned int nb_target_neighbors = target_neighbors[target_idx].size();
  if (nb_ref_neighbors>=2 && nb_target_neighbors>=2){
    double ref_x_center = 0.0;
    double ref_y_center = 0.0;
    for (unsigned int i=0 ; i<nb_ref_neighbors ; i++){
      int n_idx = _element_neighbors[ref_idx][i];
      ref_x_center += _stroke_paths[_elements[n_idx]]->center()[0];
      ref_y_center += _stroke_paths[_elements[n_idx]]->center()[1];    
    }
    ref_x_center /= (double)nb_ref_neighbors;
    ref_y_center /= (double)nb_ref_neighbors;
    
    double target_x_center = 0.0;
    double target_y_center = 0.0;
    for (unsigned int i=0; i<nb_target_neighbors; i++){
      int n_idx = target_neighbors[target_idx][i];
      target_x_center += _target_positions[n_idx][0];
      target_y_center += _target_positions[n_idx][1];
    }
    target_x_center /= (double)nb_target_neighbors;
    target_y_center /= (double)nb_target_neighbors;
    
    PIXEL ref_pos = _stroke_paths[_elements[ref_idx]]->center();
    UVvec pos_shift ((ref_pos[0]-ref_x_center)/target_scale, (ref_pos[1]-ref_y_center)/target_scale);
    UVpt target_center (target_x_center, target_y_center);
    _corrected_positions[target_idx] = target_center + pos_shift;
  } else {
    _corrected_positions[target_idx] = _target_positions[target_idx];    
  }
  
  // vary scale
  double ref_length = _stroke_paths[ref_idx]->axis_a().length();
  double ref_width = _stroke_paths[ref_idx]->axis_b().length();
  double close_length, close_width;
  get_close_value(ref_length, close_length, sorted_lengths, false);
  get_close_value(ref_width, close_width, sorted_widths, false);
  double length_ratio = close_length/ref_length;
  double width_ratio = close_width/ref_width;
  if (close_length<2.0 || ref_length<2.0){
    length_ratio = 1.0;
  }
  UVvec scale(length_ratio, width_ratio);
  _corrected_scales[target_idx] = scale;
  // vary orientation
  double close_angle;
  get_close_value(ref_angle, close_angle, sorted_angles, true);
  if (abs(ref_angle-close_angle) > abs(ref_angle-(close_angle+M_PI))){
    close_angle += M_PI;
  } else if (abs(ref_angle-close_angle) > abs(ref_angle-(close_angle-M_PI))){
    close_angle -= M_PI;      
  }
  _corrected_angles[target_idx] = close_angle;

}

int 
StrokeGroup::find_best_match(int target_idx, const vector< vector<int> >& target_neighborhoods, double target_scale,
			     double proximity_threshold, double parallelism_threshold, double overlapping_threshold){
  
//   unsigned int nb_max_relevant_pairs = 0;
//   vector< pair<int, double> > match_candidates;
//   pair<int, double> best_match (-1, 1e19);

  vector<double> proximity_measures, parallelism_measures, overlapping_measures, 
    superimposition_measures, color_diff_measures;
  
  double min_proximity = 1e19;
  int min_prox_idx = -1;

  UVvec target_origin_vec = _target_positions[target_idx]-UVpt(0.0, 0.0);
  double max_dist;
  vector<int> neighbors1;
  get_n_ring_neighbors(target_idx, target_neighborhoods, neighbors1, max_dist);
  unsigned int nb_neighbors1 = neighbors1.size();

  
  unsigned int nb_ref_elements = _elements.size();
  double mean_length = 0.0;
  for (unsigned int i=0; i<nb_ref_elements ; i++){
    mean_length += _stroke_paths[_elements[i]]->axis_a().length();
  }
  mean_length /= nb_ref_elements;

  for (unsigned int i=0; i<nb_ref_elements ; i++){

    VEXEL ref_origin_vec = _stroke_paths[_elements[i]]->center()-PIXEL(0.0, 0.0);
    double dummy_max_dist;
    vector<int> neighbors2;
    get_n_ring_neighbors(i, _element_neighbors, neighbors2, dummy_max_dist);
    unsigned int nb_neighbors2 = neighbors2.size();

    // 1- extract src and dest neighborhoods
    unsigned int nb_src_neighbors, nb_dest_neighbors;
    vector<int> src_neighborhood, dest_neighborhood;
    bool reversed;
    if (nb_neighbors1<nb_neighbors2){
      nb_src_neighbors = nb_neighbors1;
      src_neighborhood = neighbors1;
      nb_dest_neighbors = nb_neighbors2;
      dest_neighborhood = neighbors2;
      reversed = false;
    } else {	
      nb_src_neighbors = nb_neighbors2;
      src_neighborhood = neighbors2;
      nb_dest_neighbors = nb_neighbors1;
      dest_neighborhood = neighbors1;
      reversed = true;
    }
    
    // 2- compute matching from src to dest
    vector< vector< pair<int, double> > > dest_candidates(nb_dest_neighbors);
    for (unsigned int j=0; j<nb_src_neighbors ; j++){
      int src_idx = src_neighborhood[j];
      double min_dest_dist = 1e19;
      int min_dest_idx = -1;
      for (unsigned int k=0; k<nb_dest_neighbors ; k++){
	int dest_idx = dest_neighborhood[k];
	PIXEL src_pos, dest_pos;
	if (reversed){
	  // src is ref
	  src_pos = _stroke_paths[_elements[src_idx]]->center() - ref_origin_vec;
	  dest_pos = PIXEL ((_target_positions[dest_idx][0]-target_origin_vec[0])*target_scale, 
			    (_target_positions[dest_idx][1]-target_origin_vec[1])*target_scale);
	} else {
	  // src is target
	  src_pos = _stroke_paths[_elements[dest_idx]]->center() - ref_origin_vec;
	  dest_pos = PIXEL ((_target_positions[src_idx][0]-target_origin_vec[0])*target_scale, 
			    (_target_positions[src_idx][1]-target_origin_vec[1])*target_scale);
	}
	double cur_dist = src_pos.dist(dest_pos);
	if (cur_dist<min_dest_dist){
	  min_dest_dist = cur_dist;
	  min_dest_idx = k;
	}
      }
      if (min_dest_idx!=-1){
	dest_candidates[min_dest_idx].push_back(pair<int, double>(j, min_dest_dist));
      }
    }
    
    // 3- Keep best matches from dest to src
    vector< pair<int, int> > relevant_pairs;
    for (unsigned int j=0 ; j<nb_dest_neighbors ; j++){
      double min_src_dist = 1e19;
      int min_src_idx = -1;
      unsigned int nb_dest_candidates = dest_candidates[j].size();
      for (unsigned int k=0 ; k<nb_dest_candidates ; k++){
	double cur_dist = dest_candidates[j][k].second;
	if (cur_dist<min_src_dist){
	  min_src_dist = cur_dist;
	  min_src_idx = dest_candidates[j][k].first;
	}
      }
      if (min_src_idx!=-1){
	if (reversed){
	  // src is ref
	  relevant_pairs.push_back(pair<int, int>(src_neighborhood[min_src_idx], dest_neighborhood[j]));
	} else {
	  // src is target
	  relevant_pairs.push_back(pair<int, int>(dest_neighborhood[j], src_neighborhood[min_src_idx]));
	}
	// TODO: relevant_weight ?
      }
    }
    
    // 4- Compute neighborhood match 
    unsigned int nb_relevant_pairs = relevant_pairs.size();
    double neighborhood_proximity = 0.0;
    double neighborhood_parallelism = 0.0;
    double neighborhood_overlapping = 0.0;
    double neighborhood_superimposition = 0.0;
    double neighborhood_color_diff = 0.0;
    int nb_relevant_neighbors = 0;
    for (unsigned int j=0 ; j<nb_relevant_pairs ; j++){
      int ref_idx = relevant_pairs[j].first;
      int target_idx = relevant_pairs[j].second;
      int synth_idx = _target_indices[target_idx];
      if (synth_idx != -1){
	neighborhood_proximity += compute_element_proximity(ref_idx, synth_idx);
	neighborhood_parallelism += compute_element_parallelism(ref_idx, synth_idx);
	neighborhood_overlapping += compute_element_overlapping(ref_idx, synth_idx);
	neighborhood_superimposition += compute_element_superimposition(ref_idx, synth_idx);
	neighborhood_color_diff += compute_element_color_diff(ref_idx, synth_idx);
	nb_relevant_neighbors++;
      }
    }
    if (nb_relevant_neighbors>0){
      neighborhood_proximity /= nb_relevant_neighbors;
      neighborhood_parallelism /= nb_relevant_neighbors;
      neighborhood_overlapping /= nb_relevant_neighbors;
      neighborhood_color_diff /= nb_relevant_neighbors;
    } else {
      neighborhood_proximity = 1e19;
      neighborhood_parallelism = 1e19;
      neighborhood_overlapping = 1e19;
      neighborhood_color_diff = 1e19;
    }
    
    proximity_measures.push_back(neighborhood_proximity);
    parallelism_measures.push_back(neighborhood_parallelism);
    overlapping_measures.push_back(neighborhood_overlapping);
    superimposition_measures.push_back(neighborhood_superimposition);
    color_diff_measures.push_back(neighborhood_color_diff);

    if (neighborhood_proximity<min_proximity){
      min_proximity = neighborhood_proximity;
      min_prox_idx = i;
    }
  }
  
  // 6- Pick one candidate with good match 
  vector<int> nearest_candidates;
  cerr << "thresholds: " << proximity_threshold*mean_length << " / " 
       << parallelism_threshold << " / " << overlapping_threshold << endl;
  for (unsigned int i=0 ; i<nb_ref_elements ; i++){
    cerr << "match candidate #" << i << ": (" << proximity_measures[i] << " / " 
	 << parallelism_measures[i] << " / " <<  overlapping_measures[i] << " / "
	 << color_diff_measures[i] << ")";

    if ( color_diff_measures[i] > COLOR_THRESHOLD){
      cerr << " => reject (color)" << endl;
      continue;
    }

    if ( parallelism_measures[i] <= parallelism_threshold &&
	 overlapping_measures[i] <= overlapping_threshold && 
	 superimposition_measures[i] <= proximity_threshold){
      cerr << " => KEEP (par/ov)" << endl;
      nearest_candidates.push_back(i);
    } else if (proximity_measures[i] <= proximity_threshold*mean_length){
      cerr << " => KEEP (prox)" << endl;
      nearest_candidates.push_back(i);
    } else {
      cerr << " => reject" << endl;
    }
  }
  cerr << endl;


  if (nearest_candidates.empty()){
    nearest_candidates.push_back(min_prox_idx);
  }
  unsigned int nb_nearest_candidates = nearest_candidates.size();
  double r_val = (double)rand()/(double)RAND_MAX;
  int r_idx = (int)(r_val*nb_nearest_candidates);
  return nearest_candidates[r_idx];
}


double 
StrokeGroup::compute_element_proximity(int ref_idx, int target_idx){
  PIXEL aligned_pos (0.0, 0.0);
  VEXEL target_axis_a = _stroke_paths[_elements[target_idx]]->axis_a();
  VEXEL target_axis_b = _stroke_paths[_elements[target_idx]]->axis_b();
  VEXEL ref_axis_a = _stroke_paths[_elements[ref_idx]]->axis_a();
  VEXEL ref_axis_b = _stroke_paths[_elements[ref_idx]]->axis_b();
  double dist1 = StrokePaths::hausdorff_distance(aligned_pos, ref_axis_a, ref_axis_b, 
						 aligned_pos, target_axis_a, target_axis_b);
  double dist2 = StrokePaths::hausdorff_distance(aligned_pos, target_axis_a, target_axis_b,
						 aligned_pos, ref_axis_a, ref_axis_b);
  return max(dist1, dist2);    
}

double 
StrokeGroup::compute_element_parallelism(int ref_idx, int target_idx){
  VEXEL target_axis_a = _stroke_paths[_elements[target_idx]]->axis_a();
  VEXEL ref_axis_a = _stroke_paths[_elements[ref_idx]]->axis_a();
  return target_axis_a.angle(ref_axis_a);
}

double 
StrokeGroup::compute_element_overlapping(int ref_idx, int target_idx){
  VEXEL target_axis_a = _stroke_paths[_elements[target_idx]]->axis_a();
  VEXEL ref_axis_a = _stroke_paths[_elements[ref_idx]]->axis_a();
  double ref_l = ref_axis_a.length();
  double tar_l = target_axis_a.length();
  if (ref_l>tar_l){
    return ref_l/tar_l;
  } else {
    return tar_l/ref_l;
  }
}

double 
StrokeGroup::compute_element_superimposition(int ref_idx, int target_idx){
  VEXEL target_axis_b = _stroke_paths[_elements[target_idx]]->axis_a();
  VEXEL ref_axis_b = _stroke_paths[_elements[ref_idx]]->axis_a();
  double ref_w = ref_axis_b.length();
  double tar_w = target_axis_b.length();
  if (ref_w>tar_w){
    return 0.5*(ref_w-tar_w);
  } else {
    return 0.5*(tar_w-ref_w);
  }
}

double 
StrokeGroup::compute_element_color_diff(int ref_idx, int target_idx){
  return 0.0;
}


// int 
// StrokeGroup::find_best_match(int idx, const vector< vector<int> >& neighborhoods){
  
//   double best_match = 1e19;
//   int best_match_idx = -1;
//   unsigned int nb_best_relevant_pairs = 0;
//   vector< pair<int, int> > best_relevant_pairs;
//   vector<double> best_relevant_matches;
//   unsigned int nb_synth_elements = neighborhoods.size();
//   double max_dist1;
//   vector<int> neighbors1;
//   get_n_ring_neighbors(idx, neighborhoods, neighbors1, max_dist1);
//   unsigned int nb_neighbors1 = neighbors1.size();
//   UVvec vec_from_origin1 = _target_positions[idx]-UVpt(0.0, 0.0);
//   for (unsigned int i=0; i<nb_synth_elements ; i++){
//     if ((int)i==idx){
//       continue;
//     }

//     // 1- extract src and dest neighborhoods
//     double max_dist2;
//     vector<int> neighbors2;
//     get_n_ring_neighbors(i, neighborhoods, neighbors2, max_dist2);
//     unsigned int nb_neighbors2 = neighbors2.size();
//     UVvec vec_from_origin2 = _target_positions[i]-UVpt(0.0, 0.0);
//     unsigned int nb_src_neighbors, nb_dest_neighbors;
//     vector<int> src_neighborhood, dest_neighborhood;
//     UVvec src_vec_from_origin, dest_vec_from_origin;
//     bool reversed;
//     if (nb_neighbors1<nb_neighbors2){
//       nb_src_neighbors = nb_neighbors1;
//       src_neighborhood = neighbors1;
//       src_vec_from_origin = vec_from_origin1;
//       nb_dest_neighbors = nb_neighbors2;
//       dest_neighborhood = neighbors2;
//       dest_vec_from_origin = vec_from_origin2;
//       reversed = false;
//     } else {	
//       nb_src_neighbors = nb_neighbors2;
//       src_neighborhood = neighbors2;
//       src_vec_from_origin = vec_from_origin2;
//       nb_dest_neighbors = nb_neighbors1;
//       dest_neighborhood = neighbors1;
//       dest_vec_from_origin = vec_from_origin1;
//       reversed = true;
//     }
    
//     // 2- compute matching from src to dest
//     vector< vector< pair<int, double> > > dest_candidates(nb_dest_neighbors);
//     for (unsigned int j=0; j<nb_src_neighbors ; j++){
//       UVpt src_pos = _target_positions[src_neighborhood[j]]-src_vec_from_origin;
//       double best_dest_match = 1e19;
//       int best_dest_match_idx = -1;
//       for (unsigned int k=0; k<nb_dest_neighbors ; k++){
// 	UVpt dest_pos = _target_positions[dest_neighborhood[k]]-dest_vec_from_origin;
// 	double m = src_pos.dist(dest_pos);
// 	if (m<best_dest_match){
// 	  best_dest_match = m;
// 	  best_dest_match_idx = k;
// 	}
//       }
//       dest_candidates[best_dest_match_idx].push_back(pair<int, double>(j, best_dest_match));
//     }
    
//     // 3- Keep best matches from dest to src
//     vector< pair<int, int> > relevant_pairs;
//     vector<double> relevant_matches;
//     for (unsigned int j=0 ; j<nb_dest_neighbors ; j++){
//       double best_src_match = 1e19;
//       int best_src_match_idx = -1;
//       unsigned int nb_dest_candidates = dest_candidates[j].size();
//       for (unsigned int k=0 ; k<nb_dest_candidates ; k++){
// 	double cur_match = dest_candidates[j][k].second;
// 	if (cur_match<best_src_match){
// 	  best_src_match = cur_match;
// 	  best_src_match_idx = dest_candidates[j][k].first;
// 	}
//       }
//       if (best_src_match_idx!=-1){
// 	relevant_pairs.push_back(pair<int, int>(src_neighborhood[best_src_match_idx], dest_neighborhood[j]));
// 	relevant_matches.push_back(best_src_match);
//       }
//     }
    
//     // 4- Compute neighborhood match and update best match
//     double neighborhood_match;
//     unsigned int nb_relevant_pairs = relevant_pairs.size();
//     if (nb_relevant_pairs<2){
//       neighborhood_match = 1e19;
//     } else {
//       neighborhood_match = 0.0;
//       double sigma = (max_dist1>max_dist2) ? max_dist1*0.5 : max_dist2*0.5;
// //       cerr << "sigma=" << sigma << endl;
//       for (unsigned int j=0 ; j<nb_relevant_pairs ; j++){
// 	//	double weight = normal_distrib(d/sigma);
// 	double weight = 1.0;
// 	neighborhood_match += relevant_matches[j] * weight;
//       }      
//     }
    
//     if (nb_relevant_pairs>nb_best_relevant_pairs ||
// 	(nb_relevant_pairs==nb_best_relevant_pairs && neighborhood_match<best_match)){
//       best_match = neighborhood_match;
//       best_match_idx = i;
//       nb_best_relevant_pairs = nb_relevant_pairs;
//       best_relevant_pairs.clear();
//       for (unsigned int j=0 ; j<nb_relevant_pairs ; j++){
// 	if (reversed){
// 	  best_relevant_pairs.push_back(pair<int, int>(relevant_pairs[j].second, relevant_pairs[j].first));
// 	} else {
// 	  best_relevant_pairs.push_back(pair<int, int>(relevant_pairs[j].first, relevant_pairs[j].second));	    
// 	}
// 	best_relevant_matches.push_back(relevant_matches[j]);
//       }
//     }
    
//   }

//   // debug
//   if (best_match_idx==-1){
//     cerr << "No match found !!!" << endl;
//   } else {
//     unsigned int nb_best_relevant_pairs = best_relevant_pairs.size();
//     cerr << "nb_relevant_pairs=" << best_relevant_pairs.size() << endl;
//     for (unsigned int i=0 ; i<nb_best_relevant_pairs ; i++){
//       // draw relevant pairs with same color
//       double val = (double)i/(double)nb_best_relevant_pairs;
//       COLOR c(val, 0.0, 1.0-val);
//       WORLD::show(NDCZpt(2*_target_positions[idx][0]-1.0, 2*_target_positions[idx][1]-1.0, 0.0), 
// 		  NDCZpt(2*_target_positions[best_relevant_pairs[i].first][0]-1.0, 2*_target_positions[best_relevant_pairs[i].first][1]-1.0, 0.0), 
// 		  2.0, c, 1.0, true);
//       WORLD::show(NDCZpt(2*_target_positions[best_match_idx][0]-1.0, 2*_target_positions[best_match_idx][1]-1.0, 0.0), 
// 		  NDCZpt(2*_target_positions[best_relevant_pairs[i].second][0]-1.0, 2*_target_positions[best_relevant_pairs[i].second][1]-1.0, 0.0), 
// 		  2.0, c, 1.0, true);
//     }
//   }

//   return best_match_idx;
// }


void
StrokeGroup::get_n_ring_neighbors(int idx, const vector< vector<int> >& neighborhoods, 
				  vector<int>& neighbors, double& max_dist){
  
//   max_dist = 0.0;
//   UVpt center_pos = _target_positions[idx];
//   unsigned int nb_neighbors = neighborhoods[idx].size();
//   for (unsigned int i=0 ; i<nb_neighbors ; i++){
//     int n_idx = neighborhoods[idx][i];
//     UVpt n_pos = _target_positions[n_idx];
//     double d = center_pos.dist(n_pos);
//     if (d>max_dist){
//       max_dist = d;
//     }
//     neighbors.push_back(n_idx);
//   }

  vector<bool> processed;
  unsigned int nb_synthesized_elements = _target_positions.size();
  for (unsigned int i=0 ; i<nb_synthesized_elements ; i++){
    processed.push_back(false);
  }
  
  UVpt center_pos = _target_positions[idx];
  max_dist = 0.0;
  queue< pair<int, int> > active_elements;
  processed[idx] = true;
  unsigned int nb_neighbors = neighborhoods[idx].size();
  for (unsigned int i=0 ; i<nb_neighbors ; i++){
    int n_idx = neighborhoods[idx][i];
    active_elements.push( pair<int, int> (n_idx, 1));
    processed[n_idx] = true;
  }
  while(!active_elements.empty()){
    pair<int, int> cur_seed = active_elements.front();
    active_elements.pop();
    int cur_idx = cur_seed.first;
    int ring_idx = cur_seed.second+1;
    neighbors.push_back(cur_idx);
    UVpt n_pos = _target_positions[cur_idx];
    double d = center_pos.dist(n_pos);
    if (d>max_dist){
      max_dist = d;
    }
    if (ring_idx>_ring_nb){
      continue;
    } 
    unsigned int nb_neighbors = neighborhoods[cur_idx].size();
    for (unsigned int j=0 ; j<nb_neighbors ; j++){
      int n_idx = neighborhoods[cur_idx][j];
      if (!processed[n_idx]){
	active_elements.push(pair<int, int> (n_idx, ring_idx));
	processed[n_idx] = true;
      }
    }
  }
}

void 
StrokeGroup::element_synthesis(GestureCell* target_cell){
  cerr << "element_synthesis" << endl;
  // 1- Distribute elements to define their positions
  _target_positions.clear();
  if (_reference_frame==AXIS){
    if (_distribution==LLOYD) {
      lloyd_1D(target_cell);
    } else {
      stratified_1D(target_cell);
    }
  } else {
    if (_distribution==LLOYD) {
      lloyd_2D(target_cell);
    } else {
      stratified_2D(target_cell);
    }
  }


  // 2- Synthesize an element at each location
  _target_indices.clear();
  _target_scales.clear();
  _target_angles.clear();
  vector<double> sorted_lengths = _length.measures;
  sort(sorted_lengths.begin(), sorted_lengths.end());
  vector<double> sorted_widths = _width.measures;
  sort(sorted_widths.begin(), sorted_widths.end());
  vector<double> sorted_angles = _orientation.measures;
  sort(sorted_angles.begin(), sorted_angles.end());
  unsigned int nb_synth_elements = _target_positions.size();

  int prev_shape_idx = -1;
  for (unsigned int i=0 ; i<nb_synth_elements ; i++){
    // 1- Pick a random shape index
    int shape_idx = get_shape_idx(prev_shape_idx);
    _target_indices.push_back(shape_idx);
    prev_shape_idx = shape_idx;
        
    // 2- Choose the extents of each element
    double ref_length = _stroke_paths[shape_idx]->axis_a().length();
    double ref_width = _stroke_paths[shape_idx]->axis_b().length();
    double target_length, target_width;
    get_close_value(ref_length, target_length, sorted_lengths, false);
    get_close_value(ref_width, target_width, sorted_widths, false);
    double length_ratio = target_length/ref_length;
    double width_ratio = target_width/ref_width;
    if (target_length<2.0 || ref_length<2.0){
      length_ratio = 1.0;
    }
    UVvec scale(length_ratio, width_ratio);
    _target_scales.push_back(scale);
    
    // 3- Choose the orientation of each element
    double ref_angle =  get_angle(_stroke_paths[shape_idx]);
    double target_angle;
    get_close_value(ref_angle, target_angle, sorted_angles, true);
    if (abs(ref_angle-target_angle) > abs(ref_angle-(target_angle+M_PI))){
      target_angle += M_PI;
    } else if (abs(ref_angle-target_angle) > abs(ref_angle-(target_angle-M_PI))){
      target_angle -= M_PI;      
    }
    _target_angles.push_back(target_angle);
    
    // 4- Optionally pick a candidate stroke style 
    if (_style_analyzed){
      // Pick a candidate pressure from nearest neighbors
      // Shift and scale element's stroke pressure
    }
  }
}


int
StrokeGroup::get_shape_idx(int prev_idx){
  int nb_elements = _elements.size();
  int shape_idx;
  do {
    double r_val = (double)rand()/(double)RAND_MAX;
    shape_idx = _elements[(int)(r_val*nb_elements)];
  } while (shape_idx==prev_idx);
  
  return shape_idx;
}

void
StrokeGroup::get_close_value(const double ref_val, double& target_val, 
			     const vector<double>& measures, bool periodic){
  vector<double>::const_iterator l_bound = lower_bound(measures.begin(), measures.end(), ref_val);
  bool right_val = bool(rand()%2);
  if (l_bound==measures.begin()) {
    if (periodic && !right_val) {
      target_val = *(measures.end()-1);
    } else {
      target_val = *(l_bound+1);
    }
  } else if (l_bound==measures.end()-1) {    
    if (periodic && right_val) {
      target_val = *(measures.begin());
    } else {
      target_val = *(l_bound-1);
    }
  } else {
    target_val = (right_val) ? *(l_bound+1) : *(l_bound-1);
  }
  
}

void
StrokeGroup::lloyd_1D(GestureCell* target_cell){

  // 1- Initialise nodes with random positions 
  unsigned int nb_ref_elements = _elements.size();
  int nb_elements = (int)(nb_ref_elements*target_cell->scale()/_bbox.width());
  vector<double> cur_nodes;
  vector<double> v_pos;
  double offset_std = _offset.std/target_cell->scale();
  for (int i=0 ; i<nb_elements ; i++){
    double u_rand = (double)rand()/(double)RAND_MAX;
    cur_nodes.push_back(u_rand);
    double v_sample = sample(0.5, offset_std);    
    v_pos.push_back(v_sample);
  }
  
  if (nb_elements<2){
    return;
  }

  // 2- Build the Delaunay triangulation and compute the edge length min and max
  sort(cur_nodes.begin(), cur_nodes.end());
  vector< pair<int, int> > cur_edges;
  for (int i=1 ; i<nb_elements ; i++){
    UVpt cur_node (cur_nodes[i], v_pos[i]);
    UVpt prev_node (cur_nodes[i-1], v_pos[i-1]);
    cur_edges.push_back(pair<int, int>(i, i-1));
  }

  // 3- Iterate Lloyd algorithm 
  int count = 0;
  while (count<MAX_COUNT){

    // compute the voronoi diagram and move the nodes to the center of their voronoi region 
    vector< pair<double, double> > bounds;
    bounds.push_back(pair<double,double>(0.0, (cur_nodes[0]+cur_nodes[1])*0.5));
    for (int i=1 ; i<nb_elements-1 ; i++){
      double inf_bound = (cur_nodes[i-1]+cur_nodes[i])*0.5;
      double sup_bound = (cur_nodes[i+1]+cur_nodes[i])*0.5;
      bounds.push_back(pair<double, double>(inf_bound, sup_bound));
    }
    bounds.push_back(pair<double,double>((cur_nodes[nb_elements-2]+cur_nodes[nb_elements-1])*0.5, 1.0));
    for (int i=0 ; i<nb_elements ; i++){
      cur_nodes[i] = (bounds[i].first+bounds[i].second)*0.5;
    }

    count++;
  }

  // 4- Copy the positions back
  for (int i=0 ; i<nb_elements ; i++){
    _target_positions.push_back(UVpt(cur_nodes[i], v_pos[i]));
  }
}


void
StrokeGroup::stratified_1D(GestureCell* target_cell){
  // 1- extract a minimum, a maximum and an average proximity
  double min_prox = 1e19;
  double max_prox = 0.0;
  double prox_avg = 0.0;
  unsigned int nb_elements = _elements.size();
  for (unsigned int i=0 ; i<nb_elements ; i++){
    if (_relative_position[i].measures.size()==0){
      continue;
    }
    if (_relative_position[i].min < min_prox){
      min_prox = _relative_position[i].min;
    }
    if (_relative_position[i].max > max_prox){
      max_prox = _relative_position[i].max;
    }
    prox_avg += _relative_position[i].avg;
  }
  prox_avg /= (double)nb_elements;

  // 2- fit a grid of voxels on the target cell that match the average proximity
  double grid_size_u = prox_avg/target_cell->scale();
  double nb_voxels_u = 1.0/grid_size_u;
  double voxel_u_offset = (ceil(nb_voxels_u)-nb_voxels_u)*grid_size_u;
  
  // 3- sample each voxel using a gaussian pdf that ensures the max proximity
  int grid_size = (int)ceil(nb_voxels_u);
  vector<UVpt> candidate_pos(grid_size);
  double voxel_std = (max_prox-prox_avg)*0.5/(2.0*target_cell->scale());
  double offset_std = _offset.max/(2.0*target_cell->scale());
  for (int i=0 ; i<grid_size ; i++){
    double voxel_x_pos = (i+0.5)*grid_size_u - voxel_u_offset;
    double voxel_y_pos = 0.5;
    double sample_x_pos = sample(voxel_x_pos, voxel_std);
    double sample_y_pos = sample(voxel_y_pos, offset_std);
    candidate_pos[i] = UVpt(sample_x_pos, sample_y_pos);
  }

  // 4- remove pairs with a proximity smaller than the minimum allowed
  double min_prox_uv = min_prox/target_cell->scale();
  bool candidates_remain = true;
  int count = 0;
  
  
  while(candidates_remain){
    int rem_idx = -1;
    double max_dist = 0.0;
    candidates_remain = false;
    for (int i=0 ; i<grid_size ; i++){
      int cur_idx = i;
      UVpt cur_sample = candidate_pos[cur_idx];
      if (cur_sample == UVpt(-1.0,-1.0)){
	continue;
      }

      // compute the barycenter of points closest than the minimum proximity
      double x_sum=0.0, y_sum=0.0;
      int norm = 0;
      for (int k=i-1 ; k<=i+1 ; k++){
	if (k>=0 && k<grid_size){
	  int n_idx = k;
	  UVpt n_sample = candidate_pos[n_idx];
	  if (n_idx!=cur_idx && n_sample!=UVpt(-1.0,-1.0) && n_sample.dist(cur_sample)<=min_prox_uv){
	    x_sum += n_sample[0];
	    y_sum += n_sample[1];
	    norm ++;
	  }
	}
      }
      if (norm>0){
	UVvec bary_vec = (UVpt(x_sum/norm, y_sum/norm)-candidate_pos[cur_idx]);
	if (bary_vec.length()>max_dist){
	  max_dist = bary_vec.length();
	  rem_idx = cur_idx;
	}
      }
    }
    if (rem_idx != -1){
      candidate_pos[rem_idx] = UVpt(-1.0, -1.0);
      candidates_remain = true;
      count++;
    }
  }


  // 5- Copy the positions back
  for (int i=0 ; i<grid_size ; i++){
    if (candidate_pos[i]!=UVpt(-1.0,-1.0)){
      _target_positions.push_back(candidate_pos[i]);
    }
  }
}

bool
StrokeGroup::degenerated_edge(int idx, const vector< pair<int, int> >& edges, const vector<UVpt>& node_pos,
			      const vector<bool>& border_edges, const vector< vector<int> >& node_edges){

  // 1- find one or two adjacent triangles
  vector<int> adjacent_nodes;
  int node1 = edges[idx].first;
  int node2 = edges[idx].second;
  unsigned int nb_node1_edges = node_edges[node1].size();
  unsigned int nb_node2_edges = node_edges[node2].size();
  for (unsigned int i=0 ; i<nb_node1_edges ; i++){
    int n1_idx = (edges[node_edges[node1][i]].first==node1) ? edges[node_edges[node1][i]].second : edges[node_edges[node1][i]].first;
    for (unsigned int j=0 ; j<nb_node2_edges; j++){
      int n2_idx = (edges[node_edges[node2][j]].first==node2) ? edges[node_edges[node2][j]].second : edges[node_edges[node2][j]].first;
      if (n1_idx==n2_idx){
	UVpt pos1 = node_pos[node1];
	UVpt pos2 = node_pos[node2];
	UVpt pos3 = node_pos[n1_idx];
	double theta1 = (pos2-pos1).angle(pos3-pos1);
	double theta2 = (pos1-pos2).angle(pos3-pos2);
	double theta3 = (pos2-pos3).angle(pos1-pos3);
	if (theta1<MAX_ANGLE && theta2<MAX_ANGLE && theta3<MAX_ANGLE){
 	  return false;
	}
      }
    }
  }
  return true;
}

void
StrokeGroup::lloyd_2D(GestureCell* target_cell){
  // 1- Initialise nodes with random positions 
  unsigned int nb_ref_elements = _elements.size();
  int nb_elements = (int)(nb_ref_elements*target_cell->scale()*target_cell->scale()/(_bbox.width()*_bbox.height()));
  vector<UVpt> cur_nodes;
  for (int i=0 ; i<nb_elements ; i++){
    double u_rand = (double)rand()/(double)RAND_MAX;
    double v_rand = (double)rand()/(double)RAND_MAX;
    cur_nodes.push_back(UVpt(u_rand, v_rand));
  }
  cerr << "random" << endl;

  // 2- build the Delaunay triangulation and compute min and max prox
  vector< pair<int, int> > cur_edges;
  vector<bool> border_edges;
  vector< vector<int> > node_edges;
  delaunay_edges(cur_nodes, cur_edges, border_edges, node_edges);
  cerr << "delaunay" << endl;

  // 3- Iterate Lloyd algorithm 
  int count = 0;
  cerr << "count= " << endl;
  while (/*(min_prox<ref_min_prox || max_prox>ref_max_prox) &&*/ count<MAX_COUNT){
    cerr << count << endl;

    // compute the voronoi regions and move the nodes to their centroids
    vector<UVpt> centroids;
    voronoi_centroids(cur_nodes, cur_edges, border_edges, node_edges, centroids);
    cur_nodes = centroids;
 
    // update the Delaunay triangulation and the min and max prox
    cur_edges.clear();
    border_edges.clear();
    node_edges.clear();
    delaunay_edges(cur_nodes, cur_edges, border_edges, node_edges);
    
    count++;
  }

  // 4- Copy the positions back
  for (int i=0 ; i<nb_elements ; i++){
    _target_positions.push_back(cur_nodes[i]);
  }
}

void
StrokeGroup::stratified_2D(GestureCell* target_cell){
  // 1- extract a minimum, a maximum and a std proximity
  double min_prox = 1e19;
  double max_prox = 0.0;
  double prox_avg = 0.0;
  unsigned int nb_elements = _elements.size();
  for (unsigned int i=0 ; i<nb_elements ; i++){
    if (_relative_position[i].measures.size()==0){
      continue;
    }
    if (_relative_position[i].min < min_prox){
      min_prox = _relative_position[i].min;
    }
    if (_relative_position[i].max > max_prox){
      max_prox = _relative_position[i].max;
    }
    prox_avg += _relative_position[i].avg;
  }
  prox_avg /= (double)nb_elements;

  // 2- fit a grid of voxels on the target cell that match the average proximity
  double grid_size_uv = prox_avg/target_cell->scale();
  double nb_voxels_uv = 1.0/grid_size_uv;
  double voxel_uv_offset = (ceil(nb_voxels_uv)-nb_voxels_uv)*grid_size_uv;
  
  // 3- sample each voxel using a gaussian pdf that ensures the max proximity
  int grid_size = (int)ceil(nb_voxels_uv);
  vector<UVpt> candidate_pos(grid_size*grid_size);
  double voxel_std = (max_prox-prox_avg)*0.5/(2.0*target_cell->scale());
  for (int i=0 ; i<grid_size ; i++){
    double voxel_x_pos = (i+0.5)*grid_size_uv - voxel_uv_offset;
    for (int j=0 ; j<grid_size ; j++){
      double voxel_y_pos = (j+0.5)*grid_size_uv - voxel_uv_offset;
      double sample_x_pos = sample(voxel_x_pos, voxel_std);
      double sample_y_pos = sample(voxel_y_pos, voxel_std);
      candidate_pos[j*grid_size+i] = UVpt(sample_x_pos, sample_y_pos);
    }
  }

  // 4- remove pairs with a proximity smaller than the minimum allowed
  double min_prox_uv = min_prox/target_cell->scale();
  bool candidates_remain = true;
  int count = 0;
  
  
  while(candidates_remain){
    int rem_idx = -1;
    double max_dist = 0.0;
    candidates_remain = false;
    for (int i=0 ; i<grid_size ; i++){
      for (int j=0 ; j<grid_size ; j++){
	int cur_idx = j*grid_size+i;
	UVpt cur_sample = candidate_pos[cur_idx];
	if (cur_sample == UVpt(-1.0,-1.0)){
	  continue;
	}

	// compute the barycenter of points closest than the minimum proximity
	double x_sum=0.0, y_sum=0.0;
	int norm = 0;
	for (int k=i-1 ; k<=i+1 ; k++){
	  for (int l=j-1 ; l<=j+1 ; l++){
	    if (k>=0 && k<grid_size && l>=0 && l<grid_size){
	      int n_idx = l*grid_size + k;
	      UVpt n_sample = candidate_pos[n_idx];
	      if (n_idx!=cur_idx && n_sample!=UVpt(-1.0,-1.0) && n_sample.dist(cur_sample)<=min_prox_uv){
		x_sum += n_sample[0];
		y_sum += n_sample[1];
		norm ++;
	      }
	    }
	  }
	}
	if (norm>0){
	  UVvec bary_vec = (UVpt(x_sum/norm, y_sum/norm)-candidate_pos[cur_idx]);
	  if (bary_vec.length()>max_dist){
	    max_dist = bary_vec.length();
	    rem_idx = cur_idx;
	  }
	}
      }
    }
    if (rem_idx != -1){
      candidate_pos[rem_idx] = UVpt(-1.0, -1.0);
      candidates_remain = true;
      count++;
    }
  }


  // 5- Copy the positions back
  for (int i=0 ; i<grid_size*grid_size ; i++){
    if (candidate_pos[i]!=UVpt(-1.0,-1.0)){
      _target_positions.push_back(candidate_pos[i]);
    }
  }
}

void 
StrokeGroup::group_synthesis(GestureCell* target_cell){
  cerr << "group_synthesis" << endl;
  _corrected_indices = _target_indices;
  _corrected_positions = _target_positions;
  _corrected_scales = _target_scales;
  _corrected_angles = _target_angles;


  // 1- reconstruct a neighborhood from synthesized elements
  vector< vector<int> > synthesized_neighborhoods;
  vector<int> ordered_elements;
  if (_reference_frame==AXIS){
    compute_synthesized_neighborhoods_1D(synthesized_neighborhoods);
    compute_element_order_1D(synthesized_neighborhoods, ordered_elements);
  } else {
    compute_synthesized_neighborhoods_2D(synthesized_neighborhoods);    
    compute_element_order_2D(synthesized_neighborhoods, ordered_elements);
  }
  
  // 2- Adjust position, orientation, extent and stroke style based on 1-ring neighborhood comparisons
  unsigned int nb_synth_elements = synthesized_neighborhoods.size();
  for (unsigned int i=0 ; i<nb_synth_elements ; i++){
    // 1- Get best reference element
    double best_match = 1e19;
    int closest_ref_idx = -1;
    vector<int> ref_relevant_edges, target_relevant_edges;
    unsigned int nb_ref_elements = _elements.size();
    int idx = ordered_elements[i];
    for (unsigned int j=0 ; j<nb_ref_elements ; j++){
      vector<int> ref_edges, target_edges;
      double match;
      match = get_1ring_match(idx, _target_positions[idx], synthesized_neighborhoods[idx], target_edges, target_cell->scale(), 
			      j, _stroke_paths[_elements[j]]->center(), _element_neighbors[j], ref_edges);
      if (match<best_match){
	best_match = match;
	closest_ref_idx = j;
	ref_relevant_edges = ref_edges;
	target_relevant_edges = target_edges;
      }
    }
    
    if (closest_ref_idx==-1){
      _corrected_indices[idx] = _target_indices[idx];
      _corrected_positions[idx] = _target_positions[idx];
      _corrected_scales[idx] = _target_scales[idx];
      _corrected_angles[idx] = _target_angles[idx];
      continue;
    } 

    
    // 2- Adjust position, orientation, length and width
    
    // compute a relative ref position 
    vector<int> ref_neighbors = _element_neighbors[closest_ref_idx];
    unsigned int nb_ref_neighbors = ref_neighbors.size();
    vector<PIXEL> neighbor_ref_pos;
    for (unsigned int k=0 ; k<nb_ref_neighbors ; k++){
      PIXEL n_pos = _stroke_paths[_elements[ref_neighbors[k]]]->center();
      neighbor_ref_pos.push_back(n_pos);
    }
    PIXEL center_ref_pos = _stroke_paths[_elements[closest_ref_idx]]->center();
    PIXEL one_ring_ref_pos = get_1ring_position(center_ref_pos, neighbor_ref_pos, ref_relevant_edges);
    VEXEL pos_shift = center_ref_pos - one_ring_ref_pos;

     
    // adjust target position
    vector<int> target_neighbors = synthesized_neighborhoods[idx];
    unsigned int nb_target_neighbors = target_neighbors.size();
    vector<PIXEL> neighbor_target_pos;
    for (unsigned int k=0 ; k<nb_target_neighbors ; k++){
      int n_idx = target_neighbors[k];
      PIXEL n_pos (_corrected_positions[n_idx][0]*target_cell->scale(),
		   _corrected_positions[n_idx][1]*target_cell->scale());
      neighbor_target_pos.push_back(n_pos);
    }
    PIXEL center_target_pos (_corrected_positions[idx][0]*target_cell->scale(),
			     _corrected_positions[idx][1]*target_cell->scale());
    PIXEL one_ring_target_pos = get_1ring_position(center_target_pos, neighbor_target_pos, target_relevant_edges);
    PIXEL shifted_pos = one_ring_target_pos + pos_shift;
    UVpt corrected_pos (shifted_pos[0]/target_cell->scale(), shifted_pos[1]/target_cell->scale());
    
    // compute a corrected angle
    double corrected_angle = _orientation.measures[closest_ref_idx];

    // compute corrected length and width
    double corrected_length = _length.measures[closest_ref_idx];
    double corrected_width = _width.measures[closest_ref_idx];
    double init_length = _stroke_paths[_target_indices[idx]]->axis_a().length();
    double init_width = _stroke_paths[_target_indices[idx]]->axis_b().length();
    UVvec corrected_scale = UVvec(corrected_length/init_length, corrected_width/init_width);
    
    // assign corrected values
    _corrected_indices[idx] = _target_indices[idx];
    _corrected_positions[idx] = corrected_pos;
    _corrected_angles[idx] = corrected_angle;
    _corrected_scales[idx] = corrected_scale;
    
//     cerr << "position: " << _target_positions[idx] << " -> " << _corrected_positions[idx]<< endl;
//     cerr << "angle: " << _target_angles[idx] << " -> " << _corrected_angles[idx]<< endl;
//     cerr << "scale: " << _target_scales[idx] << " -> " << _corrected_scales[idx]<< endl;
//     cerr << endl;
    
  }

}

void 
StrokeGroup::compute_synthesized_neighborhoods_1D(vector< vector<int> >& neighborhoods){
  unsigned int nb_synthesized_elements = _target_positions.size();
  for (unsigned int i=0 ; i<nb_synthesized_elements ; i++){
    int left_neighbor = -1;
    int right_neighbor = -1; 
    double min_left_dist = -1e19;
    double min_right_dist = 1e19;
    CUVpt& cur_center = _target_positions[i];
    for (unsigned int j=0 ; j<nb_synthesized_elements ; j++){
      if (i!=j){
	CUVpt& cand_center = _target_positions[j];
	double cand_dist = cand_center[0]-cur_center[0];
	if (cand_dist>0.0 && cand_dist<min_right_dist){
	  right_neighbor = j;
	  min_right_dist = cand_dist;
	}
	if (cand_dist<0.0 && cand_dist>min_left_dist){
	  left_neighbor = j;
	  min_left_dist = cand_dist;
	}
	
      }
    }

    vector<int> neighbors;
    if (left_neighbor!=-1){
      neighbors.push_back(left_neighbor);      
    }
    if (right_neighbor!=-1){
      neighbors.push_back(right_neighbor);      
    }
    neighborhoods.push_back(neighbors);
    
  }
}

void 
StrokeGroup::compute_synthesized_neighborhoods_2D(vector< vector<int> >& neighborhoods){
  vector< pair<int, int> > edges;
  vector<bool> border_edges;
  vector< vector<int> > node_edges;
  delaunay_edges(_target_positions, edges, border_edges, node_edges);
  
  unsigned int nb_nodes = node_edges.size();
  for (unsigned int i=0 ; i<nb_nodes ; i++){
    unsigned int nb_node_edges = node_edges[i].size();
    vector<int> neighbors;
    for (unsigned int j=0 ; j<nb_node_edges ; j++){
      if (!degenerated_edge(node_edges[i][j], edges, _target_positions, border_edges, node_edges)){
	int n_idx = (edges[node_edges[i][j]].first==(int)i) ? edges[node_edges[i][j]].second : edges[node_edges[i][j]].first;
	neighbors.push_back(n_idx);
// 	WORLD::show(NDCZpt(_target_positions[i][0], _target_positions[i][1], 0.0), 
// 		    NDCZpt(_target_positions[n_idx][0], _target_positions[n_idx][1], 0.0), 
// 		    1.0, COLOR::black, 1.0, false);
//       } else {
// 	int n_idx = (edges[node_edges[i][j]].first==(int)i) ? edges[node_edges[i][j]].second : edges[node_edges[i][j]].first;
// 	WORLD::show(NDCZpt(_target_positions[i][0], _target_positions[i][1], 0.0), 
// 		    NDCZpt(_target_positions[n_idx][0], _target_positions[n_idx][1], 0.0), 
// 		    1.0, COLOR::red, 1.0, false);	
      }
    }
    neighborhoods.push_back(neighbors);
  }
}


void 
StrokeGroup::compute_element_order_1D(const vector< vector<int> >& neighborhoods, vector<int>& ordered_elements){
  if (_synth_method==CAUSAL){
    // 1- find center element
    unsigned int nb_synthesized_elements = _target_positions.size();
    double min_dist = 1e19;
    UVpt virtual_center(0.5, 0.5);
    int center_idx = -1;
    for (unsigned int i=0 ; i<nb_synthesized_elements ; i++){
      double d = virtual_center.dist(_target_positions[i]);
      if (d<min_dist){
	min_dist = d;
	center_idx = i;
      }
    }
    if (center_idx==-1){
      return;
    }
    
    // 2- order in concentric rings around center element
    vector<bool> processed;
    for (unsigned int i=0 ; i<nb_synthesized_elements ; i++){
      processed.push_back(false);
    }
  
    queue<int> active_elements;
    active_elements.push(center_idx);
    processed[center_idx] = true;
    while(!active_elements.empty()){
      int cur_idx = active_elements.front();
      active_elements.pop();
      ordered_elements.push_back(cur_idx);
      unsigned int nb_neighbors = neighborhoods[cur_idx].size();
      for (unsigned int j=0 ; j<nb_neighbors ; j++){
	int n_idx = neighborhoods[cur_idx][j];
	if (!processed[n_idx]){
	  active_elements.push(n_idx);
	  processed[n_idx] = true;
	}
      }
    }
    
  } else {
    unsigned int nb_synthesized_elements = _target_indices.size();
    for (unsigned int i=0 ; i<nb_synthesized_elements ; i++){
      ordered_elements.push_back(i);
    }
  }
}


void 
StrokeGroup::compute_element_order_2D(const vector< vector<int> >& neighborhoods, vector<int>& ordered_elements){
  if (_synth_method==CAUSAL){
    // 1- find center element
    unsigned int nb_synthesized_elements = _target_positions.size();
    double min_dist = 1e19;
    UVpt virtual_center(0.5, 0.5);
    int center_idx = -1;
    for (unsigned int i=0 ; i<nb_synthesized_elements ; i++){
      double d = virtual_center.dist(_target_positions[i]);
      if (d<min_dist){
	min_dist = d;
	center_idx = i;
      }
    }
    
    // 2- order in concentric rings around center element
    vector<bool> processed;
    for (unsigned int i=0 ; i<nb_synthesized_elements ; i++){
      processed.push_back(false);
    }
  
    queue<int> active_elements;
    active_elements.push(center_idx);
    processed[center_idx] = true;
    while(!active_elements.empty()){
      int cur_idx = active_elements.front();
      active_elements.pop();
      ordered_elements.push_back(cur_idx);
      unsigned int nb_neighbors = neighborhoods[cur_idx].size();
      for (unsigned int j=0 ; j<nb_neighbors ; j++){
	int n_idx = neighborhoods[cur_idx][j];
	if (!processed[n_idx]){
	  active_elements.push(n_idx);
	  processed[n_idx] = true;
	}
      }
    }

//     // debug
//     unsigned int nb_ordered_elements = ordered_elements.size();
//     for (unsigned int i=0 ; i<nb_ordered_elements ; i++){
//       double val = (double)i/(double)nb_ordered_elements;
//       COLOR c(val, val, val);
//       WORLD::show(NDCZpt(_target_positions[ordered_elements[i]][0], _target_positions[ordered_elements[i]][1], 0.0), 
// 		  4.0, c, 1.0-val, false);  
//     }
    
   
  } else {
    unsigned int nb_synthesized_elements = _target_indices.size();
    for (unsigned int i=0 ; i<nb_synthesized_elements ; i++){
      ordered_elements.push_back(i);
    }
  }
}


double 
StrokeGroup::get_1ring_match(int target_center_idx, CUVpt& target_center, const vector<int>& target_neighbors, vector<int>& relevant_target_edges, double target_scale, 
			     int ref_center_idx, CPIXEL& ref_center, const vector<int>& ref_neighbors, vector<int>& relevant_ref_edges){
  unsigned int nb_target_neighbors = target_neighbors.size();
  unsigned int nb_ref_neighbors = ref_neighbors.size();
  
  // 1- find best matching neighbors from target to ref
  vector<int> target_best_matches;
  for (unsigned int i=0 ; i<nb_target_neighbors ; i++){
    int target_idx = target_neighbors[i];
    PIXEL target_pos; 
    if (_synth_method==CAUSAL){
      target_pos = PIXEL ((_corrected_positions[target_idx][0]-target_center[0])*target_scale,
			  (_corrected_positions[target_idx][1]-target_center[1])*target_scale);
    } else {
      target_pos = PIXEL ((_target_positions[target_idx][0]-target_center[0])*target_scale,
			  (_target_positions[target_idx][1]-target_center[1])*target_scale);
    }
    double min_dist = 1e19;
    int best_match_idx = -1;
    for (unsigned int j=0 ; j<nb_ref_neighbors ; j++){
      int ref_idx = _elements[ref_neighbors[j]];
      PIXEL ref_pos (_stroke_paths[ref_idx]->center()[0]-ref_center[0],
		     _stroke_paths[ref_idx]->center()[1]-ref_center[1]);
      double d = target_pos.dist(ref_pos);
      if (d<min_dist){
	min_dist = d;
	best_match_idx = j;
      }
    }
    target_best_matches.push_back(best_match_idx);
  }
  
  // 2- find best matching neighbors from ref to target
  vector<int> ref_best_matches;
  for (unsigned int i=0 ; i<nb_ref_neighbors ; i++){
    int ref_idx = _elements[ref_neighbors[i]];
    PIXEL ref_pos (_stroke_paths[ref_idx]->center()[0]-ref_center[0],
		   _stroke_paths[ref_idx]->center()[1]-ref_center[1]);
    double min_dist = 1e19;
    int best_match_idx = -1;
    for (unsigned int j=0 ; j<nb_target_neighbors ; j++){
      int target_idx = target_neighbors[j];
      PIXEL target_pos; 
      if (_synth_method==CAUSAL){
	target_pos = PIXEL ((_corrected_positions[target_idx][0]-target_center[0])*target_scale,
			    (_corrected_positions[target_idx][1]-target_center[1])*target_scale);
      } else {
	target_pos = PIXEL ((_target_positions[target_idx][0]-target_center[0])*target_scale,
			    (_target_positions[target_idx][1]-target_center[1])*target_scale);
      }
      double d = ref_pos.dist(target_pos);
      if (d<min_dist){
	min_dist = d;
	best_match_idx = j;
      }
    }
    ref_best_matches.push_back(best_match_idx);
  }

  // 3- keep only agreeing matches to get relevant edges
  int nb_relevant_edges = 0;
  for (unsigned int i=0 ; i<nb_target_neighbors ; i++){
    int ref_idx = target_best_matches[i];
    if (ref_idx == -1 || ref_best_matches[ref_idx]!=(int)i){
      relevant_target_edges.push_back(-1);
    } else {
      relevant_target_edges.push_back(ref_idx);
      nb_relevant_edges++;
    }
  }
  for (unsigned int i=0 ; i<nb_ref_neighbors ; i++){
    int target_idx = ref_best_matches[i];
    if (target_idx == -1 || target_best_matches[target_idx]!=(int)i){
      relevant_ref_edges.push_back(-1);
    } else {
      relevant_ref_edges.push_back(target_idx);
    }
  }

  // 4- compute a match for the whole 1ring neighborhood
  if (nb_relevant_edges<2){
    return 1e19;
  }
  double match = get_element_match(target_center_idx, target_center, ref_center_idx, ref_center, target_scale);
  for (unsigned int i=0 ; i<nb_target_neighbors; i++){
    int best_match_idx = target_best_matches[i];
    if (best_match_idx!=-1){
      int target_idx = target_neighbors[i];
      int ref_idx = ref_neighbors[best_match_idx];
      double element_match = get_element_match(target_idx, target_center, ref_idx, ref_center, target_scale); 
      match += element_match;
    }
  }
  match *= (nb_target_neighbors+nb_ref_neighbors+1)/(2*nb_relevant_edges+1);
  return match;
}


double 
StrokeGroup::get_element_match(int target_idx, CUVpt& target_origin, int ref_idx, CPIXEL& ref_origin, double target_scale){

  PIXEL ref_center (_stroke_paths[_elements[ref_idx]]->center()[0]-ref_origin[0],
		    _stroke_paths[_elements[ref_idx]]->center()[1]-ref_origin[1]);
  VEXEL ref_axis_a = _stroke_paths[_elements[ref_idx]]->axis_a();
  VEXEL ref_axis_b = _stroke_paths[_elements[ref_idx]]->axis_b();

  PIXEL target_center;
  VEXEL target_axis_a, target_axis_b;
  if (_synth_method==CAUSAL){
    target_center = PIXEL ((_corrected_positions[target_idx][0]-target_origin[0])*target_scale,
			   (_corrected_positions[target_idx][1]-target_origin[1])*target_scale);
    int ref_el_idx = _elements[_corrected_indices[target_idx]];
    target_axis_a = _stroke_paths[ref_el_idx]->axis_a()*_corrected_scales[target_idx][0];
    target_axis_b = _stroke_paths[ref_el_idx]->axis_b()*_corrected_scales[target_idx][1];
  } else {
    target_center = PIXEL ((_target_positions[target_idx][0]-target_origin[0])*target_scale,
			   (_target_positions[target_idx][1]-target_origin[1])*target_scale);
    int ref_el_idx = _elements[_target_indices[target_idx]];
    target_axis_a = _stroke_paths[ref_el_idx]->axis_a()*_target_scales[target_idx][0];
    target_axis_b = _stroke_paths[ref_el_idx]->axis_b()*_target_scales[target_idx][1];
  }
    
  double dist1 = StrokePaths::hausdorff_distance(ref_center, ref_axis_a, ref_axis_b, 
						 target_center, target_axis_a, target_axis_b);
  double dist2 = StrokePaths::hausdorff_distance(target_center, target_axis_a, target_axis_b,
						 ref_center, ref_axis_a, ref_axis_b);

  return max(dist1, dist2);
}

PIXEL
StrokeGroup::get_1ring_position(CPIXEL& center_pos, const vector<PIXEL>& neighbor_pos, vector<int>& relevant_edges){
  unsigned int nb_neighbors = neighbor_pos.size();
  int nb_relevant_edges = 0;
  for (unsigned int i=0 ; i<nb_neighbors ; i++){
    if (relevant_edges[i]!=-1){
      nb_relevant_edges++;
    }
  }
  if (nb_relevant_edges<2){
    return center_pos;
  }
  
  double x_sum = 0.0;
  double y_sum = 0.0;
  for (unsigned int i=0 ; i<nb_neighbors ; i++){
    if (relevant_edges[i]==-1) {
      continue;
    }
    x_sum += neighbor_pos[i][0];
    y_sum += neighbor_pos[i][1];
  }
  return PIXEL(x_sum/nb_relevant_edges, y_sum/nb_relevant_edges);
}


double
StrokeGroup::get_1ring_angle(double center_angle, const vector<VEXEL>& neighbor_axis, vector<int>& relevant_edges){
  unsigned int nb_neighbors = neighbor_axis.size();
  int nb_relevant_edges = 0;
  for (unsigned int i=0 ; i<nb_neighbors ; i++){
    if (relevant_edges[i]!=-1){
      nb_relevant_edges++;
    }
  }
  if (nb_relevant_edges<2){
    return center_angle;
  }
  
  VEXEL axis_sum (0.0, 0.0);
  for (unsigned int i=0 ; i<nb_neighbors ; i++){
    if (relevant_edges[i]==-1) {
      continue;
    }
    if (neighbor_axis[i]*axis_sum >= 0.0){
      axis_sum += neighbor_axis[i];
    } else {
      axis_sum -= neighbor_axis[i];
    }
  }
  VEXEL main_dir = (axis_sum*VEXEL(0.0, 1.0)>=0) ? axis_sum.normalized() : -(axis_sum.normalized());
  return  main_dir.angle(VEXEL(1.0, 0.0));
}


double
StrokeGroup::get_1ring_length(double center_length, const vector<double>& neighbor_lengths, vector<int>& relevant_edges){
  unsigned int nb_neighbors = neighbor_lengths.size();
  int nb_relevant_edges = 0;
  for (unsigned int i=0 ; i<nb_neighbors ; i++){
    if (relevant_edges[i]!=-1){
      nb_relevant_edges++;
    }
  }
  if (nb_relevant_edges<2){
    return center_length;
  }

  double length_sum = 0.0;
  for (unsigned int i=0 ; i<nb_neighbors ; i++){
    if (relevant_edges[i]==-1) {
      continue;
    }
    length_sum += neighbor_lengths[i];
  }
  return length_sum/nb_relevant_edges;  
}


double
StrokeGroup::get_1ring_width(double center_width, const vector<double>& neighbor_widths, vector<int>& relevant_edges){
  unsigned int nb_neighbors = neighbor_widths.size();
  int nb_relevant_edges = 0;
  for (unsigned int i=0 ; i<nb_neighbors ; i++){
    if (relevant_edges[i]!=-1){
      nb_relevant_edges++;
    }
  }
  if (nb_relevant_edges<2){
    return center_width;
  }
  
  double width_sum = 0.0;
  for (unsigned int i=0 ; i<nb_neighbors ; i++){
    if (relevant_edges[i]==-1) {
      continue;
    }
    width_sum += neighbor_widths[i];
  }
  return width_sum/nb_relevant_edges;  
}





double
StrokeGroup::sample(double avg, double std) {
  double x = (double)rand()/(double)RAND_MAX;
  double p;
  if (x==0.5){
    p = 0;
  } else if (x<=0) {
    p = 1e9;
  } else if (x>=1) {
    p = -1e9;
  } else if (x<0.5) {
    p = -secant(1-x);
  } else {
    p = secant(x);
  }
  
  p = p*std + avg;
  p = max(p, avg-2*std);
  p = min(p, avg+2*std);
  
  return p;
}


double 
StrokeGroup::secant(double a) {
  double x0 = 1.0;
  double x1 = 1.1;
  double xi = x0, xii= x1, x;
  double yi = normal_distrib(x0)-a, yii = normal_distrib(x1)-a, y;
  
  do {
    x = xii - yii * (xii - xi) / (yii - yi);
    y = normal_distrib(x)-a;
    
    xi = xii; xii = x;
    yi = yii; yii = y;
  } while (abs(y)>1e-14);
  
  return x;
}


double 
StrokeGroup::normal_distrib(double x){
  if (x==0) return 0.5;
  if (x<0) return 1 - normal_distrib(-x);
  if (x>12) return 1;
  
  double fac = x;
  double tot = fac;
  int n=1;
  
  while (abs(fac) > 1e-14) {
    fac = fac * x * x / ( 2.0 * n + 1);
    tot += fac; 
    n  = n+1;
  } 
  
  double zn = 1.0 / (sqrt(2*M_PI) * exp( x * x / 2));
  return 0.5 + zn * tot; 
}




bool
StrokeGroup::inside(const UVpt& p, int l) {
  if (l==0){
    return p[0]>=0.0;
  }

  if (l==1){
    return p[1]>=0.0;
  }

  if (l==2){
    return p[0]<=1.0;    
  }

  return p[1]<=1.0;
}



void
StrokeGroup::render_synthesized_strokes(GestureCell* target_cell) const{
  if (_target_indices.empty()){
    return;
  }
  double shape_threshold = 0.45;

  unsigned int nb_synthesized_strokes = _target_indices.size();
  for (unsigned int i=0 ; i<nb_synthesized_strokes ; i++){
    // blend original and corrected versions
    int idx1 = _target_indices[i];
    double press1;
    if (_correction_amount<shape_threshold){
      press1 = 1.0;
    } else if (_correction_amount>1.0-shape_threshold){
      press1 = 0.0;
    } else {
      press1 = (1.0-shape_threshold-_correction_amount)/(1.0-2*shape_threshold);
    }
    UVvec scale1 =(1.0-_correction_amount)*_target_scales[i] + _correction_amount*_corrected_scales[i];
    VEXEL main_dir1 = _stroke_paths[idx1]->axis_a();
    main_dir1 = (main_dir1*VEXEL(0.0, 1.0)>=0) ? main_dir1 : -main_dir1;
    double angle1 = (1.0-_correction_amount)*_target_angles[i] + _correction_amount*_corrected_angles[i];
//     if (main_dir1*VEXEL(1.0, 0.0)<0.0){
//       angle1 = M_PI-angle1;
//     }
    UVpt pos1 = (1.0-_correction_amount)*_target_positions[i] + _correction_amount*_corrected_positions[i];
    _stroke_paths[idx1]->synthesize(target_cell, press1, scale1, angle1, pos1);

    int idx2 = _corrected_indices[i];
    double press2;
    if (_correction_amount<shape_threshold){
      press2 = 0.0;
    } else if (_correction_amount>1.0-shape_threshold){
      press2 = 1.0;
    } else {
      press2 = 1.0 - (1.0-shape_threshold-_correction_amount)/(1.0-2*shape_threshold);
    }
    UVvec scale2 = (1.0-_correction_amount)*_target_scales[i] + _correction_amount*_corrected_scales[i];
    VEXEL main_dir2 = _stroke_paths[idx2]->axis_a();
    main_dir2 = (main_dir2*VEXEL(0.0, 1.0)>=0) ? main_dir2 : -main_dir2;
    double angle2 = (1.0-_correction_amount)*_target_angles[i] + _correction_amount*_corrected_angles[i];
//     if (main_dir2*VEXEL(1.0, 0.0)<0.0){
//       angle2 = M_PI-angle2;
//     }
    UVpt pos2 = (1.0-_correction_amount)*_target_positions[i] + _correction_amount*_corrected_positions[i];
    _stroke_paths[idx2]->synthesize(target_cell, press2, scale2, angle2, pos2);

  }  
}



/************************************************************
 ** TODO: remove the nearest neighbor approach (deprecated)**
 ***********************************************************/

void 
StrokeGroup::synthesize_NN(GestureCell* target_cell) {
  if (_elements.size()==0 || _element_pairs.size()==0){
    return;
  }

  // first create a graph with edges length following the proximity PDF
  vector<UVpt> nodes;
  vector< pair<int, int> > edges;
  if (_reference_frame==AXIS){
    compute_graph_1D(target_cell, nodes, edges);
  } else {
    compute_graph_2D(target_cell, nodes, edges);
  }

  // then compute element parameters at each node
  unsigned int nb_nodes = nodes.size();
  _target_indices = vector<int> (nb_nodes);
  _target_positions = vector<UVpt> (nb_nodes);
  _target_angles = vector<double> (nb_nodes);
  _target_scales = vector<UVvec> (nb_nodes);
  _element_history.clear();
  for (unsigned int i=0 ; i<nb_nodes ; i++){
    compute_params(target_cell, i, nodes[i]);
  }

  // then compute corrected element parameters using perceptual measures
  _corrected_indices = _target_indices;
  _corrected_scales = _target_scales;
  _corrected_angles = _target_angles;
  _corrected_positions = _target_positions;
  unsigned int nb_edges = edges.size();
  vector<bool> processed_nodes;
  for (unsigned int i=0 ; i<nb_nodes ; i++){
    processed_nodes.push_back(false);
  }
  vector< pair<double, int> > sorted_edges;
  for (unsigned int i=0 ; i<nb_edges ; i++){
    double edge_len = _target_positions[edges[i].first].dist(_target_positions[edges[i].second]);
    sorted_edges.push_back(pair<double, int>(edge_len, i));
  }
  sort(sorted_edges.begin(), sorted_edges.end());

  for (unsigned int i=0 ; i<nb_edges ; i++){
    int edge_idx = sorted_edges[i].second;
    int node_idx1 = edges[edge_idx].first;
    int node_idx2 = edges[edge_idx].second;
    if (!processed_nodes[node_idx1] && !processed_nodes[node_idx2]){
      correct_params(target_cell, edges[edge_idx]);
      processed_nodes[node_idx1] = true;
      processed_nodes[node_idx2] = true;
    }
  }

  // and finally render strokes
  render_synthesized_strokes(target_cell);
}


void
StrokeGroup::compute_graph_1D(const GestureCell* target_cell, 
			      vector<UVpt>& nodes, vector< pair<int, int> >& edges) const{

  // initialise nodes with random positions 
  int nb_elements = (int)(target_cell->scale()/_proximity.avg);
  vector<double> cur_nodes;
  for (int i=0 ; i<nb_elements ; i++){
    double u_rand = (double)rand()/(double)RAND_MAX;
    cur_nodes.push_back(u_rand);
  }

  // build the Delaunay triangulation and compute the edge length mean and std
  sort(cur_nodes.begin(), cur_nodes.end());
  vector< pair<int, int> > cur_edges;
  for (int i=1 ; i<nb_elements ; i++){
    cur_edges.push_back(pair<int, int>(i, i-1));
  }

  vector< pair<int, int> > stat_edges;
  double target_prox_avg, target_prox_std;
  compute_graph_stats_1D(cur_nodes, cur_edges, stat_edges,
			 target_prox_avg, target_prox_std);

  // iterate Lloyd algorithm until we approximate the target mean and std
  double ref_ratio = _proximity.std/_proximity.avg;
  double target_ratio = target_prox_std/target_prox_avg;
  while (target_ratio>ref_ratio){

    // compute the voronoi diagram and move the nodes to the center of their voronoi region 
    vector< pair<double, double> > bounds;
    bounds.push_back(pair<double,double>(0.0, (cur_nodes[0]+cur_nodes[1])*0.5));
    for (int i=1 ; i<nb_elements-1 ; i++){
      double inf_bound = (cur_nodes[i-1]+cur_nodes[i])*0.5;
      double sup_bound = (cur_nodes[i+1]+cur_nodes[i])*0.5;
      bounds.push_back(pair<double, double>(inf_bound, sup_bound));
    }
    bounds.push_back(pair<double,double>((cur_nodes[nb_elements-2]+cur_nodes[nb_elements-1])*0.5, 1.0));
    for (int i=0 ; i<nb_elements ; i++){
      cur_nodes[i] = (bounds[i].first+bounds[i].second)*0.5;
    }

    // update the edge length mean and std (the triangulation is unchanged)
    stat_edges.clear();
    compute_graph_stats_1D(cur_nodes, cur_edges, stat_edges, 
			   target_prox_avg, target_prox_std);
    target_ratio = target_prox_std/target_prox_avg;
  }

  // and finally scale and copy the data
  double scale = _proximity.avg/(target_cell->scale()*target_prox_avg);
  double offset = 0.5*(1-scale);  

  unsigned int nb_nodes = cur_nodes.size();
  vector<double> proximity_measures;
  for (unsigned int i=0 ; i<nb_nodes ; i++){
    nodes.push_back(UVpt(cur_nodes[i]*scale+offset, 0.5));
    proximity_measures.push_back(1e19);
    edges.push_back(pair<int, int>(-1, -1));
  }

  // recompute nearest neighbors, this time in 2D
  unsigned int nb_cur_edges = cur_edges.size();
  for (unsigned int i=0 ; i<nb_cur_edges ; i++){
    int idx1 = cur_edges[i].first;
    int idx2 = cur_edges[i].second;
    double d = nodes[idx1].dist(nodes[idx2]);
    if (d<proximity_measures[idx1]){
      proximity_measures[idx1] = d;
      edges[idx1].first = idx1;
      edges[idx1].second = idx2;
    }
    if (d<proximity_measures[idx2]){
      proximity_measures[idx2] = d;
      edges[idx2].first = idx2;
      edges[idx2].second = idx1;
    }
  }

}


void
StrokeGroup::compute_graph_stats_1D(const vector<double>& nodes, const vector< pair<int,int> >& edges,
				    vector< pair<int,int> >& stat_edges, double& edge_length_avg, double& edge_length_std) const{
  unsigned int nb_nodes = nodes.size();
  vector<double> proximity_measures;
  for (unsigned int i=0 ; i<nb_nodes ; i++){
    proximity_measures.push_back(1e19);
    stat_edges.push_back(pair<int, int>(-1, -1));
  }

  unsigned int nb_edges = edges.size();
  for (unsigned int i=0 ; i<nb_edges ; i++){
    int idx1 = edges[i].first;
    int idx2 = edges[i].second;
    double d = abs(nodes[idx1] - nodes[idx2]);
    if (d<proximity_measures[idx1]){
      proximity_measures[idx1] = d;
      stat_edges[idx1].first = idx1;
      stat_edges[idx1].second = idx2;
    }
    if (d<proximity_measures[idx2]){
      proximity_measures[idx2] = d;
      stat_edges[idx2].first = idx2;
      stat_edges[idx2].second = idx1;
    }
  }
  
  double proximity_sum=0.0, proximity_norm=0.0;
  for (unsigned int i=0 ; i<nb_nodes ; i++){
    proximity_sum += proximity_measures[i];
    proximity_norm += 1.0;
  } 
  compute_avg_std(proximity_sum, proximity_norm, proximity_measures, 
		  edge_length_avg, edge_length_std);

}


void
StrokeGroup::compute_graph_2D(const GestureCell* target_cell, 
			      vector<UVpt>& nodes, vector< pair<int, int> >& edges) const{

  // initialise nodes with random positions 
  int nb_elements = (int)(target_cell->scale()*target_cell->scale()/(_proximity.avg*_proximity.avg));
  
  vector<UVpt> cur_nodes;
  for (int i=0 ; i<nb_elements ; i++){
    double u_rand = 2*(double)rand()/(double)RAND_MAX;
    double v_rand = (double)rand()/(double)RAND_MAX;
    cur_nodes.push_back(UVpt(u_rand, v_rand));
  }

  // build the Delaunay triangulation and compute the edge length mean and a std
  vector< pair<int, int> > cur_edges;
  vector<bool> border_edges;
  vector< vector<int> > node_edges;
  delaunay_edges(cur_nodes, cur_edges, border_edges, node_edges);

  vector< pair<int, int> > stat_edges;
  double target_prox_avg, target_prox_std;
  compute_graph_stats_2D(cur_nodes, cur_edges, stat_edges,
			 target_prox_avg, target_prox_std);
  
  // iterate Lloyd algorithm until we approximate the target mean and std
  double ref_ratio = _proximity.std/_proximity.avg;
  double target_ratio = target_prox_std/target_prox_avg;
  while (target_ratio>ref_ratio){

    // compute the voronoi regions and move the nodes to their centroids
    vector<UVpt> centroids;
    voronoi_centroids(cur_nodes, cur_edges, border_edges, node_edges, centroids);
    cur_nodes = centroids;
 
    // update the Delaunay triangulation and the edge length mean and std
    cur_edges.clear();
    border_edges.clear();
    node_edges.clear();
    delaunay_edges(cur_nodes, cur_edges, border_edges, node_edges);

    stat_edges.clear();
    compute_graph_stats_2D(cur_nodes, cur_edges, stat_edges, 
			   target_prox_avg, target_prox_std);
    double last_ratio = target_ratio;
    target_ratio = target_prox_std/target_prox_avg;
    if (target_ratio>last_ratio){
      cerr << "Pb: Lloyd diverged" << endl;
    }
  }

  // and finally scale and copy the data
  double scale = _proximity.avg/(target_cell->scale()*target_prox_avg);
  UVvec offset (0.5*(1-scale), 0.5*(1-scale));

  unsigned int nb_nodes = cur_nodes.size(); 
  for (unsigned int i=0 ; i<nb_nodes ; i++){
    nodes.push_back(cur_nodes[i]*scale+offset);
  }

  unsigned int nb_edges = stat_edges.size(); 
  for (unsigned int i=0 ; i<nb_edges ; i++){
      edges.push_back(stat_edges[i]);
  }

}


void
StrokeGroup::delaunay_edges(const vector<UVpt>& nodes, vector< pair<int,int> >& edges,
			    vector<bool>& border_edges, vector< vector<int> >& node_edges) const {
  triangulateio in, out;
  unsigned int nb_nodes = nodes.size();
  if (nb_nodes<3) return;

  // initialize input data
  in.numberofpoints = nb_nodes;
  in.pointlist = (REAL*) malloc(in.numberofpoints*2*sizeof(REAL));
  for (unsigned int i=0 ; i<nb_nodes ; i++){
    in.pointlist[2*i] = nodes[i][0];
    in.pointlist[2*i+1] = nodes[i][1];
  }
  in.pointmarkerlist = 0;
  in.numberofpointattributes = 0;
  in.pointattributelist = 0;
  in.numberofsegments = 0;
  in.numberofholes = 0;
  in.numberofregions = 0;
  in.regionlist = 0;

  // initialize output data
  out.pointlist = (REAL*) NULL;
  out.pointattributelist = (REAL*) NULL;
  out.pointmarkerlist = (int*) NULL;
  out.trianglelist = (int*) NULL;
  out.triangleattributelist = (REAL*) NULL;
  out.neighborlist = (int*) NULL;
  out.segmentlist = (int*) NULL;
  out.segmentmarkerlist = (int*) NULL;
  out.edgelist = (int*) NULL;
  out.edgemarkerlist = (int*) NULL;


  // compute triangulation and write result (edges and nodes)
  triangulate("zeQ", &in, &out, 0);
  int nb_edges = out.numberofedges;
  vector< vector< pair<double,int> > > tmp_node_edges(nb_nodes);
  for (int i=0 ; i<nb_edges ; i++){
    // first add edge and check if it is a border
    int idx1 = out.edgelist[2*i];
    int idx2 = out.edgelist[2*i+1];
    pair<int, int> edge (idx1, idx2);
    edges.push_back(edge);
    border_edges.push_back((out.edgemarkerlist[i]==1));

    // then add it to the node in increasing angle order
    UVvec edge_vec = (nodes[idx2]-nodes[idx1]).normalized();
    double scal = edge_vec*(UVvec(1.0, 0.0));
    double angle, angle_inv;
    if (edge_vec*UVvec(0.0, 1.0)>=0.0) {
      angle = acos(scal);
      angle_inv = angle+M_PI;
    } else {
      angle = 2*M_PI-acos(scal);
      angle_inv = angle-M_PI;
    }
    tmp_node_edges[idx1].push_back(pair<double, int>(angle, i));
    tmp_node_edges[idx2].push_back(pair<double, int>(angle_inv, i));
  }
  for (unsigned int i=0 ; i<nb_nodes ; i++){
    sort(tmp_node_edges[i].begin(), tmp_node_edges[i].end());
    unsigned int nb_incident_edges = tmp_node_edges[i].size();
    vector<int> incident_edges;
    for (unsigned int j=0 ; j<nb_incident_edges ; j++){
      int edge_idx = tmp_node_edges[i][j].second;
      incident_edges.push_back(edge_idx);
    }
    node_edges.push_back(incident_edges);
  }  
 
  // and finally free memory
  if (in.pointlist) free(in.pointlist);
  if (out.pointlist) free(out.pointlist);
  if (out.pointattributelist) free(out.pointattributelist);
  if (out.pointmarkerlist) free(out.pointmarkerlist);
  if (out.trianglelist) free(out.trianglelist);
  if (out.triangleattributelist) free(out.triangleattributelist);
  if (out.neighborlist) free(out.neighborlist);
  if (out.segmentlist) free(out.segmentlist);
  if (out.segmentmarkerlist) free(out.segmentmarkerlist);
  if (out.edgelist) free(out.edgelist);
  if (out.edgemarkerlist) free(out.edgemarkerlist); 
}


void
StrokeGroup::compute_graph_stats_2D(const vector<UVpt>& nodes, const vector< pair<int,int> >& edges,
				    vector< pair<int,int> >& stat_edges, double& edge_length_avg, double& edge_length_std) const{
  unsigned int nb_nodes = nodes.size();
  vector<double> proximity_measures;
  for (unsigned int i=0 ; i<nb_nodes ; i++){
    proximity_measures.push_back(1e19);
    stat_edges.push_back(pair<int, int>(-1, -1));
  }

  unsigned int nb_edges = edges.size();
  for (unsigned int i=0 ; i<nb_edges ; i++){
    int idx1 = edges[i].first;
    int idx2 = edges[i].second;
    double d = nodes[idx1].dist(nodes[idx2]);
    if (d<proximity_measures[idx1]){
      proximity_measures[idx1] = d;
      stat_edges[idx1].first = idx1;
      stat_edges[idx1].second = idx2;
    }
    if (d<proximity_measures[idx2]){
      proximity_measures[idx2] = d;
      stat_edges[idx2].first = idx2;
      stat_edges[idx2].second = idx1;
    }
  }
  
  double proximity_sum=0.0, proximity_norm=0.0;
  for (unsigned int i=0 ; i<nb_nodes ; i++){
    proximity_sum += proximity_measures[i];
    proximity_norm += 1.0;
  } 
  compute_avg_std(proximity_sum, proximity_norm, proximity_measures, 
		  edge_length_avg, edge_length_std);

}


void
StrokeGroup::voronoi_centroids(const vector<UVpt>& nodes, const vector< pair<int,int> >& edges,
			       const vector<bool>& border_edges, const vector< vector<int> >& node_edges,
			       vector<UVpt>& centroids) const {

  // for each node, compute the centroid of its voronoi region
  unsigned int nb_nodes = nodes.size();
  for(unsigned int i=0 ; i<nb_nodes ; i++){
    // first determine the region vertices
    vector<UVpt> v_vertices;
    vector<UVvec> v_bissec;
    unsigned int nb_incident_edges = node_edges[i].size();
    for (unsigned int j=0 ; j<nb_incident_edges ; j++){
      int edge_idx1 = node_edges[i][j];
      // compute voronoi vertex
      UVpt start1 = nodes[edges[edge_idx1].first];
      UVpt end1 = nodes[edges[edge_idx1].second];
      if (edges[edge_idx1].second==(int)i){
	UVpt tmp = start1;
	start1 = end1;
	end1 = tmp;
      }
      
      int jp1 = (j+1==nb_incident_edges) ? 0 : j+1;
      int edge_idx2 = node_edges[i][jp1];
      UVpt start2 = nodes[edges[edge_idx2].first];
      UVpt end2 = nodes[edges[edge_idx2].second];
      if (edges[edge_idx2].second==(int)i){
	UVpt tmp = start2;
	start2 = end2;
	end2 = tmp;
      }
      
      if ((end1-start1).perpend()*(end2-start2)<0){
	// infinite vertex
	UVpt mid_pt1 = (start1+end1)*0.5;
	UVvec bissector1 = (end1-start1).normalized().perpend();
	UVpt inf_vertex1 = mid_pt1+10.0*bissector1;
	v_vertices.push_back(inf_vertex1);

	UVpt mid_pt2 = (start2+end2)*0.5;
	UVvec bissector2 = (start2-end2).normalized().perpend();
	UVpt inf_vertex2 = mid_pt2+10.0*bissector2;
	v_vertices.push_back(inf_vertex2);

       } else {
	UVpt mid_pt1 = (start1+end1)*0.5;
	UVvec bissector1 = (end1-start1).normalized().perpend();
	UVline l1 (mid_pt1, bissector1);
	
	UVpt mid_pt2 = (start2+end2)*0.5;
	UVvec bissector2 = (start2-end2).normalized().perpend();
	UVline l2 (mid_pt2, bissector2);

	UVpt vertex = l1.intersect(l2);
	v_vertices.push_back(vertex);

      }
      v_bissec.push_back((end1-start1).normalized().perpend());
    }


    // then clip the region to the UV unit square and
    // compute the centroid of the cliped region
    double area = 0.0;
    UVpt centroid (0.0, 0.0);
    vector<UVpt> cliped_vertices;
    clip_poligon(v_vertices, cliped_vertices);
    unsigned int nb_cliped_vertices = cliped_vertices.size();
    for (unsigned int j=0 ; j<nb_cliped_vertices ; j++){
      int jp1 = (j+1==nb_cliped_vertices) ? 0 : j+1;
      double alpha = cliped_vertices[j][0]*cliped_vertices[jp1][1] - cliped_vertices[jp1][0]*cliped_vertices[j][1];
      area += alpha;
      centroid[0] += (cliped_vertices[j][0]+cliped_vertices[jp1][0])*alpha;
      centroid[1] += (cliped_vertices[j][1]+cliped_vertices[jp1][1])*alpha;
    }
    area *= 0.5;
    centroid /= 6*area;
    centroids.push_back(centroid);
  }

}






void 
StrokeGroup::compute_params(const GestureCell* target_cell, int node_idx, const UVpt& init_pos){
    
  // first pick a candidate that is not into history
  int nb_elements = _elements.size();
  bool in_history = true;
  int target_idx = _target_indices[node_idx];
  while (in_history){
    double random_val = (double)rand()/(double)RAND_MAX;
    target_idx = _elements[(int)(random_val*nb_elements)];
    list<int>::const_iterator iter;
    in_history = false;
    for (iter=_element_history.begin(); iter!=_element_history.end() && !in_history; iter++) {
      in_history = (target_idx==*iter);
    }
  }
  _element_history.push_back(target_idx);
  if (_element_history.size()>_elements.size()/2 ||
      (int)_element_history.size()>HISTORY_DEPTH){
    _element_history.pop_front();
  }
  _target_indices[node_idx] = target_idx;
  

  // compute scale
  double ref_length = _stroke_paths[target_idx]->axis_a().length();
  double ref_width = _stroke_paths[target_idx]->axis_b().length();
  double length_ratio, width_ratio;
  if (!_stretching_enabled){
    width_ratio = length_ratio = 1.0;
  } else if (_type==HATCHING){
    double target_length = sample_property(_length.avg, _length.std, target_idx, 
					   _length.measures, _length.min, _length.max);
    length_ratio = target_length/ref_length;
    double target_width = sample_property(_width.avg, _width.std, target_idx, 
					  _width.measures, _width.min, _width.max);
    width_ratio = target_width/ref_width;
  } else {
    double target_length = sample_property(_length.avg, _length.std, target_idx, 
					   _length.measures, _length.min, _length.max);
    length_ratio = target_length/ref_length;
    width_ratio = length_ratio;
  }
  _target_scales[node_idx] = UVvec(length_ratio, width_ratio);

  // compute angle
  double target_angle = 0.0;
  if (_type==HATCHING || _type==FREE){
    double orientation = sample_property(_orientation.avg, _orientation.std, target_idx, 
					 _orientation.measures, _orientation.min, _orientation.max);
    if (orientation<0.0){
      orientation = -orientation;
    } else if (orientation>1.0){
      orientation = 2.0-orientation;
    }
    // target_angle in [0..PI/2]
    target_angle = acos(orientation);
  }
  if (_reference_frame==ANGULAR){
    if (init_pos.dist(UVpt(0.5, 0.5))!=0.0){
      UVvec origin_to_node = (init_pos-UVpt(0.5, 0.5)).normalized();
      target_angle += UVvec(1.0, 0.0).signed_angle(origin_to_node);
    }
  }
  _target_angles[node_idx] = target_angle;



  // compute position
  UVpt target_pos = init_pos;
  if (_reference_frame==AXIS){
    double offset = sample_property(_offset.avg, _offset.std, target_idx, 
				    _offset.measures, _offset.min, _offset.max);
    target_pos += UVvec(0.0, offset/target_cell->scale());
  }
  _target_positions[node_idx] = target_pos;

}


void
StrokeGroup::correct_params(const GestureCell* target_cell, const pair<int, int>& edge){

  if (_behavior==CLONE){
    vector< pair<double, int> > prox_ordered_pairs;
    compute_ordered_pairs(_proximity.measures, prox_ordered_pairs);
    correct_params_clone(target_cell, edge, prox_ordered_pairs);
  } else if (_behavior==COPY){
    vector< pair<double, int> > prox_ordered_pairs;
    vector< pair<double, int> > par_ordered_pairs;
    vector< pair<double, int> > ov_ordered_pairs;
    vector< pair<double, int> > sep_ordered_pairs;
    compute_ordered_pairs(_proximity.measures, prox_ordered_pairs);
    if (_type==HATCHING){
      compute_ordered_pairs(_parallelism.measures, par_ordered_pairs);
      compute_ordered_pairs(_overlapping.measures, ov_ordered_pairs);
      compute_ordered_pairs(_separation.measures, sep_ordered_pairs);
    }
    correct_params_copy(target_cell, edge, prox_ordered_pairs, par_ordered_pairs, 
			ov_ordered_pairs, sep_ordered_pairs);
  } else { // _behavior==SAMPLE
    correct_params_sample(target_cell, edge);
  }  
}

void
StrokeGroup::compute_ordered_pairs(const vector<double>& measures, vector< pair<double, int> >& ordered_pairs){
  unsigned int nb_element_pairs = _element_pairs.size();
  for (unsigned int i=0 ; i<nb_element_pairs ; i++){
    ordered_pairs.push_back(pair<double, int>(measures[i], i));
  }
  sort(ordered_pairs.begin(), ordered_pairs.end());
}  

void
StrokeGroup::correct_params_clone(const GestureCell* target_cell, const pair<int, int>& edge,
				  vector< pair<double, int> >& prox_ordered_pairs){

  // first find the reference element pair with the closest proximity
  double cur_prox = _target_positions[edge.first].dist(
     _target_positions[edge.second])*target_cell->scale();
  vector< pair<double, int> >::const_iterator closest_prox_pair =
     lower_bound(prox_ordered_pairs.begin(), prox_ordered_pairs.end(), 
                 pair<double, int> (cur_prox, 0));
  int ref_edge_idx = (uint(closest_prox_pair->second) < _element_pairs.size()) ?
     closest_prox_pair->second : _element_pairs.size()-1; 

  // then pick an element
  _corrected_indices[edge.first] = _element_pairs[ref_edge_idx].first;

  if (_type==STIPPLING){
    // compute a corrected position
    double target_prox = _proximity.measures[ref_edge_idx];
    correct_position(target_cell, edge, target_prox);
  } else if (_type==HATCHING){
    // compute a corrected angle
    double target_par = _parallelism.measures[ref_edge_idx];
    correct_angle(edge, target_par, _corrected_indices[edge.first]);

    // compute a corrected position
    double l_proj_1, l_proj_2;
    UVvec ov_vec, sep_vec;
    get_overlapping_separation_params(target_cell, edge, l_proj_1, l_proj_2, ov_vec, sep_vec);
    double target_ov = _overlapping.measures[ref_edge_idx];
    double target_sep = _separation.measures[ref_edge_idx];
    correct_position(target_cell, edge, l_proj_1, l_proj_2, ov_vec, sep_vec, target_ov, target_sep);
  }
}


void
StrokeGroup::correct_params_copy(const GestureCell* target_cell, const pair<int, int>& edge,
				 vector< pair<double, int> >& prox_ordered_pairs,
				 vector< pair<double, int> >& par_ordered_pairs,
				 vector< pair<double, int> >& ov_ordered_pairs,
				 vector< pair<double, int> >& sep_ordered_pairs){

  // first find the reference element pair with the closest proximity
  double cur_prox = _target_positions[edge.first].dist(
     _target_positions[edge.second])*target_cell->scale();
  vector< pair<double, int> >::const_iterator closest_prox_pair =
     lower_bound(prox_ordered_pairs.begin(), prox_ordered_pairs.end(), 
                 pair<double, int> (cur_prox, 0));
  int ref_edge_idx = (uint(closest_prox_pair->second) < _element_pairs.size()) ?
     closest_prox_pair->second : _element_pairs.size()-1; 

  // then pick an element
  _corrected_indices[edge.first] = _element_pairs[ref_edge_idx].first;
 
  if (_type==STIPPLING){
    // then compute a corrected position
    double target_prox = _proximity.measures[ref_edge_idx];
    correct_position(target_cell, edge, target_prox);
  } else  if (_type==HATCHING){
    // compute a corrected angle
    double cur_par = abs(cos(_target_angles[edge.first]-_target_angles[edge.second]));
    vector< pair<double, int> >::const_iterator closest_par_pair =
       lower_bound(par_ordered_pairs.begin(), par_ordered_pairs.end(), 
                   pair<double, int> (cur_par, 0));
    int ref_par_idx = (uint(closest_par_pair->second) < _parallelism.measures.size()) ?
       closest_par_pair->second : _parallelism.measures.size()-1;
    double target_par = _parallelism.measures[ref_par_idx];
    correct_angle(edge, target_par, _corrected_indices[edge.first]);

    // compute a corrected position
    double l_proj_1, l_proj_2;
    UVvec ov_vec, sep_vec;
    get_overlapping_separation_params(target_cell, edge, l_proj_1, l_proj_2, ov_vec, sep_vec);
    double cur_ov = ov_vec.length();
    vector< pair<double, int> >::const_iterator closest_ov_pair =
       lower_bound(ov_ordered_pairs.begin(), ov_ordered_pairs.end(), 
                   pair<double, int> (cur_ov, 0));
    int ref_ov_idx = (uint(closest_ov_pair->second) < _overlapping.measures.size()) ?
       closest_ov_pair->second : _overlapping.measures.size()-1;
    double target_ov = _overlapping.measures[ref_ov_idx];

    double cur_sep = sep_vec.length();
    vector< pair<double, int> >::const_iterator closest_sep_pair =
       lower_bound(sep_ordered_pairs.begin(), sep_ordered_pairs.end(), 
                   pair<double, int> (cur_sep, 0));
    int ref_sep_idx = (uint(closest_sep_pair->second) < _separation.measures.size()) ?
       closest_sep_pair->second : _separation.measures.size()-1;
    double target_sep = _separation.measures[ref_sep_idx];
    correct_position(target_cell, edge, l_proj_1, l_proj_2, ov_vec, sep_vec, target_ov, target_sep);
  }
}


void
StrokeGroup::correct_params_sample(const GestureCell* target_cell, const pair<int, int>& edge){

  // the picked element stays the same
  _corrected_indices[edge.first] = _target_indices[edge.first];

   if (_type==STIPPLING){
     // compute a corrected position
     double target_prox = sample(_proximity.avg, _proximity.std);
     correct_position(target_cell, edge, target_prox);
   } else if (_type==HATCHING){
     // compute a corrected angle
     double target_par = sample(_parallelism.avg, _parallelism.std);
     correct_angle(edge, target_par, _corrected_indices[edge.first]);

     // compute a corrected position
    double l_proj_1, l_proj_2;
    UVvec ov_vec, sep_vec;
    get_overlapping_separation_params(target_cell, edge, l_proj_1, l_proj_2, ov_vec, sep_vec);
    double target_ov = sample(_overlapping.avg, _overlapping.std);
    double target_sep = sample(_separation.avg, _separation.std);
    correct_position(target_cell, edge, l_proj_1, l_proj_2, ov_vec, sep_vec, target_ov, target_sep);
 }
}

void 
StrokeGroup::get_overlapping_separation_params(const GestureCell* target_cell, const pair<int, int>& edge, double& l_proj_1, double& l_proj_2,
					       UVvec& ov_vec, UVvec& sep_vec){
  // first retrieve a center and an axis for each synthesized path
  UVpt c1 = _target_positions[edge.first];
  UVvec axis1 = UVvec(cos(_target_angles[edge.first]), sin(_target_angles[edge.first]));
  axis1 *= _target_scales[edge.first][0]*_stroke_paths[_target_indices[edge.first]]->axis_a().length()/target_cell->scale();

  UVpt c2 = _target_positions[edge.second];
  UVvec axis2 = UVvec(cos(_target_angles[edge.second]), sin(_target_angles[edge.second]));
  axis2 *= _target_scales[edge.second][0]*_stroke_paths[_target_indices[edge.second]]->axis_a().length()/target_cell->scale();

  // then compute the virtual line between synthesized paths
  double l1 = axis1.length();
  double l2 = axis2.length();
  UVpt virtual_center = (l1*c1+l2*c2)/(l1+l2);
  UVvec virtual_axis = (axis1+axis2).normalized();
  UVline virtual_line (virtual_center, virtual_axis);

  // then compute their projected lengths
  UVpt proj_start1 = virtual_line.project(c1-axis1*0.5);
  UVpt proj_end1 = virtual_line.project(c1+axis1*0.5);
  UVpt proj_start2 = virtual_line.project(c2-axis2*0.5);
  UVpt proj_end2 = virtual_line.project(c2+axis2*0.5);
  l_proj_1 = (proj_end1-proj_start1).length();
  l_proj_2 = (proj_end2-proj_start2).length();

  // and finally compute the overlapping and separation vectors
  ov_vec = virtual_line.project(c1)-virtual_line.project(c2);
  UVline perp_virtual_line (virtual_center, virtual_axis.perpend());
  sep_vec = perp_virtual_line.project(c2)-perp_virtual_line.project(c1);

}


void
StrokeGroup::correct_position(const GestureCell* target_cell, const pair<int, int>& edge, 
			      double target_prox){
  // first retrieve synthesized elements positions
  UVpt cur_pos = _target_positions[edge.first];
  UVpt cur_neighbor_pos = _target_positions[edge.second];
  
  // then compute current and target proximity distances
  UVvec edge_vec = (cur_pos-cur_neighbor_pos);
  double cur_prox_dist = edge_vec.length();
  double target_prox_dist = target_prox/target_cell->scale();

  // and finally correct position to match target proximity distance
  UVpt corrected_pos = cur_pos + edge_vec.normalized()*(target_prox_dist-cur_prox_dist);  
  _corrected_positions[edge.first] = corrected_pos;
}


void
StrokeGroup::correct_angle(const pair<int, int>& edge, double target_par, int corrected_idx){
  // first retrieve synthesized elements angles
  double cur_angle = _target_angles[edge.first];
  if (cur_angle>M_PI/2){
    cur_angle = M_PI - cur_angle;
  }
  double cur_neighbor_angle = _target_angles[edge.second];
  if (cur_neighbor_angle>M_PI/2){
    cur_neighbor_angle = M_PI - cur_neighbor_angle;
  }
  
  // then compute current and target parallelism angle
  double cur_par_angle = (cur_angle>cur_neighbor_angle) ? 2*(cur_angle-cur_neighbor_angle)/M_PI : 2*(cur_neighbor_angle-cur_angle)/M_PI;
  double target_par_angle = target_par;
  if (target_par_angle<0.0) {
    target_par_angle = -target_par_angle;
  } else if (target_par_angle>1.0) {
    target_par_angle = 2.0-target_par_angle;
  }
 
  // and finally correct angle to match target parallelism angle
  double dir_sign = (cur_angle>cur_neighbor_angle) ? 1 : -1;
  double corrected_angle = cur_angle + dir_sign*(target_par_angle-cur_par_angle)*M_PI*0.5;
  _corrected_angles[edge.first] = corrected_angle;
}


void
StrokeGroup::correct_position(const GestureCell* target_cell, const pair<int, int>& edge, 
			      double l_proj_1, double l_proj_2, CUVvec& ov_vec, CUVvec& sep_vec, 
			      double target_ov, double target_sep){
  // first retrieve synthesized elements positions
  UVpt cur_pos = _target_positions[edge.first];

  // then compute current and target overlapping ratios
  double cur_ov_ratio = 2*ov_vec.length()/(l_proj_1+l_proj_2);
  double target_ov_ratio = target_ov;

  
  // then compute current and target separation distances
  double cur_sep_dist = sep_vec.length();
  double target_sep_dist = target_sep/target_cell->scale();


  // and finally correct position to match target overlapping and separation distances
  UVpt corrected_pos = cur_pos;
  corrected_pos += ov_vec.normalized()*(target_ov_ratio-cur_ov_ratio) * 0.5 * (l_proj_1+l_proj_2);
  corrected_pos += sep_vec.normalized()*(cur_sep_dist-target_sep_dist);

  _corrected_positions[edge.first] = corrected_pos;
}


bool 
StrokeGroup::valid_uv(CUVpt& pos) {
  return (pos[0]>=0 && pos[0]<=1 && pos[1]>=0 && pos[1]<=1);
}


void
StrokeGroup::clip_poligon(const vector<UVpt>& vertices, vector<UVpt>& cliped_vertices) {
  UVline left_line(UVpt(0.0, 0.0), UVpt(0.0, 1.0));
  vector<UVpt> left_cliped_vertives;
  sutherland_hodgman_polygon_clip (vertices, left_line, 0, left_cliped_vertives);

  UVline bottom_line(UVpt(0.0, 0.0), UVpt(1.0, 0.0));
  vector<UVpt> bottom_cliped_vertives;
  sutherland_hodgman_polygon_clip (left_cliped_vertives, bottom_line,1,  bottom_cliped_vertives);

  UVline right_line(UVpt(1.0, 0.0), UVpt(1.0, 1.0));
  vector<UVpt> right_cliped_vertives;
  sutherland_hodgman_polygon_clip (bottom_cliped_vertives, right_line, 2, right_cliped_vertives);
  
  UVline top_line(UVpt(0.0, 1.0), UVpt(1.0, 1.0));
  sutherland_hodgman_polygon_clip (right_cliped_vertives, top_line, 3, cliped_vertices);  
}


void
StrokeGroup::sutherland_hodgman_polygon_clip(const vector<UVpt>& vertices, 
					     const UVline& line, int line_idx,
					     vector<UVpt>& cliped_vertices) {
  if (vertices.size()==0) return;

  UVpt prev = vertices.back();
  unsigned int nb_vertices = vertices.size();
  for (unsigned int i=0; i<nb_vertices; i++){
    UVpt cur = vertices[i];
    if (inside(cur, line_idx)){
      if (inside(prev, line_idx)){
	cliped_vertices.push_back(cur);
      } else { 
	cliped_vertices.push_back(line.intersect(UVline(prev, cur)));
	cliped_vertices.push_back(cur);
      }
    } else {
      if (inside(prev, line_idx)) {
	cliped_vertices.push_back(line.intersect(UVline(prev, cur)));
      }
    }
    prev = cur;
  }
}

double 
StrokeGroup::sample_property(double avg, double std, int same_idx,
			     const vector<double>& measures, double min, double max) const{
  double prop_val;
  if (_behavior==CLONE){
    prop_val = measures[same_idx];
  } else if (_behavior==COPY){
    double random_val = (double)rand()/(double)RAND_MAX;
    unsigned int nb_measures = measures.size();
    int target_idx = (int)(random_val*nb_measures);
    prop_val = measures[target_idx];
  } else { // _behavior == SAMPLE
    prop_val = sample(avg, std);
  }

  // XXX: is it really useful ?
  if (prop_val<min) {
    prop_val = min;
  } else if (prop_val>max){
    prop_val = max;
  }

  return prop_val;
}




