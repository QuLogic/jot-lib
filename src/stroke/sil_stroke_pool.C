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
#include "gtex/gl_extensions.H"
#include "sil_stroke_pool.H"
#include "npr/sil_and_crease_texture.H"

/*****************************************************************
 * SilStrokePool
 *****************************************************************/

TAGlist*  SilStrokePool::_ssp_tags = 0;

/////////////////////////////////////////////////////////////////
// SilStrokePool Methods
/////////////////////////////////////////////////////////////////

#define DEFAULT_COHER_GLOBAL        true
#define DEFAULT_COHER_SIGMA_ONE     false
#define DEFAULT_COHER_FIT_TYPE      SilAndCreaseTexture::SIL_FIT_OPTIMIZE
#define DEFAULT_COHER_COVER_TYPE    SilAndCreaseTexture::SIL_COVER_TRIMMED
#define DEFAULT_COHER_PIX           48.0f
#define DEFAULT_COHER_WF            1.0f
#define DEFAULT_COHER_WS            1.0f
#define DEFAULT_COHER_WB            1.0f 
#define DEFAULT_COHER_WH            1.0f
#define DEFAULT_COHER_MV            2
#define DEFAULT_COHER_MP            5
#define DEFAULT_COHER_M5            5
#define DEFAULT_COHER_HJ            3
#define DEFAULT_COHER_HT            15


/////////////////////////////////////
// tags()
/////////////////////////////////////

CTAGlist &
SilStrokePool::tags() const
{
   if (!_ssp_tags) {
      _ssp_tags = new TAGlist;
      *_ssp_tags += BStrokePool::tags();    

      *_ssp_tags += new TAG_val<SilStrokePool,bool>(
         "coher_global",
         &SilStrokePool::coher_global_);
      *_ssp_tags += new TAG_val<SilStrokePool,bool>(
         "coher_sigma_one",
         &SilStrokePool::coher_sigma_one_);
      *_ssp_tags += new TAG_val<SilStrokePool,int>(
         "coher_fit_type",
         &SilStrokePool::coher_fit_type_);
      *_ssp_tags += new TAG_val<SilStrokePool,int>(
         "coher_cover_type",
         &SilStrokePool::coher_cover_type_);
      *_ssp_tags += new TAG_val<SilStrokePool,float>(
         "coher_pix",
         &SilStrokePool::coher_pix_);
      *_ssp_tags += new TAG_val<SilStrokePool,float>(
         "coher_wf",
         &SilStrokePool::coher_wf_);
      *_ssp_tags += new TAG_val<SilStrokePool,float>(
         "coher_ws",
         &SilStrokePool::coher_ws_);
      *_ssp_tags += new TAG_val<SilStrokePool,float>(
         "coher_wb",
         &SilStrokePool::coher_wb_);
      *_ssp_tags += new TAG_val<SilStrokePool,float>(
         "coher_wh",
         &SilStrokePool::coher_wh_);
      *_ssp_tags += new TAG_val<SilStrokePool,int>(
         "coher_mv",
         &SilStrokePool::coher_mv_);
      *_ssp_tags += new TAG_val<SilStrokePool,int>(
         "coher_mp",
         &SilStrokePool::coher_mp_);
      *_ssp_tags += new TAG_val<SilStrokePool,int>(
         "coher_m5",
         &SilStrokePool::coher_m5_);
      *_ssp_tags += new TAG_val<SilStrokePool,int>(
         "coher_hj",
         &SilStrokePool::coher_hj_);
      *_ssp_tags += new TAG_val<SilStrokePool,int>(
         "coher_ht",
         &SilStrokePool::coher_ht_);

   }
   return *_ssp_tags;
}

/////////////////////////////////////
// Constructor
/////////////////////////////////////
SilStrokePool::SilStrokePool(OutlineStroke* p) : 
   BStrokePool(p) 
{ 
   _write_strokes = false; 

   _coher_global     = DEFAULT_COHER_GLOBAL;    
   _coher_sigma_one  = DEFAULT_COHER_SIGMA_ONE; 
   _coher_fit_type   = DEFAULT_COHER_FIT_TYPE;  
   _coher_cover_type = DEFAULT_COHER_COVER_TYPE;
   _coher_pix        = DEFAULT_COHER_PIX;       
   _coher_wf         = DEFAULT_COHER_WF;        
   _coher_ws         = DEFAULT_COHER_WS;        
   _coher_wb         = DEFAULT_COHER_WB;              
   _coher_wh         = DEFAULT_COHER_WH;        
   _coher_mv         = DEFAULT_COHER_MV;        
   _coher_mp         = DEFAULT_COHER_MP;        
   _coher_m5         = DEFAULT_COHER_M5;        
   _coher_hj         = DEFAULT_COHER_HJ;        
   _coher_ht         = DEFAULT_COHER_HT;

   _path_index_stamp =  (uint)-1;     
   _group_index_stamp = (uint)-1;  
   
   _selected_stroke_path_index = (uint)-1;
   _selected_stroke_group_index = (uint)-1;
   _selected_stroke_path_index_stamp = (uint)-1;     
   _selected_stroke_group_index_stamp = (uint)-1;    

}

/////////////////////////////////////
// draw_flat()
/////////////////////////////////////
void
SilStrokePool::draw_flat(CVIEWptr& v) 
{

   BStrokePool::draw_flat(v);

}

/////////////////////////////////////
// set_prototype_internal()
/////////////////////////////////////
int
SilStrokePool::set_prototype_internal(OutlineStroke* p)
{

   double new_mesh_size = p->get_original_mesh_size();
   double new_period = (p->get_offsets())?
                           (p->get_offsets()->get_pix_len()):
                           ((double)max(1.0f,(p->get_angle())));


   _prototypes[_edit_proto] = p;

   if (_edit_proto == 0)
   {
      if ((_cur_mesh_size != new_mesh_size) || (_cur_period != new_period))
      {
         for (int i=1; i<_prototypes.num(); i++)
         {
            _prototypes[i]->set_original_mesh_size(new_mesh_size);
            if (_prototypes[i]->get_offsets())
               _prototypes[i]->get_offsets()->set_pix_len(new_period);
            else
               _prototypes[i]->set_angle((float)new_period);
         }
         return BSTROKEPOOL_SET_PROTOTYPE__NEEDS_FLUSH;
      }
      else
      {
         for (int i=1; i<_prototypes.num(); i++)
         {
            assert(_prototypes[i]->get_original_mesh_size() == _cur_mesh_size);
            if (_prototypes[i]->get_offsets())
               assert(_prototypes[i]->get_offsets()->get_pix_len() == _cur_period);
            else
               assert(_prototypes[i]->get_angle() == _cur_period);
         }

         return 0;
      }
   }
   else
   {
      if ((_cur_mesh_size != new_mesh_size) || (_cur_period != new_period))
      {
         p->set_original_mesh_size(_cur_mesh_size);
         if (p->get_offsets())
            p->get_offsets()->set_pix_len(_cur_period);
         else
            p->set_angle((float)_cur_period);

         return BSTROKEPOOL_SET_PROTOTYPE__NEEDED_ADJUSTMENT;
      }
      else
      {
         return 0;
      }
   }
}
/* end of file sil_stroke_pool.C */
