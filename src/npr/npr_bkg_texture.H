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
 * npr_bkg_texture.H
 **********************************************************************/
#ifndef NPR_BKG_TEXTURE_H_IS_INCLUDED
#define NPR_BKG_TEXTURE_H_IS_INCLUDED

#include "geom/texturegl.H"    // for ConventionalTexture
#include "gtex/basic_texture.H"

class NPRBkgTexCB : public GLStripCB {
 public:
   virtual void faceCB(CBvert* v, CBface* f);
};

class NPRBkgTexture : public BasicTexture {
   /******** STATIC MEMBER VARIABLES ********/
   static TAGlist       *_nbt_tags;

 protected:
   //******** Member Variables ********
   int                  _transparent;
   int                  _annotate;

 public:
    // ******** CONSTRUCTOR ********
   NPRBkgTexture(Patch* patch = 0) :
      BasicTexture(patch, new NPRBkgTexCB), _transparent(1), _annotate(1) {}

   //******** Member Methods ********
 public:
   //******** Accessors ********
   COLOR                get_color() const {  return VIEW::peek()->color();    }
   double               get_alpha() const {  return VIEW::peek()->get_alpha();}

   // XXX - Deprecated
   virtual int          get_transparent() const    { return _transparent;     }
   virtual int          get_annotate() const       { return _annotate;     }

 protected:
   //******** Internal Methods ********
   void                 update(CVIEWptr& v);

 public:
   //******** GTexture VIRTUAL METHODS ********
   virtual int          draw(CVIEWptr& v); 

   //******** DATA_ITEM VIRTUAL METHODS ********
   virtual DATA_ITEM*   dup() const                { return new NPRBkgTexture; }
   virtual CTAGlist&    tags() const;

   //******** IO Methods **********

   // XXX - Deprecated
   void                 put_transparent(TAGformat &d) const;
   void                 get_transparent(TAGformat &d);
   void                 put_annotate(TAGformat &d) const;
   void                 get_annotate(TAGformat &d);

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS2("NPRBkgTexture", OGLTexture, CDATA_ITEM *);

};

#endif // NPR_BKG_TEXTURE_H_IS_INCLUDED

/* end of file npr_bkg_texture.H */
