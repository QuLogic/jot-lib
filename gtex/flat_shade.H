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
#ifndef FLAT_SHADE_H_IS_INCLUDED
#define FLAT_SHADE_H_IS_INCLUDED

/*!
 *  \file flat_shade.H
 *  \brief Contains the definition of the FlatShadeTexture GTexture and related
 *  classes.
 *
 *  \sa flat_shade.C
 *
 */

#include "basic_texture.H"
#include "gl_sphir_tex_coord_gen.H"

/*!
 *  \brief A callback class for rendering each face of a Patch with a flat
 *  shading style.
 *
 *  This is the GLStripCB for the FlatShadeTexture GTexture.
 *
 *  \sa FlatShadeTexture
 *
 */
class FlatShadeStripCB : public GLStripCB {
   
 public:
   
   FlatShadeStripCB() : _do_texcoords(false) {
      _auto_UV = new GLSphirTexCoordGen;
      _auto_UV->setup();
      _use_auto=false;
   }

   virtual ~FlatShadeStripCB() { delete _auto_UV; }
      
   //! \name Accessor Functions
   //@{
      
   void enable_texcoords() { _do_texcoords = true; }
   void disable_texcoords() { _do_texcoords = false; }
   bool texcoords_enabled() { return _do_texcoords; }

   void enable_autoUV() { _use_auto = true; }
   void disable_autoUV() { _use_auto =false; }
   bool autoUV_enabled() { return _use_auto; }
      
   //@}
      
   //! \name Callback Hooks
   //@{
   
   virtual void faceCB(CBvert* v, CBface* f);
      
   //@}
      
 private:
   //! Should texture coordinates be sent to OpenGL with each vertex?
   bool         _do_texcoords;
   bool         _use_auto;
   TexCoordGen* _auto_UV;
};

/*!
 *  \brief A GTexture that renders Patches in a flat shaded style.
 *
 */
class FlatShadeTexture : public BasicTexture {
   
 public:
   
   FlatShadeTexture(Patch* patch = 0, StripCB* cb=0) :
      BasicTexture(patch, cb ? cb : new FlatShadeStripCB),
      _debug_uv_in_dl(false),
      _has_uv_coords(false),
      _check_uv_coords_stamp(0) {}
      
   virtual ~FlatShadeTexture();
      
   //! \name Run-Time Type Id
   //@{
            
   DEFINE_RTTI_METHODS3("Flat Shading", FlatShadeTexture*, BasicTexture, CDATA_ITEM*);

      
   //@}
      
   //! \name Statics
   //@{
      
   static void toggle_debug_uv() { _debug_uv = !_debug_uv; }
   static bool debug_uv()        { return _debug_uv; }
      
   //@}
      
   //! \name GTexture Virtual Methods
   //@{
      
   virtual int draw(CVIEWptr& v);
      
   //@}
      
   //! \name DATA_ITEM Virtual Methods
   //@{
      
   virtual DATA_ITEM  *dup() const { return new FlatShadeTexture; }
      
   //@}

 protected:
   
   //! True if "debug uv" calls are in current display list.
   bool         _debug_uv_in_dl;
      
   //! True if some faces have uv-coords.
   bool         _has_uv_coords;
      
   //! _patch->stamp() at last check for uv-coords.
   uint         _check_uv_coords_stamp;
   TEXTUREptr   _debug_uv_tex;   //!< texture image for revealing uv-coords
   str_ptr      _debug_tex_path; //!< path to above image
      
   //! Tells whether to make "debug uv" calls when rendering:
   static bool _debug_uv;
};

#endif // FLAT_SHADE_H_IS_INCLUDED

// end of file flat_shade.H
