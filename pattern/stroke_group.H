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
#ifndef STROKE_GROUP_H
#define STROKE_GROUP_H

#include "disp/bbox.H"
#include "pattern/gesture_cell.H"
#include "mlib/points.H"
#include <vector>
#include <list>
//#include "pattern/stroke_path.H"

#ifdef SINGLE
#define REAL float
#else /* not SINGLE */
#define REAL double
#endif /* not SINGLE */
extern "C" {
#include "triangle/triangle.h"
}

class StrokePaths;


struct Property{
  std::vector<double> measures;
  double avg;
  double std;
  double min;
  double max;
};

class StrokeGroup{
public:
   // constructor / destructor
  StrokeGroup(int type, int ref_frame) 
    : _type(FREE), _reference_frame(ref_frame), _style_analyzed(true),
      _behavior(0), _distribution(0), _stretching_enabled(false), _correction_amount(1.0),
      _ring_nb(1), _synth_method(CAUSAL){}
  ~StrokeGroup();
  enum {HATCHING=0, STIPPLING, FREE};
  enum {PARALLEL=0, CAUSAL};
  enum {LLOYD=0, STRATIFIED};

  // accessors
  int nb_paths() const { return _stroke_paths.size();}
  void add_path(StrokePaths* path);
  const std::vector<StrokePaths*>& paths() const { return _stroke_paths; }
  const std::vector<int>& elements() const { return _elements; }
  const std::vector< std::pair<int, int> >& element_pairs() const { return _element_pairs; }
  const std::vector< std::vector<int> >& element_neighbors() const { return _element_neighbors; }
  void set_bbox(CBBOXpix& bbox);
  int type() const {return _type;}
  int reference_frame() const {return _reference_frame;}
  enum {AXIS=0, CARTESIAN, ANGULAR};
  void set_behavior(int b) { _behavior=b; }
  void set_distribution (int d) { _distribution = d;}
  enum {MIMIC=0, EFROS, SAMPLE, COPY, CLONE};
  void enable_stretching(bool b) { _stretching_enabled=b; }
  void set_ring_nb(int n) { _ring_nb = n; }
  void set_correction_amount(double amount) { _correction_amount = amount; }
  
  // processes
  void analyze(bool analyze_style);
  void copy(GestureCell* target_cell, mlib::CUVpt& offset, bool stretch) const;
  void synthesize_NN(GestureCell* target_cell) ;
  void synthesize_1ring(GestureCell* target_cell) ;
  void synthesize_Efros(GestureCell* target_cell) ;
  void render_synthesized_strokes(GestureCell* target_cell) const;

private:
  // analysis
  void compute_elements();
  void compute_element_neighbors();
  void compute_element_neighbors_1D();
  void compute_element_neighbors_2D();

  double get_angle(const StrokePaths* path) const;
  double get_orientation(const StrokePaths* path) const;
  double get_offset(const StrokePaths* path) const;

  double get_relative_position(const StrokePaths* sp1, const StrokePaths* sp2) const;
  double get_relative_length(const StrokePaths* sp1, const StrokePaths* sp2) const;
  double get_relative_width(const StrokePaths* sp1, const StrokePaths* sp2) const;
  double get_relative_orientation(const StrokePaths* sp1, const StrokePaths* sp2) const;

  double get_proximity(const StrokePaths* sp1, const StrokePaths* sp2) const;
  double get_parallelism(const StrokePaths* sp1, const StrokePaths* sp2) const;
  double get_overlapping(const StrokePaths* sp1, const StrokePaths* sp2) const ;
  double get_separation(const StrokePaths* sp1, const StrokePaths* sp2) const ;
  void get_projected_lengths(const StrokePaths* sp1, const StrokePaths* sp2,
			     double& l1, double& l2, double& l_proj) const;

  void push_stat(double measure, double confidence, std::vector<double>& measures, 
		 double& sum, double& norm, double& min, double& max);
  void compute_avg_std(double sum, double norm, const std::vector<double>& candidates,
		       double& avg, double& std) const;

  // synthesis
  
  void element_synthesis(GestureCell* target_cell);
  int  get_shape_idx(int prev_idx);
  void get_close_value(const double ref_val, double& target_val, 
		       const std::vector<double>& sorted_measures, bool periodic);
  void lloyd_1D(GestureCell* target_cell);
  void lloyd_2D(GestureCell* target_cell);
  void stratified_1D(GestureCell* target_cell);
  void stratified_2D(GestureCell* target_cell);
  bool degenerated_edge(int idx, const std::vector< std::pair<int, int> >& edges, const std::vector<mlib::UVpt>& node_pos,
			const std::vector<bool>& border_edges, const std::vector< std::vector<int> >& node_edges);

	void distribute_elements_1D(GestureCell* target_cell);
	void distribute_elements_2D(GestureCell* target_cell);


  void group_synthesis(GestureCell* target_cell);
  void compute_synthesized_neighborhoods_1D(std::vector< std::vector<int> >& neighborhoods);
  void compute_synthesized_neighborhoods_2D(std::vector< std::vector<int> >& neighborhoods);
  void compute_element_order_1D(const std::vector< std::vector<int> >& neighborhoods, std::vector<int>& ordered_elements);
  void compute_element_order_2D(const std::vector< std::vector<int> >& neighborhoods, std::vector<int>& ordered_elements);
  int get_ref_center();
  void get_sorted_parameters(std::vector<double>& sorted_lengths, std::vector<double>& sorted_widths, std::vector<double>& sorted_angles);
  void synthesize_element(int ref_idx, int target_idx, const std::vector< std::vector<int> >& target_neighbors, double target_scale,
			  const std::vector<double>& sorted_lengths, const std::vector<double>& sorted_widths, const std::vector<double>& sorted_angles);
  int find_best_match(int idx, const std::vector< std::vector<int> >& neighborhoods, double target_scale,
		      double proximity_threshold, double parallelism_threshold, double overlapping_threshold);
  void get_n_ring_neighbors(int idx, const std::vector< std::vector<int> >& neighborhoods, 
			    std::vector<int>& neighbors, double& max_dist);
  double compute_element_proximity(int ref_idx, int target_idx);
  double compute_element_parallelism(int ref_idx, int target_idx);
  double compute_element_overlapping(int ref_idx, int target_idx);
  double compute_element_superimposition(int ref_idx, int target_idx);
  double compute_element_color_diff(int ref_idx, int target_idx);

  double get_1ring_match(int target_center_idx, mlib::CUVpt& target_center, const std::vector<int>& target_neighbors, 
			 std::vector<int>& relevant_target_edges, double target_scale, 
			 int ref_center_idx, mlib::CPIXEL& ref_center, 
			 const std::vector<int>& ref_neighbors, std::vector<int>& relevant_ref_edges);
  double get_element_match(int target_idx, mlib::CUVpt& target_origin, int ref_idx, mlib::CPIXEL& ref_origin, double target_scale);
  mlib::PIXEL get_1ring_position(mlib::CPIXEL& center_pos, const std::vector<mlib::PIXEL>& neighbor_pos, std::vector<int>& relevant_edges);
  double get_1ring_angle(double center_angle, const std::vector<mlib::VEXEL>& neighbor_axis, std::vector<int>& relevant_edges);
  double get_1ring_length(double center_length, const std::vector<double>& neighbor_lengths, std::vector<int>& relevant_edges);
  double get_1ring_width(double center_width, const std::vector<double>& neighbor_widths, std::vector<int>& relevant_edges);

  // TODO: deprecate that
  void compute_graph_1D(const GestureCell* target_cell, 
			std::vector<mlib::UVpt>& nodes, 
			std::vector< std::pair<int, int> >& edges) const;
  void compute_graph_stats_1D(const std::vector<double>& nodes, const std::vector< std::pair<int,int> >& edges,
			      std::vector< std::pair<int,int> >& stat_edges, double& edge_length_avg, double& edge_length_std) const;
  void compute_graph_2D(const GestureCell* target_cell, 
			std::vector<mlib::UVpt>& nodes, 
			std::vector< std::pair<int, int> >& edges) const;
  void delaunay_edges(const std::vector<mlib::UVpt>& nodes, std::vector< std::pair<int,int> >& edges,
			 std::vector<bool>& border_edges, std::vector< std::vector<int> >& node_edges) const;
  void compute_graph_stats_2D(const std::vector<mlib::UVpt>& nodes, const std::vector< std::pair<int,int> >& edges,
			      std::vector< std::pair<int,int> >& stat_edges, double& edge_length_avg, double& edge_length_std) const;
  void voronoi_centroids(const std::vector<mlib::UVpt>& nodes, const std::vector< std::pair<int,int> >& edges,
			 const std::vector<bool>& border_edges, const std::vector< std::vector<int> >& node_edges,
			 std::vector<mlib::UVpt>& centroids) const;
  void compute_params(const GestureCell* target_cell, int node_idx, const mlib::UVpt& node_pos) ;
  void correct_params(const GestureCell* target_cell, const std::pair<int, int>& edge);
  void compute_ordered_pairs(const std::vector<double>& measures, std::vector< std::pair<double, int> >& ordered_pairs);
  void correct_params_sample(const GestureCell* target_cell, const std::pair<int, int>& edge);
  void correct_params_copy(const GestureCell* target_cell, const std::pair<int, int>& edge,
			   std::vector< std::pair<double, int> >& prox_ordered_pairs,
			   std::vector< std::pair<double, int> >& par_ordered_pairs,
			   std::vector< std::pair<double, int> >& ov_ordered_pairs,
			   std::vector< std::pair<double, int> >& sep_ordered_pairs);
  void correct_params_clone(const GestureCell* target_cell, const std::pair<int, int>& edge,
			    std::vector< std::pair<double, int> >& prox_ordered_pairs);
  void get_overlapping_separation_params(const GestureCell* target_cell, const std::pair<int, int>& edge, double& l_proj_1, double& l_proj_2,
					 mlib::UVvec& ov_vec, mlib::UVvec& sep_vec);
  void correct_position(const GestureCell* target_cell, const std::pair<int, int>& edge, double prox);
  void correct_angle(const std::pair<int, int>& edge, double par, int corrected_index);
  void correct_position(const GestureCell* target_cell, const std::pair<int, int>& edge, 
			double l_proj_1, double l_proj_2, mlib::CUVvec& ov_vec, mlib::CUVvec& sep_vec, 
			double target_ov, double target_sep);
  double sample_property(double avg, double std, int same_idx,
			 const std::vector<double>& measures, double min, double max) const;


  static double sample(double avg, double std);
  static double secant(double a);
  static double normal_distrib(double x);
  static bool valid_uv(mlib::CUVpt& pos);
  static void clip_poligon(const std::vector<mlib::UVpt>& vertices, std::vector<mlib::UVpt>& cliped_vertices);
  static void sutherland_hodgman_polygon_clip(const std::vector<mlib::UVpt>& vertices, const mlib::UVline& line, int line_idx,
					      std::vector<mlib::UVpt>& cliped_vertices);
  static bool inside(const mlib::UVpt& p, int l);

private:
  // analysis vars
  std::vector<StrokePaths*>     _stroke_paths;
  std::vector<int>              _elements;
  std::vector< pair<int, int> > _element_pairs;
  std::vector< std::vector<int> > _element_neighbors; 
  int _type;
  int _reference_frame;
  BBOXpix _bbox;

  // element properties
  Property _length;
  Property _width;
  Property _orientation;
  Property _offset;

  // element 1-ring properties
  std::vector<Property> _relative_position;
  std::vector<Property> _relative_length;
  std::vector<Property> _relative_width;
  std::vector<Property> _relative_orientation;
  
  // element pair properties
  Property _proximity;
  Property _parallelism;
  Property _overlapping;
  Property _separation;

  // synthesis params
  bool _style_analyzed;
  int _behavior;
  int _distribution;
  bool _stretching_enabled;
  double _correction_amount;
  int _ring_nb;
  int _synth_method;
  std::list<int> _element_history;
  static int HISTORY_DEPTH;
  static int MAX_COUNT;
  static double MAX_ANGLE;
  static double PROX_THRESHOLD_1D;
  static double PAR_THRESHOLD_1D;
  static double OV_THRESHOLD_1D;
  static double PROX_THRESHOLD_2D;
  static double PAR_THRESHOLD_2D;
  static double OV_THRESHOLD_2D;
  static double COLOR_THRESHOLD;

  // synthesized elements params
  std::vector<int> _target_indices;
  std::vector<mlib::UVvec> _target_scales;
  std::vector<double> _target_angles;
  std::vector<mlib::UVpt> _target_positions;

  std::vector<int> _corrected_indices;
  std::vector<mlib::UVvec> _corrected_scales;
  std::vector<double> _corrected_angles;
  std::vector<mlib::UVpt> _corrected_positions;
};


#endif // STROKE_GROUP_H
