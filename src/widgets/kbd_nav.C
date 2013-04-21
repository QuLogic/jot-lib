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
#include "std/config.H"
#include "geom/world.H"
#include "kbd_nav.H"

using mlib::Wvec;
using mlib::CWvec;
using mlib::Wtransf;

#define STEP_LEFT     '4'
#define STEP_RIGHT    '6'
#define STEP_BACK     '2'
#define STEP_FRONT    '8'
#define STEP_UP       '1'
#define STEP_DOWN     '3'
#define STEP_TILTUP   '0'
#define STEP_TILTDOWN '5'
#define STEP_ROTLEFT  '7'
#define STEP_ROTRIGHT '9'

// XXX - should use global versions
inline double sign      (double x)   { return x>0 ? 1 : -1; }
inline double sqr_signed(double x)   { return sqr(x)*sign(x); }
inline Wvec   project   (CWvec &v1, CWvec &v2)
              { return (v1 * v2.normalized()) * v2.normalized(); }


kbd_nav::kbd_nav(
   VIEWptr   &view
   ):_data(view->cam()->data())
{
   _speed_lr = _speed_fb = _speed_ud = _speed_tilt = _speed_rot = 0;

   for (int i = 0; i < 256; i++)
      _kmap[i] = 0;
 
   for (const char *c = "1234567890"; *c; c++) {
      Event  down_ev(NULL, Evd(*c, KEYD));
      Event  up_ev  (NULL, Evd(*c, KEYU));
      _entry += Arc(up_ev,   Cb(&Key_int::up));
      _entry += Arc(down_ev, Cb(&Key_int::down));
   }
   VIEWint_list::add(view, &_entry);// get notified for each relevant key event
   WORLD::timer_callback(this);     // get called back each frame to navigate
}

int
kbd_nav::tick()
{
   static bool KBD_NAVIGATION =  Config::get_var_bool("KBD_NAVIGATION",true);
   if (!KBD_NAVIGATION)  return 0;

   // bigger number = faster acceleration:
   static double CAM_SPEED = Config::get_var_dbl("CAM_SPEED",0.1); // was 0.3

   // bigger number = faster rotational acceleration:
   static double ROT_SPEED = Config::get_var_dbl("ROT_SPEED",0.03);// was 0.15

   // bigger number = more gradual slow down:
   static double DECAY     = Config::get_var_dbl("DECAY",200.0);   // was 30
   
   double decay              = DECAY;
   int    frames_to_full     = 5;
   int    rate               = (int)(decay/frames_to_full) + 1;
   double cam_speed_hack     = CAM_SPEED;
   double cam_rot_speed_hack = ROT_SPEED;
   double coeff              = cam_speed_hack    /sqr(decay);
   double coeff_rot          = cam_rot_speed_hack/sqr(decay);
   double factor             = 9;

   if (_kmap[(int)STEP_LEFT    ]&2) step_left     (rate);
   if (_kmap[(int)STEP_RIGHT   ]&2) step_right    (rate);
   if (_kmap[(int)STEP_BACK    ]&2) step_back     (rate);
   if (_kmap[(int)STEP_FRONT   ]&2) step_front    (rate);
   if (_kmap[(int)STEP_UP      ]&2) step_up       (rate);
   if (_kmap[(int)STEP_DOWN    ]&2) step_down     (rate);
   if (_kmap[(int)STEP_TILTUP  ]&2) step_tilt_up  (rate);
   if (_kmap[(int)STEP_TILTDOWN]&2) step_tilt_down(rate);
   if (_kmap[(int)STEP_ROTLEFT ]&2) step_rot_left (rate);
   if (_kmap[(int)STEP_ROTRIGHT]&2) step_rot_right(rate);

   _speed_lr   += _speed_lr   > 0 ? -1 : (_speed_lr   < 0 ? 1 : 0);
   _speed_fb   += _speed_fb   > 0 ? -1 : (_speed_fb   < 0 ? 1 : 0);
   _speed_ud   += _speed_ud   > 0 ? -1 : (_speed_ud   < 0 ? 1 : 0);
   _speed_tilt += _speed_tilt > 0 ? -1 : (_speed_tilt < 0 ? 1 : 0);
   _speed_rot  += _speed_rot  > 0 ? -1 : (_speed_rot  < 0 ? 1 : 0);

   _speed_lr   = clamp(_speed_lr,   (int)-decay, (int)decay);
   _speed_fb   = clamp(_speed_fb,   (int)-decay, (int)decay);
   _speed_ud   = clamp(_speed_ud,   (int)-decay, (int)decay);
   _speed_tilt = clamp(_speed_tilt, (int)-decay, (int)decay);
   _speed_rot  = clamp(_speed_rot,  (int)-decay, (int)decay);

   Wvec ATV((_data->at_v() - project(_data->at_v(), Wvec::Y())).normalized());
   Wvec delt((-sqr_signed(_speed_lr)*coeff*_data->right_v() +
               sqr_signed(_speed_ud)*coeff*_data->pup_v  ()/2.0)-
               sqr_signed(_speed_fb)*coeff*ATV      );
   _data->translate(delt);

   const double rads = sqr_signed(_speed_tilt) * coeff_rot / factor;
   if (rads != 0) {
      Wtransf  xf(Wtransf::rotation(_data->right_v(), rads));
      _data->set_at(_data->from() + xf * (_data->at() - _data->from()));
   }

   _data->swivel(-sqr_signed(_speed_rot) * coeff_rot / factor);
   if (!_data->persp()) {
      _data->set_width (_data->width() + _speed_fb/1000.);
      _data->set_height(_data->height() + _speed_fb/1000.);
   }

#define DOWN_SHIFT(X) ((X & 1) + (X << 1))
   // keys have two flags in bit positions 0 & 1
   // bit 0 is the current state of the button
   // bit 1 is a "handled" flag that is always
   //       set when the key goes down, but only
   //       goes away when some reader of the _kmap
   //       decides to turn it off (i.e., when it's handled
   // in this case, I turn off the handled flag right
   // here by shifting the 0 bit into the first bit
   // position.  Thus, if the key is off, bit 0 is off,
   // and bit 1 will become off after the shift.
   // if bit 0 is on, then both bit 0 and bit 1 will
   // be on.
   _kmap[(int)STEP_LEFT    ] = DOWN_SHIFT(_kmap[(int)STEP_LEFT    ]);
   _kmap[(int)STEP_RIGHT   ] = DOWN_SHIFT(_kmap[(int)STEP_RIGHT   ]);
   _kmap[(int)STEP_BACK    ] = DOWN_SHIFT(_kmap[(int)STEP_BACK    ]);
   _kmap[(int)STEP_FRONT   ] = DOWN_SHIFT(_kmap[(int)STEP_FRONT   ]);
   _kmap[(int)STEP_UP      ] = DOWN_SHIFT(_kmap[(int)STEP_UP      ]);
   _kmap[(int)STEP_DOWN    ] = DOWN_SHIFT(_kmap[(int)STEP_DOWN    ]);
   _kmap[(int)STEP_TILTUP  ] = DOWN_SHIFT(_kmap[(int)STEP_TILTUP  ]);
   _kmap[(int)STEP_TILTDOWN] = DOWN_SHIFT(_kmap[(int)STEP_TILTDOWN]);
   _kmap[(int)STEP_ROTLEFT ] = DOWN_SHIFT(_kmap[(int)STEP_ROTLEFT ]);
   _kmap[(int)STEP_ROTRIGHT] = DOWN_SHIFT(_kmap[(int)STEP_ROTRIGHT]);
 
   return 0;
}

/* end of file kbd_nav.C */
