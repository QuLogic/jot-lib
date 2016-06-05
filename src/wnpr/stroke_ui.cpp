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
// StrokeUI
////////////////////////////////////////////
//
// -Manages a GLUI window that handles the interface
// -Actual 'live' variables reside within in object
// -Clients can update the vars, and update() refreshes the widgets
// -Client is reposible to supplying extra widgets and using the data
//
////////////////////////////////////////////



//This is relative to JOT_ROOT, and should
//contain ONLY the texture dots for hatching strokes
#define TEXTURE_DIRECTORY        "nprdata/stroke_textures/"
#define PRESET_DIRECTORY         "nprdata/stroke_presets/"
#define PAPER_DIRECTORY          "nprdata/paper_textures/"
#define ID_SHIFT                 10
#define ID_MASK                  ((1<<ID_SHIFT)-1)

#define STROKE_PARAMS_MIN_W      110
#define STROKE_PREVIEW_W         120
#define STROKE_PREVIEW_H         120

#define NUM_PREVIEW_STROKES     5

#define STROKEUI_DEFAULT_COLOR           COLOR::black
#define STROKEUI_DEFAULT_ALPHA           1.0f
#define STROKEUI_DEFAULT_WIDTH           8.0f
#define STROKEUI_DEFAULT_FADE            3.0f
#define STROKEUI_DEFAULT_HALO            0.0f
#define STROKEUI_DEFAULT_TAPER           40.0f
#define STROKEUI_DEFAULT_FLARE           0.2f
#define STROKEUI_DEFAULT_AFLARE          0.0f
#define STROKEUI_DEFAULT_ANGLE           0.0f
#define STROKEUI_DEFAULT_CONTRAST        0.5f
#define STROKEUI_DEFAULT_BRIGHTNESS      0.5f
#define STROKEUI_DEFAULT_TEX_FILE        (Config::JOT_ROOT() + "nprdata/stroke_textures/1D--dark-8.png")
#define STROKEUI_DEFAULT_PAPER_FILE      ""

#include "gtex/gl_extensions.H"
#include "base_jotapp/base_jotapp.H"
#include "geom/winsys.H"
#include "geom/world.H"
#include "glui/glui_jot.H"
#include "std/config.H"
#include "std/file.H"
#include <fstream>

#include "stroke_ui.H"

using namespace mlib;

/**********************************************************************
 * STROKE_UI_JOB:
 **********************************************************************/

/////////////////////////////////////
// updated()
/////////////////////////////////////
bool    
STROKE_UI_JOB::needs_update()
{
   // XXX - Hack to solve this problem:
   //
   //   When the stroke image gets redrawn, the view is resized,
   //   which marks the camera dirty, which tells the camera
   //   observers, one of which is the Draw_int, which hides the
   //   current easel, causing on-screen strokes to disappear.
   //
   //   In this hack, when we find that we are putting in a
   //   request to update the stroke image, we tell the Draw_int
   //   to save the current easel, and then later when we get
   //   word that the update has happened, we restore the easel.

   if (!AUX_JOB::needs_update())
      return 0;
   
//     if (Draw_int::get_instance()) 
//        Draw_int::get_instance()->save_easel();

   return 1;
}

void    
STROKE_UI_JOB::updated()
{ 
   AUX_JOB::updated();

   if (_ui)
      _ui->stroke_img_updated();  

//     if (Draw_int::get_instance())
//        Draw_int::get_instance()->restore_easel();
}

/*****************************************************************
 * StrokeUI
 *****************************************************************/

/////////////////////////////////////
// Static variables
/////////////////////////////////////

vector<StrokeUI*>        StrokeUI::_ui;
map<VIEWimpl*,StrokeUI*> StrokeUI::_hash;

/////////////////////////////////////
// Constructor
/////////////////////////////////////

StrokeUI::StrokeUI(VIEWptr v) :
   _id(0),
   _init(false),
   _view(v),
   _client(nullptr),
   _glui(nullptr)
{
   _ui.push_back(this);
   _id = (_ui.size()-1);

   _params.set_color(STROKEUI_DEFAULT_COLOR); 
   _params.set_alpha(STROKEUI_DEFAULT_ALPHA);
   _params.set_width(STROKEUI_DEFAULT_WIDTH);
   _params.set_fade(STROKEUI_DEFAULT_FADE);
   _params.set_halo(STROKEUI_DEFAULT_HALO);
   _params.set_taper(STROKEUI_DEFAULT_TAPER);
   _params.set_flare(STROKEUI_DEFAULT_FLARE);
   _params.set_aflare(STROKEUI_DEFAULT_AFLARE);
   _params.set_angle(STROKEUI_DEFAULT_ANGLE);
   _params.set_contrast(STROKEUI_DEFAULT_CONTRAST);
   _params.set_brightness(STROKEUI_DEFAULT_BRIGHTNESS);
   _params.set_texture(STROKEUI_DEFAULT_TEX_FILE);
   _params.set_paper(STROKEUI_DEFAULT_PAPER_FILE);

   // Defer init() until the first build()
}

/////////////////////////////////////
// Destructor
/////////////////////////////////////

StrokeUI::~StrokeUI()
{
   // XXX - Need to clean up? Nah, we never destroy these
   err_msg("StrokeUI::~StrokeUI - Error!!! Destructor not implemented.");
}

/////////////////////////////////////
// init()
/////////////////////////////////////
void
StrokeUI::init()
{
   assert(!_init);

   int i;

   _job = make_shared<STROKE_UI_JOB>();
   _job->set_ui(this);

   _init = true;

   for (i=0; i<NUM_PREVIEW_STROKES; i++)
      _strokes.add(new STROKE_GEL);

   for (i=0; i<_strokes.num(); i++) 
      _job->add(_strokes[i]);

   assert(NUM_PREVIEW_STROKES > 0);

   //Based on the size of various GUI parts (variable due to
   //listbox's holding uknown filenames), this sets the preview 
   //image size to fill the space
   set_preview_size();

   // Now grab that size so we can manually compute the NDC
   // coords of the default stroke (sinewave)
   int w, h;
   w = _bitmapbox[BITMAPBOX_PREVIEW]->get_image_w();
   h = _bitmapbox[BITMAPBOX_PREVIEW]->get_image_h();

   //Setup default stroke
   _strokes[0]->pts().clear();

   float maxx, maxy;

   if (w >= h)
   {
      maxx = (float)w/(float)h;
      maxy = 1.0;
   }
   else
   {
      maxx = 1.0;
      maxy = (float)h/(float)w;
   }

   const float xfrac = 0.9;
   const float yfrac = 0.7;
   const int steps = 12;

   for (i=0; i < steps; i++)
      _strokes[0]->pts().push_back(
            NDCZpt( -xfrac*maxx + (float)i * ((2.0 * xfrac * maxx) / (float)(steps-1)), 
                     yfrac*maxy * sin( 2.0*M_PI * (float)i/(float)(steps-1) ), 
                     0));

   _curr_stroke = 0;

}
/////////////////////////////////////
// set_preview_size()
/////////////////////////////////////
void
StrokeUI::set_preview_size()
{
   assert(_init);

   int new_w, new_h;
   
   new_w = _rollout[ROLLOUT_SHAPE]->get_w() - _panel[PANEL_STROKE]->get_w() + _bitmapbox[BITMAPBOX_PREVIEW]->get_image_w();
   new_h = _panel[PANEL_PRESET]->get_h() - _bitmapbox[BITMAPBOX_PREVIEW]->get_h() + _bitmapbox[BITMAPBOX_PREVIEW]->get_image_h();

//   new_w = 4*(new_w/4);

   _bitmapbox[BITMAPBOX_PREVIEW]->set_img_size(new_w, new_h);

   _job->set_size(new_w, new_h);
}

/////////////////////////////////////
// fetch() - Implicit Constructor
/////////////////////////////////////

StrokeUI*
StrokeUI::fetch(CVIEWptr& v)
{
   if (!v) 
   {
      err_msg("StrokeUI::fetch() - Error! view is nil");
      return nullptr;
   }
   if (!v->impl()) {
      err_msg("StrokeUI::fetch() - Error! view->impl() is nil");
      return nullptr;
   }

   // hash on the view implementation rather than the view itself

   map<VIEWimpl*,StrokeUI*>::iterator it;
   it = _hash.find(v->impl());

   if (it != _hash.end()) {
      return it->second;
   } else {
      StrokeUI *sui = new StrokeUI(v);
      assert(sui);
      _hash[v->impl()] = sui;
      return sui;
   }
}

/////////////////////////////////////
// capture() 
/////////////////////////////////////

bool
StrokeUI::capture(CVIEWptr& v, StrokeUIClient *suic)
{

   StrokeUI* sui;

   if (!(sui = StrokeUI::fetch(v)))
      {
         err_msg("StrokeUI::capture() - Error! Failed to fetch StrokeUI!");
         return false;
      }

   if (!sui->internal_capture(suic))
      {
         err_msg("StrokeUI::capture() - Error! Failed to capture StrokeUI!");
         return false;
      }
   else
      {
         err_mesg(ERR_LEV_SPAM, "StrokeUI::capture() - StrokeUI sucessfully captured by client.");
         return true;
      }

}

/////////////////////////////////////
// release() 
/////////////////////////////////////

bool
StrokeUI::release(CVIEWptr& v, StrokeUIClient *suic)
{
   
   StrokeUI* sui;

   if (!(sui = StrokeUI::fetch(v)))
      {
         err_msg("StrokeUI::release - Error! Failed to fetch StrokeUI!");
         return false;
      }

   if (!sui->internal_verify(suic))
      {
         err_msg("StrokeUI::release() - Error! This StrokeUIClient is not authorized on this StrokeUI!");
         return false;
      }

   if (!sui->internal_release())
      {
         err_msg("StrokeUI::release() - Error! Failed to release StrokeUI!");
         return false;
      }
   else 
      {
         err_mesg(ERR_LEV_SPAM, "StrokeUI::release() - StrokeUI sucessfully released client.");
         return true;
      }
        
}

/////////////////////////////////////
// show() 
/////////////////////////////////////

bool
StrokeUI::show(CVIEWptr& v, StrokeUIClient *suic)
{
   
   StrokeUI* sui;

   if (!(sui = StrokeUI::fetch(v)))
      {
         err_msg("StrokeUI::show - Error! Failed to fetch StrokeUI!");
         return false;
      }

   if (!sui->internal_verify(suic))
      {
         err_msg("StrokeUI::show() - Error! This StrokeUIClient is not authorized on this StrokeUI!");
         return false;
      }

   if (!sui->internal_show())
      {
         err_msg("StrokeUI::show() - Error! Failed to show StrokeUI!");
         return false;
      }
   else 
      {
         err_mesg(ERR_LEV_SPAM, "StrokeUI::show() - StrokeUI sucessfully showed StrokeUI.");
         return true;
      }
        
}

/////////////////////////////////////
// hide() 
/////////////////////////////////////

bool
StrokeUI::hide(CVIEWptr& v, StrokeUIClient *suic)
{
   
   StrokeUI* sui;

   if (!(sui = StrokeUI::fetch(v)))
      {
         err_msg("StrokeUI::hide - Error! Failed to fetch StrokeUI!");
         return false;
      }

   if (!sui->internal_verify(suic))
      {
         err_msg("StrokeUI::hide() - Error! This StrokeUIClient is not authorized on this StrokeUI!");
         return false;
      }

   if (!sui->internal_hide())
      {
         err_msg("StrokeUI::hide() - Error! Failed to hide StrokeUI!");
         return false;
      }
   else 
      {
         err_mesg(ERR_LEV_SPAM, "StrokeUI::hide() - StrokeUI sucessfully hid StrokeUI.");
         return true;
      }
        
}

/////////////////////////////////////
// update() 
/////////////////////////////////////

bool
StrokeUI::update(CVIEWptr& v, StrokeUIClient *suic)
{
   
   StrokeUI* sui;

   if (!(sui = StrokeUI::fetch(v)))
      {
         err_msg("StrokeUI::update - Error! Failed to fetch StrokeUI!");
         return false;
      }

   if (!sui->internal_verify(suic))
      {
         err_msg("StrokeUI::update() - Error! This StrokeUIClient is not authorized on this StrokeUI!");
         return false;
      }

   if (!sui->internal_update())
      {
         err_msg("StrokeUI::update() - Error! Failed to update StrokeUI!");
         return false;
      }
   else 
      {
         err_mesg(ERR_LEV_SPAM, "StrokeUI::update() - StrokeUI sucessfully updated StrokeUI.");
         return true;
      }
        
}

/////////////////////////////////////
// set_params() 
/////////////////////////////////////

bool
StrokeUI::set_params(CVIEWptr& v, StrokeUIClient *suic, CBaseStroke* sp)
{
   
   StrokeUI* sui;

   if (!(sui = StrokeUI::fetch(v)))
      {
         err_msg("StrokeUI::set_params() - Error! Failed to fetch StrokeUI!");
         return false;
      }

   if (!sui->internal_verify(suic))
      {
         err_msg("StrokeUI::set_params() - Error! This StrokeUIClient is not authorized on this StrokeUI!");
         return false;
      }

   if (!sui->internal_set_params(sp))
      {
         err_msg("StrokeUI::set_params() - Error! Failed to set params of StrokeUI!");
         return false;
      }
   else 
      {
         //err_mesg(ERR_LEV_SPAM, "StrokeUI::set_params() - StrokeUI sucessfully set params of StrokeUI.");
         return true;
      }
        
}

/////////////////////////////////////
// get_params() 
/////////////////////////////////////

CBaseStroke* 
StrokeUI::get_params(CVIEWptr& v, StrokeUIClient *suic)
{
   
   StrokeUI* sui;
   CBaseStroke* sp;

   if (!(sui = StrokeUI::fetch(v)))
      {
         err_msg("StrokeUI::get_params() - Error! Failed to fetch StrokeUI!");
         return nullptr;
      }

   if (!sui->internal_verify(suic))
      {
         err_msg("StrokeUI::get_params() - Error! This StrokeUIClient is not authorized on this StrokeUI!");
         return nullptr;
      }

   if (!(sp=sui->internal_get_params()))
      {
         err_msg("StrokeUI::get_params() - Error! Failed to get params of StrokeUI!");
         return nullptr;
      }
   else 
      {
         //err_mesg(ERR_LEV_SPAM, "StrokeUI::get_params() - StrokeUI sucessfully got params of StrokeUI.");
         return sp;
      }
        
}

/////////////////////////////////////
// internal_verify()
/////////////////////////////////////

bool
StrokeUI::internal_verify(StrokeUIClient *suic)
{

   // Verifies that a particular client is
   // qualified to invoke a method

   if (!suic) 
      {
         err_msg("StrokeUI::internal_verify() - Error! StrokeUIClient is NULL.");
         return false;
      }

   if (!_client) 
      {
         err_msg("StrokeUI::internal_verify() - Error! StrokeUI has not been captured by any client.");
         return false;
      }

   if (_client != suic) 
      {
         err_msg("StrokeUI::internal_verify() - Error! StrokeUI has not been captured by this client.");
         return false;
      }

   return true;

}


/////////////////////////////////////
// internal_capture()
/////////////////////////////////////

bool
StrokeUI::internal_capture(StrokeUIClient *suic)
{

   // A client can capture the StrokeUI as long as it's
   // presently unused (_client = 0)

   if (!suic) 
      {
         err_msg("StrokeUI::internal_capture() - Error! StrokeUIClient is NULL.");
         return false;
      }

   if (_client) 
      {
         err_msg("StrokeUI::internal_capture() - Error! StrokeUI already in use.");
         return false;
      }

   _client = suic;

   return true;

}


/////////////////////////////////////
// internal_release()
/////////////////////////////////////

bool
StrokeUI::internal_release()
{

   // A client can release the StrokeUI as long as it's hidden

   if (_glui) 
      {
         err_msg("StrokeUI::internal_release() - Error! StrokeUI must be hidden before releasing.");
         return false;
      }

   _client = nullptr;

   return true;

}


/////////////////////////////////////
// build()
/////////////////////////////////////
//
// -Setup the widgets 
// -All widgets pointers are strored in arrays and
//  indexed by id types declared in the header
// -We only get here if there's a client, but no glui,
//  since show build the glui and hide destroys it
// -Client is also called to build the docked widgets
//
/////////////////////////////////////

void
StrokeUI::build() 
{
   int i;
   int id = _id << ID_SHIFT;

   // Create the glui widget that will contain the controls
   // There should be NO glui since build is called in show
   // which calls here to create the glui.  Hide destroys it...
   assert(!_glui);
   // But a client must exist...
   assert(_client);

   int root_x, root_y, root_w, root_h;
   _view->win()->size(root_w,root_h);
   _view->win()->position(root_x,root_y);

   _glui = GLUI_Master.create_glui("NPR Stroke Editor", 0, root_x + root_w + 10, root_y);
   _glui->set_main_gfx_window(_view->win()->id());

   //Init the control arrays
   assert(_listbox.empty());      _listbox.resize(LIST_NUM, nullptr);
   assert(_button.empty());       _button.resize(BUT_NUM, nullptr);
   assert(_slider.empty());       _slider.resize(SLIDE_NUM, nullptr);
   assert(_edittext.empty());     _edittext.resize(EDITTEXT_NUM, nullptr);
   assert(_panel.empty());        _panel.resize(PANEL_NUM, nullptr);
   assert(_rollout.empty());      _rollout.resize(ROLLOUT_NUM, nullptr);
   assert(_bitmapbox.empty());    _bitmapbox.resize(BITMAPBOX_NUM, nullptr);

   assert(_texture_filenames.empty());
   assert(_paper_filenames.empty());
   assert(_preset_filenames.empty());

   // Panel containing pen buttons
   _panel[PANEL_PEN] = new GLUI_Panel(_glui, "");
   assert(_panel[PANEL_PEN]);

   //Prev pen
   _button[BUT_PREV_PEN] = new GLUI_Button(
      _panel[PANEL_PEN], "Previous Mode", 
      id+BUT_PREV_PEN, button_cb);
   assert(_button[BUT_PREV_PEN]);

   new GLUI_Column(_panel[PANEL_PEN], false);

   //Prev pen
   _button[BUT_NEXT_PEN] = new GLUI_Button(
      _panel[PANEL_PEN], "Next Mode", 
      id+BUT_NEXT_PEN, button_cb);
   assert(_button[BUT_NEXT_PEN]);

   // Panel containing preset controls and preview
   _panel[PANEL_STROKE] = new GLUI_Panel(_glui, "Stroke Editor");
   assert(_panel[PANEL_STROKE]);

   //Sub-panel containing presets controls
   _panel[PANEL_PRESET] = new GLUI_Panel(_panel[PANEL_STROKE], "Presets");
   assert(_panel[PANEL_PRESET]);           

   //Preset list
   _listbox[LIST_PRESET] = new GLUI_Listbox(
      _panel[PANEL_PRESET], 
      "", nullptr,
      id+LIST_PRESET, listbox_cb);
   assert(_listbox[LIST_PRESET]);
   _listbox[LIST_PRESET]->set_w(STROKE_PREVIEW_W);
   _listbox[LIST_PRESET]->add_item(0, "-=NEW=-");
   fill_preset_listbox(_listbox[LIST_PRESET], _preset_filenames, Config::JOT_ROOT() + PRESET_DIRECTORY);

   new GLUI_Separator(_panel[PANEL_PRESET]);

   //New preset name box
   _edittext[EDITTEXT_SAVE] = new GLUI_EditText(
      _panel[PANEL_PRESET], "", 
      GLUI_EDITTEXT_TEXT, nullptr,
      id+LIST_PRESET, edittext_cb);
//XXX XXX - Wrong id! Fix this and see what breaks...
   assert(_edittext[EDITTEXT_SAVE]);
   _edittext[EDITTEXT_SAVE]->disable();
   _edittext[EDITTEXT_SAVE]->set_w(STROKE_PREVIEW_W);

   new GLUI_Separator(_panel[PANEL_PRESET]);

   //Preset save button
   _button[BUT_SAVE] = new GLUI_Button(
      _panel[PANEL_PRESET], "Save", 
      id+BUT_SAVE, button_cb);
   assert(_button[BUT_SAVE]);
   _button[BUT_SAVE]->set_w(STROKE_PREVIEW_W-6);

   //Preset save button
   _button[BUT_DEBUG] = new GLUI_Button(
      _panel[PANEL_PRESET], "Debug", 
      id+BUT_DEBUG, button_cb);
   assert(_button[BUT_DEBUG]);
   _button[BUT_DEBUG]->set_w(STROKE_PREVIEW_W-6);

   new GLUI_Column(_panel[PANEL_STROKE], false);

   // Preview/test bitmap
   _bitmapbox[BITMAPBOX_PREVIEW] = new GLUI_BitmapBox(
      _panel[PANEL_STROKE], "Preview",
      id+BITMAPBOX_PREVIEW, bitmapbox_cb);
   assert(_bitmapbox[BITMAPBOX_PREVIEW]);
   _bitmapbox[BITMAPBOX_PREVIEW]->set_img_size(STROKE_PREVIEW_W,STROKE_PREVIEW_H);

   // Rollout containing color/texture
   _rollout[ROLLOUT_COLORTEXTURE] = new GLUI_Rollout(_glui, "Stroke Color/Texture", true);
   assert(_rollout[ROLLOUT_COLORTEXTURE]);


   //      COL1  COL2   COL3
   // ROW: R     Tex    Paper
   // ROW: G,    Angle, Cont
   // ROW: B,    Alpha, Brig

   //      COL1  COL2     COL3
   // ROW: R     Tex      Paper
   // ROW: G,    Period,  Cont
   // ROW: B,    Phase,   Brig
   // ROW: A,    Foo2,    Foo

   // COL1
   _slider[SLIDE_H] = new GLUI_Slider(
      _rollout[ROLLOUT_COLORTEXTURE], "Hue", 
      id+SLIDE_H, slider_cb,
      GLUI_SLIDER_FLOAT, 
      0.0, 1.0,
      nullptr);
   assert(_slider[SLIDE_H]);
   _slider[SLIDE_H]->set_num_graduations(201);

   _slider[SLIDE_S] = new GLUI_Slider(
      _rollout[ROLLOUT_COLORTEXTURE], "Saturation", 
      id+SLIDE_S, slider_cb,
      GLUI_SLIDER_FLOAT, 
      0.0, 1.0,
      nullptr);
   assert(_slider[SLIDE_S]);
   _slider[SLIDE_S]->set_num_graduations(201);

   _slider[SLIDE_V] = new GLUI_Slider(
      _rollout[ROLLOUT_COLORTEXTURE], "Value", 
      id+SLIDE_V, slider_cb,
      GLUI_SLIDER_FLOAT, 
      0.0, 1.0,
      nullptr);
   assert(_slider[SLIDE_V]);
   _slider[SLIDE_V]->set_num_graduations(201);

   _slider[SLIDE_A] = new GLUI_Slider(
      _rollout[ROLLOUT_COLORTEXTURE], "Alpha", 
      id+SLIDE_A, slider_cb,
      GLUI_SLIDER_FLOAT, 
      0.0, 1.0,
      &(_params.alpha_()));
   assert(_slider[SLIDE_A]);
   _slider[SLIDE_A]->set_num_graduations(201);

   new GLUI_Column(_rollout[ROLLOUT_COLORTEXTURE], false);

   // COL2
   _panel[PANEL_TEXTURE] = new GLUI_Panel(_rollout[ROLLOUT_COLORTEXTURE], "Texture");
   _listbox[LIST_TEXTURE] = new GLUI_Listbox(
      _panel[PANEL_TEXTURE], 
      "", nullptr,
      id+LIST_TEXTURE, listbox_cb);
   assert(_listbox[LIST_TEXTURE]);
   _listbox[LIST_TEXTURE]->set_w(STROKE_PARAMS_MIN_W);
   _listbox[LIST_TEXTURE]->add_item(0, "----");
   fill_texture_listbox(_listbox[LIST_TEXTURE], _texture_filenames, Config::JOT_ROOT() + TEXTURE_DIRECTORY);

   _slider[SLIDE_ANGLE] = new GLUI_Slider(
      _rollout[ROLLOUT_COLORTEXTURE], "Period", 
      id+SLIDE_ANGLE, slider_cb,
      GLUI_SLIDER_FLOAT, 
      0.0, 2048.0,
      &(_params.angle_()));
   assert(_slider[SLIDE_ANGLE]);
   _slider[SLIDE_ANGLE]->set_num_graduations(2049);

   _slider[SLIDE_PHASE] = new GLUI_Slider(
      _rollout[ROLLOUT_COLORTEXTURE], "Phase", 
      id+SLIDE_PHASE, slider_cb,
      GLUI_SLIDER_FLOAT, 
      0.0, 1.0,
      &(_params.offset_phase_()));
   assert(_slider[SLIDE_PHASE]);
   _slider[SLIDE_PHASE]->set_num_graduations(1001);

   _slider[SLIDE_FOO2] = new GLUI_Slider(
      _rollout[ROLLOUT_COLORTEXTURE], "Foo2", 
      id+SLIDE_FOO2, slider_cb,
      GLUI_SLIDER_FLOAT, 
      0.0, 10,
      nullptr);
   assert(_slider[SLIDE_FOO2]);
   _slider[SLIDE_FOO2]->set_num_graduations(100);
   _slider[SLIDE_FOO2]->disable();

   new GLUI_Column(_rollout[ROLLOUT_COLORTEXTURE], false);

   // COL3
   _panel[PANEL_PAPER] = new GLUI_Panel(_rollout[ROLLOUT_COLORTEXTURE], "Paper");
   _listbox[LIST_PAPER] = new GLUI_Listbox(
      _panel[PANEL_PAPER], 
      "", nullptr,
      id+LIST_PAPER, listbox_cb);
   assert(_listbox[LIST_PAPER]);
   _listbox[LIST_PAPER]->set_w(STROKE_PARAMS_MIN_W);
   _listbox[LIST_PAPER]->add_item(0, "----");
   fill_paper_listbox(_listbox[LIST_PAPER], _paper_filenames, Config::JOT_ROOT() + PAPER_DIRECTORY);

   _slider[SLIDE_CONTRAST] = new GLUI_Slider(
      _rollout[ROLLOUT_COLORTEXTURE], "Contrast", 
      id+SLIDE_CONTRAST, slider_cb,
      GLUI_SLIDER_FLOAT, 
      0.0, 1.0,
      &(_params.contrast_()));
   assert(_slider[SLIDE_CONTRAST]);
   _slider[SLIDE_CONTRAST]->set_num_graduations(201);

   _slider[SLIDE_BRIGHTNESS] = new GLUI_Slider(
      _rollout[ROLLOUT_COLORTEXTURE], "Brightness", 
      id+SLIDE_BRIGHTNESS, slider_cb,
      GLUI_SLIDER_FLOAT, 
      0.0, 1.0,
      &(_params.brightness_()));
   assert(_slider[SLIDE_BRIGHTNESS]);
   _slider[SLIDE_BRIGHTNESS]->set_num_graduations(201);

   _slider[SLIDE_FOO] = new GLUI_Slider(
      _rollout[ROLLOUT_COLORTEXTURE], "Foo", 
      id+SLIDE_FOO, slider_cb,
      GLUI_SLIDER_FLOAT, 
      0.0, 1.0,
      nullptr);
   assert(_slider[SLIDE_FOO]);
   _slider[SLIDE_FOO]->set_num_graduations(101);
   _slider[SLIDE_FOO]->disable();

   // Rollout containing Shape/Appearance 
   _rollout[ROLLOUT_SHAPE] = new GLUI_Rollout(_glui, "Stroke Shape/Appearance", true);
   assert(_rollout[ROLLOUT_SHAPE]);

   //      COL1   COL2      COL3
   // ROW: W,     WTaper,   WFlare
   // ROW: Halo,  AFade,    AFlare

   // XXX - Change so that all 3 columns save same width
   // decided by max of paper and texture list boxes

   // COL1

   _slider[SLIDE_WIDTH] = new GLUI_Slider(
      _rollout[ROLLOUT_SHAPE], "Width", 
      id+SLIDE_WIDTH, slider_cb,
      GLUI_SLIDER_FLOAT, 
      1, 80,
      &(_params.width_()));
   assert(_slider[SLIDE_WIDTH]);
   _slider[SLIDE_WIDTH]->set_num_graduations(159);

   _slider[SLIDE_HALO] = new GLUI_Slider(
      _rollout[ROLLOUT_SHAPE], "Halo", 
      id+SLIDE_HALO, slider_cb,
      GLUI_SLIDER_FLOAT, 
      0, 20,
      &(_params.halo_()));
   assert(_slider[SLIDE_HALO]);
   _slider[SLIDE_HALO]->set_num_graduations(41);
   _slider[SLIDE_HALO]->disable();
   new GLUI_Column(_rollout[ROLLOUT_SHAPE], false);

   // COL2
   _slider[SLIDE_TAPER] = new GLUI_Slider(
      _rollout[ROLLOUT_SHAPE], "Taper", 
      id+SLIDE_TAPER, slider_cb,
      GLUI_SLIDER_FLOAT, 
      0, 200, 
      &(_params.taper_()));
   assert(_slider[SLIDE_TAPER]);
   _slider[SLIDE_TAPER]->set_num_graduations(201);

   _slider[SLIDE_ALPHA_FADE] = new GLUI_Slider(
      _rollout[ROLLOUT_SHAPE], "Fade", 
      id+SLIDE_ALPHA_FADE, slider_cb,
      GLUI_SLIDER_FLOAT, 
      0, 200,
      &(_params.fade_()));
   assert(_slider[SLIDE_ALPHA_FADE]);
   _slider[SLIDE_ALPHA_FADE]->set_num_graduations(201);

   new GLUI_Column(_rollout[ROLLOUT_SHAPE], false);

   // COL3
   _slider[SLIDE_FLARE] = new GLUI_Slider(
      _rollout[ROLLOUT_SHAPE], "Flare", 
      id+SLIDE_FLARE, slider_cb,
      GLUI_SLIDER_FLOAT, 
      0, 1, 
      &(_params.flare_()));
   assert(_slider[SLIDE_FLARE]);
   _slider[SLIDE_FLARE]->set_num_graduations(101);

   _slider[SLIDE_AFLARE] = new GLUI_Slider(
      _rollout[ROLLOUT_SHAPE], "AFlare", 
      id+SLIDE_AFLARE, slider_cb,
      GLUI_SLIDER_FLOAT, 
      0, 1, 
      &(_params.aflare_()));
   assert(_slider[SLIDE_AFLARE]);
   _slider[SLIDE_AFLARE]->set_num_graduations(101);


   // Cleanup sizes
   _column_width = _panel[PANEL_TEXTURE]->get_w();
   if (_column_width < _panel[PANEL_PAPER]->get_w())
   {
      _column_width = _panel[PANEL_PAPER]->get_w();
      _listbox[LIST_TEXTURE]->set_w(_listbox[LIST_PAPER]->get_w());
   }
   else
   {
      _listbox[LIST_PAPER]->set_w(_listbox[LIST_TEXTURE]->get_w());
   }
   for (i=0; i<SLIDE_NUM; i++) 
      _slider[i]->set_w((int)_column_width);

   int button_size = (_rollout[ROLLOUT_SHAPE]->get_w()-_panel[PANEL_PEN]->get_w())/2;
   _button[BUT_PREV_PEN]->set_w(_button[BUT_PREV_PEN]->get_w()+button_size);
   _button[BUT_NEXT_PEN]->set_w(_button[BUT_NEXT_PEN]->get_w()+button_size);

   // First time round, setup the stroke rendering job
   // and size the preview image
   // Otherwise, just fix up the image size
   if (!_init) 
      init();
   else 
      set_preview_size();

   // Add the Plugin Rollout
   _rollout[ROLLOUT_PLUGIN] = new GLUI_Rollout(_glui,
      (string(_client->plugin_name()) + " Plug-in Controls").c_str(), true);
   assert(_rollout[ROLLOUT_PLUGIN]);
   // Tell the client to draw in
   _client->build(_glui, _rollout[ROLLOUT_PLUGIN], _rollout[ROLLOUT_SHAPE]->get_w());

   // XXX - Perhaps should remember this state when changing clients
   _rollout[ROLLOUT_COLORTEXTURE]->close();
   _rollout[ROLLOUT_SHAPE]->close();

   //Show will actually show it...
   _glui->hide(); 
}

/////////////////////////////////////
// destroy
/////////////////////////////////////

void
StrokeUI::destroy() 
{

   assert(_client);
   assert(_glui);

   //Let the client do some cleanup

   _client->destroy(_glui,_rollout[ROLLOUT_PLUGIN]);

   //Hands off these soon to be bad things

   _listbox.clear();
   _button.clear();
   _slider.clear();
   _edittext.clear();
   _panel.clear(); 
   _bitmapbox.clear();
   _rollout.clear();

   _texture_filenames.clear();
   _paper_filenames.clear();
   _preset_filenames.clear();

   //Recursively kills off all controls, and itself
   _glui->close();

   _glui = nullptr;


}

/////////////////////////////////////
// fill_texture_listbox()
/////////////////////////////////////
//
// -Retrieve list of files in texture directory
// -Populate the listbox with the filenames
//
/////////////////////////////////////

void
StrokeUI::fill_texture_listbox(
   GLUI_Listbox   *listbox,
   vector<string> &save_files,
   const string   &full_path
   )
{
   int j=0;
   vector<string> in_files = dir_list(full_path);
   for (auto & file : in_files)
   {
      string::size_type len = file.length();
      if ( (len>3) && 
            (file.substr(len-4) == ".png"))
      {
         save_files.push_back(full_path + file);
         listbox->add_item(1+j++, file.c_str());
      }
   }
}

/////////////////////////////////////
// fill_paper_listbox()
/////////////////////////////////////
//
// -Retrieve list of files in paper directory
// -Populate the listbox with the filenames
//
/////////////////////////////////////

void
StrokeUI::fill_paper_listbox(
   GLUI_Listbox   *listbox,
   vector<string> &save_files,
   const string   &full_path
   )
{
   int j=0;
   vector<string> in_files = dir_list(full_path);
   for (auto & file : in_files)
   {
      string::size_type len = file.length();
      if ( (len>3) && 
            (file.substr(len-4) == ".png"))
      {
         save_files.push_back(full_path + file);
         listbox->add_item(1+j++, file.c_str());
      }
   }
}

/////////////////////////////////////
// fill_preset_listbox()
/////////////////////////////////////
//
// -Retrieve list of files in texture directory
// -Populate the listbox with the filenames
//
/////////////////////////////////////

void
StrokeUI::fill_preset_listbox(
   GLUI_Listbox   *listbox,
   vector<string> &save_files,
   const string   &full_path
   )
{
   vector<string>::size_type i;

   //First clear out any previous presets
   for (i = 1; i <= save_files.size(); i++) {
      int foo = listbox->delete_item(i);
      assert(foo);
   }
   save_files.clear();

   vector<string> in_files = dir_list(full_path);
   for (i = 0; i < in_files.size(); i++) {
      string::size_type len = in_files[i].length();

      if ( (len>3) && (in_files[i].substr(len-4) == ".pre"))
      {
         string basename = in_files[i].substr(0, len-4);

         if ( jot_check_glui_fit(listbox, basename.c_str()) )
         {
            save_files.push_back(full_path + in_files[i]);
            listbox->add_item(save_files.size(), basename.c_str());
         }
         else
         {
            err_mesg(ERR_LEV_WARN, "StrokeUI::fill_preset_listbox - Discarding preset file (name too long for listbox): %s", basename.c_str());
         }
      }
      else if (in_files[i] != "CVS")
      {
         err_mesg(ERR_LEV_WARN, "StrokeUI::fill_preset_listbox - Discarding preset file (bad name): %s", in_files[i].c_str());
      }
   }
}


/////////////////////////////////////
// internal_show()
/////////////////////////////////////

bool
StrokeUI::internal_show()
{
   if (_glui) 
   {
      err_msg("StrokeUI::internal_show() - Error! StrokeUI is already showing!");         
      return false;
   } 
   else 
   {
      build();
      
      AuxRefImage *aux = AuxRefImage::lookup(_view);
      assert(_init);
      if (aux) aux->add(_job);

      update_stroke_img();

      PaperEffect::add_obs(this);

      if (!_glui) 
      {
         err_msg("StrokeUI::internal_show() - Error! StrokeUI failed to build GLUI object!");         
         return false;
      } 
      else 
      {
         _glui->show();
         return true;
      }
   }
}


/////////////////////////////////////
// internal_hide()
/////////////////////////////////////

bool
StrokeUI::internal_hide()
{
   if(!_glui) 
   {
      err_msg("StrokeUI::internal_hide() - Error! StrokeUI is slready hidden!");         
      return false;
   }
   else
   {
      _glui->hide();

      assert(_init);
      AuxRefImage *aux = AuxRefImage::lookup(_view);
      if (aux) aux->rem(_job);

      PaperEffect::rem_obs(this);

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
StrokeUI::internal_update()
{

   if(!_glui) 
   {
      err_msg("StrokeUI::internal_update() - Error! No GLUI object to update (not showing)!");         
      return false;
   }
   else 
   {
      // Update the controls that don't use
      // 'live' variables
      update_non_lives();

      //This presumably means that the params were
      //changed externally. So we move the preset to
      //'new' so as not to unintentionally pollute
      //any existing preset

      _button[BUT_SAVE]->enable();
      _listbox[LIST_PRESET]->set_int_val(0);

      _glui->sync_live();

      update_stroke_img();

      return true;
   }

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
StrokeUI::update_non_lives()
{
   vector<string>::iterator i;

   i = std::find(_paper_filenames.begin(), _paper_filenames.end(), _params.get_paper_file());
   if (i == _paper_filenames.end())
      _listbox[LIST_PAPER]->set_int_val(0);
   else
      _listbox[LIST_PAPER]->set_int_val(i - _paper_filenames.begin() + 1);

   i = std::find(_texture_filenames.begin(), _texture_filenames.end(), _params.get_texture_file());
   if (i == _texture_filenames.end())
      _listbox[LIST_TEXTURE]->set_int_val(0);
   else
      _listbox[LIST_TEXTURE]->set_int_val(i - _texture_filenames.begin() + 1);

   HSVCOLOR hsv(_params.get_color());
   _slider[SLIDE_H]->set_float_val(hsv[0]);
   _slider[SLIDE_S]->set_float_val(hsv[1]);
   _slider[SLIDE_V]->set_float_val(hsv[2]);

}

/////////////////////////////////////
// preset_selected()
/////////////////////////////////////

void
StrokeUI::preset_selected()
{

   int val = _listbox[LIST_PRESET]->get_int_val();

   if (val == 0)
   {
      _button[BUT_SAVE]->enable();
   }
   else
   {
      if (!load_preset(_preset_filenames[val-1].c_str()))
         return;

      // Update the controls that don't use
      // 'live' variables
      update_non_lives();

      _button[BUT_SAVE]->disable();

      _glui->sync_live();

      update_stroke_img();

      assert(_client);
      _client->changed();

   }
        
}

/////////////////////////////////////
// preset_save_button()
/////////////////////////////////////

void
StrokeUI::preset_save_button()
{

   int val = _listbox[LIST_PRESET]->get_int_val();

   if (val == 0)
   {
      //Enter save mode in which only the 
      //filename box is available.
      _glui->disable();
      _edittext[EDITTEXT_SAVE]->enable();

      WORLD::message("Enter preset name in box and hit <ENTER>");

   }
   else
   {

      if (!save_preset(_preset_filenames[val-1].c_str()))
         return;
      _button[BUT_SAVE]->disable();
   }

}

/////////////////////////////////////
// preset_save_text()
/////////////////////////////////////

void
StrokeUI::preset_save_text()
{
   int i,j;
   const char *goodchars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890-_";

   const char *origtext = _edittext[EDITTEXT_SAVE]->get_text();
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
         _edittext[EDITTEXT_SAVE]->set_text(newtext);
         delete(newtext);
         return;
      }

   fix = false;
   while (!jot_check_glui_fit(_listbox[LIST_PRESET], newtext) && strlen(newtext)>0)
      {
         fix = true;
         newtext[strlen(newtext)-1]=0;
      }

   if (fix) 
      {
         WORLD::message("Truncated name to fit listbox. Continue editing.");
         _edittext[EDITTEXT_SAVE]->set_text(newtext);
         delete(newtext);
         return;
      }

   if (save_preset((Config::JOT_ROOT() + PRESET_DIRECTORY + newtext + ".pre").c_str()))
      {
         _preset_filenames.push_back(Config::JOT_ROOT() + PRESET_DIRECTORY + newtext + ".pre");
         _listbox[LIST_PRESET]->add_item(_preset_filenames.size(), newtext);
         _listbox[LIST_PRESET]->set_int_val(_preset_filenames.size());
      }
   else
      {
         WORLD::message("Failed to save.");
      }

   _edittext[EDITTEXT_SAVE]->set_text("");
   _glui->enable();
   _edittext[EDITTEXT_SAVE]->disable();
   _button[BUT_SAVE]->disable();


}

/////////////////////////////////////
// save_preset()
/////////////////////////////////////

bool
StrokeUI::save_preset(const char *f)
{

   fstream fout;
   fout.open(f,ios::out);
   
   if (!fout) 
   {
      err_msg("StrokeUI::save_preset - Error! Could not open file: '%s'", f);         
      return false;
   }

   STDdstream stream(&fout);

   _params.format(stream);

   fout.close();

   return true;

}

/////////////////////////////////////
// load_preset()
/////////////////////////////////////

bool
StrokeUI::load_preset(const char *f)
{

   fstream fin;
#if (defined (WIN32) && defined(_MSC_VER) && (_MSC_VER <=1200)) /*VS 6.0*/
   fin.open(f, ios::in | ios::nocreate);
#else
   fin.open(f, ios::in);
#endif
   if (!fin) 
   {
      err_msg("StrokeUI::load_preset() - Error! Could not open file: '%s'", f);
      return false;
   }

   STDdstream d(&fin);
   string str;

   d >> str;

   if (str == BaseStroke::static_name()) {
      //In case we're testing offsets, let
      //trash the old ones
      _params.set_offsets(nullptr);
      _params.decode(d);
   }
   else
   {
      err_msg("StrokeUI::load_preset() - Error! Not BaseStroke: '%s'", str.c_str());
   }
   fin.close();

   return true;
}

/////////////////////////////////////
// params_changed()
/////////////////////////////////////

void
StrokeUI::params_changed() 
{

   _button[BUT_SAVE]->enable();

   update_stroke_img();

   assert(_client);
   _client->changed();
}

/////////////////////////////////////
// edittext_cb()
/////////////////////////////////////
//
// -Common callback for all buttons
//
/////////////////////////////////////

void
StrokeUI::edittext_cb(int id)
{
   assert((id >> ID_SHIFT) < (int)_ui.size());

   switch(id&ID_MASK) {
       case EDITTEXT_SAVE:
         _ui[id >> ID_SHIFT]->preset_save_text();
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
StrokeUI::listbox_cb(int id)
{
   assert((id >> ID_SHIFT) < (int)_ui.size());

   int i;

   switch (id&ID_MASK) {
       case LIST_TEXTURE:
         i = _ui[id >> ID_SHIFT]->_listbox[LIST_TEXTURE]->get_int_val();
         if (i>0)
            _ui[id >> ID_SHIFT]->_params.set_texture(_ui[id >> ID_SHIFT]->_texture_filenames[i-1]);
         else 
            _ui[id >> ID_SHIFT]->_params.set_texture("");
         _ui[id >> ID_SHIFT]->params_changed();
         break;
       case LIST_PAPER:
         i = _ui[id >> ID_SHIFT]->_listbox[LIST_PAPER]->get_int_val();
         if (i>0)
            _ui[id >> ID_SHIFT]->_params.set_paper(_ui[id >> ID_SHIFT]->_paper_filenames[i-1]);
         else 
            _ui[id >> ID_SHIFT]->_params.set_paper("");
         _ui[id >> ID_SHIFT]->params_changed();
         break;
       case LIST_PRESET:
         _ui[id >> ID_SHIFT]->preset_selected();
         break;
   }
}

/////////////////////////////////////
// button_cb()
/////////////////////////////////////
//
// -Common callback for all buttons
//
/////////////////////////////////////

void
StrokeUI::button_cb(int id)
{
   assert((id >> ID_SHIFT) < (int)_ui.size());

   switch(id&ID_MASK) {
       case BUT_SAVE:
         _ui[id >> ID_SHIFT]->preset_save_button();
       break;
       case BUT_DEBUG:
         //BaseStroke::set_debug(!BaseStroke::get_debug());
         GLExtensions::set_debug(!GLExtensions::get_debug());
         if (GLExtensions::get_debug()) {
            WORLD::message("Debug ON");
         } else {
            WORLD::message("Debug OFF");
         }
         _ui[id >> ID_SHIFT]->params_changed();
      break;
      case BUT_NEXT_PEN:
         BaseJOTapp::instance()->next_pen();
      break;
      case BUT_PREV_PEN:
         BaseJOTapp::instance()->prev_pen();
      break;
   }
}


/////////////////////////////////////
// slider_cb()
/////////////////////////////////////
//
// -Common callback for all sliders
//
/////////////////////////////////////

void
StrokeUI::slider_cb(int id)
{
   assert((id >> ID_SHIFT) < (int)_ui.size());

   HSVCOLOR c;

   switch(id&ID_MASK) {
       case SLIDE_H:
       case SLIDE_S:
       case SLIDE_V:
         c.set(_ui[id >> ID_SHIFT]->_slider[SLIDE_H]->get_float_val(), 
               _ui[id >> ID_SHIFT]->_slider[SLIDE_S]->get_float_val(), 
               _ui[id >> ID_SHIFT]->_slider[SLIDE_V]->get_float_val() );
         _ui[id >> ID_SHIFT]->_params.set_color(c);
         _ui[id >> ID_SHIFT]->params_changed();
         break;
       case SLIDE_A:
         _ui[id >> ID_SHIFT]->params_changed();
         break;
       case SLIDE_ALPHA_FADE:
         _ui[id >> ID_SHIFT]->params_changed();
         break;
       case SLIDE_WIDTH:
         _ui[id >> ID_SHIFT]->params_changed();
         break;
       case SLIDE_HALO:
         _ui[id >> ID_SHIFT]->params_changed();
         break;
       case SLIDE_TAPER:
         _ui[id >> ID_SHIFT]->params_changed();
         break;
       case SLIDE_FLARE:
         _ui[id >> ID_SHIFT]->params_changed();
         break;
       case SLIDE_AFLARE:
         _ui[id >> ID_SHIFT]->params_changed();
         break;
       case SLIDE_ANGLE:
         _ui[id >> ID_SHIFT]->params_changed();
         break;
       case SLIDE_CONTRAST:
         _ui[id >> ID_SHIFT]->params_changed();
         break;
       case SLIDE_BRIGHTNESS:
         _ui[id >> ID_SHIFT]->params_changed();
         break;
       case SLIDE_PHASE:
         _ui[id >> ID_SHIFT]->params_changed();
         break;
       case SLIDE_FOO2:
         _ui[id >> ID_SHIFT]->params_changed();
         break;
   }
}


/////////////////////////////////////
// bitmapbox_cb()
/////////////////////////////////////
//
// -Common callback for all bitmapboxes
//
/////////////////////////////////////

void
StrokeUI::bitmapbox_cb(int id)
{
   assert((id >> ID_SHIFT) < (int)_ui.size());

   switch(id&ID_MASK) {
       case BITMAPBOX_PREVIEW:
         _ui[id >> ID_SHIFT]->handle_preview_bitmapbox();
       break;
   }
}

/////////////////////////////////////
// stroke_img_updated()
/////////////////////////////////////

void
StrokeUI::stroke_img_updated() 
{
   err_mesg(ERR_LEV_SPAM, "StrokeUI::stroke_img_updated()");

   assert(_init);

   _bitmapbox[BITMAPBOX_PREVIEW]->copy_img(
      _job->image().data(),
      _job->image().width(),
      _job->image().height(),_job->image().bpp());
}


/////////////////////////////////////
// update_stroke_img()
/////////////////////////////////////

void
StrokeUI::update_stroke_img() 
{
   err_mesg(ERR_LEV_SPAM, "StrokeUI::update_stroke_img()");

   for (int i=0; i<_strokes.num();i++)
      {
         _strokes[i]->stroke()->copy(_params);
      }

   assert(_init);
   _job->set_dirty();
}


/////////////////////////////////////
// xy_to_ndcz()
/////////////////////////////////////
        
NDCZpt          
StrokeUI::xy_to_ndcz(int x, int y)
{
   //Need a custom conversion since the internal
   //pix->ndc stuff uses the current view's dims
        
//   float xy_x, xy_y;

   float i_w_2 = (float)_bitmapbox[BITMAPBOX_PREVIEW]->get_image_w()/2.0;
   float i_h_2 = (float)_bitmapbox[BITMAPBOX_PREVIEW]->get_image_h()/2.0;
   float ratio = i_w_2/i_h_2;

   if (ratio >= 1)
      return NDCZpt(((float)x - i_w_2)/i_w_2*ratio,((float)y - i_h_2)/i_h_2,0);
   else
      return NDCZpt(((float)x - i_w_2)/i_w_2,(((float)y - i_h_2)/i_h_2)/ratio,0);
}

/////////////////////////////////////
// handle_preview_bitmapbox()
/////////////////////////////////////

void
StrokeUI::handle_preview_bitmapbox() 
{
   int x, y, k, m;
   int event = _bitmapbox[BITMAPBOX_PREVIEW]->get_event();
   NDCZpt ndcz;

   switch (event)
      {
       case GLUI_BITMAPBOX_EVENT_NONE:
         //If we get here, there should be a pending event...
         err_msg("StrokeUI::handle_preview_bitmapbox() - No event!!!!");
       break;
       case GLUI_BITMAPBOX_EVENT_MOUSE_DOWN:
         x = _bitmapbox[BITMAPBOX_PREVIEW]->get_event_x();
         y = _bitmapbox[BITMAPBOX_PREVIEW]->get_event_y();

         ndcz = xy_to_ndcz(x,y);
         _strokes[_curr_stroke]->pts().clear();
         _strokes[_curr_stroke]->pts().push_back(ndcz);
         update_stroke_img();
       break;
       case GLUI_BITMAPBOX_EVENT_MOUSE_MOVE:
         x = _bitmapbox[BITMAPBOX_PREVIEW]->get_event_x();
         y = _bitmapbox[BITMAPBOX_PREVIEW]->get_event_y();

         ndcz = xy_to_ndcz(x,y);
         if ((_strokes[_curr_stroke]->pts().size() == 0) ||
             (_strokes[_curr_stroke]->pts().back() != ndcz))
            _strokes[_curr_stroke]->pts().push_back(ndcz);
         update_stroke_img();
       break;
       case GLUI_BITMAPBOX_EVENT_MOUSE_UP:
         x = _bitmapbox[BITMAPBOX_PREVIEW]->get_event_x();
         y = _bitmapbox[BITMAPBOX_PREVIEW]->get_event_y();

         _curr_stroke = (_curr_stroke+1)%_strokes.num();
       break;
       case GLUI_BITMAPBOX_EVENT_KEY:
         k = _bitmapbox[BITMAPBOX_PREVIEW]->get_event_key();
         m = _bitmapbox[BITMAPBOX_PREVIEW]->get_event_mod();
       break;
       case GLUI_BITMAPBOX_EVENT_MIDDLE_DOWN:
       case GLUI_BITMAPBOX_EVENT_MIDDLE_MOVE:
       case GLUI_BITMAPBOX_EVENT_MIDDLE_UP:
       case GLUI_BITMAPBOX_EVENT_RIGHT_DOWN:
       case GLUI_BITMAPBOX_EVENT_RIGHT_MOVE:
       case GLUI_BITMAPBOX_EVENT_RIGHT_UP:
       break;
       default:
         err_msg("StrokeUI::handle_preview_bitmapbox() - Unknown event!!!");
       break;
      }

}
