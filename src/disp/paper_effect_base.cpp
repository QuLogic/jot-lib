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
#include "paper_effect_base.hpp"

/////////////////////////////////////
// Static Variable Initialization
/////////////////////////////////////
set<PaperEffectObs*>    PaperEffectBase::_obs;

string                  PaperEffectBase::_paper_tex;
string                  PaperEffectBase::_paper_filename = "";

bool                    PaperEffectBase::_delayed_activate = false;
bool                    PaperEffectBase::_delayed_activate_state = false;
bool                    PaperEffectBase::_is_inited = false;
bool                    PaperEffectBase::_is_supported = false;
bool                    PaperEffectBase::_is_active = false;

float                   PaperEffectBase::_brig = 0.5f;
float                   PaperEffectBase::_cont = 0.5f;

////////////////////////////////////////////////////////////////////////////////
// PaperEffectBase Methods
////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////
// notify_paper_changed()
/////////////////////////////////////

void
PaperEffectBase::notify_paper_changed()
{
   set<PaperEffectObs*>::iterator it;
   for (it=_obs.begin(); it!=_obs.end(); ++it)
      (*it)->paper_changed();
}

/////////////////////////////////////
// notify_usage_toggled()
/////////////////////////////////////

void
PaperEffectBase::notify_usage_toggled()
{
   set<PaperEffectObs*>::iterator it;
   for (it=_obs.begin(); it!=_obs.end(); ++it)
      (*it)->usage_changed();
}
