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
#include "std/config.H"
#include "disp/cam_focus.H"
#include "net/stream.H"
#include "net/net_types.H"


using namespace mlib;

CAMdata::CAMdata(CAM* c) :
   _cam(c),
   _from(0,0,5),
   _at(0,0,0),
   _up(0,1,5),
   _center(0,0,0),
   _focal(1),
   _width(1),
   _height(1),
   _perspective(true),
   _tilt_up(0),
   _tilt_left(0),
   _iod(2.25), 
   _feature_scale(0.5),
   _film_normal(0,0,1),
   _film_normal_flag(false),
   _loaded_from_file(false),
   _cblist(0),
   _cached(0),
   _stamp(0)
{
   _iod = Config::get_var_dbl("IOD",2.25,true);
   changed();
}


void
CAMdata::cache() const
{
   if (!_cached) {
      CAMdata *me = (CAMdata *)this; // cast away const'ness
      me->_cached  = 1;
      me->_dist    = (_center-_from).length();
      me->_at_v    = (_at-_from).normalized();
      me->_up_v    = (_up-_from).normalized(); 
      me->_pup_v   = _up_v.orthogonalized(_at_v).normalized(); 
      me->_right_v = cross(_at_v, _pup_v).normalized();
   }
}

void
CAMdata::changed()
{
   _cached = 0;
   int i;
   for (i = LEFT; i < NUM_EYE_POSITIONS; i++) {
      _pos_mat_dirty[i] = 1;
      _proj_mat_dirty = 1;
   }
   if (_cam)
      _cam->data_changed(); // sets CAM::_ndc_proj_dirty = 1;

   ++_stamp;

   for (i=0; i<_cblist.num(); i++)
      _cblist[i]->notify(this);
}

void      
CAMdata::start_manip()
{
   for (int i=0; i<_cblist.num(); ++i)
      _cblist[i]->notify_manip_start(this);   
}
   
void
CAMdata::end_manip()
{
   for (int i=0; i<_cblist.num(); ++i)
      _cblist[i]->notify_manip_end(this);
   
}
/* -------------------------------------------------------------------------
 * DESCR   :    finds the (unit length) film plane normal vector.  It prints
 *              an error message and returns FALSE if anything goes wrong.
 *
 * RETURNS :    Success boolean.  The computed normal vetor is passed back
 *              in the argument @normal@.
 * ------------------------------------------------------------------------- */
bool
CAMdata::film_normal(
   CWvec      &at,             /* at vector                            */
   CWvec      &up,             /* up vector                            */
   Wvec       &normal          /* normal vector ( will be computed )   */
   ) const
{
   double x = tan(_tilt_left);
   double y = tan(_tilt_up);

   Wvec v1 = cross(at, up).normalized();// normalized cross product of at and up
   Wvec v2 = cross(v1, at).normalized();// normalized cross product of v1 and at

   normal = (x * v1 + y * v2 + sqrt(1 - x*x - y*y) * at).normalized();

   return true;
}


/* -------------------------------------------------------------------------
 * DESCR   :    finds the vectors that go "up" and "to the right" in the 
 *              film plane, when given the up and normal vector.
 *
 * RETURNS :    Success boolean. The computed vectors are returned in @f_up@
 *              and @f_right@, respectively.
 * ------------------------------------------------------------------------- */
bool
CAMdata::film_vectors(
   CWvec& up,           // up vector
   CWvec& normal,       // normal vector
   Wvec&  f_up,         // up vector in film plane
   Wvec&  f_right       // right vector in film plane
   ) const
{
   double dot = -(up * normal);

   // degenerate case is when normal is nearly parallel to up:
   if (fabs(dot) > 1.0-epsAbsSqrdMath()) {
      cerr << "CAMdata::film_vectors: bad dot product" << endl;
      cerr << "up:       " << up        << endl;
      cerr << "normal:   " << normal    << endl;
      cerr << "fabs_dot: " << fabs(dot) << endl;

      // why fail when we can persevere? use arbitrary up:
      f_up = normal.perpend();
      ((CAMdata*)this)->_up = _from + f_up;
   } else {
      f_up = up.orthogonalized(normal).normalized();
   }
   f_right = cross(normal, f_up).normalized();

   return  true;
}


/* -------------------------------------------------------------------------
 * DESCR   :    sets up some of the vectors needed by both public routines.
 *              It prints an error message and returns FALSE if anything 
 *              goes wrong.
 *
 * RETURNS :    Success boolean. The resulting vectors are returned in
 *              the corresponding arguments.
 * ------------------------------------------------------------------------- */
bool
CAMdata::setup_vectors(
   Wvec        &at,            /* Vector from "from" to "at"           */
   Wvec        &up,            /* Vector from "from" to "up"           */
   Wpt         &center,        /* Center point of film rectangle       */
   Wvec        &normal,        /* film plane normal                    */
   Wvec        &f_up,          /* "up" vector projected on film plane  */
   Wvec        &f_right        /* Vector to right in film plane        */
   ) const
{
   up = (_up - _from).normalized(); /* Find "up" and "at" direction vectors */
   at = (_at - _from).normalized();
   
   center = _from + _focal * at;   /* Find the center of the film area */

   // says if someone has explicitly set the film plane normal
   // bypass the film_normal function 
   if (_film_normal_flag) {
      normal = _film_normal;
      film_vectors(up, _film_normal, f_up, f_right);
   }
   else {
      film_normal(at, up, normal);
      film_vectors(up, normal, f_up, f_right);
   }

   return  true;
}

/* -------------------------------------------------------------------------
 * This returns the matrix which transforms world to eye coordinates, 
 * (see gl redbook p. 96) transformed by the given screen and eye specs
 * if needed for stereo viewing.
 *
 * The matrix returned should be loaded into the MODELVIEW stack whenever 
 * drawing is about to start. This is already done by gl_view.C. 
 *
 * This matrix is cached for each eye.
 * -------------------------------------------------------------------------
 */
CWtransf&
CAMdata::xform(SCREENptr screen, eye e) const
{
   if (!_pos_mat_dirty[e])
      return _pos_matrix[e];

   Wvec        upv;           /* Vector from "from" to "up"           */
   Wvec        atv;           /* Vector from "from" to "at"           */
   Wvec        normalv;       /* normal vector of film  plane         */
   Wvec        proj_upv;      /* "up" vector projected on film        */
   Wvec        rightv;        /* Vector to right in film plane        */

   Wpt         center;         /* Center of film area                  */
   Wtransf     rot;

   CAMdata *me     =(CAMdata *)this;

   // XXX - none of this is documented very well.
   //       what does the following code block do?
   //       we're commenting it out because changing
   //       the camera every frame is causing problems
   //       with all the camera observers who are getting
   //       fake notifications about camera changes.
   //       also commenting out the related code block
   //       below re: screen->pop_eye_offset()
   //
   // setup the camera based on the screen being used
//    if (screen) {
//       // saves current camera parameters & sets new eye offset
//       screen->push_eye_offset(_cam, e);
//    }

   // Find at, up, proj_up, and normal vectors in world space
   setup_vectors(atv, upv, center, normalv, proj_upv, rightv);

   // Translate center to origin
   // Rotate so that normal lies on negative z-axis, and proj_up is on y-axis
   rot(2, 0) = -normalv[0];
   rot(2, 1) = -normalv[1];
   rot(2, 2) = -normalv[2];

   rot(0, 0) = rightv[0];
   rot(0, 1) = rightv[1];
   rot(0, 2) = rightv[2];

   rot(1, 0) = proj_upv[0];
   rot(1, 1) = proj_upv[1];
   rot(1, 2) = proj_upv[2];

   /* Transform at vector by rotation matrix */
   atv = rot * atv;

   /* Shear so that atv lies on negative z-axis */
   me->_pos_matrix[e] = 
       Wtransf::shear(Wvec(0,0,-1), Wvec(atv[0]/atv[2], atv[1]/atv[2], 0.0)) *
       rot *
       Wtransf::translation(Wpt(0,0,0)-center);

   // XXX - commented out; see note above
   //
   // undo any modifications to the camera parameters
//    if (screen) {
//       screen->pop_eye_offset(&*_cam);
//    }

   me->_pos_mat_dirty[e] = false;

   return _pos_matrix[e];
}


/*
 * -------------------------------------------------------------------------
 * DESCR   :    returns a matrix which transforms eye coordinates to the 
 *              window coordinates. (see gl redbook p. 96)
 *              The matrix includes the perspective transformation 
 *              if the camera has the perspective flag on;
 *              otherwise it excludes the perspective transformation and 
 *              returns a matrix suitable for orthogonal viewing. All 
 *              points that lie within the view volumedefined by the camera 
 *              will have x-coordinates between @min[0]@ and @min[0] + width@, 
 *              and will have y-coordinates between @min[1]@ and 
 *              @min[1] + height@ .
 *
 *              For perspective projections, the z-coordinates of points will
 *              be zero for points on the film plane, and will be negative for
 *              points within the view volume.  After homogenization, points
 *              within the view volume will be coerced to points whose
 *              z-coordinate is between 0.0 and -1.0. The user should 
 *              clip (before homogenization) all points whose z-coordinates are
 *              positive, which will remove points on the camera side of the
 *              film plane. 
 *              ****  Above is all lies!!  See "GL" comment in code below!!!
 *
 *              For orthographic projections, the z-coordinates of points will
 *              be zero for points on the film plane, and will be +-K for
 *              points whose distance from the film plane is K.  To use a
 *              z-buffer of finite resolution, the user must decide on a range
 *              of z values to map to the z-buffer range.
 *
 *              The projection matrix is cached once computed.
 * ------------------------------------------------------------------------- 
 */

CWtransf&
CAMdata::projection_xform(
   SCREENptr   &screen,
   CNDCpt      &min,
   double       width,
   double       height,
   eye          e
   )  const
{
   if (!_proj_mat_dirty)
      return _proj_matrix;

   // setup the camera based on the screen being used

   // saves current camera parameters & sets new eye offset
   if (screen)
      screen->push_eye_offset(_cam, e);

   CAMdata *me =(CAMdata *)this;

   // Scale so that corners of film rectangle range from -1 to 1 in x and y
   me->_proj_matrix = Wtransf::scaling(Wvec(2./_width, 2./_height, 1.0));

   // Do perspective transformation taking canonical pyramid into 4-space
   if (_perspective) {
      // _film_normal_flag is true when doing off-axis projection
      double dp = (_film_normal_flag ?
                   fabs(_film_normal.normalized() * at_v()) :
                   1.0
                   );

      Wtransf persp;
      persp(0, 0) = _focal * dp;
      persp(1, 1) = _focal * dp;
      persp(3, 3) = _focal * dp;
      persp(3, 2) = -1;
      persp.set_perspective(true);

      /*    f  0  0  0
       *    0  f  0  0
       *    0  0  1  0
       *    0  0 -1  f
       * which sends [1 0 0 1] to [f 0 0 f], [0 1 0 1] to [0 f 0 f],
       * [0 0 0 1] to [0 0 0 f], and [0 0 f 1] to [0 0 f 0];
       * These last two have the effect (after homogenization) of leaving the
       * center of the film plane at the origin, and sending the eye (at
       * [0 0 f 1] out to infinity. Points with negative z coordinates
       * get sent into negative z-space. A point at one focal distance out
       * from the film plane gets sent to z = -1/2. A point way, way out in
       * negative z-space gets sent to z = -1.
       */
      me->_proj_matrix = persp * _proj_matrix;
   }

   /* translate so that the lower left hand corner of the film plane is at 0 */
   /* scale so that film plane goes from 0 to width in x, 0 to height in y */
   /* translate so that lower left hand corner of film plane is at 'min' */

#ifdef posse
   double  scaling = -1.0 / (_perspective ? 1000.0 : 1000.0); // this change okay??
#else
   double  scaling = -1.0 / (_perspective ? 1.0 : 1000.0);
#endif
   me->_proj_matrix = 
            Wtransf::translation(Wvec(1, 1, 0.0)) *
            Wtransf::scaling    (Wvec(width * .5, height * .5, scaling)) *
            Wtransf::translation(Wvec(min[0], min[1], 0.0)) *
            _proj_matrix;

   if (_perspective) {

      // ARGHH!!! hack ... 
      // 
      // we've changed our minds.  Instead we want the center of the film plane,
      // [0 0 0 1], to map to [0 0 f f].   This is because GL clips points 
      // between 1 and -1 in Z instead of 0 and -1 in Z as previously expected

      me->_proj_matrix = Wtransf::translation(Wvec(0,0,-1)) * 
               Wtransf::scaling    (Wvec(1,1,2)) * _proj_matrix;
   }

   // undo any modifications to the camera parameters
   if (screen)
      screen->pop_eye_offset(&*_cam);

   me->_proj_mat_dirty = false;

   return _proj_matrix;
}


bool
CAMdata::operator == (const CAMdata &cam) const
{
   return _from         == cam._from            && 
          _at           == cam._at              &&
          _up           == cam._up              &&
          _center       == cam._center          &&
          _focal        == cam._focal           &&
          _width        == cam._width           &&
          _height       == cam._height          &&
          _perspective  == cam._perspective     &&
          _tilt_up      == cam._tilt_up         &&
          _tilt_left    == cam._tilt_left       &&
          _iod          == cam._iod;
}

CAMdata &
CAMdata::operator= (const CAMdata &cam)
{
   // Copy values from another CAMdata, then notify
   // observers of the change.
   //
   // BUT, if the camera is identical to this, do nothing.
   // (I.e., do not notify observers about a supposed "change").

   if (!(&cam == this || cam == *this)) {
      _from        = cam._from;
      _at          = cam._at;
      _up          = cam._up;
      _center      = cam._center;
      _focal       = cam._focal;
      _width       = cam._width;
      _height      = cam._height;
      _perspective = cam._perspective;
      _tilt_up     = cam._tilt_up;
      _tilt_left   = cam._tilt_left;
      _iod         = cam._iod;
      changed();
   }
   return *this;
}

void
CAMdata::rotate(
   CWline &axis,
   double   ang
   )
{
   Wtransf combined(Wtransf::rotation(axis, ang));
   _from = combined * from();
   _at   = combined * at  ();
   _up   = combined * up  ();
   changed();
}

void
CAMdata::translate(
   CWvec &t
   )
{
   if (t.length() > 0) {
      _from   += t;
      _at     += t;
      _center += t;
      _up     += t;
      changed();
   }
}

void
CAMdata::swivel(
   double rads
   )
{
   if (rads != 0) {
      Wtransf rot(Wtransf::rotation(Wline(from(), up_v()), rads));
      _from = rot * _from;
      _at   = rot * _at;
      _up   = rot * _up;
      changed();
   }
}

STDdstream &
operator<<(STDdstream &ds, CCAMdataptr &cam) 
{
   return ds << cam->_from
            << cam->_at
            << cam->_up
            << cam->_center
            << cam->_focal
            << (int)cam->_perspective
            << cam->_iod;
}

STDdstream &
operator>>(STDdstream &ds, CAMdataptr &cam) 
{
   // Fill our camera with incoming data
   int persp;
   ds >> cam->_from
      >> cam->_at
      >> cam->_up
      >> cam->_center
      >> cam->_focal
      >> persp
      >> cam->_iod;
   // _perspective is a bool
   cam->_perspective = persp ? true : false;
   cam->_loaded_from_file = true;
   cam->changed();
   return ds;
}

void
CAM::set_aspect(
   double a
   )
{
   if ( a > 1 ) {
      _width  = 2.0/a;
      _height = 2.0;
      _min    = NDCpt(-a, -1);
   } else {
      _width  = 2.0;
      _height = 2.0*a;
      _min    = NDCpt(-1, -1/a);
   }
   _data->changed();
}

CAM &
CAM::operator = (const CAM &cam)
{
   _min   = cam._min;
   _width = cam._width;
   _height= cam._height;

   // NDC coords stuff
   _min   = cam._min;
   _width = cam._width;
   _height= cam._height;
   _zoom  = cam._zoom;
   _ndc_proj_dirty = true;

   *_data = *cam._data;
   return *this;
}

int
CAM::operator == (const CAM &cam)
{
   return _min   == cam._min  &&
          _width == cam._width && _height == cam._height &&
          *_data == *cam._data;
}

/* 
 *   This returns the vector from a point on the film plane, p,
 * to the From point of the camera.
 *
 * the point, p, should be given in NPC (normalized projection 
 * coordiantes [-1,-1] to [1,1]) 
 *
 */
Wvec
CAM::film_dir(
   CXYpt    &p
   ) const
{
   CNDCpt       ndc_p(p);
   CCAMdataptr &dat = data();
   if (!dat->persp())
      return dat->at_v();

   Wpt    fpt(dat->from() + dat->at_v() * dat->focal());
   Wvec   fr (dat->right_v() * ndc_p[0]/2 * dat->width() );
   Wvec   fu (dat->pup_v()   * ndc_p[1]/2 * dat->height());
   return ((fpt + fr + fu) - dat->from()).normalized();
}

/*
 *  This converts a point on the film plane (given in XY) to a
 * point in world space.  This is done by projecting the XY 
 * point onto a plane in world space that is parallel to the 
 * film plane and that contains the world space point, wpt.
 *
 */
Wpt 
CAM::xy_to_w(
   CXYpt     &pt,
   CWpt      &wpt
  ) const
{
   CCAMdataptr &dat = data();
   CWvec        wvec   = wpt - dat->from();
   double       length = wvec * dat->at_v();

   if (!dat->persp())               // Orthographic camera
      return dat->from() + length * dat->at_v() + 
             dat->right_v() * pt[0] / width()  * dat->width() +
             dat->pup_v()   * pt[1] / height() * dat->height();
   else {                            // Perspective camera
      Wvec rdir(pt);
      return dat->from() + rdir * (length/(rdir* dat->at_v()));
   }
}

/*
 *  Same as above, except the point is projected onto a plane 
 * parallel to the film plane that is "focal_length"+epsilon 
 * units away from the camera's From point.
 *
 */
Wpt
CAM::xy_to_w(
   CXYpt     &pt
  ) const
{
   CCAMdataptr dat = data();
   if (!dat->persp())
        return xy_to_w(pt, dat->from());
   else return xy_to_w(pt, dat->from() + dat->at_v() * dat->focal() * 1.01);
}

Wpt
CAM::xy_to_w(
   CXYpt  &pt,
   double  d
   ) const
{
   CCAMdataptr dat = data();
   if (!d)
      d = dat->distance();
   return xy_to_w(pt, dat->from() + dat->at_v() * d);
}

/*
 * Converts a point in world space to a point on the camera's film
 * plane in XY.
 */
XYpt   
CAM::w_to_xy(
   CWpt  &wpt
  ) const
{
   CCAMdataptr &dat = data();
   Wvec         wvec(wpt - dat->from());

   if (!dat->persp())
      return XYpt(wvec * dat->right_v() / dat->width () * width(),
                  wvec * dat->pup_v()   / dat->height() * height());
      
   else {
      NDCpt  aspect(XYpt(1,1)); // convert 1,1 point to NDCpt to 
                                // effectively get aspect ratio

      Wvec svec((wvec.normalized() * 
                 (wvec.length() / (wvec * dat->at_v())) - dat->at_v()) * 
                 dat->focal());

      return XYpt(svec * dat->right_v()/dat-> width() * 2/aspect[0],
                  svec * dat->pup_v  ()/dat->height() * 2/aspect[1]);
   }
}

CWtransf&
CAM::ndc_projection() const
{
   if (_ndc_proj_dirty) {
      int w, h;
      VIEW_SIZE(w,h);
      Wvec scale = (w>h) ? Wvec(w/(double)h, 1, 1) : Wvec(1, h/(double) w, 1);
      ((CAM*)this)->_ndc_proj = Wtransf::scaling(scale)*projection_xform()*xform();
      ((CAM*)this)->_ndc_proj_inv = _ndc_proj.inverse();
      ((CAM*)this)->_ndc_proj_dirty = 0;
   }
   return _ndc_proj;
}

CWtransf&
CAM::ndc_projection_inv() const
{
   // Compute the cached value
   ndc_projection();
   
   return _ndc_proj_inv;
}


void
SCREEN::push_eye_offset(
   CCAMptr &cam,
   CAMdata::eye e
   )
{
   // Minimize REFptr op's because this function is called a lot
   CAMdata *data = (CAMdata *) cam->data_ptr();

   *_saved_cam->data() = *data;

   double clip_ratio = data->focal() / (data->from() - data->at()).length();

   // save the "up vector" to redefine
   // the 'up' point after assigning a new 'from' position.
   Wvec up_vec  = data->up_v();

   double dist_to_screen = (data->from() - data->at()).length();
   data->set_focal(dist_to_screen * clip_ratio);
}

void 
SCREEN::pop_eye_offset(
   const CAM *c
   )
{
   // This is called a lot, so do not do any REFptr op's
   *((CAMdata *)c->data_ptr()) = *_saved_cam->data_ptr();
}

/* end of file cam.C */
