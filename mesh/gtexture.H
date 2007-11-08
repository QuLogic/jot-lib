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
 * gtexture.H
 **********************************************************************/
#ifndef GTEXTURE_H_IS_INCLUDED
#define GTEXTURE_H_IS_INCLUDED

//#include "patch.H"
#include "disp/ref_img_client.H"
#include "disp/view.H"
#include "stripcb.H"

class Patch;
class GTexture;
typedef const GTexture CGTexture;
/*!
 *  \brief Convenience methods defined in gtexture.H
 *
 */
class GTexture_list : public RIC_array<GTexture> {
   
 public:
      
   //******** MANAGERS ********
   GTexture_list(int n=0)                   : RIC_array<GTexture>(n)    {}
   GTexture_list(const GTexture_list& list) : RIC_array<GTexture>(list) {}
   GTexture_list(GTexture* g) : RIC_array<GTexture>(1) {
      add(g);
   }
   GTexture_list(GTexture* g1, GTexture* g2) : RIC_array<GTexture>(2) {
      add(g1);
      add(g2);
   }
   GTexture_list(GTexture* g1, GTexture* g2, GTexture* g3) :
      RIC_array<GTexture>(3) {
      add(g1);
      add(g2);
      add(g3);
   }
   GTexture_list(GTexture* g1, GTexture* g2, GTexture* g3, GTexture* g4) :
      RIC_array<GTexture>(4) {
      add(g1);
      add(g2);
      add(g3);
      add(g4);
   }
   GTexture_list(GTexture* g1, GTexture* g2, GTexture* g3, GTexture* g4,
                 GTexture* g5) :
      RIC_array<GTexture>(5) {
      add(g1);
      add(g2);
      add(g3);
      add(g4);
      add(g5);
   }
   GTexture_list(GTexture* g1, GTexture* g2, GTexture* g3, GTexture* g4,
                 GTexture* g5, GTexture* g6) :
      RIC_array<GTexture>(6) {
      add(g1);
      add(g2);
      add(g3);
      add(g4);
      add(g5);
      add(g6);
   }
   GTexture_list(GTexture* g1, GTexture* g2, GTexture* g3, GTexture* g4,
                 GTexture* g5, GTexture* g6, GTexture* g7) :
      RIC_array<GTexture>(7) {
      add(g1);
      add(g2);
      add(g3);
      add(g4);
      add(g5);
      add(g6);
      add(g7);
   }
   GTexture_list(GTexture* g1, GTexture* g2, GTexture* g3, GTexture* g4,
                 GTexture* g5, GTexture* g6, GTexture* g7, GTexture* g8) :
      RIC_array<GTexture>(8) {
      add(g1);
      add(g2);
      add(g3);
      add(g4);
      add(g5);
      add(g6);
      add(g7);
      add(g8);
   }

   //******** CONVENIENCE ********

   // defined below
   void write_stream(ostream& os) const;
   void delete_all();
   void set_patch(Patch* p) const;
   void push_alpha(double a) const;
   void pop_alpha() const;
   void set_alpha(double a) const;

   //******** UTILITIES ********
 public:
   // return an expanded list containing all GTextures in this list, plus
   // all GTextures that are "owned" by any GTextures in this list
   // (and any owned by them, etc.):
   GTexture_list get_all() const {
      GTexture_list ret(num());
      add_slave_textures(ret, *this);
      return ret;
   }
 protected:
   // output everything in the list, plus all their slave GTextures
   // (continuing recursively):
   static void add_slave_textures(
      GTexture_list& ret,       // returned list
      const GTexture_list& list // input list
      );
};
typedef const GTexture_list CGTexture_list;

/**********************************************************************
 * GTexture:
 *
 *      Base class for a procedural texture associated w/ a patch.
 *      Simple examples include procedures that just implement Gouraud
 *      shading or other conventional rendering styles. More inter-
 *      esting examples include "Graftal textures," as described in:
 *
 *        Michael A. Kowalski, Lee Markosian, J. D. Northrup, Lubomir
 *        Bourdev, Ronen Barzel, Loring S. Holden and John
 *        Hughes. Art-Based Rendering of Fur, Grass, and Trees,
 *        Proceedings of SIGGRAPH 99, Computer Graphics Proceedings,
 *        Annual Conference Series, pp. 433-438 (August 1999, Los
 *        Angeles, California). Addison Wesley Longman. Edited by Alyn
 *        Rockwood. ISBN 0-20148-560-5.
 **********************************************************************/
class GTexture : public DATA_ITEM, public RefImageClient {
 public:

   //******** MANAGERS ********

   GTexture(Patch* p = 0, StripCB* cb=0);
   virtual ~GTexture() { delete _cb; }  

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("Textured patch", GTexture*, DATA_ITEM, CDATA_ITEM *);
   STAT_STR_RET type() const { return class_name(); }

   //******** ACCESSORS ********

   Patch*       patch()         const   { return _patch; }
   BMESH*       mesh()          const; // defined in patch.H

   StripCB*     cb()            const   { return _cb; }
   void         set_cb(StripCB* cb)     { delete _cb; _cb = cb; } 

   GTexture*    ctrl()          const   { return _ctrl; }
   void         set_ctrl(GTexture* c)   { _ctrl = c; }

   //******** VIRTUAL METHODS ********

   virtual int  set_color(CCOLOR&)              { return 0; }

   // set the opacity of this GTexture and its slaves:
   virtual void set_alpha(double a) {
      if (_alphas.empty()) {
         push_alpha(a);
      } else {
         _alphas.last() = a;
         gtextures().set_alpha(a);
      }
   }

   // returns the patch alpha * alpha on top of stack:
   double alpha() const;

   virtual void changed()                       {}

   // Optional list of GTextures owned by this GTexture.
   // E.g., HiddenLineTexture owns a SolidColorTexture and
   // a WireframeTexture that do all the actual work.
   virtual GTexture_list gtextures() const { return GTexture_list(); }

   virtual void set_patch(Patch* p);

   //******** FADING STUFF ********
   // XXX - under construction -- documentation pending
   //
   // these methods relate to fading between GTextures when
   // switching:
   //
   // push_alpha() and pop_alpha() exist as an alternative to adding
   // an alpha parameter to the regular draw() method:
   virtual void push_alpha(double a)  {
      _alphas += a;
      gtextures().push_alpha(a);
   }
   virtual void pop_alpha () {
      _alphas.pop();
      gtextures().pop_alpha();
   }

   // The following is used for fading between GTextures when
   // switching. If the newly introduced GTexture knows the
   // outgoing one draws filled triangles of the Patch, then the
   // new one can draw first with alpha = 1, and the outgoing
   // GTexture can draw on top of it using an alpha corresponding
   // to how faded it is. If the outgoing GTexture doesn't draw
   // filled, the new one has to fade itself in.
   virtual bool draws_filled() const  { return true; }

   virtual void draw_with_alpha(double alpha) {
      push_alpha(alpha);
      draw(VIEW::peek());
      pop_alpha();
   }

   // The next 2 methods are given default implementations for
   // convenience. They work for simple GTextures like smooth
   // shading or wireframe. Compound GTextures like hidden
   // line have to override these. I.e., if the implementation
   // of hidden line style draws solid colored triangles with
   // wireframe over them, then in draw_filled_tris() it
   // should just draw the solid colored part, and in
   // draw_non_filled_tris() it should draw just the wireframe
   // part.
   virtual void draw_filled_tris(double alpha) {
      if (draws_filled())
         draw_with_alpha(alpha);
   }
   virtual void draw_non_filled_tris(double alpha) {
      if (!draws_filled())
         draw_with_alpha(alpha);
   }

   //******** I/O FUNCTIONS ********

   virtual int read_stream (istream &, str_list &)              { return 1;}
   virtual int write_data  (ostream &)                  const   { return 0;};
   virtual int write_stream(ostream &os) const {
      os << _begin_tag << class_name() << endl;
      write_data(os);
      os << _end_tag << endl; return 1;
   }

   //******** DATA_ITEM VIRTUAL FUNCTIONS ********

   virtual DATA_ITEM   *dup()        const { return 0; }
   virtual CTAGlist    &tags()       const;

 //*******************************************************
 // PROTECTED
 //*******************************************************
 protected:
   Patch*       _patch; // patch that owns this
   StripCB*     _cb;    // used for rendering tri strips etc.

   // for drawing the texture with a specified amount of
   // transparency thrown in:
   ARRAY<double> _alphas;

   // optional (usually nil) "controlling" texture that this one
   // defers to:
   GTexture*    _ctrl;

   // for I/O:
   static Cstr_ptr      _type_name;
   static Cstr_ptr      _begin_tag;
   static Cstr_ptr      _end_tag;
};

inline void 
GTexture_list::write_stream(ostream& os) const 
{
   for (int k=0; k<_num; k++)
      _array[k]->write_stream(os);
}

inline void 
GTexture_list:: delete_all() 
{
   while (!empty())
      delete pop();
}

inline void 
GTexture_list::set_patch(Patch* p) const 
{
   for (int k=0; k<_num; k++)
      _array[k]->set_patch(p);
}

inline void 
GTexture_list::push_alpha(double a) const 
{
   for (int k=0; k<_num; k++)
      _array[k]->push_alpha(a);
}

inline void 
GTexture_list::pop_alpha() const 
{
   for (int k=0; k<_num; k++)
      _array[k]->pop_alpha();
}

inline void 
GTexture_list::set_alpha(double a) const 
{
   for (int k=0; k<_num; k++)
      _array[k]->set_alpha(a);
}

typedef void (*init_fade_func_t)(GTexture*, GTexture*, double, double);

// Given a list of items, return the items in the
// list that are of type T.
//
// E.g.:
//
//    GTexture_list all_textures;
//
//    GTexture_list wireframe_textures =
//      get_sublist<GTexture_list, WireframeTexture>(all_textures);
//
template <class L, class T>
inline L
get_sublist(const L& list)
{
   L ret(list.num());
   for (int i=0; i<list.num(); i++) {
      if (T::isa(list[i]))
         ret += list[i];
   }
   return ret;
}

#endif // GTEXTURE_H_IS_INCLUDED

// end of file gtexture.H
