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
////////////////////////////////////////////
// SilUI
////////////////////////////////////////////

#define ID_SHIFT                 10
#define ID_MASK                  ((1<<ID_SHIFT)-1)

#define DEFAULT_TRACK_STROKES    1
#define DEFAULT_ZOOM_STROKES     0

#define BUFFER_NAME_W            180
#define BUFFER_BUTTON_W          75

#define BUFFER_DIRECTORY         "nprdata/debugging/"

#include "std/support.H"
#include "glew/glew.H"

#include "geom/winsys.H"
#include "geom/world.H"
#include "tess/tex_body.H"
#include "glui/glui.h"
#include "std/stop_watch.H"
#include "npr/npr_texture.H"
#include "std/config.H"

#include "sil_ui.H"

/*****************************************************************
 * SilUI
 *****************************************************************/

/////////////////////////////////////
// Static variables
/////////////////////////////////////

TAGlist*                SilUI::_sui_tags = 0;
ARRAY<SilUI*>           SilUI::_ui;
HASH                    SilUI::_hash(16);

/////////////////////////////////////
// tags()
/////////////////////////////////////

CTAGlist &
SilUI::tags() const
{
   if (!_sui_tags) {
      _sui_tags = new TAGlist;
        
      *_sui_tags += new TAG_meth<SilUI>(
         "path",
         &SilUI::put_paths,
         &SilUI::get_paths,
         1);
      *_sui_tags += new TAG_meth<SilUI>(
         "selected_gel",
         &SilUI::put_selected_gel,
         &SilUI::get_selected_gel,
         1);
      *_sui_tags += new TAG_meth<SilUI>(
         "selected_patch",
         &SilUI::put_selected_patch,
         &SilUI::get_selected_patch,
         1);
      *_sui_tags += new TAG_meth<SilUI>(
         "vote_path_index",
         &SilUI::put_vote_path_index,
         &SilUI::get_vote_path_index,
         1);
      *_sui_tags += new TAG_meth<SilUI>(
         "stroke_path_index",
         &SilUI::put_stroke_path_index,
         &SilUI::get_stroke_path_index,
         1);
      *_sui_tags += new TAG_meth<SilUI>(
         "vote_index",
         &SilUI::put_vote_index,
         &SilUI::get_vote_index,
         1);

      *_sui_tags += new TAG_val<SilUI,int>(
         "always_update",
         &SilUI::always_update_);
      *_sui_tags += new TAG_val<SilUI,int>(
         "sigma_one",
         &SilUI::sigma_one_);
      *_sui_tags += new TAG_val<SilUI,int>(
         "fit_type",
         &SilUI::fit_type_);
      *_sui_tags += new TAG_val<SilUI,int>(
         "cover_type",
         &SilUI::cover_type_);
      *_sui_tags += new TAG_val<SilUI,float>(
         "fit_pix",
         &SilUI::fit_pix_);
      *_sui_tags += new TAG_val<SilUI,float>(
         "weight_fit",
         &SilUI::weight_fit_);
      *_sui_tags += new TAG_val<SilUI,float>(
         "weight_scale",
         &SilUI::weight_scale_);
      *_sui_tags += new TAG_val<SilUI,float>(
         "weight_distort",
         &SilUI::weight_distort_);
      *_sui_tags += new TAG_val<SilUI,float>(
         "weight_heal",
         &SilUI::weight_heal_);

   }
   return *_sui_tags;
}

/////////////////////////////////////
// put_paths()
/////////////////////////////////////
void
SilUI::put_paths(TAGformat &d) const
{
   cerr << "SilUI::put_paths()" << endl;


   ZXedgeStrokeTexture *zb = ((NPRTexture *)(((TEXBODY*)&*_bufferGEL)->cur_rep()->patches()[0]->
                                    find_tex(NPRTexture::static_name())))->
                                       stroke_tex()->sil_and_crease_tex()->zx_edge_tex();
   LuboPathList&  plb = zb->paths();

   d.id();
   plb.format(*d);
   d.end_id();

}

/////////////////////////////////////
// get_paths()
/////////////////////////////////////
void
SilUI::get_paths(TAGformat &d)
{
   cerr << "SilUI::get_paths() - ";

   ZXedgeStrokeTexture *zb = ((NPRTexture *)(((TEXBODY*)&*_bufferGEL)->cur_rep()->patches()[0]->
                                    find_tex(NPRTexture::static_name())))->
                                       stroke_tex()->sil_and_crease_tex()->zx_edge_tex();
   LuboPathList&  plb = zb->paths();

   //Grab the class name... should be LuboPathList
   str_ptr str;
   *d >> str;      

   if ((str != LuboPathList::static_name())) {
      // XXX - should throw away stuff from unknown obj
      cerr << "SilUI::get_paths() - Not LuboPathList: '" << str << "'" << endl; return; }

   plb.delete_all();
   plb.decode(*d);

   zb->regen_paths_notify();
   zb->regen_group_notify();

}

/////////////////////////////////////
// put_selected_gel()
/////////////////////////////////////
void
SilUI::put_selected_gel(TAGformat &d) const
{
   cerr << "SilUI::put_selected_gel()" << endl;

   int index = _listbox[LIST_MESH]->get_int_val();

   d.id();
   *d << index;
   d.end_id();

}

/////////////////////////////////////
// get_selected_gel()
/////////////////////////////////////
void
SilUI::get_selected_gel(TAGformat &d)
{
   cerr << "SilUI::get_selected_gel()\n";

   int index;

   *d >> index;      

   update_mesh_list();

   if (index > _mesh_names.num())
   {
      cerr << "SilUI::get_selected_gel() - ERROR!!! GEL index was bad!\n";
      index = 0;  
   }

   _listbox[LIST_MESH]->set_int_val(index);

   select_name();
}

/////////////////////////////////////
// put_selected_patch()
/////////////////////////////////////
void
SilUI::put_selected_patch(TAGformat &d) const
{
   cerr << "SilUI::put_selected_patch()" << endl;

   int index;

   if (_selectedGEL)
   {
      CBMESHptr b = ((TEXBODY*)&*_selectedGEL)->cur_rep();   assert(b != NULL);
      CPatch_list &pl = b->patches();
      index = (_selectedPatch)?(pl.get_index(_selectedPatch)):(-1);
   }
   else
      index = -1;

   d.id();
   *d << index;
   d.end_id();

}

/////////////////////////////////////
// get_selected_patch()
/////////////////////////////////////
void
SilUI::get_selected_patch(TAGformat &d)
{
   cerr << "SilUI::get_selected_patch()\n";

   int index;

   *d >> index;      

   if (_selectedGEL)
   {
      CBMESHptr b = ((TEXBODY*)&*_selectedGEL)->cur_rep(); assert(b != NULL);
      CPatch_list &pl = b->patches();
      assert(index < pl.num());

      deselect_patch();
      if (index != -1)  select_patch(pl[index]);
   }

}

/////////////////////////////////////
// put_vote_path_index()
/////////////////////////////////////
void
SilUI::put_vote_path_index(TAGformat &d) const
{
   cerr << "SilUI::put_vote_path_index()" << endl;

   d.id();
   *d << _votePathIndex;
   d.end_id();

}

/////////////////////////////////////
// get_vote_path_index()
/////////////////////////////////////
void
SilUI::get_vote_path_index(TAGformat &d)
{
   cerr << "SilUI::get_vote_path_index()\n";

   int index;

   *d >> index;      

   clear_votepath_index();

   if (index != -1)
   {
      assert(_observedTexture);

      ZXedgeStrokeTexture *z = _observedTexture->stroke_tex()->sil_and_crease_tex()->zx_edge_tex();
      const LuboPathList&  pl = z->paths();

      assert(index < pl.num());

      _votePathIndex = index;
      _votePathIndexStamp = z->path_stamp();

   }
}


/////////////////////////////////////
// put_stroke_path_index()
/////////////////////////////////////
void
SilUI::put_stroke_path_index(TAGformat &d) const
{
   cerr << "SilUI::put_stroke_path_index()" << endl;

   d.id();
   *d << _strokePathIndex;
   d.end_id();

}

/////////////////////////////////////
// get_stroke_path_index()
/////////////////////////////////////
void
SilUI::get_stroke_path_index(TAGformat &d)
{
   cerr << "SilUI::get_stroke_path_index()\n";

   int index;

   *d >> index;      

   clear_strokepath_index();

   if (index != -1)
   {
      assert(_observedTexture);
      assert(_votePathIndex != -1);

      ZXedgeStrokeTexture *z = _observedTexture->stroke_tex()->sil_and_crease_tex()->zx_edge_tex();
      const LuboPathList&  pl = z->paths();

      assert(index < pl[_votePathIndex]->groups().num());

      _strokePathIndex = index;
      _strokePathId = pl[_votePathIndex]->groups()[_strokePathIndex].id();
      _strokePathIndexStamp = z->group_stamp();         
      assert(_votePathIndexStamp == _strokePathIndexStamp);
   }
}

/////////////////////////////////////
// put_vote_index()
/////////////////////////////////////
void
SilUI::put_vote_index(TAGformat &d) const
{
   cerr << "SilUI::put_vote_index()" << endl;

   d.id();
   *d << _voteIndex;
   d.end_id();

}

/////////////////////////////////////
// get_vote_index()
/////////////////////////////////////
void
SilUI::get_vote_index(TAGformat &d)
{
   cerr << "SilUI::get_vote_index()\n";

   int index;

   *d >> index;      

   clear_vote_index();

   if (index != -1)
   {
      assert(_observedTexture);
      assert(_votePathIndex != -1);
      assert(_strokePathIndex != -1);

      ZXedgeStrokeTexture *z = _observedTexture->stroke_tex()->sil_and_crease_tex()->zx_edge_tex();
      const LuboPathList&  pl = z->paths();

      assert(index < pl[_votePathIndex]->groups()[_strokePathIndex].num() );

      _voteIndex = index;
   }
}

/////////////////////////////////////
// Constructor
/////////////////////////////////////

SilUI::SilUI(VIEWptr v) :
   _selectedGEL(NULL),
   _selectedPatch(NULL),
   _observedTexture(NULL),
   _votePathIndex(-1),
   _votePathIndexStamp(0),
   _strokePathId(-1),
   _strokePathIndex(-1),
   _strokePathIndexStamp(0),
   _voteIndex(-1),
   _lastDrawStamp(0),
   _id(0),
   _init(false),
   _view(v),
   _glui(0)
{
       
   _ui += this;
   _id = (_ui.num()-1);
    
   // Defer init() until the first build()

   WORLD::get_world()->schedule(this);

   _always_update = Config::get_var_bool("ALWAYS_UPDATE",false)?(1):(0);
   _sigma_one     = Config::get_var_bool("SIGMA_ONE",false)?(1):(0);

   _fit_type = (Config::get_var_bool("FIT_SIGMA",false))?(RADBUT_FIT_SIGMA):
                                             ((Config::get_var_bool("FIT_PHASE",false))?(RADBUT_FIT_PHASE):
                                                                            ((Config::get_var_bool("FIT_INTERPOLATE",false))?(RADBUT_FIT_INTERPOLATE):
                                                                                                                 (RADBUT_FIT_OPTIMIZE)));


   _cover_type = ((Config::get_var_bool("COVER_MAJORITY",false))?(RADBUT_COVER_MAJORITY):
                                                 (Config::get_var_bool("COVER_ONE_TO_ONE",false)?(RADBUT_COVER_ONE_TO_ONE):
                                                                                   (RADBUT_COVER_HYBRID))) - RADBUT_COVER_MAJORITY;

   _fit_pix        = min(max(Config::get_var_dbl("FIT_PIX",48.0,true),6.0),200.0);
   _weight_fit     = min(max(Config::get_var_dbl("WEIGHT_FIT",1.0,true),0.0),100.0);
   _weight_scale   = min(max(Config::get_var_dbl("WEIGHT_SCALE",1.0,true),0.0),100.0);
   _weight_distort = min(max(Config::get_var_dbl("WEIGHT_DISTORT",1.0,true),0.0),100.0);
   _weight_heal    = min(max(Config::get_var_dbl("WEIGHT_HEAL",1.0,true),0.0),100.0);


   BMESHptr b = new BMESH();
   b->Icosahedron();
   assert(b->patches().num() == 1);
   b->patch(0)->get_tex(NPRTexture::static_name());

   _bufferGEL = new TEXBODY(b);
   ((GEOM*)&*_bufferGEL)->set_name("Data Buffer");

}

/////////////////////////////////////
// Destructor
/////////////////////////////////////

SilUI::~SilUI()
{
   // XXX - Need to clean up? Nah, we never destroy these

   WORLD::get_world()->unschedule(this);

   cerr << "SilUI::~SilUI - Error!!! Destructor not implemented.\n";
}

/////////////////////////////////////
// init()
/////////////////////////////////////
void
SilUI::init()
{
   assert(!_init);

   _init = true;

}

/////////////////////////////////////
// fetch() - Implicit Constructor
/////////////////////////////////////

SilUI*
SilUI::fetch(CVIEWptr& v)
{
   if (!v) {
      err_msg("SilUI::fetch() - Error! view is nil");
      return 0;
   }
   if (!v->impl()) {
      err_msg("SilUI::fetch() - Error! view->impl() is nil");
      return 0;
   }

   // hash on the view implementation rather than the view itself

   SilUI* sui = (SilUI*)_hash.find((long)v->impl());

   if (!sui)
      {
         sui = new SilUI(v); 
         assert(sui); 
         _hash.add((long)v->impl(), (void *)sui);

      }

   return sui;
}

/////////////////////////////////////
// is_vis() 
/////////////////////////////////////

bool
SilUI::is_vis(CVIEWptr& v)
{
   
   SilUI* sui;

   if (!(sui = SilUI::fetch(v)))
   {
      err_msg("SilUI::show - Error! Failed to fetch SilUI!");
      return false;
   }

   return sui->internal_is_vis();

}


/////////////////////////////////////
// show() 
/////////////////////////////////////

bool
SilUI::show(CVIEWptr& v)
{
   
   SilUI* sui;

   if (!(sui = SilUI::fetch(v)))
      {
         err_msg("SilUI::show - Error! Failed to fetch SilUI!");
         return false;
      }

   if (!sui->internal_show())
      {
         err_msg("SilUI::show() - Error! Failed to show SilUI!");
         return false;
      }
   else 
      {
         err_msg("SilUI::show() - SilUI sucessfully showed SilUI.");
         return true;
      }
        
}

/////////////////////////////////////
// hide() 
/////////////////////////////////////

bool
SilUI::hide(CVIEWptr& v)
{
   
   SilUI* sui;

   if (!(sui = SilUI::fetch(v)))
      {
         err_msg("SilUI::hide - Error! Failed to fetch SilUI!");
         return false;
      }

   if (!sui->internal_hide())
      {
         err_msg("SilUI::hide() - Error! Failed to hide SilUI!");
         return false;
      }
   else 
      {
         err_msg("SilUI::hide() - SilUI sucessfully hid SilUI.");
         return true;
      }
        
}

/////////////////////////////////////
// update() 
/////////////////////////////////////

bool
SilUI::update(CVIEWptr& v)
{
   
   SilUI* sui;

   if (!(sui = SilUI::fetch(v)))
      {
         err_msg("SilUI::update - Error! Failed to fetch SilUI!");
         return false;
      }

   if (!sui->internal_update())
      {
         err_msg("SilUI::update() - Error! Failed to update SilUI!");
         return false;
      }
   else 
      {
         err_msg("SilUI::update() - ViewUI sucessfully updated SilUI.");
         return true;
      }
        
}

/////////////////////////////////////
// internal_is_vis()
/////////////////////////////////////

bool
SilUI::internal_is_vis()
{
   if (_glui) 
   {
      return true;
   } 
   else 
   {
      return false;
   }
}



/////////////////////////////////////
// internal_show()
/////////////////////////////////////

bool
SilUI::internal_show()
{
   if (_glui) 
   {
      cerr << "SilUI::internal_show() - Error! SilUI is "
           << "already showing!"
           << endl;
      return false;
   } 
   else 
   {
      build();
      
      assert(_init);

      if (!_glui) 
      {
         cerr << "SilUI::internal_show() - Error! SilUI "
              << "failed to build GLUI object!"
              << endl;
         return false;
      } 
      else 
      {
         _glui->show();

         // Update the controls that don't use
         // 'live' variables
         update_non_lives();

         _glui->sync_live();

         return true;
      }
   }
}


/////////////////////////////////////
// internal_hide()
/////////////////////////////////////

bool
SilUI::internal_hide()
{
   if(!_glui) 
   {
      cerr << "SilUI::internal_hide() - Error! SilUI is "
           << "already hidden!"
           << endl;
      return false;
   }
   else
   {
      _listbox[LIST_MESH]->set_int_val(0);
      select_name();

      _glui->hide();

      assert(_init);
 
      destroy();

      assert(!_glui);

      return true;
   }

}

/////////////////////////////////////
// internal_update()
/////////////////////////////////////
//
// -Forces GLUI to look at live variables
//  and repost the widgets
//
/////////////////////////////////////

bool
SilUI::internal_update()
{

   if(!_glui) 
   {
      cerr << "SilUI::internal_update() - Error! "
           << " No GLUI object to update (not showing)!"
           << endl;
      return false;
   }
   else 
   {
      // Update the controls that don't use
      // 'live' variables
      update_non_lives();

      _glui->sync_live();

      return true;
   }

}


/////////////////////////////////////
// build()
/////////////////////////////////////

void
SilUI::build() 
{
   int i;
   int id = _id << ID_SHIFT;

   assert(!_glui);

   int root_x, root_y, root_w, root_h;
   _view->win()->size(root_w,root_h);
   _view->win()->position(root_x,root_y);

   _glui = GLUI_Master.create_glui("Sil Debugging", 0, root_x + root_w + 10, root_y);
   _glui->set_main_gfx_window(_view->win()->id());

   //Init the control arrays
   assert(_button.num()==0);        for (i=0; i<BUT_NUM; i++)        _button.add(0);
   assert(_slider.num()==0);        for (i=0; i<SLIDE_NUM; i++)      _slider.add(0);
   assert(_listbox.num()==0);       for (i=0; i<LIST_NUM; i++)       _listbox.add(0);
   assert(_panel.num()==0);         for (i=0; i<PANEL_NUM; i++)      _panel.add(0);
   assert(_rollout.num()==0);       for (i=0; i<ROLLOUT_NUM; i++)    _rollout.add(0);
   assert(_graph.num()==0);         for (i=0; i<GRAPH_NUM; i++)      _graph.add(0);
   assert(_text.num()==0);          for (i=0; i<TEXT_NUM; i++)       _text.add(0);
   assert(_edittext.num()==0);      for (i=0; i<EDITTEXT_NUM; i++)   _edittext.add(0);
   assert(_checkbox.num()==0);      for (i=0; i<CHECK_NUM; i++)      _checkbox.add(0);
   assert(_radgroup.num()==0);      for (i=0; i<RADGROUP_NUM; i++)   _radgroup.add(0);
   assert(_radbutton.num()==0);     for (i=0; i<RADBUT_NUM; i++)     _radbutton.add(0);

   assert(_mesh_names.num() == 0);
   assert(_buffer_filenames.num() == 0);

   //Top panel
   _panel[PANEL_TOP] = _glui->add_panel("Selection");
   assert(_panel[PANEL_TOP]);

   //Mesh Panel
   _panel[PANEL_MESH] = _glui->add_panel_to_panel(
      _panel[PANEL_TOP],
      "Mesh");
   assert(_panel[PANEL_MESH]);

   //Mesh selector
   _listbox[LIST_MESH] = _glui->add_listbox_to_panel(
      _panel[PANEL_MESH], 
      "", NULL,
      id+LIST_MESH, listbox_cb);
   assert(_listbox[LIST_MESH]);

   _listbox[LIST_MESH]->add_item(0, "----");
   _listbox[LIST_MESH]->set_int_val(0);
   
   _glui->add_column_to_panel(_panel[PANEL_TOP],false);

   //Patch Panel
   _panel[PANEL_PATCH] = _glui->add_panel_to_panel(
      _panel[PANEL_TOP],
      "Patch");
   assert(_panel[PANEL_PATCH]);

   //Patch text
   _text[TEXT_PATCH] = _glui->add_statictext_to_panel(
      _panel[PANEL_PATCH],
      "XX of YY");
   assert(_text[TEXT_PATCH]);
   _text[TEXT_PATCH]->set_w(_text[TEXT_PATCH]->string_width("XX of YY"));

   _glui->add_column_to_panel(_panel[PANEL_PATCH],false);

   //Patch
   _button[BUT_PATCH_NEXT] = _glui->add_button_to_panel(
      _panel[PANEL_PATCH],
      "Next",
      id+BUT_PATCH_NEXT,
      button_cb);
   assert(_button[BUT_PATCH_NEXT]);
   _button[BUT_PATCH_NEXT]->set_w(50);

   _glui->add_column_to_panel(_panel[PANEL_TOP],false);

   //Rate
   _slider[SLIDE_RATE] = _glui->add_slider_to_panel(
      _panel[PANEL_TOP], 
      "Rate", 
      GLUI_SLIDER_INT, 
      1, 100,
      NULL,
      id+SLIDE_RATE, slider_cb);
   assert(_slider[SLIDE_RATE]);
   _slider[SLIDE_RATE]->set_num_graduations(100);

   _glui->add_column_to_panel(_panel[PANEL_TOP],false);

   //Refresh Panel
   _panel[PANEL_REFRESH] = _glui->add_panel_to_panel(
      _panel[PANEL_TOP],
      "Hack");
   assert(_panel[PANEL_REFRESH]);

   //Refresh
   _button[BUT_REFRESH] = _glui->add_button_to_panel(
      _panel[PANEL_REFRESH],
      "Refresh",
      id+BUT_REFRESH,
      button_cb);
   assert(_button[BUT_REFRESH]);
   _button[BUT_REFRESH]->set_w(70);

   //Buffer rollout
   _rollout[ROLLOUT_BUFFER] = _glui->add_rollout("Data Buffer",true);
   assert(_rollout[ROLLOUT_BUFFER]);

   //Grab button
   _button[BUT_BUFFER_GRAB] = _glui->add_button_to_panel(
      _rollout[ROLLOUT_BUFFER],
      "Grab",
      id+BUT_BUFFER_GRAB,
      button_cb);
   assert(_button[BUT_BUFFER_GRAB]);
   _button[BUT_BUFFER_GRAB]->disable();
   _button[BUT_BUFFER_GRAB]->set_w(BUFFER_BUTTON_W);

   //Save button
   _button[BUT_BUFFER_SAVE] = _glui->add_button_to_panel(
      _rollout[ROLLOUT_BUFFER],
      "Save",
      id+BUT_BUFFER_SAVE,
      button_cb);
   assert(_button[BUT_BUFFER_SAVE]);
   _button[BUT_BUFFER_SAVE]->disable();
   _button[BUT_BUFFER_SAVE]->set_w(BUFFER_BUTTON_W);

   _glui->add_column_to_panel(_rollout[ROLLOUT_BUFFER],false);

   //Buffer save list
   _listbox[LIST_BUFFER] = _glui->add_listbox_to_panel(
      _rollout[ROLLOUT_BUFFER],
      "", NULL,
      id+LIST_BUFFER, listbox_cb);
   assert(_listbox[LIST_BUFFER]);
   _listbox[LIST_BUFFER]->disable();
   _listbox[LIST_BUFFER]->set_w(BUFFER_NAME_W);
   _listbox[LIST_BUFFER]->add_item(0, "-=<NEW>=-");
   _listbox[LIST_BUFFER]->set_int_val(0);
   fill_buffer_listbox(_listbox[LIST_BUFFER], _buffer_filenames, Config::JOT_ROOT() + BUFFER_DIRECTORY);

   //Buffer name editor
   _edittext[EDITTEXT_BUFFER_NAME] = _glui->add_edittext_to_panel(
      _rollout[ROLLOUT_BUFFER], "", 
      GLUI_EDITTEXT_TEXT, NULL, 
      id+EDITTEXT_BUFFER_NAME, edittext_cb);
   assert(_edittext[EDITTEXT_BUFFER_NAME]);
   _edittext[EDITTEXT_BUFFER_NAME]->disable();
   _edittext[EDITTEXT_BUFFER_NAME]->set_w(BUFFER_NAME_W);

   _glui->add_column_to_panel(_rollout[ROLLOUT_BUFFER],false);

   //Buffer misc text 1
   _edittext[EDITTEXT_BUFFER_MISC1] = _glui->add_edittext_to_panel(
      _rollout[ROLLOUT_BUFFER], "", 
      GLUI_EDITTEXT_TEXT, NULL, 
      id+EDITTEXT_BUFFER_MISC1, edittext_cb);
   assert(_edittext[EDITTEXT_BUFFER_MISC1]);
   _edittext[EDITTEXT_BUFFER_MISC1]->disable();
   _edittext[EDITTEXT_BUFFER_MISC1]->set_w(10);

   //Buffer misc text 2
   _edittext[EDITTEXT_BUFFER_MISC2] = _glui->add_edittext_to_panel(
      _rollout[ROLLOUT_BUFFER], "", 
      GLUI_EDITTEXT_TEXT, NULL, 
      id+EDITTEXT_BUFFER_MISC2, edittext_cb);
   assert(_edittext[EDITTEXT_BUFFER_MISC2]);
   _edittext[EDITTEXT_BUFFER_MISC2]->disable();
   _edittext[EDITTEXT_BUFFER_MISC2]->set_w(10);

   //Options rollout
   _rollout[ROLLOUT_OPTIONS] = _glui->add_rollout("Options",true);
   assert(_rollout[ROLLOUT_OPTIONS]);

   //Stroke tracking/path track
   _checkbox[CHECK_TRACK_STROKES] = _glui->add_checkbox_to_panel(
      _rollout[ROLLOUT_OPTIONS],
      "Track Strokes",
      NULL,
      id+CHECK_TRACK_STROKES,
      checkbox_cb);
   assert(_checkbox[CHECK_TRACK_STROKES]);
   _checkbox[CHECK_TRACK_STROKES]->set_int_val(DEFAULT_TRACK_STROKES);

   //Stroke zoom
   _checkbox[CHECK_ZOOM_STROKES] = _glui->add_checkbox_to_panel(
      _rollout[ROLLOUT_OPTIONS],
      "Zoom Strokes",
      &_zoom_strokes,
      id+CHECK_ZOOM_STROKES,
      checkbox_cb);
   assert(_checkbox[CHECK_ZOOM_STROKES]);
   _checkbox[CHECK_ZOOM_STROKES]->set_int_val(DEFAULT_ZOOM_STROKES);

   //Always update
   _checkbox[CHECK_ALWAYS_UPDATE] = _glui->add_checkbox_to_panel(
      _rollout[ROLLOUT_OPTIONS],
      "Always Update",
      &_always_update,
      id+CHECK_ALWAYS_UPDATE,
      checkbox_cb);
   assert(_checkbox[CHECK_ALWAYS_UPDATE]);
   _checkbox[CHECK_ALWAYS_UPDATE]->set_int_val((Config::get_var_bool("ALWAYS_UPDATE",false))?(1):(0));

   //Always update
   _checkbox[CHECK_SIGMA_ONE] = _glui->add_checkbox_to_panel(
      _rollout[ROLLOUT_OPTIONS],
      "Sigma=1",
      &_sigma_one,
      id+CHECK_SIGMA_ONE,
      checkbox_cb);
   assert(_checkbox[CHECK_SIGMA_ONE]);
   _checkbox[CHECK_SIGMA_ONE]->set_int_val((Config::get_var_bool("SIGMA_ONE",false))?(1):(0));

   _glui->add_column_to_panel(_rollout[ROLLOUT_OPTIONS],false);
 
   //Fit-type radio buttons
   _radgroup[RADGROUP_FIT] = _glui->add_radiogroup_to_panel(
      _rollout[ROLLOUT_OPTIONS],
      &_fit_type,
      id+RADGROUP_FIT, radiogroup_cb);
   assert(_radgroup[RADGROUP_FIT]);

   _radbutton[RADBUT_FIT_SIGMA] = _glui->add_radiobutton_to_group(
      _radgroup[RADGROUP_FIT],
      "Sigma");
   assert(_radbutton[RADBUT_FIT_SIGMA]);

   _radbutton[RADBUT_FIT_PHASE] = _glui->add_radiobutton_to_group(
      _radgroup[RADGROUP_FIT],
      "Phase");
   assert(_radbutton[RADBUT_FIT_PHASE]);

   _radbutton[RADBUT_FIT_INTERPOLATE] = _glui->add_radiobutton_to_group(
      _radgroup[RADGROUP_FIT],
      "Interp");
   assert(_radbutton[RADBUT_FIT_INTERPOLATE]);

   _radbutton[RADBUT_FIT_OPTIMIZE] = _glui->add_radiobutton_to_group(
      _radgroup[RADGROUP_FIT],
      "Optim");
   assert(_radbutton[RADBUT_FIT_PHASE]);

   //_radgroup[RADGROUP_FIT]->set_int_val(RADBUT_FIT_OPTIMIZE);

   _glui->add_column_to_panel(_rollout[ROLLOUT_OPTIONS],false);



   //Cover-type radio buttons
   _radgroup[RADGROUP_COVER] = _glui->add_radiogroup_to_panel(
      _rollout[ROLLOUT_OPTIONS],
      &_cover_type,
      id+RADGROUP_COVER, radiogroup_cb);
   assert(_radgroup[RADGROUP_COVER]);

   _radbutton[RADBUT_COVER_MAJORITY] = _glui->add_radiobutton_to_group(
      _radgroup[RADGROUP_COVER],
      "Maj");
   assert(_radbutton[RADBUT_COVER_MAJORITY]);

   _radbutton[RADBUT_COVER_ONE_TO_ONE] = _glui->add_radiobutton_to_group(
      _radgroup[RADGROUP_COVER],
      "1to1");
   assert(_radbutton[RADBUT_COVER_ONE_TO_ONE]);

   _radbutton[RADBUT_COVER_HYBRID] = _glui->add_radiobutton_to_group(
      _radgroup[RADGROUP_COVER],
      "Hyb");
   assert(_radbutton[RADBUT_COVER_HYBRID]);

   //_radgroup[RADGROUP_COVER]->set_int_val(RADBUT_COVER_HYBRID - RADBUT_COVER_MAJORITY);

   _glui->add_column_to_panel(_rollout[ROLLOUT_OPTIONS],false);



   //Pix fit
   _slider[SLIDE_FIT_PIX] = _glui->add_slider_to_panel(
      _rollout[ROLLOUT_OPTIONS], 
      "Pix", 
      GLUI_SLIDER_FLOAT, 
      6, 200,
      &_fit_pix,
      id+SLIDE_FIT_PIX, slider_cb);
   assert(_slider[SLIDE_FIT_PIX]);
   _slider[SLIDE_FIT_PIX]->set_num_graduations(195);


   _glui->add_column_to_panel(_rollout[ROLLOUT_OPTIONS],false);

   //Energy weights
   _slider[SLIDE_WEIGHT_FIT] = _glui->add_slider_to_panel(
      _rollout[ROLLOUT_OPTIONS], 
      "Wf", 
      GLUI_SLIDER_FLOAT, 
      0, 100,
      &_weight_fit,
      id+SLIDE_WEIGHT_FIT, slider_cb);
   assert(_slider[SLIDE_WEIGHT_FIT]);
   _slider[SLIDE_WEIGHT_FIT]->set_num_graduations(201);


   _glui->add_column_to_panel(_rollout[ROLLOUT_OPTIONS],false);

   _slider[SLIDE_WEIGHT_SCALE] = _glui->add_slider_to_panel(
      _rollout[ROLLOUT_OPTIONS], 
      "Ws", 
      GLUI_SLIDER_FLOAT, 
      0, 100,
      &_weight_scale,
      id+SLIDE_WEIGHT_SCALE, slider_cb);
   assert(_slider[SLIDE_WEIGHT_SCALE]);
   _slider[SLIDE_WEIGHT_SCALE]->set_num_graduations(201);


   _glui->add_column_to_panel(_rollout[ROLLOUT_OPTIONS],false);

   _slider[SLIDE_WEIGHT_DISTORT] = _glui->add_slider_to_panel(
      _rollout[ROLLOUT_OPTIONS], 
      "Wd", 
      GLUI_SLIDER_FLOAT, 
      0, 100,
      &_weight_distort,
      id+SLIDE_WEIGHT_DISTORT, slider_cb);
   assert(_slider[SLIDE_WEIGHT_DISTORT]);
   _slider[SLIDE_WEIGHT_DISTORT]->set_num_graduations(201);


   _glui->add_column_to_panel(_rollout[ROLLOUT_OPTIONS],false);

   _slider[SLIDE_WEIGHT_HEAL] = _glui->add_slider_to_panel(
      _rollout[ROLLOUT_OPTIONS], 
      "Wh", 
      GLUI_SLIDER_FLOAT, 
      0, 100,
      &_weight_heal,
      id+SLIDE_WEIGHT_HEAL, slider_cb);
   assert(_slider[SLIDE_WEIGHT_HEAL]);
   _slider[SLIDE_WEIGHT_HEAL]->set_num_graduations(201);



   //Path rollout
   _rollout[ROLLOUT_PATH] = _glui->add_rollout("Path",true);
   assert(_rollout[ROLLOUT_PATH]);

   //Path info panel
   _panel[PANEL_PATH_INFO] = _glui->add_panel_to_panel(
      _rollout[ROLLOUT_PATH],
      "");
   assert(_panel[PANEL_PATH_INFO]);

   _text[TEXT_PATH] = _glui->add_statictext_to_panel(
      _panel[PANEL_PATH_INFO],
      "Howdy");
   assert(_text[TEXT_PATH]);

   //Path panel
   _panel[PANEL_PATH] = _glui->add_panel_to_panel(
      _rollout[ROLLOUT_PATH],
      "Path Selection");
   assert(_panel[PANEL_PATH]);

   //Path buttons
   _button[BUT_PATH_PREV] = _glui->add_button_to_panel(
      _panel[PANEL_PATH],
      "Prev",
      id+BUT_PATH_PREV,
      button_cb);
   assert(_button[BUT_PATH_PREV]);

   _glui->add_column_to_panel(_panel[PANEL_PATH],false);

   _button[BUT_PATH_NEXT] = _glui->add_button_to_panel(
      _panel[PANEL_PATH],
      "Next",
      id+BUT_PATH_NEXT,
      button_cb);
   assert(_button[BUT_PATH_NEXT]);

   _graph[GRAPH_PATH] = _glui->add_graph_to_panel(
      _rollout[ROLLOUT_PATH],
      "Word",
      id+GRAPH_PATH,
      graph_cb);
   assert(_graph[GRAPH_PATH]);

   //Seg rollout
   _rollout[ROLLOUT_SEG] = _glui->add_rollout("Seg",true);
   assert(_rollout[ROLLOUT_SEG]);

   //Seg info panel
   _panel[PANEL_SEG_INFO] = _glui->add_panel_to_panel(
      _rollout[ROLLOUT_SEG],
      "");
   assert(_panel[PANEL_SEG_INFO]);

   _text[TEXT_SEG] = _glui->add_statictext_to_panel(
      _panel[PANEL_SEG_INFO],
      "Howdy");
   assert(_text[TEXT_SEG]);


   //Seg panel
   _panel[PANEL_SEG] = _glui->add_panel_to_panel(
      _rollout[ROLLOUT_SEG],
      "Segment Selection");
   assert(_panel[PANEL_SEG]);

   //Seg buttons
   _button[BUT_SEG_PREV] = _glui->add_button_to_panel(
      _panel[PANEL_SEG],
      "Prev",
      id+BUT_SEG_PREV,
      button_cb);
   assert(_button[BUT_SEG_PREV]);

   _glui->add_column_to_panel(_panel[PANEL_SEG],false);

   _button[BUT_SEG_NEXT] = _glui->add_button_to_panel(
      _panel[PANEL_SEG],
      "Next",
      id+BUT_SEG_NEXT,
      button_cb);
   assert(_button[BUT_SEG_NEXT]);

   _graph[GRAPH_SEG] = _glui->add_graph_to_panel(
      _rollout[ROLLOUT_SEG],
      "Pimp",
      id+GRAPH_SEG,
      graph_cb);
   assert(_graph[GRAPH_SEG]);

   //Vote rollout
   _rollout[ROLLOUT_VOTE] = _glui->add_rollout("Vote",true);
   assert(_rollout[ROLLOUT_VOTE]);

   //Vote info panel
   _panel[PANEL_VOTE_INFO] = _glui->add_panel_to_panel(
      _rollout[ROLLOUT_VOTE],
      "");
   assert(_panel[PANEL_VOTE_INFO]);

   _text[TEXT_VOTE] = _glui->add_statictext_to_panel(
      _panel[PANEL_VOTE_INFO],
      "Howdy");
   assert(_text[TEXT_VOTE]);

   //Vote panel
   _panel[PANEL_VOTE] = _glui->add_panel_to_panel(
      _rollout[ROLLOUT_VOTE],
      "Vote Selection");
   assert(_panel[PANEL_VOTE]);

   //Vote buttons
   _button[BUT_VOTE_PREV] = _glui->add_button_to_panel(
      _panel[PANEL_VOTE],
      "Prev",
      id+BUT_VOTE_PREV,
      button_cb);
   assert(_button[BUT_VOTE_PREV]);

   _glui->add_column_to_panel(_panel[PANEL_VOTE],false);

   _button[BUT_VOTE_NEXT] = _glui->add_button_to_panel(
      _panel[PANEL_VOTE],
      "Next",
      id+BUT_VOTE_NEXT,
      button_cb);
   assert(_button[BUT_VOTE_NEXT]);

   //Vote data panel
   _panel[PANEL_VOTE_DATA] = _glui->add_panel_to_panel(
      _rollout[ROLLOUT_VOTE],
      "Marzipan");
   assert(_panel[PANEL_VOTE_DATA]);

   //Vote data text lines
   _text[TEXT_VOTE_1] = _glui->add_statictext_to_panel(
      _panel[PANEL_VOTE_DATA],
      "");
   assert(_text[TEXT_VOTE_1]);

   _text[TEXT_VOTE_2] = _glui->add_statictext_to_panel(
      _panel[PANEL_VOTE_DATA],
      "");
   assert(_text[TEXT_VOTE_2]);

   _text[TEXT_VOTE_3] = _glui->add_statictext_to_panel(
      _panel[PANEL_VOTE_DATA],
      "");
   assert(_text[TEXT_VOTE_3]);

   _text[TEXT_VOTE_4] = _glui->add_statictext_to_panel(
      _panel[PANEL_VOTE_DATA],
      "");
   assert(_text[TEXT_VOTE_4]);

   update_mesh_list();  //adjust_control_sizes() happens, too...
   update_non_lives();
   clear_path();
   clear_seg();
   clear_vote();

   if (!_init) init();

   _rollout[ROLLOUT_BUFFER]->close();
   _rollout[ROLLOUT_OPTIONS]->close();

   //Show will actually show it...
   _glui->hide(); 
}

/////////////////////////////////////
// adjust_control_sizes()
/////////////////////////////////////

void
SilUI::adjust_control_sizes() 
{
   
   static int wo = Config::get_var_int("SIL_DEBUG_W",640,true);
   static int ho = Config::get_var_int("SIL_DEBUG_H",110,true);

   // Cleanup sizes
   int dw,wx,w,wr,wp,wt,wg;
   
   wx = _panel[PANEL_TOP]->get_w();

   w = max(wx,wo);

   dw = (w - wx)/3;

   _button[BUT_PATCH_NEXT]->set_w( _button[BUT_PATCH_NEXT]->get_w() + dw);   
   _button[BUT_REFRESH]->set_w( _button[BUT_REFRESH]->get_w() + dw);   
   _slider[SLIDE_RATE]->set_w( _slider[SLIDE_RATE]->get_w() + dw);   

   wr = _rollout[ROLLOUT_BUFFER]->get_w();
   dw = (w - wr);

   _edittext[EDITTEXT_BUFFER_MISC1]->set_w( _edittext[EDITTEXT_BUFFER_MISC1]->get_w() + dw);   
   _edittext[EDITTEXT_BUFFER_MISC2]->set_w( _edittext[EDITTEXT_BUFFER_MISC2]->get_w() + dw);   

   wr = _rollout[ROLLOUT_OPTIONS]->get_w();
   dw = (w - wr)/5;

   _slider[SLIDE_FIT_PIX       ]->set_w( _slider[SLIDE_FIT_PIX       ]->get_w() + dw);   
   _slider[SLIDE_WEIGHT_FIT    ]->set_w( _slider[SLIDE_WEIGHT_FIT    ]->get_w() + dw);
   _slider[SLIDE_WEIGHT_SCALE  ]->set_w( _slider[SLIDE_WEIGHT_SCALE  ]->get_w() + dw);
   _slider[SLIDE_WEIGHT_DISTORT]->set_w( _slider[SLIDE_WEIGHT_DISTORT]->get_w() + dw);
   _slider[SLIDE_WEIGHT_HEAL   ]->set_w( _slider[SLIDE_WEIGHT_HEAL   ]->get_w() + dw);

   wr = _rollout[ROLLOUT_PATH]->get_w();
   wp = _panel[PANEL_PATH]->get_w();
   wt = _panel[PANEL_PATH_INFO]->get_w();

   _graph[GRAPH_PATH]->set_graph_size(
      _graph[GRAPH_PATH]->get_graph_w() + (w - wr),
      max(_graph[GRAPH_PATH]->get_graph_h(),ho));   

   wg = _graph[GRAPH_PATH]->get_w();

   _text[TEXT_PATH]->set_w(
      _text[TEXT_PATH]->get_w() + (wg - wt));

   dw = (wg - wp)/2;

   _button[BUT_PATH_PREV]->set_w( _button[BUT_PATH_PREV]->get_w() + dw);
   _button[BUT_PATH_NEXT]->set_w( _button[BUT_PATH_NEXT]->get_w() + dw);



   wr = _rollout[ROLLOUT_SEG]->get_w();
   wp = _panel[PANEL_SEG]->get_w();
   wt = _panel[PANEL_SEG_INFO]->get_w();

   _graph[GRAPH_SEG]->set_graph_size(
      _graph[GRAPH_SEG]->get_graph_w() +
         (w - _rollout[ROLLOUT_SEG]->get_w()),
      max(_graph[GRAPH_SEG]->get_graph_h(),ho));   

   wg = _graph[GRAPH_SEG]->get_w();

   _text[TEXT_SEG]->set_w(
      _text[TEXT_SEG]->get_w() + (wg - wt));

   dw = (wg - _panel[PANEL_SEG]->get_w())/2;

   _button[BUT_SEG_PREV]->set_w( _button[BUT_SEG_PREV]->get_w() + dw);
   _button[BUT_SEG_NEXT]->set_w( _button[BUT_SEG_NEXT]->get_w() + dw);


   wg = wg;
   wr = _rollout[ROLLOUT_VOTE]->get_w();
   wp = _panel[PANEL_VOTE]->get_w();
   wt = _panel[PANEL_VOTE_INFO]->get_w();

   _text[TEXT_VOTE]->set_w(
      _text[TEXT_VOTE]->get_w() + (wg - wt));

   _text[TEXT_VOTE_1]->set_w(
      _text[TEXT_VOTE_1]->get_w() + (wg - wt));
   _text[TEXT_VOTE_2]->set_w(
      _text[TEXT_VOTE_2]->get_w() + (wg - wt));
   _text[TEXT_VOTE_3]->set_w(
      _text[TEXT_VOTE_3]->get_w() + (wg - wt));
   _text[TEXT_VOTE_4]->set_w(
      _text[TEXT_VOTE_4]->get_w() + (wg - wt));

   dw = (wg - _panel[PANEL_VOTE]->get_w())/2;

   _button[BUT_VOTE_PREV]->set_w( _button[BUT_VOTE_PREV]->get_w() + dw);
   _button[BUT_VOTE_NEXT]->set_w( _button[BUT_VOTE_NEXT]->get_w() + dw);
}

/////////////////////////////////////
// destroy
/////////////////////////////////////

void
SilUI::destroy() 
{

   assert(_glui);

   //Hands off these soon to be bad things

   _button.clear();
   _slider.clear();
   _panel.clear(); 
   _rollout.clear();
   _listbox.clear();
   _graph.clear();
   _text.clear();
   _edittext.clear();
   _checkbox.clear();
   _radgroup.clear();
   _radbutton.clear();

   _mesh_names.clear();
   _buffer_filenames.clear();

   //Recursively kills off all controls, and itself
   _glui->close();

   _glui = NULL;


}

/////////////////////////////////////
// update_non_lives()
/////////////////////////////////////
//
// -Update the controls that changed
//  but don't have 'live' variables
//
/////////////////////////////////////

void
SilUI::update_non_lives()
{

   update_patch();

   update_path();

   update_seg();

   update_vote();
   
}

/////////////////////////////////////
// update_mesh_list()
/////////////////////////////////////

bool
SilUI::update_mesh_list()
{
   int new_i,old_i,i,j;
   CGELlist& list = _view->active();

   //-Hold on to the current selected name
   //-Update the list with new names
   //-Make sure the old name's on the list
   //-Update the selected name if its missing... 

   old_i = _listbox[LIST_MESH]->get_int_val();

   str_ptr old_name = ( old_i==0 ) ? (NULL_STR) : (_mesh_names[old_i-1]);

   //Clear out any previous names
   for (i=1; i<=_mesh_names.num();i++)
   {
      assert(_listbox[LIST_MESH]->delete_item(i));
   }
   _mesh_names.clear();



   new_i = 0;
   i=0;

   i++;

   assert(valid_gel(_bufferGEL));
   _mesh_names += _bufferGEL->name();
   _listbox[LIST_MESH]->add_item(i, **_mesh_names.last());
   if (old_name == _mesh_names.last()) new_i = i;

   for (j=0; j < list.num(); j++) 
   {
      if ( valid_gel(list[j]) )
      {
         i++;
         _mesh_names += list[j]->name();
         _listbox[LIST_MESH]->add_item(i, **_mesh_names.last());

         if (old_name == _mesh_names.last()) new_i = i;
      }
   }

   _listbox[LIST_MESH]->set_int_val(new_i);

   adjust_control_sizes();

   if ((!new_i) && (old_i))
     return true;
   return false;
}
/////////////////////////////////////
// update_patch()
/////////////////////////////////////

void
SilUI::update_patch()
{
   if (_selectedGEL)
   {
      assert(_selectedPatch);

      CPatch_list &pl = _selectedPatch->mesh()->patches();

      int i = pl.get_index(_selectedPatch);
      assert(i!=BAD_IND);
      str_ptr text = str_ptr(i+1) + " of " + str_ptr(pl.num());
      _text[TEXT_PATCH]->set_text(**text);

      _button[BUT_PATCH_NEXT]->enable();
   }
   else
   {
      assert(!_selectedPatch);
      _text[TEXT_PATCH]->set_text("N/A");

      _button[BUT_PATCH_NEXT]->disable();
   }
  
}

/////////////////////////////////////
// update_path()
/////////////////////////////////////

void
SilUI::update_path()
{
   const  double           bkg_color[4] = {0.8,0.8,0.8,1.0};

   if (_selectedGEL)
   {
      assert(_selectedPatch);
      if (_observedTexture)
      {
         ZXedgeStrokeTexture *z = _observedTexture->stroke_tex()->sil_and_crease_tex()->zx_edge_tex();

         const LuboPathList&  pl = z->paths();

         update_path_indices();

         //Path number
         str_ptr text;
         if (_votePathIndex == -1)
         {
            text = str_ptr("Showing NONE of ") + str_ptr(pl.num()) + " Paths";
         }
         else
         {
            text = str_ptr("Showing Path ") + str_ptr(_votePathIndex+1) 
                     + " of " + str_ptr(pl.num())  + " - "  
                     + SilAndCreaseTexture::sil_stroke_pool(
                        SilAndCreaseTexture::type_and_vis_to_sil_stroke_pool(
                           pl[_votePathIndex]->type(), pl[_votePathIndex]->vis()));
         }
         _text[TEXT_PATH]->set_text(**text);   

         if (strcmp(_graph[GRAPH_PATH]->name,"Parameter Votes vs. Arc-length") != 0)
         {
            _graph[GRAPH_PATH]->set_name("Parameter Votes vs. Arc-length");
            _graph[GRAPH_PATH]->set_background(bkg_color);
         }

         _button[BUT_PATH_NEXT]->enable();
         _button[BUT_PATH_PREV]->enable();
         _graph[GRAPH_PATH]->enable();   
      }
      else
      {
         _graph[GRAPH_PATH]->set_name("N/A");
         _text[TEXT_PATH]->set_text("No NPR Texture on this Patch!");
         _button[BUT_PATH_NEXT]->disable();
         _button[BUT_PATH_PREV]->disable();
         _graph[GRAPH_PATH]->disable();   
      }
   }
   else
   {
      assert(!_selectedPatch);
      assert(!_observedTexture);

      //Leave graph setup to the monolithic fps tick call

      _button[BUT_PATH_NEXT]->disable();
      _button[BUT_PATH_PREV]->disable();
      _graph[GRAPH_PATH]->disable();   
   }
}

/////////////////////////////////////
// update_seg()
/////////////////////////////////////

void
SilUI::update_seg()
{

   const  double           bkg_color[4] = {0.8,0.8,0.8,1.0};

   if (_selectedGEL)
   {
      assert(_selectedPatch);
      if (_observedTexture)
      {
         ZXedgeStrokeTexture *z = _observedTexture->stroke_tex()->sil_and_crease_tex()->zx_edge_tex();
         const LuboPathList&  pl = z->paths();

         update_path_indices();

         if (_votePathIndex != -1)
         {
            //Stroke number
            str_ptr text;
            if (_strokePathIndex == -1)
            {
               text = str_ptr("Showing NONE of ") + str_ptr(pl[_votePathIndex]->groups().num()) + " Strokes";
            }
            else
            {
               text = str_ptr("Showing Stroke ") + str_ptr(_strokePathIndex+1) 
                     + " of " + str_ptr(pl[_votePathIndex]->groups().num()) + " - " 
                     + VoteGroup::vg_status(pl[_votePathIndex]->groups()[_strokePathIndex].status()) + " - " 
                     + VoteGroup::fit_status(pl[_votePathIndex]->groups()[_strokePathIndex].fstatus());
            }
            _text[TEXT_SEG]->set_text(**text);   

            if (strcmp(_graph[GRAPH_SEG]->name,"Parameter Votes vs. Arc-length") != 0)
            {
               _graph[GRAPH_SEG]->set_name("Parameter Votes vs. Arc-length");
               _graph[GRAPH_SEG]->set_background(bkg_color);
            }

            _button[BUT_SEG_NEXT]->enable();
            _button[BUT_SEG_PREV]->enable();
            _graph[GRAPH_SEG]->enable();   
         }
         else
         {
            assert(_strokePathIndex == -1);

            if (strcmp(_graph[GRAPH_SEG]->name,"N/A") != 0) _graph[GRAPH_SEG]->set_name("N/A");
            _text[TEXT_SEG]->set_text("No path selected.");
            _button[BUT_SEG_NEXT]->disable();
            _button[BUT_SEG_PREV]->disable();
            _graph[GRAPH_SEG]->disable();   
         }
      }
      else
      {
          _graph[GRAPH_SEG]->set_name("N/A");
         _text[TEXT_SEG]->set_text("");
         _button[BUT_SEG_NEXT]->disable();
         _button[BUT_SEG_PREV]->disable();
         _graph[GRAPH_SEG]->disable();   
      }
   }
   else
   {
      assert(!_selectedPatch);
      assert(!_observedTexture);
      assert(_votePathIndex == -1);

      //Leave graph setup to the monolithic fps tick call

      _button[BUT_SEG_NEXT]->disable();
      _button[BUT_SEG_PREV]->disable();
      _graph[GRAPH_SEG]->disable();   
   }
}

/////////////////////////////////////
// update_vote()
/////////////////////////////////////

void
SilUI::update_vote()
{

   if (_selectedGEL)
   {
      str_ptr text;
      
      assert(_selectedPatch);

      if (_observedTexture)
      {
         ZXedgeStrokeTexture *z = _observedTexture->stroke_tex()->sil_and_crease_tex()->zx_edge_tex();
         const LuboPathList&  pl = z->paths();

         update_path_indices();

         //Path
         if (_votePathIndex != -1)
         {
            LuboPath *l = pl[_votePathIndex];

            //Stroke 
            if (_strokePathIndex != -1)
            {
               VoteGroup &g = l->groups()[_strokePathIndex];

               //Vote
               if (_voteIndex != -1)
               {
                  //Actually draw the good in the notify_cb
                  text = str_ptr("Showing Vote ") + str_ptr(_voteIndex+1) + " of " + str_ptr(g.num());
               }
               else
               {
                  text = str_ptr("Showing NONE of ") + str_ptr(g.num()) + " Votes";
               }

               if (strcmp(_panel[PANEL_VOTE_DATA]->name,"Vote Information") != 0)
                  _panel[PANEL_VOTE_DATA]->set_name("Vote Information");

               _button[BUT_VOTE_NEXT]->enable();
               _button[BUT_VOTE_PREV]->enable();
            }
            else
            {
               assert(_voteIndex == -1);

               if (strcmp(_panel[PANEL_VOTE_DATA]->name,"N/A") != 0)
                  _panel[PANEL_VOTE_DATA]->set_name("N/A");

               text = str_ptr("No stroke selected");

               _button[BUT_VOTE_NEXT]->disable();
               _button[BUT_VOTE_PREV]->disable();
            }
         }
         else
         {
            assert(_voteIndex == -1);
            assert(_strokePathIndex == -1);

            if (strcmp(_panel[PANEL_VOTE_DATA]->name,"N/A") != 0)
               _panel[PANEL_VOTE_DATA]->set_name("N/A");

            text = str_ptr("No path and stroke selected");

            _button[BUT_VOTE_NEXT]->disable();
            _button[BUT_VOTE_PREV]->disable();
         }
      }
      else
      {

         assert(_votePathIndex == -1);
         assert(_strokePathIndex == -1);
         assert(_voteIndex == -1);

          _panel[PANEL_VOTE_DATA]->set_name("N/A");
         
          text = str_ptr("");

         _button[BUT_VOTE_NEXT]->disable();
         _button[BUT_VOTE_PREV]->disable();
      }

      _text[TEXT_VOTE]->set_text(**text);   
   }
   else
   {
      assert(!_selectedPatch);
      assert(!_observedTexture);
      assert(_votePathIndex == -1);
      assert(_strokePathIndex == -1);
      assert(_voteIndex == -1);

      //Leave graph setup to the monolithic fps tick call

      _button[BUT_VOTE_NEXT]->disable();
      _button[BUT_VOTE_PREV]->disable();
   }
      
}

/////////////////////////////////////
// clear_path()
/////////////////////////////////////

void
SilUI::clear_path()
{
   _text[TEXT_PATH]->set_text("");

   _graph[GRAPH_PATH]->set_num_series(0);
   _graph[GRAPH_PATH]->redraw(); 
}


/////////////////////////////////////
// clear_seg()
/////////////////////////////////////

void
SilUI::clear_seg()
{
   _text[TEXT_SEG]->set_text("");

   _graph[GRAPH_SEG]->set_num_series(0);
   _graph[GRAPH_SEG]->redraw(); 
}


/////////////////////////////////////
// clear_vote()
/////////////////////////////////////

void
SilUI::clear_vote()
{
   _text[TEXT_VOTE]->set_text("");
   _text[TEXT_VOTE_1]->set_text("");
   _text[TEXT_VOTE_2]->set_text("");
   _text[TEXT_VOTE_3]->set_text("");
   _text[TEXT_VOTE_4]->set_text("");

}

/////////////////////////////////////
// buffer_grab()
/////////////////////////////////////

void
SilUI::buffer_grab()
{

   if (_observedTexture)
   {
      if (_selectedGEL != _bufferGEL)
      {
         //Currently observed stuff
         ZXedgeStrokeTexture *z = _observedTexture->stroke_tex()->sil_and_crease_tex()->zx_edge_tex();
         const LuboPathList&  pl = z->paths();

         //The buffer's stuff
         ZXedgeStrokeTexture *zb = ((NPRTexture *)(((TEXBODY*)&*_bufferGEL)->cur_rep()->patches()[0]->
                                          find_tex(NPRTexture::static_name())))->
                                             stroke_tex()->sil_and_crease_tex()->zx_edge_tex();
         LuboPathList&  plb = zb->paths();

         plb.delete_all();
         plb.clone(pl);

         //Need this, really?
         zb->regen_group_notify();
         zb->regen_paths_notify();

         //Hang on to the current selections
         CPatch_list &pal = _selectedPatch->mesh()->patches();
         int old_patch = pal.get_index(_selectedPatch); assert(old_patch!=BAD_IND);
         int old_votepath = _votePathIndex;
         int old_strokepath = _strokePathIndex;
         int old_vote = _voteIndex;

         //Now select the buffer
         _listbox[LIST_MESH]->set_int_val(1);
         _listbox[LIST_BUFFER]->set_int_val(0);

         //Update the pointers (this mauls the selected graphs)
         select_name();

         //And reselect...
         CPatch_list &npal = _selectedPatch->mesh()->patches();
         while (old_patch != npal.get_index(_selectedPatch)) select_next_patch();
         if (old_votepath == -1)
         {
            clear_votepath_index();
         }
         else
         {
            while (old_votepath != _votePathIndex) select_votepath_next();
         }
         if (old_strokepath == -1)
         {
            clear_strokepath_index();
         }
         else
         {
            while (old_strokepath != _strokePathIndex) select_strokepath_next();
         }
         if (old_vote == -1)
         {
            clear_vote_index();
         }
         else
         {
            while (old_vote != _voteIndex) select_vote_next();
         }

      }
      else
      {
         cerr << "SilUI::buffer_grab() - Buffer is selected! Cannot grab this!\n";
      }
   }
   else
   {
      cerr << "SilUI::buffer_grab() - No NPRTexture selected!\n";
   }
}

/////////////////////////////////////
// fill_buffer_listbox()
/////////////////////////////////////

void
SilUI::fill_buffer_listbox(
   GLUI_Listbox *listbox,
   str_list     &save_files,
   Cstr_ptr     &full_path
   )
{
   int i;

   //First clear out any previous presets
   for (i=1; i<=save_files.num();i++)
   {
      int foo = listbox->delete_item(i);
      assert(foo);
   }
   save_files.clear();

   str_list in_files = dir_list(full_path);
   for (i = 0; i < in_files.num(); i++) {
      int len = in_files[i].len();

      if ( (len>3) && (strcmp(&(**in_files[i])[len-4],".sil") == 0))
      {
         char *basename = new char[len+1];       
         assert(basename);
         strcpy(basename,**in_files[i]);
         basename[len-4] = 0;

         if ( listbox->check_item_fit(basename) == 1)
         {
            save_files += full_path + in_files[i];
            listbox->add_item(save_files.num(), basename);
         }
         else
         {
            cerr << "SilUI::fill_buffer_listbox - Discarding file (name too long for listbox): " << basename << "\n";
         }
         delete basename;
      }
      else
      {
         cerr << "SilUI::fill_buffer_listbox - Discarding file (bad name): " << **in_files[i] << "\n";
      }
   }
}

/////////////////////////////////////
// buffer_name_text()
/////////////////////////////////////
void
SilUI::buffer_name_text()
{
   int i,j;
   char *goodchars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890-_";

   char *origtext = _edittext[EDITTEXT_BUFFER_NAME]->get_text();
   int origlen = strlen(origtext);

   char *newtext;

   bool fix;

   //Here we replace bad chars with _'s,
   //and truncate to fit in the listbox
   //If no adjustments occur, we go ahead
   //and save and add the new preset.

   if (origlen == 0)
   {
      //TELL
      return;
   }

   newtext = new char[origlen+1]; 
   assert(newtext);
   strcpy(newtext,origtext);

   fix = false;
   for (i=0; i<origlen; i++)
   {
      bool good = false;
      for (j=0; j<(int)strlen(goodchars); j++)
      {
         if (newtext[i]==goodchars[j])
         {
            good = true;
         }
      }
      if (!good)
      {
         newtext[i] = '_';
         fix = true;
      }
   }

   if (fix) 
   {
      WORLD::message("Replaced bad characters with '_'. Continue editing.");
      _edittext[EDITTEXT_BUFFER_NAME]->set_text(newtext);
      delete(newtext);
      return;
   }

   fix = false;
   while (!_listbox[LIST_BUFFER]->check_item_fit(newtext) && strlen(newtext)>0)
   {
      fix = true;
      newtext[strlen(newtext)-1]=0;
   }

   if (fix) 
   {
      WORLD::message("Truncated name to fit listbox. Continue editing.");
      _edittext[EDITTEXT_BUFFER_NAME]->set_text(newtext);
      delete(newtext);
      return;
   }

   if (buffer_save(**(Config::JOT_ROOT() + BUFFER_DIRECTORY + newtext + ".sil")))
   {
      _buffer_filenames += (Config::JOT_ROOT() + BUFFER_DIRECTORY + newtext + ".sil");
      _listbox[LIST_BUFFER]->add_item(_buffer_filenames.num(), newtext);
      _listbox[LIST_BUFFER]->set_int_val(_buffer_filenames.num());
   }
   else
   {
      WORLD::message("Failed to save.");
   }

   _edittext[EDITTEXT_BUFFER_NAME]->set_text("");
   _glui->enable();
   _edittext[EDITTEXT_BUFFER_NAME]->disable();
   //XXX temp
   _edittext[EDITTEXT_BUFFER_MISC1]->disable();
   _edittext[EDITTEXT_BUFFER_MISC2]->disable();
   //_button[BUT_SAVE]->disable();
}

/////////////////////////////////////
// buffer_save_button()
/////////////////////////////////////

void
SilUI::buffer_save_button()
{

   int val = _listbox[LIST_BUFFER]->get_int_val();

   if (val == 0)
   {
      //Enter save mode in which only the 
      //filename box is available.
      _glui->disable();
      _edittext[EDITTEXT_BUFFER_NAME]->enable();

      WORLD::message("Enter filename in box and hit <ENTER>");
   }
   else
   {
      if (!buffer_save(**_buffer_filenames[val-1])) return;
      //_button[BUT_SAVE]->disable();
   }

}

/////////////////////////////////////
// buffer_load()
/////////////////////////////////////

bool
SilUI::buffer_load(const char *f)
{

   fstream fin;
#if (defined (WIN32) && defined(_MSC_VER) && (_MSC_VER <=1200)) /*VS 6.0*/
   fin.open(f, ios::in | ios::nocreate);
#else
   fin.open(f, ios::in);
#endif   
   
   if (!fin) 
   {
      cerr << "SilUI::buffer_load() - Error! Could not open file: " <<  f << "\n";
      return false;
   }

   STDdstream d(&fin);
   str_ptr str;

   d >> str;       

   if ((str == SilUI::static_name())) 
   {
      decode(d);
   }
   else
      cerr << "SilUI::buffer_load() - Error: Not SilUI: '" << str << "'" << endl;

   fin.close();

   return true;
}

/////////////////////////////////////
// buffer_save()
/////////////////////////////////////

bool
SilUI::buffer_save(const char *f)
{

   fstream fout;
   fout.open(f, ios::out);
   if (!fout) { cerr << "SilUI::buffer_save - Error! Could not open file: " <<  f << "\n"; return false; }

   STDdstream stream(&fout);

   format(stream);

   fout.close();

   return true;

}

/////////////////////////////////////
// buffer_selected()
/////////////////////////////////////

void
SilUI::buffer_selected()
{

   int val = _listbox[LIST_BUFFER]->get_int_val();

   if (val == 0)
   {
      //_button[BUT_SAVE]->enable();
   }
   else
   {
      if (!buffer_load(**_buffer_filenames[val-1]))
         return;

      // Update the controls that don't use
      // 'live' variables
      update_non_lives();

      _glui->sync_live();

   }
        
}

/////////////////////////////////////
// button_cb()
/////////////////////////////////////

void
SilUI::button_cb(int id)
{
   assert((id >> ID_SHIFT) < _ui.num());

   switch(id&ID_MASK)
   {
      case BUT_REFRESH:
         cerr << "SilUI::button_cb() - Refresh\n";
         if (_ui[id >> ID_SHIFT]->update_mesh_list())
         {
            _ui[id >> ID_SHIFT]->select_name();
            _ui[id >> ID_SHIFT]->update_non_lives();
         }
      break;
      case BUT_BUFFER_GRAB:
         cerr << "SilUI::button_cb() - Buffer Grab\n";
         _ui[id >> ID_SHIFT]->buffer_grab();
         _ui[id >> ID_SHIFT]->update_non_lives();
      break;
      case BUT_BUFFER_SAVE:
         cerr << "SilUI::button_cb() - Buffer Save\n";
         _ui[id >> ID_SHIFT]->buffer_save_button();
         _ui[id >> ID_SHIFT]->update_non_lives();
      break;

      case BUT_PATCH_NEXT:
         cerr << "SilUI::button_cb() - Patch Next\n";
         _ui[id >> ID_SHIFT]->select_next_patch();
         _ui[id >> ID_SHIFT]->update_non_lives();
      break;
      case BUT_PATH_PREV:
         cerr << "SilUI::button_cb() - Prev. Path\n";
         _ui[id >> ID_SHIFT]->select_votepath_prev();
         _ui[id >> ID_SHIFT]->update_non_lives();
      break;
      case BUT_PATH_NEXT:
         cerr << "SilUI::button_cb() - Next Path\n";
         _ui[id >> ID_SHIFT]->select_votepath_next();
         _ui[id >> ID_SHIFT]->update_non_lives();
      break;
      case BUT_SEG_PREV:
         cerr << "SilUI::button_cb() - Prev. Seg.\n";
         _ui[id >> ID_SHIFT]->select_strokepath_prev();
         _ui[id >> ID_SHIFT]->update_non_lives();
      break;
      case BUT_SEG_NEXT:
         cerr << "SilUI::button_cb() - Next Seg.\n";
         _ui[id >> ID_SHIFT]->select_strokepath_next();
         _ui[id >> ID_SHIFT]->update_non_lives();
      break;
      case BUT_VOTE_PREV:
         cerr << "SilUI::button_cb() - Prev. Vote\n";
         _ui[id >> ID_SHIFT]->select_vote_prev();
         _ui[id >> ID_SHIFT]->update_non_lives();
      break;
      case BUT_VOTE_NEXT:
         cerr << "SilUI::button_cb() - Next Vote\n";
         _ui[id >> ID_SHIFT]->select_vote_next();
         _ui[id >> ID_SHIFT]->update_non_lives();
      break;

   }
}

/////////////////////////////////////
// slider_cb()
/////////////////////////////////////

void
SilUI::slider_cb(int id)
{
   assert((id >> ID_SHIFT) < _ui.num());

   switch(id&ID_MASK)
   {
      case SLIDE_RATE:
         //_ui[id >> ID_SHIFT]->update_non_lives();
      break;
      case SLIDE_WEIGHT_FIT:
         //_ui[id >> ID_SHIFT]->update_non_lives();
      break;
      case SLIDE_WEIGHT_SCALE:
         //_ui[id >> ID_SHIFT]->update_non_lives();
      break;
      case SLIDE_WEIGHT_DISTORT:
         //_ui[id >> ID_SHIFT]->update_non_lives();
      break;
      case SLIDE_WEIGHT_HEAL:
         //_ui[id >> ID_SHIFT]->update_non_lives();
      break;

   }
}

/////////////////////////////////////
// listbox_cb()
/////////////////////////////////////
//
// -Common callback for all listboxes
//
/////////////////////////////////////

void
SilUI::listbox_cb(int id)
{
   assert((id >> ID_SHIFT) < _ui.num());

   switch (id&ID_MASK)
   {
    case LIST_MESH:
      cerr << "SilUI::listbox_cb - Mesh()\n";
      _ui[id >> ID_SHIFT]->select_name();
      _ui[id >> ID_SHIFT]->update_non_lives();
    break;
    case LIST_BUFFER:
      cerr << "SilUI::listbox_cb - Buffer()\n";
      _ui[id >> ID_SHIFT]->buffer_selected();
    break;

   }
}

/////////////////////////////////////
// edittext_cb()
/////////////////////////////////////

void
SilUI::edittext_cb(int id)
{
   assert((id >> ID_SHIFT) < _ui.num());

   switch(id&ID_MASK)
   {
       case EDITTEXT_BUFFER_NAME:
         cerr << "SilUI::edittext_cb() - Buffer\n";
         _ui[id >> ID_SHIFT]->buffer_name_text();
       break;
       case EDITTEXT_BUFFER_MISC1:
         cerr << "SilUI::edittext_cb() - Misc1\n";
//         _ui[id >> ID_SHIFT]->preset_save_text();
       break;
 
       case EDITTEXT_BUFFER_MISC2:
         cerr << "SilUI::edittext_cb() - Misc2\n";
//         _ui[id >> ID_SHIFT]->preset_save_text();
       break;
   }

}

/////////////////////////////////////
// graph_cb()
/////////////////////////////////////
//
// -Common callback for all graphs
//
/////////////////////////////////////

void
SilUI::graph_cb(int id)
{
   assert((id >> ID_SHIFT) < _ui.num());

   switch (id&ID_MASK)
   {
    case GRAPH_PATH:
      cerr << "SilUI::graph_cb() - Path\n";
    break;
    case GRAPH_SEG:
      cerr << "SilUI::graph_cb() - Seg\n";
    break;

   }
}

/////////////////////////////////////
// checkbox_cb()
/////////////////////////////////////
//
// -Common callback for all checkboxes
//
/////////////////////////////////////

void
SilUI::checkbox_cb(int id)
{
   assert((id >> ID_SHIFT) < _ui.num());

   switch (id&ID_MASK)
   {
    case CHECK_TRACK_STROKES:
      cerr << "SilUI::checkbox_cb() - Track strokes\n";
    break;
    case CHECK_ZOOM_STROKES:
      cerr << "SilUI::checkbox_cb() - Zoom strokes\n";
    break;
    case CHECK_ALWAYS_UPDATE:
      cerr << "SilUI::checkbox_cb() - Always update\n";
    break;

   }
}

/////////////////////////////////////
// radiogroup_cb()
/////////////////////////////////////

void
SilUI::radiogroup_cb(int id)
{
   assert((id >> ID_SHIFT) < _ui.num());

   switch(id&ID_MASK)
   {
       case RADGROUP_FIT:
         cerr << "SilUI::radiogroup_cb() - Fit type\n";
       break;
       case RADGROUP_COVER:
         cerr << "SilUI::radiogroup_cb() - Fit cover\n";
       break;

   }
}

/////////////////////////////////////
// valid_gel()
/////////////////////////////////////

bool
SilUI::valid_gel(CGELptr g)
{
   //Valid GELs are TEXBODYs having at least one Patch with an NPRTexture

   if (TEXBODY::isa(g))
   {
      CBMESHptr b = ((TEXBODY*)&*g)->cur_rep();
      if (b != NULL)
      {
         CPatch_list &pl = b->patches();

         for (int i=0; i<pl.num(); i++)
         {
            NPRTexture *t = (NPRTexture *)pl[i]->find_tex(NPRTexture::static_name());
            if (t) return true;
         }
      }
   }
   
   return false;
}


/////////////////////////////////////
// select_gel()
/////////////////////////////////////

void
SilUI::select_gel(GEL *g)
{
   assert(!_selectedGEL);
   assert(!_selectedPatch);
   assert(!_observedTexture);

   assert(g);
   
   _selectedGEL = g;

   assert(TEXBODY::isa(g));
   
   CBMESHptr b = ((TEXBODY*)&*g)->cur_rep();
   
   assert(b != NULL);
   
   CPatch_list &pl = b->patches();

   assert(pl.num() > 0);

   //Default to patch with most triangles

   int max_i=0;
   int max_tris=pl[0]->cur_faces().num();

   for (int i=1; i<pl.num(); i++)
   {
      if (pl[i]->cur_faces().num() > max_tris)
      {
         max_i = i;
         max_tris = pl[i]->cur_faces().num();
      }
   }

   select_patch(pl[max_i]);

}

/////////////////////////////////////
// deselect_gel()
/////////////////////////////////////

void
SilUI::deselect_gel()
{
   if (_selectedGEL)
   {
      deselect_patch();
      _selectedGEL = NULL;
   }

   assert(!_selectedGEL);
   assert(!_selectedPatch);
   assert(!_observedTexture);

}

/////////////////////////////////////
// select_patch()
/////////////////////////////////////

void
SilUI::select_patch(Patch *p)
{
   assert(!_selectedPatch);
   assert(!_observedTexture);

   assert(p);

   _selectedPatch = p;

   NPRTexture *t = (NPRTexture *)p->find_tex(NPRTexture::static_name());

   if (t)
   {
      start_observing(t);
      assert(_observedTexture);
   }
   else
   {
      assert(!_observedTexture);
   }

   clear_path();
   clear_seg();
   clear_vote();
}

/////////////////////////////////////
// deselect_patch()
/////////////////////////////////////

void
SilUI::deselect_patch()
{
   if (_selectedPatch)
   {
      stop_observing();
      _selectedPatch = NULL;
   }

   assert(!_selectedPatch);
   assert(!_observedTexture);

   clear_path();
   clear_seg();
   clear_vote();

}

/////////////////////////////////////
// start_observing()
/////////////////////////////////////

void
SilUI::start_observing(NPRTexture *t)
{
   assert(t);
   _observedTexture = t;
   _observedTexture->add_obs(this);
   init_votepath_index();
}


/////////////////////////////////////
// stop_observing()
/////////////////////////////////////

void
SilUI::stop_observing()
{
   if (_observedTexture)
   {
      _observedTexture->rem_obs(this);
      _observedTexture = NULL;
   }
   assert(!_observedTexture);
   clear_votepath_index();
}


/////////////////////////////////////
// select_name()
/////////////////////////////////////

void
SilUI::select_name()
{
   update_mesh_list();

   deselect_gel();

   int index = _listbox[LIST_MESH]->get_int_val();

   if (index > 1)
   {
      _button[BUT_BUFFER_GRAB]->enable();

      _button[BUT_BUFFER_SAVE]->disable();
      _listbox[LIST_BUFFER]->disable();
      _edittext[EDITTEXT_BUFFER_NAME]->disable();
      _edittext[EDITTEXT_BUFFER_MISC1]->disable();
      _edittext[EDITTEXT_BUFFER_MISC1]->disable();
   }
   else if (index == 1)
   {
      _button[BUT_BUFFER_GRAB]->disable();

      _button[BUT_BUFFER_SAVE]->enable();
      _listbox[LIST_BUFFER]->enable();
      _edittext[EDITTEXT_BUFFER_NAME]->disable();
      _edittext[EDITTEXT_BUFFER_MISC1]->disable();
      _edittext[EDITTEXT_BUFFER_MISC2]->disable();
   }
   else
   {
      _button[BUT_BUFFER_GRAB]->disable();
      _button[BUT_BUFFER_SAVE]->disable();
      _listbox[LIST_BUFFER]->disable();
      _edittext[EDITTEXT_BUFFER_NAME]->disable();
      _edittext[EDITTEXT_BUFFER_MISC1]->disable();
      _edittext[EDITTEXT_BUFFER_MISC2]->disable();
      return;
   }

   str_ptr name = _mesh_names[index-1];

   CGELlist& list = _view->active();

   GELptr g = NULL;

   if (name == _bufferGEL->name())
   {
      g = _bufferGEL;
   }
   else
   {
      for (int i=0; i < list.num() && (g == NULL); i++) 
      {
         if (name == list[i]->name()) g = list[i];
      }
   }

   assert(g != NULL);

   select_gel(&*g);
}

/////////////////////////////////////
// select_next_patch()
/////////////////////////////////////

void
SilUI::select_next_patch()
{
   if (_selectedGEL)
   {
      assert(_selectedPatch);

      assert(TEXBODY::isa(_selectedGEL));
   
      CBMESHptr b = ((TEXBODY*)_selectedGEL)->cur_rep();
   
      assert(b != NULL);
   
      CPatch_list &pl = b->patches();

      int i = pl.get_index(_selectedPatch);

      assert(i!=BAD_IND);

      deselect_patch();
            
      select_patch(pl[(i+1)%(pl.num())]);

   }
   else
   {
      assert(!_selectedPatch);
   }

}

/////////////////////////////////////
// notify_draw()
/////////////////////////////////////

void
SilUI::notify_draw(SilAndCreaseTexture *t)
{
   assert(t);

   const double  inner_dot_color[4] = {1.0,1.0,1.0,0.9};

   const double  world_color[4] = {0.1,0.1,1.0,0.5};

   const double  world_color_selected[4] = {1.0,0.4,0.1,1.0};

   const double  series_color[6][4] = {{0.0,0.0,1.0,0.5},{1.0,0.0,0.0,0.5},{0.0,1.0,0.0,0.5},
                                          {1.0,0.0,1.0,0.5},{0.0,1.0,1.0,0.5},{1.0,1.0,0.0,0.5}};

   const double  series_color_selected[6][4] = {{0.0,0.0,1.0,1.0},{1.0,0.0,0.0,1.0},{0.0,1.0,0.0,1.0},
                                                 {1.0,0.0,1.0,1.0},{0.0,1.0,1.0,1.0},{1.0,1.0,0.0,1.0}};

   const double  series_color_fit[4] = {0.0, 0.0, 0.0, 0.8};

   const LuboPathList&  pl = t->zx_edge_tex()->paths();

   update_non_lives();
   update_path_indices();

   static ARRAY<double>    x;
   static ARRAY<double>    y;

   if (_votePathIndex == -1)
   {

      _graph[GRAPH_PATH]->set_num_series(0);
      _graph[GRAPH_PATH]->redraw(); 

      _graph[GRAPH_SEG]->set_num_series(0);
      _graph[GRAPH_SEG]->redraw(); 

      _text[TEXT_VOTE_1]->set_text("");   
      _text[TEXT_VOTE_2]->set_text("");   
      _text[TEXT_VOTE_3]->set_text("");   
      _text[TEXT_VOTE_4]->set_text("");   

   }
   else
   {

      LuboPath *l = pl[_votePathIndex];
      ARRAY<VoteGroup> &gs = l->groups();

      ARRAY<unsigned int>  path_ids;
      ARRAY<int>           group_cnt;

      int total = 0;
      int seg_total = 0;

      int i=0;
      for ( ; i<gs.num(); i++) 
      {
         VoteGroup &g = gs[i];

         if (g.num()>0)
         {
            path_ids.add_uniquely( g.vote(0)._path_id );

            if (path_ids.num() > group_cnt.num())
            {
               group_cnt.add(0);
            }

            for (int j=1; j<g.num(); j++) 
            {
               //XXX - Do we care?
               //assert(g.vote(0)._path_id == g.vote(j)._path_id);
            }

            int index = path_ids.get_index(g.vote(0)._path_id);

            group_cnt[index] = group_cnt[index] + 1;

            total++;

            if ((int)g.id() == _strokePathId)
            {
               assert(i == _strokePathIndex);
               seg_total++;
            }
         }
      }
      assert((seg_total == 0) || (seg_total == 1));
      
      _graph[GRAPH_PATH]->set_num_series(2*total);
      _graph[GRAPH_SEG]->set_num_series(2*seg_total + 2 + ((_voteIndex!=-1)?(2):(0)));


      for (int j=0; j<path_ids.num(); j++)
      {
         int cnt = 0;

         for (int i=0; i<gs.num(); i++) 
         {
            VoteGroup &g = gs[i];
            
            if (g.num() > 0)
            {
               if (path_ids[j] == g.vote(0)._path_id)
               {
                  total--;

                  x.clear(); y.clear();
               
                  for (int k=0; k<g.num(); k++) 
                  {
                     x.add(g.vote(k)._s);
                     y.add(g.vote(k)._t);
                  }

                  if ((int)g.id() != _strokePathId)
                  {
                     _graph[GRAPH_PATH]->set_series_name(2*total,**(str_ptr("Path #") + str_ptr(total)));
                     _graph[GRAPH_PATH]->set_series_type(2*total,GLUI_GRAPH_SERIES_LINE);
                     _graph[GRAPH_PATH]->set_series_size(2*total,1.0);
                     _graph[GRAPH_PATH]->set_series_color(2*total, &series_color[cnt%6][0]);

                     _graph[GRAPH_PATH]->set_series_data(2*total,x.num(),x.array(),y.array());

                     _graph[GRAPH_PATH]->set_series_name(2*total+1,**(str_ptr("Path #") + str_ptr(total)));
                     _graph[GRAPH_PATH]->set_series_type(2*total+1,GLUI_GRAPH_SERIES_DOT);
                     _graph[GRAPH_PATH]->set_series_size(2*total+1,4.0);
                     _graph[GRAPH_PATH]->set_series_color(2*total+1,&series_color[j%6][0]);

                     _graph[GRAPH_PATH]->set_series_data(2*total+1,x.num(),x.array(),y.array());
                  }
                  else
                  {
                     seg_total--;

                     _graph[GRAPH_PATH]->set_series_name(2*total,**(str_ptr("Path #") + str_ptr(total)));
                     _graph[GRAPH_PATH]->set_series_type(2*total,GLUI_GRAPH_SERIES_LINE);
                     _graph[GRAPH_PATH]->set_series_size(2*total,1.4);
                     _graph[GRAPH_PATH]->set_series_color(2*total, &series_color_selected[cnt%6][0]);

                     _graph[GRAPH_PATH]->set_series_data(2*total,x.num(),x.array(),y.array());

                     _graph[GRAPH_PATH]->set_series_name(2*total+1,**(str_ptr("Path #") + str_ptr(total)));
                     _graph[GRAPH_PATH]->set_series_type(2*total+1,GLUI_GRAPH_SERIES_DOT);
                     _graph[GRAPH_PATH]->set_series_size(2*total+1,5.0);
                     _graph[GRAPH_PATH]->set_series_color(2*total+1,&series_color_selected[j%6][0]);

                     _graph[GRAPH_PATH]->set_series_data(2*total+1,x.num(),x.array(),y.array());


                     _graph[GRAPH_SEG]->set_series_name(2*seg_total,**(str_ptr("Path #") + str_ptr(seg_total)));
                     _graph[GRAPH_SEG]->set_series_type(2*seg_total,GLUI_GRAPH_SERIES_LINE);
                     _graph[GRAPH_SEG]->set_series_size(2*seg_total,1.0);
                     _graph[GRAPH_SEG]->set_series_color(2*seg_total, &series_color[cnt%6][0]);

                     _graph[GRAPH_SEG]->set_series_data(2*seg_total,x.num(),x.array(),y.array());

                     _graph[GRAPH_SEG]->set_series_name(2*seg_total+1,**(str_ptr("Path #") + str_ptr(seg_total)));
                     _graph[GRAPH_SEG]->set_series_type(2*seg_total+1,GLUI_GRAPH_SERIES_DOT);
                     _graph[GRAPH_SEG]->set_series_size(2*seg_total+1,4.0);
                     _graph[GRAPH_SEG]->set_series_color(2*seg_total+1,&series_color[j%6][0]);

                     _graph[GRAPH_SEG]->set_series_data(2*seg_total+1,x.num(),x.array(),y.array());


                     x.clear(); y.clear();
               
                     for (int k=0; k<g.nfits(); k++) 
                     {
                        x.add(g.fit(k)[0]);
                        y.add(g.fit(k)[1]);
                     }

                     _graph[GRAPH_SEG]->set_series_name(2*seg_total+2,**(str_ptr("Path #") + str_ptr(seg_total)));
                     _graph[GRAPH_SEG]->set_series_type(2*seg_total+2,GLUI_GRAPH_SERIES_LINE);
                     _graph[GRAPH_SEG]->set_series_size(2*seg_total+2,1.0);
                     _graph[GRAPH_SEG]->set_series_color(2*seg_total+2, &series_color_fit[0]);

                     _graph[GRAPH_SEG]->set_series_data(2*seg_total+2,x.num(),x.array(),y.array());

                     _graph[GRAPH_SEG]->set_series_name(2*seg_total+3,**(str_ptr("Path #") + str_ptr(seg_total)));
                     _graph[GRAPH_SEG]->set_series_type(2*seg_total+3,GLUI_GRAPH_SERIES_DOT);
                     _graph[GRAPH_SEG]->set_series_size(2*seg_total+3,4.0);
                     _graph[GRAPH_SEG]->set_series_color(2*seg_total+3,&series_color_fit[0]);

                     _graph[GRAPH_SEG]->set_series_data(2*seg_total+3,x.num(),x.array(),y.array());


                     if (_voteIndex != -1)
                     {
                        x.clear(); y.clear();
            
                        x.add(g.vote(_voteIndex)._s);
                        y.add(g.vote(_voteIndex)._t);

                        _graph[GRAPH_SEG]->set_series_name(2*seg_total+4,**(str_ptr("Path #") + str_ptr(seg_total+1)));
                        _graph[GRAPH_SEG]->set_series_type(2*seg_total+4,GLUI_GRAPH_SERIES_DOT);
                        _graph[GRAPH_SEG]->set_series_size(2*seg_total+4,7.0);
                        _graph[GRAPH_SEG]->set_series_color(2*seg_total+4,&series_color_selected[j%6][0]);

                        _graph[GRAPH_SEG]->set_series_data(2*seg_total+4,x.num(),x.array(),y.array());

                        _graph[GRAPH_SEG]->set_series_name(2*seg_total+5,**(str_ptr("Path #") + str_ptr(seg_total+1)));
                        _graph[GRAPH_SEG]->set_series_type(2*seg_total+5,GLUI_GRAPH_SERIES_DOT);
                        _graph[GRAPH_SEG]->set_series_size(2*seg_total+5,4.0);
                        _graph[GRAPH_SEG]->set_series_color(2*seg_total+5,inner_dot_color);

                        _graph[GRAPH_SEG]->set_series_data(2*seg_total+5,x.num(),x.array(),y.array());

                     }
                  }
                  cnt++;
               }
            }
         }
      
      }
      
      assert(total == 0);
      assert(seg_total == 0);

      _graph[GRAPH_PATH]->set_min_x(true,0.01);   _graph[GRAPH_PATH]->set_max_x(true,0.01);
      _graph[GRAPH_PATH]->set_min_y(true,0.05);   _graph[GRAPH_PATH]->set_max_y(true,0.05);

      _graph[GRAPH_PATH]->redraw(); 


      if (_checkbox[CHECK_ZOOM_STROKES]->get_int_val() == 1)
      {
         _graph[GRAPH_SEG]->set_min_x(true,0.01);   _graph[GRAPH_SEG]->set_max_x(true,0.01);
         _graph[GRAPH_SEG]->set_min_y(true,0.05);   _graph[GRAPH_SEG]->set_max_y(true,0.05);
      }
      else
      {
         _graph[GRAPH_SEG]->set_min_x(false, _graph[GRAPH_PATH]->get_min_x());   
         _graph[GRAPH_SEG]->set_max_x(false, _graph[GRAPH_PATH]->get_max_x());
         _graph[GRAPH_SEG]->set_min_y(false, _graph[GRAPH_PATH]->get_min_y());   
         _graph[GRAPH_SEG]->set_max_y(false, _graph[GRAPH_PATH]->get_max_y());
      }

      _graph[GRAPH_SEG]->redraw(); 
   


      if (_voteIndex != -1)
      {
         assert(_strokePathIndex != -1);
         LuboVote &v = gs[_strokePathIndex].vote(_voteIndex);
         str_ptr text;
            
         //Output some stuff 
         text = str_ptr("PATH_ID =") + str_ptr((int)v._path_id) + "    STROKE_ID = " + str_ptr((int)v._stroke_id);
         _text[TEXT_VOTE_1]->set_text(**text);   
         text = str_ptr("S = ") + str_ptr(v._s) + "      T =" + str_ptr(v._t);
         _text[TEXT_VOTE_2]->set_text(**text);   
         text = str_ptr("WORLD_DIST = ") + str_ptr(v._world_dist) + "      NDC_DIST =" + str_ptr(v._ndc_dist);
         _text[TEXT_VOTE_3]->set_text(**text);   
         text = str_ptr("CONF = ") + str_ptr(v._conf) + "      STATUS=" + LuboVote::lv_status(v._status);
         _text[TEXT_VOTE_4]->set_text(**text);   
      }
      else
      {
         _text[TEXT_VOTE_1]->set_text("");   
         _text[TEXT_VOTE_2]->set_text("");   
         _text[TEXT_VOTE_3]->set_text("");   
         _text[TEXT_VOTE_4]->set_text("");   
      }
      
      // Only draw this stuff if we're NOT looking at the buffer

      if (_selectedGEL != _bufferGEL)
      {

         // Push affected state:
         glPushAttrib(
            GL_CURRENT_BIT             |
            GL_ENABLE_BIT              |
            GL_POINT_BIT               |
            GL_LINE_BIT                |
            GL_HINT_BIT
            );

         // Set state for drawing strokes:
         glDisable(GL_LIGHTING);												// GL_ENABLE_BIT
         glDisable(GL_CULL_FACE);											// GL_ENABLE_BIT
         glDisable(GL_DEPTH_TEST);										   // GL_ENABLE_BIT
         glEnable(GL_BLEND);													// GL_ENABLE_BIT

         glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);                 // GL_HINT_BIT
         glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);                  // GL_HINT_BIT

         glEnable(GL_POINT_SMOOTH);                               // GL_ENABLE_BIT
         glEnable(GL_LINE_SMOOTH);                                // GL_ENABLE_BIT

         // Set projection and modelview matrices for drawing in NDC:
         glMatrixMode(GL_PROJECTION);
         glPushMatrix();
         glLoadMatrixd(VIEW::peek()->ndc_proj().transpose().matrix());

         glMatrixMode(GL_MODELVIEW);
         glPushMatrix();
         glLoadIdentity();


         glColor4dv(world_color);                                 // GL_CURRENT_BIT

         glPointSize(3.0);                                        // GL_POINT_BIT               
         glLineWidth(2.0);                                        // GL_LINE_BIT               

         glBegin(GL_POINTS);

         for (i=0; i<l->num(); i++)
         {  
            glVertex3dv(l->pt(i).data());
         }
         glEnd();
         glBegin(GL_LINE_STRIP);
         for (i=0; i<l->num(); i++)
         {  
            glVertex3dv(l->pt(i).data());
         }
         glEnd();

         if (_strokePathIndex != -1)
         {
            VoteGroup &g = l->groups()[_strokePathIndex];
            double len = l->length();

            glColor4dv(world_color_selected);                        // GL_CURRENT_BIT

            glPointSize(4.5);                                        // GL_POINT_BIT               
            glLineWidth(1.5);                                        // GL_LINE_BIT               

            glBegin(GL_POINTS);
               for (i=0; i<g.num(); i++)
               {  
                  glVertex3dv(l->pt(g.vote(i)._s/len).data());
               }
            glEnd();
            glBegin(GL_LINE_STRIP);
               for (i=0; i<g.num(); i++)
               {  
                  glVertex3dv(l->pt(g.vote(i)._s/len).data());
               }
            glEnd();


            if (_voteIndex != -1)
            {
               glColor4dv(world_color_selected);                     // GL_CURRENT_BIT
               glPointSize(7.0);                                     // GL_POINT_BIT               
               glBegin(GL_POINTS);
                  glVertex3dv(l->pt(g.vote(_voteIndex)._s/len).data());
               glEnd();

               glColor4dv(inner_dot_color);                          // GL_CURRENT_BIT
               glPointSize(4.0);                                     // GL_POINT_BIT               
               glBegin(GL_POINTS);
                  glVertex3dv(l->pt(g.vote(_voteIndex)._s/len).data());
               glEnd();
            }
         }


         glMatrixMode(GL_MODELVIEW);
         glPopMatrix();

         glMatrixMode(GL_PROJECTION);
         glPopMatrix();

         glMatrixMode(GL_MODELVIEW);

         glPopAttrib();   

      }
     
   }
   
   _lastDrawStamp = _view->stamp();


}

/////////////////////////////////////
// tick()
/////////////////////////////////////

int
SilUI::tick()
{
   if (!internal_is_vis()) return 1;

   if (_listbox[LIST_MESH]->get_int_val() == 0)
   {
      // XXX - If no object's selected, display 
      // graphs of FPS and TPS as a test...
      fps_display();
   }
   else
   {
      // Otherwise, do the debugging display stuff...

      if (_listbox[LIST_MESH]->get_int_val() == 1)
      {
         //The buffer is selected, so manually invoke thangs...

         NPRTexture *tb = (NPRTexture *)(((TEXBODY*)&*_bufferGEL)->cur_rep()->patches()[0]->find_tex(NPRTexture::static_name()));

         SilAndCreaseTexture *sb = tb->stroke_tex()->sil_and_crease_tex();
         ZXedgeStrokeTexture *zb = sb->zx_edge_tex();

         //Force recalculation of fits
         if (_always_update)
         {
            // Updates the groups regen. stamp, too...
            sb->generate_sil_groups();
            // Force path regen stamp to update, 
            // even though they haven't changed, since
            // the rest of the code assumes both stamps
            // update together...
            zb->regen_paths_notify();
         }

         notify_draw(sb);

      }
      else
      {
         //Do nothing.  notify_draw() will be called automatically...
      }
   }

   return 1;
}

/////////////////////////////////////
// fps_display()
/////////////////////////////////////

#define FPS_NUM 100

void
SilUI::fps_display()
{
   const  double           bkg_color[4] = {0.8,0.8,0.8,1.0};
   const  double           fps_color[4] = {1.0,0.0,0.0,1.0};
   const  double           tps_color[4] = {0.0,0.0,1.0,1.0};

   static stop_watch       watch;
   static ARRAY<double>    x;
   static ARRAY<double>    fps;
   static ARRAY<double>    tps;
   static unsigned int     stamp;
   static int              tris;

   int i;

   update_non_lives();

   if (fps.num() == 0)
   {
      for (i=0; i<FPS_NUM; i++)
      {
         x.add(i+1.0);
         fps.add(0.0);
         tps.add(0.0);
      }
      watch.set();
      tris = 0;
      stamp = _view->stamp();
   }

   if (!(strcmp(_graph[GRAPH_PATH]->name,"Frames per Second")==0))
   {
      //Graph not configures for this output, lets do that...

      _graph[GRAPH_PATH]->set_name("Frames per Second");
      _graph[GRAPH_PATH]->set_background(bkg_color);
      _graph[GRAPH_PATH]->set_num_series(1);
      _graph[GRAPH_PATH]->set_series_name(0,"FPS");
      _graph[GRAPH_PATH]->set_series_type(0,GLUI_GRAPH_SERIES_LINE);
      _graph[GRAPH_PATH]->set_series_size(0,1.5);
      _graph[GRAPH_PATH]->set_series_color(0,fps_color);

      _graph[GRAPH_SEG]->set_name("Triangles per Second");
      _graph[GRAPH_SEG]->set_background(bkg_color);
      _graph[GRAPH_SEG]->set_num_series(1);
      _graph[GRAPH_SEG]->set_series_name(0,"TPS");
      _graph[GRAPH_SEG]->set_series_type(0,GLUI_GRAPH_SERIES_LINE);
      _graph[GRAPH_SEG]->set_series_size(0,1.5);
      _graph[GRAPH_SEG]->set_series_color(0,tps_color);

      _panel[PANEL_VOTE_DATA]->set_name("N/A");
      //_text[TEXT_VOTE]->set_text("N/A");

      watch.set();
      tris = 0;
      stamp = _view->stamp();
   }

   //Stuff we do every frame

   tris += _view->tris();

   double elapsed_time = watch.elapsed_time();
   int elapsed_frames = _view->stamp() - stamp;

   //Only add a sample if we've waited long enough...
   if (elapsed_time > 0.25 && elapsed_frames > _slider[SLIDE_RATE]->get_int_val())
   {
      double new_fps = elapsed_frames/elapsed_time;
      double new_tps = tris/elapsed_time;


      for (i=0; i<fps.num()-1; i++)
      {
         fps[i]=fps[i+1];
         tps[i]=tps[i+1];
      }
      fps[i] = new_fps;
      tps[i] = new_tps;

      watch.set();
      tris = 0;
      stamp = _view->stamp();

      //Now show it!

      _graph[GRAPH_PATH]->set_series_data(0,fps.num(),x.array(),fps.array());
      _graph[GRAPH_PATH]->set_min_x(true);   _graph[GRAPH_PATH]->set_max_x(true);
      _graph[GRAPH_PATH]->set_min_y(true);   _graph[GRAPH_PATH]->set_max_y(true);
      _graph[GRAPH_PATH]->redraw();

      _graph[GRAPH_SEG]->set_series_data(0,tps.num(),x.array(),tps.array());
      _graph[GRAPH_SEG]->set_min_x(true);    _graph[GRAPH_SEG]->set_max_x(true);
      _graph[GRAPH_SEG]->set_min_y(true);    _graph[GRAPH_SEG]->set_max_y(true);
      _graph[GRAPH_SEG]->redraw();

      //cerr << "FPS: " << new_fps << "TPS: " << new_tps << "\n";

      char text_fps[64];
      char text_tris[64];

      if (new_tps < 1e3)
         sprintf(text_tris, "%5.1f tris/sec",  new_tps);
      else if (new_tps < 1e6)
         sprintf(text_tris, "%5.1fK tris/sec", new_tps/1e3);
      else
         sprintf(text_tris, "%5.1fM tris/sec", new_tps/1e6);
      
      sprintf(text_fps, "%5.1f fps ", new_fps);

      _text[TEXT_PATH]->set_text(text_fps);
      _text[TEXT_SEG]->set_text(text_tris);

   }

}

/////////////////////////////////////
// init_votepath_index()
/////////////////////////////////////

void
SilUI::init_votepath_index()
{
   // Set index to first votepath (0) or none (-1)
   // if there aren't any...

   assert(_votePathIndex == -1);

   assert(_observedTexture);
   ZXedgeStrokeTexture *z = _observedTexture->stroke_tex()->sil_and_crease_tex()->zx_edge_tex();

   const LuboPathList&  pl = z->paths();

   if (pl.num())
   {
      _votePathIndex = 0;
      _votePathIndexStamp = z->path_stamp();
   }
   else
   {
      _votePathIndex = -1;
      _votePathIndexStamp = 0;
   }

   // This stuff bubbles down
   init_strokepath_index();
}

/////////////////////////////////////
// init_strokepath_index()
/////////////////////////////////////

void
SilUI::init_strokepath_index()
{
   // Set index to first group (0) in current votepath or none (-1)
   // if there aren't any, or -1 if there aren't and votepaths

   assert(_strokePathIndex == -1);

   assert(_observedTexture);
   ZXedgeStrokeTexture *z = _observedTexture->stroke_tex()->sil_and_crease_tex()->zx_edge_tex();

   const LuboPathList&  pl = z->paths();

   if (!pl.num())
   {
      assert(_votePathIndex == -1);
      _strokePathId = -1;
      _strokePathIndex = -1;
      _strokePathIndexStamp = 0;
      assert(_strokePathIndexStamp == _votePathIndexStamp);
   }
   else
   {
      if (_votePathIndex == -1)
      {
         _strokePathId = -1;
         _strokePathIndex = -1;
         _strokePathIndexStamp = 0;
         assert(_strokePathIndexStamp == _votePathIndexStamp);
      }
      else
      {
         LuboPath *l = pl[_votePathIndex];

         ARRAY<VoteGroup> &g = l->groups();

         if (g.num()>0)
         {
            _strokePathId = g[0].id();
            _strokePathIndex = 0;
            _strokePathIndexStamp = z->path_stamp();
            assert(_strokePathIndexStamp == _votePathIndexStamp);
         }
         else
         {
            _strokePathId = -1;
            _strokePathIndex = -1;
            _strokePathIndexStamp = 0;
         }
      }
   }
   
   init_vote_index();
}

/////////////////////////////////////
// init_vote_index()
/////////////////////////////////////

void
SilUI::init_vote_index()
{
   // Set index to first vote (0) in current goup or none (-1)
   // if there aren't any or nothing's selected...

   assert(_voteIndex == -1);

   assert(_observedTexture);
   ZXedgeStrokeTexture *z = _observedTexture->stroke_tex()->sil_and_crease_tex()->zx_edge_tex();

   const LuboPathList&  pl = z->paths();

   if (pl.num()==0)
   {
      assert(_votePathIndex == -1);
      assert(_strokePathIndex == -1);
      assert(_strokePathIndexStamp == 0);
      _voteIndex = -1;
   }
   else
   {
      if (_votePathIndex == -1)
      {
         assert(_strokePathIndex == -1);
         assert(_strokePathIndexStamp == 0);
         _voteIndex = -1;
      }
      else
      {
         LuboPath *l = pl[_votePathIndex];

         ARRAY<VoteGroup> &gs = l->groups();

         if (gs.num()==0)
         {
            assert(_strokePathIndex == -1);
            assert(_strokePathIndexStamp == 0);
            _voteIndex = -1;
         }
         else
         {
            if (_strokePathIndex == -1)
            {
               assert(_strokePathIndexStamp == 0);
               _voteIndex = -1;
            }
            else
            {
               VoteGroup &g = gs[_strokePathIndex];

               if (g.num()==0)
               {
                  assert(_strokePathIndexStamp == z->group_stamp());
                  _voteIndex = -1;
               }
               else
               {
                  _voteIndex = 0;
                  assert(_strokePathIndexStamp == z->group_stamp());

               }
            }
         }
      }
   }

}

/////////////////////////////////////
// clear_votepath_index()
/////////////////////////////////////

void
SilUI::clear_votepath_index()
{
   // Set index to no selection
   
   _votePathIndex = -1;
   _votePathIndexStamp = 0;

   // This stuff bubbles down
   clear_strokepath_index();
}

/////////////////////////////////////
// clear_strokepath_index()
/////////////////////////////////////

void
SilUI::clear_strokepath_index()
{
   // Set index to no selection
   
   _strokePathId = -1;
   _strokePathIndex = -1;
   _strokePathIndexStamp = 0;

   clear_vote_index();
}


/////////////////////////////////////
// clear_vote_index()
/////////////////////////////////////

void
SilUI::clear_vote_index()
{
   // Set index to no selection
   
   _voteIndex = -1;

}

/////////////////////////////////////
// update_path_indices()
/////////////////////////////////////

void
SilUI::update_path_indices()
{
   //Find indices suitable for this view, or re-init if we lost it somehow...

   assert(_observedTexture);
   ZXedgeStrokeTexture *z = _observedTexture->stroke_tex()->sil_and_crease_tex()->zx_edge_tex();
   const LuboPathList&  pl = z->paths();

   //We aren't tracking a path
   if (_votePathIndex == -1)
   {
      //Nothin' to do
      assert(_strokePathIndex == -1);
   }
   //We are tracking a path
   else
   {
      //And our stamp's kosher
      if (_votePathIndexStamp == z->path_stamp())
      {
         //Nuttin to do
         assert(z->path_stamp() == z->group_stamp());
         assert((_strokePathIndex == -1)||(_votePathIndexStamp == _strokePathIndexStamp));
      }
      else
      {
         assert(_votePathIndex != -1);

         //Should be a newer stamp
         assert(_votePathIndexStamp < z->path_stamp());

         assert((_strokePathIndex == -1)||(_votePathIndexStamp == _strokePathIndexStamp));

         //First see if this is the buffer...
         if (_selectedGEL == _bufferGEL)
         {
            // The vote paths should change, so let's assert
            // that our current selection is kosher...
            assert(_votePathIndex < pl.num());
            _votePathIndexStamp = z->path_stamp(); 

            // For now, just keep the same stroke path index
            // or the max index if we're now out of range
            if (_strokePathIndex != -1)
            {
               if (_strokePathIndex >= pl[_votePathIndex]->groups().num())
               {
                  _strokePathIndex = pl[_votePathIndex]->groups().num() - 1;
               }
               if (_strokePathIndex != -1)
               {
                  _strokePathId = pl[_votePathIndex]->groups()[_strokePathIndex].id();
                  _strokePathIndexStamp = z->group_stamp();         
                  assert(_votePathIndexStamp == _strokePathIndexStamp);
                  if (_voteIndex >= pl[_votePathIndex]->groups()[_strokePathIndex].num())
                     clear_vote_index();
               }
               else
               {
                  clear_strokepath_index();
               }
            }

         }
         //Else proceed as usual...
         else if (_checkbox[CHECK_TRACK_STROKES]->get_int_val()==0)
         {
            // Path tracking....

            _votePathIndex = (unsigned int)pl.votepath_id_to_index(_votePathIndex);
            _votePathIndexStamp = z->path_stamp();         

            if (_votePathIndex == -1)
            {
               clear_votepath_index(); //clears strokepath, too
            }
            else
            {
               _strokePathIndex = (unsigned int)pl.strokepath_id_to_index(_strokePathId, _votePathIndex);
               if (_strokePathIndex != -1)
               {
                  _strokePathId = pl[_votePathIndex]->groups()[_strokePathIndex].id();
                  _strokePathIndexStamp = z->group_stamp();         
                  assert(_votePathIndexStamp == _strokePathIndexStamp);
                  //Update vote selection -- Just deselect if out of range
                  if (_voteIndex >= pl[_votePathIndex]->groups()[_strokePathIndex].num())
                     clear_vote_index();
               }
               else
               {
                  clear_strokepath_index();
               }
            }

         }
         else
         {
            // Stroke/group tracking...
            if (_strokePathId != -1)
            {
               if (pl.strokepath_id_to_indices(_strokePathId, &_votePathIndex, &_strokePathIndex))
               {
                  _votePathIndexStamp = z->path_stamp(); 
                  _strokePathId = pl[_votePathIndex]->groups()[_strokePathIndex].id();
                  _strokePathIndexStamp = z->group_stamp();
                  assert(_votePathIndexStamp == _strokePathIndexStamp);

                  if (_voteIndex >= pl[_votePathIndex]->groups()[_strokePathIndex].num())
                     clear_vote_index();
               }
               else
               {
                  clear_votepath_index(); //clears all indices
               }
            }
            else
            {
               //No stroke/group, so just do path tracking

               _votePathIndex = (unsigned int)pl.votepath_id_to_index(_votePathIndex);
               _votePathIndexStamp = z->path_stamp();         
 
               if (_votePathIndex != -1)
               {
                  assert(_voteIndex == -1);
               }
               else
               {
                  clear_votepath_index();
               }
            }
         }
      }
   }

}

/////////////////////////////////////
// select_votepath_next()
/////////////////////////////////////

void
SilUI::select_votepath_next()
{

   if (_votePathIndex == -1) 
   {
      init_votepath_index();
   }
   else
   {
      update_path_indices();

      if (_votePathIndex == -1) 
      {
         init_votepath_index();
      }
      else
      {
         assert(_observedTexture);

         ZXedgeStrokeTexture *z = _observedTexture->stroke_tex()->sil_and_crease_tex()->zx_edge_tex();
         const LuboPathList&  pl = z->paths();

         //if (_checkbox[CHECK_TRACK_STROKES]->get_int_val()==0)
         //{

            _votePathIndex = (_votePathIndex + 1) % pl.num();
            assert(_votePathIndexStamp == z->path_stamp());

            _strokePathIndex = pl.strokepath_id_to_index(_strokePathId, _votePathIndex);
            if (_strokePathIndex != -1)
            {
               _strokePathId = pl[_votePathIndex]->groups()[_strokePathIndex].id();
               assert(_strokePathIndexStamp = z->group_stamp());
               assert(_votePathIndexStamp == _strokePathIndexStamp);
            }
            else
            {
               clear_strokepath_index();
            }

         //}
         //else
         //{
            //XXX - Same policy in either case...
         //}

      }
   }
}

/////////////////////////////////////
// select_votepath_prev()
/////////////////////////////////////

void
SilUI::select_votepath_prev()
{
   if (_votePathIndex == -1) 
   {
      init_votepath_index();
   }
   else
   {
      update_path_indices();

      if (_votePathIndex == -1) 
      {
         init_votepath_index();
      }
      else
      {
         assert(_observedTexture);

         ZXedgeStrokeTexture *z = _observedTexture->stroke_tex()->sil_and_crease_tex()->zx_edge_tex();
         const LuboPathList&  pl = z->paths();

         //if (_checkbox[CHECK_TRACK_STROKES]->get_int_val()==0)
         //{

            _votePathIndex = (_votePathIndex - 1 + pl.num()) % pl.num();
            assert(_votePathIndexStamp == z->path_stamp());

            _strokePathIndex = pl.strokepath_id_to_index(_strokePathId, _votePathIndex);
            if (_strokePathIndex != -1)
            {
               _strokePathId = pl[_votePathIndex]->groups()[_strokePathIndex].id();
               assert(_strokePathIndexStamp = z->group_stamp());
               assert(_votePathIndexStamp == _strokePathIndexStamp);
            }
            else
            {
               clear_strokepath_index();
            }

         //}
         //else
         //{
            //XXX - Same policy in either case...
         //}

      }
   }
}

/////////////////////////////////////
// select_strokepath_next()
/////////////////////////////////////

void
SilUI::select_strokepath_next()
{

   if (_strokePathIndex == -1) 
   {
      if (_votePathIndex == -1) 
      {
         init_votepath_index();
      }
      else
      {
         init_strokepath_index();
      }
   }
   else
   {
      update_path_indices();

      if (_votePathIndex == -1) 
      {
         init_votepath_index();
      }
      else if(_strokePathIndex == -1)
      {
         init_strokepath_index();
      }
      else
      {
         assert(_observedTexture);

         ZXedgeStrokeTexture *z = _observedTexture->stroke_tex()->sil_and_crease_tex()->zx_edge_tex();
         const LuboPathList&  pl = z->paths();

         //if (_checkbox[CHECK_TRACK_STROKES]->get_int_val()==0)
         //{

            _strokePathIndex = (_strokePathIndex + 1) % pl[_votePathIndex]->groups().num();
            assert(_votePathIndexStamp == z->path_stamp());
            assert(_strokePathIndexStamp == z->group_stamp());
            assert(_strokePathIndexStamp == _votePathIndexStamp);
            _strokePathId = pl[_votePathIndex]->groups()[_strokePathIndex].id();
 
            _voteIndex = -1;
            init_vote_index();
         //}
         //else
         //{
            //XXX - Same policy in either case...
         //}

      }
   }
}

/////////////////////////////////////
// select_strokepath_prev()
/////////////////////////////////////

void
SilUI::select_strokepath_prev()
{

   if (_strokePathIndex == -1) 
   {
      if (_votePathIndex == -1) 
      {
         init_votepath_index();
      }
      else
      {
         init_strokepath_index();
      }
   }
   else
   {
      update_path_indices();

      if (_votePathIndex == -1) 
      {
         init_votepath_index();
      }
      else if(_strokePathIndex == -1)
      {
         init_strokepath_index();
      }
      else
      {
         assert(_observedTexture);

         ZXedgeStrokeTexture *z = _observedTexture->stroke_tex()->sil_and_crease_tex()->zx_edge_tex();
         const LuboPathList&  pl = z->paths();

         //if (_checkbox[CHECK_TRACK_STROKES]->get_int_val()==0)
         //{

            _strokePathIndex = (_strokePathIndex - 1 + pl[_votePathIndex]->groups().num()) % pl[_votePathIndex]->groups().num();
            assert(_votePathIndexStamp == z->path_stamp());
            assert(_strokePathIndexStamp == z->group_stamp());
            assert(_strokePathIndexStamp == _votePathIndexStamp);
            _strokePathId = pl[_votePathIndex]->groups()[_strokePathIndex].id();

            _voteIndex = -1;
            init_vote_index();
         //}
         //else
         //{
            //XXX - Same policy in either case...
         //}

      }
   }
}

/////////////////////////////////////
// select_vote_next()
/////////////////////////////////////

void
SilUI::select_vote_next()
{

   assert(_strokePathIndex != -1);

   update_path_indices();

   if (_votePathIndex == -1) 
   {
      init_votepath_index();
   }
   else if(_strokePathIndex == -1)
   {
      init_strokepath_index();
   }
   else if(_voteIndex == -1)
   {
      init_vote_index();
   }
   else
   {
      assert(_observedTexture);

      ZXedgeStrokeTexture *z = _observedTexture->stroke_tex()->sil_and_crease_tex()->zx_edge_tex();
      int num = z->paths()[_votePathIndex]->groups()[_strokePathIndex].num();

      //if (_checkbox[CHECK_TRACK_STROKES]->get_int_val()==0)
      //{

         _voteIndex = (_voteIndex + 1) % num;
         assert(_strokePathIndexStamp == z->group_stamp());

      //}
      //else
      //{
         //XXX - Same policy in either case...
      //}

   }

}

/////////////////////////////////////
// select_vote_prev()
/////////////////////////////////////

void
SilUI::select_vote_prev()
{

   assert(_strokePathIndex != -1);

   update_path_indices();

   if (_votePathIndex == -1) 
   {
      init_votepath_index();
   }
   else if(_strokePathIndex == -1)
   {
      init_strokepath_index();
   }
   else if(_voteIndex == -1)
   {
      init_vote_index();
   }
   else
   {
      assert(_observedTexture);

      ZXedgeStrokeTexture *z = _observedTexture->stroke_tex()->sil_and_crease_tex()->zx_edge_tex();
      const LuboPathList&  pl = z->paths();
      int num = pl[_votePathIndex]->groups()[_strokePathIndex].num();

      //if (_checkbox[CHECK_TRACK_STROKES]->get_int_val()==0)
      //{

         _voteIndex = (_voteIndex - 1 + num) % num;
         assert(_strokePathIndexStamp == z->group_stamp());

      //}
      //else
      //{
         //XXX - Same policy in either case...
      //}

   }

}
