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
#ifndef _RECORDER_UI_H_IS_INCLUDED_
#define _RECORDER_UI_H_IS_INCLUDED_


#include "disp/recorder.H"
#include "glui/glui.h"

/*****************************************************************
 * RecorderUI
 *****************************************************************/
class RecorderUI : public RecorderUIBase {
 public:
   //******** MANAGERS ********
   RecorderUI(Recorder* rec);
   virtual ~RecorderUI() {}

   //******** ENUMS ********
   enum button_id_t {
      RECORD_BUTTON_ID,
      PLAY_BUTTON_ID,
      PAUSE_BUTTON_ID,
      STOP_BUTTON_ID,
      FWD_BUTTON_ID,
      REV_BUTTON_ID,
      RENDER_ON_BUTTON_ID,
      RENDER_OFF_BUTTON_ID,
      NEW_PATH_BUTTON_ID,
      DEL_PATH_BUTTON_ID,
      OPEN_PATH_BUTTON_ID,
      SAVE_PATH_BUTTON_ID,
      PLAY_FRAMES_BUTTON_ID,
      NUM_BUTTONS
   };

   enum color_edittext_id_t {
      NAME_EDITTEXT_ID,
      NUM_COLOR_EDITTEXT_IDS
   };

   //******** GLUI CALLBACKS ********
   static void button_cb(int id);
   static void set_fps_cb(int id);
   static void path_listbox_cb(int id);  
   static void name_edittext_cb(int id);
   static void resync_cb(int id);
   static void set_framenum_cb(int id);
   static void set_render_state();
   static void set_sync();
    
   void update();

   //******** RecorderUIBase VIRTUAL METHODS ********
   virtual void show();
   virtual void hide();
    
   virtual void set_play_all_frames();

   virtual void set_frame_num(int n) {
      if (_fnum_edittext)
         _fnum_edittext->set_int_val(n);
   } 

   virtual void sync_live() {
      if (_glui)
         _glui->sync_live() ;
   }
  
   virtual int add_path_entry(int, char*);
   virtual int del_path_entry(int );

   virtual void update_checks();

 protected:

   //******** MEMBERS ********
   GLUI*        _glui;   

   // assume that there is only one recorder per record window
   static Recorder*         _rec; 
   static RecorderUI*       _instance;  // XXX - and just one UI too

   static GLUI_Listbox*     _path_listbox;
   static GLUI_EditText*    _fnum_edittext;
   static GLUI_EditText*    _name_edittext;
   static GLUI_Spinner*     _fps_spin;
  
   static GLUI_Checkbox*        _check_rec;
   static GLUI_Checkbox*        _check_play;
   static GLUI_Checkbox*        _check_pause;
   static GLUI_Checkbox*        _check_sync;

   static GLUI_RadioGroup*  _radio_render;
   static GLUI_RadioGroup*  _radio_frames;

   void init();
};

#endif // _RECORDER_UI_H_IS_INCLUDED_

/* end of file recorder_ui.H */
