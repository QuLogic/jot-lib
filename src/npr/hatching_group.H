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
#ifndef HATCHING_GROUP_HEADER
#define HATCHING_GROUP_HEADER

#include "std/time.H"
#include "mesh/patch.H"
#include "mesh/bmesh.H"
#include "stroke/hatching_stroke.H"

////////////////////////////////////////////
// HatchingGroupParams
////////////////////////////////////////////
//
// -Holds various stroke and lod settings
// -Copied to and from one of these held by pen
//
////////////////////////////////////////////


#define DEFAULT_ANIM_LO_THRESH          0.4f
#define DEFAULT_ANIM_HI_THRESH          0.6f    
#define DEFAULT_ANIM_LO_WIDTH           0.7f
#define DEFAULT_ANIM_HI_WIDTH           1.5f
#define DEFAULT_ANIM_GLOBAL_SCALE       0.01f
#define DEFAULT_ANIM_LIMIT_SCALE        0.1f
#define DEFAULT_ANIM_TRANS_TIME         0.5f
#define DEFAULT_ANIM_MAX_LOD            3.0f
#define DEFAULT_ANIM_MAX_LOD            3.0f
#define DEFAULT_ANIM_STYLE              0

#define CHatchingGroupParams const HatchingGroupParams

class HatchingGroupParams : public DATA_ITEM {

   friend class HatchingPenUI;     

 private:
   /******** STATIC MEMBER VARIABLES ********/
   static TAGlist          *_hp_tags;

   /******** MEMBERS VARS ********/

   //Even though we draw with a HatchingStroke
   //we use a BaseStroke to stroke the stroke
   //parameters, and as the interface to the StrokeUI
   BaseStroke     _stroke;

   float          _anim_lo_thresh;
   float          _anim_hi_thresh;
   float          _anim_lo_width;
   float          _anim_hi_width;
   float          _anim_global_scale;
   float          _anim_limit_scale;
   float          _anim_trans_time;
   float          _anim_max_lod;
   int            _anim_style;

 public:

   /******** MEMBERS METHODS ********/

   //Constructor/Destructor

   HatchingGroupParams() : 
      _anim_lo_thresh(DEFAULT_ANIM_LO_THRESH),
      _anim_hi_thresh(DEFAULT_ANIM_HI_THRESH),
      _anim_lo_width(DEFAULT_ANIM_LO_WIDTH),
      _anim_hi_width(DEFAULT_ANIM_HI_WIDTH),
      _anim_global_scale(DEFAULT_ANIM_GLOBAL_SCALE),
      _anim_limit_scale(DEFAULT_ANIM_LIMIT_SCALE),
      _anim_trans_time(DEFAULT_ANIM_TRANS_TIME),
      _anim_max_lod(DEFAULT_ANIM_MAX_LOD),
      _anim_style(DEFAULT_ANIM_STYLE) {}

   ~HatchingGroupParams() {}

   //Access methods

   inline float anim_lo_thresh()    const    { return _anim_lo_thresh;   }
   inline float anim_hi_thresh()    const    { return _anim_hi_thresh;   }
   inline float anim_lo_width()     const    { return _anim_lo_width;    }
   inline float anim_hi_width()     const    { return _anim_hi_width;    }
   inline float anim_global_scale() const    { return _anim_global_scale;}
   inline float anim_limit_scale()  const    { return _anim_limit_scale; }
   inline float anim_trans_time()   const    { return _anim_trans_time;  }
   inline float anim_max_lod()      const    { return _anim_max_lod;     }
   inline int   anim_style()        const    { return _anim_style;       }
   inline CBaseStroke& stroke()     const    { return _stroke;           }

   //Setting methods

   void anim_lo_thresh(float t)     { _anim_lo_thresh = t;   }
   void anim_hi_thresh(float t)     { _anim_hi_thresh = t;   }
   void anim_lo_width(float w)      { _anim_lo_width = w;    }
   void anim_hi_width(float w)      { _anim_hi_width = w;    }
   void anim_global_scale(float s)  { _anim_global_scale = s;}
   void anim_limit_scale(float s)   { _anim_limit_scale = s; }
   void anim_trans_time(float t)    { _anim_trans_time = t;  }
   void anim_max_lod(float l)       { _anim_max_lod = l;     }
   void anim_style(int s)           { _anim_style = s;       }
   void stroke(CBaseStroke &s)      { _stroke.copy(s);       }

   //Duping methods

   void copy(HatchingGroupParams *p)
   {
      _anim_lo_thresh      = p->_anim_lo_thresh;
      _anim_hi_thresh      = p->_anim_hi_thresh;
      _anim_lo_width       = p->_anim_lo_width;
      _anim_hi_width       = p->_anim_hi_width;
      _anim_global_scale   = p->_anim_global_scale;
      _anim_limit_scale    = p->_anim_limit_scale;
      _anim_trans_time     = p->_anim_trans_time;
      _anim_max_lod        = p->_anim_max_lod;
      _anim_style          = p->_anim_style;

      _stroke.copy(p->_stroke);
   }

   /******** DATA_ITEM METHODS ********/
   DEFINE_RTTI_METHODS_BASE("HatchingGroupParams", CHatchingGroupParams*);
   virtual DATA_ITEM    *dup() const { return new HatchingGroupParams; }
   virtual CTAGlist     &tags() const;

   /******** I/O Access Methods ********/
 protected:
  
   float&  anim_lo_thresh_()        { return _anim_lo_thresh;   }
   float&  anim_hi_thresh_()        { return _anim_hi_thresh;   }
   float&  anim_lo_width_()         { return _anim_lo_width;    }
   float&  anim_hi_width_()         { return _anim_hi_width;    }
   float&  anim_global_scale_()     { return _anim_global_scale;}
   float&  anim_limit_scale_()      { return _anim_limit_scale; }
   float&  anim_trans_time_()       { return _anim_trans_time;  }
   float&  anim_max_lod_()          { return _anim_max_lod;     }
   int&    anim_style_()            { return _anim_style;       }

   virtual void         get_base_stroke (TAGformat &d);
   virtual void         put_base_stroke (TAGformat &d) const;

};


////////////////////////////////////////////
// HatchingGroup
////////////////////////////////////////////
//
// -Virtual base class of hatching groups 
// -Bare essentials of the interface
// -Actual derived classes also subclass
//  HatchingGroupBase which supplies
//  additional common internal methods
// -Provides the constructor via type index
//
////////////////////////////////////////////
#define CHatchingGroup const HatchingGroup

class HatchingGroup : public DATA_ITEM, public BMESHobs {
 public:
   /******** HATCHING GROUP TYPES ********/
   enum hatchinggroup_t 
   {
      BASE_HATCHING_GROUP = -1,
      FIXED_HATCHING_GROUP = 0,
      FREE_HATCHING_GROUP,
      NUM_HATCHING_GROUP_TYPES
   };
   enum hatchingcurve_t 
   {
      CURVE_MODE_PLANE = 0,
      CURVE_MODE_PROJECT,
      NUM_CURVE_MODE
   };
   enum hatchingstyle_t 
   {
      STYLE_MODE_NEAT = 0,
      STYLE_MODE_SLOPPY_ADD,
      STYLE_MODE_SLOPPY_REP,
      NUM_STYLE_MODE
   };

 private:
   /******** STATIC MEMBER VARIABLES ********/
   static TAGlist          *_hg_tags;

 public:
   /******** STATIC MEMBER METHODS ********/
   static const char * name(int t)
   {
      switch(t)
         {
          case BASE_HATCHING_GROUP: return "Base"; break;
          case FIXED_HATCHING_GROUP: return "Fixed"; break;
          case FREE_HATCHING_GROUP: return "Free"; break;
          default: return "";
         }
   }

   static const char * curve_mode_name(int t)
   {
      switch(t)
         {
          case CURVE_MODE_PLANE: return "Plane"; break;
          case CURVE_MODE_PROJECT: return "Project"; break;
          default: return "";
         }
   }

   static const char * style_mode_name(int t)
   {
      switch(t)
         {
          case STYLE_MODE_NEAT: return "Neat"; break;
          case STYLE_MODE_SLOPPY_ADD: return "Sloppy Add"; break;
          case STYLE_MODE_SLOPPY_REP: return "Sloppy Repl."; break;
          default: return "";
         }
   }

 protected:
   /******** MEMBER VARIABLES ********/
   Patch *                         _patch; 
   HatchingGroupParams             _params;
   bool                            _complete;
   HatchingStroke                  _prototype;
   long                            _random_seed;
   unsigned int                    _stamp;

 public:
   /******** CONSTRUCTOR/DESTRUCTOR ********/
   HatchingGroup(Patch *p=0) : 
      _patch(p), 
      _complete(false),
      _random_seed((long)the_time()) { _stamp = VIEW::stamp()-1; update_prototype(); }

   virtual ~HatchingGroup() { cerr << "HatchingGroup::~HatchingGroup()\n";}

   /******** MEMBER METHODS ********/

   //Random seed accessor
   long                 random_seed() { return _random_seed; }

   // Patch management
   Patch *              patch() { return _patch; }
   void                 set_patch(Patch *p) { _patch = p; }

   // Visiblity/LOD setting management
   bool                 is_complete() { return _complete; }

   // Parameters management
   void                 set_params(HatchingGroupParams *p) { _params.copy(p); update_prototype(); }
   HatchingGroupParams* get_params() { return &_params; }

   HatchingStroke*      prototype() { return &_prototype; }
 public:
   /******** MEMBER METHODS ********/

   //Updates the prototype stroke with current
   //params.  Should be subclassed to also
   //cause strokes of the hatches to also update
   //their strokes...
   virtual void         update_prototype();

   /******** VIRTUAL MEMBER METHODS ********/
   // These should be pure virtual, but to be
   // a DATA_ITEM, we must be instantiatable
   // so we have them do nothing instead...
   virtual int          type() { return BASE_HATCHING_GROUP;}

   virtual void         select() {}
   virtual void         deselect() {}

   virtual bool         complete() { return false;}

   virtual int          draw(CVIEWptr & /* v */) { return 0;}

   virtual bool         query_pick(mlib::CNDCpt & /* pt */) { return false; }

   virtual bool         add(mlib::CNDCpt_list &, const ARRAY<double>&, int) { return false; }

   virtual void         kill_animation() {}

   virtual bool         undo_last() { return false; }

   /******** DATA_ITEM METHODS ********/
   DEFINE_RTTI_METHODS_BASE("HatchingGroup", CHatchingGroup*);
   virtual DATA_ITEM    *dup() const { return new HatchingGroup; }
   virtual CTAGlist     &tags() const;

   /******** I/O functions ********/
   virtual void         get_hatching_group_params (TAGformat &d);
   virtual void         put_hatching_group_params (TAGformat &d) const;

};

#endif //end of HatchingGroupHeader
