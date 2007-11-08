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
// animator.C

#include "disp/animator.H"
#include "net/io_manager.H"
#include "std/config.H"

// The disp library shouldn't reference anything in the geom
// library.  (Doing so causes link problems in some apps,
// e.g. cpmesh). The following hack lets those apps compile by
// defining the symbol DONT_LINK_GEOM_IN_DISP:

#ifdef DONT_LINK_GEOM_IN_DISP
inline void
show_msg(Cstr_ptr& msg)
{
   err_msg("%s", **msg);
}
#else
#include "geom/world.H"
inline void
show_msg(Cstr_ptr& msg)
{
   WORLD::message(msg);
}
#endif

static bool debug = true; // "for now"
/*****************************************************************
 * Animator
 *****************************************************************/
TAGlist* Animator::_a_tags = 0;

Animator::Animator(VIEWptr view) :
   _view(view),
   _name("animation"),
   _fps(12),
   _start_frame(0),
   _end_frame(0),
   _cur_frame(0),
   _jog_size(10),
   _loop(false),
   _on(false),
   _play_on(false),
   _rend_on(false),
   _sync_on(false),
   _time_scale(1.0),
   _last_loaded(-1)
{
   // Since the animator is stopped, the clock must also be stopped:
   _timer.reset_hold();
}

/////////////////////////////////////
// toggle_activation()
/////////////////////////////////////
void
Animator::toggle_activation()
{
   if (!_on) {
      if (_fps <= 0) {
         show_msg("Cannot activate Animator!");
         err_adv(debug, "Animator::toggle_activation() - No fps set");
      } else if (_start_frame < 0) {
         show_msg("Cannot activate Animator!");
         err_adv(debug, "Animator::toggle_activation() - No start_frame set");
      } else if (_end_frame < _start_frame) {
         show_msg("Cannot activate Animator!");
         err_adv(debug, "Animator::toggle_activation() - Bad end_frame");
      } else if (_name == NULL_STR) {
         show_msg("Cannot activate Animator!");
         err_adv(debug, "Animator::toggle_activation() - No name set");
      } else {
         show_msg("Animator ON");
         _on = true;
         _timer.set_elapsed_time(0); // reset time to 0
      }
   } else {
      _on = false;
      show_msg("Animator OFF");
   }
}

/////////////////////////////////////
// press_beginning()
/////////////////////////////////////
void
Animator::press_beginning()
{
   _timer.set_elapsed_time(0);
   _cur_frame = _start_frame;
}

/////////////////////////////////////
// press_play()
/////////////////////////////////////
void
Animator::press_play()
{
   if (!_play_on) {
      _play_on = true;
      _timer.resume();
      show_msg("Playing...");
   } else {
      show_msg("Already Playing!");
   }
}

/////////////////////////////////////
// press_stop()
/////////////////////////////////////
void
Animator::press_stop()
{
   if (_play_on) {
      _play_on = false;
      _timer.pause();
      show_msg("Stopped");
   } else {
      show_msg("Already Stopped");
   }
}

/////////////////////////////////////
// press_render()
/////////////////////////////////////

void
Animator::press_render()
{
   if (_rend_on) {
      _rend_on = false;

      show_msg("Render to Disk Mode OFF");
   } else {
      if (_sync_on) {
         _rend_on = true;
         show_msg("Render to Disk Mode ON");
      } else {
         show_msg("Engage time sync mode first!");
      }
   }
}

/////////////////////////////////////
// press_sync()
/////////////////////////////////////
void
Animator::press_sync()
{
   if (_rend_on) {
      //Sanity check
      assert(_sync_on);
      show_msg("Disengage render to disk mode first!");
   } else {
      if (_sync_on) {
         _sync_on = false;
         show_msg("Time Sync Mode OFF");
      } else {
         _sync_on = true;
         show_msg("Time Sync Mode ON");
      }
   }
}

inline int
wrap(int a, int b, int c)
{
   // Given b <= c, if a is outside the interval [b,c], then
   // remap it inside the interval by making it "wrap around."
   // E.g.:
   //
   //   wrap(11, 3, 9) == 5
   //
   // because
   //
   //   3 + (11 - 3)%(9 - 3) == 5

   int d = c - b;
   if (d == 0)
      return b;   // b == c, so return b
   assert(d > 0); // c > b
   if (a < b) {
      return c + div(a-b,d).rem;
   } else if (a > c) {
      return b + div(a-b,d).rem;
   } else {
      return a;
   }
}

void
Animator::inc_frame(int inc)
{
   // advance the frame by the given amount, either clamping the
   // result to [start,end] or wrapping around in that interval:

   _cur_frame = _loop ?
      wrap (_cur_frame + inc, _start_frame, _end_frame) :
      clamp(_cur_frame + inc, _start_frame, _end_frame);
}

/////////////////////////////////////
// step()
/////////////////////////////////////
void
Animator::step(int inc)
{   
   char buf[1024];
   if (_play_on) {
      show_msg("Press stop before stepping");
      return;
   } else if (_sync_on) {
      // sync mode: advance the frame number by requested amount
      inc_frame(inc);
      sprintf(buf,"Frame %d of %d to %d", _cur_frame, _start_frame, _end_frame);
      show_msg(buf);
   } else {
      // real-time mode. advance the time by dt = inc / fps
      assert(_timer.is_paused());
      assert(_fps != 0);
      double dt = double(inc)/_fps;
      _timer.inc_elapsed_time(dt);
      sprintf(buf,"Advanced time by %1.3f seconds", dt);
      show_msg(buf);
      return;
   }
}

/////////////////////////////////////
// pre_draw_CB()
/////////////////////////////////////
double
Animator::pre_draw_CB()
{   
  
   char bname[1024];

   //Now make sure the desired frame's loaded
   if (_cur_frame >= 0 && _cur_frame <= _end_frame && _last_loaded != _cur_frame) {
      sprintf(bname, "%s[%05d].jot", **_name, _cur_frame);
      str_ptr fname = IOManager::cwd() + bname; 
      LOADobs::load_status_t status;
      cerr <<  "loading in " << fname << endl;
      NetStream s(fname, NetStream::ascii_r);      
      LOADobs::notify_load_obs(s, status, true, false);

      _last_loaded = _cur_frame;
   }

   //Finally adjust the frame time appropriately

   if (_sync_on) {
      return _time_scale * (_cur_frame - _start_frame) / _fps;
   } else {
      return _time_scale * _timer.elapsed_time();
   }
}

/////////////////////////////////////
// post_draw_CB()
/////////////////////////////////////

void
Animator::post_draw_CB()
{
   bool use_alpha = Config::get_var_bool("GRAB_ALPHA",false);

   if (_play_on && _rend_on) {
      // grab the current frame and write it to disk
      char buf[1024];
      sprintf (buf, "%s[%dFPS][%dto%d]-%05d.png",
               **_name, _fps, _start_frame, _end_frame, _cur_frame);
      err_msg("Animator::post_draw_CB() - Writing '%s'...", buf);

      int w,h;
      VIEW_SIZE (w,h);
      Image output (w, h, use_alpha ? 4 : 3); // 4 bytes for rgba, 3 for rgb
      VIEWimpl* impl = _view->impl();
      if (impl) {
         _view->set_grabbing_screen(1);
         impl->prepare_buf_read();
         impl->read_pixels(output.data(),use_alpha);
         _view->set_grabbing_screen(0);
         impl->end_buf_read();
      }
      if (!output.write_png(buf)) {
         err_msg("Animator::post_draw_CB() - Error writing file!");
      }
   }
   // in sync mode, advance the frame number:
   if (_sync_on && _play_on) {
      if (_cur_frame >= _end_frame && !_loop) {
         // if not looping and the last frame is finished, stop
         press_stop();
      } else {
         // otherwise, increment frame count
         // (loop to beginning if in loop mode)
         inc_frame(1);
      }
   }
}

STDdstream&
Animator::format(STDdstream &d)  const
{
   STDdstream &ret = DATA_ITEM::format(d);

   // After saving out the Animator class, check if indeed multiple
   // frames exist (i.e. animation!), and step through frame-by-frame,
   // loading each frame, and saving to the new location...

   if ( (_fps > 0) &&
        (_start_frame >=0) &&
        (_end_frame > _start_frame) &&
        (_name != NULL_STR) ) {
      cerr << "Animator::format: Found .jot file containing multiple frames."
           << endl
           << "Each frame will be loaded and resaved..." << endl;

      str_ptr bname, lname, sname;

      char buf[6];

      LOADobs::load_status_t lstatus;
      SAVEobs::save_status_t sstatus;

      for (int i = _end_frame; i >= _start_frame; i--) {
         sprintf(buf, "%05d", i);

         str_ptr bname = _name + "[" + buf + "].jot";

         str_ptr lname = IOManager::cwd() + bname;  //IOManager::cached_prefix()
         str_ptr sname = IOManager::cwd() + bname;  //IOManager::cached_prefix()

         err_msg("Animator::format() - Saving frame: '%s'...", **sname);

         //Keep scope issolated so load stream closes before saving
         //just in case we're overwriting...
         {
            NetStream l(lname, NetStream::ascii_r);
            LOADobs::notify_load_obs(l, lstatus, true, false);

            if (lstatus != LOADobs::LOAD_ERROR_NONE) {
               cerr << "Animator::format: Error loading scene update frame: "
                    << lname << ", Aborting..." << endl;
               break;
            }
         }

         {
            NetStream s(sname, NetStream::ascii_w);
            SAVEobs::notify_save_obs(s, sstatus, true, false);

            if (sstatus != SAVEobs::SAVE_ERROR_NONE) {
               cerr << "Animator::format: Error saving scene update frame: "
                    << sname << ", Aborting..." << endl;
               break;
            }
         }
      }
   }

   return ret;
}

/////////////////////////////////////
// tags()
/////////////////////////////////////
CTAGlist &
Animator::tags() const
{
   if (!_a_tags) {
      _a_tags = new TAGlist;

      *_a_tags += new TAG_val<Animator,int>(
         "fps",
         &Animator::fps_);
      *_a_tags += new TAG_val<Animator,int>(
         "start_frame",
         &Animator::start_frame_);
      *_a_tags += new TAG_val<Animator,int>(
         "end_frame",
         &Animator::end_frame_);


      *_a_tags += new TAG_meth<Animator>(
         "name",
         &Animator::put_name,
         &Animator::get_name,
         1);
   }
   return *_a_tags;
}
/////////////////////////////////////
// get_name()
/////////////////////////////////////
void
Animator::get_name (TAGformat &d)
{
   str_ptr str, space;
   *d >> str;

   if (!(*d).ascii())
      *d >> space;

   if (str == "NULL_STR") {
      _name = NULL_STR;
      err_mesg(ERR_LEV_SPAM, "Animator::get_name() - Loaded NULL string.");
   } else {
      _name = str;
      err_mesg(ERR_LEV_SPAM, "Animator::get_name() - Loaded string: '%s'.", **str);
   }

}

/////////////////////////////////////
// put_name()
/////////////////////////////////////
void
Animator::put_name (TAGformat &d) const
{
   d.id();
   if (_name == NULL_STR) {
      err_mesg(ERR_LEV_SPAM, "Animator::put_name() - Wrote NULL string.");
      *d << "NULL_STR";
      *d << " ";
   } else {
      *d << _name;
      *d << " ";
      err_mesg(ERR_LEV_SPAM, "Animator::put_name() - Wrote string: '%s'.", **_name);
   }
   d.end_id();
}

// end of file animator.C
