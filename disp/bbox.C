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
#include "std/support.H"
#include "disp/bbox.H"
#include "disp/gel.H"
#include "disp/ray.H"
#include "disp/view.H"

using namespace mlib;

BBOX::BBOX(
   CGELptr &g, 
   int      recurse
   ) :_valid(true)
{
   GELptr gel((GEL *)&*g); // sigh.. get_bb isn't const cuz it caches
   *this += BBOX(gel->bbox(recurse));
}

//
// Return the corners of the bounding box in pts
//
bool
BBOX::points(
   Wpt_list& pts
   ) const 
{
   pts.clear();
   if (_valid) {
      pts.realloc(8);
      Wvec d = dim();
      Wvec dx(d[0],    0,    0);
      Wvec dy(   0, d[1],    0);
      Wvec dz(   0,    0, d[2]);
      pts += _min;
      pts += (_min + dx);
      pts += (_min + dy);
      pts += (_min + dz);
      pts += (_max - dx);
      pts += (_max - dy);
      pts += (_max - dz);
      pts += _max;
   }
   return _valid;
}

//
// Does our bounding box intersect with a ray?
//
bool
BBOX::intersects(
   CRAYhit   &r, 
   CWtransf  &m
   ) const
{
   if (!_valid) {
      err_msg("BBOX::intersects: BBOX is not valid");
      return 0;
   }

   Wtransf m_inv = m.inverse();
   Wpt     p     = m_inv * r.point();
   Wvec    d     = m_inv * r.vec();

   // check yz-plane farthest from p in direction d:
   if (fabs(d[0]) > 1e-8) {
      double qx = (d[0]<0) ? _min[0] : _max[0];
      double t = (qx - p[0])/d[0];
      Wpt h = p + d*t;
      if (_min[1]<=h[1] && _max[1]>=h[1] && _min[2]<=h[2] && _max[2]>=h[2])
         return 1;
   }
   // check xz-plane farthest from p in direction d:
   if (fabs(d[1]) > 1e-8) {
      double qy = (d[1]<0) ? _min[1] : _max[1];
      double t = (qy - p[1])/d[1];
      Wpt h = p + d*t;
      if (_min[0]<=h[0] && _max[0]>=h[0] && _min[2]<=h[2] && _max[2]>=h[2])
         return 1;
   }
   // check xy-plane farthest from p in direction d:
   if (fabs(d[2]) > 1e-8) {
      double qz = (d[2]<0) ? _min[2] : _max[2];
      double t = (qz - p[2])/d[2];
      Wpt h = p + d*t;
      if (_min[0]<=h[0] && _max[0]>=h[0] && _min[1]<=h[1] && _max[1]>=h[1])
         return 1;
   }
   return 0;
}

/////////////////////////////////////////////////////////////////
// BBOX::is_off_screen()
//
//    tests whether the bbox is clearly
//    out of the view frustum
//
//    should be named BBOX::is_definitely_off_screen()
//    because if it returns true, the bbox is definitely
//    offscreen, but if it returns false the bbox may
//    or may not be offscreen.
//
/////////////////////////////////////////////////////////////////
bool      
BBOX::is_off_screen()
{
   // bounding box better be valid
   if (!valid())
      return 0;

   // return true if the bounding box is known to be offscreen.
   // to decide this, let A be the smallest cone containing 
   // the view  frustum whose tip is at the camera's eye 
   // point. let a be the angle of the cone -- i.e., between
   // its axis and a line along it.
   //
   // define a second cone B whose tip is at the camera's
   // eye point and that just contains the bounding box.
   // let b be the angle defining that cone.
   //
   // let c be the angle between the camera "at vector" and
   // the vector from the eye to the center of the bbox.
   //
   // then the bbox must be offscreen if:
   //
   //    a + b < c.
   //
   // rather than check that inequality directly,
   // we foolishly aim for greater efficiency by
   // testing the equivalent inequality:
   //
   //    cos(a + b) > cos(c)
   //
   // or
   //
   //    cos(a)cos(b) - sin(a)sin(b) > cos(c)
   //
   // taking
   //
   //   s = cos(a),
   //   t = cos(b), and
   //   u = cos(c),
   //
   // this becomes
   //
   //    s*t - sqrt((1 - s^2)(1 - t^2)) > u.
   //
   // the folowing code checks for this.

   // but first -- return false if eyepoint is
   // inside the bounding sphere of this bbox.
   // (need this to avoid taking the square root 
   //  of a negative number below).
   CCAMdataptr &camdata = VIEW::peek_cam_const()->data();
   CWpt&            eye = camdata->from();
   Wpt                o = center();
   Wvec               v = o - eye;
   double            R2 = dim().length_sqrd()/4;
   double            L2 = v.length_sqrd();
   if (L2 <= R2)
      return 0;

   // now, t = cos(b) = sqrt(R2/L2)

   // to compute s = cos(a), use:
   // s = d/sqrt(x^2 + y^2 + d^2):
   double      d = camdata->focal ();   // distance to film plane
   double      x = camdata->width ()/2; // semi-width of film plane
   double      y = camdata->height()/2; // semi-height of film plane
   double   x2y2 = sqr(x) + sqr(y);     // x^2 + y^2
   double x2y2d2 = x2y2 + sqr(d);       // x^2 + y^2 + d^2
   double   R2L2 = R2/L2;               // R2/L2

   // u = cos(c)
   double u = (camdata->at_v() * v)/sqrt(L2);

   // return true if s*t > u + sqrt((1 - s^2)(1 - t^2))
   return d*sqrt((1 - R2L2)/x2y2d2) > u + sqrt(R2L2*x2y2/x2y2d2);
}

BBOX &
BBOX::operator*=(
   CWtransf &x
   ) 
{
   if (!valid()) 
      return *this;

   Wpt_list pts(8);
   points(pts);     // Get corners of bounding box
   pts.xform(x);    // Multiply points by xform

   reset();         // Make box invalid

   update(pts);
   return *this;
}

//
// Add a point to the bounding box
//
BBOX &
BBOX::update(
   CWpt &p
   )
{
   if (!_valid) {
      _min = p;
      _max = p;
      _valid = true;
   }
   else {
      for (int i=0;i<3;i++) {
         if (p[i] < _min[i]) _min[i] = p[i];
         if (p[i] > _max[i]) _max[i] = p[i];
      }
   }

   return *this;
}

BBOX &
BBOX::update(
   CWpt_list &pts
   )
{
   for (int j = 0; j < pts.num(); j++)
      update(pts[j]);
   return *this;
}

void
BBOX::ndcz_bounding_box(
   CWtransf &obj_to_ndc, 
   NDCZpt& min_pt, 
   NDCZpt& max_pt
   ) const
{
   Wpt_list point_list;
   points(point_list);

   // convert to ndcz points
   ARRAY<NDCZpt> ndcz_list;
   int         i;
   for (i = 0; i < point_list.num(); i++) {
      ndcz_list += NDCZpt(point_list[i], obj_to_ndc);
   }

   // init min and max to sentinel values
   int coord;
   for(coord = 0; coord < 3; coord++) {
      min_pt[coord] = 1e9;
      max_pt[coord] = -1e9;
   }

   // find min and max for each coordinate
   for (i = 0; i < ndcz_list.num(); i++) {
      for (coord = 0; coord < 3; coord++) {
         if (ndcz_list[i][coord] < min_pt[coord])
            min_pt[coord] = ndcz_list[i][coord];
         else if (ndcz_list[i][coord] > max_pt[coord])
            max_pt[coord] = ndcz_list[i][coord];
      }
   }
}

// 
// BBOX2D Methods
//

BBOX2D::BBOX2D(
   CGELptr &g, 
   int      recurse
   ) : _valid(true)
{
   GELptr gel((GEL *)&*g);// sigh.. get_bb isn't const cuz it caches
   Wpt_list pts;
   gel->bbox(recurse).points(pts);
   for (int i = 0; i < pts.num(); i++)
      update(pts[i]);
}

BBOX2D &
BBOX2D::update(
   CXYpt &p
   )
{
   if (!_valid) {
      _min = p;
      _max = p;
      _valid = true;
   }
   else {
      for (int i=0;i<2;i++) {
         if (p[i] < _min[i]) _min[i] = p[i];
         if (p[i] > _max[i]) _max[i] = p[i];
      }
   }

   return *this;
}

bool
BBOX::contains(
   CWpt &p
   ) const 
{ 
   if (!_valid)
      return false;
   else
      return p[0] >= _min[0] && p[1] >= _min[1] && p[2] >= _min[2] &&
             p[0] <= _max[0] && p[1] <= _max[1] && p[2] <= _max[2];
}

bool
BBOX::overlaps(
   CBBOX &bbox
   ) const 
{ 
   if (!_valid)
      return false;
   else
      return (bbox.min()[0] <= max()[0] && bbox.max()[0] >= min()[0] &&
              bbox.min()[1] <= max()[1] && bbox.max()[1] >= min()[1] &&
              bbox.min()[2] <= max()[2] && bbox.max()[2] >= min()[2]);
}


bool
BBOX2D::overlaps(
   CBBOX2D &bbox
   ) const
{
   if (!_valid)
      return false;
   else
      return (bbox.min()[0] <= max()[0] && bbox.max()[0] >= min()[0] &&
              bbox.min()[1] <= max()[1] && bbox.max()[1] >= min()[1]);
}

bool
BBOX2D::contains(
   CXYpt &p
   ) const 
{ 
   if (!_valid)
      return false;
   else
      return p[0] >= _min[0] && p[1] >= _min[1] &&
             p[0] <= _max[0] && p[1] <= _max[1]; 
}

double
BBOX2D::dist(
   CXYpt   &p
   ) const
{
   if (!_valid)
      return -1;
   else {
      double dist = 1e9;

      if (contains(p)) {
         dist = 0;
      } else {
         // check distance to top, left, right, bottom edges
         //
         //      a -------------- b                          
         //      |                |                          
         //      |                |                          
         //      |                |                          
         //      |                |                          
         //      |                |                          
         //      c -------------- d                          
         //
         // get 4 corners: a, b, c, d
         // XXX - should add methods BBOX2D::height(), BBOX2D::width()
         // NOTE: for optimized compiles to work, 
         //       avoid calling dim()[0] or dim()[1]:
         double h = dim()*XYvec(0,1); // height
         double w = dim()*XYvec(1,0); // width

         XYpt a = _min + XYvec(0, h);
         XYpt b = _max;
         XYpt c = _min;
         XYpt d = _min + XYvec(w,0);
         // XXX - need method in mlib/line.H: Line::dist_to_seg(const P&)
         dist = ::min(dist, XYline(a,b).project_to_seg(p).dist(p));
         dist = ::min(dist, XYline(a,c).project_to_seg(p).dist(p));
         dist = ::min(dist, XYline(c,d).project_to_seg(p).dist(p));
         dist = ::min(dist, XYline(b,d).project_to_seg(p).dist(p));
      }

      return dist;
   }
}

BBOX2D &
BBOX2D::operator+=(CXYpt_list &pts)
{
   for (int i = 0; i < pts.num(); i++)
      (void) update(pts[i]);
   return *this;
}





// 
// BBOXpix Methods
//

BBOXpix &
BBOXpix::update(CPIXEL &p) {
  if (!_valid) {
    _min = p;
    _max = p;
    _valid = true;
  } else {
    for (int i=0;i<2;i++) {
      if (p[i] < _min[i]) _min[i] = p[i];
      if (p[i] > _max[i]) _max[i] = p[i];
    }
  }

  return *this;
}

bool
BBOXpix::contains(CPIXEL &p) const { 
  if (!_valid) {
    return false;
  } else {
    return (p[0] >= _min[0] && p[1] >= _min[1] &&
	    p[0] <= _max[0] && p[1] <= _max[1]);
  }
}

bool
BBOXpix::overlaps(CBBOXpix &bbox) const { 
  if (!_valid){
    return false;
  } else {
    return (bbox.min()[0] <= max()[0] && bbox.max()[0] >= min()[0] &&
	    bbox.min()[1] <= max()[1] && bbox.max()[1] >= min()[1]);
  }
}

double
BBOXpix::dist(CPIXEL   &p) const {
  if (!_valid) return -1;


  double dist = 1e9;
  
  if (contains(p)) {
    dist = 0.0;
  } else {
    // check distance to top, left, right, bottom edges
    //
    //      a -------------- b                          
    //      |                |                          
    //      |                |                          
    //      |                |                          
    //      |                |                          
    //      |                |                          
    //      c -------------- d                          
    //
    // get 4 corners: a, b, c, d
    // XXX - should add methods BBOX2D::height(), BBOX2D::width()
    // NOTE: for optimized compiles to work, 
    //       avoid calling dim()[0] or dim()[1]:
    double h = height(); // height
    double w = width(); // width
    
    PIXEL a = _min + VEXEL(0, h);
    PIXEL b = _max;
    PIXEL c = _min;
    PIXEL d = _min + VEXEL(w,0);
    // XXX - need method in mlib/line.H: Line::dist_to_seg(const P&)
    dist = ::min(dist, PIXELline(a,b).project_to_seg(p).dist(p));
    dist = ::min(dist, PIXELline(a,c).project_to_seg(p).dist(p));
    dist = ::min(dist, PIXELline(c,d).project_to_seg(p).dist(p));
    dist = ::min(dist, PIXELline(b,d).project_to_seg(p).dist(p));
  }
  
  return dist;
}


BBOXpix &
BBOXpix::operator+=(CPIXEL_list &pts) {
  for (int i = 0; i < pts.num(); i++)
    (void) update(pts[i]);
  return *this;
}

