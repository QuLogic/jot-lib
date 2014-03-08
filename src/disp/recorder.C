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
#include <fstream>
#include "disp/recorder.H"

static string recorder_image_path = Config::get_var_str("RECORDER_IMAGE_PATH","imagedir/");

int 
CameraPath::write_stream( iostream& os ) { 

   STDdstream out(&os);
   out << state_list.size();
   vector<CamState*>::size_type i;
   for ( i = 0 ; i < state_list.size() ; i++ ) {
      out << state_list[i]->t();
      out << state_list[i]->cam()->data();
   }
   return 1;
}

int 
CameraPath::read_stream ( iostream& is ) { 

   STDdstream in(&is);
   int fnum;
   double ftime;
   CAMptr cptr = make_shared<CAM>("state");
   in >> fnum;
   for ( int i=0; i < fnum; i++ ) { 
      cerr << "frame " << i << endl;
      in >> ftime;
      CAMdataptr cd = cptr->data(); // XXX - temp variable to avoid warnings
      in >> cd;                 //          here
      state_list.push_back(new CamState ( ftime, cptr ));
   }
   return 1;
}

Recorder::Recorder(VIEWptr view) : 
   _view(view),
   _swatch(new stop_watch()),
   _cur_path(nullptr),
   _cur_path_num(-1),
   _fps(24),
   _on(false),
   _record_on(false), 
   _play_on(false), 
   _render_on(false), 
   _paused(false),
   _path_pos(0), 
   _start_time(0),
   _path_time(0),
   _next_time(0),
   _sync(false),
   _ui(nullptr)
{  
}


Recorder::~Recorder() 
{ 
   delete _ui;
   vector<CameraPath*>::size_type i;
   for ( i=0; i < _campaths.size() ; i++ )
      delete _campaths[i];
   _view = nullptr;
}


void
Recorder::activate() 
{
   assert (_ui);
  
   _ui->show();
  
   _on = true;
   _swatch->set();
  
}       

void
Recorder::deactivate() 
{
  
   if ( _ui) _ui->hide();
   _on = false;
  
}



// power button

bool
Recorder::on() { return _on; }

 
void 
Recorder::rec_record() 
{ 
   _swatch->set();
   _record_on = true;
   _render_on = false;


   if ( _paused && _play_on ) {    //stop during playback to overwrite
      _swatch->set(_start_time);
      _paused = false;
   } 

   _play_on = false;
   _next_time = 0;
   _start_time =0;
}

// play button
 
void 
Recorder::rec_play()
{

   _play_on     = true;
   _record_on   = false;
   _target_frame = _cur_path->state_list.size()-1;
   _swatch->set();
  
   _paused      = false;
  
   cerr << "\nsetplay\n";
}
void
Recorder::replay() { 

   //
   // set proper environment to
   // replay things exactly
   //
   //   
   //

   _path_pos = 0;
   _ui->set_frame_num( _path_pos );

   _play_all_frames = true;
   _ui->set_play_all_frames();

   //start playing again
      
   _play_on = true;
   _swatch->set();
   _paused  = false;
   _ui->update_checks();
}

void 
Recorder::set_sync(bool s) {  //accessor for the ui  
   if ( s && !_sync ) { 
      _target_frame = _path_pos;
      replay();
   }
   else if ( !s && _sync ) { 
      _path_time = _cur_path->state_list[_path_pos]->t();
   }
   _sync = s;
}
//step forward / back
void 
Recorder::step_fwd()
{
   //using the step functions shifts you into play all frames mode
   _play_all_frames=true;
  
   _play_on     = true;
   _paused      = false;
   _target_frame = min ( _path_pos+1 , (int)_cur_path->state_list.size()-1 );
   _path_pos = _target_frame;
   _ui->set_frame_num ( _path_pos );
   _path_time   = _cur_path->state_list[_path_pos]->t();
   _start_time  = _path_time;

   cerr << "step fwd to: " << _path_pos << "\n";
}

void 
Recorder::step_rev()
{
   //using the step functions shifts you into play all frames mode
   _play_all_frames=true;
  
   _play_on     = true;
   _paused      = false;
   _target_frame = max (_path_pos-1 , 0 );
   _path_pos = _target_frame;
   _ui->set_frame_num ( _path_pos );

   _path_time   = _cur_path->state_list[_path_pos]->t();
   _start_time  = _path_time;

   //can't go 'backward' in sync mode 
   //start from the beginning and 
   //work forward 
   if ( _sync )  replay();
  

   cerr << "step rev to: " << _path_pos << "\n";
}


// stop everything, time to zero, 
void 
Recorder::rec_stop()
{
   _play_on           = false;
   _record_on      = false;
   _render_on      = false;
   _paused         = false;
   _path_pos       = 0;
   _path_time      = 0;
   _target_frame   = _cur_path->state_list.size()-1 ;
   cerr << "\nstop\n";
} 

// pause record or playback -- hit pause again to resume.
//  REC OR PLAY WILL RESET, NOT UNPAUSE

void 
Recorder::rec_pause()
{
   if (!_paused) { // when you pause, preserve the time you paused at 
      _start_time = _swatch->elapsed_time();
   }
   else {       // when you unpause, restore to that time.
      _swatch->set( _start_time );
   }
   _paused = !_paused;
}

//render out to images?
void 
Recorder::render_on()
{
   _render_on   =       true;
}

void Recorder::render_off()
{
   _render_on   =       false;
}


int 
Recorder::new_path() 
{ 

   string filename = ( _name_buf != "" ) ? _name_buf : "path";
   _campaths.push_back ( new CameraPath( filename ));
   int id       = _campaths.size()-1;
   _ui->add_path_entry ( id, filename.c_str() );
   _cur_path_num = id;
   _cur_path    = _campaths[_cur_path_num];
   _path_pos    = 0;
   _path_time   = 0;
   _ui->sync_live();
   return 1;
}


int 
Recorder::del_path(int k) 
{ 
   delete _campaths[k];
   _campaths[k] = nullptr;
   _cur_path_num = 1;
   _cur_path = nullptr;
   _ui->sync_live();
   set_path(-1);
   return _ui->del_path_entry(k);
  
}


int 
Recorder::save_path ( int pathnum ) { 
  
   if ( _campaths.size() == 0 || pathnum < 0 || pathnum >= (int)_campaths.size() ) return 0;
 
   CameraPath * cpath = _campaths[pathnum];
   fstream fout;
   fout.open ( cpath->get_name().c_str(), ios::out );
   if ( !fout )
      {
         err_ret("Recorder::save_path: error writing file");
      }
   int tmp = cpath->write_stream ( fout );
   fout.close();
   err_msg("wrote %s", cpath->get_name().c_str() );
   return tmp;
}

int 
Recorder::open_path ( const char * filename ) {
   cerr << "Recorder::open_path  - - Nothin yet" << endl;
   cerr << "filename is " << filename << endl; 
   fstream fin;
#if (defined (WIN32) && defined(_MSC_VER) && (_MSC_VER <=1300)) /*VS 6.0*/
   fin.open ( filename, ios::in | ios::nocreate);
#else
   fin.open ( filename, ios::in );
#endif
   if ( !fin )
      {
         err_ret("Recorder::open_path: error opening file");
      }
   CameraPath * tmp = new CameraPath ( filename );
   if ( !tmp->read_stream (fin )) return 0;
   fin.close();
   _campaths.push_back ( tmp );
   _ui->add_path_entry ( _campaths.size()-1, filename );
   //set the opened path to be the current path
   cerr << "num is " << _campaths.size() << endl;
   set_path ( _campaths.size()-1 ) ;
   cerr << "end open " << endl;

   return 1;
}


//set current path
int 
Recorder::num_paths() 
{ 
   return _campaths.size();
}
  
int 
Recorder::set_path( int pathnum ) 
{ 
   if ( pathnum < 0 || pathnum >= (int)_campaths.size() ) {
      cerr << "illegal path value:" << pathnum  ;
      _cur_path = nullptr;
      _cur_path_num = -1;
      _ui->sync_live();
      return -1;
   }
   _cur_path    = _campaths[pathnum];
   _cur_path_num = pathnum;
   _path_pos    = 0;
   _path_time   = 0;
   _ui->sync_live();
   return _cur_path_num;

}


int *
Recorder::get_path () { 
   return &_cur_path_num;
}
 
//frame rate;
  
int* Recorder::get_fps() { return &_fps; }

int Recorder::set_fps(int fps) { return (_fps = fps); }  

//save/load paths to file 

/*
 * draw loop concerns;
 */


//callbacks before/after frame is drawn;

void
Recorder::set_pos(int pos) 
{ 
   if ( pos > 0 && pos < (int)_cur_path->state_list.size() ) {
      //setting pos enables play all frames
      _play_all_frames     = true;

      if ( _sync ) { 
         if ( pos > _path_pos) { 
            target_frame()      = pos ;  
            _play_on                    = true;
            _paused                     = false;
         } else if ( pos < _path_pos ) { 
            target_frame() = pos ;
            replay();
            _paused                     = false;
         }
      } else { 
         _target_frame     = pos;
         _path_pos         = pos; 
         _play_on          = true;
         _paused           = false;
      }
   }
} 

void 
Recorder::pre_draw_CB()
{
   //set/get time, cam_pos, etc...
   CAM tmp("temp");
   
   if ( !_cur_path )
      return;

   if ( !_paused ) { 

      if ( _sync && ( _path_pos > _target_frame ) )
         replay();
      
      double ttime = _swatch->elapsed_time();
      
      if ( _play_on )  { 

         //forward to closest path after current time
         if ( _path_pos >= (int)_cur_path->state_list.size() ||
              _cur_path->state_list.empty() ) {
            cerr << "end of state list reached::camera stop" << endl;
            rec_stop() ; 
            return; 
         } 

         if ( _render_on || _play_all_frames ) {

            CAMptr mine = (_cur_path->state_list[_path_pos]->cam());          
            _view->cam()->data()->set_from ( mine->data()->from() );
            _view->cam()->data()->set_at ( mine->data()->at() );
            _view->cam()->data()->set_up ( mine->data()->up() );
            _view->cam()->data()->set_center ( mine->data()->center() );
            _view->cam()->data()->set_focal ( mine->data()->focal() );
            _view->cam()->data()->set_persp ( mine->data()->persp() );
            _view->cam()->data()->set_iod ( mine->data()->iod() );
            _view->set_frame_time( _cur_path->state_list[_path_pos]->t());
            
            //update UI to reflect which frame is being shown
            _ui->set_frame_num ( _path_pos );
            
            if ( _sync ) { 
               if ( _path_pos == _target_frame )   { 
                  cerr << "at target frame: pausing:" << endl ; 
                  rec_pause(); 
               } else if ( _path_pos > _target_frame ) { 
                  cerr << "ack, we need to set back"<< endl; 
                  replay(); 
               } else _path_pos++;
            } else { 
               if ( _path_pos >= (int)_cur_path->state_list.size() -1 ) rec_stop();
               else if ( _path_pos >= _target_frame ) rec_pause();
               else _path_pos++;
            }   
         } else {                
            while ( _cur_path->state_list[_path_pos]->t() < ttime ) { 
               _path_pos++;
               if ( _path_pos >= (int)_cur_path->state_list.size()) {
                  rec_stop(); 
                  return;
               }
            }
            
            CAMptr mine = (_cur_path->state_list[_path_pos]->cam());          
            _view->cam()->data()->set_from ( mine->data()->from() );
            _view->cam()->data()->set_at ( mine->data()->at() );
            _view->cam()->data()->set_up ( mine->data()->up() );
            _view->cam()->data()->set_center ( mine->data()->center() );
            _view->cam()->data()->set_focal ( mine->data()->focal() );
            _view->cam()->data()->set_persp ( mine->data()->persp() );
            _view->cam()->data()->set_iod ( mine->data()->iod() );
            
            _view->set_frame_time( _cur_path->state_list[_path_pos]->t());
            _ui->set_frame_num ( _path_pos );
         }
         //set view camera to stored camera
      } else if ( _record_on ) { 
         while ( ttime >= _next_time ) {
            _cur_path->add( _next_time, _view->cam()); //always adds to end of list
            _path_pos = _cur_path->state_list.size()-1;
            _next_time += 1.0/_fps; //set next time to get data
            
         }       
      }
   } else if ( _paused && _play_on) {
      if ( _sync && ( _path_pos > _target_frame )  ) {
         replay();
         _paused = true; 
      }
      
      if ( _path_pos < 0 ||
           _path_pos >= (int)_cur_path->state_list.size() ||
           _cur_path->state_list.empty() ) {
         rec_stop();
         return;
      }
   }

   _ui->update_checks();
}
  
void 
Recorder::post_draw_CB()
{
   if ( !_paused && _render_on && _play_on) { 

      /*make directory */

      char num[32]; //yes, this is absurd
      sprintf (num, "%06d", _path_pos); //1 million frames

      //mkdir("imagedir");
      string base_dir = recorder_image_path;
      string filename = base_dir + _cur_path->get_name() +
         "_" + num + ".png";
      cerr << "writing " << filename << "\n";
      int w,h; VIEW_SIZE (w,h);
      Image output (w,h,3);
      //sketchy/

      VIEWimpl* impl = _view->impl();

      if (impl) { 
         _view->set_grabbing_screen(1);
         impl->prepare_buf_read();
         impl->read_pixels(output.data());
         _view->set_grabbing_screen(0);
         impl->end_buf_read();
      }

      if ( !output.write_png(filename.c_str())) {cerr << "error writing file!"; }
  
   }
}

