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
    stroke_pattern.H
    
              StrokeGroup
              =    =    =
             /     +     \
    StipplingGroup | HatchingGroup
            +      |      +
            |      |      |
       Stipple     |     Hatch
             \     |    /
	      =    |   =
              StrokePaths
	           +
                   |
                Stroke

    (=:derives from, +:contains at least one)

    StrokePattern 
       - example stroke group container with XML save and load methods
       - analyse strokes drawn by the user as input in a UV space
       - synthesize new strokes in a target cell from the analysed 
         example stroke groups and a user-specified synthesis method
    StrokeGroup
       - generic StrokePaths container
    StipplingGroup
       - Stipple container with elements distributed evenly in UV space
    HatchingGroup
       - Hatch container with elements distributed evenly and parallely in UV space
    StrokePaths
       - generic StrokePaths or Stroke container
    Stipple
       - StrokePaths or Stroke container fitting in an epsilon point
    Hatch
       - StrokePaths or Stroke container fitting in an epsilon segment  
    Stroke
       - contains a gesture and a set of behavior labels (attribute, LOD, lighting, animation, etc)

    -------------------
    Pascal Barla
    Summer 2005
 ***************************************************************************/

#ifndef STROKE_PATTERN_H
#define STROKE_PATTERN_H

#include <vector>
#include "disp/bbox.H"
#include "pattern/gesture_cell.H"
#include "pattern/gesture_stroke.H"
#include "mlib/points.H"

class StrokeGroup;
class StrokePaths;

class StrokePattern{
public:
  // constructor / destructor
  StrokePattern() : _complete(true), _epsilon(0.0), _style_adjust(1.0), _analyze_style(true), _ring_nb(1), _correction_amount(1.0) {}
  StrokePattern(const StrokePattern& old_pattern, double new_eps);
  ~StrokePattern() { delete_groups(); }

  // accessors
  void      set_epsilon(double eps);
  void      set_style_adjust(double sa);
  void      set_analyze_style(bool b);
  void      set_ring_nb(int n) { _ring_nb = n; };
  void      set_correction_amount(double amount) { _correction_amount = amount; };
  void      add_group(int type, int ref_frame, const std::vector<GestureStroke>& strokes);
  void      clear()            { delete_groups(); _bbox.reset(); _complete=true; } 
  bool      empty() const      { return _stroke_groups.empty(); }
  const std::vector<StrokeGroup*>& groups() const { return _stroke_groups; }
  CBBOXpix& bbox() const       { return _bbox; }

  // processes
  void synthesize(int mode, int distrib, bool stretched, GestureCell* target_cell) ;
  void render_synthesized_strokes(GestureCell* target_cell);
  enum{SYNTH1, SYNTH2, SYNTH3, COPY, REPEAT, STRETCH};

  // file management
  void save(std::string name) const;
  void load(std::string name) const;

private:
  void delete_groups();
  void fit_singletons(const std::vector<GestureStroke>& strokes, int path_type,
		      std::vector<StrokePaths*>& singletons);
  void extract_elements(std::vector<StrokePaths*>& candidates,
			StrokeGroup* group);
private:
  std::vector<StrokeGroup*> _stroke_groups;
  BBOXpix                  _bbox;
  bool                     _complete;
  double                   _epsilon;
  double                   _style_adjust;
  bool                     _analyze_style;
  int                      _ring_nb;
  double                   _correction_amount;
};



#endif // STROKE_PATTERN_H
