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
#ifndef _RECORDER_H_IS_INCLUDED_
#define _RECORDER_H_IS_INCLUDED_


#include "disp/cam.H"
#include "disp/view.H"
#include "std/stop_watch.H"
#include "std/fstream.H"

class RecorderUIBase;

class CamState { 

 protected:

   double _t;
   CAMptr _cam;
 
 public:
   CamState (double time, CCAMptr& ncam) { 
      _cam =  new CAM ( "state_cam" ) ;
      _t = time;  //time from start of path
      *_cam = *ncam;
   }
   ~CamState() { 
      _cam = 0;
   }
   double t() { return _t;}
   CAMptr cam() { return _cam; };

};


class CameraPath { 

 protected:

   str_ptr _name;

 public:

   int write_stream(iostream& fout);
   int read_stream( iostream& fin);

   ARRAY <CamState*> state_list;

   CameraPath() { 
      _name = str_ptr ("path");

   }
   ~CameraPath () { 
      for ( int i=0; i < state_list.num() ; i++ ) delete state_list[i];
   }

   CameraPath(str_ptr name ) {
      _name = name;
   }

   str_ptr get_name() { return _name; }


   void add(double time, CCAMptr& ncam) { 
      state_list += new CamState (time, ncam);
    
   }


};

class Recorder { 

 protected:

   VIEWptr _view;
   stop_watch* _swatch;
   ARRAY <CameraPath*> _campaths;
   CameraPath* _cur_path;

   int       _cur_path_num;  
   int       _fps;
   bool      _on; //recorder is on
   int       _record_on; //recorder is recording
   int       _play_on;
   int       _render_on;
   int       _paused;
   int       _path_pos;
  
   bool      _play_all_frames;
   double    _start_time;
   double    _path_time;
   double    _next_time;
  
   bool      _sync;       //maintain sync for random settings
   int       _target_frame; 

   RecorderUIBase* _ui;

 public:

   Recorder(VIEWptr view);

   ~Recorder();

   str_ptr _name_buf;

   //******** ACCESSORS ********
   RecorderUIBase* get_ui()                     const   { return _ui; }
   void            set_ui(RecorderUIBase* ui)           { _ui = ui; }
   
   /*
    * recorder functions  (glui menu items )
    */
   bool on();

   void rec_record();
   void rec_play();
   void rec_stop(); // stop record / playback
   void rec_pause();
   void step_fwd();
   void step_rev();
   void replay();

   //render out to images?
   void render_on();
   void render_off();

   // LIVE VARS FOR UI
   int& rec_on()			{ return _record_on; } 
   int& play_on() 		   { return _play_on; }
   int& rend_on() 		   { return _render_on ; }
   int& pause_on() 		{ return _paused ; }
   int& path_pos()		   { return _path_pos; }
  
   bool& sync()           { return _sync; }
   void  set_sync( bool s);
   int&  target_frame()   { return _target_frame; }


   bool& play_all_frames() 	{ return _play_all_frames; }
   // start/stop

   void activate();
   void deactivate();
   /*
    * path management 
    *
    */

   int save_path(int pathnum);
   int open_path(char * filename);

   //create/delete paths;
   int new_path();
   int del_path(int num);

  
   //set current path
   int num_paths();
   int set_path( int pathnum );
   int* get_path();

   //set path position ( frame ) ;
   void set_pos ( int pos );

   //frame rate;
   int *get_fps();
   int set_fps(int fps);

   //save/load paths to file 

   /*
    * draw loop concerns;
    */

   //callbacks before/after frame is drawn;
   void pre_draw_CB();
   void post_draw_CB();
};

/*****************************************************************
 * RecorderUIBase:
 *
 *      Base class for RecorderUI, defined in lib draw.
 *      This base class defines the API for RecorderUI,
 *      so we don't have to reference draw lib from disp.
 *****************************************************************/
class RecorderUIBase {
 public:
   //******** MANAGERS ********
   RecorderUIBase()             {}
   virtual ~RecorderUIBase()    {}

   //******** VIRTUAL METHODS ********
   virtual void show()                          = 0;
   virtual void hide()                          = 0;
    
   virtual void set_play_all_frames()           = 0;
   virtual void set_frame_num (int)             = 0;
   virtual void update_checks()                 = 0;

   virtual void sync_live()                     = 0;
  
   virtual int add_path_entry(int, char*)       = 0;
   virtual int del_path_entry(int )             = 0;
};

#endif // _RECORDER_H_IS_INCLUDED_

/* end of file recorder.H */
