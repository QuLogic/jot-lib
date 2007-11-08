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

//XXX - Used to be DLL functions...
extern "C" void distrib();
extern "C" void distrib_startnet(int);
extern "C" void distrib_client(FD_MANAGER *, char *, int);
extern "C" void distrib_cam(STDdstream *, CCAMdataptr &);
extern "C" void distrib_win(STDdstream *, WINSYS *);
extern "C" void distrib_view(STDdstream *, CVIEWptr &);
extern "C" void distrib_done(STDdstream *);
extern "C" void distrib_send_geom(CGEOMptr &);
extern "C" void distrib_render_mode();
extern "C" void distrib_clip_info();
extern "C" void distrib_display_geom( CGELptr &, int);

/*****************************************************************
 * Distrib
 *****************************************************************/

class DISTRIB : public Network,  
                public XFORMobs,   public GRABobs,  public DISPobs,
                public COLORobs,   public EXISTobs,
                public GEOMobs,    public TRANSPobs,  
                public SAVEobs,    public LOADobs,  public CAMobs,     
                /*public BOOTHobs,*/   public HASHobs,  public TEXTUREobs, 
                public JOTvar_obs
{
 protected:
   /******** STATIC MEMBER VARIABLES ********/
   static DISTRIB*   _d;

 protected:
   /******** MEMBER VARIABLES ********/
   bool              _cam_loaded;
   bool              _processing_gate;

 public:
   /******** STATIC MEMBER METHODS ********/
   static bool       cam_loaded     ()             { return _d && _d->_cam_loaded;}
   static void       set_distrib    (DISTRIB *d)   { _d = d; }
   static DISTRIB*   get_distrib    ()             { return _d; }

 public:   
   /******** CONSTRUCTOR/DECONSTRUCTOR *******/
   DISTRIB(FD_MANAGER *manager);

 public:   
   /******** MEMBER METHODS ********/
   void     set_cam_loaded ()             { _cam_loaded = true;      }
   void     set_processing_gate(bool pg)  { _processing_gate = pg;   }

 protected:   
   /******** INERNAL MEMBER METHODS ********/
   bool     should_save(CGEOMptr &g);

   bool     load(NetStream &);
   bool     save(NetStream &, bool to_file, bool full_scene);
   
   LOADobs::load_status_t  load_stream(NetStream &, bool from_file, bool full_scene);
   SAVEobs::save_status_t  save_stream(NetStream &, bool to_file,   bool full_scene);

 public:   
   /******** Network METHODS ********/
   virtual int       interpret(NETenum, NetStream *);
   virtual void      add_client(NetStream  *);
   virtual void      remove_stream(NetStream  *);
   int               processing() const            { return _processing_gate && Network::processing(); }

   /******** *Obs METHODS ********/
 public:
   virtual void    notify_jot_var (DATA_ITEM *);
   virtual void    notify_xform   (CGEOMptr &, STATE state);
   virtual void    notify_grab    (CGELptr  &, int flag);
   virtual void    notify         (CGELptr  &, int flag);
   virtual void    notify_exist   (CGELptr  &, int flag);
   virtual void    notify_color   (CGEOMptr &, APPEAR *);
   virtual void    notify_transp  (CGEOMptr &);
   virtual void    notify_geom    (CGEOMptr &);
   virtual void    notify         (CCAMdataptr &data);
   virtual void    notify_hash    (CGELptr &, hashdist *);
   virtual void    notify_texture (CGEOMptr &);
   virtual void    notify_load    (NetStream &s, LOADobs::load_status_t &status, bool from_file, bool full_scene);
   virtual void    notify_save    (NetStream &s, SAVEobs::save_status_t &status, bool to_file, bool full_scene);

};

#endif
