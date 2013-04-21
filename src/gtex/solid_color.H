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
 * solid_color.H:
 **********************************************************************/
#ifndef SOLID_COLOR_H_IS_INCLUDED
#define SOLID_COLOR_H_IS_INCLUDED

#include "basic_texture.H"

/**********************************************************************
 * SolidColorTexture:
 *
 *      XXX - should rename UnlitColorTexture or something
 *            this draws the patch without lighting
 **********************************************************************/
class SolidColorTexture : public BasicTexture {
 public:
   static TAGlist*      _solid_color_texture_tags;

   //******** MANAGERS ********
   SolidColorTexture(Patch* patch = 0,
                     CCOLOR& col = COLOR::white,
                     StripCB* cb = 0) :
      BasicTexture(patch, cb ? cb : new GLStripCB),
      _color(col),
      _alpha(1),
      _track_view_color(false) {}

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3(
      "SolidColoring", SolidColorTexture*, BasicTexture, CDATA_ITEM*
      );

   // Set this to make the GTexture track the view's background color:
   void set_track_view_color(bool b = true) { _track_view_color = b; }

   CCOLOR& get_color()   const { return _color; }
   //******** GTexture VIRTUAL METHODS ********
   virtual int  set_color(CCOLOR& c)   { _color = c; return 1; }
   virtual void set_alpha(double a)    { _alpha = a; }
   virtual int draw(CVIEWptr& v); 
   virtual int  write_stream(ostream& os) const;
   virtual int  read_stream (istream& is, str_list &leftover);

   //******** DATA_ITEM VIRTUAL METHODS ********
   virtual DATA_ITEM  *dup() const { return new SolidColorTexture; }
   virtual CTAGlist &tags         ()             const;

   /******** I/O functions ********/
   COLOR&         color_()             { return _color;  }

 protected:
   COLOR        _color;
   double       _alpha;
   bool         _track_view_color;
};


/**********************************************************************
 * VertColorStripCB:
 *
 *      for drawing individual vertex colors
 **********************************************************************/
class VertColorStripCB : public GLStripCB {
 public:
   virtual void faceCB(CBvert* v, CBface*) {
      if (v->has_color())
         GL_COL(v->color(), v->alpha());
      glVertex3dv(v->loc().data());
   }
};

/**********************************************************************
 * VertColorTexture:
 *
 *      draws mesh with vertex colors, no lighting.
 **********************************************************************/
class VertColorTexture : public SolidColorTexture {
 public:
   //******** MANAGERS ********
   VertColorTexture(Patch* patch = 0, StripCB* cb=0) :
      SolidColorTexture(patch, COLOR::white, cb ? cb : new VertColorStripCB) {}

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS2("Vertex colors", BasicTexture, CDATA_ITEM *);

   //******** DATA_ITEM METHODS ********
   virtual DATA_ITEM  *dup() const { return new VertColorTexture; }
};

#endif // SOLID_COLOR_H_IS_INCLUDED

/* end of file solid_color.H */
