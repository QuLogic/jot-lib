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
#ifndef SMOOTH_SHADE_H_IS_INCLUDED
#define SMOOTH_SHADE_H_IS_INCLUDED

/*!
 *  \file smooth_shade.H
 *  \brief Contains the definition of the SmoothShadeTexture class and related
 *  classes.
 *
 *  \sa smooth_shade.C
 *
 */

#include "basic_texture.H"

/*!
 *  \brief A callback class for rendering each faces of a Patch with a smooth
 *  (gouraud) shading style.
 *
 *  Handles callbacks for drawing triangle strips for the SmoothShadeTexture
 *  GTexture.
 *
 *  \sa SmoothShadeTexture
 *
 */
class SmoothShadeStripCB : public GLStripCB {
   
   public:
   
      SmoothShadeStripCB() : do_texcoords(false) { }
      
      //! \name Accessor Functions
      //@{
      
      void enable_texcoords() { do_texcoords = true; }
      void disable_texcoords() { do_texcoords = false; }
      bool texcoords_enabled() { return do_texcoords; }
      
      //@}
      
      //! \name Callback Hooks
      //@{

      //! \brief "face" callback.
      //!
      //! Issue vertex normals suitable for gouraud shading
      //! (plus colors, and texture and spatial coordinates)
      //! to OpenGL when drawing triangle strips.
      virtual void faceCB(CBvert* v, CBface* f);
      
      //@}
      
   private:
   
      //! Should texture coordinates be sent to OpenGL with each vertex?
      bool do_texcoords;

};

/*!
 *  \brief A GTexture that renders Patches with a smooth (gouraud) shaded style.
 *
 */
class SmoothShadeTexture : public BasicTexture {
   
   public:

      SmoothShadeTexture(Patch* patch = 0, StripCB* cb=0) :
         BasicTexture(patch, cb ? cb : new SmoothShadeStripCB) {}
      
      //! \name Run-Time Type Id
      //@{
      
      DEFINE_RTTI_METHODS3("Smooth Shading", SmoothShadeTexture*,
                           BasicTexture, CDATA_ITEM *);
      
      //@}
      
      //! \name GTexture Virtual Methods
      //@{
      
      virtual int draw(CVIEWptr& v); 
      
      //@}
      
      //! \name DATA_ITEM Virtual Methods
      //@{
      
      virtual DATA_ITEM  *dup() const { return new SmoothShadeTexture; }
      
      //@}
   
};

#endif // SMOOTH_SHADE_H_IS_INCLUDED

// end of file smooth_shade.H
