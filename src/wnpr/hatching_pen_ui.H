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
#ifndef _HATCHING_PEN_UI_H_IS_INCLUDED_
#define _HATCHING_PEN_UI_H_IS_INCLUDED_

////////////////////////////////////////////
// HatchingPenUI
////////////////////////////////////////////
//
// -Manages a GLUI window that handles the interface
// -Actual 'live' variables reside within HatchingPen
// -Selection updates these vars, and update() refreshes the widgets
// -Apply button tells HatchingPen to apply the current settings
//
////////////////////////////////////////////


#include "wnpr/stroke_ui.H"

class HatchingPen;

class GLUI;
class GLUI_Listbox;
class GLUI_Button;
class GLUI_Slider;
class GLUI_StaticText;
class GLUI_Panel;
class GLUI_Checkbox;

/*****************************************************************
 * HatchingPenUI
 *****************************************************************/
class HatchingPenUI : public StrokeUIClient {
 public:
	/******** WIDGET ID NUMBERS ********/
	enum button_id_t {
		BUT_TYPE = 0,
      BUT_CURVES,
      BUT_STYLE,
		BUT_DELETE_ALL,
      BUT_UNDO_LAST,
      BUT_APPLY,
      BUT_NUM
   };

   enum slider_id_t {
		SLIDE_LO_THRESH = 0,
		SLIDE_HI_THRESH,
		SLIDE_LO_WIDTH,
		SLIDE_HI_WIDTH,
      SLIDE_GLOBAL_SCALE,
      SLIDE_LIMIT_SCALE,
      SLIDE_FOO,
		SLIDE_TRANS_TIME,
      SLIDE_MAX_LOD,
		SLIDE_NUM
   };

	enum checkbox_id_t {
		CHECK_CONFIRM = 0, 
		CHECK_NUM
   };

	enum panel_id_t {
		PANEL_TYPE = 0,
		PANEL_CURVES,
		PANEL_STYLE,
      PANEL_DELETE_ALL,
      PANEL_UNDO_LAST,
      PANEL_APPLY,
      PANEL_CONTROLS,
		PANEL_NUM
   };
   enum rollout_id_t {
      ROLLOUT_ANIM=0,
      ROLLOUT_NUM
   };

 protected:
	// Since we must use static members for callbacks,
	// and we *might* (?) have more that one of these
	// instantiated, we maintain a list of all these ui's
	// and use the callback id (shifted by ID_SHIFT)
	// to identify the appropriate object...
	 
	/******** STATIC MEMBERS VARS ********/

	static ARRAY<HatchingPenUI*>	_ui;

	/******** MEMBERS VARS ********/

	int							_id;

   HatchingPen*				_pen;

	ARRAY<GLUI_Button*>		_button;
	ARRAY<GLUI_Slider*>		_slider;
	ARRAY<GLUI_Checkbox*>	_checkbox;
	ARRAY<GLUI_Panel*>		_panel;
	ARRAY<GLUI_Rollout*>		_rollout;

	int							_delete_confirm;

 public:

   /******** CONSTRUCTOR/DECONSTRUCTOR ********/
   HatchingPenUI(HatchingPen* pen);
   virtual ~HatchingPenUI();

	/******** MEMBERS METHODS ********/
   void			show();
   void			hide();
	void			update();

 protected:

	/******** STATIC CALLBACK METHODS ********/
   static void button_cb(int id);
   static void checkbox_cb(int id);
   static void slider_cb(int id);

   // ******** StrokeUIClient Method ********
	virtual void	      build(GLUI* g, GLUI_Panel *p, int w);
	virtual void	      destroy(GLUI* g, GLUI_Panel *p);
	virtual void	      changed();
   virtual const char * plugin_name() { return "Hatching";}  

};




#endif // _HATCHING_PEN_UI_H_IS_INCLUDED_

/* end of file hatching_pen_ui.H */
