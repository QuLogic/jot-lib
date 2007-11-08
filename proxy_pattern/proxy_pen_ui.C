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
    proxy_pen_ui.C
  ***************************************************************************/
#include "std/support.H"
#include "glew/glew.H"

#include "geom/winsys.H"
#include "glui/glui.h" 

#include "gui/light_ui.H"
#include "gui/patch_ui.H"
#include "gui/hatching_ui.H"
#include "gui/halftone_ui.H"
#include "gui/tone_shader_ui.H"
#include "gui/proxy_texture_ui.H"
#include "gui/gui.H"
#include "gui/painterly_ui.H"

#include "proxy_pen_ui.H"

ProxyPenUI::ProxyPenUI() :
   BaseUI("Proxy Pen UI")   
{
   _light_ui = new LightUI(VIEW::peek());
   _patch_ui = new PatchUI(this);
   _hatching_ui = new HatchingUI(this);
   _halftone_ui = new HalftoneUI(this);
  // _tone_shader_ui = new ToneShaderUI(this);
   _proxy_texture_ui = new ProxyTextureUI(this);
   _painterly_ui = new PainterlyUI(this);
}

void
ProxyPenUI::update_non_lives()
{   
   //Need to take care of updates for all child UI's   
   _light_ui->update_non_lives();
   _patch_ui->update_non_lives();
   _hatching_ui->update_non_lives();
   _halftone_ui->update_non_lives();
  // _tone_shader_ui->update_non_lives();
   _proxy_texture_ui->update_non_lives();
   _painterly_ui->update_non_lives();
}


void
ProxyPenUI::build(GLUI* glui, GLUI_Panel* base, bool open)
{   
   _patch_ui->build(glui,NULL, true);
   _light_ui->build(glui,NULL, false);
   _hatching_ui->build(glui,NULL, false);
   _halftone_ui->build(glui,NULL, false);
   //_tone_shader_ui->build(glui,NULL, false); 
   _proxy_texture_ui->build(glui, NULL, false);
   _painterly_ui->build(glui, NULL, false);
}
