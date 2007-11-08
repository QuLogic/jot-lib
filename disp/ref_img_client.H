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
 * ref_img_client.H
 **********************************************************************/
#ifndef REF_IMG_CLIENT_IS_INCLUDED
#define REF_IMG_CLIENT_IS_INCLUDED

#include "net/data_item.H"
#include "disp/colors.H"

#define CVIEWptr const VIEWptr
class VIEWptr;

/*****************************************************************
 * RefImageClient:
 *
 *    An abstract base class that can make approprite drawing
 *    calls for creating various types of reference images.
 *
 *    (A reference image is an image read from the frame buffer
 *    into main memory so that it can be used by the program for
 *    some purpose.)
 *****************************************************************/
class RefImageClient {
 public:
   //******** MANAGERS ********
   RefImageClient() {}
   virtual ~RefImageClient() {}

   //******** RUN-TIME TYPE ID ********
   static  STAT_STR_RET static_name() { RET_STAT_STR("RefImageClient"); } 

   //******** DRAWING ********

   // XXX - should draw methods be const?

   // request needed reference images
   // e.g. via: ColorRefImage::schedule_update()
   virtual void request_ref_imgs() {}

   // main draw method (draw final image):
   virtual int draw(CVIEWptr&) = 0;

   // draw color reference image number i:
   virtual int draw_color_ref(int i)    { return 0; }

   // The visibility reference image is used for O(1) picking 
   // (not counting the cost of preparing the image). Only invisible
   // objects should do nothing for this call ... other objects that
   // can't or won't do picking via the visibility reference image
   // should draw in solid black to cover up any objects they occlude:
   virtual int draw_vis_ref() { return 0; }

   // draw ID image:
   virtual int draw_id_ref()            { return 0; }

   // subroutines for drawing the ID image;
   // these are explained in Rob Kalnins thesis:

   // Triangles for see thru objects
   virtual int draw_id_ref_pre1()       { return 0; }

   // Hidden lines with z testing
   virtual int draw_id_ref_pre2()       { return 0; }

   // Triangles for see thru objects
   // (just write the z's)
   virtual int draw_id_ref_pre3()       { return 0; }

   // Visible lines with z testing
   virtual int draw_id_ref_pre4()       { return 0; }

   // halo tone supression reference image
   virtual int draw_halo_ref()          { return 0; }

   // Final pass rendering call, ignored by most, can be used to
   // draw strokes after visibility determination has been made:
   // (The return value doesn't mean anything.)
   virtual int draw_final(CVIEWptr &) { return 0; }
};
typedef const RefImageClient CRefImageClient;

/*****************************************************************
 * RICL:
 *
 *   A RefImageClient that consists of an ARRAY or LIST of
 *   RefImageClient pointers (ref counted for LIST).
 *
 *   Template parameter L should be a list type, e.g.
 *   LIST<GELptr>, or ARRAY<RefImageClient*>...
 *****************************************************************/
template <class L>
class RICL : public L, public RefImageClient {
 public:
   //******** MANAGERS ********

   RICL(int n=0)    : L(n) {}
   RICL(const L& l) : L(l) {}

   //******** RefImageClient VIRTUAL METHODS ********

   virtual void request_ref_imgs() {
      for (int i=0; i<num(); i++)
         (*this)[i]->request_ref_imgs();
   }
   virtual int draw(CVIEWptr& v) {
      int ret = 0;
      for (int i=0; i<num(); i++)
         ret += (*this)[i]->draw(v);
      return ret;
   }
   virtual int draw_color_ref(int n) {
      int ret = 0;
      for (int i=0; i<num(); i++)
         ret += (*this)[i]->draw_color_ref(n);
      return ret;
   }
   virtual int draw_vis_ref() {
      int ret = 0;
      for (int i=0; i<num(); i++)
         ret += (*this)[i]->draw_vis_ref();
      return ret;
   }
   virtual int draw_id_ref() {
      int ret = 0;
      for (int i=0; i<num(); i++)
         ret += (*this)[i]->draw_id_ref();
      return ret;
   }
   virtual int draw_id_ref_pre1() {
      int ret = 0;
      for (int i=0; i<num(); i++)
         ret += (*this)[i]->draw_id_ref_pre1();
      return ret;
   }
   virtual int draw_id_ref_pre2() {
      int ret = 0;
      for (int i=0; i<num(); i++)
         ret += (*this)[i]->draw_id_ref_pre2();
      return ret;
   }
   virtual int draw_id_ref_pre3() {
      int ret = 0;
      for (int i=0; i<num(); i++)
         ret += (*this)[i]->draw_id_ref_pre3();
      return ret;
   }
   virtual int draw_id_ref_pre4() {
      int ret = 0;
      for (int i=0; i<num(); i++)
         ret += (*this)[i]->draw_id_ref_pre4();
      return ret;
   }

  virtual int draw_halo_ref() {
      int ret = 0;
      for (int i=0; i<num(); i++)
         ret += (*this)[i]->draw_halo_ref();
      return ret;
   }

   virtual int draw_final(CVIEWptr &v) {
      int ret = 0;
      for (int i=0; i<num(); i++)
         ret += (*this)[i]->draw_final(v);
      return ret;
   }

   using L::num;
};

// ARRAY-based version (pointers not ref-counted):
template <class T>
class RIC_array: public RICL<ARRAY<T*> > {
 public:
   RIC_array(int n=0)                   : RICL<ARRAY<T*> >(n) {}
   RIC_array(const RICL<ARRAY<T*> >& l) : RICL<ARRAY<T*> >(l) {}
};

// LIST-based version (pointers are ref-counted):
template <class T>
class RIC_list: public RICL<LIST<T> > {
 public:
   RIC_list(int n=0)                 : RICL<LIST<T> >(n) {}
   RIC_list(const RICL<LIST<T> >& l) : RICL<LIST<T> >(l) {}
};

// Convenience class: RefImageClient_list is the same as:
//   RIC_array<RefImageClient>, or
//   RICL<ARRAY<RefImageClient*> >
typedef RIC_array<RefImageClient> RefImageClient_list;

// Print names of items in a list (type L); assuming list
// items support the class_name() method:
template <class L>
inline void print_names(std::ostream& out, const L& list)
{
   for (int i=0; i<list.num(); i++)
      out << "  " << list[i]->class_name() << std::endl;
}

#endif // REF_IMG_CLIENT_IS_INCLUDED

// end of file ref_img_client.H

