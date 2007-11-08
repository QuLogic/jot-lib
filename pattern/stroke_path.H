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
#ifndef STROKE_PATH_H
#define STROKE_PATH_H

#include <vector>
#include <vector>
#include "disp/bbox.H"
#include "pattern/gesture_cell.H"
#include "pattern/gesture_stroke.H"
#include "mlib/points.H"

class StrokePaths{
public:
  // constructor / destructor
  StrokePaths(double epsilon, double style_adjust, int type, bool anal_style, const GestureStroke& stroke);
  virtual ~StrokePaths() {}

  // accessors
  virtual bool cluster_stippling(StrokePaths* path);
  virtual bool cluster_hatching(StrokePaths* path);
  virtual bool cluster_free(StrokePaths* path);
  virtual void set_bbox(CBBOXpix& bbox);

  const std::vector<GestureStroke>& strokes() const {return _strokes;}

  enum {POINT, LINE, BBOX, INVALID};
  double epsilon() const { return _epsilon; }
  int type() const { return _type;}

  mlib::CPIXEL& center() const { return _center;}
  mlib::CVEXEL& axis_a() const { return _axis_a;}
  mlib::CVEXEL& axis_b() const { return _axis_b;}
  mlib::CPIXEL_list& pts() const { return _pts; }
  double isotropic_confidence() const { return _isotropic_confidence;}
  double anisotropic_confidence() const { return _anisotropic_confidence;}
  
  // processes
  virtual void copy(GestureCell* target_cell,  mlib::CUVpt& offset,  bool stretch) const;
  virtual void synthesize(GestureCell* target_cell, double target_pressure,
			  mlib::CUVvec& target_scale, double target_angle, mlib::CUVpt& target_pos) const;

  static double hausdorff_distance(mlib::CPIXEL& center1, mlib::CVEXEL& axis_a1, mlib::CVEXEL& axis_b1,
				   mlib::CPIXEL& center2, mlib::CVEXEL& axis_a2, mlib::CVEXEL& axis_b2);
  static bool inside_box(mlib::CPIXEL& pt, mlib::CPIXEL& center, mlib::CVEXEL& axis_a, mlib::CVEXEL& axis_b);

private:
  // fitting
  bool match_point(const GestureStroke& stroke);
  bool match_line(const GestureStroke& stroke);
  bool match_bbox(const GestureStroke& stroke);
  void get_stroke_pts(const GestureStroke& stroke);
  void fit_gaussian(mlib::CPIXEL_list& pts, mlib::PIXEL& center, mlib::VEXEL& axis_a, mlib::VEXEL& axis_b);
  void adjust_bbox(mlib::CPIXEL_list& pts, mlib::PIXEL& center, mlib::VEXEL& axis_a, mlib::VEXEL& axis_b);

  // clustering
  std::pair<mlib::PIXEL, mlib::PIXEL> longest_pair(mlib::CPIXEL& start1, mlib::CPIXEL& end1, mlib::CPIXEL& start2, mlib::CPIXEL& end2);
  bool match_proximity(StrokePaths* path) const;
  bool match_continuation(StrokePaths* path) const;
  bool collinear_pair(mlib::CPIXEL& center1, mlib::CVEXEL& axis_a1, mlib::CVEXEL& axis_b1,
		      mlib::CPIXEL& center2, mlib::CVEXEL& axis_a2, mlib::CVEXEL& axis_b2) const;
  void cluster_strokes(StrokePaths* path);

private:
  std::vector<GestureStroke> _strokes;
  mlib::PIXEL_list _pts;
  
  double _epsilon;
  double _style_adjust;
  int _type;

  mlib::PIXEL _center;
  mlib::VEXEL _axis_a;
  mlib::VEXEL _axis_b;
  double      _isotropic_confidence;
  double      _anisotropic_confidence;
  bool _analyze_style;
};


#endif // STROKE_PATH_H
