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
    pattern_stroke.H
    
    PatternStroke
        -Stroke used to render Pattern3dStroke(pattern_group.c) in 2d
        -Derives from BaseStroke
        -Has check_vert_visibility Basestroke method to use visibility 
         check: if there is a face on the ref image(thus visible) that
       intersects with the location of the vertex, then draw it.
    -------------------
    Simon Breslav
    Fall 2004
 ***************************************************************************/

#if (defined (WIN32) && defined(_MSC_VER) && (_MSC_VER <=1300)) /*VS 6.0*/
#pragma warning(disable: 4786)
#endif

#include "pattern_stroke.H"

PatternStroke::PatternStroke(CNDCZvec_list& g) 
{   
    _pts = g;
}
void 
PatternStroke::draw(CNDCZpt& c)
{
   //cerr << "Stroke had " << _pts.num() << endl;
   glBegin(GL_LINE_STRIP);   
	      for(int i=0; i < _pts.num(); ++i)
         {
            CNDCZpt p = c + _pts[i]; 
            glVertex2dv(p.data());
         }
	glEnd();
}

/*
#include "gtex/ref_image.H" 
#include "geom/gl_view.H" 
#include "std/config.H"
#include "std/run_avg.H"

#include "mlib/statistics.H"
#include "stroke/base_stroke.H"
#include "mesh/bface.H"
#include <map>

#include "pattern_stroke.H"
#include "pattern_grid.H"
#include "pattern_texture.H"
#include "disp/colors.H"

using namespace mlib;

using std::map;

#define PARALLEL_STD_THRESH           0.15f
#define PARALLEL_MAX_MIN_DIFF_THRESH  0.25f
#define HATCHING_STRAIGHTNESS_THRESH  0.94f
#define HATCHING_SPREAD_THRESH        0.11f  
#define STIPPLING_SPREAD_THRESH       0.11f
#define STIPPLING_STROKE_SIM_DIFF     1.0f

#define COLOR_W       1.0f
#define DENSITY_W     1.0f
#define LENGTH_W      1.0f
#define DIRECTION_W   1.0f
static bool debug_stroke = Config::get_var_bool("DEBUG_STROKE",false,true);
static int foo = DECODER_ADD(PatternStroke);


bool PatternStroke::taggle_vis = false;
PatternStroke::PatternStroke() 
{   
    _vis_type = VIS_TYPE_SCREEN | VIS_TYPE_SUBCLASS;
    _verts.set_proto(new PatternVertexData);
    _draw_verts.set_proto(new PatternVertexData);
    _refine_verts.set_proto(new PatternVertexData);
}

BaseStroke* 
PatternStroke::copy() const
{
    PatternStroke *s =  new PatternStroke;
    assert(s);
    s->copy(*this);
    return s;
}
void			
PatternStroke::copy(CPatternStroke& s)
{
   BaseStroke::copy(s);
}


bool  
PatternStroke::check_vert_visibility(CBaseStrokeVertex &v)
{
    if(taggle_vis)     
       return true;
    
    CBface* f_id = ((PatternVertexData *)v._data)->_face;
    //assert(f_id);
    if(!f_id || !f_id->front_facing()) return false;
    
   
    static IDRefImage* id_ref       = 0;
    static uint        id_ref_stamp = UINT_MAX;
    // cache id ref image for the current frame
    
    if (id_ref_stamp != VIEW::stamp()) {
      IDRefImage::set_instance(VIEW::peek());
      id_ref = IDRefImage::instance();     

       id_ref_stamp = VIEW::stamp();
    }
    CBface* f = id_ref->intersect(v._base_loc);
    //CBface* f = VisRefImage::Intersect(v._base_loc);
    if(!f) return false;
    if(f != f_id){
      if(!f_id->is_quad()) 
         return false;   
      if(f != f_id->quad_partner()) 
         return false;   
    }
    return true;
}

// *****************************************************************
// * Pattern3dStroke
// *****************************************************************
Pattern3dStroke::Pattern3dStroke(QuadCell* cell,  
                                 CARRAY<CBface*> &faces,
                                 CWpt_list &pl,
                                 CARRAY<Wvec> &nl,
                                 CARRAY<Wvec>& bar,
                                 CARRAY<double>& alpha,
                                 CARRAY<double>& width,
                                 CUVpt_list& uv_p, 
                                 CBaseStrokeOffsetLISTptr &ol,
                                 BaseStroke * proto,
                                 double winding_, 
                                 double straightness_) : _cell(cell), 
                                                         _winding(winding_),
                                                         _straightness(straightness_)
{
   _stroke = new PatternStroke();
   _stroke->copy(*proto);
   assert(pl.num() == nl.num());
   _start_width = _stroke->get_width(); 
  
   _pts.clear();
   _pts +=(pl);
  
   _norms.clear();
   _norms +=(nl);

   _offsets = ol;

   _pix_size = VIEW::peek()->cam()->data()->from().dist(pl[pl.num()/2]);//at_length(pl[pl.num()/2], 1);

   //XXX - get a current light from the pen
   //_light_num = cell->pg()->get_current_light_num();   
   //if(VIEW::peek()->light_get_in_cam_space(_light_num))
   //   _light = cell->get_inv() * VIEW::peek()->cam()->data()->at_v();    
   //else 
   //   _light = cell->get_inv() * VIEW::peek()->light_get_coordinates_v(_light_num);   
   
   assert(_pts.num() == _norms.num());  
   _pts.update_length();

   double world_width = world_length(_pts.average(), _stroke->get_width());
   _area = _pts.length() * world_width;   
    
  
   _faces.clear();
   _faces += (faces);
   
   _bar.clear(); 
   _bar += bar;
   
   _alpha.clear();
   _alpha += alpha;
   
   _width.clear();
   _width += width;
  
   _uv_p.clear();                              
   _uv_p += uv_p;
   
   _avg_uv_pt = _uv_p.average();
   _offset_uv = uv_2_offsetUV(_avg_uv_pt, _uv_p);
    //err_msg("pts num %d and faces num %d",_pts.num(), _faces.num());

   _visible = true;
}
Pattern3dStroke::Pattern3dStroke(QuadCell* cell,
                                 double pix_size,
                                 double start_width,
                                 //Wvec light,
                                 //int light_num,
                                 CARRAY<CBface*> &faces,
                                 CWpt_list &pl,
                                 CARRAY<Wvec> &nl,
                                 CARRAY<Wvec>& bar,
                                 CARRAY<double>& alpha, 
                                 CARRAY<double>& width, 
                                 CUVpt_list& uv_p, 
                                 CBaseStrokeOffsetLISTptr &ol,
                                 BaseStroke * proto,
                                 double winding_,
                                 double straightness_) : _cell(cell),
                                                         _winding(winding_),
                                                         _straightness(straightness_)
{
   _stroke = new PatternStroke();
   _stroke->copy(*proto);
   assert(pl.num() == nl.num());
   _start_width = start_width;

   _pts.clear();
   _pts += (pl);

   _norms.clear();
   _norms +=(nl);

   _offsets = ol;

   _pix_size = pix_size;
   
   //_light = light;
   //_light_num = light_num;
   assert(_pts.num() == _norms.num());

   _pts.update_length();

   double world_width = world_length(_pts.average(), _stroke->get_width());
   _area = _pts.length() * world_width;
   
   _faces.clear();
   _faces +=(faces);
   
    _bar.clear(); 
   _bar += bar;
   
   _alpha.clear();
   _alpha += alpha;
   
   _width.clear();
   _width += width;
  
   _uv_p.clear();                              
   _uv_p += uv_p;
   
   _avg_uv_pt = _uv_p.average();
   _offset_uv = uv_2_offsetUV(_avg_uv_pt, _uv_p);
   
  // err_msg("pts num %d and faces num %d",_pts.num(), _faces.num());

   _visible = true;
}
Pattern3dStroke::~Pattern3dStroke()
{  delete _stroke; }

void
Pattern3dStroke::draw_start()
{
   if (_visible && _stroke){
      _stroke->draw_start();
   }
}

int
Pattern3dStroke::draw(CVIEWptr &v)
{
   Wpt_list bla;   
   for(int i=0; i < _faces.num(); ++i){
      bla.add_uniquely(_faces[i]->quad_centroid());
   }
  
   draw_setup();
   if (_visible && _stroke){
      return _stroke->draw(v);
   }
   return 0;
}

void
Pattern3dStroke::draw_end()
{
   if (_visible && _stroke){
      _stroke->draw_end();
   }
}



void
Pattern3dStroke::draw_setup()
{  
   
   double x = _pix_size / VIEW::peek()->cam()->data()->from().dist(_pts[_pts.num()/2]);//at_length(_pts[_pts.num()/2], 1) / _pix_size;
   
   
   double lo_width      = 0.0;  //zoom out
   double hi_width      = 0.5;  //zoom in
   double desired_frac;
   
   if(x < 1){  //we are  farther away from the object
      desired_frac = lo_width + (1 - lo_width) *  x;
   } else {
      desired_frac = hi_width + (1 - hi_width) *  x;
   }
   
   double new_width = _start_width * desired_frac;
   _stroke->set_width(new_width);
   
   
   //double c = 0.125;
   //double new_alpah = max(((sqrt(4*c*x + (1-2*c)*(1-2*c)-4*c*c))/(2*c)) - ((1-2*c)/(2*c)), 0.0);
   // _stroke->set_alpha(new_alpah);   
 
   stroke_pts_setup();
}


void
Pattern3dStroke::stroke_pts_setup()
{
   if (!_visible) return;
   
  
   NDCZpt pt;

   _stroke->clear();  // XXX make sure that realloc is not called each time as a 
                      // result of clear...

   CWtransf &inv_tran = _cell->pg()->patch()->inv_xform();
   //static double pix_spacing = max(Config::get_var_dbl("HATCHING_PIX_SAMPLING",5,true),0.000001);
   double pix_spacing = 3;
   //Estimated pixel length of stroke   
   
   // XXX - need to get meshes pix size...
   // static double view_pix_size = at_length(_pts[0], 1);
   // static uint update_length = VIEW::stamp();
   // if(update_length != VIEW::stamp()){
   //   view_pix_size = at_length(_pts[0], 1);
   //   update_length = VIEW::stamp();
   //}
   double pix_len = _offsets->get_pix_len(); // * (view_pix_size / _pix_size); 
   //_cell->pg()->patch()->mesh()->pix_size() /_pix_size;
    
   //Desired samples
   double num = pix_len/pix_spacing;

   //Spacing
   double gap = max((double)_pts.num() / num, 1.0);
   //cerr << "gap is " << gap << endl;
   //double gap = 1.0;
   
   double i=0;  int j = 0;   
   int n = _pts.num()-1;
   double w,a;
   Wtransf norm_xf = inv_tran.transpose();
   while (j<=n){
      assert(_faces[j]); 
      pt = NDCZpt(_pts[j],_cell->pg()->patch()->obj_to_ndc());     
      double ratio=1.0;
      
      if(_cell->pg()->get_light_on() && (_cell->pg()->get_light_width() || _cell->pg()->get_light_alpha()))      
         ratio = get_light_ratio(_norms[j]);
      
      w =  (!_cell->pg()->get_light_on() || !_cell->pg()->get_light_width()) 
           ? _width[j] 
           : ratio * _width[j]; 
      a =  (!_cell->pg()->get_light_on() || !_cell->pg()->get_light_alpha()) 
           ? _alpha[j] 
           : ratio * _alpha[j];  
      
      _stroke->add(pt, _faces[j], w, a, norm_xf * _norms[j], true);
      i += gap;
      j = (int)i;      
   } 

}
double       
Pattern3dStroke::get_light_ratio(Wvec normal)
{
  double lumin = color().luminance();
   
   
  VIEWptr view = VIEW::peek();
  Wvec light_c = (view->light_get_in_cam_space(_cell->pg()->get_light_num()))
     ? (view->cam()->xform().inverse() * view->light_get_coordinates_v(_cell->pg()->get_light_num()))
     : view->light_get_coordinates_v(_cell->pg()->get_light_num());
  
   
  // light_c =  _cell->get_inv() * light_c;
  double nDotVP = max(0.0, normal * light_c);
  
  Wvec R = 2 * (normal * light_c)*normal - light_c;
  R = R.normalized();   
  double nDotR = max(0.0, (-1) * view->cam()->data()->at_v() * R);
   
  double x;
  if(_cell->pg()->get_light_type() == 0)
     x = nDotVP;
  else 
     x = nDotR;
  
  double a = _cell->pg()->get_light_a();
  double b = _cell->pg()->get_light_b();
  double f = 0;
  
  if(x < a) 
    f=0;
  else if(x > b)
    f=1;
  else{
    double tmp =  (x-a)/(b-a);
    f = 3 * tmp * tmp - 2 * tmp*tmp*tmp;     
  }
  f = (lumin > 0.5) ?  f : (1-f);
  
  return f; 
}




void
Pattern3dStroke::reverse()
{ 
   _pts.reverse();
   _norms.reverse();

   // XXX not sure if offsets matter for us in this case
   //_offsets.reverse();   
}

UVpt_list  
Pattern3dStroke::uv_2_offsetUV(UVpt avg_pt, UVpt_list uv)
{
   UVpt_list list;
   for(int i=0; i < uv.num(); ++i)
      list += uv_2_offsetUV(avg_pt, uv[i]);
   
   return list;   
}

UVpt_list  
Pattern3dStroke::offsetUV_2_uv(UVpt avg_pt, UVpt_list offset_uv)
{
   UVpt_list list;
   for(int i=0; i < offset_uv.num(); ++i)
      list += offsetUV_2_uv(avg_pt, offset_uv[i]);   
   return list;   
}

bool      
Pattern3dStroke::is_parallel_to(Pattern3dStroke * stroke, double& avg_dist, bool dir)
{
   //int number = (_pts.num() > stroke->_pts.num()) ? stroke->_pts.num() : _pts.num();
   ARRAY<double> dist;
   //ARRAY<double> dist2;      
   int           number_of_good=0;
   int           number_of_good2=0;
   
   
   bool current_dir = (stroke_distance_dir((Pattern3dStroke *)this ,stroke));
   if(current_dir != dir){
        err_adv(debug_stroke, "Dirs are wrong %d (%d)",dir, current_dir);
        return false;
   }
   
   for(int i = 0; i < _pts.num(); ++i){
      Wpt tmp_point;
      double distance=0;
      int position;      
      
      stroke->_pts.closest(_pts[i], tmp_point, distance, position);
      
      // If the point on the other curve is endpoint then don't count the curve
      if(position != 0 && position != stroke->_pts.num()-1){
          dist += distance;
          number_of_good++;
      }      
   }
  
   for(int j = 0; j < stroke->_pts.num(); ++j){
      Wpt tmp_point;
      double distance;
      int position;
      _pts.closest(stroke->_pts[j], tmp_point, distance, position);       
                     
      if(position != 0 && position !=  _pts.num()-1){
           dist += distance;
           number_of_good2++;
      }
   }
   
   
   err_adv(debug_stroke,"=== number of good is %d, %d and number is %d, %d",number_of_good, number_of_good2, stroke->_pts.num(), _pts.num());
   if(number_of_good < (_pts.num()*0.4) || number_of_good2 < (stroke->_pts.num()*0.4)){
      err_adv(debug_stroke,"GOOD THRESH");
      return false;
   }
   
   //dist += dist2;
   double average;
   double std_d; 
   double _max; 
   double _min;
   statistics(dist, debug_stroke, &average, &std_d, &_max, &_min);   


   avg_dist =  average;
   if(fabs(std_d) > PARALLEL_STD_THRESH){
      err_adv(debug_stroke,"STD THRESH");
      return false;
   }
   if(fabs(_max - _min) > PARALLEL_MAX_MIN_DIFF_THRESH){
      err_adv(debug_stroke,"MIN MAX THRESH");
      return false; 
   }
   
   return true;
}

bool
Pattern3dStroke::stroke_distance_dir (Pattern3dStroke* a, Pattern3dStroke* b)
{


   int a_pt_loc = a->get_verts_num() / 2;
   int b_pt_loc = b->get_verts_num() / 2;

   Wvec a_normal = a->get_norms()[a_pt_loc];
   Wvec b_normal = b->get_norms()[b_pt_loc];

   Wvec a_tangent = avarage_vector(a); //a->get_pts().tan(a_pt_loc);
   Wvec b_tangent = avarage_vector(b); //b->get_pts().tan(b_pt_loc);

   Wvec dir_a = cross(a_normal, a_tangent).normalized();
   Wvec dir_b = cross(b_normal, b_tangent).normalized();

    // XXX this makes sure strokes are oriented the same way,
   if((dir_a * dir_b) < 0){
        err_adv(debug_stroke, "Strokes do not flow in the same direction, reversing");
        b->reverse();
   }

   Wline dir_to_b(a->get_pts()[a_pt_loc], b->get_pts()[b_pt_loc]);

   return ((dir_a * dir_to_b.vector()) < 0) ? true : false;
}

// XXX --add up all the tangents and then avarage them...
Wvec 
Pattern3dStroke::avarage_vector(Pattern3dStroke* stroke)
{        
   RunningAvg<Wvec> avg(stroke->_pts.tan(0));   
   for(int i=1; i < stroke->_pts.num(); ++i){       
       avg.add(stroke->_pts.tan(i)); 
   }
   return avg.val();   
}
/////////////////////////////////////
// Static Variable Initialization
/////////////////////////////////////
TAGlist *       Pattern3dStroke::_ps_tags = 0;

/////////////////////////////////////
// tags()
/////////////////////////////////////
CTAGlist &
Pattern3dStroke::tags() const
{
   
   if (!_ps_tags) {
      _ps_tags = new TAGlist;
      *_ps_tags += new TAG_meth<Pattern3dStroke>(
         "faces",
         &Pattern3dStroke::put_faces,
         &Pattern3dStroke::get_faces,
         0);
      *_ps_tags += new TAG_meth<Pattern3dStroke>(
         "wpts",
         &Pattern3dStroke::put_pts,
         &Pattern3dStroke::get_pts,
         0);
      *_ps_tags += new TAG_meth<Pattern3dStroke>(
         "norms",
         &Pattern3dStroke::put_norms,
         &Pattern3dStroke::get_norms,
         0);
      *_ps_tags += new TAG_meth<Pattern3dStroke>(
         "alpha",
         &Pattern3dStroke::put_alpha,
         &Pattern3dStroke::get_alpha,
         0);
      *_ps_tags += new TAG_meth<Pattern3dStroke>(
         "width",
         &Pattern3dStroke::put_width,
         &Pattern3dStroke::get_width,
         0);   
      *_ps_tags += new TAG_meth<Pattern3dStroke>(
         "pix_size",
         &Pattern3dStroke::put_pix_size,
         &Pattern3dStroke::get_pix_size,
         0);

   }
   return *_ps_tags;
}
void    
Pattern3dStroke::put_faces (TAGformat &d) const
{
   d.id();
   *d << _faces;
   d.end_id();      
}
   
void    
Pattern3dStroke::get_faces (TAGformat &d)
{
   
}
   
void    
Pattern3dStroke::get_pts (TAGformat &d)
{
   *d >> _pts;
}

void    
Pattern3dStroke::put_pts (TAGformat &d) const
{   
   d.id();
   *d << _pts;
   d.end_id();   
}

void    
Pattern3dStroke::get_norms (TAGformat &d)
{
   *d >> _norms;
  
}

void    
Pattern3dStroke::put_norms (TAGformat &d) const
{
   d.id();
   *d <<  _norms;
   d.end_id();      
}
   
void    
Pattern3dStroke::put_alpha(TAGformat &d) const
{
   d.id();
   *d <<  _alpha;
   d.end_id();         
}
   
void    
Pattern3dStroke::get_alpha (TAGformat &d)
{
   *d >> _alpha;   
}
   
void    
Pattern3dStroke::put_width(TAGformat &d) const
{
   d.id();
   *d <<  _width;
   d.end_id();         
}
   
void    
Pattern3dStroke::get_width(TAGformat &d)
{
   *d >> _width;   
}

void    
Pattern3dStroke::put_pix_size(TAGformat &d) const
{
   d.id();
   *d <<  _pix_size;
   d.end_id();         
}
   
void    
Pattern3dStroke::get_pix_size(TAGformat &d)
{
   *d >> _pix_size;
}   

// ****************************
// Stroke List
// ****************************
void
Stroke_List::add_strokes(ARRAY<Pattern3dStroke*>& stroke)
{
       for(int i=0; i < stroke.num(); ++i){            
          if(stroke_can_be_added(stroke[i])){ 
             add(stroke[i]);          
             stroke.remove(i);
             --i;
             update_avg_info();
          } else {
             cerr << "----cannot add the stroke" << endl;
          }
          
       }    
}

bool 
Stroke_List::stroke_can_be_added(Pattern3dStroke* stroke)
{
     if(empty())
        return true;
     //XXX commeted for debuging
     //if(_color != stroke->color())
     //    return false;   
     
     
     return true;     
}
void
Stroke_List::strokes_not_good(ARRAY<Pattern3dStroke*>& wrong_group)
{ }

void 
Stroke_List::update_avg_info()
{    
   
   double sum_avg_length = 0;
   double sum_avg_spread = 0;
   double sum_avg_winding = 0;
   double sum_avg_straightness = 0;
   double sum_area =0;     
   for(int i=0; i < num(); ++i){    
       sum_avg_length += _array[i]->length(); 
       sum_avg_spread += _array[i]->spread();    
       sum_avg_winding += _array[i]->winding();     
       sum_avg_straightness += _array[i]->straightness();
       sum_area +=  _array[i]->area();   
   }
   
   
   if(num() > 0){
      _color  = _array[0]->color(); 
      _area = sum_area;    
      _avg_length = sum_avg_length / num(); 
      _avg_spread = sum_avg_spread / num();      
      _avg_winding = sum_avg_winding / num();    
      _avg_straightness = sum_avg_straightness / num();
     
   }     
}


Wvec 
Stroke_List::avarage_vector() const
{
   RunningAvg<Wvec> avg(Pattern3dStroke::avarage_vector(_array[0]));   
   for(int i=1; i < num(); ++i){
       avg.add(Pattern3dStroke::avarage_vector(_array[i])); 
   }
   return avg.val();   
}

Pattern3dStroke*
Stroke_List::add_closest_parallel_stroke(Pattern3dStroke* start_stroke,
                                             ARRAY<Pattern3dStroke*>& unsorted,
                                             bool dir)
{
    Pattern3dStroke* new_stroke;
    double distance=0;
    // First get the strokes to one side
    
      
       stroke_map_less sorted;
       for(int i =0; i < unsorted.num(); ++i){
           if(stroke_can_be_added(unsorted[i])){
              if(start_stroke->is_parallel_to(unsorted[i], distance, dir))
                sorted[distance] = i;                     
           }       
       }
       if(sorted.empty())
          return 0;
       
       new_stroke = unsorted[sorted.begin()->second];
       if(new_stroke){
         add(new_stroke);
         update_avg_info();
         unsorted.remove(sorted.begin()->second);     
       }   
    return new_stroke;
}



//function add_strokes(ARRAY strokes){
//   pick a random stroke that qualifies as hatching
//   for evry stroke that is in strokes list
//      get closest_parallel_stroke to the right
//     
//   for evry stroke that is left
//      get closest_parallel_stroke to the left
//     
//}
 
// ********** Structured_Hatching ********* //
void
Structured_Hatching::add_strokes(ARRAY<Pattern3dStroke*>& stroke)
{    
    Pattern3dStroke* first_stroke;
    //Pick one stroke that qualifies
    for(int i=0; i < stroke.num(); ++i){            
          if(stroke_can_be_added(stroke[i])){ 
             first_stroke = stroke[i];             
             add(stroke[i]);
             stroke.remove(i);             
             update_avg_info();
             break;             
          }          
    }    

    
    
    if(empty())
       return;
    bool dir = true;   //direction
    
    // Get all the strokes in right direction   
    Pattern3dStroke* start_stroke = first_stroke;
    while ((start_stroke = add_closest_parallel_stroke(start_stroke ,stroke, dir)))
    {}
    
    start_stroke = first_stroke;
    dir = false;
    while ((start_stroke = add_closest_parallel_stroke(start_stroke ,stroke, dir)))
    {}
         
    // Visually Check if strokes are in correct order
    CCOLOR bla = Color::firebrick;//random();       
    for(int k=0; k < num(); ++k){         
       _array[k]->get_stroke()->set_color(bla);
       //err_adv(debug_stroke, "Hatch Stroke %d lenght is %f", k, _array[k]->get_pts().length());
    }
   
    
    
}

bool
Structured_Hatching::stroke_can_be_added(Pattern3dStroke* stroke)
{
    
    if(!Stroke_List::stroke_can_be_added(stroke))
      return false;
   
   // Is the stroke a straight enouth?
   if(stroke->straightness() < HATCHING_STRAIGHTNESS_THRESH)
      return false;
    if(stroke->spread() < HATCHING_SPREAD_THRESH){
      err_adv(debug_stroke,"Structured_Hatching::stroke_can_be_added - spread too small");
      return false;
    }
   
   return true;  
}

double 
Structured_Hatching::is_differnt_to_group(Stroke_List* list)
{
   assert(!empty() && !list->empty());
   if(class_name() != list->class_name())
      return PatternGrid::MAX_DIFF_THRESHOLD + 1;
   
   double dist = 0;
      
   // Density Distance
   dist += fabs(density() - list->density()) * DENSITY_W;
   //cerr << "Density Diff: " << density() << " " << list->density() << endl; 
   // Color Distance   
   dist += color().dist(list->color()) * COLOR_W;
   //cerr << "Color  Diff: " << color() << " " << list->color()<< endl; 
   
   // Length Distance
   dist += fabs(avg_length() - list->avg_length()) * LENGTH_W;   
   //cerr << "Length Diff: " << avg_length() << " " << list->avg_length()<< endl; 
   
   // Direction Distance
   double angle =  line_angle(avarage_vector(), list->avarage_vector());
   dist += (angle / M_PI_2) * DIRECTION_W; // normalize the angle
   //cerr << "Direction Diff: " << angle <<  endl; 
      	
   return dist;
      
}

void
Structured_Hatching::strokes_not_good(ARRAY<Pattern3dStroke*>& wrong_group)
{    
    if(num() < 4) {
         //put all the strokes you stole in a temp list and delete yourself
         for(int i=0; i < num(); ++i){
                 
            CCOLOR bla = Color::black; 
            _array[i]->get_stroke()->set_color(bla);
            wrong_group.add(_array[i]);
         }
         clear();
    }        
}

// ********** Stippling ********* //

bool
Stippling::stroke_can_be_added(Pattern3dStroke* stroke)
{   
  
   if(!Stroke_List::stroke_can_be_added(stroke))
      return false;
   
   if(stroke->spread() > STIPPLING_SPREAD_THRESH)
      return false;
   
   if(!empty()){     
       double diff = 0;
       diff += (avg_length() - stroke->length()) * (avg_length() - stroke->length());  
       diff += (avg_spread() - stroke->spread()) * (avg_spread() - stroke->spread());  
       diff += (avg_winding() - stroke->winding()) * (avg_winding() - stroke->winding());   
       diff += (avg_straightness() - stroke->straightness()) * (avg_straightness() - stroke->straightness());       
       diff = sqrt(diff);

      err_msg("Stippling Differnce : %f ",diff);  
      if(diff > STIPPLING_STROKE_SIM_DIFF){
          return false;
      }
          
   }   
   
   CCOLOR bla = Color::blue_pencil_d;//random();      
   stroke->get_stroke()->set_color(bla);
   
   return true;  
}



double 
Stippling::is_differnt_to_group(Stroke_List* list)
{
   
   assert(!empty() && !list->empty());
   
   if(class_name() != list->class_name())
      return PatternGrid::MAX_DIFF_THRESHOLD + 1;
   
   double dist = 0;
      
   // Density Distance
   dist += fabs(density() - list->density()) * DENSITY_W;
  
   // Color Distance   
   dist += color().dist(list->color()) * COLOR_W;  
   
   // Length Distance
   dist += fabs(avg_length() - list->avg_length()) * LENGTH_W;   
 
      	
   return dist; 
}

void
Stippling::strokes_not_good(ARRAY<Pattern3dStroke*>& wrong_group)
{    
    if(num() < 4) {
         //put all the strokes you stole in a temp list and delete yourself
         for(int i=0; i < num(); ++i){
                 wrong_group.add(_array[i]);
         }
         clear();
    }        
}
// ********** OtherStrokes ********* //
double 
OtherStrokes::is_differnt_to_group(Stroke_List* list)
{
   assert(!empty());
   if(class_name() != list->class_name())
      return PatternGrid::MAX_DIFF_THRESHOLD + 1;
   
   double dist = 0;
      
   // Density Distance
   dist += fabs(density() - list->density()) * DENSITY_W;
  
   // Color Distance   
   dist += color().dist(list->color()) * COLOR_W;  
   
   // Length Distance
   dist += fabs(avg_length() - list->avg_length()) * LENGTH_W;   
   
   return dist;
}

bool 
OtherStrokes::stroke_can_be_added(Pattern3dStroke* stroke)
{
     if(!Stroke_List::stroke_can_be_added(stroke))
      return false;    
     
     CCOLOR bla = Color::black; 
     stroke->get_stroke()->set_color(bla);
     
     
     return true;     
}

double 
NullStrokes::is_differnt_to_group(Stroke_List* list)
{  
   //assert(!empty());
   double dist = 0;
      
   // Density Distance
   dist += list->density() * DENSITY_W;
   dist += list->avg_length() * LENGTH_W; 
   return dist;
}
*/
