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
/**********************************************************************
 * fader_texture.H:
 **********************************************************************/
#ifndef FADER_TEXTURE_H_IS_INCLUDED
#define FADER_TEXTURE_H_IS_INCLUDED

#include "mesh/gtexture.H"

/**********************************************************************
 * FaderTexture:
 *
 *      Fades from one texture to another over a given time interval.
 **********************************************************************/
class FaderTexture : public GTexture {
 public:
   //******** MANAGERS ********
   FaderTexture(GTexture* base, GTexture* fader, double start, double dur) :
      GTexture(base->patch()),
      _base(base),
      _fader(fader),
      _start_time(start),
      _duration(dur) {
      _base->set_ctrl(this);      // take control of the base
   }

   virtual ~FaderTexture() {}

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS2("FaderTexture", GTexture, CDATA_ITEM*);

   //******** GTexture VIRTUAL METHODS ********
   virtual int draw(CVIEWptr& v);

   //******** DATA_ITEM VIRTUAL METHODS ********
   virtual DATA_ITEM  *dup() const { return 0; }

 protected:
   GTexture*    _base;          // texture that is fading in
   GTexture*    _fader;         // texture that is fading away
   double       _start_time;    // frame time at start of fade
   double       _duration;      // fade duration (in frame time)

   // convenience: convert time to opacity (alpha)
   double fade(double t) const {
      return 1.0 - clamp((t - _start_time)/_duration, 0.0, 1.0);
   }
};

#endif // FADER_TEXTURE_H_IS_INCLUDED

/* end of file fader_texture.H */
