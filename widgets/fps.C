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
#include "disp/view.H"
#include "geom/world.H"
#include "std/config.H"
#include "fps.H"

static bool debug = Config::get_var_bool("DEBUG_FPS",false);

using mlib::XYpt;

inline XYpt
lr_corner_loc(double left, double up)
{
   int w=0, h=0;
   VIEW::peek()->get_size(w,h);
   if (0) {
      cerr << "width: " << w << ", height: " << h << endl;
      cerr << "location in pixels: " << mlib::PIXEL(w-left, up) << endl;
   }
   return mlib::PIXEL(w-left, up);
}

FPS::FPS()
{
   XYpt c = lr_corner_loc(50, 10);
   // ugh, the above is broken since window dimensions are not
   // available when this is called... switching to hack for now:
   c = XYpt(0.671875,-0.95); // fixed below in tick()

   _text = new TEXT2D(str_ptr("fps"), str_ptr::null, c);

   _text->set_loc(c);
   _text->set_string(str_ptr(""));

   GEOMptr text(&*_text);
   NETWORK       .set(text, 0);
   NO_COLOR_MOD  .set(text, 1);
   NO_XFORM_MOD  .set(text, 1);
   NO_DISP_MOD   .set(text, 1);
   NO_COPY       .set(text, 1);
   DONOT_CLIP_OBJ.set(text, 1);

   WORLD::create(text, false);

   _last_display = VIEW::stamp();
   _clock.set();
}

int
FPS::tick(void)
{
   HSVCOLOR hsv(VIEW::peek()->color());
   if ((hsv[2] > 0.5) && (VIEW::peek()->get_alpha() > 0.5)) {
      _text->set_color(COLOR(0,0,0));
   } else {
      _text->set_color(COLOR(1,1,1));
   }

   const int frames_drawn = (VIEW::stamp() - _last_display);
   const double secs = _clock.elapsed_time();

   if (secs > 1 && frames_drawn > 10) {
      char fps_str[128];
      double fps = frames_drawn/secs;
      if (fps < 1.0)
         sprintf(fps_str, "%5.1g fps ", fps);
      else
         sprintf(fps_str, "%5.1f fps ", fps);

      char tps_str[128] = "";

      static bool do_tps = Config::get_var_bool("JOT_SHOW_TRIS_PER_SEC",false);
      if (do_tps) {
         double tps = frames_drawn * VIEWS[0]->tris() / secs;
         if (tps < 1e3)
            sprintf(tps_str, "%5.1f tris/sec",  tps);
         else if (tps < 1e6)
            sprintf(tps_str, "%5.1fK tris/sec", tps/1e3);
         else
            sprintf(tps_str, "%5.1fM tris/sec", tps/1e6);
      }

      // XXX - the string changes frequently, soon filling up the
      // strpool hash table, which only has 128 slots
      if (_text)
         _text->set_string(str_ptr(fps_str) + tps_str);
      else
         err_msg("%s", fps);

      _last_display = VIEW::stamp();
      _clock.set();

      if (_text) {
         double dx = 1 - _text->bbox2d().max()[0];
         if (dx < 0) {
            if (debug) {
               cerr << "FPS::tick: overflowing right margin by "
                    << fabs(dx) << " units in XY space; fixing..."
                    << endl;
            }
            _text->mult_by(Wtransf::translation(Wvec(dx, 0, 0)));
         }
      }
   }

   return 0; // returning -1 removes and destroys this timer
}

/* end of file fps.C */
