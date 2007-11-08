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
#include "pattern/stroke_path.H"
#include "pattern/gesture_stroke.H"
#include "pattern/eigen_solver.H"
#include "stroke/base_stroke.H"

using namespace mlib;


// debug
#include "geom/world.H"

///////////////////
// constructor
///////////////////

StrokePaths::StrokePaths(double eps, double sa, int type, bool anal_style, const GestureStroke& stroke)
  : _epsilon(eps), _style_adjust(sa),
    _center(stroke.gesture()->center()), 
    _axis_a(0.0, 0.0), _axis_b(0.0, 0.0),
    _isotropic_confidence(1.0), 
    _anisotropic_confidence(1.0),
    _analyze_style(anal_style){
  // first try to find a matching type
  if (type==POINT && match_point(stroke)){
    _type = POINT;
  } else if (type==LINE && match_line(stroke)){
    _type = LINE;
  } else if (type==BBOX && match_bbox(stroke)){
    _type = BBOX;
  } else {
    _type = INVALID;
  }

  // then clone the stroke and add it to the path
  GestureStroke cloned_stroke = stroke;
  _strokes.push_back(cloned_stroke);
}


bool
StrokePaths::match_point(const GestureStroke& stroke){
  CGESTUREptr& gest = stroke.gesture();
  double max_dist = gest->spread();
  if (gest->pts().num()==1){
    _axis_a = VEXEL(1.0, 0.0)*max_dist;
  } else {
    _axis_a = gest->endpt_vec().normalized()*max_dist;
  }
  _axis_b = _axis_a.normalized().perpend()*max_dist;
  return max_dist<_epsilon*0.5;
}


bool
StrokePaths::match_line(const GestureStroke& stroke){
  PIXEL start = stroke.gesture()->start();
  PIXEL end = stroke.gesture()->end();
  VEXEL ends_vec = end-start;
  CPIXEL_list pts = stroke.gesture()->pts();
  int nb_pts = pts.num();

  // first compute the center and principal axis 
  double proj_min=0.0, proj_max=1.0;
  for (int i=1 ; i<nb_pts-1 ; i++){
    VEXEL pt_vec = pts[i]-start;
    double proj = pt_vec.normalized()*ends_vec.normalized()*pt_vec.length()/ends_vec.length();
    if (proj<proj_min){
      proj_min = proj;
      start = pts[i];
    } else if (proj>proj_max){
      proj_max = proj;
      end = pts[i];
    }
  }
  _center = (start+end)*0.5;
  _axis_a = end-start;

  // then compute the secondary axis
  double pos_dist=0.0, neg_dist=0.0;
  bool match = true;
  PIXELline line_a (_center, _axis_a);
  for (int i=0 ; i<nb_pts && match; i++){
    VEXEL proj_vec = pts[i]-line_a.project(pts[i]);
    double sign_dist;
    if (_axis_a.normalized().perpend()*proj_vec>0){
      sign_dist = proj_vec.length();
    } else {
      sign_dist = -proj_vec.length();
    }

    if (sign_dist>pos_dist){
      pos_dist = sign_dist;
    } else if (sign_dist<neg_dist){
      neg_dist = sign_dist;
    }

    if (pos_dist-neg_dist > _epsilon){
      match = false;
    }

  } 
  _axis_b = _axis_a.normalized().perpend()*(pos_dist-neg_dist);
  return match;
}


bool
StrokePaths::match_bbox(const GestureStroke& stroke){
  // 1- Extract the pixels constituing the stroke
  get_stroke_pts(stroke);

  // 2- Fit a gaussian distribution to the set of points
  fit_gaussian(_pts, _center, _axis_a, _axis_b);

  // 3- Adjust the bbox center
  adjust_bbox(_pts, _center, _axis_a, _axis_b);
  return true;
}

void 
StrokePaths::get_stroke_pts(const GestureStroke& stroke){
  if (_analyze_style){
    BaseStroke* stroke_proto = stroke.drawer()->base_stroke_proto();
    double width = 1.0;
     if (stroke_proto){
      width = stroke_proto->get_width();
    }

    CPIXEL_list& gest_pts = stroke.gesture()->pts();
    CARRAY<double>& gest_press = stroke.gesture()->pressures();
    int nb_gest_pts = gest_pts.num();
    for (int i=0 ; i<nb_gest_pts-1 ; i++){
      VEXEL pts_dir = gest_pts[i+1]-gest_pts[i];
      if (pts_dir.length()!=0.0){
	double thickness = width * ((stroke_proto->get_press_vary_width())?gest_press[i]:1.0);
	_pts += gest_pts[i]+pts_dir.normalized().perpend()*thickness*0.5;
	_pts += gest_pts[i]-pts_dir.normalized().perpend()*thickness*0.5;
      }
    }
  } else {
    _pts = stroke.gesture()->pts();
    
    // add one or two  points to deal with single point or
    // straight line singularities
    if (_pts.num()==1){
      _pts += _pts[0]+0.5*VEXEL(1.0, 0.0);
    }
    VEXEL endpt_vec = (_pts[_pts.num()-1]-_pts[0]);
    _pts += _pts[0] + endpt_vec*0.5 + endpt_vec.normalized().perpend()*0.5;
  }
}



void
StrokePaths::fit_gaussian(CPIXEL_list& pts, PIXEL& center,
			  VEXEL& axis_a, VEXEL& axis_b){
  // compute the mean of the points
  center = pts[0];
  int nb_pts = pts.num();
  for (int i = 1; i < nb_pts; i++){
    center += pts[i];
  }
  double fInvQuantity = 1.0/nb_pts;
  center *= fInvQuantity;

  // compute the covariance matrix of the points
  double fSumXX = 0.0, fSumXY = 0.0, fSumYY = 0.0;
  for (int i = 0; i < nb_pts; i++) {
    VEXEL kDiff = pts[i] - center;
    fSumXX += kDiff[0]*kDiff[0];
    fSumXY += kDiff[0]*kDiff[1];
    fSumYY += kDiff[1]*kDiff[1];
  }
  fSumXX *= fInvQuantity;
  fSumXY *= fInvQuantity;
  fSumYY *= fInvQuantity;

  // compute eigen vectors and eigen values
  VEXEL eigen_vec1, eigen_vec2;
  double eigen_val1, eigen_val2;
  EigenSolver eg(fSumXX, fSumXY, fSumYY);
  eg.solve(eigen_vec1, eigen_val1, eigen_vec2, eigen_val2);
  axis_a = eigen_vec1*eigen_val1;
  axis_b = eigen_vec2*eigen_val2;
}

void
StrokePaths::adjust_bbox(CPIXEL_list& pts, PIXEL& center,
			 VEXEL& axis_a, VEXEL& axis_b){
  // Let C be the box center and let U0 and U1 be the box axes.  Each
  // input point is of the form X = C + y0*U0 + y1*U1.  The following code
  // computes min(y0), max(y0), min(y1), max(y1), min(y2), and max(y2).
  // The box center is then adjusted to be
  //   C' = C + 0.5*(min(y0)+max(y0))*U0 + 0.5*(min(y1)+max(y1))*U1


  double fY0Min, fY0Max;
  double fY1Min, fY1Max;

  axis_a = axis_a.normalized();
  axis_b = axis_b.normalized();
  VEXEL kDiff = pts[0] - center;
  fY0Max = fY0Min = kDiff*axis_a;
  fY1Max = fY1Min = kDiff*axis_b;
  
  int nb_pts = pts.num();
  for (int i = 1; i < nb_pts; i++){
    kDiff = pts[i] - center;
    
    double fY0 = kDiff * axis_a;
    if (fY0 < fY0Min) {
      fY0Min = fY0;
    } else if (fY0 > fY0Max) {
      fY0Max = fY0;
    }

    double fY1 = kDiff * axis_b;
    if (fY1 < fY1Min) {
      fY1Min = fY1;
    }
    else if (fY1 > fY1Max) {
      fY1Max = fY1;
    }
  }

  center += 0.5*(fY0Min+fY0Max)*axis_a  + 0.5*(fY1Min+fY1Max)*axis_b;
  axis_a *= (fY0Max - fY0Min);
  axis_b *= (fY1Max - fY1Min)*_style_adjust;
  
}



////////////////
// accessors
////////////////

bool
StrokePaths::cluster_stippling(StrokePaths* path){
  if (_type!=POINT || path->type()!=POINT){
    return false;
  }

  double max_dist = _center.dist(path->center()) + _axis_a.length()*0.5 + path->axis_a().length()*0.5;
  if (max_dist>=_epsilon){ 
    return false;
  }

  // cluster strokes
  cluster_strokes(path);

  // update path parameters
  _center = (_center+path->center())*0.5;
  _axis_a = (_axis_a+path->axis_a()).normalized()*max_dist;
  _axis_b = _axis_a.normalized().perpend()*max_dist;

  return true;  
}


bool
StrokePaths::cluster_hatching(StrokePaths* path){
  if (_type!=LINE || path->type()!=LINE){
    return false;
  }

  // first compute the virtual line
  PIXEL v_center = (_center+path->center())*0.5;
  VEXEL v_dir;
  if (_axis_a.length()<_epsilon && path->axis_a().length()<_epsilon){
    v_dir = (_center-path->center()).normalized();
  } else if (_axis_a.length()<_epsilon) {
    v_dir = path->axis_a().normalized();
  } else if (path->axis_a().length()<_epsilon) {
    v_dir = _axis_a.normalized();
  } else {
    VEXEL axis2 = (_axis_a*path->axis_a() >= 0) ? path->axis_a() : -path->axis_a();
    v_dir = (_axis_a+axis2).normalized();
  }
  PIXELline v_line (v_center, v_dir);

  // then project extremities and compute projected distances
  bool cur_is_pos;
  double neg_dist=0.0, pos_dist=0.0, cur_dist;
  PIXEL start1 = _center-_axis_a*0.5;
  PIXEL start1_proj = v_line.project(start1);
  cur_dist = start1_proj.dist(start1) /*+ _axis_b.length()*0.5*/;
  cur_is_pos = (v_dir.perpend()*(start1-v_center)>=0);
  if (cur_is_pos && cur_dist>pos_dist){
    pos_dist = cur_dist;
  } else if (!cur_is_pos && cur_dist>neg_dist){
    neg_dist = cur_dist;
  }
  PIXEL end1 = _center+_axis_a*0.5;
  PIXEL end1_proj = v_line.project(end1);
  cur_dist = end1_proj.dist(end1) /*+ _axis_b.length()*0.5*/;
  cur_is_pos = (v_dir.perpend()*(end1-v_center)>=0);
  if (cur_is_pos && cur_dist>pos_dist){
    pos_dist = cur_dist;
  } else if (!cur_is_pos && cur_dist>neg_dist){
    neg_dist = cur_dist;
  }

  PIXEL start2 = path->center()-path->axis_a()*0.5;
  PIXEL start2_proj = v_line.project(start2);
  cur_dist = start2_proj.dist(start2) /*+ path->axis_b().length()*0.5*/;
  cur_is_pos = (v_dir.perpend()*(start2-v_center)>=0);
  if (cur_is_pos && cur_dist>pos_dist){
    pos_dist = cur_dist;
  } else if (!cur_is_pos && cur_dist>neg_dist){
    neg_dist = cur_dist;
  }
  
  PIXEL end2 = path->center()+path->axis_a()*0.5;
  PIXEL end2_proj = v_line.project(end2);
  cur_dist = end2_proj.dist(end2) /*+ path->axis_b().length()*0.5*/;
  cur_is_pos = (v_dir.perpend()*(end2-v_center)>=0);
  if (cur_is_pos && cur_dist>pos_dist){
    pos_dist = cur_dist;
  } else if (!cur_is_pos && cur_dist>neg_dist){
    neg_dist = cur_dist;
  }
  
  // validate clustering
  if (pos_dist+neg_dist >= _epsilon) {
    return false;
  }

  pair<PIXEL, PIXEL> max_pair = longest_pair(start1_proj, end1_proj, start2_proj, end2_proj);
  VEXEL l_proj_vec = max_pair.first-max_pair.second;
  VEXEL line1_proj = end1_proj-start1_proj;
  VEXEL line2_proj = end2_proj-start2_proj;
  if (l_proj_vec.length()-(line1_proj.length()+line2_proj.length()) >= _epsilon) {
    return false;
  }
  
  // cluster strokes
  cluster_strokes(path);
  
  // update path parameters
  _center = (max_pair.first+max_pair.second)*0.5;
  _axis_a = l_proj_vec;
  _axis_b = _axis_a.normalized().perpend() * (pos_dist+neg_dist);
 
  return true;
}

pair<PIXEL, PIXEL>
StrokePaths::longest_pair(CPIXEL& start1, CPIXEL& end1, CPIXEL& start2, CPIXEL& end2){
  double l;
  pair<PIXEL, PIXEL> max_pair (start1, end1); 
  double max_length = (end1-start1).length();

  l = (end2-start2).length();
  if (l>max_length){
    max_length = l;
    max_pair = pair<PIXEL, PIXEL>(start2, end2);
  }

  l = (start2-start1).length();
  if (l>max_length){
    max_length = l;
    max_pair = pair<PIXEL, PIXEL>(start1, start2);
  }

  l = (end2-start1).length();
  if (l>max_length){
    max_length = l;
    max_pair = pair<PIXEL, PIXEL>(start1, end2);
  }

  l = (start2-end1).length();
  if (l>max_length){
    max_length = l;
    max_pair = pair<PIXEL, PIXEL>(end1, start2);
  }

  l = (end2-end1).length();
  if (l>max_length){
    max_length = l;
    max_pair = pair<PIXEL, PIXEL>(end1, end2);
  }
 
  return max_pair;
}


bool
StrokePaths::cluster_free(StrokePaths* path){  
  // 1- Check that proximity or overlapping/continuation are valid
  if (!match_proximity(path) && !match_continuation(path)){
    return false;
  }

  // 2- append pixels and strokes
  cluster_strokes(path);
  _pts += path->pts();

  // 3- Fit a gaussian distribution to the set of points
  fit_gaussian(_pts, _center, _axis_a, _axis_b);

  // 4- Adjust the bounding box center and axis 
  adjust_bbox(_pts, _center, _axis_a, _axis_b);

  return true;
}

bool 
StrokePaths::match_proximity(StrokePaths* path) const{
  double dist1 = hausdorff_distance(_center, _axis_a, _axis_b, 
				    path->center(), path->axis_a(), path->axis_b());
  if (dist1<_epsilon){
//     cerr << "prox match: dist=" << dist1 << endl;
    return true;
  }

  double dist2 = hausdorff_distance(path->center(), path->axis_a(), path->axis_b(),
				    _center, _axis_a, _axis_b );
  if (dist2<_epsilon){
//     cerr << "prox match: dist=" << dist2 << endl;
    return true;    
  }

  return false;
}


double
StrokePaths::hausdorff_distance(CPIXEL& center1, CVEXEL& axis_a1, CVEXEL& axis_b1,
				CPIXEL& center2, CVEXEL& axis_a2, CVEXEL& axis_b2) {

  // 1- compute corners of first bbox
  PIXEL corners1[4];
  corners1[0] = center1+0.5*axis_a1+0.5*axis_b1;
  corners1[1] = center1-0.5*axis_a1+0.5*axis_b1;
  corners1[2] = center1-0.5*axis_a1-0.5*axis_b1;
  corners1[3] = center1+0.5*axis_a1-0.5*axis_b1;

  // 2- compute edges of second bbox
  PIXELline edges2[4];
  edges2[0] = PIXELline(center2-0.5*axis_a2-0.5*axis_b2, axis_a2);
  edges2[1] = PIXELline(center2-0.5*axis_a2-0.5*axis_b2, axis_b2);
  edges2[2] = PIXELline(center2+0.5*axis_a2+0.5*axis_b2, -axis_a2);
  edges2[3] = PIXELline(center2+0.5*axis_a2+0.5*axis_b2, -axis_b2);

  // 3- for each corner, compute its distance to all edges and find the hausdorff distance
  double haus_dist = 0.0;
  for (int i=0 ; i<4 ; i++){
    if (inside_box(corners1[i], center2, axis_a2, axis_b2)){
      continue;
    }
    double min_dist = 1e19;
    for (int j=0 ; j<4 ; j++){
      double cur_dist = (corners1[i] - edges2[j].project_to_seg(corners1[i])).length();
      if (cur_dist<min_dist){
	min_dist = cur_dist;
      }
    }
    if (min_dist>haus_dist){
      haus_dist = min_dist;
    }
  }
 
  return haus_dist;
}

bool 
StrokePaths::inside_box(CPIXEL& pt, CPIXEL& center, CVEXEL& axis_a, CVEXEL& axis_b) {
  double proj_len;
  VEXEL kDiff = pt - center;
  proj_len = kDiff * axis_a.normalized();
  if (proj_len > axis_a.length()*0.5 || proj_len < -axis_a.length()*0.5) {
    return false;
  }
  proj_len = kDiff * axis_b.normalized();
  if (proj_len > axis_b.length()*0.5 || proj_len < -axis_b.length()*0.5) {
    return false;
  }
  return true;  
}


bool
StrokePaths::match_continuation(StrokePaths* path) const{
  if (collinear_pair(_center, _axis_a, _axis_b,
		     path->center(), path->axis_a(), path->axis_b())){
    //     cerr << "cont match" << endl;
    return true;
  }

  if (collinear_pair(_center, _axis_b, _axis_a,
		     path->center(), path->axis_a(), path->axis_b())){
//     cerr << "cont match" << endl;
    return true;
  }

  if (collinear_pair(path->center(), path->axis_a(), path->axis_b(),
		     _center, _axis_a, _axis_b)){
//     cerr << "cont match" << endl;
    return true;
  }

  if (collinear_pair(path->center(), path->axis_b(), path->axis_a(),
		     _center, _axis_a, _axis_b)){
//     cerr << "cont match" << endl;
    return true;
  }

  return false;
}

bool
StrokePaths::collinear_pair(CPIXEL& center1, CVEXEL& axis_a1, CVEXEL& axis_b1,
			    CPIXEL& center2, CVEXEL& axis_a2, CVEXEL& axis_b2) const{
  
  // 1- Test for parallelism
  if (axis_b1.length()>=_epsilon){
    return false;
  }

  PIXEL corners2[4];
  corners2[0] = center2+0.5*axis_a2+0.5*axis_b2;
  corners2[1] = center2-0.5*axis_a2+0.5*axis_b2;
  corners2[2] = center2-0.5*axis_a2-0.5*axis_b2;
  corners2[3] = center2+0.5*axis_a2-0.5*axis_b2;
  for (int i=0 ; i<4 ; i++){
    VEXEL kDiff = corners2[i] - center1;
    double proj_len = kDiff * axis_b1.normalized();
    if (abs(proj_len) > 0.5*_epsilon) {
      return false;
    }    
  }

  // 2- Test for overlapping
  double min_proj1 = -0.5*axis_a1.length();
  double max_proj1 = 0.5*axis_a1.length();
  double min_proj2 = 1e19;
  double max_proj2 = -1e19;
  for (int i=0 ; i<4 ; i++){
    VEXEL kDiff = corners2[i] - center1;
    double proj_len = kDiff * axis_a1.normalized();
    if (proj_len<min_proj2) {
      min_proj2 = proj_len;
    }
    if (proj_len>max_proj2){
      max_proj2 = proj_len;
    }
  }
  
  if (min_proj2<=max_proj1 && max_proj2>=min_proj1){
    return true;
  }

  // 3- Test for continuation
  double gap = (min_proj2>max_proj1) ? min_proj2-max_proj1 :  min_proj1-max_proj2;
  return gap<_epsilon;
}


void 
StrokePaths::cluster_strokes(StrokePaths* path){
  const vector<GestureStroke>& clustered_strokes = path->strokes();
  unsigned int nb_clustered_strokes = clustered_strokes.size();
  for (unsigned int i=0 ; i<nb_clustered_strokes ; i++){
    _strokes.push_back(clustered_strokes[i]);
  }
}

void 
StrokePaths::set_bbox(CBBOXpix& bbox){
  unsigned int nb_strokes = _strokes.size();
  for(unsigned int i=0; i<nb_strokes ; i++){
    _strokes[i].set_bbox(bbox);
  }    
}


////////////////
// processes
////////////////


void 
StrokePaths::copy(GestureCell* target_cell, CUVpt& offset, bool stretch) const {
  unsigned int nb_strokes = _strokes.size();
  for(unsigned int i=0; i<nb_strokes ; i++){
    _strokes[i].copy(target_cell, offset, stretch);
  }  
}


void 
StrokePaths::synthesize(GestureCell* target_cell, double target_pressure,
			CUVvec& target_scale, double target_angle, CUVpt& target_pos) const{

  // copy strokes, stretching them if necessary
  unsigned int nb_strokes = _strokes.size();
  double ref_angle; 
  if (_type==LINE || _type==BBOX){
    VEXEL main_dir = (_axis_a*VEXEL(0.0, 1.0)>=0) ? _axis_a : -_axis_a;
    ref_angle =  main_dir.angle(VEXEL(1.0, 0.0));
   } else {
    ref_angle = 0.0;
  }
  UVpt ref_pos(_center[0], _center[1]);
  for(unsigned int i=0; i<nb_strokes ; i++){
    _strokes[i].synthesize(target_cell, target_pressure, ref_angle, ref_pos,
			   target_scale, target_angle, target_pos);
  } 
}
