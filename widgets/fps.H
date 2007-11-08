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
#ifndef FPS_H_IS_INCLUDED
#define FPS_H_IS_INCLUDED

#include "std/stop_watch.H"
#include "disp/gel.H"
#include "geom/text2d.H"

/*****************************************************************
 * FPS:
 *
 *      Displays frames per second in lower left of window.
 *****************************************************************/
MAKE_PTR_SUBC(FPS, FRAMEobs);
class FPS : public FRAMEobs {
  public:

   //******** MANAGERS ********
   FPS();

   //******** FRAMEobs VIRTUAL METHODS ********
   int tick(void);

  protected:
   TEXT2Dptr    _text;          // text that is displayed
   unsigned int _last_display;  // VIEW::stamp() when last updated
   stop_watch   _clock;         // clock measuring time since update
};

#endif // FPS_H_IS_INCLUDED
