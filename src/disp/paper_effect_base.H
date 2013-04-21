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
#ifndef PAPER_EFFECT_BASE_IS_INCLUDED
#define PAPER_EFFECT_BASE_IS_INCLUDED

#include "std/support.H"
#include "mlib/global.H" // for bool

/*****************************************************************
 * PaperEffectObs
 *
 * PaperEffectObs's are notified when ever some client
 * toggles usage (i.e. NPRview calls usage_toggled() when
 * the effect is turned on/off) or when the paper is
 * changed...
 *
 *****************************************************************/
class PaperEffectObs {
 public:
   virtual ~PaperEffectObs() {}
   virtual void      usage_changed() {}
   virtual void      paper_changed() {}
};

/*****************************************************************
 * PaperEffectBase:
 *
 *   Base class for PaperEffect (see npr/paper_effect.H),
 *   which achieves the media simulation described in the
 *   Siggraph 2002 paper: WYSIWYG NPR.
 *
 *   This is here in the disp library so serialization code
 *   in view.C can read and write parameters used by the
 *   PaperEffect class without referencing the npr library.
 *
 *****************************************************************/
class PaperEffectBase {
 protected:
   /******** STATIC MEMBER VARIABLES ********/
   static ARRAY<PaperEffectObs*>    _obs;

   static str_ptr    _paper_tex;   
   static str_ptr    _paper_filename;

   static bool       _delayed_activate;
   static bool       _delayed_activate_state;
   static bool       _is_inited;
   static bool       _is_supported;
   static bool       _is_active;

   static float      _brig;
   static float      _cont;

   /******** INTERNAL METHODS ********/
   static void    notify_usage_toggled();
   static void    notify_paper_changed();

 public:
   //******** MANAGERS ********
   PaperEffectBase()            {}
   virtual ~PaperEffectBase()   {}

   /******** INTERFACE METHODS ********/
   static void    rem_obs(PaperEffectObs* p)    { if (p) _obs -= p; }
   static void    add_obs(PaperEffectObs* p)    { if (p) _obs += p; }

   static str_ptr get_paper_tex()               { return _paper_tex; }
   static void    set_paper_tex(Cstr_ptr &p)    { _paper_tex = p; notify_paper_changed(); }

   static bool    is_active()                   { return _is_active; }

   static float   get_brig ()                   { return _brig; }
   static void    set_brig (float b)            { _brig = b; }

   static float   get_cont ()                   { return _cont; }
   static void    set_cont (float c)            { _cont = c; }

   static void    set_delayed_activate(bool a)  { _delayed_activate = true; _delayed_activate_state = a; }
};

#endif // PAPER_EFFECT_BASE_IS_INCLUDED

/* end of file paper_effect_base.H */
