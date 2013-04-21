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
/***************************************************************************
    proxy_pen_ui.H
 ***************************************************************************/
#ifndef _PROXY_PEN_UI_H_IS_INCLUDED_
#define _PROXY_PEN_UI_H_IS_INCLUDED_

#include "gui/base_ui.H"

class ProxyPen;
class BaseStroke;
class LightUI;
class PatchUI;
class HatchingUI;
class HalftoneUI;
class ToneShaderUI;
class ProxyTextureUI;
class PainterlyUI;

class ProxyPenUI: public BaseUI {
 public:
   //******** ENUMS ********  
 public:
   ProxyPenUI();
   virtual ~ProxyPenUI() {}
   
   virtual  void     update_non_lives();
   virtual  void     build(GLUI*, GLUI_Panel*, bool open);   
   LightUI*          get_light_ui() const { return _light_ui; }
 protected:        
    //******** MEMBERS METHODS ********   
   //******** MEMBERS ********     
   LightUI*          _light_ui;
   PatchUI*          _patch_ui;
   HatchingUI*       _hatching_ui;
   HalftoneUI*       _halftone_ui;
   //ToneShaderUI*     _tone_shader_ui;
   ProxyTextureUI*   _proxy_texture_ui;
   PainterlyUI*   _painterly_ui;
   //******** CALLBACK METHODS ********    
   //******** Convenience Methods ********  
};
#endif
