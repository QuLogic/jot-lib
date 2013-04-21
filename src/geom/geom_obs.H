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
#ifndef GEOM_OBS_H_HAS_BEEN_INCLUDED
#define GEOM_OBS_H_HAS_BEEN_INCLUDED

#include "geom/geom.H"

//--------------------------------------------
// XFORMobs -
//   An object that can be notified when some
// other object is transformed
//--------------------------------------------
class XFORMobs;
typedef ARRAY<XFORMobs*>     XFORMobs_list;
typedef const XFORMobs_list CXFORMobs_list;
typedef const XFORMobs      CXFORMobs;
class XFORMobs {
 public:
   enum STATE { START = 0, // Start of manipulation
                MIDDLE,    // Middle of manipulation
                END,       // End of manipulation
                PRIMARY,   // top-level calls to set_xform/mult_by
                GRAB,      // Start of spreadsheet drag & drop
                DRAG,      // Spreadsheet drag 
                DROP,      // Spreadsheet drop
                NET,       // xform from the network
                EVERY 
   };
   virtual ~XFORMobs() {}

   // called when a GEOM was transformed:
   virtual void notify_xform(CGEOMptr&, STATE state) = 0;

   // send message that a GEOM was transformed
   static  void notify_xform_obs      (CGEOMptr&, STATE start);
   static  void notify_xform_every_obs(CGEOMptr&);

   // add/remove this observer to list for the given GEL:
   void   xform_obs(CGELptr& g)  { xform_obs_list(g).add_uniquely(this); }
   void unobs_xform(CGEOMptr &g) { xform_obs_list(g).rem(this); }

   // add/remove this observer to list for all GEOMs:
   void   xform_obs() { _all_xf.add_uniquely(this); }
   void unobs_xform() { _all_xf.rem(this); }

   // add/remove this observer to list for every transforms:
   // XXX - todo: figure out what this is for:
   void   every_xform_obs() { _every_xf.add_uniquely(this); }
   void unobs_every_xform() { _every_xf.rem(this); }

 protected :
   static HASH          _hash_xf;       // maps GEL to xform observors 
   static XFORMobs_list _all_xf;
   static XFORMobs_list _every_xf;

   static XFORMobs_list &xform_obs_list(CGELptr &g)  {
      XFORMobs_list *list = (XFORMobs_list*) _hash_xf.find((long)&*g);
      if (!list)
         _hash_xf.add((long)&*g, (void*)(list = new XFORMobs_list));
      return *list;
   }
};


#define CCOLORobs_list const COLORobs_list
#define CCOLORobs      const COLORobs
class COLORobs;
typedef ARRAYptrs<COLORobs*> COLORobs_list;
//--------------------------------------------
// COLORobs -
//   An object that can be notified when some
// other object's color has changed
//--------------------------------------------
class COLORobs {
   static COLORobs_list *_all_col;
   static COLORobs_list *col_list() { if (!_all_col)
         _all_col = new COLORobs_list; return _all_col; }
 public:
   virtual ~COLORobs() {}
   virtual void notify_color    (CGEOMptr &, APPEAR *app) = 0;
   static  void notify_color_obs(CGEOMptr &g, APPEAR *app = 0) {
      for (int i=0; i < col_list()->num(); i++)
         (*_all_col)[i]->notify_color(g,
                                      app ? app
                                      : (APPEAR *) &*g); }

   /* ---  object color observer --- */
   void   color_obs  ()            { col_list()->add_uniquely(this); }
   void   unobs_color()            { col_list()->rem(this); }
};



#define CTRANSPobs_list const TRANSPobs_list
#define CTRANSPobs      const TRANSPobs
class TRANSPobs;
typedef ARRAYptrs<TRANSPobs*> TRANSPobs_list;
//--------------------------------------------
// TRANSPobs -
//   An object that can be notified when some
// other object's transp has changed
//--------------------------------------------
class TRANSPobs {
   static TRANSPobs_list _all_transp;
 public:
   virtual ~TRANSPobs() {}
   virtual void notify_transp    (CGEOMptr &) = 0;
   static  void notify_transp_obs(CGEOMptr &g) {
      for (int i=0; i < _all_transp.num(); i++)
         _all_transp[i]->notify_transp(g); }

   /* ---  object transp observer --- */
   void   transp_obs  ()            { _all_transp.add_uniquely(this); }
   void   unobs_transp()            { _all_transp.rem(this); }
};

#define CTEXTUREobs_list const TEXTUREobs_list
#define CTEXTUREobs      const TEXTUREobs
class TEXTUREobs;
typedef ARRAYptrs<TEXTUREobs*> TEXTUREobs_list;
//--------------------------------------------
// TEXTUREobs -
//   An object that can be notified when some
// other object's texture has changed
//--------------------------------------------
class TEXTUREobs {
   static TEXTUREobs_list *_all_texture;

   static TEXTUREobs_list *texobs_list() { if (!_all_texture)
         _all_texture = new TEXTUREobs_list; return _all_texture; }
 public:
   virtual ~TEXTUREobs() {}
   virtual void notify_texture    (CGEOMptr &) = 0;
   static  void notify_texture_obs(CGEOMptr &g) {
      for (int i=0; i < texobs_list()->num(); i++)
         (*_all_texture)[i]->notify_texture(g); }

   /* ---  object texture observer --- */
   void   texture_obs  ()            { texobs_list()->add_uniquely(this); }
   void   unobs_texture()            { texobs_list()->rem(this); }
};

#define CCSGobs_list const CSGobs_list
#define CCSGobs      const CSGobs
class CSGobs;
typedef ARRAYptrs<CSGobs*> CSGobs_list;
//--------------------------------------------
// CSGobs -
//   An object that can be notified when some
// other object's csg has changed
//--------------------------------------------
class CSGobs {
   static CSGobs_list _all_csg;
 public:
   virtual ~CSGobs() {}
   virtual void notify_csg    (const GEOMlist &l) = 0;
   static  void notify_csg_obs(const GEOMlist &l) {
      for (int i=0; i < _all_csg.num(); i++)
         _all_csg[i]->notify_csg(l); }

   /* ---  object csg observer --- */
   void   csg_obs  ()            { _all_csg.add_uniquely(this); }
   void   unobs_csg()            { _all_csg.rem(this); }
};

#define CGEOMobs_list const GEOMobs_list
#define CGEOMobs      const GEOMobs
class GEOMobs;
typedef ARRAYptrs<GEOMobs*> GEOMobs_list;
//--------------------------------------------
// GEOMobs -
//   An object that can be notified when some
// other object's geom has changed
//--------------------------------------------
class GEOMobs {
   static GEOMobs_list _all_geom;
 public:
   virtual ~GEOMobs() {}
   virtual void notify_geom    (CGEOMptr &l) = 0;
   static  void notify_geom_obs(CGEOMptr &l) {
      for (int i=0; i < _all_geom.num(); i++)
         _all_geom[i]->notify_geom(l); }

   /* ---  object geom observer --- */
   void   geom_obs  ()            { _all_geom.add_uniquely(this); }
   void   unobs_geom()            { _all_geom.rem(this); }
};

#endif // GEOM_OBS_H_HAS_BEEN_INCLUDED

// end of file geom_obs.H
