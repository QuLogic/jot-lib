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
/**********************************************************************
* img_line.C
**********************************************************************/
#include "gtex/util.H"
#include "mesh/patch.H"
#include "mlib/point2i.H"

#include "img_line.H"

#include <queue>

ImgLineTexture::ImgLineTexture(Patch* patch) :
   OGLTexture(patch),
   _ridge (new RidgeShader(patch)),
   _strokes_drawn_stamp(0),
   _apply_snake(1)
{
   _prototype.set_flare((float)1.0);
   _prototype.set_taper(100);
   _prototype.set_alpha(1);
   _prototype.set_width(5.5);
   _prototype.set_use_paper(0);
}

ImgLineTexture::~ImgLineTexture() 
{
   delete _ridge;
   _bstrokes.delete_all();

}

void 
ImgLineTexture::request_ref_imgs()
{
   // request IDRefImage
   IDRefImage::schedule_update(VIEW::peek(),true);

   // request two color images;
   //   first one in texture memory has tone,
   //   second one in main memory has output of ridge shader...
   ColorRefImage::schedule_update(0, false,  true); // 1st image: in tex mem
   ColorRefImage::schedule_update(1,  true, false); // 2nd image: in main mem
}

void
ImgLineTexture::build_stroke_one_side(Cpoint2i &start_pix, CXYvec &start_dir, ARRAY<Point2i> &stroke_array, ARRAY<float> &width_array, ARRAY<XYvec> &dir_array)
{
   bool find_pixel;
   double max_dist, dist;
   Point2i max_pixel, neighbor_pixel;
   double radius;
   Point2i pix = start_pix;
   XYvec dir, prev_dir = start_dir;
   double angle;

   stroke_array.clear();
   width_array.clear();
   dir_array.clear();

   bool first = true;

   do{
      max_dist = 0;
      find_pixel = false;

      angle = get_angle(pix);

      dir[0] = sin(angle);
      dir[1] = -cos(angle);

      radius = get_radius(pix);

      if(dir[0]*prev_dir[0] + dir[1]*prev_dir[1] < 0.0){
	 dir[0] = -dir[0];
	 dir[1] = -dir[1];
      }

      int r = int(round(radius));
      for(int m = -r-2; m <= r+2; m++){
	 for(int n = -r-2; n <= r+2; n++){
	    neighbor_pixel = pix + Vec2i(m, n);

	    int neighbor_grey_val = _col_ref->alpha(neighbor_pixel);
	    int neighbor_red_val = _col_ref->red(neighbor_pixel);

	    if (get_ctrl_patch(_id_ref->simplex(neighbor_pixel)) != _patch ||
		  neighbor_grey_val == 0 || neighbor_red_val == 0){
	       continue;
	    }

	    dist = pix.dist(neighbor_pixel);

	    if(dist > radius)
	    continue;

	    XYvec temp(neighbor_pixel[0]-pix[0], neighbor_pixel[1]-pix[1]);

	    if(temp*dir < 0){
	       if(!first){
		  _col_ref->set(neighbor_pixel[0], neighbor_pixel[1], 
		  _col_ref->color(neighbor_pixel[0], neighbor_pixel[1]), 
		  0.0);
	       }
	       continue;
	    }

	    dist = fabs(temp*dir);

	    _col_ref->set(neighbor_pixel[0], neighbor_pixel[1], 
	    _col_ref->color(neighbor_pixel[0], neighbor_pixel[1]), 0.0);

	    if(max_dist < dist){
	       max_dist = dist;
	       max_pixel = neighbor_pixel;
	       find_pixel = true;
	    }
	 }
      }

      if(find_pixel){
	 stroke_array += max_pixel;
	 width_array += radius;
	 dir_array += dir;
	 pix = max_pixel;
      }

      prev_dir = dir;
      first = false;

   }while(find_pixel == true);
}

bool
ImgLineTexture::add_stroke(const uint pixel, int &bnum)
{
   double radius, angle;
   XYvec dir;

   int grey_val = _col_ref->alpha(pixel);
   int red_val = _col_ref->red(pixel);
   ARRAY<Point2i> pos_array;

   if(grey_val == 0 || red_val == 0){
      return false;
   }

   angle = get_angle(pixel);
   radius = get_radius(pixel);

   Point2i pix = _col_ref->uint_to_pix(pixel);

   // mark the pixel as visited by putting 0 in alpha channel:
   _col_ref->set(pix[0], pix[1], _col_ref->color(pix[0], pix[1]), 0);

   if ( bnum == _bstrokes.num() ) {
      // No more old strokes to re-use;
      // have to allocate a new one:
      _bstrokes.add( _prototype.copy() );
      //_bstrokes[bnum]->set_color(COLOR::random());
      _bstrokes[bnum]->set_offsets(_offsets);
   }
   assert ( bnum < _bstrokes.num() );

   Point2i neighbor_pixel;

   dir[0] = sin(angle);
   dir[1] = -cos(angle);

   ARRAY<Point2i> tmp_array;
   ARRAY<float> width_array, width_array2; // XXX - for debugging
   ARRAY<XYvec> dir_array, dir_array2;   // XXX - for debugging

   build_stroke_one_side(pix, dir, tmp_array, width_array, dir_array);

   // add stroke vertices, starting with farthest
   for(int k = tmp_array.num()-1; k >= 0; k--){
      pos_array.add(tmp_array[k]);
      dir_array2.add(dir_array[k]);
      width_array2.add(width_array[k]);
   }

   // add stroke vertex at center
   pos_array.add(pix);
   dir_array2.add(dir);
   width_array2.add(radius);

   dir[0] = -sin(angle);
   dir[1] = cos(angle);

   // build stroke on the other side of the center
   build_stroke_one_side(pix, dir, tmp_array, width_array, dir_array);

   // add remaining vertices from nearest to farthest
   for(int k = 0; k < tmp_array.num(); k++){
      pos_array.add(tmp_array[k]);
      dir_array2.add(dir_array[k]);
      width_array2.add(width_array[k]);
   }


   if(_apply_snake){
      apply_snake(pos_array, width_array2, dir_array2);
   }


   for(int k = 0; k < pos_array.num(); k++){
      if(BaseStroke::get_debug()) {
	 _bstrokes[bnum]->add(
	 _col_ref->pix_to_ndc(pos_array[k]),
	 width_array2[k],
	 dir_array2[k]);
      } 
      else {
	 _bstrokes[bnum]->add(_col_ref->pix_to_ndc(pos_array[k]), 1.0f);
      }
   }

   if(_bstrokes[bnum]->num_verts() >= 3){
      bnum++;
      return true;
   }
   else{
      _bstrokes.remove(bnum);
      return false;
   }
}

void 
ImgLineTexture::build_strokes(CVIEWptr &v)
{
   int bnum = 0;
   _start_offset = get_start_offset(), _end_offset = get_end_offset();

   _bstrokes.clear_strokes();

   // access the color reference image:
   _col_ref = ColorRefImage::lookup(1,v);
   _id_ref  =    IDRefImage::lookup(v);

   const ARRAY<uint>& pixels = _patch->pixels();

   priority_queue<PriorityPixel, vector<PriorityPixel> > queue;

   double radius;

   for (int i=0; i<pixels.num(); i++) {
      int grey_val = _col_ref->alpha(pixels[i]);
      int red_val = _col_ref->red(pixels[i]);

      if(grey_val == 0 || red_val == 0){
	 continue;
      }

      radius = get_radius(pixels[i]);

      PriorityPixel p;
      p._priority = radius;
      p._pixel = pixels[i];
      queue.push(p);
   }

   uint pixel;

   /*ARRAY<uint> tmp_array;
   ARRAY<uint> tmp_val_array;

   for(i = 0; i < _previous_start_pixels.num(); i++){
      pixel = _previous_start_pixels[i];

      Point2i p = _col_ref->uint_to_pix(pixel);

      _col_ref->set(p, _col_ref->red(p), _col_ref->green(p), _previous_start_vals[i], _col_ref->alpha(p));

      if(add_stroke(pixel, bnum)){
	 tmp_array.add(pixel);
	 tmp_val_array.add(_col_ref->blue(pixel));
      }
   }

   _previous_start_pixels.clear();
   _previous_start_vals.clear();

   for(i = 0; i < tmp_array.num(); i++){
      _previous_start_pixels.add(tmp_array[i]);
      _previous_start_vals.add(tmp_val_array[i]);
   }
*/
   while(!queue.empty()){ 
      pixel = queue.top()._pixel;
      queue.pop();

      add_stroke(pixel, bnum);
/*      if(add_stroke(pixel, bnum)) {
	 _previous_start_pixels.add(pixel);
         _previous_start_vals.add(_col_ref->blue(pixel));
      }*/
   }

}

void
ImgLineTexture::rebuild_if_needed(CVIEWptr &v)
{
   if(_strokes_drawn_stamp != VIEW::stamp()){
      build_strokes(v);

      _strokes_drawn_stamp = VIEW::stamp();
   }
}

int
ImgLineTexture::draw(CVIEWptr& v)
{
   rebuild_if_needed(v);

   return _patch->faces().num();
}

int
ImgLineTexture::draw_final(CVIEWptr& v)
{
   _bstrokes.draw(v);

   return _patch->faces().num();
}

void 
ImgLineTexture::apply_snake(ARRAY<Point2i> &point_array, ARRAY<float> &width_array, ARRAY<XYvec> &dir_array)
{
   ARRAY<Point2i> t;
   ARRAY<float> t_w;
   ARRAY<XYvec> t_d;

   for(int k = 0; k < point_array.num(); k++){
      t.add(point_array[k]);
      t_w.add(width_array[k]);
      t_d.add(dir_array[k]);
   }

   update_snake_positions(1, t);

   point_array.clear();
   width_array.clear();
   dir_array.clear();

   for(int k = 0; k < t.num(); k++){
      if(point_array.add_uniquely(t[k])){
	 width_array.add(t_w[k]);
	 dir_array.add(t_d[k]);
      }
   }

}

void
ImgLineTexture::get_snake_matrix(const int size)
{
   int i, j, k, s;
   double dBeta[3];

   _matrix.init(size, size); 

   for (i = 0; i < size; i++)
      for (j = 0; j < size; j++)
	_matrix[i][j] = 0.0;

   dBeta[0] = dBeta[1] = dBeta[2] = 10.0;

   for (k = 2; k < size-2 ; k++) {
      s = k+1; 

      _matrix[k][k-2] = dBeta[0];
      _matrix[k][k-1] = -(2*dBeta[0]+2*dBeta[1]);
      _matrix[k][k] =	dBeta[0] + 4*dBeta[1] + dBeta[2];
      _matrix[k][k+1] = -(2*dBeta[1]+2*dBeta[2]);
      _matrix[k][k+2] = dBeta[2];
   }

   // first row
   k = 0; s = 1;

   _matrix[0][0] = dBeta[0] + 4*dBeta[1] + dBeta[2];
   _matrix[0][1] = -(2*dBeta[1] + 2*dBeta[2]);
   _matrix[0][2] = dBeta[2];

   // second row
   k = 1; s = 2;

   _matrix[1][0] = -(2*dBeta[0] + 2*dBeta[1]);
   _matrix[1][1] = dBeta[0] + 4*dBeta[1] + dBeta[2];
   _matrix[1][2] = -(2*dBeta[1] + 2*dBeta[2]);
   _matrix[1][3] = dBeta[2];

   // the last row
   k = size-1; s = size; // size == num_points-2

   _matrix[size-1][size-3] = dBeta[0];
   _matrix[size-1][size-2] = -(2*dBeta[0] + 2*dBeta[1]);
   _matrix[size-1][size-1] = dBeta[0] + 4*dBeta[1] + dBeta[2];

   // second from the last
   k = size-2; s = size-1; 

   _matrix[size-2][size-4] = dBeta[0];
   _matrix[size-2][size-3] = -(2*dBeta[0] + 2*dBeta[1]);
   _matrix[size-2][size-2] = dBeta[0] + 4*dBeta[1] + dBeta[2];
   _matrix[size-2][size-1] = -(2*dBeta[1] + 2*dBeta[2]);

   for (k = 0; k < size; k++){
      _matrix[k][k] += 20.0;
   }

}

void 
ImgLineTexture::lu_decompose(const int matrix_size)
{
   float dSum0, dSum1;
   int i, j, k;

   int size = matrix_size;

   _lower_matrix.init(size, size);
   _upper_matrix.init(size, size);

   for(i = 0 ; i < size ; i++){
      _lower_matrix[i][i] = 1.0f;
   }

   _upper_matrix[0][0] = _matrix[0][0];

   for(j = 1 ; j < size ; j++) {
      _upper_matrix[0][j] = _matrix[0][j];
      _lower_matrix[j][0] = _matrix[j][0]/_upper_matrix[0][0];
   }

   for(i = 1 ; i < size-1 ; i++) {
      dSum0 = 0.0f;
      for(k = 0 ; k < i ; k++) 
	 dSum0 += _lower_matrix[i][k] * _upper_matrix[k][i-k];

      _upper_matrix[i][i-i] = _matrix[i][i] - dSum0;

      for(j = i + 1 ; j < size ; j++) {
	 dSum0 = 0.0f;
	 dSum1 = 0.0f;
	 for(k = 0 ; k < i ; k++) {
	    dSum0 += (_lower_matrix[i][k] * _upper_matrix[k][j-k]);
	    dSum1 += (_lower_matrix[j][k] * _upper_matrix[k][i-k]);
	 }
	 _upper_matrix[i][j-i] = _matrix[i][j] - dSum0;
	 _lower_matrix[j][i] = (_matrix[j][i] - dSum1) / _upper_matrix[i][i-i];
      }
   }

      dSum0 = 0.0f;

   for(k = 0 ; k < size-1 ; k++) 
   dSum0 += (_lower_matrix[size-1][k] * _upper_matrix[k][size-1-k]);

   _upper_matrix[size-1][size-1-(size-1)] = _matrix[size-1][size-1] - dSum0;
}

void
ImgLineTexture::update_snake_positions(
   const int max_movement,
   ARRAY<Point2i> &point_array
   )
{
   int size = point_array.num()-2;
   ARRAY<Vec2d> sols;


   if(size <= 3)
      return;

   get_snake_matrix(size);
   lu_decompose(size);

   _right_term.init(size);

   for(int i = 0; i < max_movement; i++){
      get_right_term(point_array);
      lu_back_substitution(size, sols);

      for(int k = 0; k < sols.num(); k++){
         // XXX - okay to cast double to int?
	 point_array[k+1] = Point2i((int)sols[k][0], (int)sols[k][1]);
      }
   }
}

void
ImgLineTexture::get_right_term(ARRAY<Point2i> &point_array)
{
   int index, num_points;
   Vec2d first_point, last_point, cur_point;
   Vec2d dir;

   num_points = point_array.num();

   first_point = Vec2d(point_array[0][0], point_array[0][1]); 
   last_point = Vec2d(point_array[num_points-1][0], point_array[num_points-1][1]);


   for (index = 1; index < num_points-1; index++) {

      float angle = get_angle(point_array[index]);

      dir[0] = cos(angle);
      dir[1] = sin(angle);

      cur_point = Vec2d(point_array[index][0], point_array[index][1]);

      if( index == 1 ){
	 _right_term[index-1] = 20.0*cur_point + (3*10.0)*first_point - dir;
      }
      else if( index == 2 ){
	 _right_term[index-1] = 20.0*cur_point - 10.0*first_point - dir;
      }
      else if( index == num_points-2 ){
	 _right_term[index-1] = 20.0*cur_point + (3*10.0)*last_point - dir;
      }
      else if ( index == num_points-3 ){
	 _right_term[index-1] = 20.0*cur_point - 10.0*last_point - dir;
      }
      else{
	 _right_term[index-1] = 20.0*cur_point - dir;
      }
   }
} 
void 
ImgLineTexture::lu_back_substitution(const int matrix_size, ARRAY<Vec2d> &sols)
{
   int i, j, size = matrix_size;
   ARRAY<Vec2d> ys(size);
   Vec2d sum;

   // Step 1

   ys[0] = _right_term[0];

   for(i = 0; i < size; i++){
      sols.add(Vec2d());
   }

   for(i = 1 ; i < size; i++) {
      sum[0] = sum[1] = 0.0;

      for(j = 0 ; j < i  ; j++) {
	 sum += _lower_matrix[i][j] * ys[j];

      }
      ys[i] = _right_term[i] - sum;
   }

   // Step 2
   sols[size-1] = ys[size-1] /_upper_matrix[size-1][size-1-(size-1)];

   for(i = size-2 ; i >= 0 ; i--) {
      sum[0] = sum[1] = 0.0;

      for(j = i + 1 ; j < size ; j++) {
	 sum += _upper_matrix[i][j-i] * sols[j];
      }
      sols[i] = (ys[i] - sum) / _upper_matrix[i][i-i];
   }

}

// end of file img_line.C
