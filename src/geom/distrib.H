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
#ifndef _JOT_GEOM_DISTRIB_H
#define _JOT_GEOM_DISTRIB_H

#include "net/net.H"      
#include "net/io_manager.H"
#include "disp/jot_vars.H" 
#include "disp/cam.H"
#include "geom/geom.H" 
#include "geom/world.H" 
#include "geom/winsys.H" 

/*****************************************************************
 * Distrib
 *****************************************************************/

class DISTRIB : public FD_EVENT,
                public XFORMobs,   public DISPobs,
                public COLORobs,   public EXISTobs,
                public GEOMobs,    public TRANSPobs,  
                public SAVEobs,    public LOADobs,  public CAMobs,     
                /*public BOOTHobs,*/   public HASHobs,
                public JOTvar_obs
{
 protected:
   /******** STATIC MEMBER VARIABLES ********/
   static DISTRIB*   _d;

 protected:
   /******** MEMBER VARIABLES ********/
   bool              _cam_loaded;

 public:
   /******** STATIC MEMBER METHODS ********/
   static bool       cam_loaded     ()             { return _d && _d->_cam_loaded;}
   static DISTRIB*   get_distrib    ();

 public:   
   /******** CONSTRUCTOR/DECONSTRUCTOR *******/
   DISTRIB();

 public:   
   /******** MEMBER METHODS ********/
   void     set_cam_loaded ()             { _cam_loaded = true;      }

 protected:   
   /******** INTERNAL MEMBER METHODS ********/

   bool     load(NetStream &);
   bool     save(NetStream &, bool full_scene);
   
   LOADobs::load_status_t  load_stream(NetStream &, bool full_scene);
   SAVEobs::save_status_t  save_stream(NetStream &, bool full_scene);

   int       interpret(NETenum, NetStream *);
   virtual void sample() {};

   /******** *Obs METHODS ********/
 public:
   virtual void    notify_jot_var (DATA_ITEM *);
   virtual void    notify_xform   (CGEOMptr &, STATE state);
   virtual void    notify         (CGELptr  &, int flag);
   virtual void    notify_exist   (CGELptr  &, int flag);
   virtual void    notify_color   (CGEOMptr &, APPEAR *);
   virtual void    notify_transp  (CGEOMptr &);
   virtual void    notify_geom    (CGEOMptr &);
   virtual void    notify         (CCAMdataptr &data);
   virtual void    notify_hash    (CGELptr &, hashdist *);
   virtual void    notify_load    (NetStream &s, LOADobs::load_status_t &status, bool full_scene);
   virtual void    notify_save    (NetStream &s, SAVEobs::save_status_t &status, bool full_scene);

};

#endif
