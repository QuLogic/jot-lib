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
#include "disp/view.H"
#include "gtex/gl_extensions.H" // has to come before any gl.h include
#include "gtex/paper_effect.H"
#include "geom/texturegl.H"
#include "geom/gl_util.H"
#include "std/config.H"
#include "base_stroke.H"

using namespace mlib;

#define OVERDRAW_STROKE_TEXTURE  Config::JOT_ROOT() + "nprdata/system_textures/overdraw.png"
#define OVERDRAW_STROKE_WIDTH    20.0f
#define OVERDRAW_STROKE_ALPHA    0.9f
#define OVERDRAW_STROKE_FLARE    0.0f
#define OVERDRAW_STROKE_TAPER    1.0f
#define OVERDRAW_STROKE_FADE     1.0f
#define OVERDRAW_STROKE_AFLARE   0.0f
#define OVERDRAW_STROKE_HALO     0.0f
#define OVERDRAW_STROKE_COLOR    COLOR(0.1f,0.7f,0.1f)
#define OVERDRAW_STROKE_ANGLE    30.0f
#define OVERDRAW_STROKE_PHASE    0.25f


/*****************************************************************
 * Stroke Texture Remapping
 *****************************************************************/

char *stroke_remap_base = "nprdata/stroke_textures/";
char *stroke_remap_fnames[][2] = 
{
   {"mydot3.png",    "2D--gauss-nar-8-s.png"},
   {"one_d.png",     "1D--gauss-med-16.png"},
   {"one_d_fat.png", "2D--gauss-med-16.png"},
   {NULL,            NULL}
};

/*****************************************************************
 * BaseStrokeOffset
 *****************************************************************/

static int foo3 = DECODER_ADD(BaseStrokeOffset);

TAGlist*  BaseStrokeOffset::_bso_tags = 0;

/////////////////////////////////////////////////////////////////
// BaseStrokeOffset Methods
/////////////////////////////////////////////////////////////////

/////////////////////////////////////
// tags()
/////////////////////////////////////

CTAGlist &
BaseStrokeOffset::tags() const
{
   if (!_bso_tags) {
      _bso_tags = new TAGlist;
      *_bso_tags += new TAG_val<BaseStrokeOffset,double>(
         "pos",
         &BaseStrokeOffset::pos_);
      *_bso_tags += new TAG_val<BaseStrokeOffset,double>(
         "len",
         &BaseStrokeOffset::len_);
      *_bso_tags += new TAG_val<BaseStrokeOffset,double>(
         "press",
         &BaseStrokeOffset::press_);
      *_bso_tags += new TAG_val<BaseStrokeOffset,int>(
         "type",
         &BaseStrokeOffset::type_);
   }

   return *_bso_tags;
}

/*****************************************************************
 * BaseStrokeOffsetLIST
 *****************************************************************/

static int foo2 = DECODER_ADD(BaseStrokeOffsetLIST);

TAGlist*       BaseStrokeOffsetLIST::_bsol_tags = 0;

////////////////////////////////////////////////////////////////////////////////
// BaseStrokeOffsetLIST Methods
////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////
// tags()
/////////////////////////////////////

CTAGlist &
BaseStrokeOffsetLIST::tags() const
{
   if (!_bsol_tags) {
      _bsol_tags = new TAGlist;
      *_bsol_tags += new TAG_val<BaseStrokeOffsetLIST,double>(
         "pix_len",
         &BaseStrokeOffsetLIST::pix_len_);
      *_bsol_tags += new TAG_val<BaseStrokeOffsetLIST,int>(
         "replicate",
         &BaseStrokeOffsetLIST::replicate_);
      *_bsol_tags += new TAG_val<BaseStrokeOffsetLIST,int>(
         "hangover",
         &BaseStrokeOffsetLIST::hangover_);
      *_bsol_tags += new TAG_val<BaseStrokeOffsetLIST,int>(
         "manual_t",
         &BaseStrokeOffsetLIST::manual_t_);

      *_bsol_tags += new TAG_meth<BaseStrokeOffsetLIST>(
         "offset",
         &BaseStrokeOffsetLIST::put_offsets,
         &BaseStrokeOffsetLIST::get_offset,
         1);

      *_bsol_tags += new TAG_meth<BaseStrokeOffsetLIST>(
         "offsets",
         &BaseStrokeOffsetLIST::put_all_offsets,
         &BaseStrokeOffsetLIST::get_all_offsets,
         1);

   }

   return *_bsol_tags;
}

//XXX - Deprecated version... Just load, but don't save.

/////////////////////////////////////
// get_offset()
/////////////////////////////////////
// Gets one offset at a time
// Keeping them nicely separated
// will help preserve IO format
// even when we much with the fields
// in the BaseStrokeOffset
void
BaseStrokeOffsetLIST::get_offset(TAGformat &d)
{
   //err_mesg(ERR_LEV_SPAM, "BaseStrokeOffsetLIST::get_offset()");

   //Grab the class name... should be BaseStrokeOffset
   str_ptr str;
   *d >> str;      

   if ((str != BaseStrokeOffset::static_name())) {
      // XXX - should throw away stuff from unknown obj
      err_mesg(ERR_LEV_ERROR, 
         "BaseStrokeOffsetLIST::get_offset() - Error!! 'Not BaseStrokeOffset': '%s'...",
           **str);
      return;
   }

   BaseStrokeOffset o;
   o.decode(*d);
   add(o);
}

/////////////////////////////////////
// put_offsets()
/////////////////////////////////////
// Dumps all of the offsets at once
void
BaseStrokeOffsetLIST::put_offsets(TAGformat &d) const
{
   //err_mesg(ERR_LEV_SPAM, "BaseStrokeOffsetLIST::put_offsets()");
/*    
   int i;

   for (i=0; i<num(); i++)
      {
         d.id();
         ((*this)[i]).format(*d);
         d.end_id();
      }
*/
}



//XXX - New version...

/////////////////////////////////////
// get_all_offsets()
/////////////////////////////////////
void
BaseStrokeOffsetLIST::get_all_offsets(TAGformat &d)
{
   //err_mesg(ERR_LEV_SPAM, "BaseStrokeOffsetLIST::get_all_offsets()");
   
   int i;

   ARRAY<BaseStrokeOffset> os;

   *d >> os;

   for (i=0; i<os.num(); i++)
   {
      add(os[i]);
   }
}

//This implements operator>> to make ARRAY::operator>> work...

STDdstream  &operator>>(STDdstream &ds, BaseStrokeOffset &o)
{
   //Grab the class name... should be BaseStrokeOffset
   str_ptr str;
   ds >> str;      

   if ((str != BaseStrokeOffset::static_name())) 
   {
      // XXX - should throw away stuff from unknown obj
      err_mesg(ERR_LEV_ERROR, 
         "BaseStrokeOffset::operator>>() - Error!! Not 'BaseStrokeOffset': '%s'...",
           **str);
   }
   else
   {
      o.decode(ds);
   }
   return ds;
}


/////////////////////////////////////
// put_all_offsets()
/////////////////////////////////////
// Dumps all of the offsets at once
void
BaseStrokeOffsetLIST::put_all_offsets(TAGformat &d) const
{
   //err_mesg(ERR_LEV_SPAM, "BaseStrokeOffsetLIST::put_all_offsets()");
   int i;
   
   ARRAY<BaseStrokeOffset> os;

   for (i=0; i<num(); i++)
   {
      os += ((*this)[i]);
   }

   d.id();
   *d << os;
   d.end_id();

}

/////////////////////////////////////
// copy()
/////////////////////////////////////
void
BaseStrokeOffsetLIST::copy(CBaseStrokeOffsetLIST& o)
{
   //Clear the offsets
   clear();

   //Copy the fields
   _pix_len =     o._pix_len;
   _replicate =   o._replicate;
   _hangover =    o._hangover;
   _manual_t =    o._manual_t;
   //Copy the array of offsets
   for (int i=0; i<o.num(); i++) 
      add(o[i]);

}
/////////////////////////////////////
// fetch
/////////////////////////////////////
void
BaseStrokeOffsetLIST::fetch(int i, BaseStrokeOffset& o) 
{
   // XXX - We Should cache this stuff
   if (!_replicate)
   {
      o = _array[i];
      return;
   }

   // fetch num is number of offset samples to be applied
   // along the length of the base stroke. here we return
   // offset number i between 0 and fetch num.

   //assert(i >= 0 && i < fetch_num());
   assert(_num > 0);

   //Grab the right offset
   o = _array[i % _num];

   //Now adjust it's t value appropriately
   if (!_manual_t)
   {
      double frac = _fetch_stretch * _pix_len / _fetch_len;

      // integer division, drop remainder
      o._pos += (double)((int)(i / _num)) - _fetch_phase; 
      o._pos *= frac;
   }
   else
   {
      o._pos += floor(_fetch_lower_t-_fetch_phase) + ((int)(i / _num)) + _fetch_phase;
   }
}

/////////////////////////////////////
// fetch_num
/////////////////////////////////////
int
BaseStrokeOffsetLIST::fetch_num()  
{
   //XXX - We should cache this stuff

   if (!_replicate)
      return num();

   if (!_manual_t)
   {
      assert(_pix_len);
      assert(_fetch_len);
      assert(_fetch_stretch);
      assert(_fetch_phase >= 0.0);
      assert(_fetch_phase <= 1.0);

      int factor = (int)ceil(_fetch_len / (_fetch_stretch * _pix_len));

      if (_fetch_phase) factor++;

      return num()*factor;
   }
   else
   {
      //return num()*(int)fabs(floor(_fetch_lower_t) - ceil(_fetch_upper_t));
      return num()*(int)fabs(floor(_fetch_lower_t-_fetch_phase) - 
                              ceil(_fetch_upper_t-_fetch_phase)   );
   }
}

/////////////////////////////////////
// get_at_t
/////////////////////////////////////
void
BaseStrokeOffsetLIST::get_at_t(double t, BaseStrokeOffset& o) const
{ 

   // A hack method that'll likely vanish one day.
   // Interpolates and returns the offset
   // at t, assuming that offsets range in t
   // from 0 to 1 without breaks or doubling
   // back.

   assert(num() > 1);
   assert(t >= 0.0);
   assert(t <= 1.0);
   assert(first()._pos == 0.0);
   assert(last()._pos  == 1.0);
   assert(first()._type == BaseStrokeOffset::OFFSET_TYPE_BEGIN);
   assert(last()._type  == BaseStrokeOffset::OFFSET_TYPE_END);
   
   if (t == 0.0)
   {
      o = first();
   }
   else if (t == 1.0)
   {
      o = last();
   }
   else
   {
      int i=1;
      while (t > (*this)[i]._pos) i++;

      BaseStrokeOffset &oi_1 = (*this)[i-1];
      BaseStrokeOffset &oi   = (*this)[i];
      double frac = (t - oi_1._pos)/(oi._pos - oi_1._pos);

      o._pos = t;
      o._len =    oi_1._len *    (1.0 - frac) +    oi._len *   (frac);
      o._press =  oi_1._press *  (1.0 - frac) +    oi._press * (frac);
      o._type = BaseStrokeOffset::OFFSET_TYPE_MIDDLE;

   }


}


/*****************************************************************
 * BaseStroke
 *****************************************************************/

#define  BASE_STROKE_DEFAULT_PIX_RES         3.5
#define  BASE_STROKE_DEFAULT_USE_DEPTH       false

#define  BASE_STROKE_DEFAULT_MIN_T           0.0f
#define  BASE_STROKE_DEFAULT_MAX_T           1.0f
#define  BASE_STROKE_DEFAULT_GEN_T           true

#define  BASE_STROKE_DEFAULT_COLOR           COLOR::black
#define  BASE_STROKE_DEFAULT_ALPHA           1.0f
#define  BASE_STROKE_DEFAULT_WIDTH           7.0f
#define  BASE_STROKE_DEFAULT_FADE            .0f
#define  BASE_STROKE_DEFAULT_HALO            0.0f
#define  BASE_STROKE_DEFAULT_TAPER           40.0f
#define  BASE_STROKE_DEFAULT_FLARE           0.2f
#define  BASE_STROKE_DEFAULT_AFLARE          0.0f
#define  BASE_STROKE_DEFAULT_ANGLE           0.0f
#define  BASE_STROKE_DEFAULT_CONTRAST        0.5f
#define  BASE_STROKE_DEFAULT_BRIGHTNESS      0.5f
#define  BASE_STROKE_DEFAULT_OFFSET_STRETCH  1.0f
#define  BASE_STROKE_DEFAULT_OFFSET_PHASE    0.0f
#define  BASE_STROKE_DEFAULT_TEX             NULL
#define  BASE_STROKE_DEFAULT_TEX_FILE        NULL_STR
#define  BASE_STROKE_DEFAULT_PAPER           NULL
#define  BASE_STROKE_DEFAULT_PAPER_FILE      NULL_STR

float PIX_RES = (float)Config::get_var_dbl("PIX_RES",BASE_STROKE_DEFAULT_PIX_RES,true);

/////////////////////////////////////
// Static Variable Initialization
/////////////////////////////////////

static int foo = DECODER_ADD(BaseStroke);

TAGlist*        BaseStroke::_bs_tags = 0;
unsigned int    BaseStroke::_stamp = 0;
int             BaseStroke::_strokes_drawn = 0;
float           BaseStroke::_scale = 0;
float           BaseStroke::_max_x = 0;
float           BaseStroke::_max_y = 0;
Wpt             BaseStroke::_cam;
Wvec            BaseStroke::_cam_at_v;
bool            BaseStroke::_debug = Config::get_var_bool("DEBUG_BASE_STROKES",false,true);
bool            BaseStroke::_repair = Config::get_var_bool("BASE_STROKE_REPAIR_INTERSECTIONS",false,true);

LIST<str_ptr>*    BaseStroke::_stroke_texture_names = 0;
LIST<TEXTUREptr>* BaseStroke::_stroke_texture_ptrs = 0;
LIST<str_ptr>*    BaseStroke::_stroke_texture_remap_orig_names = 0;
LIST<str_ptr>*    BaseStroke::_stroke_texture_remap_new_names = 0;

////////////////////////////////////////////////////////////////////////////////
// BaseStroke Methods
////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////
// Constructor
/////////////////////////////////////

BaseStroke::BaseStroke() 
{
   _color =          BASE_STROKE_DEFAULT_COLOR;
   _alpha =          BASE_STROKE_DEFAULT_ALPHA;
   _width =          BASE_STROKE_DEFAULT_WIDTH;
   _fade =           BASE_STROKE_DEFAULT_FADE;
   _halo =           BASE_STROKE_DEFAULT_HALO;
   _taper =          BASE_STROKE_DEFAULT_TAPER;
   _flare =          BASE_STROKE_DEFAULT_FLARE;
   _aflare =         BASE_STROKE_DEFAULT_AFLARE;
   _angle =          BASE_STROKE_DEFAULT_ANGLE;
   _contrast =       BASE_STROKE_DEFAULT_CONTRAST;
   _brightness =     BASE_STROKE_DEFAULT_BRIGHTNESS;
   _offset_stretch = BASE_STROKE_DEFAULT_OFFSET_STRETCH;
   _offset_phase =   BASE_STROKE_DEFAULT_OFFSET_PHASE;
   _pix_res =        PIX_RES;
   _use_depth =      BASE_STROKE_DEFAULT_USE_DEPTH;
   _tex =            BASE_STROKE_DEFAULT_TEX;
   _tex_file =       BASE_STROKE_DEFAULT_TEX_FILE;
   _paper =          BASE_STROKE_DEFAULT_PAPER;
   _paper_file =     BASE_STROKE_DEFAULT_PAPER_FILE;

   _vis_type =       VIS_TYPE_SCREEN;

   _min_t =          BASE_STROKE_DEFAULT_MIN_T;
   _max_t =          BASE_STROKE_DEFAULT_MAX_T;
   _gen_t =          BASE_STROKE_DEFAULT_GEN_T;

   _dirty =       true;
   _ndc_length =  -1.0;

   _offsets = NULL;
   _propagate_offsets = false;  
   // by default, do not copy offsets
   // automatically when this stroke is copied

   _draw_verts.set_proto(new BaseStrokeVertexData);
   _refine_verts.set_proto(new BaseStrokeVertexData);
   _verts.set_proto(new BaseStrokeVertexData);

   _highlight_color = COLOR::red;
   _is_highlighted = false;
   _press_vary_width = 0;
   _press_vary_alpha = 0;

   _offset_lower_t = 0.0;
   _offset_upper_t = 1.0;

   _overdraw = false;
   _overdraw_stroke = NULL;

   _use_paper = 1;
}


///////////////////////////////////// 
// Destructor
/////////////////////////////////////

BaseStroke::~BaseStroke()
{
   clear();

   if (_overdraw_stroke) delete _overdraw_stroke;
   _overdraw_stroke = NULL;
}

/////////////////////////////////////
// tags()
/////////////////////////////////////

CTAGlist &
BaseStroke::tags() const
{
   if (!_bs_tags) {
      _bs_tags = new TAGlist;
      *_bs_tags += new TAG_val<BaseStroke,COLOR>(
         "color",
         &BaseStroke::color_);
      *_bs_tags += new TAG_val<BaseStroke,float>(
         "alpha",
         &BaseStroke::alpha_);
      *_bs_tags += new TAG_val<BaseStroke,float>(
         "width",
         &BaseStroke::width_);
      *_bs_tags += new TAG_val<BaseStroke,float>(
         "halo",
         &BaseStroke::halo_);
      *_bs_tags += new TAG_val<BaseStroke,float>(
         "taper",
         &BaseStroke::taper_);
      *_bs_tags += new TAG_val<BaseStroke,float>(
         "flare",
         &BaseStroke::flare_);
      *_bs_tags += new TAG_val<BaseStroke,float>(
         "aflare",
         &BaseStroke::aflare_);
      *_bs_tags += new TAG_val<BaseStroke,float>(
         "fade",
         &BaseStroke::fade_);
      *_bs_tags += new TAG_val<BaseStroke,float>(
         "angle",
         &BaseStroke::angle_);
      *_bs_tags += new TAG_val<BaseStroke,float>(
         "contrast",
         &BaseStroke::contrast_);
      *_bs_tags += new TAG_val<BaseStroke,float>(
         "brightness",
         &BaseStroke::brightness_);
      *_bs_tags += new TAG_val<BaseStroke,float>(
         "offset_stretch",
         &BaseStroke::offset_stretch_);
      *_bs_tags += new TAG_val<BaseStroke,float>(
         "offset_phase",
         &BaseStroke::offset_phase_);

      *_bs_tags += new TAG_val<BaseStroke,int>(
         "press_vary_width",
         &BaseStroke::press_vary_width_);

      *_bs_tags += new TAG_val<BaseStroke,int>(
         "press_vary_alpha",
         &BaseStroke::press_vary_alpha_);
        
      *_bs_tags += new TAG_meth<BaseStroke>(
         "stroke_texture_file",
         &BaseStroke::put_texture_file,
         &BaseStroke::get_texture_file,
         1);

      if(_use_paper){
         *_bs_tags += new TAG_meth<BaseStroke>(
         "paper_texture_file",
         &BaseStroke::put_paper_file,
         &BaseStroke::get_paper_file,
         1);
      }

      *_bs_tags += new TAG_meth<BaseStroke>(
         "offsets",
         &BaseStroke::put_offsets,
         &BaseStroke::get_offsets,
         1);

   }
   return *_bs_tags;
}

/////////////////////////////////////
// put_texture_file()
/////////////////////////////////////
void
BaseStroke::put_texture_file(TAGformat &d) const
{
   //err_mesg(ERR_LEV_SPAM, "BaseStroke::put_texture_file()");
        
   //XXX - May need something to handle filenames with spaces

   d.id();
   if (_tex_file == NULL_STR)
      {
         // << "BaseStroke::put_texture_file() - Wrote NULL string.\n";
         *d << "NULL_STR";
         *d << " ";
      }
   else
      {
         if (!_tex_file.contains(Config::JOT_ROOT()))
            {
               //err_mesg(ERR_LEV_SPAM, "BaseStroke::put_texture_file() - Wrote NULL string.");
               *d << "NULL_STR";
               *d << " ";
            }
         else
            {
               //Here we strip off JOT_ROOT
               str_ptr str;
               int i;
               for (i=Config::JOT_ROOT().len(); i<(int)_tex_file.len(); i++)
                  str = str + str_ptr(_tex_file[i]);
               *d << **str;
               *d << " ";
               //err_mesg(ERR_LEV_SPAM, "BaseStroke::put_texture_file() - Wrote string: '%s'.", **str);
            }
      }
   d.end_id();
}

/////////////////////////////////////
// get_texture_file()
/////////////////////////////////////
void
BaseStroke::get_texture_file(TAGformat &d)
{
   //err_mesg(ERR_LEV_SPAM, "BaseStroke::get_texture_file()");

   //XXX - May need something to handle filenames with spaces

   str_ptr str, tex, space;
   *d >> str;      
   if (!(*d).ascii()) *d >> space; 

   if (str == "NULL_STR") 
      {
         tex = NULL_STR;
         //err_mesg(ERR_LEV_SPAM, "BaseStroke::get_texture_file() - Loaded NULL string.");
      }
   else
      {
      //Here we prepend JOT_ROOT
      tex = Config::JOT_ROOT() + str;
         //err_mesg(ERR_LEV_SPAM, "BaseStroke::get_texture_file() - Loaded string: '%s'.", **str);
      }
   set_texture(tex);

}


/////////////////////////////////////
// put_paper_file()
/////////////////////////////////////
void
BaseStroke::put_paper_file(TAGformat &d) const
{
   //err_mesg(ERR_LEV_SPAM, "BaseStroke::put_paper_file()");
        
   //XXX - May need something to handle filenames with spaces

   d.id();
   if (_paper_file == NULL_STR)
   {
      //err_mesg(ERR_LEV_SPAM, "BaseStroke::put_paper_file() - Wrote NULL string.");
      *d << "NULL_STR";
      *d << " ";
   }
   else
   {
      if (!_paper_file.contains(Config::JOT_ROOT()))
      {
         //err_mesg(ERR_LEV_SPAM, "BaseStroke::put_paper_file() - Wrote NULL string.");
         *d << "NULL_STR";
         *d << " ";
      }
      else
      {
         //Here we strip off JOT_ROOT
         str_ptr str;
/*
         int i;
         for (i=Config::JOT_ROOT().len(); i<(int)_paper_file.len(); i++)
            str = str + str_ptr(_paper_file[i]);
*/
         //JOT_ROOT should be prefix
         assert(strstr(**_paper_file,**Config::JOT_ROOT()) == **_paper_file );

         //Now strip it off...
         str = &((**_paper_file)[Config::JOT_ROOT().len()]);
         
         *d << **str;
         *d << " ";
         //err_mesg(ERR_LEV_SPAM, "BaseStroke::put_paper_file() - Wrote string: '%s'.", **str);
      }
   }
   d.end_id();
}

/////////////////////////////////////
// get_paper_file()
/////////////////////////////////////
void
BaseStroke::get_paper_file(TAGformat &d)
{
   //err_mesg(ERR_LEV_SPAM, "BaseStroke::get_paper_file()");

   //XXX - May need something to handle filenames with spaces

   str_ptr str, paper, space;
   *d >> str;      
   if (!(*d).ascii()) *d >> space; 

   if (str == "NULL_STR") 
      {
         paper = NULL_STR;
         //err_mesg(ERR_LEV_SPAM, "BaseStroke::get_paper_file() - Loaded NULL string.");
      }
   else
      {
         //Here we prepend JOT_ROOT
         paper = Config::JOT_ROOT() + str;
         //err_mesg(ERR_LEV_SPAM, "BaseStroke::get_paper_file() - Loaded string: '%s'.", **paper);
      }
   set_paper(paper);

}

/////////////////////////////////////
// put_offsets()
/////////////////////////////////////
void
BaseStroke::put_offsets(TAGformat &d) const
{
   //err_mesg(ERR_LEV_SPAM, "BaseStroke::put_offsets()");

   if (_offsets)
      {
         d.id();
         _offsets->format(*d);
         d.end_id();
      }
}

/////////////////////////////////////
// get_offsets()
/////////////////////////////////////
void
BaseStroke::get_offsets(TAGformat &d)
{
   //err_mesg(ERR_LEV_SPAM, "BaseStroke::get_offsets()");

   //Grab the class name... should be BaseStrokeOffset
   str_ptr str;
   *d >> str;      

   if ((str != BaseStrokeOffsetLIST::static_name())) 
   {
      // XXX - should throw away stuff from unknown obj
      err_mesg(ERR_LEV_ERROR, "BaseStroke::get_offsets() - Not BaseStrokeOffsetLIST: '%s'!!", **str);
      return;
   }

   BaseStrokeOffsetLISTptr o = new BaseStrokeOffsetLIST;
   assert(o);
   o->decode(*d);

   _offsets = o;

}
/////////////////////////////////////
// set_texture()
/////////////////////////////////////
//
// User sets texture via it's filename
// Setting the name as NULL_STR (or a bad filename) 
// will disable texturing (enable antialiasing)
//
/////////////////////////////////////

void
BaseStroke::set_texture(str_ptr tf)
{
   int ind;

   if (!_stroke_texture_names)
   {
      _stroke_texture_names = new LIST<str_ptr>; assert(_stroke_texture_names);
      _stroke_texture_ptrs = new LIST<TEXTUREptr>; assert(_stroke_texture_ptrs);

      _stroke_texture_remap_orig_names = new LIST<str_ptr>; assert(_stroke_texture_remap_orig_names);
      _stroke_texture_remap_new_names = new LIST<str_ptr>; assert(_stroke_texture_remap_new_names);

      int i = 0;
      while (stroke_remap_fnames[i][0] != NULL)
      {
         _stroke_texture_remap_orig_names->add(Config::JOT_ROOT() + stroke_remap_base + stroke_remap_fnames[i][0]);
         _stroke_texture_remap_new_names->add(Config::JOT_ROOT() + stroke_remap_base + stroke_remap_fnames[i][1]);
         i++;
      }
   }

   if (_tex_file == tf)
   {
      //No change
   }
   else if (tf == NULL_STR)
   {
      _tex = NULL;
      _tex_file = NULL_STR;
   }
   else if ((ind = _stroke_texture_names->get_index(tf)) != BAD_IND)
   {
      //Finding original name in cache...

      //If its a failed texture...
      if ((*_stroke_texture_ptrs)[ind] == NULL)
      {
         //...see if it was remapped...
         int ii = _stroke_texture_remap_orig_names->get_index(tf);
         //...and change to looking up the remapped name            
         if (ii != BAD_IND)
         {
            str_ptr old_tf = tf;
            tf = (*_stroke_texture_remap_new_names)[ii];

            ind = _stroke_texture_names->get_index(tf);

            err_mesg(ERR_LEV_SPAM, 
               "BaseStroke::set_texture() - Previously remapped --===<<[[{{ (%s) ---> (%s) }}]]>>===--", 
                  **old_tf, **tf );
         }
      }

      //Now see if the final name yields a good texture...
      if ((*_stroke_texture_ptrs)[ind] != NULL)
      {
         _tex = (*_stroke_texture_ptrs)[ind];
         _tex_file = tf;
         //err_mesg(ERR_LEV_SPAM, "BaseStroke::set_texture() - Using cached copy of texture.");
      }
      else
      {
         err_mesg(ERR_LEV_INFO, "BaseStroke::set_texture() - **ERROR** Previous caching failure: '%s'...", **tf);
         _tex = NULL;
         _tex_file = NULL_STR;
      }
   }
   //Haven't seen this name before...
   else
   {
      err_mesg(ERR_LEV_SPAM, "BaseStroke::set_texture() - Not in cache...");
      
      Image i(**tf);

      //Can't load the texture?
      if (i.empty())
      {
         //...check for a remapped file...
         int ii = _stroke_texture_remap_orig_names->get_index(tf);

         //...and use that name instead....
         if (ii != BAD_IND)
         {
            //...but also indicate that the original name is bad...

            _stroke_texture_names->add(tf);
            _stroke_texture_ptrs->add(NULL);

            str_ptr old_tf = tf;
            tf = (*_stroke_texture_remap_new_names)[ii];

            err_mesg(ERR_LEV_ERROR, 
               "BaseStroke::set_texture() - Remapping --===<<[[{{ (%s) ---> (%s) }}]]>>===--", 
                  **old_tf, **tf );

            i.load_file(**tf);
         }
      }

      //If the final name loads, store the cached texture...
      if (!i.empty())
	   {
         TEXTUREglptr t = new TEXTUREgl();

         t->set_save_img(true);
         t->set_image(i.copy(),i.width(),i.height(),i.bpp());

         _stroke_texture_names->add(tf);
         _stroke_texture_ptrs->add(t);

         err_mesg(ERR_LEV_INFO, "BaseStroke::set_texture() - Cached: (w=%d h=%d bpp=%u) %s",
            i.width(), i.height(), i.bpp(), **tf);;

         _tex = t;
         _tex_file = tf;
	   }
      //Otherwise insert a failed NULL
	   else
	   {
         err_mesg(ERR_LEV_ERROR, "BaseStroke::set_texture() - *****ERROR***** Failed loading to cache: '%s'...", **tf);
         
         _stroke_texture_names->add(tf);
         _stroke_texture_ptrs->add(NULL);

         _tex = NULL;
         _tex_file = NULL_STR;
	   }
   }   
}

/////////////////////////////////////
//
// This version sets the texture via
// a TEXTUREptr, so the filename must
// also be provided (though it isn't
// verified in any way)
//
//////////////////////////////////////
void
BaseStroke::set_texture(TEXTUREptr tp, str_ptr tn)
{
   _tex = tp;
   _tex_file = tn;
}

/////////////////////////////////////
// set_paper()
/////////////////////////////////////
//
// User sets paper via it's filename
// Setting the name as NULL_STR (or a bad filename) 
// will disable paper texturing 
//
/////////////////////////////////////
void
BaseStroke::set_paper(str_ptr tf)
{
   if (_paper_file == tf) return;

   str_ptr ret_tf;
   TEXTUREptr t;

   if (tf == NULL_STR)
   {
      _paper_file = NULL_STR;
      _paper = NULL;
   }
   else if ((t = PaperEffect::get_texture(tf, ret_tf)) != 0)
   {
      _paper_file = ret_tf;
      _paper = t;
   }
   else
   {
      _paper_file = NULL_STR;
      _paper = NULL;
   }   
   
}

/////////////////////////////////////
//
// This version sets the texture via
// a TEXTUREptr, so the filename must
// also be provided (though it isn't
// verified in any way)
//
//////////////////////////////////////
void
BaseStroke::set_paper(TEXTUREptr tp, str_ptr tn)
{
   _paper = tp;
   _paper_file = tn;
}

/////////////////////////////////////
// copy()
/////////////////////////////////////
BaseStroke* 
BaseStroke::copy() const
{

   BaseStroke *s =  new BaseStroke;
   assert(s);
   s->copy(*this);
   return s;

}

/////////////////////////////////////
// copy()
/////////////////////////////////////
void        
BaseStroke::copy(CBaseStroke& s)
{

   _color            = s._color;
   _width            = s._width;
   _alpha            = s._alpha;
   _fade             = s._fade;
   _halo             = s._halo;  
   _taper            = s._taper;
   _flare            = s._flare;
   _aflare           = s._aflare;
   _angle            = s._angle;
   _contrast         = s._contrast;
   _brightness       = s._brightness;
   
   //_offset_stretch   = s._offset_stretch; 
   _offset_phase     = s._offset_phase; 

   _tex_file         = s._tex_file;    
   _paper_file       = s._paper_file;     

   _tex              = s._tex;
   _paper            = s._paper;       
   _pix_res          = s._pix_res;
   _use_depth        = s._use_depth;
   _use_paper        = s._use_paper;

   _press_vary_width = s._press_vary_width;
   _press_vary_alpha = s._press_vary_alpha;

   // Copying params from base class
   // into subclass shouldn't muck with this
   //_vis_type   = s._vis_type;

   if (s._propagate_offsets)
   {
      if (s._offsets == NULL)
      {
         _offsets = NULL;
      }
      else
      {
         _offsets = new BaseStrokeOffsetLIST;

         _offsets->copy(*s._offsets);
      }
   }

   set_dirty();

}


/////////////////////////////////////
// clear()
/////////////////////////////////////
void
BaseStroke::clear()
{
   set_dirty();
   _verts.clear();
}

/////////////////////////////////////
// draw_start()
/////////////////////////////////////
void
BaseStroke::draw_start()
{
   // Push affected state:
   glPushAttrib(
      GL_CURRENT_BIT            |
      GL_ENABLE_BIT             |
      GL_COLOR_BUFFER_BIT       |
      GL_TEXTURE_BIT
      );

   glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

   // Set state for drawing strokes:
   glDisable(GL_LIGHTING);												// GL_ENABLE_BIT
   glDisable(GL_CULL_FACE);											// GL_ENABLE_BIT
   if (!_use_depth)  
      glDisable(GL_DEPTH_TEST);										// GL_ENABLE_BIT
   glEnable(GL_BLEND);													// GL_ENABLE_BIT
	if (_use_paper && PaperEffect::is_alpha_premult())
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);				// GL_COLOR_BUFFER_BIT
	else
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);		// GL_COLOR_BUFFER_BIT

   // Enable or disable texturing:
   if (_tex && (!_debug))
   {
      glEnable(GL_TEXTURE_2D);  // GL_ENABLE_BIT
      _tex->apply_texture();    // GL_TEXTURE_BIT
   }
   else
   {
      glDisable(GL_TEXTURE_2D); // GL_ENABLE_BIT
   }

   if (!_debug)
   {
      glEnableClientState(GL_VERTEX_ARRAY);                    //GL_CLIENT_VERTEX_ARRAY_BIT)
      glEnableClientState(GL_COLOR_ARRAY);                     //GL_CLIENT_VERTEX_ARRAY_BIT)
      if (_tex) glEnableClientState(GL_TEXTURE_COORD_ARRAY);   //GL_CLIENT_VERTEX_ARRAY_BIT)

      if (GLExtensions::gl_arb_multitexture_supported())
      {
#ifdef GL_ARB_multitexture
         glClientActiveTextureARB(GL_TEXTURE1_ARB); 
         glEnableClientState(GL_TEXTURE_COORD_ARRAY);          //GL_CLIENT_VERTEX_ARRAY_BIT)
         glClientActiveTextureARB(GL_TEXTURE0_ARB); 
#endif
      }
   }

   // Set projection and modelview matrices for drawing in NDC:
   glMatrixMode(GL_PROJECTION);
   glPushMatrix();
   glLoadMatrixd(VIEW::peek()->ndc_proj().transpose().matrix());

   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
   glLoadIdentity();

   // Cache view related info:
   if (_stamp != VIEW::stamp()) 
   {
      _stamp = VIEW::stamp();
      int w, h;
      VIEW_SIZE(w, h);
      _scale = (float)VIEW::pix_to_ndc_scale();
      
      _max_x = w*_scale/2;
      _max_y = h*_scale/2;
      
      _cam = VIEW::peek_cam()->data()->from();
      _cam_at_v = VIEW::peek_cam()->data()->at_v();
      
      _strokes_drawn = 0;
   }

   if (!_debug && _use_paper) 
      PaperEffect::begin_paper_effect(_paper, _contrast, _brightness);
}

/////////////////////////////////////
// draw_end()
/////////////////////////////////////
void
BaseStroke::draw_end()
{
   if (!_debug && _use_paper) 
      PaperEffect::end_paper_effect(_paper);

   glMatrixMode(GL_PROJECTION);
   glPopMatrix();

   glMatrixMode(GL_MODELVIEW);
   glPopMatrix();

   glPopClientAttrib();

   glPopAttrib();
}

/////////////////////////////////////
// draw()
/////////////////////////////////////
int
BaseStroke::draw(CVIEWptr &v)
{
   if (_debug) return draw_debug(v);
   
   int tris = 0;

   if (_verts.num() < 2) return 0;

   _strokes_drawn++;

   update();

   if (_draw_verts.num() < 2) return 0;

   tris = draw_body();

   if (_overdraw)
   {
      assert(_overdraw_stroke);
      draw_end();
      _overdraw_stroke->draw_start();
      tris += _overdraw_stroke->draw(v);
      _overdraw_stroke->draw_end();
      draw_start();
      
   }
   return tris;
}

/////////////////////////////////////
// draw_debug()
/////////////////////////////////////
int
BaseStroke::draw_debug(CVIEWptr &)
{
   
   int tris = 0;

   if (_verts.num() < 2) return 0;

   _strokes_drawn++;

   // changed the draw_debug for checking ImgLineTexture
   // --yunjin
   //
   /*update();

   if (_draw_verts.num() < 2) return 0;

   tris += draw_dots();*/

   tris += draw_circles();
   
   return tris;
}

/////////////////////////////////////
// draw_circles()
/////////////////////////////////////
int
BaseStroke::draw_circles()
{
  //cerr<<"draw_circles"<<endl;
   int i;

   GL_COL( _color, _alpha );

   GLUquadricObj *qobj;
   qobj = gluNewQuadric();
   gluQuadricDrawStyle(qobj, GLU_LINE);
   gluQuadricNormals(qobj, GLU_NONE);

   glColor3f(0.0, 0.0, 0.0);
   glBegin(GL_LINE_STRIP);
   for (i=0; i < _verts.num(); i++) {
      BaseStrokeVertex *v = &(_verts[i]);

      /*glPushMatrix();
      glTranslatef(v->_base_loc[0], v->_base_loc[1], v->_base_loc[2]);
      gluDisk(qobj, 0, v->_width*VIEW::pix_to_ndc_scale(), 5, 1);
      glPopMatrix();*/
      glVertex3dv(v->_base_loc.data());
   }
   glEnd();

   glColor3f(1.0, 0.0, 0.0);
   glBegin(GL_LINES);

   mlib::NDCZpt p2;
   for (i=0; i < _verts.num(); i++) {
      BaseStrokeVertex *v = &(_verts[i]);

      glVertex3dv(v->_base_loc.data());
      p2 = v->_base_loc + v->_dir*v->_width*VIEW::pix_to_ndc_scale();
      glVertex3dv(p2.data());
   }
   glEnd();

   return 0;
}

/////////////////////////////////////
// draw_dots()
/////////////////////////////////////
int
BaseStroke::draw_dots()
{
   NDCZpt q, p;
   double w = _width * _scale / 2.0f;
   double l0, ln, u, du;
   bool transition;
   int i;

   COLOR color;
   float alpha = 0.8f;

   l0 = _draw_verts[0]._l;
   ln = _draw_verts[_draw_verts.num()-1]._l;
   du = 1.0/(ln - l0);

   if (_tex) glDisable(GL_TEXTURE_2D);

   // Smoothed points seems to fail on
   // most cards, but we'll try anyway...
   //glPushAttrib(GL_POINT_BIT | GL_HINT_BIT);
   //glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
   //glEnable(GL_POINT_SMOOTH);

   glPointSize(2.0);
   color.set( 0.25, 0.25, 1.0);

   glBegin(GL_POINTS);

   GL_COL( color, alpha );

   for (i=0; i < _verts.num(); i++) {
      BaseStrokeVertex *v = &(_verts[i]);

      if (v->_vis_state == BaseStrokeVertex::VIS_STATE_CLIPPED)
         continue;

      if (v->_vis_state == BaseStrokeVertex::VIS_STATE_TRANSITION) {
         v = v->_refined_vertex;
         /*XXX*/assert(v);
      }

      glVertex3dv(v->_base_loc.data());
   }

   glEnd();
   
   glPointSize(1.0);
   glBegin(GL_POINTS);

   for (i=0; i < _draw_verts.num(); i++) {
      const BaseStrokeVertex& v = _draw_verts[i];
      if (_offsets == NULL)
         u = (v._l-l0) * du;
      else
         u=0.0;

      transition = (v._vis_state == BaseStrokeVertex::VIS_STATE_TRANSITION);
      if (transition)
         color.set(0.0, 0.0, 0.0);
      else
         color.set( u, 1.0-u, 0.0);

      GL_COL( color, alpha );
      q = v._loc + v._dir * ( v._width * w );
      glVertex3dv(q.data());

      if (!transition)
         color.set(0.0, 0.0, 0.0);

      GL_COL( color, alpha );
      glVertex3dv(v._loc.data());

      if (!transition)
         color.set( u, 0.0, 1.0-u);

      GL_COL( color, alpha );
      p = v._loc + v._dir * ( -v._width * w );
      glVertex3dv(p.data());

   }
   glEnd();

   //glPopAttrib();

   return 0;
}

/////////////////////////////////////
// compute_vertex_arrays()
/////////////////////////////////////
void
BaseStroke::compute_vertex_arrays()
{
   _array_verts.clear();
   _array_tex_0.clear();
   _array_color.clear();
   _array_i0.clear();
   _array_i1.clear();
   _array_counts.clear();
   _array_counts_total = 0;

   int count;
   const double w = _width * _scale / 2.0;
   const double a = _alpha * ((_width < 2.0)?(_width*_width/4.0):1.0);
   COL4 col((_is_highlighted)?(_highlight_color):(_color),1.0);
   UVpt uv0(0.5, 0.0);
   UVpt uv1(0.5, 1.0);

   if (!_repair)
   {
      for (int i=0; i < _draw_verts.num(); i++) 
      {
         const BaseStrokeVertex& v = _draw_verts[i];
         const NDCZvec bar(v._dir * (v._width * w));
         col.set(a * v._alpha);
      
         const bool transition = (v._vis_state == BaseStrokeVertex::VIS_STATE_TRANSITION);
         if ((transition) && (v._trans_type == BaseStrokeVertex::TRANS_VISIBLE))
         {
            count = 0;
         }

         _array_color.add(col);
         _array_color.add(col);
         if (_tex)
         {
            // XXX - If no offsets, but there's an angle (period)
            // then do 2d texturing... Optimize this...
            if (_angle && (!_offsets))
            {
               uv1[0] = uv0[0] = v._t + _offset_phase;
            }
            _array_tex_0.add(uv0);
            _array_tex_0.add(uv1);
         }
         else
         {
            _array_i0.add(2*i);
            _array_i1.add(2*i+1);
         }
         _array_verts.add(v._loc + bar);
         _array_verts.add(v._loc - bar);

         count += 2;
         if ((transition) && (v._trans_type == BaseStrokeVertex::TRANS_CLIPPED))
         {
            _array_counts.add(count);
            _array_counts_total += count;
         }
      }
   }
   else
   {

      NDCvec   oldd;
      NDCZpt   oldv;
      NDCZpt   oldtop;
      NDCZpt   oldbot;
      bool     swap = false;

      for (int i=0; i < _draw_verts.num(); i++) 
      {
         const BaseStrokeVertex& v = _draw_verts[i];
         const NDCZvec bar(v._dir * (v._width * w));
         const bool transition = (v._vis_state == BaseStrokeVertex::VIS_STATE_TRANSITION);
         col.set(a * v._alpha);

         NDCZpt  top = v._loc + bar;
         NDCZpt  bot = v._loc - bar;

         NDCvec  d   = NDCvec(v._dir[1],-v._dir[0]);
         NDCZpt  ve  = v._loc;
   
         if (transition && (v._trans_type == BaseStrokeVertex::TRANS_VISIBLE))
         {
            count = 0;
         }
         else
         {
            const bool reversal = (d*oldd <= 0.0);
            const double t = (top-oldv)*oldd;
            const double b = (bot-oldv)*oldd;

            if (!reversal)
            {
               if ((t<gEpsZeroMath) && (b>-gEpsZeroMath))
               {
                  top = oldtop;
                  d = (bot-top).perpendicular().normalized();
                  ve = (bot+top)/2.0;
               }
               else if ((t>-gEpsZeroMath) && (b<gEpsZeroMath))
               {
                  bot = oldbot;
                  d = (bot-top).perpendicular().normalized();
                  ve = (bot+top)/2.0;
               }
               else if ((t<gEpsZeroMath) && (b<gEpsZeroMath))
               {
                  bot = oldbot;
                  top = oldtop;
                  d = oldd;
                  ve = oldv;
               }
            }
            else
            {
               if (( (t<gEpsZeroMath) && (b>-gEpsZeroMath)) || 
                   ((t>-gEpsZeroMath) && (b<gEpsZeroMath)))
               {
                  bot = oldbot;
                  top = oldtop;
                  d = oldd;
                  ve = oldv;
               }
               swap = !swap;
            }
         }

         
         if (!swap)
         {
                           _array_color.add(col); _array_color.add(col);
            if (_tex)   {  if (_angle && (!_offsets)) uv1[0] = uv0[0] = v._t; 
                           _array_tex_0.add(uv0); _array_tex_0.add(uv1);      }
            else        {     _array_i0.add(2*i);  _array_i1.add(2*i+1);      }
                           _array_verts.add(top); _array_verts.add(bot);
         }
         else
         {
                           _array_color.add(col);  _array_color.add(col);
            if (_tex)   {  if (_angle && (!_offsets)) uv1[0] = uv0[0] = v._t;
                           _array_tex_0.add(uv0);  _array_tex_0.add(uv1);     }
            else        {     _array_i0.add(2*i);     _array_i1.add(2*i+1);   }
                           _array_verts.add(bot);  _array_verts.add(top);
         }

         count += 2;

         if ((transition) && (v._trans_type == BaseStrokeVertex::TRANS_CLIPPED))
         {
            _array_counts.add(count);
            _array_counts_total += count;
         }
         else
         {
            oldd = d;
            oldv = ve;
            oldtop = top;
            oldbot = bot;
         }
      }
   }

}

/////////////////////////////////////
// init_overdraw()
/////////////////////////////////////
void
BaseStroke::init_overdraw()
{
   assert(!_overdraw_stroke);

   _overdraw_stroke = new BaseStroke(); assert(_overdraw_stroke);

   _overdraw_stroke->set_texture(OVERDRAW_STROKE_TEXTURE);
   _overdraw_stroke->set_width(  OVERDRAW_STROKE_WIDTH);
   _overdraw_stroke->set_alpha(  OVERDRAW_STROKE_ALPHA);
   _overdraw_stroke->set_flare(  OVERDRAW_STROKE_FLARE);
   _overdraw_stroke->set_taper(  OVERDRAW_STROKE_TAPER);
   _overdraw_stroke->set_fade(   OVERDRAW_STROKE_FADE);
   _overdraw_stroke->set_aflare( OVERDRAW_STROKE_AFLARE);
   _overdraw_stroke->set_halo(   OVERDRAW_STROKE_HALO);
   _overdraw_stroke->set_color(  OVERDRAW_STROKE_COLOR); 
   _overdraw_stroke->set_angle(  OVERDRAW_STROKE_ANGLE);  
   _overdraw_stroke->set_offset_phase(  OVERDRAW_STROKE_PHASE);  
}

/////////////////////////////////////
// compute_overdraw()
/////////////////////////////////////
void
BaseStroke::compute_overdraw()
{
   int i;

   if (!_overdraw) return;
 
   if (!_overdraw_stroke) init_overdraw();

   assert(_overdraw_stroke->get_offsets() == NULL);

   _overdraw_stroke->clear();


   NDCZpt_list pts(_verts.num());

   for (i=0; i < _verts.num(); i++) 
   {
      assert(_verts[i]._good == true);
      pts += _verts[i]._base_loc;
   }

   pts.update_length();
   double factor = (_scale*max(_overdraw_stroke->get_angle(),1.0f));
   _overdraw_stroke->set_max_t(pts.length()/factor);
   _overdraw_stroke->set_min_t(0);
   for (i=0; i < pts.num(); i++) 
   {  
      _overdraw_stroke->add(pts.partial_length(i)/factor, pts[i]);
   }
}

#include "geom/gl_view.H"
/////////////////////////////////////
// draw_body()
/////////////////////////////////////
int
BaseStroke::draw_body()
{
   double a = _alpha * ((_width < 2.0)?(_width*_width/4.0):1.0);

   int start,i;
GL_VIEW::print_gl_errors("BaseStroke::draw_body() [1] - ");
   glVertexPointer(3, GL_DOUBLE, 0, _array_verts.array());
   glColorPointer(4, GL_DOUBLE, 0, _array_color.array());
   if (_tex) glTexCoordPointer(2, GL_DOUBLE, 0, _array_tex_0.array());
GL_VIEW::print_gl_errors("BaseStroke::draw_body() [2] - ");
   if (GLExtensions::gl_arb_multitexture_supported())
   {
#ifdef GL_ARB_multitexture
      glClientActiveTextureARB(GL_TEXTURE1_ARB); 
      glTexCoordPointer(3, GL_DOUBLE, 0, _array_verts.array());
      glClientActiveTextureARB(GL_TEXTURE0_ARB); 
#endif
   }
GL_VIEW::print_gl_errors("BaseStroke::draw_body() [3] - ");
   if (GLExtensions::gl_ext_compiled_vertex_array_supported())
   {
#ifdef GL_EXT_compiled_vertex_array
      glLockArraysEXT(0,_array_counts_total);
#endif
   }
GL_VIEW::print_gl_errors("BaseStroke::draw_body() [4] - ");
   if (!_tex )
   {
      glEnable(GL_LINE_SMOOTH);
      // find the minimum supported line width:
      static GLfloat line_widths[2] = {0};
      if (line_widths[1] == 0) {
         // initialize just one time
         glGetFloatv(GL_LINE_WIDTH_RANGE, line_widths);      // gets min and max
      }
      // use line width a, or the minimum supported width,
      // whichever is bigger:
      glLineWidth(max(GLfloat(a), line_widths[0]));
GL_VIEW::print_gl_errors("BaseStroke::draw_body() [5] - ");
      start = 0;
      for (i=0; i< _array_counts.num(); i++)
      {
         glDrawElements(GL_LINE_STRIP, _array_counts[i]/2, GL_UNSIGNED_INT, &(_array_i0.array()[start]));
         start += _array_counts[i]/2;
      }
      start = 0;
      for (i=0; i< _array_counts.num(); i++)
      {
         glDrawElements(GL_LINE_STRIP, _array_counts[i]/2, GL_UNSIGNED_INT, &(_array_i1.array()[start]));
         start += _array_counts[i]/2;
      }
GL_VIEW::print_gl_errors("BaseStroke::draw_body() [6] - ");
      glDisable(GL_LINE_SMOOTH);
   }
GL_VIEW::print_gl_errors("BaseStroke::draw_body() [7] - ");
   // XXX - Drawing the triangles before the lines causes
   // havoc when compiled arrays are used with paper... Don't
   // ask me why.  Maybe an NVidia bug (imagine that!).
   start = 0;
   for (i=0; i< _array_counts.num(); i++)
   {
      glDrawArrays(GL_QUAD_STRIP, start, _array_counts[i]);
      start += _array_counts[i];
   }
 GL_VIEW::print_gl_errors("BaseStroke::draw_body() [8] - ");
   if (GLExtensions::gl_ext_compiled_vertex_array_supported())
   {
#ifdef GL_EXT_compiled_vertex_array
      glUnlockArraysEXT();
#endif
   }
GL_VIEW::print_gl_errors("BaseStroke::draw_body() [9] - ");
   int ret;
   ret = _array_counts_total - 2*_array_counts.num();
   assert(ret>0);
   return ret;
  
}


/////////////////////////////////////
// update()
/////////////////////////////////////
void
BaseStroke::update()
{
//    if (!_dirty) return;

   _draw_verts.clear();
   _refine_verts.clear();

   compute_length_l();

   compute_visibility();

   // Bail out if nothing's seen
   if (_vis_verts == 0) return;

   compute_refinements();

   // Dumping singular verts might leave nothing...
   if (_vis_verts == 0) return;

   compute_t();

   compute_connectivity();

   compute_resampling();

   compute_directions();

   compute_stylization();

   compute_vertex_arrays();
   
   compute_overdraw();

   _dirty = false;
}

/////////////////////////////////////
// compute_length_l()
/////////////////////////////////////
void
BaseStroke::compute_length_l()
{
   static ARRAY<double> distances;    

   // Note, bad verts (_good=false) are 'placeholder' clipped verts
   // which *might* not have a valid _base_loc.  For instance,
   // we may want to induce a break in a stroke, or account
   // for free hatching which can give a vertex from a uv position
   // that lies in a hole in the uv-map... In this case, 
   // rather than tossing the bad verts, the subclassed stroke 
   // uses the uv in the _data field and leverages the 
   // refinement code in the stroke class to produce a refined,  
   // visible vertex via uv interp. (by subclassing the refine
   // vertex function.) These points must be treated with care here: when
   // we run into a bad vert, we just give it the l value of
   // the previous good vert, and make sure NOT to update last_vert,
   // so that last_vert always points to the last good vert
   // (for the sake of the length computation.)

   //We also set all verts to visible here incase were using
   //the offset generation routines.
   int i, good_verts;
   BaseStrokeVertex *last_vert;
   BaseStrokeVertex *this_vert;

   distances.clear();
   _ndc_length = 0.0;

   last_vert = &(_verts[0]);
   last_vert->_vis_state = BaseStrokeVertex::VIS_STATE_VISIBLE;
   // Check if the vert has undefined _base_loc
   // and init the last_vert to NULL if so, 
   // indicating that no good last vert exists.
   if (!last_vert->_good)
   {
      good_verts = 0;
      last_vert = 0;
   }
   else
      good_verts = 1;


   distances += 0.0;    

   for (i=1; i<_verts.num(); i++)
   {

      this_vert = &(_verts[i]);
      this_vert->_vis_state = BaseStrokeVertex::VIS_STATE_VISIBLE;
      if (this_vert->_good)
      {
         good_verts++;
         if (last_vert)
         {
            _ndc_length +=
               (this_vert->_base_loc - last_vert->_base_loc).planar_length();
         }
         last_vert = this_vert;
      }
      distances += _ndc_length;
   }

   //If there's only 1 'good' vert, then we'll have no
   //length (yet) but we might get a length later from 
   //vis. refinement at a good to bad transition
   if (good_verts>1)
      assert(_ndc_length != 0.0);

   for (i=0; i<_verts.num(); i++)
   {
      _verts[i]._l = distances[i];
   }

}
/////////////////////////////////////
// compute_t()
/////////////////////////////////////
void
BaseStroke::compute_t()
{
   //By this point, visibility is done on the _verts
   //(either for real, or via the default visibility
   //in compute_length_l), so we can compute the _t's 
   //of the refined vertex set, as _ndc_length and _l
   //cannot change anymore (as in refinement).  

   BaseStrokeVertex* v;

   if (_gen_t)
   {
      assert(_max_t > _min_t);
      float dt = (float)(_max_t - _min_t);

      if (_verts[0]._vis_state == BaseStrokeVertex::VIS_STATE_TRANSITION)
         _verts[0]._refined_vertex->_t = _min_t; //To avoid rounding error
      else
         _verts[0]._t = _min_t;                  //To avoid rounding error

      int i;
      for (i=1; i<_verts.num()-1; i++)
      {
         v = &(_verts[i]);
         if (v->_vis_state == BaseStrokeVertex::VIS_STATE_TRANSITION)
            v = v->_refined_vertex;
         v->_t = _min_t + (dt) * (_verts[i]._l/_ndc_length);
      }
      if (_verts[i]._vis_state == BaseStrokeVertex::VIS_STATE_TRANSITION)
         _verts[i]._refined_vertex->_t = _max_t; //To avoid rounding error
      else
         _verts[i]._t = _max_t;                  //To avoid rounding error
   }
}

/////////////////////////////////////
// compute_visibility()
/////////////////////////////////////

#define FRONT_DOT 0.05

void
BaseStroke::compute_visibility()
{
   int refs = 0;
   bool last_vis, this_vis;
   
   _vis_verts = 0;

   last_vis = set_vert_visibility(_verts[0]);
   if (last_vis) _vis_verts++;
   for (int i=1; i<_verts.num(); i++)
      {
         this_vis = set_vert_visibility(_verts[i]);
         if (this_vis != last_vis) refs++;
         if (this_vis) _vis_verts++;
         last_vis = this_vis;
      }

   // We total all the vis to not vis transitions
   // so that the _refine_verts array can be 
   // prealloced to the right size.  This is 
   // necessary so that this array is not
   // implicitly resized during a particular
   // update() causing the _refined_vertex
   // pointer in vertices to become invalid.

   // Note: realloc(0) doubles the size!!!
   // I remembered the hard way...
   if (refs) _refine_verts.realloc(refs);


}

/////////////////////////////////////
// set_vert_visibility()
/////////////////////////////////////

bool
BaseStroke::set_vert_visibility(BaseStrokeVertex &v)
{

   CNDCZpt &pt = v._base_loc;

   // Check the _good flag (sometimes invisible placeholders are handy)
   if ( !v._good)
      {
         v._vis_state = BaseStrokeVertex::VIS_STATE_CLIPPED;   
         v._vis_reason = BaseStrokeVertex::VIS_REASON_BAD_VERT;
      }
   // Check if on screen
   else if ( (_vis_type & VIS_TYPE_SCREEN) && 
             ((pt[2] < 0)|| (pt[2] > 1)||(fabs(pt[0])>_max_x)||(fabs(pt[1])>_max_y)))
      {
         v._vis_state = BaseStrokeVertex::VIS_STATE_CLIPPED;   
         v._vis_reason = BaseStrokeVertex::VIS_REASON_OFF_SCREEN;
      }
   // Check if front facing
   else if ( (_vis_type & VIS_TYPE_NORMAL) &&
             (((_cam - Wpt(pt)).normalized() * v._norm) < FRONT_DOT ) )
      {
         v._vis_state = BaseStrokeVertex::VIS_STATE_CLIPPED;   
         v._vis_reason = BaseStrokeVertex::VIS_REASON_BACK_FACING;
      }
   else if ( (_vis_type & VIS_TYPE_SUBCLASS) &&
             !check_vert_visibility(v))
      {
         v._vis_state = BaseStrokeVertex::VIS_STATE_CLIPPED;   
         v._vis_reason = BaseStrokeVertex::VIS_REASON_SUBCLASS;
      }
   else
      {
         v._vis_state = BaseStrokeVertex::VIS_STATE_VISIBLE;
         return true;
      }
   return false;
}
   
////////////////////////////////////
// compute_refinements
/////////////////////////////////////
//
// -At vis transitions, we insert a replacement
//  visible vertex upto some specified
//  pixel resolution (if necessary)
// -This vertex is placed in a pointer
//  in the _vert[] 
// -The vis flags are updated at the
//  transition vertex
// -If no refinement is needed, we
//  fill the pointer with the verts
//  own address 
// -vis flags updated
//
/////////////////////////////////////
void
BaseStroke::compute_refinements()
{

   BaseStrokeVertex                 *last_vert, *this_vert, *new_vert;
   BaseStrokeVertex::vis_state_t     last_vis,   this_vis;

   last_vert = &(_verts[0]);
   // If the first vert's visible, then set it as a clipped
   // to vis transition and have its refined postion point to itself
   // Note that the 'clipping reason' is set to ENDPOINT indicating
   // that the transition isn't due to a clipping discontinuity,
   // but rather because this is an endpoint of the given stroke.
   if (last_vert->_vis_state == BaseStrokeVertex::VIS_STATE_VISIBLE)
      {
         last_vert->_vis_state = BaseStrokeVertex::VIS_STATE_TRANSITION;
         last_vert->_vis_reason = BaseStrokeVertex::VIS_REASON_ENDPOINT;
         last_vert->_trans_type = BaseStrokeVertex::TRANS_VISIBLE;
         last_vert->_refined_vertex = last_vert;
      }
   last_vis = last_vert->_vis_state;

   for (int i=1; i<_verts.num(); i++)
      {
         this_vert = &(_verts[i]);
         this_vis = this_vert->_vis_state;

         // If this vert is clipped...
         if (this_vis == BaseStrokeVertex::VIS_STATE_CLIPPED)
            {
               // ...and the last was visible, we refine the transition...
               if (last_vis == BaseStrokeVertex::VIS_STATE_VISIBLE)
                  {
                     new_vert = refine_vert(i-1,true);
                     // ...and if refinement succeeds, we mark the clipped vert
                     // as a vis to clipped transition and give it the
                     // pointer to the refined vert
                     if (new_vert)
                        {
                           this_vert->_vis_state = BaseStrokeVertex::VIS_STATE_TRANSITION;
                           //this_vert->_vis_reason remains unchanged so we know why we're a transition
                           this_vert->_trans_type = BaseStrokeVertex::TRANS_CLIPPED;
                           this_vert->_refined_vertex = new_vert;
                           _vis_verts++;
                        }
                     // ...and if refinement fails, we mark the vis vert as the transition
                     // and have it point to itself
                     else
                        {
                           last_vert->_vis_state = BaseStrokeVertex::VIS_STATE_TRANSITION;
                           last_vert->_vis_reason = this_vert->_vis_reason;
                           last_vert->_trans_type = BaseStrokeVertex::TRANS_CLIPPED;
                           last_vert->_refined_vertex = last_vert;
                        }
                  }
               // ...and the last vert is a transition...
               else if (last_vis == BaseStrokeVertex::VIS_STATE_TRANSITION)
                  {
                     // ...from clipped to visible, then we refine the transition...
                     if (last_vert->_trans_type == BaseStrokeVertex::TRANS_VISIBLE)
                        {
                           new_vert = refine_vert(i-1,true);   
                           // ...and if refinement succeeds, we mark the clipped vert
                           // as a vis to clipped transition and give it the
                           // pointer to the refined vert
                           if (new_vert)
                              {
                                 this_vert->_vis_state = BaseStrokeVertex::VIS_STATE_TRANSITION;
                                 //this_vert->_vis_reason remains unchanged so we know why we're a transition
                                 this_vert->_trans_type = BaseStrokeVertex::TRANS_CLIPPED;
                                 this_vert->_refined_vertex = new_vert;
                                 _vis_verts++;
                              }
                           // ...and if refinement fails, we check to see if the transition
                           // vertex is refined...
                           else
                              {
                                 // ...and if so, we use its unrefined position, since
                                 // this couldn't be improved upon
                                 if (last_vert != last_vert->_refined_vertex)
                                    {
                                       this_vert->_vis_state = BaseStrokeVertex::VIS_STATE_TRANSITION;
                                       //this_vert->_vis_reason remains unchanged so we know why we're a transition
                                       this_vert->_trans_type = BaseStrokeVertex::TRANS_CLIPPED;
                                       this_vert->_refined_vertex = last_vert;
                                       _vis_verts++;
                                    }
                                 // ...and if not, we dump the transition as a singular
                                 // vertex, since it's contribution is less than the 
                                 // given refinement resolution
                                 else 
                                    {
                                       last_vert->_vis_state = BaseStrokeVertex::VIS_STATE_CLIPPED;
                                       last_vert->_vis_reason = BaseStrokeVertex::VIS_REASON_SINGULAR;
                                       _vis_verts--;
                                    }
                              }
                        }
                     // ...from visible to clipped, we do nothing
                  }
               // ... and the last was clipped, we do nothing
            }
         // Otherwise this vert is visible...
         else 
            {
               // ...and if the last vert is clipped, then we refine the transition...
               if (last_vis == BaseStrokeVertex::VIS_STATE_CLIPPED)
                  {
                     new_vert = refine_vert(i-1,false);
                     // ...and if refinement succeeds, we mark the clipped vert
                     // as a clipped to vis transition and give it the
                     // pointer to the refined vert
                     if (new_vert)
                        {
                           last_vert->_vis_state = BaseStrokeVertex::VIS_STATE_TRANSITION;
                           //last_vert->_vis_reason unchaged so we know why we're a transition
                           last_vert->_trans_type = BaseStrokeVertex::TRANS_VISIBLE;
                           last_vert->_refined_vertex = new_vert;
                           _vis_verts++;
                        }
                     // ...and if refinement fails, we mark the vis vert as the transition
                     // and have it point to itself
                     else
                        {
                           this_vert->_vis_state = BaseStrokeVertex::VIS_STATE_TRANSITION;
                           this_vert->_vis_reason = last_vert->_vis_reason;
                           this_vert->_trans_type = BaseStrokeVertex::TRANS_VISIBLE;
                           this_vert->_refined_vertex = this_vert;
                        }
                  }
               // ...and the last vert is a transition...
               else if (last_vis == BaseStrokeVertex::VIS_STATE_TRANSITION)
                  {
                     // ...from visible to clipped, then we refine the transition...
                     if (last_vert->_trans_type == BaseStrokeVertex::TRANS_CLIPPED)
                        {
                           new_vert = refine_vert(i-1,false);  
                           // ...and if refinement succeeds, we mark the visible vert
                           // as a clipped to visible transition and give it the
                           // pointer to the refined vert (effectively moving its position)
                           if (new_vert)
                              {
                                 this_vert->_vis_state = BaseStrokeVertex::VIS_STATE_TRANSITION;
                                 this_vert->_vis_reason = last_vert->_vis_reason;
                                 this_vert->_trans_type = BaseStrokeVertex::TRANS_VISIBLE;
                                 this_vert->_refined_vertex = new_vert;
                              }
                           // ...and if refinement fails, we still mark the vert as the transition
                           // and have it point to itself
                           else
                              {
                                 this_vert->_vis_state = BaseStrokeVertex::VIS_STATE_TRANSITION;
                                 this_vert->_vis_reason = last_vert->_vis_reason;
                                 this_vert->_trans_type = BaseStrokeVertex::TRANS_VISIBLE;
                                 this_vert->_refined_vertex = this_vert;
                              }
                        }
                     // ...from clipped to visible, we do nothing
                  }
               // ...and the last vert is visible, we do nothing
            }
      
         last_vert = this_vert;
         last_vis = last_vert->_vis_state;
      }

   // Fix up the last vert if its not clipped...
   if (last_vis != BaseStrokeVertex::VIS_STATE_CLIPPED)
      {
         // ...so if its visible, set it as a vis to clipped transition
         //and have it point to itself as the refinement
         if (last_vis == BaseStrokeVertex::VIS_STATE_VISIBLE)
            {
               last_vert->_vis_state = BaseStrokeVertex::VIS_STATE_TRANSITION;
               last_vert->_vis_reason = BaseStrokeVertex::VIS_REASON_ENDPOINT;
               last_vert->_trans_type = BaseStrokeVertex::TRANS_CLIPPED;
               last_vert->_refined_vertex = last_vert;
            }
         // ...so if its a clipped to visible transtion, dump it
         // as a singular point
         else if (last_vert->_trans_type == BaseStrokeVertex::TRANS_VISIBLE)
            {
               last_vert->_vis_state = BaseStrokeVertex::VIS_STATE_CLIPPED;
               last_vert->_vis_reason = BaseStrokeVertex::VIS_REASON_SINGULAR;
               _vis_verts--;
            }
      }



}

////////////////////////////////////
// refine_vertex
/////////////////////////////////////
BaseStrokeVertex*
BaseStroke::refine_vert(int i, bool left)
{

   // Pixel resolution of refinement
   float thresh = 1.0f * _scale;

   float good_frac, bad_frac, mid_frac;
   BaseStrokeVertex* new_vert;
   BaseStrokeVertex* verts[4];
   
   //Strore the verts for the spline interpolation
   verts[1] = &(_verts[i]);
   verts[2] = &(_verts[i+1]);

   NDCZpt old_base_loc;
   NDCZpt l_loc = verts[1]->_base_loc;
   NDCZpt r_loc = verts[2]->_base_loc;

   //Bail out if were already within the resolution threshold
   if ((l_loc - r_loc).planar_length() <= thresh) return NULL;

   //Strore the verts for the spline interpolation
   verts[0] = &(_verts[(i>0)?(i-1):(0)]);
   verts[3] = &(_verts[((i+2)<_verts.num())?(i+2):(i+1)]);

   //Alloc a fresh vertex
   new_vert = &(_refine_verts.next());

   //Intialize the bounds and location depending
   //upon whether the left of right vert is visible
   if (left)
      {
         good_frac = 0.0;
         bad_frac = 1.0;
         new_vert->_base_loc = l_loc;
      }
   else
      {
         good_frac = 1.0;
         bad_frac = 0.0;
         new_vert->_base_loc = r_loc;
      }

   do 
      {
         //Save the old position
         old_base_loc = new_vert->_base_loc;

         //Compute a new position
         mid_frac = (good_frac + bad_frac)/2.0f;
         interpolate_refinement_vert(new_vert, verts, mid_frac);

         //Update the search range
         if (set_vert_visibility(*new_vert))
            good_frac = mid_frac;
         else
            bad_frac = mid_frac;

         // XXX - Careful if we get back a 'bad' vert
      } while ( !new_vert->_good || ((old_base_loc - new_vert->_base_loc).planar_length() > thresh) );

   //If good_frac moved, then we found a refined vert
   if ( ((left) && (good_frac > 0.0)) || ((!left) && (good_frac < 1.0)) )
      {
         //But we killed it looking for something better
         if (good_frac != mid_frac)
            {
               interpolate_refinement_vert(new_vert, verts, good_frac);
            }

         //Since l values were already assigned to the 
         //basestroke's verts, we must give this new
         //base stroke vert an l value. Rather than interp
         //the l values of the the [1],[2] verts, we
         //compute it as an offset from the 'good' vert
         //since the 'bad' vert might have good=false
         //indicating that its _base_loc is bogus, and
         //thus in possession of a bogus _l
         if (left)
            new_vert->_l = verts[1]->_l + (verts[1]->_base_loc - new_vert->_base_loc).planar_length();
         else
            new_vert->_l = verts[2]->_l - (verts[2]->_base_loc - new_vert->_base_loc).planar_length();

         //Now do the same for the t value.  If _gen_t
         //is true, we can just compute t from l (later). But
         //if it is false, we can safely interpolate from
         //[1] to [2], as the user had to supply a t 
         //for both input verts.

         if (!_gen_t)
            new_vert->_t = verts[1]->_t*(1.0-good_frac) + verts[2]->_t*(good_frac);
//         else
//            new_vert->_t = _min_t + (_max_t - _min_t)*(new_vert->_l/_ndc_length);

         // XXX - Alright, this was a bad design call... Refining a 'bad' vert
         // at the start or end of a stroke can result in _l < 0 or > _ndc_length.
         // We look for this, and adjust as necessary by adding the most negative
         // _l to all _l or extending _ndc_length.  Note that each of the two
         // cases can only happen once per stroke.

         if (new_vert->_l < 0.0)
         {
            double val = fabs(new_vert->_l);

            for (int j=0; j<_verts.num(); j++)
               _verts[j]._l += val;
            
            _ndc_length += val;

            new_vert->_l = 0.0;
         }
         else if (new_vert->_l > _ndc_length)
         {
            _ndc_length = new_vert->_l;
         }


         return new_vert;
      }
   //Otherwise, trash the new vertex
   else
      {
         _refine_verts.del();
         return NULL;
      }
}


////////////////////////////////////
// interpolate_refinement_vert
/////////////////////////////////////

// This version is called by refine_vertex().
// In the base class, it just calls to
// interpolate_vert(), but a subclass might
// want to modify this.  For instance, the
// HatchingStroke interpolates free hatching
// strokes differently from fixed strokes by use uv!

void
BaseStroke::interpolate_refinement_vert(
   BaseStrokeVertex *v, 
   BaseStrokeVertex **vl,
   double u)
{
   interpolate_vert(v,vl,u);
}


////////////////////////////////////
// interpolate_vert
/////////////////////////////////////

// This version is called with the left
// of the inner pair of vertices given
// as a pointer, with the assumption
// that the other 3 verts can be determined
// via the prev,next pointers. This is useful
// after compute_length_t assembles this
// connectivity after refinement.

void
BaseStroke::interpolate_vert(
   BaseStrokeVertex *v, 
   BaseStrokeVertex *vleft,
   double u)
{
   static BaseStrokeVertex *vl[4];
   static BaseStrokeVertex *vlast = NULL;
   static unsigned int stamp = 666;

   if ((vlast != vleft) || (stamp != VIEW::stamp()))
      {
         //Check the stamp, cause the connectivity could
         //change for this vertex in a new frame
         stamp = VIEW::stamp();
         vlast = vleft;
         vl[0] = vlast->_prev_vert;
         vl[1] = vlast;
         vl[2] = vlast->_next_vert;
         vl[3] = vlast->_next_vert->_next_vert;
      }

   interpolate_vert(v,vl,u);

}


////////////////////////////////////
// interpolate_vert
/////////////////////////////////////

// This version explicity takes an
// array of 4 vertex pointers.  It
// is called directly during refinement
// when connectivity isn't defined.

void
BaseStroke::interpolate_vert(
   BaseStrokeVertex *v, 
   BaseStrokeVertex **vl, 
   double u)
{
   double  _6u;
   
   double   u2;
   double _3u2;
   double _6u2;
   
   double   u3;
   double _2u3;
   
   
   //We do this as lazy as possible. 
   //Only relevant quantities are interpolated. 
   //Only the position is splined, while others
   //are liniearly interpolated.
   
   v->_good = true;
   
   if (_vis_type & VIS_TYPE_NORMAL) 
      v->_norm    = ((1-u) * vl[1]->_norm + u * vl[2]->_norm).normalized();
   
   //XXX - Interpolate for v->_data? Or just let subclass worry about it?
   //      Right now, only HatchingStrokes for Free hatching use the
   //      this for uv, and they override interpolate_refinement_vert
   //      to handle this.  They don't care about uv beyond vis interpolation
   //      so currently nobody needs _data in the final sampled verts
   
   //_t, _loc, _width, _alpha  - need not be initialized yet
   //_vis_* - will be set by the caller (if at all)
   
   //XXX - We've assumed that vertices are unique
   assert(vl[2]->_l != vl[0]->_l);
   assert(vl[3]->_l != vl[1]->_l);
   
   NDCZvec m1, m2;
   
   double di = (vl[2]->_l - vl[1]->_l);
   
   // If the endpoints are replicated pointers, then
   // we use the parabola-slope enpoint scheme
   // Note, if (vl[0] == vl[1]) && (vl[2] == vl[3])
   // then we do the first case (which ends up with
   // m1=m2=(vl[2]->_base_loc - vl[1]->_base_loc)
   if (vl[0] == vl[1]) 
      {
         m2 = (vl[3]->_base_loc - vl[1]->_base_loc) * (di/(vl[3]->_l - vl[1]->_l));
         m1 = (vl[2]->_base_loc - vl[1]->_base_loc) * 2.0 - m2;
      }
   // If the endpoints are replicated pointers, then
   // we use the parabola-slope enpoint scheme
   else if (vl[2] == vl[3])
      {
         m1 = (vl[2]->_base_loc - vl[0]->_base_loc) * (di/(vl[2]->_l - vl[0]->_l));
         m2 = (vl[2]->_base_loc - vl[1]->_base_loc) * 2.0 - m1;
      }
   // In general, the slopes are taken as the
   // cords between the adjacent points
   else
      {
         m1 = (vl[2]->_base_loc - vl[0]->_base_loc) * (di/(vl[2]->_l - vl[0]->_l));
         m2 = (vl[3]->_base_loc - vl[1]->_base_loc) * (di/(vl[3]->_l - vl[1]->_l));
      }

   // If u is outside [0,1] then we must be trying
   // to apply an offset that extends beyond the
   // end of the stroke.  That's cool, but let's
   // make sure that we didn't screw up by asserting
   // that we're extending beyond the t=_min_t or t=_max_t
   // endpoints of the stroke. Then just use the
   // slope at the end to extend outward...
   if (u>1.0)
      {
         // This function is sometimes called while generating
         // the offsets, before they have been set.
         //assert(_offsets != NULL);
         assert( vl[2]==vl[3]);
         assert( vl[2]->_t == _max_t);
         v->_base_loc =  vl[2]->_base_loc + m2 * (u - 1.0);
         v->_dir = m2;

      }
   else if (u<0.0)
      {
         // This function is sometimes called while generating
         // the offsets, before they have been set.
         //assert(_offsets != NULL);
         assert( vl[0]==vl[1]);
         assert( vl[1]->_t == _min_t);
         v->_base_loc =  vl[1]->_base_loc + m1 * u;
         v->_dir = m1;
      }
   else
      {

         _6u =  6*u;
   
         u2 =  u*u;
         _3u2 = 3*u2;
         _6u2 = 6*u2;
   
         u3 = u*u2;
         _2u3 = 2*u3;


         v->_base_loc =(
            vl[1]->_base_loc * (1     - _3u2 + _2u3) +
            vl[2]->_base_loc * (        _3u2 - _2u3) +
            m1               * (    u - 2*u2 +   u3) +
            m2               * (      -   u2 +   u3)
            );

         // This is the tangent (not the normal)
         v->_dir =(
            ((vl[1]->_base_loc * (      - _6u  + _6u2) -
              vl[2]->_base_loc * (      - _6u  + _6u2)) +
             m1 * (    1 - 4*u  + _3u2)  +
             m2 * (      - 2*u  + _3u2)));
      }
}

////////////////////////////////////
// interpolate_offset
/////////////////////////////////////

// This version explicity takes an
// array of 4 offsets.  It is called directly during resampling.

void
BaseStroke::interpolate_offset(
   BaseStrokeOffset *o,
   BaseStrokeOffset **ol, 
   double u)
{
   double _1_u = 1.0 - u;
   double u2 = u * u;
   double u3 = u * u2;
   double _3u3 =  3*u3;

   //Who cares?!

   //o->_type = 
   //o->_pos    = (_1_u) * ol[1]->_pos    + u * ol[2]->_pos;

   o->_press  = (_1_u) * ol[1]->_press  + u * ol[2]->_press;

   o->_len = ((  - u + 2*u2 -   u3) * ol[0]->_len +
              (2 -     5*u2 + _3u3) * ol[1]->_len +
              (    u + 4*u2 - _3u3) * ol[2]->_len +
              (      -   u2 +   u3) * ol[3]->_len) / 2.0;


}   


/////////////////////////////////////
// compute_connectivity()
/////////////////////////////////////
void
BaseStroke::compute_connectivity()
{

   int i;
   BaseStrokeVertex *last_vert = 0;        // XXX - this was below with no value assigned
   BaseStrokeVertex *this_vert = 0;

   // At this point, the _verts are finalized, so
   // we can now fill out their connectivity (prev,next)
   // pointers.  Previous to refinement, the connectivity
   // could change by insertion or array resizing (which
   // would invalidate the pointers).

   // These pointers are handy to the spline interp
   // function. We need only account properly for the
   // visible verts -- doubling up at vis transitions.

   for (i=0; i<_verts.num(); i++)
      {
         this_vert = &(_verts[i]);
         if (this_vert->_vis_state == BaseStrokeVertex::VIS_STATE_TRANSITION)
            {
               if (this_vert->_trans_type == BaseStrokeVertex::TRANS_VISIBLE)
                  {
                     //When next/prev is self-referential we'll
                     //know we hit a transition, but not why, so...
                     this_vert->_refined_vertex->_vis_reason = this_vert->_vis_reason;

                     this_vert = this_vert->_refined_vertex;
                     //XXX//
                     assert(this_vert != NULL);
                     this_vert->_prev_vert = this_vert;
                  }
               else // (this_vert->_trans_type == BaseStrokeVertex::TRANS_CLIPPED)
                  {
                     //When next/prev is self-referential we'll
                     //know we hit a transition, but not why, so...
                     this_vert->_refined_vertex->_vis_reason = this_vert->_vis_reason;

                     this_vert = this_vert->_refined_vertex;
                     //XXX//
                     assert(this_vert != NULL);
                     this_vert->_prev_vert = last_vert; // XXX - no value assigned to last vert here
                     last_vert->_next_vert = this_vert;
                     this_vert->_next_vert = this_vert;
                  }
            }
         else if (this_vert->_vis_state == BaseStrokeVertex::VIS_STATE_VISIBLE)
            {
               this_vert->_prev_vert = last_vert;
               last_vert->_next_vert = this_vert;
            }
         //else - skip clipped verts
         last_vert = this_vert;
      }

}


////////////////////////////////////
// compute_resampling
/////////////////////////////////////
void
BaseStroke::compute_resampling()
{
   if (_offsets)
   {
      setup_offsets();
      compute_resampling_with_offsets();
   }
   else
   {
      compute_resampling_without_offsets();
   }
}
////////////////////////////////////
// compute_resampling_with_offsets
/////////////////////////////////////
void
BaseStroke::setup_offsets()
{
   
   if (!_offsets->get_replicate()) return;

   //If replicating, setup the parameters that adjust
   //the t values of rubberstamped offsets

   // The fetch length = the length of the base stroke, in pixels.
   // Don't deviate from this policy!
   _offsets->set_fetch_len(_ndc_length / _scale);

   // Likely to be the ratio of curr mesh size to original (>0)
   _offsets->set_fetch_stretch((double)_offset_stretch);

   // Likely given by the Lubofaction (0-1)
   _offsets->set_fetch_phase((double)_offset_phase);

   // Set by people doing newer funked Lubo
   _offsets->set_fetch_lower_t(_offset_lower_t);
   _offsets->set_fetch_upper_t(_offset_upper_t);


}

////////////////////////////////////
// compute_resampling_with_offsets
/////////////////////////////////////
void
BaseStroke::compute_resampling_with_offsets()
{

   // Whether offsets can extend beyond the ends of the basepath
   int hangover = _offsets->get_hangover();

   // Index into _draw_verts of the first
   // sampled vertex for a particular
   // contiguous offset chain. After sampling
   // an entire chain, we use step back through
   // all the sampled verts and normalize
   // their _t using ndc_last.
   int start_index;

   int o;
   bool outer_done, in_stroke, l_to_r;

   double t_off_l, t_off_r; //Endpoints of and offset pair in the offset list
   double t_seg_l, t_seg_r; //Endpoints of a sampled visible segment within a pair
   

   double ndc_delta = _pix_res * _scale;

   // In clipped stretches of the sampled stroke
   // we use this approximation to account
   // for the ndc distance travelled along the final
   // stroke for the sake of the _l's in the sampled verts.
   double t_to_ndc = _ndc_length / (_max_t - _min_t);

   BaseStrokeOffset offsets[4], *offset[4], new_offset;
   for (o=0; o<4; o++) offset[o] = &offsets[o];

   //Current index, i, into _verts and the
   //associated t and pointers for i and i+1
   int               i;
   double            ti,   ti_1;
   BaseStrokeVertex  *vi,  *vi_1;
   
   BaseStrokeVertex v1, v2;
   BaseStrokeVertex *vert_left, *vert_right;
   vert_left = &v1;  vert_right = &v2;

   // The l values assigned to sampled points cannot
   // be taken as the l values of the underlying basestroke
   // because the l value can double back, and doesn't
   // reflect the spacing between samples, anyway.
   // Instead, we assign l as the ndc distance from
   // the begininning of each contiguous offset segment.
   // In regions where the basestroke is clipped, we
   // estimate the cumulative ndc distance as the
   // ndc spacing of clipped base stroke verts.

   // ndc_last tracks the ndc distance (from the start) 
   // of the last sampled vert, and ndc_gap is the leftover ndc spacing
   // between ndc_last from the previous offset pair
   // and it's t_off_r.  Since we're not forcing sampling at
   // every offset, then we'll end up sampling between
   // the offsets, and must account for the leftover
   // gap of the previous offset pair when beginning
   // the sampling in the following pair.
   double ndc_last, ndc_gap;

   o = 0;

   // We need to walk about the base stroke and maintain
   // the four vertices needed for splining, as well as
   // an index into the basestroke, each of which persists
   // over the running loops.  At the end of the
   // outer loop, the index will be left consistent
   // with the last sampled position, and thus should
   // be close to the starting position in the next iteration.
   // In the case where we're still in the midst of a
   // visible stroke segment, the index into the base
   // stroke should be exaxcly the interval necessary
   // to begin the next loop.

   // Initialize the basestroke index to the one enclosing
   // the first offset.

   // XXX - A binary search would be optimal, but for
   // now let just set it to 0 and get things working...
   i = 0;

   vi = &(_verts[i]);
   vi_1 = &(_verts[i+1]);

   if (vi->_vis_state == BaseStrokeVertex::VIS_STATE_TRANSITION)
      vi = vi->_refined_vertex;
   if (vi_1->_vis_state == BaseStrokeVertex::VIS_STATE_TRANSITION)
      vi_1 = vi_1->_refined_vertex;

   ti = vi->_t;
   ti_1 = vi_1->_t;


   // Keeps track of when were in the midst of a visible
   // stroke segment.
   in_stroke = false;

   // We sample the final stroke between offset pairs
   // offset[1]/offset[2], and maintain [0]/[3]
   // as the offsets bordering the region as necessary
   // arguments to the spline function...

   while (o < (_offsets->fetch_num()-1))
   {
      _offsets->fetch(o,*(offset[1]));

         // Note, this must be an OFFSET_TYPE_BEGIN
         // since the inner loop won't return until it 
         // processes a full BEGIN to END segment

      /*XXX*/ assert(offset[1]->_type == BaseStrokeOffset::OFFSET_TYPE_BEGIN);

      // Setup the remaining initial values for inner loop

         // We double up the vertices at offset end-points
         // for the sake of the spline 
      *(offset[0]) = *(offset[1]);

      // Note, the outer loops gives the start as i, but
      // we choose to pair the offset as (o-1,o), so...
      o+=1;

      _offsets->fetch(o,*(offset[2]));

      // In the inner loop we refine all offset pairs
      // (o-1,o) i.e. offset[1]/offset[2], in the contiguous
      // segment that starts at the point found in the 
      // outer loop. We exit the loop with index
      // i equal to the point immediately after the 
      // OFFSET_TYPE_END endpoint offset. Then the
      // outer loop can step into and setup the next segment

      // We keep track of the left and right of the offset pair.
      t_off_l = offset[1]->_pos;
      
      outer_done = false;

      // Reset the ndc distance tally for this continuous profile.
      ndc_last = 0;
      start_index = _draw_verts.num();
      
      while (!outer_done)
      {
         // We store this, rather than looking in the offset[]
         // on the off chance that we later spline these (and
         // not just the offset length)
         t_off_r = offset[2]->_pos;
         
         // Offsets i-2.i-1.i are in offset[0,1,2], but we
         // must fetch i+1 into offset[3]. 

         // If offset i is an endpoint then
         // we just replicate this endpoint
         if (offset[2]->_type == BaseStrokeOffset::OFFSET_TYPE_END)
            {
               *(offset[3]) = *(offset[2]);

                     // This is the last pair in this segment
                     outer_done = true;
                  }
               // Otherwise we fetch offset i+1 (which must exist
               // because all offset segments terminate at a
               // OFFSET_TYPE_END)
               else
                  {
                     /*XXX*/assert((o+1) < _offsets->fetch_num());
                     _offsets->fetch(o+1,*(offset[3]));
                  }
            
               // Now all offset[0,1,2,3] are prepared, so we
               // generate vertices within pair offset[1,2].
               // At this point, the basestroke has had its
               // verices refined to account for visibility breaks.
               // Such break can occur within offset pairs, so
               // we must be careful to introduce breaks in the
               // resampled stroke at both offset breaks and
               // visibility breaks. Within any given offset
               // pair, we must loop over all visible segments...

               // XXX - Offset pairs must have some t extent
               /*XXX*/ assert(t_off_l != t_off_r);

               // Loop over visible segments for this offset pair. 
               // We're done with this pair when t_seg_r, the right end
               // of sampled region, is the same as the right end
               // of the current offset pair's region.
               t_seg_l = t_seg_r = t_off_l;

               // The basestroke's t increases with the index into
               // the array, however, the offset pair's t may
               // run in either direction.  We set l_to_r to true
               // if the direction of t matches the basestroke.
               l_to_r = t_off_r > t_off_l;

               while (t_seg_r != t_off_r)
                  {
                     t_seg_l = t_seg_r;
            
                     if (in_stroke)
                        {
                           // Must be first pass in this loop. Right endpoint
                           // vertex from last offset pair will be the left
                           // endpoint of this pair. So just swap the pointers.
               
                           BaseStrokeVertex *v = vert_left;
                           vert_left = vert_right;
                           vert_right = v;

                           // Note, vi/vi_1 should contain t_seg_l
                        }
                     else
                        {
                           // If we're not in a stroke, we must find
                           // a valid t_seg_l in case the visibility
                           // in basestroke fails at t_seg_l.
               
                           // First get the basestroke pair that encloses
                           // t_seg_l. We check separately the two cases
                           // for the offset segment's l_to_r status and
                           // in each case handle the cases of being left
                           // of right of the correct interval separately

                           // This all looks like alot of werk, but it isn't.
               
                           if (l_to_r)
                              {
                                 if (ti <= t_seg_l)
                                    {
                                       while (ti_1 <= t_seg_l)
                                          {
                                             if (i+1 < (_verts.num()-1))
                                                {
                                                   // We're left of the right base pair
                                                   // so walk right until we're good
                                                   i++;
                                                   ti = ti_1;
                                                   vi = vi_1;
                                                   vi_1 = &(_verts[i+1]);
                                                   if (vi_1->_vis_state == BaseStrokeVertex::VIS_STATE_TRANSITION)
                                                      vi_1 = vi_1->_refined_vertex;
                                                   ti_1 = vi_1->_t;
                                                }
                                             else
                                                {
                                                   //In this case, we're at the rightmost base pair
                                                   //but we still cannot manage to enclose t_seg_l

                                                   //This can happen when the last base point is
                                                   //clipped and replaced with a refined vertex
                                                   //with ti_1<_max_t while t_seg_l >= ti_1

                                                   //This can also happen if t_seg_l >= _max_t because
                                                   //the offset profile extends off the end of the
                                                   //stroke.  

                                                   //In either case, we want to stop at this i, i+1
                                                   //base pair and proceed to checking the vis of the pair
                                                   break;
                                                }
                                          }
                                    }
                                 else
                                    {
                                       do
                                          {
                                             if (i > 0)
                                                {
                                                   // We're right of the right base pair
                                                   // so walk left until we're good
                                                   i--;
                                                   ti_1 = ti;
                                                   vi_1 = vi;
                                                   /*XXX*/assert(i >= 0);
                                                   vi = &(_verts[i]);
                                                   if (vi->_vis_state == BaseStrokeVertex::VIS_STATE_TRANSITION)
                                                      vi = vi->_refined_vertex;
                                                   ti = vi->_t;
                                                }
                                             else
                                                {
                                                   //In this case, we're at the leftmost base pair
                                                   //but we still cannot manage to enclose t_seg_l

                                                   //This can happen when the first base point is
                                                   //clipped and replaced with a refined vertex
                                                   //with ti>_min_t while t_seg_l < ti

                                                   //This can also happen if t_seg_l < _min_t because
                                                   //the offset profile extends off the end of the
                                                   //stroke.  

                                                   //In either case, we want to stop at this i, i+1
                                                   //base pair and proceed to checking the vis of the pair
                                                   break;
                                                }
                                          } while (ti > t_seg_l);
                                    }
                              }
                           else // !(l_to_r)
                              {
                                 if (ti < t_seg_l)
                                    {
                                       while (ti_1 < t_seg_l)
                                          {
                                             if (i+1 < (_verts.num()-1))
                                                {
                                                   // We're left of the right base pair
                                                   // so walk right until we're good
                                                   i++;
                                                   ti = ti_1;
                                                   vi = vi_1;
                                                   vi_1 = &(_verts[i+1]);
                                                   if (vi_1->_vis_state == BaseStrokeVertex::VIS_STATE_TRANSITION)
                                                      vi_1 = vi_1->_refined_vertex;
                                                   ti_1 = vi_1->_t;
                                                }
                                             else
                                                {
                                                   //In this case, we're at the rightmost base pair
                                                   //but we still cannot manage to enclose t_seg_l

                                                   //This can happen when the last base point is
                                                   //clipped and replaced with a refined vertex
                                                   //with ti_1<_max_t while t_seg_l >= ti_1

                                                   //This can also happen if t_seg_l >= _max_t because
                                                   //the offset profile extends off the end of the
                                                   //stroke.  

                                                   //In either case, we want to stop at this i, i+1
                                                   //base pair and proceed to checking the vis of the pair
                                                   break;
                                                }
                                          }
                                    }
                                 else
                                    {
                                       do
                                          {
                                             if (i > 0)
                                                {
                                                   // We're right of the right base pair
                                                   // so walk left until we're good
                                                   i--;
                                                   ti_1 = ti;
                                                   vi_1 = vi;
                                                   /*XXX*/assert(i >= 0);
                                                   vi = &(_verts[i]);
                                                   if (vi->_vis_state == BaseStrokeVertex::VIS_STATE_TRANSITION)
                                                      vi = vi->_refined_vertex;
                                                   ti = vi->_t;
                                                }
                                             else
                                                {
                                                   //In this case, we're at the leftmost base pair
                                                   //but we still cannot manage to enclose t_seg_l

                                                   //This can happen when the first base point is
                                                   //clipped and replaced with a refined vertex
                                                   //with ti>_min_t while t_seg_l <= ti

                                                   //This can also happen if t_seg_l <= _min_t because
                                                   //the offset profile extends off the end of the
                                                   //stroke.  

                                                   //In either case, we want to stop at this i, i+1
                                                   //base pair and proceed to checking the vis of the pair
                                                   break;
                                                }

                                          } while (ti >= t_seg_l);

                                    }
                              }
               
                           // Now we've the right base pair for the starting
                           // point, t_seg_l.  We must check if this is
                           // a visible base pair. If not, we must walk toward
                           // t_off_r until a valid start is found.
                           // If we reach t_seg_r without doing so, we
                           // can skip this entire offset pair. Otherwise,
                           // move t_seg_l, fire up in_stroke and compute
                           // vert_left

                           if (l_to_r)
                              {
                                 BaseStrokeVertex::vis_reason_t reason;
                  
                                 bool vis = false;

                                 //But before checking the vis of the base pair,
                                 //take care if we couldn't get an enclosing base pair
                                 //If t_seg_l is not contained in ti, ti_1, then:

                                 // If the offset pair starts to the right of the last base pair
                                 if (ti_1 <= t_seg_l)
                                    {
                                       //The last base pair is only partially visible (t<_max_t) 
                                       //so we skip the remainder of this offset
                                       if (ti_1 < _max_t)
                                          {
                                             // Do nothing, just fail out with vis = false
                                          }
                                       //Else this offset extends off the end of the stroke. 
                                       else
                                          {
                                             // If the last vertex is clipped we dump this offset pair
                                             if ( (_verts[i+1]._vis_state == BaseStrokeVertex::VIS_STATE_CLIPPED) ||
                                                  (!hangover) )
                                                {
                                                   // Do nothing, just fail out with vis = false
                                                }
                                             // Otherwise handle this piece that hangs off
                                             else
                                                {
                                                   vis = true;
                                                   // Record that this offset begins at an offset break
                                                   reason = BaseStrokeVertex::VIS_REASON_OFFSET;
                      
                                                   assert(t_seg_l >= _max_t );
                                                }
                                          }
                                    }
                                 // If the offset pair starts to the left of the first base pair
                                 else if (t_seg_l < ti)
                                    {
                                       // If the first base pair's clipped short (t>0)
                                       if (ti > _min_t)
                                          {
                                             //If the right side of the offset pair also
                                             //falls short, we can just bail on the rest
                                             //of this offset pair
                                             if (t_off_r <= ti)
                                                {
                                                   // Do nothing, just fail out  with vis = false
                                                }
                                             // Otherwise, we can move the starting
                                             // point up to ti and proceed.
                                             else
                                                {
                                                   vis = true;
                                                   // We shift t_seg_l to the left side
                                                   // of this visible base pair, and account
                                                   // for the additional distance travelled
                                                   // in the clipped region.
                                                   ndc_last += (ti - t_seg_l) * t_to_ndc;
                                                   t_seg_l = ti;
                                                   // Record that this offset begins due to base stroke vis reason
                                                   reason = _verts[i]._vis_reason;
                                                   /*XXX*/assert(_verts[i]._vis_state == BaseStrokeVertex::VIS_STATE_TRANSITION);
                                                }
                                          }
                                       //Otherwise, we're hanging off before the start
                                       //of the stroke. No problem
                                       else
                                          {
                                             // If the first vert is clipped or we're not allowing hangovers 
                                             // we have to walk right to see if we can find a visible
                                             // base vertex in this offset pair
                                             if ((_verts[i]._vis_state == BaseStrokeVertex::VIS_STATE_CLIPPED) ||
                                                 (!hangover))
                                                {
                                                   if (_verts[i]._vis_state == BaseStrokeVertex::VIS_STATE_TRANSITION)
                                                      {
                                                         // If we get here, the first vert's fully visible
                                                         // but we're disallowing hangovers.  We move the
                                                         // sampling up to t_min if that fits in the offset pair
                                                         assert(_verts[i]._trans_type == BaseStrokeVertex::TRANS_VISIBLE);
                                                         assert(ti == _min_t);

                                                         if (ti < t_off_r )
                                                            {
                                                               vis = true;
                                                               // We shift t_seg_l to the left side
                                                               // of this visible base pair, and account
                                                               // for the additional distance travelled
                                                               // in the clipped region.
                                                               ndc_last += (ti - t_seg_l) * t_to_ndc;
                                                               t_seg_l = ti;
                                                               // Record that this offset begins due to base stroke vis reason
                                                               reason = _verts[i]._vis_reason;
                                                            }
                                                         //else fail out with vis=false

                                                      }
                                                   else
                                                      {
                                                         while ( !vis && (ti_1 < t_off_r ) )
                                                            {
                                                               // Walk right until we find a visible base
                                                               // pair, or we walk out of this offset pair
                                                               i++;
                                                               ti = ti_1;
                                                               vi = vi_1;
                                                               // XXX - This should be safe, since we'd have to have
                                                               // a totally clipped stroke to fail, and we'd
                                                               //not be here in that case, but let's check:
                                                               assert(i+1 < _verts.num());

                                                               vi_1 = &(_verts[i+1]);
                                                               if (vi_1->_vis_state == BaseStrokeVertex::VIS_STATE_VISIBLE)
                                                                  {
                                                                     vis = true;
                                                                     // We shift t_seg_l to the left side
                                                                     // of this visible base pair, and account
                                                                     // for the additional distance travelled
                                                                     // in the clipped region.
                                                                     ndc_last += (ti - t_seg_l) * t_to_ndc;
                                                                     t_seg_l = ti;
                                                                     // Record that this offset begins due to base stroke vis reason
                                                                     reason = _verts[i]._vis_reason;
                                                                     /*XXX*/assert(_verts[i]._vis_state == BaseStrokeVertex::VIS_STATE_TRANSITION);
                                                                  }
                                                               else if (vi_1->_vis_state == BaseStrokeVertex::VIS_STATE_TRANSITION)
                                                                  {
                                                                     if (vi_1->_trans_type == BaseStrokeVertex::TRANS_CLIPPED)
                                                                        {
                                                                           vis = true;
                                                                           // We shift t_seg_l to the left side
                                                                           // of this visible base pair and account
                                                                           // for the additional distance travelled
                                                                           // in the clipped region.
                                                                           ndc_last += (ti - t_seg_l) * t_to_ndc;
                                                                           t_seg_l = ti;
                                                                           // Record that this offset begins due to base stroke vis reason
                                                                           reason = _verts[i]._vis_reason;
                                                                           /*XXX*/assert(_verts[i]._vis_state == BaseStrokeVertex::VIS_STATE_TRANSITION);
                                                                        }
                                                                     vi_1 = vi_1->_refined_vertex;
                                                                  }
                                                               ti_1 = vi_1->_t;
                                                            }
                                                      }
                                                }
                                             // Otherwise handle this piece that hangs off
                                             else
                                                {
                                                   vis = true;
                                                   // Record that this offset begins at an offset break
                                                   reason = BaseStrokeVertex::VIS_REASON_OFFSET;
                        
                                                   assert(t_seg_l < _min_t);
                                                }
                                          }

                                    }
                                 // If all's well we enclose t_seg_l, just check the vis of the pair
                                 else if ( (_verts[i]._vis_state == BaseStrokeVertex::VIS_STATE_VISIBLE) ||
                                           (  ( _verts[i]._vis_state  == BaseStrokeVertex::VIS_STATE_TRANSITION ) && 
                                              ( _verts[i]._trans_type == BaseStrokeVertex::TRANS_VISIBLE        )))
                                    {
                                       vis = true;
                                       // We keep t_seg_l as it falls in a visible base pair

                                       // Record that this offset begins at an offset break
                                       reason = BaseStrokeVertex::VIS_REASON_OFFSET;
                                    }
                                 // If vis fails...
                                 else
                                    {
                                       while ( !vis && (ti_1 < t_off_r ) )
                                          {
                                             // Walk right until we find a visible base
                                             // pair, or we walk out of this offset pair
                                             if ((i+1) == (_verts.num()-1))
                                                {
                                                   // If we can't go any further, it
                                                   // must be because the right side off the offset
                                                   // fall off the end of the stroke.  Well, turf
                                                   // this, then, its as if we found the whole thing clipped.
                                                   assert(t_off_r > _max_t);
                                                   break;
                                                }
                                             i++;
                                             ti = ti_1;
                                             vi = vi_1;
                                             vi_1 = &(_verts[i+1]);
                                             if (vi_1->_vis_state == BaseStrokeVertex::VIS_STATE_VISIBLE)
                                                {
                                                   vis = true;
                                                   // We shift t_seg_l to the left side
                                                   // of this visible base pair, and account
                                                   // for the additional distance travelled
                                                   // in the clipped region.
                                                   ndc_last += (ti - t_seg_l) * t_to_ndc;
                                                   t_seg_l = ti;
                                                   // Record that this offset begins due to base stroke vis reason
                                                   reason = _verts[i]._vis_reason;
                                                   /*XXX*/assert(_verts[i]._vis_state == BaseStrokeVertex::VIS_STATE_TRANSITION);
                                                }
                                             else if (vi_1->_vis_state == BaseStrokeVertex::VIS_STATE_TRANSITION)
                                                {
                                                   if (vi_1->_trans_type == BaseStrokeVertex::TRANS_CLIPPED)
                                                      {
                                                         vis = true;
                                                         // We shift t_seg_l to the left side
                                                         // of this visible base pair and account
                                                         // for the additional distance travelled
                                                         // in the clipped region.
                                                         ndc_last += (ti - t_seg_l) * t_to_ndc;
                                                         t_seg_l = ti;
                                                         // Record that this offset begins due to base stroke vis reason
                                                         reason = _verts[i]._vis_reason;
                                                         /*XXX*/assert(_verts[i]._vis_state == BaseStrokeVertex::VIS_STATE_TRANSITION);
                                                      }
                                                   vi_1 = vi_1->_refined_vertex;
                                                }
                                             ti_1 = vi_1->_t;
                                          }
                                    }
                                 if (!vis)
                                    {
                                       // Failed to find a visible segment so
                                       // let's get out of this inner loop by
                                       // continuing as if we just sampled up to
                                       // the region's right limit. Note that we
                                       // left the base pair setup for the next
                                       // offset pair!
                                       t_seg_r = t_off_r;

                                       // Do something to approximate stroke
                                       // parameter advancement in this clipped
                                       // region so that when/if we pick up more
                                       // of this continuguous offset, we use the
                                       // right _l's in the sampled verts

                                       ndc_last += (t_off_r - t_seg_l) * t_to_ndc;
                     
                                       continue;
                                       // XXX - a 'break' might work just as well
                                    }
                                 else
                                    {
                                       // Otherwise compute the left vert

                                       BaseStrokeVertex *new_vert = &(_draw_verts.next());
                                       interpolate_vert(new_vert, vi, (t_seg_l - ti)/(ti_1 - ti));
               
                                       ndc_gap = 0;

                                       new_vert->_l = ndc_last;

                                       new_vert->_vis_state = BaseStrokeVertex::VIS_STATE_TRANSITION;
                                       new_vert->_vis_reason = reason;
                                       new_vert->_trans_type = BaseStrokeVertex::TRANS_VISIBLE;

                                       new_vert->_dir = new_vert->_dir.perpendicular().normalized();

                                       interpolate_offset(&new_offset, offset, (t_seg_l - t_off_l )/( t_off_r - t_off_l ));

                                       apply_offset( new_vert, &new_offset);


                                       // Store this in vert_left so we can compute
                                       // the width of the segment and use it to
                                       // decide sample spacing... For now, only the
                                       // _loc is useful
                                       vert_left->_loc = new_vert->_loc;
                                          
                                       in_stroke = true;
                                    }
                              }
                           else //!(l_to_r)
                              {
                                 BaseStrokeVertex::vis_reason_t reason;
                  
                                 bool vis = false;
                           
                                 //If t_seg_l is not contained in ti, ti_1, then:

                                 //If the offset pair starts to the left of the first base pair
                                 if (t_seg_l <= ti)
                                    {
                                       // If the first base pair's clipped short of t=0
                                       // we can toss the remainder of this offset pair
                                       if (ti > _min_t)
                                          {
                                             // Do nothing, just fail out with vis = false
                                          }
                                       // Else, we're hanging off the start of the base path
                                       // no problem...
                                       else
                                          {
                                             // If the first base vert is clipped we can dump this offset pair
                                             // or we disallow hangovers
                                             if ((_verts[i]._vis_state == BaseStrokeVertex::VIS_STATE_CLIPPED) ||
                                                 (!hangover))
                                                {
                                                   // Do nothing, just fail out with vis = false
                                                }
                                             //Otherwise lets handle this bit that hangs off the end
                                             else
                                                {
                                                   vis = true;
                                                   // Record that this offset begins at an offset break
                                                   reason = BaseStrokeVertex::VIS_REASON_OFFSET;

                                                   assert(t_seg_l <= _min_t);
                                                }
                                          }

                                    }
                                 // If the offset pair starts to the right of the last base pair
                                 else if (ti_1 < t_seg_l)
                                    {
                                       // If the last base pair's clipped short
                                       // of reaching t=1
                                       if (ti_1 < _max_t)
                                          {
                                             //If the right side of the offset pair also
                                             //exceeds, we can just bail on the rest
                                             //of this offset pair
                                             if (ti_1 <= t_off_r)
                                                {
                                                   // Do nothing, just fail out
                                                }
                                             // Otherwise, we can move the starting
                                             // point down to ti_1 and proceed.
                                             else
                                                {
                                                   vis = true;
                                                   // We shift t_seg_l to the right side
                                                   // of this visible base pair and account
                                                   // for the distance travelled in the
                                                   // clipped region

                                                   ndc_last += (t_seg_l - ti_1) * t_to_ndc;
                                                   t_seg_l = ti_1;
                                                   // Record that this offset begins due to base stroke vis reason
                                                   reason = _verts[i+1]._vis_reason;
                                                   /*XXX*/assert(_verts[i+1]._vis_state == BaseStrokeVertex::VIS_STATE_TRANSITION);
                                                }
                                          }
                                       //Otherwise, we're hanging off after the end
                                       //of the stroke. No problem
                                       else
                                          {
                                             // If the last base vert is clipped over we're
                                             // not allowing hangovers then we
                                             // must walk left to see if a visible base
                                             // vertex fits in this offset pair
                                             if ((_verts[i+1]._vis_state == BaseStrokeVertex::VIS_STATE_CLIPPED) ||
                                                 (!hangover))
                                                {
                                                   if (_verts[i+1]._vis_state == BaseStrokeVertex::VIS_STATE_TRANSITION)
                                                      {
                                                         // If we get here, the last vert's fully visible
                                                         // but we're disallowing hangovers.  We move the
                                                         // sampling to t_max if that fits in the offset pair
                                                         assert(_verts[i+1]._trans_type == BaseStrokeVertex::TRANS_CLIPPED);
                                                         assert(ti_1 == _max_t);

                                                         if (ti_1 > t_off_r )
                                                            {
                                                               vis = true;
                                                               // We shift t_seg_l to the right side
                                                               // of this visible base pair and account
                                                               // for the distance travelled in the
                                                               // clipped region

                                                               ndc_last += (t_seg_l - ti_1) * t_to_ndc;
                                                               t_seg_l = ti_1;
                                                               // Record that this offset begins due to base stroke vis reason
                                                               reason = _verts[i+1]._vis_reason;
                                                            }
                                                         //else fail out with vis=false

                                                      }
                                                   else
                                                      {
                                                         while ( !vis && (ti > t_off_r ) )
                                                            {
                                                               // Walk left until we find a visible base
                                                               // pair, or we walk out of this offset pair
                                                               i--;
                                                               ti_1 = ti;
                                                               vi_1 = vi;
                                                               // XXX - This should be safe unless the
                                                               // whole stroke's clipped, but then we'd not get here.
                                                               assert(i >= 0);
                                                               vi = &(_verts[i]);
                                                               if (vi->_vis_state == BaseStrokeVertex::VIS_STATE_VISIBLE)
                                                                  {
                                                                     vis = true;
                                                                     // We shift t_seg_l to the right side
                                                                     // of this visible base pair and account
                                                                     // for the distance travelled in the
                                                                     // clipped region

                                                                     ndc_last += (t_seg_l - ti_1) * t_to_ndc;
                                                                     t_seg_l = ti_1;
                                                                     // Record that this offset begins due to base stroke vis reason
                                                                     reason = _verts[i+1]._vis_reason;
                                                                     /*XXX*/assert(_verts[i+1]._vis_state == BaseStrokeVertex::VIS_STATE_TRANSITION);
                                                                  }
                                                               else if (vi->_vis_state == BaseStrokeVertex::VIS_STATE_TRANSITION)
                                                                  {
                                                                     if (vi->_trans_type == BaseStrokeVertex::TRANS_VISIBLE)
                                                                        {
                                                                           vis = true;
                                                                           // We shift t_seg_l to the right side
                                                                           // of this visible base pair and account
                                                                           // for the distance travelled in the
                                                                           // clipped region
                                                                           ndc_last += (t_seg_l - ti_1) * t_to_ndc;
                                                                           t_seg_l = ti_1;
                                                                           // Record that this offset begins due to base stroke vis reason
                                                                           reason = _verts[i+1]._vis_reason;
                                                                           /*XXX*/assert(_verts[i+1]._vis_state == BaseStrokeVertex::VIS_STATE_TRANSITION);
                                                                        }
                                                                     vi = vi->_refined_vertex;
                                                                  }
                                                               ti = vi->_t;
                                                            }
                                                      }
                                                }
                                             // Otherwise, let's handle this bit that hangs off
                                             else
                                                {
                                                   vis = true;
                                                   // Record that this offset begins at an offset break
                                                   reason = BaseStrokeVertex::VIS_REASON_OFFSET;

                                                   assert(t_seg_l > _max_t);
                                                }
                                          }

                                    }
                                 else if ( (_verts[i]._vis_state == BaseStrokeVertex::VIS_STATE_VISIBLE) ||
                                           (  ( _verts[i]._vis_state  == BaseStrokeVertex::VIS_STATE_TRANSITION ) && 
                                              ( _verts[i]._trans_type == BaseStrokeVertex::TRANS_VISIBLE        )))
                                    {
                                       vis = true;
                                       // We keep t_seg_l as it falls in a visible base pair

                                       // Record that this offset begins at an offset break
                                       reason = BaseStrokeVertex::VIS_REASON_OFFSET;
                                    }
                                 else
                                    {
                                       while ( !vis && (ti > t_off_r ) )
                                          {
                                             // Walk left until we find a visible base
                                             // pair, or we walk out of this offset pair
                                             if (i == 0)
                                                {
                                                   // If we can't go any further, it
                                                   // must be because the right side off the offset
                                                   // fall off the end of the stroke.  Well, turf
                                                   // this, then, its as if we found the whole thing clipped.
                                                   assert(t_off_r < _min_t);
                                                   break;
                                                }
                                             i--;
                                             ti_1 = ti;
                                             vi_1 = vi;
                                             vi = &(_verts[i]);
                                             if (vi->_vis_state == BaseStrokeVertex::VIS_STATE_VISIBLE)
                                                {
                                                   vis = true;
                                                   // We shift t_seg_l to the right side
                                                   // of this visible base pair and account
                                                   // for the distance travelled in the
                                                   // clipped region
                                                   ndc_last += (t_seg_l - ti_1) * t_to_ndc;
                                                   t_seg_l = ti_1;
                                                   // Record that this offset begins due to base stroke vis reason
                                                   reason = _verts[i+1]._vis_reason;
                                                   /*XXX*/assert(_verts[i+1]._vis_state == BaseStrokeVertex::VIS_STATE_TRANSITION);
                                                }
                                             else if (vi->_vis_state == BaseStrokeVertex::VIS_STATE_TRANSITION)
                                                {
                                                   if (vi->_trans_type == BaseStrokeVertex::TRANS_VISIBLE)
                                                      {
                                                         vis = true;
                                                         // We shift t_seg_l to the right side
                                                         // of this visible base pair and account
                                                         // for the distance travelled in the
                                                         // clipped region
                                                         ndc_last += (t_seg_l - ti_1) * t_to_ndc;
                                                         t_seg_l = ti_1;
                                                         // Record that this offset begins due to base stroke vis reason
                                                         reason = _verts[i+1]._vis_reason;
                                                         /*XXX*/assert(_verts[i+1]._vis_state == BaseStrokeVertex::VIS_STATE_TRANSITION);
                                                      }
                                                   vi = vi->_refined_vertex;
                                                }
                                             ti = vi->_t;
                                          }
                                    }
                                 if (!vis)
                                    {
                                       // Failed to find a visible segment so
                                       // let's get out of this inner loop by
                                       // continuing as if we just sampled up to
                                       // the region's right limit. Note that we
                                       // left the base pair setup for the next
                                       // offset pair!
                                       t_seg_r = t_off_r;

                                       // Do something to approximate stroke
                                       // parameter advancement in this clipped
                                       // region so that when/if we pick up more
                                       // of this continuguous offset, we use the
                                       // right _l's in the sampled verts

                                       ndc_last += (t_seg_l - t_off_r) * t_to_ndc;
                     
                                       continue;
                                       // XXX - a 'break' might work just as well
                                    }
                                 else
                                    {
                                       // Otherwise

                                       BaseStrokeVertex *new_vert = &(_draw_verts.next());
                                       interpolate_vert(new_vert, vi, (t_seg_l - ti)/(ti_1 - ti));
               
                                       ndc_gap = 0.0;

                                       new_vert->_l = ndc_last;

                                       new_vert->_vis_state = BaseStrokeVertex::VIS_STATE_TRANSITION;
                                       new_vert->_vis_reason = reason;
                                       new_vert->_trans_type = BaseStrokeVertex::TRANS_VISIBLE;
                     
                                       new_vert->_dir = new_vert->_dir.perpendicular().normalized();                     
                     
                                       interpolate_offset(&new_offset,offset, (t_seg_l - t_off_l )/( t_off_r - t_off_l ));

                                       apply_offset( new_vert, &new_offset);

                                       // Store this in vert_left so we can compute
                                       // the width of the segment and use it to
                                       // decide sample spacing... For now, only the
                                       // _loc is useful.
                                       vert_left->_loc = new_vert->_loc;
                              
                                       in_stroke = true;
                                    }
                              }

                        }
            
                     // If we get this far, we've a vert_left, and the vi,v_i
                     // in the basestroke that contains it (or else the offset
                     // begins at a point beyond the ends of the stroke
                     // and we've already determined that the end point
                     // is visible at t=_min_t or _max_t)
            
                     // Now we must walk from t_seg_l to find t_seg_r.
                     // If the basestroke is visible throughout this offset pair
                     // then t_seg_r will be t_off_r, otherwise t_seg_r
                     // will be determined as the position of the vis
                     // discontinuity.

                     BaseStrokeVertex *v_r;

                     if (l_to_r)
                        {
                           v_r = vi_1;

                           while ( true )
                              {
                                 // Bail out if we find the r side of the offset pair
                                 if ( v_r->_t >= t_off_r )
                                    {
                                       t_seg_r = t_off_r;
                                       break;
                                    }
                                 // Bail out if the basestroke hits a discontinuity
                                 // before we can enclose t_seg_r 
                                 if (v_r == v_r->_next_vert)
                                    {
                                       // If the discontinuity is t=_max_t, then this
                                       // offset hangs off the (visible) end of
                                       // the stroke, so we're cool with drawing this
                                       if ((v_r->_t == _max_t) && (hangover))
                                          {
                                             t_seg_r = t_off_r;
                                             assert( t_seg_r > _max_t);
                                             break;   
                                          }
                                       // Otherwise we clip the offset pair short
                                       else
                                          {
                                             t_seg_r = v_r->_t;
                                             // We'll also store the reason
                                             vert_right->_vis_reason = v_r->_vis_reason;
                                             break;
                                          }
                                    }
                                 v_r = v_r ->_next_vert;
                              }
                           // Step back because we always interp from left to right
                           v_r = v_r->_prev_vert;
                        }
                     else //!(l_to_r)
                        {
                           v_r = vi;

                           while ( true )
                              {
                                 // Bail out if we find the r side of the offset pair
                                 if ( v_r->_t <= t_off_r )
                                    {
                                       t_seg_r = t_off_r;
                                       break;
                                    }
                                 // Bail out if the basestroke hits a discontinuity
                                 // before we can enclose t_seg_r 
                                 if (v_r == v_r->_prev_vert)
                                    {
                                       // If the discontinuity is t=_min_t, then this
                                       // offset hangs off the (visible) end of
                                       // the stroke, so we're cool with drawing this
                                       if ((v_r->_t == _min_t) && (hangover))
                                          {
                                             t_seg_r = t_off_r;

                                             assert(t_seg_r < _min_t);

                                             break;
                                          }
                                       // Otherwise we clip the offset pair short
                                       else
                                          {
                                             t_seg_r = v_r->_t;
                                             // We'll also store the reason
                                             vert_right->_vis_reason = v_r->_vis_reason;
                                             break;
                                          }
                                    }
                                 v_r = v_r ->_prev_vert;
                              }
                        }

                     interpolate_vert(vert_right, v_r, (t_seg_r - v_r->_t)/(v_r->_next_vert->_t - v_r->_t));

                     vert_right->_dir = vert_right->_dir.perpendicular().normalized();
                       
                     interpolate_offset(&new_offset, offset, ( t_seg_r - t_off_l )/( t_off_r - t_off_l ));

                     apply_offset(vert_right, &new_offset);

                     // We're got everything setup now.  Let's decide
                     // upon the sample spacing in parameter space

                     // Width of region to sample in basestroke parameter space
                     // Note this is negative if !(l_to_r)
                     double t_seg = t_seg_r - t_seg_l;

                     // Length of offset pair
                     double ndc_seg;

                     // Conversion from ndc length to parameter space
                     // Note this is negative if !(l_to_r)
                     double ndc_to_t;

                     // Parameter space sampling density
                     // Note this is negative if !(l_to_r)
                     double dt;

                     // Use the measured length of the offset vert pair
                     // to approximate the ndc to t conversion
                     ndc_seg = (vert_right->_loc - vert_left->_loc).planar_length();
                     ndc_to_t = t_seg / ndc_seg;
                     dt = ndc_delta * ndc_to_t;

                     //But use the base ndc_length if that produces
                     //a finer sampling
                     if ((1.0/t_to_ndc) < fabs(ndc_to_t))
                        {
                           ndc_seg = fabs(t_seg) * t_to_ndc;
                           ndc_to_t = t_seg / ndc_seg;
                           dt = ndc_delta * ndc_to_t;
                        }

                     // Init the first point to sample. Note that
                     // we account for the gap left over from the
                     // previous segment
                     double t = t_seg_l + ndc_to_t * ( ndc_delta - ndc_gap );

                     // Stop short of t_seg_r if we're running up to
                     // a visibility or offset discontinuity, since
                     // we force interpolation of the endpoint
                     double t_end = (( t_seg_r != t_off_r ) || ( outer_done ) ) ? (t_seg_r - dt/4.0) : (t_seg_r);

                     ndc_gap += ndc_seg;

                     if (l_to_r)
                        {
                           // Keeping sampling until we hit the end
                           while ( t <= t_end )
                              {
                                 // Step of the current base pair so it
                                 // includes the current sample
                                 while ( t > ti_1 )
                                    {
                                       // If we're sampling off the end of the stroke
                                       // then don't try to step i
                                       if ((i+1) == (_verts.num()-1))
                                          {
                                             assert(t > _max_t);
                                             break;
                                          }
                                       i++;
                                       ti = ti_1;
                                       vi = vi_1;
                                       vi_1 = &(_verts[i+1]);
                                       if (vi_1->_vis_state == BaseStrokeVertex::VIS_STATE_TRANSITION)
                                          vi_1 = vi_1->_refined_vertex;
                                       ti_1 = vi_1->_t;
                                    }

                                 BaseStrokeVertex *new_vert = &(_draw_verts.next());
                                 interpolate_vert(new_vert, vi, (t - ti)/(ti_1 - ti));
            
                                 new_vert->_l = (ndc_last += ndc_delta);

                                 new_vert->_vis_state = BaseStrokeVertex::VIS_STATE_VISIBLE;
                                 new_vert->_dir = new_vert->_dir.perpendicular().normalized();                  

                                 interpolate_offset(&new_offset,offset, ( t - t_off_l )/( t_off_r - t_off_l ));
                                 
                                 apply_offset( new_vert, &new_offset);

                                 t += dt;
                                 ndc_gap -= ndc_delta;
                              }
                           // To ease the next iteration, we insure that
                           // that vi,vi_1 encloses the endpoint
                           while ( t_seg_r > ti_1 )
                              {
                                 // If we're sampling beyond the end, then
                                 // we can stop here
                                 if ((i+1) == (_verts.num()-1))
                                    {
                                       assert(t_seg_r > _max_t);
                                       break;
                                    }
                                 i++;
                                 ti = ti_1;
                                 vi = vi_1;
                                 vi_1 = &(_verts[i+1]);
                                 if (vi_1->_vis_state == BaseStrokeVertex::VIS_STATE_TRANSITION)
                                    vi_1 = vi_1->_refined_vertex;
                                 ti_1 = vi_1->_t;
                              }
                        }
                     else //!(l_to_r)
                        {
                           // Keeping sampling until we hit the end
                           while ( t >= t_end )
                              {
                                 // Step of the current base pair so it
                                 // includes the current sample
                                 while ( t < ti )
                                    {
                                       // If we're sampling off the end of the stroke
                                       // then don't try to step i
                                       if (i == 0)
                                          {
                                             assert(t < _min_t);
                                             break;
                                          }
                                       i--;
                                       ti_1 = ti;
                                       vi_1 = vi;
                                       vi = &(_verts[i]);
                                       if (vi->_vis_state == BaseStrokeVertex::VIS_STATE_TRANSITION)
                                          vi = vi->_refined_vertex;
                                       ti = vi->_t;
                                    }

                                 BaseStrokeVertex *new_vert = &(_draw_verts.next());
                                 interpolate_vert(new_vert, vi, (t - ti)/(ti_1 - ti));
            
                                 new_vert->_l = (ndc_last += ndc_delta);

                                 new_vert->_vis_state = BaseStrokeVertex::VIS_STATE_VISIBLE;

                                 new_vert->_dir = new_vert->_dir.perpendicular().normalized();

                                 interpolate_offset(&new_offset,offset, ( t - t_off_l )/( t_off_r - t_off_l ));
                                 
                                 apply_offset( new_vert, &new_offset);

                                 t += dt;
                                 ndc_gap -= ndc_delta;
                              }
                           // To ease the next iteration, we insure that
                           // that vi,vi_1 encloses the endpoint
                           while ( t_seg_r < ti )
                              {
                                 // If we're sampling beyond the end, then
                                 // we can stop here
                                 if (i == 0)
                                    {
                                       assert(t_seg_r < _min_t);
                                       break;
                                    }
                                 i--;
                                 ti_1 = ti;
                                 vi_1 = vi;
                                 vi = &(_verts[i]);
                                 if (vi->_vis_state == BaseStrokeVertex::VIS_STATE_TRANSITION)
                                    vi = vi->_refined_vertex;
                                 ti = vi->_t;
                              }
               
                        }
            
                     // If we hit a vis or offset discontinuity, we
                     // now add the endpoint
                     if (( t_seg_r != t_off_r ) || ( outer_done ))
                        {
                           BaseStrokeVertex *new_vert = &(_draw_verts.next());
         
                           new_vert->_l = (ndc_last += ndc_gap);
                           new_vert->_base_loc = vert_right->_base_loc;
                           // XXX - Need this?
                           new_vert->_dir = vert_right->_dir;

                           if (_vis_type & VIS_TYPE_NORMAL) 
                              new_vert->_norm = vert_right->_norm;

                           new_vert->_width = vert_right->_width;
                           new_vert->_alpha = vert_right->_alpha;
                           new_vert->_loc = vert_right->_loc;

                           new_vert->_vis_state = BaseStrokeVertex::VIS_STATE_TRANSITION;
                           new_vert->_trans_type = BaseStrokeVertex::TRANS_CLIPPED;
               
                           if (t_seg_r != t_off_r)
                              {
                                 // If we got clipped, then this was stored...
                                 new_vert->_vis_reason = vert_right->_vis_reason;
                              }
                           else
                              {
                                 // Otherwise the reason is given as an offset discontinuity
                                 new_vert->_vis_reason = BaseStrokeVertex::VIS_REASON_OFFSET;
                              }
                           ndc_gap = 0;
                           in_stroke = false;
                        }
            

                  }

               if (!outer_done)
               {
                  BaseStrokeOffset *o1 = offset[0];
                  for (int j=1; j<4; j++)
                     offset[j-1] = offset[j];
                  offset[3] = o1;

                  t_off_l = t_off_r;
               }
               else 
               {
                  // Store the true ndc end length incase we fell short
                  // for the sake of the stylization calculations
                  if ((_draw_verts.num()-1) >= start_index)
                     _draw_verts[_draw_verts.num()-1]._lmax = ndc_last;
               }
               // Increment o. This will also leave o at the
               // offset following a continguous segment if were done
               o++;
            } 
      }


}

////////////////////////////////////
// apply_offset
/////////////////////////////////////
void
BaseStroke::apply_offset(BaseStrokeVertex* v, BaseStrokeOffset* o)
{
   // mak, sublcass this...
   // Use pressures? 
   // Different LOD policy?

  v->_width = 1.0;
  v->_alpha = 1.0;

   // XXX -
   //
   //   Arbitrary policy for now -- scale offsets as ratio
   //   of current length (_ndc_length) to "original length"
   //   (_pix_len), but never magnify.
   //
   // This scaling should be precomputed once...
   
   // Get "original length" from the offset list,
   // convert to NDC:
   double orig_len = _offsets->get_pix_len()*_scale;

   assert(orig_len > 0);

   // Below, we multiply by _scale (pix_to_ndc_scale, that is)
   // because the offset is given in units of pixels, but
   // otherwise the stroke is defined in NDC units.
   double scale = _scale * min(_ndc_length/orig_len, 1.0);

   v->_loc = v->_base_loc + v->_dir * (scale * o->_len);

}
////////////////////////////////////
// compute_resampling_without_offsets
/////////////////////////////////////
void
BaseStroke::compute_resampling_without_offsets()
{

   int i;
   double l, dl, lmax, lmin, lgap;
   bool done;
   BaseStrokeVertex::vis_state_t vis[2];
   BaseStrokeVertex::vis_reason_t reason[2];
   BaseStrokeVertex* vert[2];

   // Desired stroke resoltion in parameter space
   dl = _pix_res * _scale;

   i = 0;

   // We refine the stroke between vertex pairs
   // vert[0]/vert[1]

   // In the outer loop, we search for a transition
   // vertex indicating a clipped->visible transition
   while (i < (_verts.num()-1))
   {
      vert[0] = &(_verts[i]);
      vis[0] = vert[0]->_vis_state;
      reason[0] = vert[0]->_vis_reason;

      // Skip over the clipped vertices. Note that if
      // the vert is not a transition, it must be clipped
      // as all visible verts exist between transitions
      // (and are iterated over in the inner loop.)
      if (vis[0] != BaseStrokeVertex::VIS_STATE_TRANSITION)
      {
         /*XXX*/assert(vis[0] == BaseStrokeVertex::VIS_STATE_CLIPPED);
         i++;
      }
      // If this vert is a transition, we begin refinement
      // Note, this must be a clipped->visible transition
      // since the inner loop won't return until it passes
      // a clipped->visible/visible->clipped pair.
      else //(vis[0] == BaseStrokeVertex::VIS_STATE_TRANSITION)
      {
         // Setup the remaining initial values for inner loop

         /*XXX*/assert(vert[0]->_trans_type == BaseStrokeVertex::TRANS_VISIBLE);
         vert[0] = vert[0]->_refined_vertex;
         /*XXX*/assert(vert[0]);

         // Note, the outer loops gives the start as i, but
         // we choose to pair the vertices as (i-1,i), so...
         i+=1;

         // Init the parameter to that of the start vert (i-1)
         l = vert[0]->_l;

         // In the inner loop we refine all vertex pairs
         // (i-1,i) i.e. vert[0]/vert[1], in the visible 
         // segment that starts at the point found in the 
         // outer loop. We exit the loop with index
         // i equal to the point immediately after the 
         // visible->clipped transition vertex. Then the
         // outer loop can resume searching for more
         // visible segments
         done = false;
         while (!done)
         {
            // Vert i-1 is in vert[0], but we
            // must fetch i into vert[1]. 

            vert[1] = &(_verts[i]);
            vis[1] = vert[1]->_vis_state;
            reason[1] = vert[1]->_vis_reason;

            if (vis[1] == BaseStrokeVertex::VIS_STATE_TRANSITION)
            {
               /*XXX*/assert(vert[1]->_trans_type == BaseStrokeVertex::TRANS_CLIPPED);
               vert[1] = vert[1]->_refined_vertex;
               /*XXX*/assert(vert[1]);

               // This is the last pair in this visible segment
               done = true;

            }
   
            // Max t value for interpolation between (i-1,i)
            // is the t value of vert i
            lmin = vert[0]->_l;
            lmax = vert[1]->_l;
            lgap = lmax - lmin;

            // Because we always interpolate at the stroke
            // endpoint, we avoid also adding an intermediate
            // point too near the endpoint by lowering the
            // t cutoff for the final point pair.
            if (done) lmax -= dl/4.0;
   
            // Now all vert[0,1] are prepared, so we
            // interpolate vertices within pair vert[0,1] for 
            // t values of t(i-1) + x*dt < t(i). Note t was
            // initialized upon entry to the inner loop, and
            // persists over each loop iteration.  

            // XXX - In general, we wont sample at the vertex 
            // boundaries (except the start and end points), 
            // and at minified resolution, the sampled stroke
            // will have fewer points that the given vertices.

            // BUT, before we do this, we check if this is 
            // the first pair of vertices in the segment, and
            // interpolate the first point separately
            // so as to properly set it's visibility
            if (vis[0] == BaseStrokeVertex::VIS_STATE_TRANSITION)
            {
               BaseStrokeVertex *new_vert = &(_draw_verts.next());
                                       
               // XXX -- changed by mak:  width and alpha may
               // be interpolated by a stroke, subclass so shouldn't
               // assume they're always 1.0
               new_vert->_width = vert[0]->_width;
               new_vert->_alpha = vert[0]->_alpha;                           

               interpolate_vert(new_vert, vert[0], 0.0);
               
               // XXX - Interpolate for _t incase we're texturing...
               new_vert->_t = vert[0]->_t;

               new_vert->_l = lmin;
               new_vert->_vis_state = BaseStrokeVertex::VIS_STATE_TRANSITION;
               new_vert->_vis_reason = reason[0];
               new_vert->_trans_type = BaseStrokeVertex::TRANS_VISIBLE;

               l += dl;
            }

            while (l < lmax)
            {
               BaseStrokeVertex *new_vert = &(_draw_verts.next());
                                          
               // XXX -- changed by mak:  width and alpha may          
               // be interpolated by a stroke, subclass so shouldn't
               // assume they're always 1.0
               new_vert->_width = vert[0]->_width;
               new_vert->_alpha = vert[0]->_alpha;                           

               // Sets the base_loc, norm, data, but not
               // the vis, dir, wid, alpha, good
               
               double frac = (l-lmin)/lgap;
               interpolate_vert(new_vert, vert[0], frac);

               // XXX - Interpolate for _t incase we're texturing...
               // XXX - Optimize this a bit...
               new_vert->_t = (1.0 - frac) * vert[0]->_t + (frac) * vert[1]->_t;

               new_vert->_l = l;
               new_vert->_vis_state = BaseStrokeVertex::VIS_STATE_VISIBLE;

               l += dl;
            }

            // If this was the final interpolted pair, then
            // we now interpolate the endpoint separately
            // to handle its visibility correctly.
            if (done)
            {
               BaseStrokeVertex *new_vert = &(_draw_verts.next());
               
               // XXX -- changed by mak:  width and alpha may          
               // be interpolated by a stroke, subclass so shouldn't
               // assume they're always 1.0
               new_vert->_width = vert[0]->_alpha;
               new_vert->_alpha = vert[0]->_alpha;

               interpolate_vert(new_vert, vert[0], 1.0);
               
               // XXX - Interpolate for _t incase we're texturing...
               new_vert->_t = vert[1]->_t;

               new_vert->_l = (lmax + dl/4.0);
               
               new_vert->_vis_state = BaseStrokeVertex::VIS_STATE_TRANSITION;
               new_vert->_vis_reason = reason[1];
               new_vert->_trans_type = BaseStrokeVertex::TRANS_CLIPPED;

               //Set lmax to _ndc_length so we know how much
               //of the end of the stroke might be missing for
               //the sake of the stylization calculations

               new_vert->_lmax = _ndc_length;
            }
            // Otherwise prepare for the next pair
            else
            {
               vert[0] = vert[1];
               vis[0] = vis[1];
               reason[0] = reason[1];
            }
   
            // Increment i. This will also leave i at the
            // vert following a visible segment if were done
            i++;
         } 
      }
   }
}

/////////////////////////////////////
// compute_directions()
/////////////////////////////////////
void
BaseStroke::compute_directions()
{

   NDCZvec last_perp, this_perp;

   // If there are no offsets, then resampling from
   // the basestroke spline produced the directions
   // from the tangent and we can skip this.  Otherwise,
   // the offsets perturbed the stroke and we need to
   // regenerate the directions.

   if (_offsets) {
      for (int i=0; i<_draw_verts.num(); i++) {
         if (_draw_verts[i]._vis_state == BaseStrokeVertex::VIS_STATE_TRANSITION) {
            if (_draw_verts[i]._trans_type == BaseStrokeVertex::TRANS_VISIBLE) {
               /*XXX*/assert( (i+1) < _draw_verts.num() );
               last_perp = (_draw_verts[i+1]._loc - _draw_verts[i]._loc)
                  .perpendicular().normalized();
               _draw_verts[i]._dir = last_perp;
            } else _draw_verts[i]._dir = last_perp;
         } else {
            /*XXX*/
            assert( _draw_verts[i]._vis_state ==
                    BaseStrokeVertex::VIS_STATE_VISIBLE );
            /*XXX*/
            assert( (i+1) < _draw_verts.num() );
            this_perp = (_draw_verts[i+1]._loc - _draw_verts[i]._loc)
               .perpendicular().normalized();
            _draw_verts[i]._dir = (this_perp + last_perp).normalized();

            //  Thicken tight bends?
            // _draw_verts[i]._dir *=
            //   clamp(1.0/(fabs(_draw_verts[i]._dir * this_perp)), 0.0, 2.0); 
            last_perp = this_perp;
         }
      }
   } else {
      // If we have the normals, then, because strokes that
      // wrap around the side of objects end up with screen
      // space perpendiculars that look odd at the boundary,
      // we use the normal to produce directions that keep
      // the strokes in the surface tangent plane.
      if (_vis_type & VIS_TYPE_NORMAL) {
         // t maps world coordinates to NDCZ coordinates:
         Wtransf t = VIEW::peek_cam()->ndc_projection();
         for (int i=0; i<_draw_verts.num(); i++) {
            BaseStrokeVertex& v = _draw_verts[i];
            // tran is the linear approximation of the perspective xform t:
            Wtransf  tran = t.derivative(Wpt(v._base_loc));
            Wtransf tran_inv = tran.inverse(true);

            // rib derived from cross of stroke's 3D direction and normal
            NDCZvec normrib = cross(v._dir, NDCZvec(v._norm, tran));  
            normrib[2]=0;
            normrib = normrib.normalized();
            
            // Usual screen space version of the rib
            NDCZvec rib = v._dir.perpendicular().normalized();

            // normrib is only useful when the strokes 3d dir is 
            // parallel with the view direction
            double dot = fabs(_cam_at_v * Wvec(v._dir,tran_inv).normalized());
            dot *= dot;
            v._dir = (normrib*(dot) + rib*(1.0-dot)).normalized();
         }
      } else {
         //Otherwise, just use the perps of the tangents
         for (int i=0; i<_draw_verts.num(); i++)
            _draw_verts[i]._dir = _draw_verts[i]._dir.perpendicular().normalized();
      }
   }
}

/////////////////////////////////////
// compute_stylization()
/////////////////////////////////////
void
BaseStroke::compute_stylization()
{
   int i, num = _draw_verts.num();
   
   if (!_offsets)
   {
      for (i=0; i<num; i++)
         _draw_verts[i]._loc = _draw_verts[i]._base_loc;
   }

   // The _l values for _draw_verts are actually
   // ndc lengths.  This is nice, since the parameters
   // are in pixel measures.  

   double start_l, last_l, dl;

   if ((_taper>0.0) || (_fade>0.0))
   {
      double taper_len = _taper * _scale;
      double fade_len =  _fade * _scale;

      last_l = DBL_MAX;
      for (i=0; i < num; i++)
      {   
         BaseStrokeVertex& v = _draw_verts[i];

         // If l went backwards, we must be have hit
         // a new, separate segment that arose from
         // an offset with breaks, or this is the first 
         // time round

         //If the start point is clipped, we start the stylization
         //from the true beginning (l=0) unless the clipping is due
         //to the screen boundary, since we want the strokes to be
         //confined to the screen, or if we're not haloing
         if (v._l < last_l)
         {
            assert(v._vis_state == BaseStrokeVertex::VIS_STATE_TRANSITION);
            if ( (v._vis_reason == BaseStrokeVertex::VIS_REASON_OFF_SCREEN) ||
                 (_halo == 0.0))
              start_l = v._l;
            else
               start_l = 0.0;
         }
         //Internal breaks restart the taper, unless we're haloing
         else if ( (v._vis_state == BaseStrokeVertex::VIS_STATE_TRANSITION) &&
                   (v._trans_type == BaseStrokeVertex::TRANS_VISIBLE) && 
                   ((v._vis_reason == BaseStrokeVertex::VIS_REASON_OFF_SCREEN)
                    || (_halo == 0.0)))
         {
            start_l = v._l;
         }

         dl = v._l - start_l;
         // /*XXX*/assert(dl>=0);

         if (dl < taper_len)
            v._width *= (float)sqrt(1-pow(  (taper_len-dl)/(taper_len)  ,2));

         if (dl < fade_len) 
            v._alpha *= (float)pow((1-pow(  (fade_len-dl)/(fade_len)  ,2)),1.5);

         last_l = v._l;
      }

      last_l = -DBL_MAX;
      for (i=num-1; i >= 0; i--)
      {   
         BaseStrokeVertex& v = _draw_verts[i];

         // If l went backwards, we must be have hit
         // a new, separate segment that arose from
         // an offset with breaks or this 1st time round
         if (v._l > last_l)
         {
            assert(v._vis_state == BaseStrokeVertex::VIS_STATE_TRANSITION);
            if ( (v._vis_reason == BaseStrokeVertex::VIS_REASON_OFF_SCREEN) || (_halo == 0.0))
               start_l = v._l;
            else
               start_l = v._lmax;
         }
         //Internal breaks restart the taper, unless we're haloing
         else if ( (v._vis_state == BaseStrokeVertex::VIS_STATE_TRANSITION) &&
                   (v._trans_type == BaseStrokeVertex::TRANS_CLIPPED) && (_halo == 0.0))
         {
            start_l = v._l;
         }

         dl = start_l - v._l;
         // /*XXX*/assert(dl>=0);

         if (dl < taper_len)
            v._width *= (float)sqrt(1-pow((taper_len - dl)/(taper_len) ,2));
         if (dl < fade_len) 
            v._alpha *= (float)pow((1-pow((fade_len  - dl)/(fade_len)  ,2)),1.5);

         last_l = v._l;
      }
   }

   if (_flare>0.0 || _aflare>0.0)
   {
      double flare_len; 
      double aflare_len;

      last_l = -DBL_MAX;
      for (i=num-1; i >= 0; i--)
      {   
         BaseStrokeVertex& v = _draw_verts[i];

         // If l went backwards, we must be have hit
         // a new, separate segment that arose from
         // an offset with breaks or this 1st time round
         if (v._l > last_l)
         {
            assert(v._vis_state == BaseStrokeVertex::VIS_STATE_TRANSITION);
            start_l = v._lmax;
            flare_len = _flare * start_l;
            aflare_len = _aflare * start_l;
         }

         dl = start_l - v._l;
         if (fabs(dl) < 1e-5) dl = 0;
         // /*XXX*/assert(dl>=0);

         if (dl < flare_len)
            v._width *= (float)sqrt(1-pow((flare_len - dl)/(flare_len) ,2));
         if (dl < aflare_len) 
            v._alpha *= (float)pow((1-pow((aflare_len- dl)/(aflare_len),2)),1.5);

         last_l = v._l;
      }
   }


   if (_halo>0.0)
   {
      double halo_len = _halo * _scale;

      last_l = DBL_MAX;
      for (i=0; i < num; i++)
      {   
         BaseStrokeVertex& v = _draw_verts[i];

         //Start of a segment
         if (v._l < last_l)
         {
            assert(v._vis_state == BaseStrokeVertex::VIS_STATE_TRANSITION);
            if (!((v._vis_reason == BaseStrokeVertex::VIS_REASON_OFF_SCREEN) || 
                  (v._vis_reason == BaseStrokeVertex::VIS_REASON_ENDPOINT) || 
                  (v._vis_reason == BaseStrokeVertex::VIS_REASON_OFFSET) ) ) 
               start_l = v._l;
            else
               start_l = -DBL_MAX;
         }
         //Internal breaks 
         else if ( (v._vis_state == BaseStrokeVertex::VIS_STATE_TRANSITION) &&
                   (v._trans_type == BaseStrokeVertex::TRANS_VISIBLE) && 
                   (!(v._vis_reason == BaseStrokeVertex::VIS_REASON_OFF_SCREEN)))
         {
            start_l = v._l;
         }

         dl = v._l - start_l;
         // /*XXX*/assert(dl>=0);

         if (dl < halo_len)
         {
            double h = (halo_len-dl)/halo_len;
            v._width *= (float)pow((1-pow( h, 2.0)), 0.25);
            v._alpha *= (float)pow((1-pow( h, 2.0)), 5.0 );
         }

         last_l = v._l;
      }

      last_l = -DBL_MAX;
      for (i=num-1; i >= 0; i--)
      {   
         BaseStrokeVertex& v = _draw_verts[i];

         // If l went backwards, we must be have hit
         // a new, separate segment that arose from
         // an offset with breaks or this 1st time round
         if (v._l > last_l)
         {
            assert(v._vis_state == BaseStrokeVertex::VIS_STATE_TRANSITION);
            if (!((v._vis_reason == BaseStrokeVertex::VIS_REASON_OFF_SCREEN) || 
                  (v._vis_reason == BaseStrokeVertex::VIS_REASON_ENDPOINT) || 
                  (v._vis_reason == BaseStrokeVertex::VIS_REASON_OFFSET) ) ) 
               start_l = v._l;
            else
               start_l = DBL_MAX;
         }
         //Internal breaks 
         else if ( (v._vis_state == BaseStrokeVertex::VIS_STATE_TRANSITION) &&
                   (v._trans_type == BaseStrokeVertex::TRANS_CLIPPED) && 
                   (!(v._vis_reason == BaseStrokeVertex::VIS_REASON_OFF_SCREEN)))
         {
            start_l = v._l;
         }

         dl = start_l - v._l;
         // /*XXX*/assert(dl>=0);

         if (dl < halo_len)
         {
            double h = (halo_len-dl)/halo_len;
            v._width *= (float)pow((1-pow( h, 2.0)), 0.2);
            v._alpha *= (float)pow((1-pow( h, 2.0)), 5.0 );
         }

         last_l = v._l;
      }
   }

}


/////////////////////////////////////
// find_closest_t()
/////////////////////////////////////
bool
BaseStroke::find_closest_t(
   CNDCZpt& p,
   double cur_t,
   double &ret,
   int max_iters)
{

   BaseStrokeVertex v;
   double delt_t;
   int i;

   for (int k=0; k<max_iters; k++) 
   {
      i = interpolate_vert_by_t(&v,cur_t);

      NDCZvec delt_p(p - v._base_loc); delt_p[2] = 0;
      NDCZvec tangent(v._dir);         tangent[2] = 0;

      if (tangent * tangent < gEpsAbsMath) 
      {
         err_mesg(ERR_LEV_WARN, "BaseStroke::find_closest_t - WARNING: Tangent too short!!!");  
         return false;
      }

      delt_t = ( (_verts[i+1]._t - _verts[i]._t)/(_verts[i+1]._l - _verts[i]._l) ) * 
                  (tangent * delt_p) / tangent.planar_length();

      cur_t += delt_t;

      if (fabs(delt_t) < 1e-4) 
      {
         ret = cur_t;     
         return true;
      }
   }
   return false;
}

/////////////////////////////////////
// interpolate_vert_by_t()
/////////////////////////////////////
// Used by find_closest_t to evaluate
// the the base path at some t.  Note that
// this function assumes as base verts
// are all 'visible'
int
BaseStroke::interpolate_vert_by_t(
   BaseStrokeVertex *v, 
   double t)
{
   int i=0;

   BaseStrokeVertex* verts[4];

   //This search for the base pair containing t
   //is not efficient.  But this isn't critical.

   if (_verts[0]._t <= t)
   {
      while (_verts[i+1]._t <= t)
      {
         if (i+1 < (_verts.num()-1)) 
            i++;
         else
         {
            //In this case, we're at the rightmost base pair
            //but we still cannot manage to enclose t since
            //t >= _max_t, so stop at this i, i+1 base pair 
            assert( t >= _max_t );
            break;
         }
      }
   }
   else
   {
      //In this case, we're at the leftmost base pair
      //but we still cannot manage to enclose t since
      //t<_min_t, so stop at this i,i+1 base pair
      assert( t < _min_t );
   }

   verts[0] = &(_verts[(i>0)?(i-1):(0)]);
   verts[1] = &(_verts[i]);
   verts[2] = &(_verts[i+1]);
   verts[3] = &(_verts[((i+2)<_verts.num())?(i+2):(i+1)]);

   interpolate_vert(v, verts, ((t- _verts[i]._t) / (_verts[i+1]._t - _verts[i]._t)));

   return i;
}

/////////////////////////////////////
// find_closest_i()
/////////////////////////////////////
int
BaseStroke::find_closest_i(
   CNDCZpt& p)
{
   int i, min_i = 0;
   double d, min_d = (p-_verts[0]._base_loc).planar_length();

   for (i=0; i<_verts.num(); i++)
   {
      if ((d = (p-_verts[i]._base_loc).planar_length()) < min_d)
      {
         min_d = d;
         min_i = i;
      }
   }
   return min_i;
}
  
   
/////////////////////////////////////
// generate_offsets()
/////////////////////////////////////
// Okay, this might not be the most useful
// way to implement this, but consider
// this educational...
BaseStrokeOffsetLISTptr
BaseStroke::generate_offsets(
   CNDCZpt_list& pts,
   const ARRAY<double>& press)
{
   assert(_verts.num()>1);

   assert(pts.num() == press.num());

   int j,i;
   double t_guess, t_closest;
   BaseStrokeVertex v;
   BaseStrokeOffsetLISTptr ol = new BaseStrokeOffsetLIST;

   //Compute the _l values which are the
   //NDC partial lengths that form the
   //actual underlying spline parameter values.
   //This also computes the _t values 
   //that the offsets use to index into
   //the spline curve. Of course, if
   //_gen_t=false, the user supplied
   //_t values are used instead.

   compute_length_l();
   compute_t();

   NDCZpt prev_p;            // for catching duplicate points

   double prev_o_pos = 0.0;  // for comparing distances
                             // between successive offsets

   for (j=0; j<pts.num(); j++)
   {
      if (j>0 && prev_p == pts[j])
      {
         err_mesg(ERR_LEV_WARN, "BaseStroke::generate_offsets() - **Duplicate input pts** Dropping offset!");
         continue;
      }

      t_guess = _verts[find_closest_i(pts[j])]._t;

      if (!find_closest_t(pts[j], t_guess, t_closest, 20))
      {
         err_mesg(ERR_LEV_WARN, "BaseStroke::generate_offsets() - **Failed to converge** Dropping offset!");
         continue;
      }
   
      i = interpolate_vert_by_t(&v, t_closest);

      BaseStrokeOffset o;

      v._dir = v._dir.perpendicular().normalized();
      NDCZvec del = pts[j] - v._base_loc;    del[2]=0.0;

      // del should be parallel to _dir in a perfect world
      // but let's project del onto dir to get the distance.
   
      o._len = (v._dir * del) / VIEW::pix_to_ndc_scale();
      o._pos = (_verts[i]._l + ((_verts[i+1]._l - _verts[i]._l) * 
                                 ((t_closest - _verts[i]._t)/
                                  (_verts[i+1]._t - _verts[i]._t))) 
                                                         ) / _ndc_length;
      o._press = press[j];

      o._type = BaseStrokeOffset::OFFSET_TYPE_MIDDLE;

      // Must enforce minimum t between successive offsets
      // (otherwise an assertion may fail in BaseStroke).
      if (ol->empty() || fabs(o._pos - prev_o_pos) >= 0.001f) 
      {
         ol->add(o);
      }

      prev_o_pos = o._pos;
      prev_p = pts[j];

   }

   if (ol->empty()) 
   {
      err_mesg(ERR_LEV_WARN, "BaseStroke::generate_offsets() - No offsets generated! Skipping.");
      return 0;
   }

   if (ol->num() < 2) 
   {
      err_mesg(ERR_LEV_WARN, "BaseStroke::generate_offsets() - Less than 2 offsets generated! Skipping.");
      return 0;
   }

   (*ol)[0]._type = BaseStrokeOffset::OFFSET_TYPE_BEGIN;
   (*ol)[ol->num()-1]._type = BaseStrokeOffset::OFFSET_TYPE_END;
 
   ol->set_pix_len(_ndc_length/VIEW::pix_to_ndc_scale());

   return ol;
}


/////////////////////////////////////
// closest()
////////////////////////////////////
void
BaseStroke::closest(
   CNDCpt &pt, 
   NDCpt &ret_pt, 
   double &ret_dist)
{
   ret_dist = DBL_MAX;

   NDCpt_list pts;

   for (int i=0; i < _draw_verts.num(); i++) 
   {
      const BaseStrokeVertex& v = _draw_verts[i];

      const bool transition = (v._vis_state == BaseStrokeVertex::VIS_STATE_TRANSITION);

      if ((transition) && (v._trans_type == BaseStrokeVertex::TRANS_VISIBLE))
      {
         pts.clear();
      }

      pts += NDCpt(v._loc);

      if ((pts.num()>0) && (transition) && (v._trans_type == BaseStrokeVertex::TRANS_CLIPPED))
      {
         NDCpt new_pt;
         double new_dist;
         int foo;
         
         pts.closest(pt, new_pt, new_dist, foo);

         if (new_dist < ret_dist)
         {
            ret_dist = new_dist;
            ret_pt = new_pt;
         }
      }
   }   

}
