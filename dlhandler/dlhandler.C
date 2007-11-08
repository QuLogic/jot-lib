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
/***********************************************************************
 * AUTHOR: Loring Holden <lsh>
 *   FILE: dlhandler.C
 *   DATE: Sun Feb 13 14:02:01 2000
 *  DESCR: 
 ***********************************************************************/

#include "std/support.H"
#include "glew/glew.H"

#include "dlhandler.H"
#include "std/config.H"


static bool dl_per_view = Config::get_var_bool("JOT_MULTITHREAD",false,true) ||
                          Config::get_var_bool("JOT_DL_PER_VIEW",false,true);

static bool debug_threads = Config::get_var_bool("JOT_DEBUG_THREADS",false,true);

/***********************************************************************
 * Method : DLhandler::DLhandler
 * Params : 
 * Effects: 
 ***********************************************************************/
DLhandler::DLhandler() 
{
   int size = dl_per_view ? VIEW::num_views() : 1;
   make_dl_big_enough      (size);
   make_dl_stamp_big_enough(size);
}

/***********************************************************************
 * Method : DLhandler::dl
 * Params : 
 * Returns: int
 * Effects: 
 ***********************************************************************/
int
DLhandler::dl(CVIEWptr &v)  const
{
   const int view_id = dl_per_view ? v->view_id() : 0;
   return _dl_array[view_id];
}


/***********************************************************************
 * Method : DLhandler::valid
 * Params : 
 * Returns: bool
 * Effects: 
 ***********************************************************************/
bool
DLhandler::valid(CVIEWptr &v, int cmp_stamp) const
{
   const int view_id = dl_per_view ? v->view_id() : 0;
   ((DLhandler *) this)->make_dl_stamp_big_enough(view_id);
   return ((_dl_stamp_array[view_id] != -1) && 
           (_dl_stamp_array[view_id] >= cmp_stamp));
}


/***********************************************************************
 * Method : DLhandler::invalidate
 * Params : 
 * Returns: void
 * Effects: 
 ***********************************************************************/
void
DLhandler::invalidate() 
{
   for (int i = 0; i< _dl_stamp_array.num(); i++) {
      _dl_stamp_array[i] = -1;
   }
}


/***********************************************************************
 * Method : DLhandler::get_dl
 * Params : 
 * Returns: int
 * Effects: Gets a display list - if one doesn't already exist, one
 *          is created
 ***********************************************************************/
int
DLhandler::get_dl(CVIEWptr &v, int num_dls, int set_stamp) 
{
   const int view_id = dl_per_view ? v->view_id() : 0;

   // If we're all set, don't panic:
   if (valid(v, set_stamp))
      return _dl_array[view_id];

   // Not all set, create a display list:
   make_dl_big_enough(view_id);
   if (_dl_array[view_id] == 0) {
      _dl_array[view_id] = glGenLists(num_dls);
   }

   make_dl_stamp_big_enough(view_id);
   _dl_stamp_array[view_id] = _dl_array[view_id] == 0 ? 0 : set_stamp;
   return _dl_array[view_id];
}

/***********************************************************************
 * Method : DLhandler::close_dl
 * Params : 
 * Returns:
 * Effects: 
 ***********************************************************************/
void
DLhandler::close_dl(CVIEWptr &)
{
   glEndList();
}


/***********************************************************************
 * Method : DLhandler::delete_dl
 * Params : 
 * Returns: void
 * Effects: 
 ***********************************************************************/
void
DLhandler::delete_dl(CVIEWptr &v)
{
   const int view_id = dl_per_view ? v->view_id() : 0;
   if (_dl_array.valid_index(view_id)) {
      if (_dl_array[view_id]) {
         glDeleteLists(_dl_array[view_id], 1);
         _dl_array[view_id] = 0;
      }
      _dl_stamp_array[view_id] = -1;
   }
   return;
}

void
DLhandler::delete_all_dl()
{
   for (int i = 0; i< _dl_array.num(); i++) {
      if (_dl_array[i]) {
         glDeleteLists(_dl_array[i], 1);
         _dl_array[i] = 0;
      }
      _dl_stamp_array[i] = -1;
   }
}


void
DLhandler::make_dl_big_enough(int i)
{
   CriticalSection cs(&_dl_mutex);
   while (!_dl_array.valid_index(i)) {
      if (dl_per_view && debug_threads) {
         cerr << "Adding _dl_array entry" << endl;
      }
      _dl_array += 0;
   }
}

void
DLhandler::make_dl_stamp_big_enough(int i)
{
   CriticalSection cs(&_dl_stamp_mutex);
   while (!_dl_stamp_array.valid_index(i)) {
      if (dl_per_view && debug_threads) {
         cerr << "Adding _dl_stamp_array entry" << endl;
      }
      _dl_stamp_array += -1;
   }
}
