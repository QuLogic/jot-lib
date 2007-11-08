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
#include "paper_effect_base.H"

/////////////////////////////////////
// Static Variable Initialization
/////////////////////////////////////
ARRAY<PaperEffectObs*>  PaperEffectBase::_obs;

str_ptr                 PaperEffectBase::_paper_tex;
str_ptr                 PaperEffectBase::_paper_filename = str_ptr("");

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
   for (int i=0; i<_obs.num(); i++)
      _obs[i]->paper_changed();
}

/////////////////////////////////////
// notify_usage_toggled()
/////////////////////////////////////

void
PaperEffectBase::notify_usage_toggled()
{
   for (int i=0; i<_obs.num(); i++)
      _obs[i]->usage_changed();
}

/* end of file paper_effect_base.C */

