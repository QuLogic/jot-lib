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
#ifndef _HATCHING_GROUP_BASE_H_IS_INCLUDED_
#define _HATCHING_GROUP_BASE_H_IS_INCLUDED_

////////////////////////////////////////////
// HatchingGroupBase, ...LevelBase, ...HatchBase
////////////////////////////////////////////
//
// -Provides various methods useful to all
//  hatching group types
// -Generally a private parent class
// -Subclassed by fixed hatch groups and free
//  hatch group instances
// -HatchingLevel and HatchingHatch also here...
// -A hatch group is a set of Levels (LOD)
// -A level holds a set of Hatches
//
////////////////////////////////////////////

#include "mesh/simplex_filter.H"
#include "npr/hatching_group.H"
#include "stroke/hatching_stroke.H"
#include "disp/view.H"
#include "std/stop_watch.H"

class HatchingLevelBase;
class HatchingHatchBase;
class HatchingSelectBase;
class HatchingBackboneBase;

#define CHatchingLevelBase const HatchingLevelBase

/*****************************************************************
 * HatchingGroupBase
 *****************************************************************/

class HatchingGroupBase { 
 protected:
   /******** MEMBER VARIABLES ********/
        
   // Needed to access the HatchGroup:
   // Pointer to the fixed or free group
   // (if free group instance then this
   // points to the controlling group,
   // or if fixed then points to self)

   HatchingGroup *              _group;
   HatchingSelectBase *         _select;
   ARRAY<HatchingLevelBase*>    _level;
   bool                         _selected;

   //Stored by the child class (fixed/free)
   //Asserted to exist once group is 'complete'
   //Destroyed by child class
   //Can return ratio of original to current size
   HatchingBackboneBase *       _backbone;

   //LOD tracking
   double                       _old_desired_level;

   //Visibility speed up (cache the camera
   //position in model space coords)
   mlib::Wpt                          _cam;

   //Number of user defined levels (sloppy)
   //or derived level of reduced LOD (neat)
   //There can be more than this due to
   //LOD interpolation (neat), but we don't
   //want to save those to disk...
   int                          _num_base_levels;

 public:
   /******** CONSTRUCTOR/DESTRUCTOR ********/
   HatchingGroupBase(HatchingGroup *hg);
   virtual ~HatchingGroupBase();

   /******** MEMBER METHODS ********/
        
   inline HatchingGroup*         group()           const { return _group; }

   inline Patch*                 patch()           const { return _group->patch(); }
   inline bool                   is_complete()     const { return _group->is_complete(); }

   inline bool                   selected()        const { return _selected; }
   inline HatchingSelectBase *   selection()             { return _select; }

   //Camera position in model space (for vis).
   inline mlib::CWpt&                  cam()          const { return _cam; }

   //Queries if a pt falls on this group
   virtual bool query_visibility(mlib::CNDCpt &pt,
                                 CHatchingVertexData *hsvd) = 0;

   //Drawing methods
   virtual void    draw_setup();
   virtual void    level_draw_setup();
   virtual void    hatch_draw_setup();
   virtual int     draw(CVIEWptr &v);
   virtual int     draw_select(CVIEWptr &v);

   //Level management
   virtual void               level_sort(int (*cmp)(const void*, const void*)) 
                                 { assert(_level.num() == _num_base_levels); _level.sort(cmp); }

   virtual int                num_base_levels() const
                                 { return _num_base_levels; }

   HatchingLevelBase*         base_level(int i) 
                                 { assert(i<_num_base_levels); return _level[i];}

   CHatchingLevelBase*        base_level(int i) const
                                 { assert(i<_num_base_levels); return _level[i];}

   virtual void               add_base_level();

   virtual HatchingLevelBase* pop_base_level();
   virtual void               trash_upper_levels();

 protected:

   virtual void    clear();
   virtual void    init();

   virtual void    update_levels();
   virtual void    update_prototype();

   void generate_interpolated_level(int lev);

   //Level interpolating methods
   virtual HatchingHatchBase* interpolate(int lev,
                                          HatchingLevelBase *hlb, 
                                          HatchingHatchBase *hhb1,
                                          HatchingHatchBase *hhb2) = 0;

 public:

   /******** STATIC MEMBER METHODS ********/

   static void     compute_hatch_indices(int &level,int &index, int i, int depth);

   //Ref. image convenience methods
   static Bface *  find_face_vis(mlib::CNDCpt& pt, mlib::Wpt &p);
   static Bface *  find_face_id(mlib::CNDCpt& pt);
   static bool     query_filter_id(mlib::CNDCpt& pt, CSimplexFilter& filt);



   // Gesture processing
   static bool   smooth_gesture(mlib::CNDCpt_list &pts, mlib::NDCpt_list &spts,
                                const ARRAY<double>&prl, ARRAY<double>&sprl, int style);
   static bool   is_gesture_silly(mlib::CNDCpt_list &pts, int style);
   static bool   fit_line(mlib::CNDCpt_list &pts, double &a, double &b );
   static Bface* compute_cutting_plane(Patch *pat, double a, double b, 
                                        mlib::CNDCpt_list &pts, mlib::Wplane &p);
   static void   clip_curve_to_stroke(Patch *pat, mlib::CNDCpt_list &gestpts, 
                                    mlib::CWpt_list &allpts, mlib::Wpt_list &clippts);


   /******** HatchingGroup METHODS ********/

   // Not true for Free groups, we call to these 
   // on their instances

   virtual void    select();
   virtual void    deselect();
   virtual void    kill_animation();

};

/*****************************************************************
 * HatchingLevelBase
 *****************************************************************/


class HatchingLevelBase : protected ARRAY<HatchingHatchBase*>, public DATA_ITEM { 
public:
   /******** MEMBER TYPES ********/
   enum ext_t {         // Extinction types        
      EXT_NONE = 0,     // No extinction in progress
      EXT_GROW,         // Growing from nothing
      EXT_DIE           // Fading to nothing
   };

 private:

   /******** STATIC MEMBER VARIABLES ********/
   static TAGlist       *_hlb_tags;

 protected:
   /******** MEMBER VARIABLES ********/
   HatchingGroupBase               *_group;

   //LOD variables
        
   double         _watch;

   double         _trans_time;            //In sec (0 if not timing)
   double         _trans_begin_frac;      //Fractional multipliers
   double         _current_frac;          //to actual pixel width
   double         _desired_frac;
   double         _current_width;         //Actual pixel width
   double         _current_alpha_frac;

   double         _pix_size;              //Mesh size for this level

   ext_t          _extinction;            
 public:
   /******** CONSTRUCTOR/DESTRUCTOR ********/

   HatchingLevelBase(HatchingGroupBase *hgb = NULL);
   ~HatchingLevelBase();

   /******** MEMBER METHODS ********/

   virtual void         draw_setup();
   virtual void         hatch_draw_setup();
   virtual int          draw(CVIEWptr &v);

   virtual void         update_prototype();

   void                 add_hatch(HatchingHatchBase *hhb);

   //LOD animation stuff

   inline double        current_width()               { return _current_width; }
   inline double        current_alpha_frac()          { return _current_alpha_frac; }
   inline bool          non_zero()                    { return _desired_frac > 0.0; }
   inline bool          in_trans()                    { return _trans_time > 0.0; }
   inline void          set_desired_frac(double w)    { _desired_frac = w; }
   inline void          abort_transition()            { _trans_time = 0.0; }
   void                 start_transition(double t);

   ext_t                ext()                         { return _extinction; }
   void                 set_ext(ext_t x)              { _extinction = x; }
   void                 clear_ext()                   { _extinction = EXT_NONE; }

 protected:
        
   void                 update_transition();

 public:

    inline double                pix_size()                       { return _pix_size; }
    void                         set_pix_size(double s)           { _pix_size = s; }

    inline HatchingGroupBase*    group()                          { return _group; }
    void                         set_group(HatchingGroupBase* g)  { _group = g; }

   /******** EXPOSED ARRAY<> METHODS ********/

   ARRAY<HatchingHatchBase*>::num;
   ARRAY<HatchingHatchBase*>::pop;
   ARRAY<HatchingHatchBase*>::operator[];

   /******** DATA_ITEM METHODS ********/
 public:
   DEFINE_RTTI_METHODS_BASE("HatchingLevelBase", CDATA_ITEM *);

   virtual DATA_ITEM    *dup() const { return new HatchingLevelBase; }
   virtual CTAGlist     &tags() const;

   /******** I/O Access Methods ********/
 protected:
   double&              pix_size_()           { return _pix_size;}

   /******** I/O Methods ********/
   virtual void         get_hatch (TAGformat &d);
   virtual void         put_hatchs (TAGformat &d) const;

};

/*****************************************************************
 * HatchingHatchBase
 *****************************************************************/

#define CHatchingHatchBase const HatchingHatchBase
class HatchingHatchBase : public DATA_ITEM { 
 private:

   /******** STATIC MEMBER VARIABLES ********/
   static TAGlist       *_hhb_tags;

 protected:
   /******** MEMBER VARIABLES ********/
   HatchingLevelBase *  _level;
   HatchingStroke *     _stroke;
        
   // Basic hatch parameters
   mlib::Wpt_list             _pts;
   ARRAY<mlib::Wvec>          _norms;
   double               _pix_size;

   BaseStrokeOffsetLISTptr    _offsets;

   mlib::Wpt_list             _real_pts;
   ARRAY<mlib::Wvec>          _real_norms;
   double               _real_pix_size;
   ARRAY<bool>          _real_good;

   bool                 _visible;

 public:
   /******** CONSTRUCTOR/DESTRUCTOR ********/

   HatchingHatchBase(HatchingLevelBase *hlb, double len, 
                     mlib::CWpt_list &pl,  const ARRAY<mlib::Wvec> &nl,
                     CBaseStrokeOffsetLISTptr &ol); 
   HatchingHatchBase(HatchingLevelBase *hlb=0) { _level = hlb; } 
   ~HatchingHatchBase();

   /******** MEMBER METHODS ********/

   virtual void         draw_setup();
   virtual int          draw(CVIEWptr &v);

   virtual void         update_prototype();

   mlib::CWpt_list &          get_pts()               { return _pts;          }
   const ARRAY<mlib::Wvec>&   get_norms()             { return _norms;        }
   double               get_pix_size()          { return _pix_size;   }

   const BaseStrokeOffsetLISTptr& get_offsets() { return _offsets;      }

   virtual void         set_level(HatchingLevelBase *hlb)   { _level = hlb; }
 protected:
   //Called by 'full' constructor or at
   //end of decode to finish construction
   //Subclasses should re-override decode
   //to ensure any additional inits are done
   virtual void         init();

   virtual void         stroke_real_setup();
   virtual void         stroke_pts_setup();

   /******** DATA_ITEM METHODS ********/
 public:
   DEFINE_RTTI_METHODS_BASE("HatchingHatchBase", CDATA_ITEM*);
   virtual DATA_ITEM    *dup() const { return new HatchingHatchBase; }
   virtual CTAGlist     &tags() const;
   //Subclass this and have it append init()
   virtual STDdstream   &decode(STDdstream &ds);

   /******** I/O Access Methods ********/
 protected:
   mlib::Wpt_list&            pts_()                  { return _pts;       }
   ARRAY<mlib::Wvec>          norms_()                { return _norms;     }
   double&              pix_size_()             { return _pix_size;}

   /******** I/O Methods ********/
   virtual void         get_pts (TAGformat &d);
   virtual void         put_pts (TAGformat &d) const;

   virtual void         get_norms (TAGformat &d);
   virtual void         put_norms (TAGformat &d) const;

   virtual void         get_offsetlist (TAGformat &d);
   virtual void         put_offsetlist (TAGformat &d) const;

   //Legacy
   virtual void			get_pressures (TAGformat &d);
   virtual void			put_pressures (TAGformat &d) const;

   virtual void			get_offsets (TAGformat &d);
   virtual void			put_offsets (TAGformat &d) const;

};
////////////////////////////////////////////
// HatchingSelectBase
////////////////////////////////////////////
//
// -Base class for object to draw when a
//  hatching group base class object is drawn selected
// -If active, if is clear before all drawing, and
//  update is called for each stroke vertex computed
// -After all drawing, the selection is drawn as a 
//  bounding box for the given points
//
////////////////////////////////////////////

/*****************************************************************
 * HatchingSelectBase
 *****************************************************************/

class HatchingSelectBase { 

 protected:

   /******** MEMBER VARIABLES ********/

   HatchingGroupBase* _group;

   BaseStroke*       _left_stroke;
   BaseStroke*       _right_stroke;
   BaseStroke*       _top_stroke;
   BaseStroke*       _bottom_stroke;

   mlib::NDCZpt            _left;
   mlib::NDCZpt            _right;
   mlib::NDCZpt            _top;
   mlib::NDCZpt            _bottom;

   bool              _cleared;

 public:
   /******** CONSTRUCTOR/DESTRUCTOR ********/

   HatchingSelectBase(HatchingGroupBase *hgb); 
   ~HatchingSelectBase();

   /******** MEMBER METHODS ********/

   void              clear();
   void              update(mlib::CNDCZpt &pt);

   int               draw(CVIEWptr &v);


};

////////////////////////////////////////////
// HatchingBackboneBase
////////////////////////////////////////////
//
// -Sub-classed and instantiated into _backbone
//  by sub-classes of HatchingGroupBase
// -If assume to exist when the group is 'complete'
// -Returns the ratio of the original screen size
//  to the current screen size
//
////////////////////////////////////////////

/*****************************************************************
 * HatchingBackboneBase
 *****************************************************************/

class Vertebrae {
 public:
   mlib::Wpt           pt1;      //World pt. pair
   mlib::Wpt           pt2;
   bool          exist;    //Flagged if it exists 
                           //(might fail uv conversion in free group)
   double        len;      //Pixel length of pair

   Vertebrae() : exist(true) {}
};

#define CHatchingBackboneBase const HatchingBackboneBase
class HatchingBackboneBase : public DATA_ITEM { 

 private:
   /******** STATIC MEMBER VARIABLES ********/
   static TAGlist  *    _hbb_tags;

 protected:
   /******** MEMBER VARIABLES ********/
   double               _len;           // Total length
   ARRAY<Vertebrae*>    _vertebrae;
   Patch *              _patch;
   bool                 _use_exist;             
   GELlist              _geoms;         // for debugging

 public:
   /******** CONSTRUCTOR/DECONSTRUCTOR ********/
   HatchingBackboneBase(Patch *p=0) : _len(0), _patch(p), _use_exist(true) {}
   virtual ~HatchingBackboneBase();

   /******** MEMBER METHODS ********/
   //Returns screen size ratio
   virtual double       get_ratio();

 protected:
   virtual double       find_len();

 public:
   /******** DATA_ITEM METHODS ********/
   DEFINE_RTTI_METHODS2("HatchingBackboneBase", DATA_ITEM, CDATA_ITEM*);

   virtual DATA_ITEM* dup()  const    { return new HatchingBackboneBase; }
   virtual CTAGlist&  tags() const;

   /******** I/O Methods ********/
   virtual void         get_num (TAGformat &d);
   virtual void         put_num (TAGformat &d) const;
   
   virtual void         get_wpts1 (TAGformat &d);
   virtual void         put_wpts1 (TAGformat &d) const;

   virtual void         get_wpts2 (TAGformat &d);
   virtual void         put_wpts2 (TAGformat &d) const;

   virtual void         get_lengths (TAGformat &d);
   virtual void         put_lengths (TAGformat &d) const;

   /******** I/O Access Methods ********/
 protected:
   double&              len_() { return _len; }

};

#endif
