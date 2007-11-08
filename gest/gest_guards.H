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
 * draw_fsa.H
 **********************************************************************/
#ifndef DRAW_FSA_H_HAS_BEEN_INCLUDED
#define DRAW_FSA_H_HAS_BEEN_INCLUDED

#include "geom/fsa.H"

#include "gesture.H"

#ifdef WIN32
#ifdef DrawState
#undef DrawState
#endif
#define DrawState DrawStateWIN
#endif
typedef Guard_t <GESTUREptr>  DrawGuard;
typedef Arc_t   <GESTUREptr>  DrawArc;
typedef State_t <GESTUREptr>  DrawState;
typedef FSAT    <GESTUREptr>  DrawFSA;

class GarbageGuard : public DrawGuard {
 public:
   // When the tablet flakes out we get 3000 pixel/sec strokes that
   // end 6 feet beyond the edge of the screen. We don't want
   // those. Normal gestures are under 500 pixels/sec, with some
   // slashes reaching 1000 or so.
   virtual int exec(CGESTUREptr& g) { return g && g->speed() > 2000; }
};

class TapGuard : public DrawGuard {
 public:
   virtual int exec(CGESTUREptr& g) { return g && g->is_tap(); }
};

class DslashGuard : public DrawGuard {
 public:
   virtual int exec(CGESTUREptr& g) { return g && g->is_dslash(); }
};

class TslashGuard : public DrawGuard {
 public:
   virtual int exec(CGESTUREptr& g) { return g && g->is_tslash(); }
};

class SlashTapGuard : public DrawGuard {
 public:
   virtual int exec(CGESTUREptr& g) { return g && g->is_slash_tap(); }
};

class DoubleTapGuard : public DrawGuard {
 public:
   virtual int exec(CGESTUREptr& g) { return g && g->is_double_tap(); }
};

class ZipZapGuard : public DrawGuard {
 public:
   virtual int exec(CGESTUREptr& g) { return g && g->is_zip_zap(); }
};

class ArcGuard : public DrawGuard {
 public:
   virtual int exec(CGESTUREptr& g) { return g && g->is_arc(); }
};

class SmallArcGuard : public DrawGuard {
 public:
   virtual int exec(CGESTUREptr& g) { return g && g->is_small_arc(); }
};

class DotGuard : public DrawGuard {
 public:
   virtual int exec(CGESTUREptr& g) { return g && g->is_dot(); }
};

class ScribbleGuard : public DrawGuard {
 public:
   virtual int exec(CGESTUREptr& g) { return g && g->is_scribble(); }
};

class LassoGuard : public DrawGuard {
 public:
   virtual int exec(CGESTUREptr& g) { return g && g->is_lasso(); }
};

class CircleGuard : public DrawGuard {
 public:
   CircleGuard(double r=1.25) : _max_ratio(max(1.0, r)) {}
   void set_max_ratio(double r) { _max_ratio = max(1.0, r); }

   virtual int exec(CGESTUREptr& g) { return g && g->is_circle(_max_ratio); }

 protected:
   double       _max_ratio;
};

class SmallCircleGuard : public DrawGuard {
 public:
   SmallCircleGuard(double r=1.25) : _max_ratio(max(1.0, r)) {}
   void set_max_ratio(double r) { _max_ratio = max(1.0, r); }

   virtual int exec(CGESTUREptr& g) { return g && g->is_small_circle(_max_ratio); }

 protected:
   double       _max_ratio;
};

class EllipseGuard : public DrawGuard {
 public:
   virtual int exec(CGESTUREptr& g) { return g && g->is_ellipse(); }
};

class LineGuard : public DrawGuard {
 public:
   virtual int exec(CGESTUREptr& g) {
      return (g && g->is_line() && !g->is_slash());
   }
};

class XGuard : public DrawGuard {
 public:
   virtual int exec(CGESTUREptr& g) { return g && g->is_x(); }
};

class SlashGuard : public DrawGuard {
 public:
   virtual int exec(CGESTUREptr& g) { return g && g->is_slash(); }
};

class ClickHoldGuard : public DrawGuard {
 public:
   virtual int exec(CGESTUREptr& g) { return g && g->is_click_hold(); }
};

class StrokeGuard : public DrawGuard {
 public:
   virtual int exec(CGESTUREptr& g) {
      return (g && g->is_stroke() && !g->is_slash() && !g->is_tap());
   }
};

#endif  // DRAW_FSA_H_HAS_BEEN_INCLUDED

// end of file draw_fsa.H
