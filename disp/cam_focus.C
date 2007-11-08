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
#include "disp/cam_focus.H"
#include "std/config.H"

using namespace mlib;

// The following don't have a more logical place to go:
BaseCollide* BaseCollide::_instance = 0;
BaseGravity* BaseGravity::_instance = 0;

/*****************************************************************
 * CamFocus
 *****************************************************************/
CamFocusptr  CamFocus::_cur = 0;

// speed:    world space units per second
// duration: seconds
static const double SPEED         = Config::get_var_dbl("CAM_FOCUS_SPEED",0.5);
static const double CAM_FOCUS_DUR = Config::get_var_dbl("CAM_FOCUS_DUR",  1.5);

static const bool debug = Config::get_var_bool("DEBUG_CAM_FOCUS",false);

inline double
fdist(CCAMptr& d)
{
   // return distance between "from" and "center" points,
   // interpreted as a kind of "feature size" for the camera
   return d ? d->data()->from().dist(d->data()->center()) : 1.0;
}

CamFocus::CamFocus(
   VIEWptr v,
   CWpt&   from,
   CWpt&   at,
   CWpt&   up,
   CWpt&   center,
   double  fw,
   double  fh
   ) :
   _view(v),
   _cam(v ? v->cam() : 0),
   _width (fw == 0 ? cam()->data()->width()  : fw),
   _height(fh == 0 ? cam()->data()->height() : fh),
   _orig_time(VIEW::peek()->frame_time()),
   _last_time(_orig_time),
   _duration(CAM_FOCUS_DUR),
   _speed(SPEED),
   _max_speed(fdist(_cam)/2.0) // max speed is one basic unit per 2 seconds
{
   assert(cam() && cam()->data());

   Wpt a = at;
   if (from.is_equal(a)) {
      cerr << "CamFocus::CamFocus: ignoring zero length at vector" << endl;
      a = cam()->data()->at();
   }

   // take more time for long trips:
   double s = max(cam()->data()->from().dist(from)/_duration/_max_speed,1.0);
   err_adv(debug, "CamFocus::CamFocus: extending duration by %f", s);
   _duration *= s;

   setup(from, a, up - from, center);

   err_adv(debug, "CamFocus::CamFocus: using vectors");

   schedule();
}

CamFocus::CamFocus(VIEWptr v, CCAMptr &dest) :
   _view(v),
   _cam(v ? v->cam() : 0),
   _width(dest->data()->width()),
   _height(dest->data()->height()),
   _orig_time(VIEW::peek()->frame_time()),
   _last_time(_orig_time),
   _duration(CAM_FOCUS_DUR),
   _speed(SPEED),
   _max_speed(fdist(_cam)/2.0) // max speed is one basic unit per 2 seconds
{
   assert(dest && dest->data());
   CAMdataptr d = dest->data();
   Wpt a = d->at();
   if (d->from().is_equal(a)) {
      cerr << "CamFocus::CamFocus: ignoring zero length at vector" << endl;
      a = cam()->data()->at();
   }

   // take more time for long trips:
   double s = max(cam()->data()->from().dist(d->from())/_duration/_max_speed,1.0);
   err_adv(debug, "CamFocus::CamFocus: extending duration by %f", s);
   _duration *= s;

   setup(d->from(), a, d->up_v(), d->center());

   err_adv(debug, "CamFocus::CamFocus: using cam");

   schedule();
}

CamFocus::~CamFocus()
{
   err_adv(debug, "CamFocus::~CamFocus");

   unschedule();
}

inline Wtransf
get_mat(CWpt& o, CWpt& at, CWpt& up, CWpt& center)
{
   // create rotation matrix expressing camera orientation at target
   Wvec a = (at - o).normalized();                      // y
   Wvec u = (up - o).orthogonalized(a).normalized();    // z
   Wvec r = cross(a,u);                                 // x
   return Wtransf(r,a,u);
}

Wquat
get_quat(CWpt& o, CWpt& at, CWpt& up, CWpt& center, Wvec& a, Wvec& u, Wvec& c)
{
   Wtransf M = get_mat(o,at,up,center);
   Wtransf I = get_mat(o,at,up,center).transpose(); // same as M.inverse()

   // compute at, up, and center points in local coords:
   a = I * (at     - o);
   u = I * (up     - o);
   c = I * (center - o);
   
   return Wquat(M);
}

void
CamFocus::setup(
   CWpt& o2, CWpt& a2, CWvec& u2, CWpt& c2
   )
{
   CAMdataptr d = cam()->data();

   // camera parameters at start of animation:
   _o1 = d->from();
   _a1 = d->at();
   _u1 = d->up_v();
   _c1 = d->center();

   // camera parameters at end of animation:
   _o2 = o2;
   _a2 = a2;
   _u2 = u2;
   _c2 = c2;

   set_cur(this);
}

void
CamFocus::set_cur(CamFocus* cf)
{
   cancel_cur();
   _cur = cf;
}

void 
CamFocus::cancel_cur()
{
   if (_cur) {
      _cur->unschedule();
      _cur = 0;
   }
}

inline double
remap(double t)
{
   // add a sinuoid to remap t values so the animation
   // eases in and out, with max speed at the middle.
   // greater value of A increases the effect.

   static const double A = Config::get_var_dbl("CAM_FOCUS_REMAP_VAL",0.11);
   return t - A * sin(t * TWO_PI);
}

double 
CamFocus::cartoon_t() const
{
   return remap(t_param());
}

int
CamFocus::tick(void)
{
   CAMdataptr data(cam()->data());

   double   t = cartoon_t();

   Wpt  o = interp(_o1,_o2,t);
   Wpt  a = interp(_a1,_a2,t);
   Wvec u = interp(_u1,_u2,t);
   Wpt  c = interp(_c1,_c2,t);
   if (0 && debug) {
      cerr << "--------" << endl
           << "  this:   " << this << endl
           << "  frame:  " << VIEW::stamp() << endl
           << "  t:      " << t << endl
           << "  from:   " << o << endl
           << "  at:     " << a << endl
           << "  up:     " << u << endl
           << "  center: " << c << endl;
   }
   data->set_from  (o);
   data->set_at    (a);
   data->set_up    (o + u);
   data->set_center(c);

   // not interpolating for now:
   data->set_width (_width);
   data->set_height(_height);
   
   return 0; //(t < 1) ? 0 : -1;
}

/*****************************************************************
 * CamBreathe
 *****************************************************************/
CamBreatheptr   CamBreathe::_breathe    = 0;
double          CamBreathe::_size       = 1;

CamBreathe::CamBreathe(CAMptr  &p) :
   _cam(p),_from(p->data()->from()),_at(p->data()->at()),
   _up(Wpt()+p->data()->up_v()),_cent(p->data()->center()),
   _width(p->data()->width()),_height(p->data()->height()),
   _speed(1), kKf(0.05), kKc(0.02), kKu(0.008)
{
   _cent   = _at;
   _min    = _cam->data()->persp() ? 0.01 : 0.001;
   _width  = _cam->data()->width();
   _height = _cam->data()->height();
   _Kf     = 0;
   _Kc     = 0;
   _Ku     = 0;
   _breathe  = this;
   _tick     = 0;
   _stop     = false;
   _pause = false;
}

CamBreathe::~CamBreathe()
{
}

int
CamBreathe::tick(void)
{
   if(_stop)
      return -1;

   if(_pause)
      return 0;
    
   _speed = 2;
   //_size = 0.005 ;
  
  // cout << "Sixe: " << _size << endl;
   CAMdataptr data(_cam->data());

   Wvec upv(data->up_v());

   //"breathing" is just a simple sin wave: Sin(X*breathing speed) *
   //size of character
   data->set_from(data->from() + (sin(_tick * _speed) * upv * .005 * _size));
   data->set_at(data->at() + (sin(_tick * _speed) * upv * .005 * _size));
   data->set_up(data->up() + (sin(_tick * _speed) * upv * .005 * _size));
   data->set_center(data->at());
   data->set_width (fabs(_width));
   data->set_height(fabs(_height));

   _tick += .01;
   return 0;
}

/*****************************************************************
 * CamOrbit
 *****************************************************************/
CamOrbitptr     CamOrbit::_orbit        = 0;
CamCruiseptr    CamCruise::_cruise      = 0;

int
CamOrbit::tick(void)
{
   if(_stop)
      return -1;

   if(_pause)
      return 0;


   CAMdataptr data(_cam->data());
   data->set_center(_cent);

   XYpt        cpt   = data->center();
   double      radsq = sqr(1+fabs(cpt[0])); // squared rad of virtual cylinder

   //Toss in some XY pts
   //to simulate a mouse movement
   //that produces rotation...
   //XYpt        tp    = XYpt(0.495, 0.5); 
   // XYpt        te    = XYpt(0.5,0.5);

   Wvec   op  (tp[0], 0, 0);             // get start and end X coordinates
   Wvec   oe  (te[0], 0, 0);             //    of cursor motion
   double opsq = op * op, oesq = oe * oe;
   double lop  = opsq > radsq ? 0 : sqrt(radsq - opsq);
   double loe  = oesq > radsq ? 0 : sqrt(radsq - oesq);
   //Wvec   nop  = Wvec(op[0], 0, lop).normalized();
   //Wvec   noe  = Wvec(oe[0], 0, loe).normalized();
   Wvec   nop  = Wvec(op[0], 0, lop).normalized();
   Wvec   noe  = Wvec(oe[0], 0, loe).normalized();

   double dot  = nop * noe;

   if (fabs(dot) > 0.0001) {
      data->rotate(Wline(data->center(), Wvec::Y()),
                   -1*Acos(dot) * Sign(te[0]-tp[0]));

      CAMdata   dd = CAMdata(*data);
      data->set_up(data->from() + Wvec::Y());
      if (data->right_v() * dd.right_v() < 0)
         *data = dd;
   }


   return 0;
}

CamOrbit::~CamOrbit()
{
}

CamOrbit::CamOrbit(CAMptr  &p, mlib::Wpt center) :
   _cam(p),_from(p->data()->from()),_at(p->data()->at()),
   _up(Wpt()+p->data()->up_v()),_cent(p->data()->center()),
   _width(p->data()->width()),_height(p->data()->height()),
   _speed(1)
{
   _cent   = center;
   _min    = _cam->data()->persp() ? 0.01 : 0.001;
   _width  = _cam->data()->width();
   _height = _cam->data()->height();
   _orbit  = this;
   _tick   = 0;
   //_size   = 1;
   _pause = false;
   _stop = false;
   tp    = XYpt(0.495, 0.5);
   te    = XYpt(0.5,0.5);
}

/*****************************************************************
 * CamCruise
 *****************************************************************/
CamCruise::CamCruise(CAMptr  &p, mlib::Wpt center) :
   _cam(p),_from(p->data()->from()),_at(p->data()->at()),
   _up(Wpt()+p->data()->up_v()),_cent(p->data()->center()),
   _width(p->data()->width()),_height(p->data()->height()),
   _speed(100)
{
   _speed = .1;
   _cent   = center;
   _min    = _cam->data()->persp() ? 0.01 : 0.001;
   _width  = _cam->data()->width();
   _height = _cam->data()->height();
   _cruise  = this;
   _tick   = 0;
   _pause = true;
   _stop = false;
   _target = false;
   _travel = false;
   _start = mlib::Wpt(0,0,0);
   _dest = mlib::Wpt(0,0,0);
   tp    = XYpt(0.2, 0.5);
   te    = XYpt(0.5,0.5);
   _t    = 0;
   //size=1,height=6,regularity=20,min_dist=.35
   
}

CamCruise::~CamCruise()
{
}

int
CamCruise::tick(void)
{
   if(_stop)
      return -1;

//   if(_pause)
//      return 0;

   CAMdataptr data(_cam->data());
   //data->set_center(_cent);
   

   XYpt        cpt   = data->center();
   XYvec      delta(te-tp);
   double     ratio;



   if(_travel)
      {
      _travel = false;
      }
   else
      {
         //Collide            _collision;

         Wpt     spt  (XYpt(tp[0],te[1]));
         Wvec    svec (spt - Wline(data->from(), data->at_v()).project(spt));
 //      double  sfact(1 + delta[1]);
         Wvec velocity, movec;

			if(_pause)
				movec = Wvec(0,0,0);
			else
				movec = (data->at() - data->from());
		    
            //Wvec    gravity = BaseGravity::instance()->get_dir(data->from());
            velocity = BaseCollide::instance()->get_move(data->from(), //gravity + 
                                                         (_speed * movec.normalized() * (movec.length() * delta[1] * -4)));
        
            data->set_center(data->at());
            data->translate(velocity);
  
            ratio = data->height() / data->width() * 
               (movec * data->at_v()) * data->width() / data->focal();
            data->changed();

      }
   return 0;

}

void 
CamCruise::travel(mlib::Wpt p)
{ 
   //cout << "traveling: " << p << endl;
   _travel = true;
   _pause = true;
   _start = _cam->data()->from();
   _dest = p;
  // cout << "start: " << _start << endl; 
  // cout << "From: " << p << endl; 
   _from = _cam->data()->from();
   _at       = _cam->data()->up();
   _up       = _cam->data()->from()-_cam->data()->at_v();
   _clock.set();
}

void 
CamCruise::set_cruise(mlib::XYpt o, mlib::XYpt e)
{ 
	CAMdataptr data(_cam->data());
   _pause = false;
   tp    = o;
   te    = e;

   _move_vec  = data->at() - data->from();
}

// end of file cam_focus.C
