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
#ifndef ZXEDGE_STROKE_TEXTURE_HEADER
#define ZXEDGE_STROKE_TEXTURE_HEADER

#include "gtex/basic_texture.H"
#include "gtex/ref_image.H"
#include "mesh/uv_data.H"
#include "mesh/bsimplex.H"
#include "stroke/base_stroke.H"

/*
 * VISIBILITY DEFINITIONS
 *    SIL_VISIBLE        - sil is fully visible
 *    SIL_HIDDEN         - sil is hidden by itself or other seethru objects
 *    SIL_OCCLUDED       - sil is frontfacing but occluded by itself or
 *                         another opaque object
 *    SIL_VIS_NUM        -
 *    SIL_BACKFACING     -
 *    SIL_OUT_OF_FRUSTUM - located out of frustum and not considered for
 *                         renderning
 *
 *    I set the SIL_VIS_NUM out of order because SIL_BACKFACING and
 *    SIL_OUT_OF_FRUSTUM are just legacy, since OOF aren't even considered
 *    and BACKFACING is now a "type"
 *    
 *
 *    Standard case ( all objects opaque ):
 *       We only deal with VISIBLE, OCCLUDED, BACKFACING, OUT_OF_FRUSTUM
 *       We only render silhoutte chains found to be VISIBLE
 *
 */
enum {SIL_VISIBLE=0, SIL_HIDDEN, SIL_OCCLUDED, SIL_VIS_NUM, SIL_BACKFACING, SIL_OUT_OF_FRUSTUM };

//enumerate the various drawing combinations to use with the gui flags
// we can access a flag from a ( type,vis ) pair  = (type)*SIL_VIS_NUM + (vis);
enum {ZXFLAG_SIL_VISIBLE=0, ZXFLAG_SIL_HIDDEN,   ZXFLAG_SIL_OCCLUDED,
      ZXFLAG_BF_SIL_VISIBLE, ZXFLAG_BF_SIL_HIDDEN,  ZXFLAG_BF_SIL_OCCLUDED,
      ZXFLAG_BORDER_VISIBLE, ZXFLAG_BORDER_HIDDEN,  ZXFLAG_BORDER_OCCLUDED,
      ZXFLAG_CREASE_VISIBLE, ZXFLAG_CREASE_HIDDEN,  ZXFLAG_CREASE_OCCLUDED,
      ZXFLAG_NUM };


/*****************************************************************
 * LuboSample:
 *
 *      Simple class containing data for a single "sample"
 *      used in propagating silhouette stroke parameterizations
 *      as in Lubo Bourdev's master's thesis.
 *****************************************************************/

class LuboSample
{
public:
   LuboSample() : _id(0), _t(0), _type(STYPE_SIL), _vis(SIL_VISIBLE) {}
   LuboSample(uint id, mlib::CNDCZpt& p, mlib::CNDCZvec& n, double t) :
         _id(id),
         _p(p),
         _n(n),
         _s(NULL),        
         _bc(mlib::Wvec (0,0,0)),
         _t(t),
         _type(STYPE_SIL),
         _vis(SIL_VISIBLE)
   {}

   LuboSample(uint id, mlib::CNDCZpt& p, mlib::CNDCZvec& n, double t, Bsimplex * s, mlib::CWvec& bc) :
         _id(id),
         _p(p),
         _n(n),
         _s(s),        
         _bc(bc),
         _t(t),
         _type(STYPE_SIL),
         _vis(SIL_VISIBLE)
   {}

   // Needed to instantiate an ARRAY of LuboSamples:
   bool operator==(const LuboSample& s) const
   {
      return (_id == s._id && _p == s._p && _t == s._t);
   }

   // Publicly accessible data members:
   void        get_wpt(mlib::Wpt& wpt)    {if(_s)_s->bc2pos(_bc, wpt);}

   uint        _id;        // lubo path ID (really LuboPath *) [phase out]
   uint        _path_id;   // id of votepath
   uint        _stroke_id; // id of stroke
   mlib::NDCZpt      _p;         // screen location
   mlib::NDCZvec     _n;         // "normal" direction in NDC
   Bsimplex*   _s;         // simplex pointer ( for barycentrics )   
   mlib::Wvec        _bc;        // barycentric coordinate
   double      _t;         // stroke parameter
   int         _type;      // stroke type
   int         _vis;       // visible or invisible (hidden-line)
};




/*****************************************************************
 * LuboVote:
 *****************************************************************/
class LuboVote : public DATA_ITEM
{
public:
   /******** PUBLIC MEMBERS TYPES ********/
   enum lv_status_t {
      VOTE_GOOD = 0,
      VOTE_OUTLIER,
      VOTE_HEALER
   };

   /******** STATIC MEMBERS METHODS ********/
   static const char * lv_status(lv_status_t t)
   {
      switch(t) {
      case VOTE_GOOD:
         return "VOTE_GOOD";
         break;
      case VOTE_OUTLIER:
         return "VOTE_OUTLIER";
         break;
      case VOTE_HEALER:
         return "VOTE_HEALER";
         break;
      default:
         return "";
      }
   }

   //private:
   /******** STATIC MEMBERS VARS ********/
   //static TAGlist*   _lv_tags;

public:
   /******** MEMBERS VARIABLES ********/
   lv_status_t  _status;

   uint         _stroke_id;
   uint         _path_id;

   double       _s;
   double       _t;
   double       _conf;

   double       _world_dist;
   double       _ndc_dist;

   /******** CONSTRUCTOR/DECONSTRUCTOR ********/
   LuboVote() :
         _status(VOTE_GOOD),
         _stroke_id(0),
         _path_id(0),
         _s(0),
         _t(0),
         _conf(0) {}

   LuboVote(double cur_arclen,
            double t, double conf=0.0) :
         _status(VOTE_GOOD),
         _stroke_id(0),
         _path_id(0),
         _s(cur_arclen),
         _t(t),
   _conf(conf) {}

   /******** MEMBERS METHODS ********/

   // Needed to instantiate an ARRAY of LuboVotes:
   bool operator==(const LuboVote& v) const
   {
      return true;
   }

   /******** DATA_ITEM METHODS ********/
public:
   DEFINE_RTTI_METHODS_BASE("LuboVote", CDATA_ITEM *);
   virtual DATA_ITEM*   dup() const  { return new LuboVote; }
   //virtual CTAGlist&    tags() const;

   //protected:
   /******** I/O Methods ********/
   //virtual void   get_status (TAGformat &d);
   //virtual void   put_status (TAGformat &d) const;

   /******** I/O Access Methods ********/
   //unsigned int & stroke_id_()         { return _stroke_id; }
   //unsigned int & path_id_()           { return _path_id; }
   //double&        s_()                 { return _s; }
   //double&        t_()                 { return _t; }
   //double&        conf_()              { return _conf; }
   //double&        world_dist_()        { return _world_dist; }
   //double&        ndc_dist_()          { return _ndc_dist; }


};

/*****************************************************************
 * VoteGroup
 ******************************************************************/

class LuboPath;

class VoteGroup : public DATA_ITEM
{
public:
   /******** PUBLIC MEMBERS TYPES ********/
   enum vg_status_t {
      VOTE_GROUP_GOOD = 0,
      VOTE_GROUP_LOW_LENGTH,
      VOTE_GROUP_LOW_VOTES,
      VOTE_GROUP_BAD_DENSITY,
      VOTE_GROUP_SPLIT_LOOP,
      VOTE_GROUP_SPLIT_LARGE_DELTA,
      VOTE_GROUP_SPLIT_GAP,
      VOTE_GROUP_CULL_BACKWARDS,
      VOTE_GROUP_SPLIT_ALL_BACKTRACK,
      VOTE_GROUP_FIT_BACKWARDS,
      VOTE_GROUP_FINAL_FIT_BACKWARDS,
      VOTE_GROUP_NOT_MAJORITY,
      VOTE_GROUP_NOT_ONE_TO_ONE,
      VOTE_GROUP_NOT_HYBRID,
      VOTE_GROUP_HEALED
   };

   enum fit_status_t {
      FIT_GOOD = 0,
      FIT_NONE,
      FIT_BACKWARDS,
      FIT_OLD
   };

   /******** STATIC MEMBERS METHODS ********/
   static const char * fit_status(fit_status_t t)
   {
      switch(t) {
      case FIT_GOOD:
         return "FIT_GOOD";
         break;
      case FIT_NONE:
         return "FIT_NONE";
         break;
      case FIT_BACKWARDS:
         return "FIT_BACKWARDS";
         break;
      case FIT_OLD:
         return "FIT_OLD";
         break;
      default:
         return "";
      }
   }

   static const char * vg_status(vg_status_t t)
   {
      switch(t) {
      case VOTE_GROUP_GOOD:
         return "VOTE_GROUP_GOOD";
         break;
      case VOTE_GROUP_LOW_LENGTH:
         return "VOTE_GROUP_LOW_LENGTH";
         break;
      case VOTE_GROUP_LOW_VOTES:
         return "VOTE_GROUP_LOW_VOTES";
         break;
      case VOTE_GROUP_BAD_DENSITY:
         return "VOTE_GROUP_BAD_DENSITY";
         break;
      case VOTE_GROUP_SPLIT_LOOP:
         return "VOTE_GROUP_SPLIT_LOOP";
         break;
      case VOTE_GROUP_SPLIT_LARGE_DELTA:
         return "VOTE_GROUP_SPLIT_LARGE_DELTA";
         break;
      case VOTE_GROUP_SPLIT_GAP:
         return "VOTE_GROUP_SPLIT_GAP";
         break;
      case VOTE_GROUP_CULL_BACKWARDS:
         return "VOTE_GROUP_CULL_BACKWARDS";
         break;
      case VOTE_GROUP_SPLIT_ALL_BACKTRACK:
         return "VOTE_GROUP_SPLIT_ALL_BACKTRACK";
         break;
      case VOTE_GROUP_FIT_BACKWARDS:
         return "VOTE_GROUP_FIT_BACKWARDS";
         break;
      case VOTE_GROUP_FINAL_FIT_BACKWARDS:
         return "VOTE_GROUP_FINAL_FIT_BACKWARDS";
         break;
      case VOTE_GROUP_NOT_MAJORITY:
         return "VOTE_GROUP_NOT_MAJORITY";
         break;
      case VOTE_GROUP_NOT_ONE_TO_ONE:
         return "VOTE_GROUP_NOT_ONE_TO_ONE";
         break;
      case VOTE_GROUP_NOT_HYBRID:
         return "VOTE_GROUP_NOT_HYBRID";
         break;
      case VOTE_GROUP_HEALED:
         return "VOTE_GROUP_HEALED";
         break;
      default:
         return "";
      }
   }

private:
   /******** STATIC MEMBERS VARS ********/
   static TAGlist*      _vg_tags;

   /******** MEMBERS VARIABLES ********/

   LuboPath*            _lubo_path;

   vg_status_t          _status;
   fit_status_t         _fstatus;

   double               _confidence;

   double               _begin;
   double               _end;

   int                  _vis;   //visibility type
   uint                 _base_id;

   ARRAY<LuboVote>      _votes;     //Copies of LuboPath votes
   ARRAY<mlib::XYpt>          _fits;


public:
   /******** CONSTRUCTOR/DECONSTRUCTOR ********/

   VoteGroup (LuboPath *p=NULL) :
         _lubo_path(p),
         _status(VOTE_GROUP_GOOD),
         _fstatus(FIT_NONE),
         _confidence(0),
         _begin(0),
         _end(0),
         _vis(SIL_VISIBLE),
         _base_id(0) {}

   VoteGroup (int id, LuboPath *p=NULL) :
         _lubo_path(p),
         _status(VOTE_GROUP_GOOD),
         _fstatus(FIT_NONE),
         _confidence(0),
         _begin(0),
         _end(0),
         _vis(SIL_VISIBLE),
         _base_id(id) {}


   /******** MEMBERS METHODS ********/

   LuboPath*&        lubo_path()    { return _lubo_path; }

   vg_status_t&      status()       { return _status;    }
   fit_status_t&     fstatus()      { return _fstatus;   }

   double&           confidence()   { return _confidence;}

   double&           begin()        { return _begin;     }
   double&           end()          { return _end;       }

   uint&             id()           { return _base_id;   }
   int&              vis()          { return _vis;   }


   ARRAY<LuboVote>&  votes()        { return _votes;     }
   LuboVote&         vote(int i)    { return _votes[i];  }
   LuboVote&         first_vote()   { return _votes.first();  }
   LuboVote&         last_vote()    { return _votes.last();   }
   int               num()          { return _votes.num(); }


   ARRAY<mlib::XYpt>&      fits()         { return _fits;      }
   mlib::XYpt&             fit(int i )    { return _fits[i];   }
   int               nfits()        { return _fits.num();}

   //Query the fit
   double            get_t( double s );

   void              sort();
   void              fitsort();
   void              add(LuboVote v)  { _votes += v; }

   bool operator ==( VoteGroup g )     { return true;  }


   /******** DATA_ITEM METHODS ********/
public:
   DEFINE_RTTI_METHODS_BASE("VoteGroup", CDATA_ITEM *);
   virtual DATA_ITEM*   dup() const  { return new VoteGroup(); }
   virtual CTAGlist&    tags() const;

protected:
   /******** I/O Methods ********/
   virtual void   get_status (TAGformat &d);
   virtual void   put_status (TAGformat &d) const;
   virtual void   get_fstatus (TAGformat &d);
   virtual void   put_fstatus (TAGformat &d) const;
   virtual void   get_fits (TAGformat &d);
   virtual void   put_fits (TAGformat &d) const;
   virtual void   get_vote (TAGformat &d);
   virtual void   put_votes (TAGformat &d) const;


   /******** I/O Access Methods ********/
   unsigned int & base_id_()           { return _base_id; }
   double&        confidence_()        { return _confidence; }
   double&        begin_()             { return _begin; }
   double&        end_()               { return _end; }
};

/*****************************************************************
 * LuboPath
 *****************************************************************/
class LuboPath : public DATA_ITEM
{

   /******** STATIC MEMBERS VARS ********/
private:
   static TAGlist*   _lp_tags;
protected:
   static uint       _lubosample_id;
   static uint       _lubostroke_id;

   /******** MEMBERS VARIABLES ********/
   double            _stretch;      // cached stuff
   double            _pix_to_ndc_scale;
   double            _offset_pix_len;
   int               _line_type;     // derived from _vis and _type (cached to avoid recomputation)

   // these will be consistent over the entire path
   int               _vis;    // visible type (hidden line)
   int               _type;   // line type ( silhouette, backfacing, border, crease 

   mlib::NDCZpt_list       _pts;          // path in screen space
   ARRAY<int>        _path_id;      // path id per _pt, sent to ID ref
   ARRAY<mlib::Wvec>       _bcs;          // barycentric coords
   ARRAY<Bsimplex*>     _simplexes;    // simplexes for barycentric recalc.   
   ARRAY<double>     _len;          // distance along ffseg (NOT ACTUAL NDCZdist of POINTS);
   ARRAY<uint>       _id_set;       // list of distinct path ids
   ARRAY<int>        _id_offsets;   // offsets of these id's in the path
   ARRAY<double>     _ffseg_lengths;// length of the idref path drawn in that ID

   ARRAY<LuboVote>   _votes ;       // votes received
   ARRAY<VoteGroup>  _groups;       // grouped vote structures


public:
   /******** CONSTRUCTOR/DECONSTRUCTOR ********/
   LuboPath() :
         _stretch(1),_pix_to_ndc_scale(1),_offset_pix_len(1),
         _line_type(0),_vis(SIL_VISIBLE),_type(STYPE_SIL)
   {
      _id_set.set_unique();
   }

   /******** MEMBER METHODS ********/

   /******** Accessors/Conveniences ********/

   double stretch()                 const { return _stretch; }
   double pix_to_ndc_scale()        const { return _pix_to_ndc_scale; }
   double offset_pix_len()          const { return _offset_pix_len; }

   void   set_stretch(double s)           { _stretch = s; }
   void   set_pix_to_ndc_scale(double s)  { _pix_to_ndc_scale = s; }
   void   set_offset_pix_len(double s)    { _offset_pix_len = s; }

   int      num()                   const { return _pts.num(); }
   int      is_closed()             const { return _pts.num() > 2 && _pts[0] == _pts.last(); }
   double   length()                const { return _pts.length(); }

   mlib::CNDCZpt& pt(int i)               const { return _pts[i]; }
   mlib::CNDCZpt& last()                  const { return _pts.last(); }
   mlib::CNDCZpt  pt(double u)            const { return _pts.interpolate(u); }

   mlib::NDCvec   vec(int i)              const { return _pts[i+1] - _pts[i]; }
   mlib::NDCline  seg(int i)              const { return mlib::NDCline(pt(i), pt(i+1)); }

   mlib::NDCvec   tan(int i) const
   {
      const int n = _pts.num()-1;
      if (i<0 || i>n || n<1)
         return mlib::NDCvec::null();
      if (i==0)
         return (vec(0))  .normalized();
      if (i==n)
         return (vec(n-1)).normalized();
      return (vec(i).normalized() + vec(i-1).normalized()).normalized();
   }


   /******** Building ********/
   void add(mlib::CNDCZpt& p, bool vis, uint id);
   void add(mlib::CNDCZpt& p, bool vis, uint id, Bsimplex * f, mlib::CWvec& bc, double len = 0.0);
   void complete() { _pts.update_length(); }
   void clear();

   //******** Voting stuff ********
   ARRAY<LuboVote>&  votes()                   { return   _votes; }
   ARRAY<VoteGroup>& groups()                  { return   _groups; }

   uint              id(int i)           const { return _path_id[i];        }

   int&              vis()                     { return _vis;   }
   int&              type()                    { return _type;   }
   int&              line_type()               { return _line_type;   }

   bool              owns_id(uint id)    const { return _id_set.contains(id); }
   uint              id_set(int i )      const { return _id_set[i];         }
   ARRAY<uint>&      id_set()                  { return _id_set;            }

   int               id_offset(int i)          { return _id_offsets[i];     }
   ARRAY<int>&       id_offsets()              { return _id_offsets;        }

   double&           ffseg_length(int i)       { return _ffseg_lengths[i];  }
   ARRAY<double>&    ffseg_lengths()           { return _ffseg_lengths;     }

   void              reset_votes()             { _votes.clear(); }

   // Generate lubo samples for next frame
   void              gen_group_samples(double spacing, int path_index, ARRAY<LuboSample>& samples) const;

   // Receive a vote, record it if things go well
   double            get_closest_point(mlib::CNDCpt& p, mlib::CNDCvec& v, mlib::NDCpt& ret_pt, int& ret_index);
   double            get_closest_point_at(uint id, mlib::CNDCpt& p, mlib::CNDCvec& v, mlib::NDCpt& ret_pt, int& ret_index);

   bool              register_vote (LuboSample& sample, int path_id, mlib::CNDCpt& pt, int index );

   //******** Parameterization ********

   double&  ff_len(int i)                 {  return _len[i]; }
   bool     in_range( uint id);

   // At given index, get arc-length parameter in NDC (s varies from 0 to L over stroke, L == length):
   double get_s(int i) const
   {
      if (_pts.empty()||i<0)
         return 0;
      if (i>=_pts.num())
         return _pts.length();
      return _pts.partial_length(i);
   }

   void   get_wpt ( int i, mlib::Wpt& w )
   {
      if (_simplexes[i])
         _simplexes[i]->bc2pos( _bcs[i], w );
      else if ( _simplexes.num() > 0 && i > 0 && _simplexes[i-1])
         _simplexes[i-1]->bc2pos( _bcs[i], w );
   }

static uint  gen_sample_id()           { return ( ++_lubosample_id == 0 ) ? ++_lubosample_id : _lubosample_id; }
static uint  gen_stroke_id()           { return ( ++_lubostroke_id == 0 ) ? ++_lubostroke_id : _lubostroke_id; }

public:
   //******** Copying ********
   const LuboPath& operator=(const LuboPath &l)
   {
      _pts              = l._pts;
      _path_id          = l._path_id;
      _bcs              = l._bcs;
      _simplexes        = l._simplexes;
      _len              = l._len;
      _id_set           = l._id_set;
      _id_offsets       = l._id_offsets;
      _ffseg_lengths    = l._ffseg_lengths;

      _stretch          = l._stretch;
      _pix_to_ndc_scale = l._pix_to_ndc_scale;
      _offset_pix_len   = l._offset_pix_len;

      _votes            = l._votes;

      _groups           = l._groups;

      for (int i=0; i<_groups.num(); i++)
         _groups[i].lubo_path() = this;

      return *this;
   }

   /******** DATA_ITEM METHODS ********/
public:
   DEFINE_RTTI_METHODS_BASE("LuboPath", CDATA_ITEM *);
virtual DATA_ITEM*   dup() const  { return new LuboPath; }
   virtual CTAGlist&    tags() const;

protected:
   /******** I/O Methods ********/
   virtual void   get_pts (TAGformat &d);
   virtual void   put_pts (TAGformat &d) const;
   virtual void   get_path_id (TAGformat &d);
   virtual void   put_path_id (TAGformat &d) const;
   virtual void   get_bcs (TAGformat &d);
   virtual void   put_bcs (TAGformat &d) const;
   virtual void   get_faces (TAGformat &d);
   virtual void   put_faces (TAGformat &d) const;
   virtual void   get_len (TAGformat &d);
   virtual void   put_len (TAGformat &d) const;
   virtual void   get_id_set (TAGformat &d);
   virtual void   put_id_set (TAGformat &d) const;
   virtual void   get_id_offsets (TAGformat &d);
   virtual void   put_id_offsets (TAGformat &d) const;
   virtual void   get_ffseg_lengths (TAGformat &d);
   virtual void   put_ffseg_lengths (TAGformat &d) const;

   virtual void   get_vote (TAGformat &d);
   virtual void   put_votes (TAGformat &d) const;
   virtual void   get_group (TAGformat &d);
   virtual void   put_groups (TAGformat &d) const;

   /******** I/O Access Methods ********/
   double&        stretch_()           { return _stretch; }
   double&        pix_to_ndc_scale_()  { return _pix_to_ndc_scale; }
   double&        offset_pix_len_()    { return _offset_pix_len; }

};

/*****************************************************************
 * LuboPathList
 *****************************************************************/

class LuboPathList : public ARRAY<LuboPath*>, public DATA_ITEM
{

   /******** STATIC MEMBERS VARS ********/
private:
   static TAGlist*         _lpl_tags;

   /******** MEMBERS VARS ********/
protected:

   /******** CONSTRUCTOR/DECONSTRUCTOR ********/
public:
   LuboPathList(int n=0) : ARRAY<LuboPath*>(n) {}

   /******** MEMBERS METHODS ********/
   void delete_all()
   {
      while (!empty())
         delete pop();
   }

   void clone(const LuboPathList &pl)
   {
      assert(!num());
      for(int i=0;i<pl.num();i++) {
         LuboPath *p = new LuboPath();
         assert(p);
         add
            (p);
         *p = *(pl[i]);
      }
   }

   LuboPath* lookup(uint id, int& x) const
   {
      for (int i=x; i<_num; i++)
      if (_array[i]->owns_id(id)) { x = i; return _array[i]; }
      x = _num;
      return 0;
   }

   void gen_group_samples(double spacing, ARRAY<LuboSample>& samples) const
   {
      samples.clear();
      for (int i=0; i<_num; i++)
         _array[i]->gen_group_samples(spacing, i,  samples);
   }

   void reset_votes() const
   {
      for (int i=0; i<_num; i++)
         _array[i]->reset_votes();
   }

   /******** DATA_ITEM METHODS ********/
public:
   DEFINE_RTTI_METHODS_BASE("LuboPathList", CDATA_ITEM *);
virtual DATA_ITEM*   dup() const  { return new LuboPathList(); }
   virtual CTAGlist&    tags() const;

protected:
   /******** I/O Methods ********/
   virtual void   get_path (TAGformat &d);
   virtual void   put_paths (TAGformat &d) const;

public:
   /******** Tracking Convenience Methods ********/
   int            votepath_id_to_index(uint id) const;
   int            strokepath_id_to_index(uint id, int path_index) const;
   bool           strokepath_id_to_indices(uint id, int* path_index, int* stroke_index ) const;

};

/***********************************************************************
* SilSeg - one segment of the silhouette polyline that is created 
* after the zero-crossing silhouettes have been passed through visibility
* and screen-space sampling
***********************************************************************/

class SilSeg
{
protected:
   mlib::NDCZpt          _p;          // screen location
   bool            _e;
   int             _v;
   Bvert*          _bv;
   uint            _id;
   uint            _id_invis;
   Bsimplex*       _s;          // simplex pointer ( for barycentrics )
   mlib::Wvec            _bc;         // barycentric coordinate
   mlib::Wpt             _w;
   double          _l;
   double          _pl;
   int             _type;        // stroke type

public:

   /*CONSTRUCTORS*/
   SilSeg( mlib::CNDCZpt &pt, bool e, int v, uint id,
           Bvert *bv, Bsimplex * s, mlib::CWpt& wpt_bc, double l, double pl=0) :
         _p  (pt),
         _e  (e),
         _v  (v),
         _bv (bv),
         _id (id),
         _id_invis(0),
         _s  (s),
         _w  (wpt_bc),
         _l  (l),
         _pl (pl),
         _type(STYPE_SIL)
   {
      //XXX this needs to be schmarta..
      if (_s)
         _s->project_barycentric (wpt_bc, _bc);
   }

   SilSeg(mlib::CNDCZpt &pt, bool e, int v, uint id,
          Bvert *bv, Bsimplex * s, mlib::CWvec & bc) :
         _p  (pt),
         _e  (e),
         _v  (v),
         _bv (bv),
         _id (id),
         _id_invis(0),
         _s  (s),
         _bc (bc),
         _l  (0),
         _type(STYPE_SIL)
{}

   SilSeg ()
   {
      _p.set(0,0,0);
      _e  = false;
      _v  = SIL_VISIBLE;
      _type= STYPE_SIL;
      _id  = 0;
      _id_invis = 0;
      _bv  = NULL;
      _s  = NULL;
      _bc.set(0,0,0);
      _w.set(0,0,0);
      _l   = 0;     //arclength on its path
      _pl  = 0;     //parameter length ( 0 - ffseglen );
   }
   /*ACCESSORS */

   mlib::CNDCZpt&     p()         {return _p;  }
   bool&        e()         {return _e;  }
   int&         v()         {return _v;  }
   int&         type()      {return _type;}
   Bvert*       bv()        {return _bv; }
   Bsimplex*    s()         {return _s; }
   //Bface*       f()         {return get_bface(_s); } // do we need it?   
   uint&        id()        {return _id; }
   uint&        id_invis()  {return _id_invis; }
   mlib::CWvec        bc()        {return _bc; }
   mlib::CWpt         w()         {return _w;  }
   double&      l()         {return _l;  }
   double&      pl()        {return _pl; }

   /*OPERATORS*/
   bool operator ==( const SilSeg& z) const {return _p==z._p && _e==z._e && _v==z._v && _id==z._id && _bv==z._bv;}

};


#define CSilSeg const SilSeg
#define NDCSilPath ARRAY<SilSeg>

/*****************************************************************
 * ZXedgeStrokeTexture
 *****************************************************************/
class ZXedgeStrokeTexture : public OGLTexture
{

protected:
   /******** MEMBER VARIABLES ********/
   CEdgeStrip*                  _stroke_3d;     // if there is no patch, we use EdgeStrip
   mlib::CWpt_list*                   _polyline;       // if we don't have a EdgeStrip,
                                                // we might have a polyline
   ARRAY <ZXseg>                _pre_zx_segs;
   NDCSilPath                   _ref_segs;
   NDCSilPath                   _sil_segs;   
   ARRAY<uint>                  _path_ids;
   ARRAY<double>                _ffseg_lengths;
   BaseStroke                   _prototype;     // prototype stroke
   BaseStrokeOffsetLISTptr      _offsets;       // wiggles to apply to strokes   
   BaseStrokeArray              _bstrokes;      // collection of strokes

   // Items cached for each frame:                                               
   double        _pix_to_ndc_scale;  // pix to ndc scale
   double        _vis_sampling;      // distance to sample for vis, in pixels
   double        _stroke_sampling;   // distance between stroke samples

   // Multiplier between screen distance
   // and ref image distance:
   double        _screen_to_ref;
   bool          _draw_creases;
   bool          _render_flags[ZXFLAG_NUM];
   int           _seethru;  //SIL_VISIBLE ( 0 ) only ff visible
   int           _new_branch;   
   uint          _cam_change_stamp;
   int           _last_xform_id;
   bool          _dirty;        
   bool          _need_update;  
   uint          _stamp;
   uint          _strokes_drawn_stamp;

   unsigned int  _paths_created_stamp;  // okay -- debugging cares what
   // frame the paths were generated in
   unsigned int  _groups_created_stamp;  // okay -- debugging cares what
   // frame the paths were generated in


   // for lubo computations:
   LuboPathList         _paths;
   mlib::Wtransf              _old_ndc;
   ARRAY<LuboSample>    _lubo_samples; 
  
   double       _crease_max_bend_angle;
   
   static uint  _next_id_stamp;
   static uint  _next_id;
   static bool  _use_new_idref_method;


   //******** ID BITS ********
   // NOTE: the following code requires 32-bit framebuffer to work right.
   // XXX - should be made more general.

   static void init_next_id();

   // first bit of id need to be 1 to be a silhouette id
   uint gen_id() const { return (++_next_id << 8 ) | 0x80000000; }

   // when we are doing hidden line rendering, the second bit determines
   // if the id is a visible (1) or invisible (0) silhouette

   /*
     uint gen_id_vis()  const   { return ((++_next_id << 8 ) | 0x80000000 ) | 0x40000000; } //second bit on
     uint gen_id_invis()const   { return ((++_next_id << 8 ) | 0x80000000 ) & 0xbfffffff; } //second bit off

     bool is_path_id(uint x)            const     { return !!(x & 0x80000000); }
     bool is_vis_path_id(uint x) const     { return (!!(x & 0x80000000) && !!(x & 0x40000000)); }
     bool is_invis_path_id(uint x) const     { return (!!(x & 0x80000000) &&  !(x & 0x40000000)); }
   */

   // we're modifying the code to put the bits into the ...
   uint gen_id_vis() const
   {
      ++_next_id;
      return ( ( ( _next_id & 0x0000007f ) << 8 ) |
               ( ( _next_id & 0x003fff80 ) << 9 ) |
               0x80008000);        // first bit(id) on and 17th(vis) on
   }
   uint gen_id_invis() const
   {
      ++_next_id;
      return ( ( ( _next_id & 0x0000007f ) << 8 ) |
               ( ( _next_id & 0x003fff80 ) << 9 ) |
               0x80000000);        // first bit(id) on and 17th(vis) off
   }

   bool is_path_id(uint x)      const     { return !!(x & 0x80000000); }
   bool is_vis_path_id(uint x) const     { return (!!(x & 0x80000000) && !!(x & 0x00008000)); }
   bool is_invis_path_id(uint x)const     { return (!!(x & 0x80000000) &&  !(x & 0x00008000)); }


public:
   LMESHptr     _mesh;  
   RefImage*    _id_ref;

   /******** MEMBER METHODS ********/
protected:

   bool strokes_need_update();
   int  check_vis( int i , mlib::CNDCZpt& npt, int path_id );
   int  check_vis_mask(SilSeg& s);
   int  check_vis_mask_seethru(SilSeg& s);

   void setIDcolor(int path_id);
   void setIDcolor_param(uint path_id);
   void chop_n ( int start, int num );
   void loop_clean(int pstart, int pend, bool close_loop );

   void sil_path_preprocess();
   void sil_path_preprocess_seethru();

   int  draw_id_ref_parameterized();

   int  draw_id_ref_param_object_pass();
   int  draw_id_ref_param_invis_pass();
   int  draw_id_ref_param_vis_pass();

   void resample_ndcz();
   void resample_ndcz_seethru();

   void sils_to_ndcz();

   void ndcz_to_strokes();


   //******** PATH BUILDING ********

   bool add_path(int&);
   bool add_path_seethru(int&);
   void add_paths();
   void add_paths_seethru();

   LuboPath* find_path(mlib::CNDCpt& p) const;

   double compute_stretch();

   // if we treat front and backfacing sils as different types
   // we need to process them before the idref pass
   void sils_split_on_gradient();

   // Add crease edges to the list of valid silhouettes:
   void add_creases_to_sils();

   // Add border edges to the list of valid silhouettes:
   // (Border edges have just 1 adjacent face)
   void add_borders_to_sils();

   // Add wpaths to sils, wpaths are stored in stroke_3d var
   // which is passed into zxedge by set_stroke_3d
   void add_wpath_to_sils();

   void add_polyline_to_sils();

   // Add given edge strip to the list of valid silhouettes:
   void add_to_sils(CEdgeStrip& strip, int type, double angle=-1.0);  //we need to indicate the type now because the zx_sils need to be marked.


public:

   //******** MANAGERS ********    
   ZXedgeStrokeTexture(Patch* patch = 0);
   ZXedgeStrokeTexture(CLMESHptr& mesh);
  // ZXedgeStrokeTexture(){} //dummy constructor
   
   virtual ~ZXedgeStrokeTexture();

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS2("ZX Sil Strokes", OGLTexture, CDATA_ITEM *);

   //******** ACCESSORS ********

   
   double   get_crease_max_bend_angle() const           { return _crease_max_bend_angle;    }
   void     set_crease_max_bend_angle(double a)         { _crease_max_bend_angle = a;       }

   // Number of points tested for visibility for each one added
   // to a stroke. Useful for stepping along pre-processed
   // _npoints array. (I.e. _npoints is already sampled at
   // vis_sampling distance).
   int sample_step() const
   {
      return (int)round(_stroke_sampling/_vis_sampling);
   }

   bool get_render_flag(int type, int vis )        { return _render_flags[ SIL_VIS_NUM * type + vis];}
   bool get_render_flag(int i )              { return _render_flags[i];         }
   void set_render_flag(int type, int vis, bool val )   { _render_flags[ SIL_VIS_NUM * type + vis] = val; }
   void set_render_flag(int i, bool val )         { _render_flags[i] = val;         }

   bool check_render_flags(int type, int vis)
   {
      if ( get_seethru() )
         return get_render_flag(type, vis);
      return ( type != STYPE_BF_SIL && vis == SIL_VISIBLE && get_render_flag (type, vis) );
   }

   bool type_is_enabled(int type)
   {
      return ( get_render_flag(type, SIL_VISIBLE) ||
               get_render_flag(type, SIL_HIDDEN)  ||
               get_render_flag(type, SIL_OCCLUDED)  );
   } // do we need to draw this one in the idrefs?

   bool type_visible_only(int type)
   {
      return (  get_render_flag(type, SIL_VISIBLE)  &&
                !get_render_flag(type, SIL_HIDDEN)   &&
                !get_render_flag(type, SIL_OCCLUDED)  );
   } // do we need to draw this one in the idrefs?

   bool type_draws_to_idref_invis(int type)
   {
      if  ( get_seethru() )
         return ( type_is_enabled(type) && !type_visible_only(type) );
      return false;
   }

   bool type_draws_to_idref_vis (int type)
   {
      if  ( get_seethru() )
         return ( type_is_enabled(type) && type != STYPE_BF_SIL );
      return ( type != STYPE_BF_SIL  && get_render_flag(type, SIL_VISIBLE) );
   }

   bool  get_seethru()             { return (_seethru != 0 );   }
   void  set_seethru(int s)        { _seethru = s;      }
   bool  get_new_branch()          { return (_new_branch != 0) ;  }
   void  set_new_branch(int b)     { _new_branch=b;      }
   LuboPathList&        paths()                 { return _paths;      }
   unsigned int         path_stamp()    const   { return _paths_created_stamp; }
   unsigned int         group_stamp()   const   { return _groups_created_stamp; }

   int  get_zxflag(int i)
   {
      if ( i >= 0 && i < ZXFLAG_NUM )
         return _render_flags[i];
      return 0;    // XXX - used to return nothing. is 0 correct?
   }
   //******** BUILDING METHODS ********
   void set_stroke_3d(CEdgeStrip* stroke_3d);  // adds 3d strokes, done if no patch
                                               // is available, (e.g. Bcurve)
   void set_polyline(mlib::CWpt_list& polyline);
   
   CBaseStroke& get_prototype()  const { return _prototype; }
   void set_prototype(CBaseStroke &s);
   void set_offsets(BaseStrokeOffsetLISTptr ol);

   void set_vis_sampling( int vis)
   {
      assert ( vis != 0 );
      _vis_sampling  = vis;
   }

   void set_stroke_sampling(int stroke)
   {
      assert (stroke != 0);
      _stroke_sampling = stroke;
   }

   //In PIX!!
   double get_sampling_dist ()
   {
      static int SAMPLE_STEP = Config::get_var_int( "LUBO_SAMPLE_STEP", 4,true);
      return _vis_sampling * SAMPLE_STEP;
   }


   void destroy_state()
   {
      _paths.clear();
      _lubo_samples.clear();
   }



   // (cached per frame)
   double pix_to_ndc_scale()     const   { return _pix_to_ndc_scale; }


   // Made this public so that sil building related
   // functions can be called from the outside. --mak
   void create_paths();  // does the whole thing

   // Added the following two funcs used in path building
   // so that they can be called from the outside without
   // having to call rebuild_if_needed(), which also
   // creates strokes along the paths, which is sometimes
   // not necessary. -- mak
   void cache_per_frame_vals(CVIEWptr &v);

   // Build the strokes:
   void rebuild_if_needed(CVIEWptr& v);

   // Do the "lubo" algorithm:
   void propagate_sil_parameterization();
   void propagate_sil_parameterization_seethru();


   bool id_fits_sample(uint id, LuboSample& s)  const { return (s._vis==SIL_VISIBLE) ? is_vis_path_id(id) : is_invis_path_id(id); }
   bool sample_matches_path(LuboSample& s, LuboPath*p ) const { return ( s._type == p->type() && s._vis == p->vis() ); }

   // make another call to gen_samples from silandcrease
   // once votes are processed.
   void regen_group_samples();

   void regen_group_notify()  { _groups_created_stamp = VIEW::stamp(); }
   void regen_paths_notify()  { _paths_created_stamp = VIEW::stamp(); }

   //******** DIAGNOSTIC ********

   // For demo purposes, enable or disable the "lubo" algorithm,
   // i.e. temporally coherent silhouette parameterizations.
   // (returns true if enabled):

   static bool toggle_idref_method() { return (_use_new_idref_method = !_use_new_idref_method); }

   //******** GTexture METHODS ********

   virtual int  draw_id_ref();
   virtual int  draw(CVIEWptr& v); 
   virtual int  draw_id_ref_pre1();
   virtual int  draw_id_ref_pre2();
   virtual int  draw_id_ref_pre3();
   virtual int  draw_id_ref_pre4();

   //******** Ref_Img_Client METHODS ********
   virtual void request_ref_imgs() {
      if (strokes_need_update()) {
         IDRefImage::schedule_update();
      }
   }

   //******** DATA_ITEM VIRTUAL FUNCTIONS ********
   virtual DATA_ITEM  *dup() const { return new ZXedgeStrokeTexture(); }
};

#endif // ZXEDGE_STROKE_TEXTURE_HEADER

/* end of file zxedge_stroke_texture.H */

