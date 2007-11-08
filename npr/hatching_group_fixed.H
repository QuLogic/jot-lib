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
#ifndef _HATCHING_GROUP_FIXED_H_IS_INCLUDED_
#define _HATCHING_GROUP_FIXED_H_IS_INCLUDED_

////////////////////////////////////////////
// HatchingGroupFixed
////////////////////////////////////////////
//
// -'Fixed' Hatching Group class
// -Implements HatchingGroup abstract class
// -Subclasses HatchingGroupBase which provides
//  common hatch group functionality
//
////////////////////////////////////////////

#include "npr/hatching_group.H"
#include "npr/hatching_group_base.H"

/*****************************************************************
 * HatchingGroupFixed
 *****************************************************************/
#define CHatchingGroupFixed const HatchingGroupFixed
class HatchingGroupFixed : public HatchingGroup,
                           public HatchingGroupBase 
{

 private:
   /******** STATIC MEMBER VARIABLES ********/
   static TAGlist          *_hgf_tags;

 protected:
   /******** MEMBER VARIABLES ********/


 public:
   /******** CONSTRUCTOR/DECONSTRUCTOR *******/
   HatchingGroupFixed(Patch *p=0);
   ~HatchingGroupFixed();

 protected:
   /******** MEMBERS METHODS ********/
   //Gesture processing
   void            clip_to_patch(mlib::CNDCpt_list &pts, mlib::NDCpt_list &cpts,
                           const ARRAY<double>&prl, ARRAY<double>&cprl );
   void            slice_mesh_with_plane(Bface *f, mlib::CWplane &wpPlane, mlib::Wpt_list &wlList);

   // Visibility
   void            clear_visibility();
   bool            store_visibility(HatchingLevelBase *hlb);
   int             recurse_visibility(Bface *f, mlib::CNDCpt_list &poly);
   void            compute_convex_hull(mlib::CNDCZpt_list &pts, mlib::NDCZpt_list &hull);

 public:
   /******** HatchingGroup METHODS *******/
   virtual void    update_prototype();
         
   virtual int     type()  { return FIXED_HATCHING_GROUP; }

   virtual void    select();

   virtual void    deselect();

   virtual bool    complete();

   virtual bool    undo_last();

   virtual int     draw(CVIEWptr &v);

   virtual bool    query_pick(mlib::CNDCpt &pt);

   virtual bool    add(mlib::CNDCpt_list &pl, const ARRAY<double>&prl, int);

   virtual void    kill_animation();

   /******** BMESHobs METHODS *******/
   //Called by HatchingCollection which is
   //the subscriber of the notifications...
   virtual void    notify_change       (BMESH*, BMESH::change_t);
   virtual void    notify_xform        (BMESH*, mlib::CWtransf&, CMOD&);
   //virtual void  notify_merge        (BMESH*, BMESH*)           {}
   //virtual void  notify_split        (BMESH*, CARRAY<BMESH*>&)  {}
   //virtual void  notify_subdiv_gen   (BMESH*)                   {}
   //virtual void  notify_delete       (BMESH*)                   {}
   //virtual void  notify_sub_delete   (BMESH*)                   {}

   /******** HatchingGroupBase METHODS *******/

   virtual bool    query_visibility(mlib::CNDCpt &pt,CHatchingVertexData *hsvd);

   virtual HatchingHatchBase*      interpolate(int lev, HatchingLevelBase *hlb, 
                                               HatchingHatchBase *hhb1, HatchingHatchBase *hhb2);

   /******** DATA_ITEM METHODS ********/
   DEFINE_RTTI_METHODS2("HatchingGroupFixed", HatchingGroup, CHatchingGroup*);
   virtual DATA_ITEM             *dup() const { return new HatchingGroupFixed; }
   virtual CTAGlist              &tags() const;
   //Subclass this and have it set _complete
   virtual STDdstream            &decode(STDdstream &ds);

   /******** I/O functions ********/
   // Legacy
   virtual void                  get_hatch (TAGformat &d);
   virtual void                  put_hatchs (TAGformat &d) const;
   
   virtual void                  get_level (TAGformat &d);
   virtual void                  put_levels (TAGformat &d) const;

   virtual void                  get_visibility (TAGformat &d);
   virtual void                  put_visibility (TAGformat &d) const;

   virtual void                  get_backbone (TAGformat &d);
   virtual void                  put_backbone (TAGformat &d) const;

};

/*****************************************************************
 * HatchingFixedVertex 
 *****************************************************************/
//(for storing verts as tri index and barycentric coordiates)

#define CHatchingFixedVertex const HatchingFixedVertex
class HatchingFixedVertex
{
public:
   int   ind;
   mlib::Wvec  bar;

   HatchingFixedVertex() : ind(0), bar(mlib::Wvec(0,0,0)) {}
   HatchingFixedVertex(int i, mlib::CWvec &b) : ind(i), bar(b) {}

   int operator==(CHatchingFixedVertex &v) {
      return ((ind == v.ind) && (bar == v.bar));
   }

   CHatchingFixedVertex& operator=(CHatchingFixedVertex &v) {
      ind   = v.ind;
      bar   = v.bar;
      return *this;
   }
};

inline STDdstream &operator<<(STDdstream &ds, const HatchingFixedVertex  &v) 
       { if (ds.ascii()) ds << "{" << v.ind << v.bar <<"}";
         else ds << v.ind << v.bar;
         return ds; }

inline STDdstream &operator>>(STDdstream &ds, HatchingFixedVertex &v) 
       { if (ds.ascii()) { char brace;
              ds >> brace >> v.ind >> v.bar >> brace;}
         else ds >> v.ind >> v.bar;
         return ds;}

/*****************************************************************
 * HatchingHatchFixed
 *****************************************************************/


class HatchingHatchFixed : 
   public HatchingHatchBase, public BMESHobs {

 private:
      /******** STATIC MEMBER VARIABLES ********/
      static TAGlist                  *_hhf_tags;

 protected:
      /******** MEMBER VARIABLES ********/

      ARRAY<HatchingFixedVertex>       _verts;

 public:
      /******** CONSTRUCTOR/DECONSTRUCTOR *******/
      HatchingHatchFixed(HatchingLevelBase *hlb, 
            double len, const ARRAY<HatchingFixedVertex> &vl,
               mlib::CWpt_list &pl, const ARRAY<mlib::Wvec> &nl, 
               CBaseStrokeOffsetLISTptr &ol);

      HatchingHatchFixed(HatchingLevelBase *hlb, 
            double len, mlib::CWpt_list &pl, const ARRAY<mlib::Wvec> &nl, 
               CBaseStrokeOffsetLISTptr &ol) : 
                  HatchingHatchBase(hlb,len,pl,nl,ol) {}

      HatchingHatchFixed(HatchingLevelBase *hlb=0) :
         HatchingHatchBase(hlb) {}

   /******** HatchingHatchBase METHODS ********/
   virtual void               init();
   virtual void               draw_setup();

   /******** BMESHobs METHODS *******/
   //Called by HatchingGroupFixed which is
   //notified by HatchingCollection, the 
   //subscriber of the notifications...
   virtual void    notify_xform        (BMESH*, mlib::CWtransf&, CMOD&);
   virtual void    notify_change       (BMESH*, BMESH::change_t);

      /******** DATA_ITEM METHODS ********/
 public:
   DEFINE_RTTI_METHODS2("HatchingHatchFixed", HatchingHatchBase, CDATA_ITEM *);
   virtual DATA_ITEM* dup()  const { return new HatchingHatchFixed; }
   virtual CTAGlist&  tags() const;

 protected:
   /******** I/O Access METHODS ********/
   virtual void         get_verts (TAGformat &d);
   virtual void         put_verts (TAGformat &d) const;

   
};

/*****************************************************************
 * HatchingSimplexDataFixed
 *****************************************************************/

#define HSDF_ID 666

class HatchingSimplexDataFixed : public SimplexData {
 protected:

   ARRAY<HatchingGroupFixed *>     _hatchgroups;
        
 public:
   /******** CONSTRUCTOR/DECONSTRUCTOR *******/
         
   HatchingSimplexDataFixed(Bsimplex *s):SimplexData(HSDF_ID,s){}
   ~HatchingSimplexDataFixed(){}

   /******** MEMBER METHODS *******/

   bool    exists(HatchingGroupFixed* hgf)         { return _hatchgroups.contains(hgf);}
   bool    add(HatchingGroupFixed *hgf)            { return _hatchgroups.add_uniquely(hgf);}
   bool    remove(HatchingGroupFixed *hgf)         { return _hatchgroups.rem(hgf);}

   void    clear(void)           { _hatchgroups.clear(); }
   int     num(void)             { return _hatchgroups.num();}
 
   //For get_visibility (seems we can't convert CHatchingGroup* to CHatchingGroup*&)
   bool  hack_exists(CHatchingGroupFixed* hgf) 
   { 
      for (int i=0;i<_hatchgroups.num();i++) if (_hatchgroups[i] == hgf) return true;
      return false;
   }

   /******** STATIC MEMBER METHODS *******/

   //Use a static method to look these up
   static HatchingSimplexDataFixed *       find(CBface *f) 
      { return (HatchingSimplexDataFixed *)f->find_data(HSDF_ID);}

};

/*****************************************************************
 * HatchingBackboneFixed
 *****************************************************************/

class HatchingBackboneFixed : 
   public HatchingBackboneBase, public BMESHobs { 

 private:
      /******** STATIC MEMBER VARIABLES ********/
      static TAGlist          *_hbf_tags;

      /******** MEMBER VARIABLES ********/

 public:
      /******** CONSTRUCTOR/DESTRUCTOR ********/

      HatchingBackboneFixed(Patch *p=0);
      ~HatchingBackboneFixed();

      /******** HatchingBackboneBase METHODS ********/

      //Returns screen size
      virtual double          get_ratio();

      /******** MEMBER METHODS ********/
      //Compute from scratch
      bool                    compute(HatchingLevelBase *hlb);

      /******** BMESHobs METHODS *******/
      //Called by HatchingGroupFixed which is
      //notified by HatchingCollection, the 
      //subscriber of the notifications...
      virtual void    notify_xform        (BMESH*, mlib::CWtransf&, CMOD&);
      virtual void    notify_change       (BMESH*, BMESH::change_t);


 public:
      /******** DATA_ITEM METHODS ********/
      DEFINE_RTTI_METHODS2("HatchingBackboneFixed",
                           HatchingBackboneBase,
                           CDATA_ITEM*);
      virtual DATA_ITEM* dup()  const    { return new HatchingBackboneFixed; }
      virtual CTAGlist&  tags() const;
};

#endif
