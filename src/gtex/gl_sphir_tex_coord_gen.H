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
/*****************************************************************
 * Automatic spherical coordinates
 * by Karol Szserszen
 *
 *****************************************************************/
#ifndef GL_SPHIR_TEX_COORD_GEN_H_IS_INCLUDED
#define GL_SPHIR_TEX_COORD_GEN_H_IS_INCLUDED

#include "mesh/tex_coord_gen.H"

class GLSphirTexCoordGen : public TexCoordGen {


 public:

   virtual void setup();

   virtual UVpt uv_from_vert(CBvert* v, CBface* f);

 protected:

    UVpt compute_uv(CBvert* v);
};

#endif // GL_SPHIR_TEX_COORD_GEN_H_IS_INCLUDED

// end of file gl_sphir_tex_coord_gen.H
