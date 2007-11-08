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
#ifndef _HATCHING_GROUP_FREE_H_IS_INCLUDED_
#define _HATCHING_GROUP_FREE_H_IS_INCLUDED_

////////////////////////////////////////////
// HatchingGroupFree
////////////////////////////////////////////
//
// -'Free' Hatching Group class
// -Implements HatchingGroup abstract class
// -Holds a collection of HatchingGroupFreeInst's
//  (also defined here) which subclass HatchingGroupBase
//
////////////////////////////////////////////

#include "npr/hatching_group.H"
#include "npr/hatching_group_base.H"
#include "mesh/uv_mapping.H"

class HatchingBackboneFree;
class HatchingGroupFreeInst;
class HatchingPositionFree;
class HatchingPositionFreeInst;

/*****************************************************************
 * HatchingGroupFree
 *****************************************************************/

class HatchingGroupFree : 
   public HatchingGroup {

 private:
   /******** STATIC MEMBER VARIABLES ********/
   static TAGlist          *_hgfr_tags;

 protected:
   /******** MEMBER VARIABLES ********/
        
   UVMapping*                       _mapping;
   ARRAY<HatchingGroupFreeInst*>    _instances;
   bool                             _selected;
   HatchingPositionFree*            _position;

 public:
   /******** CONSTRUCTOR/DECONSTRUCTOR *******/

   HatchingGroupFree(Patch *p=0);
   ~HatchingGroupFree();

   /******** MEMBER METHODS ********/
 protected:

   UVMapping*     find_uv_mapping(mlib::CNDCpt_list &pl);
   void           slice_mesh_with_plane(Bface *f, mlib::CWplane &wpPlane, mlib::Wpt_list &wlList);
   void           clip_to_uv_region(mlib::CNDCpt_list &pts, mlib::NDCpt_list &cpts, 
                                          CARRAY<double>&prl, ARRAY<double>&cprl );
   void           draw_setup();

 public:

   inline UVMapping*                      mapping()         { return _mapping; }
   inline HatchingPositionFree*           position()        { return _position; }
   inline ARRAY<HatchingGroupFreeInst*>&  instances()       { return _instances; }
   inline HatchingGroupFreeInst*          base_instance()   { return _instances[0]; }

   /******** HATCHING GROUP VIRTUAL METHODS ********/
 public:
   virtual void   update_prototype();

   virtual int    type()  {       return FREE_HATCHING_GROUP; }

   virtual void   select();
   virtual void   deselect();

   virtual bool   complete();

   virtual bool   undo_last();

   virtual int    draw(CVIEWptr &v);

   virtual bool   query_pick(mlib::CNDCpt &pt);

   virtual bool   add(mlib::CNDCpt_list &pl, const ARRAY<double>&prl,int);

   virtual void   kill_animation();

   /******** DATA_ITEM METHODS ********/
   DEFINE_RTTI_METHODS2("HatchingGroupFree", HatchingGroup, CHatchingGroup*);
   virtual DATA_ITEM            *dup() const { return new HatchingGroupFree; }
   virtual CTAGlist             &tags() const;
   //Subclass this and have it set _complete
   virtual STDdstream      &decode(STDdstream &ds);

   /******** I/O functions ********/
   //Legacy
   virtual void   get_hatch (TAGformat &d);
   virtual void   put_hatchs (TAGformat &d) const;

   virtual void   get_level (TAGformat &d);
   virtual void   put_levels (TAGformat &d) const;

   virtual void   get_mapping (TAGformat &d);
   virtual void   put_mapping (TAGformat &d) const;

   virtual void   get_backbone (TAGformat &d);
   virtual void   put_backbone (TAGformat &d) const;

   virtual void   get_position (TAGformat &d);
   virtual void   put_position (TAGformat &d) const;
        
};

////////////////////////////////////////////
// HatchingGroupFreeInst
////////////////////////////////////////////
//
// -'Free' Hatching Group Instance
// -Multiple instances held by Free Hatching Groups
// -Similar to a fixed hatching group except that 
//  support for uv and scaling
// -Derived from HatchingGroupBase which provides
//  common hatching functionality
//
////////////////////////////////////////////

/*****************************************************************
 * HatchingGroupFreeInst
 *****************************************************************/

class HatchingGroupFreeInst : 
   public HatchingGroupBase {

 protected:
   /******** MEMBERS VARIABLES ********/

   HatchingPositionFreeInst*  _position;

 public:
   /******** CONSTRUCTOR/DECONSTRUCTOR *******/

   HatchingGroupFreeInst(HatchingGroup *hg);
   ~HatchingGroupFreeInst();

   /******** MEMBERS METHODS ********/

   virtual bool               query_pick(mlib::CNDCpt &pt);

   HatchingPositionFreeInst*  position() { return _position; }
   void                       set_position(HatchingPositionFreeInst* hpfri) { _position = hpfri; }

   HatchingBackboneBase*      backbone() { return _backbone; }
   void                       set_backbone(HatchingBackboneBase* hbfr) { _backbone = hbfr; }

   virtual void               update_prototype();

   HatchingGroupFreeInst*     clone();
        
   /******** HatchingGroupBase METHODS *******/

   virtual bool               query_visibility(mlib::CNDCpt &pt,CHatchingVertexData *hsvd);

   //Drawing methods
   //Override mainly to opt out when we're
   //an instance that's not being rendered
   virtual void               draw_setup();
   virtual void               level_draw_setup();
   virtual void               hatch_draw_setup();
   virtual int                draw(CVIEWptr &v);
   virtual int                draw_select(CVIEWptr &v);

   //Level interpolating methods
   virtual HatchingHatchBase* interpolate(int lev, HatchingLevelBase *hlb, 
                                                HatchingHatchBase *hhb1, HatchingHatchBase *hhb2);

};

/*****************************************************************
 * HatchingHatchFree
 *****************************************************************/

class HatchingHatchFree : 
   public HatchingHatchBase {

 private:
   /******** STATIC MEMBER VARIABLES ********/
   static TAGlist             *_hhfr_tags; 

 protected:
   /******** MEMBER VARIABLES ********/

   mlib::UVpt_list                  _uvs;

   mlib::UVpt_list                  _real_uvs;

 public:
   /******** CONSTRUCTOR/DECONSTRUCTOR *******/
   HatchingHatchFree(HatchingLevelBase *hlb, double len, 
                     mlib::CUVpt_list &uvl, mlib::CWpt_list &pl, const ARRAY<mlib::Wvec> &nl,
                     CBaseStrokeOffsetLISTptr &ol);
                                        
   HatchingHatchFree(HatchingLevelBase *hlb, HatchingHatchFree *hhf);

   HatchingHatchFree(HatchingLevelBase *hlb=0) :
      HatchingHatchBase(hlb) {}

   mlib::CUVpt_list&                get_uvs()               { return _uvs; }

   /******** HatchingHatchBase METHODS ********/
   virtual void               init();
   virtual void               stroke_real_setup();
   virtual void               stroke_pts_setup();
   virtual void               draw_setup();
        
   /******** DATA_ITEM METHODS ********/
   DEFINE_RTTI_METHODS2("HatchingHatchFree", HatchingHatchBase, CDATA_ITEM*);
   virtual DATA_ITEM          *dup() const { return new HatchingHatchFree; }
   virtual CTAGlist           &tags() const;

 protected:
   /******** I/O Access METHODS ********/
   mlib::UVpt_list&                 uvs_()                  { return _uvs; }

};

/*****************************************************************
 * HatchingBackboneFree
 *****************************************************************/

class VertebraeFree : public Vertebrae {
 public:
   mlib::UVpt                    uvpt1;          //uv pt. pair
   mlib::UVpt                    uvpt2;

   VertebraeFree() {}
};

class HatchingBackboneFree : public HatchingBackboneBase { 

 private:
   /******** STATIC MEMBER VARIABLES ********/  
   static TAGlist          *_hbfr_tags;

 protected:
   /******** MEMBER VARIABLES ********/
   HatchingGroupFreeInst*  _inst;

 public:
   /******** CONSTRUCTOR/DESTRUCTOR ********/

   HatchingBackboneFree(Patch *p,HatchingGroupFreeInst* i, HatchingBackboneFree *b);
   HatchingBackboneFree(Patch *p=0,HatchingGroupFreeInst* i=0);
   ~HatchingBackboneFree();

   /******** HatchingBackboneBase METHODS ********/

   //Returns screen size
   virtual double          get_ratio();

   /******** MEMBER METHODS ********/
   //Compute from scratch
   bool                    compute(HatchingLevelBase *hlb);

 public:
   /******** DATA_ITEM METHODS ********/
   DEFINE_RTTI_METHODS2("HatchingBackboneFree", HatchingBackboneFree, CDATA_ITEM*);
   virtual DATA_ITEM*      dup()  const    { return new HatchingBackboneFree; }
   virtual CTAGlist&       tags() const;

   /******** I/O Methods ********/ 
   //Subclassed from base to create 
   //proper Vertebrae type
   virtual void            get_num (TAGformat &d);

   virtual void            get_uvpts1 (TAGformat &d);
   virtual void            put_uvpts1 (TAGformat &d) const; 

   virtual void            get_uvpts2 (TAGformat &d);
   virtual void            put_uvpts2 (TAGformat &d) const;
        
};

/*****************************************************************
 * HatchingPositionFree
 *****************************************************************/

class HatchingPositionFree : 
   public DATA_ITEM  {

   /******** MEMBER CLASSES ********/

   class HatchingPlacement {
    public:
      int         maxk;
      double      maxu;
      double      leftu;
      double      rightu;
      double      maxdot;
      bool        good;

      int operator==(const HatchingPlacement &) 
         { cerr << "Dummy HatchingPlacement::operator== called\n"; return 0; }

   };

 private:
   /******** STATIC MEMBER VARIABLES ********/
   static TAGlist  *          _hpfr_tags;

 protected:
   HatchingGroupFree*         _group;
   UVMapping*                 _mapping;

   mlib::UVpt                       _lower_left;
   mlib::UVpt                       _upper_right;

   mlib::UVpt                       _center;
   mlib::Wvec                       _direction;
   double                     _left_dot;
   double                     _right_dot;

   mlib::Wvec                       _curr_direction;
   ARRAY<mlib::Wvec>                _search_normals;
   ARRAY<double>              _search_dots;

   CRSpline                   _normal_spline;
        
   ARRAY<HatchingPlacement>*  _curr_placements;
   ARRAY<HatchingPlacement>*  _old_placements;


   //XXXX - Keep the unsmoothed ones for now (to visualize)
   ARRAY<mlib::Wvec>                _old_search_normals;

   GEOMlist                   _geoms;
 public:
   /******** CONSTRUCTOR/DESTRUCTOR ********/

   HatchingPositionFree(HatchingGroupFree *hg=0, UVMapping *m=0);
   ~HatchingPositionFree();

   /******** MEMBER METHODS ********/
   bool                       compute(HatchingGroupFreeInst *hgfri);
   bool                       compute_bounds(HatchingGroupFreeInst *hgfri);
   bool                       compute_location();

   mlib::CUVpt&                     lower_left()    { return _lower_left; }
   mlib::CUVpt&                     upper_right()   { return _upper_right; }

   mlib::CUVpt&                     center()        { return _center; }

   HatchingGroupFree*         group()         { return _group; }

   void                       update();

 protected:

   double                     refine(double mid, double  left, double  right,
                                             double  offset, double  tol, mlib::CWvec& dir);

   void                       cache_search_normals();
   void                       smooth_search_normals();
   void                       generate_normal_spline();
   bool                       query_normal_spline(double u, mlib::Wvec &n);

   void                       cache_search_dots();
   bool                       refine_placement_maximum(int k, double& max, double& maxdot);
   bool                       refine_placement_extent(double absmaxu, double maxu, 
                                                      double maxdot, double& leftu, double& rightu);

 public:
   /******** DATA_ITEM METHODS ********/
   DEFINE_RTTI_METHODS2("HatchingPositionFree", HatchingPositionFree, CDATA_ITEM*);
   virtual DATA_ITEM          *dup() const    { return new HatchingPositionFree; }
   virtual CTAGlist           &tags() const;

   /******** I/O Methods ********/
   mlib::UVpt&                      lower_left_()  { return _lower_left; }
   mlib::UVpt&                      upper_right_() { return _upper_right; }
   mlib::UVpt&                      center_()      { return _center; }
   mlib::Wvec&                      direction_()   { return _direction; }
   double&                    left_dot_()    { return _left_dot; }
   double&                    right_dot_()   { return _right_dot; }

};

/*****************************************************************
 * HatchingPositionFreeInst
 *****************************************************************/

class HatchingPositionFreeInst {

   /******** MEMBER VARIABLES ********/
 protected:
   UVMapping*                 _mapping;
   HatchingPositionFree*      _position;

   mlib::UVpt                       _curr_lower_left;
   mlib::UVpt                       _curr_upper_right;
   
   double                     _curr_weight;

 public:
   mlib::UVpt                       _scale;
   mlib::UVvec                      _shift;

   double                     _weight;

 public:
   /******** CONSTRUCTOR/DESTRUCTOR ********/

   HatchingPositionFreeInst(UVMapping *m, HatchingPositionFree* p);
   ~HatchingPositionFreeInst();

   /******** MEMBER METHODS ********/

   void                       update();
   bool                       query(mlib::CUVpt &pt);
   bool                       query(mlib::CUVpt &pt,mlib::CUVpt &dpt);
   void                       transform(mlib::CUVpt& pt, mlib::UVpt& transpt);
   double                     curr_weight() { return _curr_weight; }

   HatchingPositionFree*      position() { return _position; }
};


#endif
