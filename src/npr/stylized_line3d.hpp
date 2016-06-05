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
#ifndef STYLIZED_LINE_3D_H_IS_INCLUDED
#define STYLIZED_LINE_3D_H_IS_INCLUDED

#include "geom/line3d.H"
#include "wpath_stroke.H"

/*****************************************************************
 * StylizedLine3D:
 *
 *   A polyline that can be rendered and intersected.
 *****************************************************************/
#define CStylizedLine3D    const StylizedLine3D
#define CStylizedLine3Dptr const StylizedLine3Dptr

MAKE_PTR_SUBC(StylizedLine3D,LINE3D);
class StylizedLine3D : public LINE3D {
 public:

   //******** MANAGERS ********

   StylizedLine3D(mlib::CWpt_list& pts = mlib::Wpt_list());

   virtual ~StylizedLine3D();

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("StylizedLine3D", StylizedLine3D*, LINE3D, CDATA_ITEM*);

   //******** RefImageClient METHODS ********

   virtual int draw(CVIEWptr&);
   virtual int draw_final(CVIEWptr&);  
   virtual int draw_id_ref_pre1();
   virtual int draw_id_ref_pre2();
   virtual int draw_id_ref_pre3();
   virtual int draw_id_ref_pre4();
 
   // draw to the visibility reference image (for picking)
   virtual void request_ref_imgs();
   
   //******** RefImageClient VIRTUAL METHODS ********

   void get_style ();

   //******** DATA_ITEM VIRTUAL METHODS ********

   virtual DATA_ITEM* dup() const { return new StylizedLine3D(); }

 protected:

   //******** MEMBER VARIABLES ********  
   
   bool         _got_style;     // to make style be applied only once
   WpathStroke* _stroke_3d;     // for rendering the curve
};

#endif // STYLIZED_LINE_3D_H_IS_INCLUDED

// end of file stylized_line3d.H
