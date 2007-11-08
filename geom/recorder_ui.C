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
#include "geom/winsys.H"


#include "recorder_ui.H"

Recorder*        	RecorderUI::_rec = 0;
RecorderUI*             RecorderUI::_instance = 0;

GLUI_Listbox*     	RecorderUI::_path_listbox 	= 0;
GLUI_EditText*     	RecorderUI::_name_edittext 	= 0;
GLUI_EditText*     	RecorderUI::_fnum_edittext 	= 0;
GLUI_Spinner*       	RecorderUI::_fps_spin 		= 0;

GLUI_Checkbox*       	RecorderUI::_check_rec 		= 0;
GLUI_Checkbox*       	RecorderUI::_check_play 	= 0;
GLUI_Checkbox*       	RecorderUI::_check_pause 	= 0;
GLUI_Checkbox*          RecorderUI::_check_sync         = 0;
GLUI_RadioGroup*       	RecorderUI::_radio_render 	= 0;
GLUI_RadioGroup*       	RecorderUI::_radio_frames 	= 0;


RecorderUI::RecorderUI(Recorder* rec) :
   _glui(0)
{
   assert(rec);

   assert(_rec == 0);  // As per the note in the header file, there should
   // be only one UI and one rec in existence, so _rec should
   // still be null.

   _rec = rec;
   _instance = this;

   init();
}

void
RecorderUI::init()
{
   //cerr << "peeks\n";
   // Get the glut main window ID from the winsys

   int main_win_id = VIEW::peek()->win()->id();

   //cerr << "windows\n";
   // Create the glui widget that will contain the rec controls
   _glui = GLUI_Master.create_glui("Recorder Control", 0);
   _glui->set_main_gfx_window(main_win_id);

   //cerr << "buttons\n";

  //recording controls 


  

  //RECORD
   GLUI_Panel* _rec_panel = _glui->add_panel ( "rec", GLUI_PANEL_NONE );

   _check_rec = _glui->add_checkbox_to_panel(_rec_panel, "", NULL, -1, 
                                             RecorderUI::button_cb );
   _check_rec->disable();
   _glui->add_column_to_panel( _rec_panel, false );
   _glui->add_button_to_panel( _rec_panel, "Record", RECORD_BUTTON_ID, 
                               RecorderUI::button_cb);

   //PLAY
   GLUI_Panel* _play_panel = _glui->add_panel ( "play", GLUI_PANEL_NONE );

   _check_play = _glui->add_checkbox_to_panel (_play_panel, "" , NULL, -1,
                                               RecorderUI::button_cb );  
   _check_play->disable();
   _glui->add_column_to_panel( _play_panel, false );
   _glui->add_button_to_panel( _play_panel, "Play", PLAY_BUTTON_ID, 
                               RecorderUI::button_cb); 


   //PAUSE
   GLUI_Panel* _pause_panel = _glui->add_panel ( "pause", GLUI_PANEL_NONE );

   _check_pause = _glui->add_checkbox_to_panel ( _pause_panel, "", NULL, -1,
                                                 RecorderUI::button_cb );
   _check_pause->disable();
   _glui->add_column_to_panel( _pause_panel, false );  
   _glui->add_button_to_panel( _pause_panel, "Pause", PAUSE_BUTTON_ID, 
                               RecorderUI::button_cb);



   GLUI_Panel* _frame_panel = _glui->add_panel ( "frame", GLUI_PANEL_NONE );


   _fnum_edittext = _glui->add_edittext_to_panel ( _frame_panel,
                                                   "Frame " , 
                                                   GLUI_EDITTEXT_INT, 
                                                   NULL, 
                                                   -1,
                                                   RecorderUI::set_framenum_cb);
  
   _check_sync  =  _glui->add_checkbox_to_panel(    _frame_panel, 
                                                    "synchronize", 
                                                    NULL, 
                                                    -1, 
                                                    RecorderUI::resync_cb );

   _glui->add_separator();

   GLUI_Panel* _step_panel = _glui->add_panel ( "step", GLUI_PANEL_NONE );

   _glui->add_button_to_panel( _step_panel, "Stop", STOP_BUTTON_ID, 
                               RecorderUI::button_cb);
   _glui->add_button_to_panel( _step_panel, "Step Rev", REV_BUTTON_ID, 
                               RecorderUI::button_cb);
   _glui->add_button_to_panel( _step_panel, "Step Fwd", FWD_BUTTON_ID, 
                               RecorderUI::button_cb);

   _glui->add_separator();
 
   GLUI_Panel* _renderpane = _glui->add_panel ( "render to disk" );

   _radio_render = _glui->add_radiogroup_to_panel ( _renderpane,  NULL, RENDER_ON_BUTTON_ID, RecorderUI::button_cb );
   assert(_radio_render);
   _glui->add_radiobutton_to_group ( _radio_render , "off" );
   _glui->add_column_to_panel      ( _renderpane, false ) ;
   _glui->add_radiobutton_to_group ( _radio_render, "on" );
   

   _glui->add_separator();

   GLUI_Panel* _framepane = _glui->add_panel ( "play all frames" );
   _radio_frames = _glui->add_radiogroup_to_panel ( _framepane,  NULL, PLAY_FRAMES_BUTTON_ID, RecorderUI::button_cb );
   _glui->add_radiobutton_to_group ( _radio_frames , "off" );
   _glui->add_column_to_panel      ( _framepane, false ) ;
   _glui->add_radiobutton_to_group ( _radio_frames, "on" );
   
   _glui->add_separator();

   _glui->add_button("Save Path", SAVE_PATH_BUTTON_ID, 
                     RecorderUI::button_cb);
   _glui->add_button("Open Path", OPEN_PATH_BUTTON_ID, 
                     RecorderUI::button_cb);
   _glui->add_button("Delete Path", DEL_PATH_BUTTON_ID, 
                     RecorderUI::button_cb);
   _glui->add_button("New Path", NEW_PATH_BUTTON_ID, 
                     RecorderUI::button_cb);

   //cerr << "edittext\n";
   _name_edittext = _glui->add_edittext ("filename",
                                         GLUI_EDITTEXT_TEXT,
                                         NULL, -1,
                                         RecorderUI::name_edittext_cb
      );

   _name_edittext->set_text("default");

   _glui->add_separator();
   // Another separator


  //cerr << "add path\n";
  //default path value
   _path_listbox = _glui->add_listbox ( "Path", 
                                        _rec->get_path(),
                                        -1, RecorderUI::path_listbox_cb);
  
   _path_listbox->add_item( -1, "no path");


   _glui->add_separator();

   //cerr << "add fps\n";


   _fps_spin = _glui->add_spinner("fps",
                                  GLUI_SPINNER_INT, 
                                  (void*) _rec->get_fps(), 
                                  -1, RecorderUI::set_fps_cb
      );
   assert(_fps_spin);

  // Set allowable width limits
   _fps_spin->set_int_limits(1, 128, GLUI_LIMIT_CLAMP);

 
    
   //cerr << "zingzang\n";
   _glui->hide();
}

void
RecorderUI::update()
{

   if(!_glui) 
      {
         cerr << "NPRPenUI::update() - Error! "
              << " No GLUI object to update (not showing)!" << endl;
      }
   else 
      {
         _glui->sync_live();
      }
}


int 
RecorderUI::add_path_entry (int id, char* text )
{ 
   return _path_listbox->add_item( id, text);
  
}

int 
RecorderUI::del_path_entry (int id) 
{
   //names are associated with a fixed id
   //so we leave a null link in the path array
   //and delete from the UI list!
   if ( id == -1 ) return 0;
   return _path_listbox->delete_item ( id );

}




void
RecorderUI::show()
{
   if(_glui) 
      _glui->show();
}


void
RecorderUI::hide()
{
   if(_glui) 
      _glui->hide();
}



void
RecorderUI::button_cb(int id)
{
   if(_rec == NULL) return;

   switch(id) {
    case(RECORD_BUTTON_ID):
      _rec->rec_record();
      break;
    case(PLAY_BUTTON_ID):
      _rec->rec_play();
      break;

    case(PAUSE_BUTTON_ID):
      _rec->rec_pause();
      break;

    case(STOP_BUTTON_ID):
      _rec->rec_stop();
      break;

    case(FWD_BUTTON_ID):
      _rec->step_fwd();
      break;

    case(REV_BUTTON_ID):
      _rec->step_rev();
      break;

    case(RENDER_ON_BUTTON_ID):
      set_render_state();
      break;

    case(PLAY_FRAMES_BUTTON_ID):
      if (_instance)
         _instance->set_play_all_frames();
      break;

    case(NEW_PATH_BUTTON_ID):
      _rec->new_path();
      break;

    case(SAVE_PATH_BUTTON_ID):
      _rec->save_path(*_rec->get_path());
      break;

    case(OPEN_PATH_BUTTON_ID):
      _rec->open_path( _name_edittext->get_text() );
      break;

    case(DEL_PATH_BUTTON_ID):
      _rec->del_path(*_rec->get_path());
      break;

    default: 
      break;
   }
}





void 
RecorderUI::path_listbox_cb(int /* id */) 
{ 
   if(_rec == NULL) return;
   int pathnum = _path_listbox->get_int_val();
   _rec->set_path(pathnum);
}

void 
RecorderUI::set_fps_cb(int /* id */) 
{ 
   if(_rec == NULL) return;
   _rec->set_fps(_fps_spin->get_int_val());
}

void 
RecorderUI::name_edittext_cb(int /* id */)
{ 
   if(_rec == NULL) return;
   _rec->_name_buf = str_ptr ( _name_edittext->get_text() );
}

void
RecorderUI::resync_cb( int /*id*/)
{
   _rec->set_sync( _check_sync->get_int_val()==1 );
   
}

void 
RecorderUI::set_framenum_cb(int /* id */)
{ 
   if(_rec == NULL) return;

   _rec->set_pos ( atoi(  _fnum_edittext->get_text() ) );
   _instance->set_frame_num(_rec->path_pos());
     
}

void
RecorderUI::set_sync() { 
   _check_sync->set_int_val( _rec->sync() );
}

void 
RecorderUI::update_checks() { 
   if ( _check_rec )   _check_rec->set_int_val    ( _rec->rec_on()           );
   if ( _check_play )  _check_play->set_int_val   ( _rec->play_on()          );
   if ( _check_pause ) _check_pause->set_int_val  ( _rec->pause_on()         );
   if ( _radio_frames ) _radio_frames->set_int_val( (int) _rec->play_all_frames()  );
}  

void 
RecorderUI::set_render_state() 
{ 

   if ( !_radio_render ) return;
   if ( _radio_render->get_int_val() ) _rec->render_on();
   else _rec->render_off();
}

void 
RecorderUI::set_play_all_frames() { 
   if ( _radio_frames ) _rec->play_all_frames() = ( _radio_frames->get_int_val()==1 );
}
