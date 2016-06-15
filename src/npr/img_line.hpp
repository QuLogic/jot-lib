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
#ifndef IMG_LINE_H_IS_INCLUDED
#define IMG_LINE_H_IS_INCLUDED

#include "geom/texture.hpp"
#include "gtex/glsl_toon.hpp"
#include "gtex/ref_image.hpp"
#include "npr/ridge.hpp"
#include "npr/binary_image.hpp"
#include "stroke/base_stroke.hpp"

#include <vector>

class Vec2d : public Vec2<Vec2d> {
public:
   Vec2d() {}
   Vec2d(double x, double y) : Vec2<Vec2d>(x, y) {}
};

class Vec2dArray : public vector<Vec2d> {
public:
   Vec2dArray(int m=0) : vector<Vec2d>() { reserve(m); }
};


class DoubleArray : public vector<double> {
public:
   DoubleArray(int m=0) : vector<double>() { reserve(m); }
   DoubleArray(const vector<double>& l) : vector<double>(l) {}
};

class DoubleMatrix : public vector<DoubleArray> {
public:
   DoubleMatrix(int m=0, int n=0) : vector<DoubleArray>() {
      init(m, n);
   }

   void init(int m = 0, int n = 0) {
      clear();
      resize(m);
      if (m && n) {
         for (int i = 0; i < m; i++) {
            at(i).reserve(n);
         }
      }
   }

   double operator ()(const int i, const int j) const { 
      return (*this)[i][j];
   }
};


/*****************************************************************
* ImgLineTexture:
*
*****************************************************************/
class ImgLineTexture : public OGLTexture {
public:
   //******** MANAGERS ********
   ImgLineTexture(Patch* patch = nullptr);
   virtual ~ImgLineTexture(); 

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3(
   "ImgLineTexture", ImgLineTexture*, OGLTexture, CDATA_ITEM*
   );

   //******** GTexture VIRTUAL METHODS ********

   virtual GTexture_list gtextures() const {
      return GTexture_list(_ridge);
   }

   virtual int draw(CVIEWptr& v); 

   //******** RefImageClient METHODS ********

   // request ref images needed by this texture
   virtual void request_ref_imgs();

   // how to draw the color reference image
   virtual int draw_color_ref(int i) {
      switch(i) {
	 case 0: return _ridge->draw_color_ref(i);
	 case 1: return _ridge->draw(VIEW::peek());
	 default:
	    return 0;
      }
   }

   virtual int draw_final(CVIEWptr &);

   //******** DATA_ITEM METHODS ********
   virtual DATA_ITEM  *dup() const { return new ImgLineTexture; }

   // ACCESSOR FOR UI
   void set_tex(const string& full_path_name){
      if(!_ridge)
	 return;
      _ridge->set_tex(full_path_name);
   }

   void set_curv_threshold(const float thd) { _ridge->set_curv_threshold(thd); }
   void set_dist_threshold(const float thd) { _ridge->set_dist_threshold(thd); }
   void set_vari_threshold(const float thd) { _ridge->set_vari_threshold(thd); }
   void set_start_offset(const int offset)  { _ridge->set_start_offset(offset);}
   void set_end_offset(const int offset)    { _ridge->set_end_offset(offset);  }
   void set_stroke_debug(const bool d)      { BaseStroke::set_debug(d);     }
   void set_apply_snake(const int d)        { _apply_snake = d; }
   void set_dark_threshold(const float thd) { _ridge->set_dark_threshold(thd); }
   void set_bright_threshold(const float thd) { _ridge->set_bright_threshold(thd); }

   float get_curv_threshold() const { return _ridge->get_curv_threshold(); }
   float get_dist_threshold() const { return _ridge->get_dist_threshold(); }
   float get_vari_threshold() const { return _ridge->get_vari_threshold(); }
   int   get_start_offset() const   { return _ridge->get_start_offset();   }
   int   get_end_offset() const     { return _ridge->get_end_offset(); }
   bool  get_stroke_debug() const   { return BaseStroke::get_debug();  }
   int   get_apply_snake() const    { return _apply_snake; }
   float get_dark_threshold() const { return _ridge->get_dark_threshold(); }
   float get_bright_threshold() const { return _ridge->get_bright_threshold(); }

   ToneShader *get_tone_shader()    { return _ridge->get_tone_shader(); }

   protected:
   //******** MEMBER DATA ********
   ColorRefImage*               _col_ref;
   IDRefImage*                  _id_ref;

   RidgeShader*                 _ridge;    // for producing the colore ref image
   // strokes:
   BaseStroke                   _prototype;     // prototype stroke
   BaseStrokeOffsetLISTptr      _offsets;       // wiggles to apply to strokes   
   BaseStrokeArray              _bstrokes;      // collection of strokes

   uint                         _strokes_drawn_stamp;

   double                       _start_offset, _end_offset;
   vector<uint>                 _previous_start_pixels;
   vector<uint>                 _previous_start_vals;

   int                          _apply_snake;
   DoubleMatrix                 _matrix, _lower_matrix, _upper_matrix;
   Vec2dArray                   _right_term;


   //******** UTILITIES ********
   void rebuild_if_needed(CVIEWptr &v);
   void build_strokes(CVIEWptr &v);
   bool add_stroke(const uint pixel, int &bnum);
   void build_stroke_one_side(Cpoint2i &start_pix, CXYvec &dir, 
	 vector<Point2i> &stroke_array, vector<float> &width_array,
	 vector<XYvec> &dir_array);
   void apply_snake(vector<Point2i> &point_array,
         vector<float> &width_array, vector<XYvec> &dir_arra);
   void get_snake_matrix(const int size);
   void lu_decompose(const int matrix_size);
   void lu_back_substitution(const int matrix_size, vector<Vec2d> &sols);
   void update_snake_positions(const int max_movement, vector<Point2i> &point_array);
   void get_right_term(vector<Point2i> &point_array);

   CBaseStroke& get_prototype()  const { return _prototype; }
   void set_prototype(CBaseStroke &s);
   void set_offsets(BaseStrokeOffsetLISTptr ol);

   double get_radius(const uint id) const { 
      double r = _col_ref->blue(id)/255.0 * (_end_offset-_start_offset+0.1) 
	 + _start_offset; 
      return (r * 1.5); 
   }
   double get_radius(Cpoint2i &pix) const { 
      uint id = _col_ref->pix_to_uint(pix);
      return get_radius(id);
   }

   double get_angle(const uint id) const {
      return _col_ref->green(id)/255.0*2.0*M_PI-M_PI;
   }

   double get_angle(Cpoint2i &pix) const {
      uint id = _col_ref->pix_to_uint(pix);
      return _col_ref->green(id)/255.0*2.0*M_PI-M_PI;
   }

};

class PriorityPixel{
public:
   bool operator < (const PriorityPixel & other) const { 
      return _priority < other._priority; 
   }

   double _priority; // _width
   uint   _pixel;
};


#endif // IMG_LINE_H_IS_INCLUDED
