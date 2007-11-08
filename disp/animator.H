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
#ifndef ANIMATOR_H_IS_INCLUDED
#define ANIMATOR_H_IS_INCLUDED

/*****************************************************************
 * Animator
 *****************************************************************/

#include "disp/view.H"
#include "std/stop_watch.H"

class Animator : public DATA_ITEM { 
 private:
   /******** STATIC MEMBER VARIABLES ********/
   static TAGlist       *_a_tags;

 protected:
   /******** MEMBER VARIABLES ********/
   VIEWptr     _view;
   str_ptr     _name;

   // sync mode:
   int         _fps;            // frames per second
   int         _start_frame;    // frame number to start at
   int         _end_frame;      // frame number to end at
   int         _cur_frame;      // current frame
   int         _jog_size;       // number of frames to skip forward/back
   bool        _loop;           // tells whether animation loops after
                                //   end frame is reached
  
   // real-time mode (sync off):
   stop_watch  _timer;

   // controls:
   bool        _on;             
   bool        _play_on;
   bool        _rend_on;
   bool        _sync_on;
   double      _time_scale; // can achieve slo-mo w/ this

   int         _last_loaded;

 public:
   /******** CONSTRUCTOR/DESTRUCTOR ********/
  Animator(VIEWptr view);
  Animator() { cerr << "Animator::Animator - Dummy constructor!!!\n"; }
   ~Animator() {}

   bool   on()          const { return _on; }
   bool   play_on()     const { return _play_on; }
   bool   rend_on()     const { return _rend_on; }

   void   toggle_activation();
   void   press_render();
   void   press_sync();

   void   press_play();
   void   press_stop();
   void   press_beginning();
   void   press_step_rev() { step(-1); }
   void   press_step_fwd() { step(+1); }
   void   press_jog_rev()  { step(-_jog_size); }
   void   press_jog_fwd()  { step(+_jog_size); }

   void   set_fps(int fps)              { _fps = fps;  }
   int    get_fps()                const{ return _fps; }
   
   void   set_jog_size(int jog)         { _jog_size = jog;  }
   int    get_jog_size()           const{ return _jog_size; }   

   void   set_time_scale(double s)      { _time_scale = s;  }
   double get_time_scale()         const{ return _time_scale; }

   void   set_name(str_ptr name)        { _name = name; }

   void   set_current_frame(int f)      { _cur_frame = f;    }
   int    get_current_frame()      const{ return _cur_frame; }       

   void   set_end_frame(int f)          { _end_frame = f;    }
   int    get_end_frame()          const{ return _end_frame; } 

   void   set_loop(bool l)              { _loop = l;   }
   bool   get_loop()               const{ return _loop;}

   /** VIEW Callbacks **/
   double      pre_draw_CB();
   void        post_draw_CB();

   /******** DATA_ITEM METHODS ********/
   DEFINE_RTTI_METHODS_BASE("Animator", CDATA_ITEM*);
   virtual DATA_ITEM    *dup() const      { return new Animator; }
   virtual CTAGlist     &tags() const;
   virtual STDdstream   &format(STDdstream &d)  const;

   /******** I/O Access Methods ********/
 protected:
   int&                 fps_()            { return _fps;          }
   int&                 start_frame_()    { return _start_frame;  }
   int&                 end_frame_()      { return _end_frame;    }

   /******** I/O Methods ********/
   virtual void         get_name (TAGformat &d);
   virtual void         put_name (TAGformat &d) const;
  
   /******** MEMBER METHODS ********/
   void        step(int inc);

   int num_frames() const { return _end_frame - _start_frame; }
   void inc_frame(int inc);
};

typedef const Animator CAnimator;

#endif // ANIMATOR_H_IS_INCLUDED

// end of file animator.H
