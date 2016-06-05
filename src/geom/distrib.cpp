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
#include "distrib.H"
#include "geom/world.H" 
#include "geom/gl_view.H"
#include "geom/texturegl.H"
#include "net/io_manager.H"
#include "std/config.H"

using namespace mlib;

/*****************************************************************
 * Globals
 *****************************************************************/

static bool debug = Config::get_var_bool("DEBUG_DISTRIB",false);

/*****************************************************************
 * Function items for networking
 *****************************************************************/

// 
// These are commands that can be sent over the net or read from a file
//

/*************************************
 * JOTdone
 *************************************/
class JOTdone : public FUNC_ITEM 
{
 public:
   JOTdone() :FUNC_ITEM("DONE") { }
   virtual STDdstream &format(STDdstream &ds) const 
   { 
      return ds << NETflush;
   }
   virtual void        put(TAGformat  &)   const { }
   virtual void        get(TAGformat  &)         { }
};

/*************************************
 * JOTclip_info
 *************************************/
class JOTclip_info : public FUNC_ITEM 
{
public:
  JOTclip_info() : FUNC_ITEM("CLIP_INFO") {}
  virtual void  put(TAGformat &d) const 
  { 
     VIEWptr v = VIEW::peek();
     *d << (int) v->is_clipping() << v->clip_plane();
  }
  virtual void  get (TAGformat &d) 
  {
    int clipping; Wplane cplane;
    *d >> clipping >> cplane;
     VIEWptr v = VIEW::peek();
     v->set_clip_plane(cplane);
     v->set_is_clipping(clipping==1);
  }
};

/*************************************
 * JOTrender_mode
 *************************************/
class JOTrender_mode : public FUNC_ITEM 
{
 public:
   JOTrender_mode() : FUNC_ITEM("RENDER_MODE") { }
   virtual void  put(TAGformat &d) const 
   { 
      *d << VIEW::peek()->rendering();
      err_adv(debug, 
         "JOTrender_mode::put: Pushing rendering mode: '%s'", 
            VIEW::peek()->rendering().c_str());
   }
   virtual void  get (TAGformat &d) 
   {
      string mode;
      *d >> mode;
      VIEW::peek()->set_rendering(mode);
      err_adv(debug, 
         "JOTrender_mode::put: Setting rendering mode from network to: '%s'", 
            mode.c_str());
   }
};

/*************************************
 * JOTupdate_geom
 *************************************/
class JOTupdate_geom : public FUNC_ITEM 
{
 protected:
   GELptr _g;
 public: 
   JOTupdate_geom(CGELptr &g):FUNC_ITEM("UPDATE_GEOM"), _g(g)  { }
   virtual void       put(TAGformat  &d)   const { *d << _g; _g->format(*d);}
   virtual void       get(TAGformat  &d)         
   { 
      *d >> _g;
      if (_g) 
      {
         string str;
         *d >> str; // skip over unneeded class name
         _g->decode(*d);
      }
   }
};

/*************************************
 * JOTsend_geom
 *************************************/
class JOTsend_geom : public FUNC_ITEM 
{
 protected:
   GELptr _g;
 public: 
   JOTsend_geom(CGELptr &g):FUNC_ITEM("SEND_GEOM"), _g(g)  { }
   virtual STDdstream &format(STDdstream &ds) const 
   { 
     _g->format(ds);
     GELdistobs::notify_distrib_obs(ds,_g);
     return ds;
   }
   virtual STDdstream &decode(STDdstream &ds)   
   { 
      err_msg("JOTsend_geom::decode: *ERROR* Should not be here!!");
      return ds;
   }
   virtual void       put(TAGformat  &)      const { }
   virtual void       get(TAGformat  &)            { }
};


/*************************************
 * JOThash
 *************************************/
class JOThash : public FUNC_ITEM 
{
 protected:
   GELptr    _g;
   hashdist *_h;
 public:
   JOThash(CGELptr &g, hashdist *h) : FUNC_ITEM("HASH"), _g(g), _h(h) {}
   virtual void  put(TAGformat &d) const {_h->notify_distrib(*d,_g); }
   virtual void  get(TAGformat &d)       { DATA_ITEM::Decode(*d); }
};

/*************************************
 * JOTtexture
 *************************************/
class JOTtexture : public FUNC_ITEM 
{
 protected:
   GEOMptr    _g;
 public:
   JOTtexture(CGEOMptr &g) : FUNC_ITEM("TEXTURE"), _g(g) {}
   virtual void  put(TAGformat &d) const 
   {
      *d << _g << (int)_g->has_texture();
      if (_g->has_texture())  // bogus!! (put_texture should *always* send _has_texture)
         _g->put_texture(d);
   }
   virtual void  get(TAGformat &d)       
   {
      int      has_texture;
      *d >> _g >> has_texture;
      if (_g && GEOM::isa(_g)) 
      {
         GEOMptr geom((GEOM *)&* _g);
         if (has_texture)  // bogus!! (_has_texture shouldn't be 'implicit' in get/put_texture)
            geom->get_texture(d);
         else
            geom->unset_texture();
      }
   }
};

/*************************************
 * JOTxform
 *************************************/
// Will xform a GEOM
class JOTxform : public FUNC_ITEM 
{
 protected:
   GEOMptr _g;
 public:
   JOTxform(CGEOMptr &g) : FUNC_ITEM("XFORM"), _g(g){}
   virtual void  put(TAGformat &d) const { *d<< _g << _g->xform();}
   virtual void  get (TAGformat &d) 
   {
      Wtransf xform;
      *d >> _g >> xform;
      if (_g && GEOM::isa(_g)) 
      {
         GEOMptr geom((GEOM *)&* _g);
         geom->set_xform(xform);
         XFORMobs::notify_xform_obs(geom, XFORMobs::NET);
      }
   }
};


/*************************************
 * JOTcam
 *************************************/
// Network command for camera location
class JOTcam : public FUNC_ITEM {
 protected:
   CAMdataptr _cam;
   DISTRIB   *_d;
 public:
   JOTcam(CCAMdataptr &cam, DISTRIB *d = nullptr):FUNC_ITEM("CHNG_CAM"), _cam(cam), _d(d) { }
   virtual void  put(TAGformat &d)      const { *d << _cam; }
   virtual void  get(TAGformat &d)
   {
      CAMdataptr cam(VIEWS[0]->cam()->data());
      *d >> cam;
   }
};

/*************************************
 * JOTwin
 *************************************/
// Network command for window things (width,height)
// XXX - Added so we can load window size from files,
// but not presently observed for distribution...
class JOTwin : public FUNC_ITEM 
{
 protected:
   WINSYS*  _win;
 public:
   JOTwin(WINSYS* win):FUNC_ITEM("CHNG_WIN"), _win(win) { }
   virtual void  put(TAGformat &d)      const   { assert(_win); *_win >> *d; }
   virtual void  get(TAGformat &d)              { *(VIEWS[0]->win()) << *d; }
};

/*************************************
 * JOTview
 *************************************/
// Network command for view things (bkg, lights, anim)
// XXX - Added so we can load from files,
// but not presently observed for distribution...
class JOTview : public FUNC_ITEM {
 public: 
   JOTview(CVIEWptr &v) : FUNC_ITEM("CHNG_VIEW"), _v(v) {}

   virtual void put(TAGformat  &d) const { _v->format(*d);
                                          }
   virtual void get(TAGformat  &d) {
      string str;
      *d >> str;
      VIEWS[0]->decode(*d);
   }

 protected:
   VIEWptr _v;
};

/*************************************
 * JOTcreate
 *************************************/
// Network command for creating a new object
class JOTcreate : public FUNC_ITEM {
 protected:
   GELptr  _g;
 public:
   JOTcreate(CGELptr &g) : FUNC_ITEM("CREATE"), _g(g) { }
   virtual void  put(TAGformat &d) const              { *d << _g->name(); }
   virtual void  get(TAGformat &d)
   {
      string name;
      *d >> name;
      GELlist gels(1);
      int i;
      // Get all GEL's with name
      for (i = 0; i < EXIST.num(); i++)
      {
         if (name == EXIST[i]->name()) gels += EXIST[i];
      }
      if (gels.empty()) 
      {
         _g = GELptr();
      }
      else 
      {
         // Destroy all but last GEL with name
         for (i = 0; i < gels.num()-1; i++) 
         {
            err_msg("JOTcreate::get: Destroying a GEL named: '%s'", name.c_str());

            WORLD::destroy(gels[i]);
         }
         // Create last GEL with name
         _g = gels.last();
      }
      if (_g) 
      {
            WORLD::create(_g);
      } 
      else 
      {
         err_msg("JOTcreate::get: Couldn't create: '%s'", name.c_str());
      }
   }
};

/*************************************
 * JOTio
 *************************************/
// Network command issued at start of file IO
// XXX - Added to assist issues with loading/saving streams to/from files 
// but likely not well behaved in distributed networked settings...
class JOTio : public FUNC_ITEM {
   public: 
     JOTio(IOManager *io):FUNC_ITEM("CHNG_IO"), _io(io)  { }
      virtual void       put(TAGformat  &d)   const { _io->format(*d);}
      virtual void       get(TAGformat  &d)   { string str; *d >> str; IOManager::instance()->decode(*d); }
   protected:
      IOManager *_io;
};


/*************************************
 * JOTgrab
 *************************************/
// grab/release an object - for object contention
// this works differently than most funcs because it expects that 
// different things will happen on different clients when messages are
// decoded -- thus global networking is maintained on except to for 
// not repeating a network broadcast of the decoded message.  
//   Most other network operations expect the same result to occur
// on each machine and thus can globally turn off networking when 
// they are decoding.
class JOTgrab: public FUNC_ITEM 
{
 protected:
   GELptr    _g; 
   int       _grab;
 public:
   static GELptr   _save;  // allow networking of everything but 
                           // a repeat of this object's grab operation
   JOTgrab(CGELptr &g, int grab) : FUNC_ITEM("GRAB"), _g(g), _grab(grab) {}
   virtual void  put(TAGformat &d) const { *d << _g << _grab; }
   virtual void  get (TAGformat &d) 
   {
      *d >> _g >> _grab;
      if (_g) 
      {
         _save = _g;
         if (_grab)  
            GRABBED.grab(_g);
         else 
            GRABBED.release(_g);
         _save = nullptr;
      }
   }
};
GELptr JOTgrab::_save = nullptr;


/*************************************
 * JOTcolor
 *************************************/
// Change a GEOM's color
class JOTcolor: public FUNC_ITEM 
{
 protected:
   GEOMptr _g;
 public:
   JOTcolor(CGEOMptr &g) : FUNC_ITEM("COLOR"), _g(g) {}
   virtual void  put(TAGformat &d) const { *d << _g << _g->color();}
   virtual void  get (TAGformat &d)      
   { 
      COLOR    col;
      *d >> _g >> col;
      if (_g) _g->set_color(col); 
   }
};

/*************************************
 * JOTdisplay
 *************************************/
// Display/undisplay an object
class JOTdisplay : public FUNC_ITEM 
{
 protected:
   int    _disp;
   GELptr _g;
 public:
   JOTdisplay(CGELptr &g, int display = 1) : FUNC_ITEM("DISPLAY"), _disp(display), _g(g) {}
   virtual void  put(TAGformat &d) const { *d << _g << _disp; }
   virtual void  get (TAGformat &d)      
   { 
      *d >> _g >> _disp;
      if (_g) 
      {
         if (_disp) WORLD::display  (_g);
         else WORLD::undisplay(_g);
      } 
   }
};

/*************************************
 * JOTdestroy
 *************************************/
// Destroy an object
class JOTdestroy : public FUNC_ITEM 
{
 protected:
   GELptr _g;
 public:
   JOTdestroy(CGELptr &g) : FUNC_ITEM("DESTROY"),_g(g) {}
   virtual void  put(TAGformat &d) const { *d<<_g; }
   virtual void  get (TAGformat &d)      
   { 
      *d>>_g; 
      if(_g) 
      {
         WORLD::destroy(_g);
      }
      else  
      {
         err_msg("JOTdestroy::get: Couldn't find GEL to destroy");
      }
   }

};

/*************************************
 * JOTtransp
 *************************************/
// Change an object's transparency
class JOTtransp : public FUNC_ITEM 
{
 protected:
   GEOMptr _g;
 public:
   JOTtransp(CGEOMptr &g) : FUNC_ITEM("TRANSP"), _g(g) {}
   virtual void  put(TAGformat &d) const 
   { 
      *d << _g << _g->has_transp() << _g->transp(); 
   }
   virtual void  get (TAGformat &d)       
   { 
      int     has_transp;
      double  transp;
      *d >> _g >> has_transp >> transp;
      if (_g) 
      {
         _g->set_transp(transp);
         if (!has_transp) _g->unset_transp();
      }
   }
};

/*****************************************************************
 * DISTRIB
 *****************************************************************/

/////////////////////////////////////////////////////////////////
// Statics 
/////////////////////////////////////////////////////////////////

DISTRIB*       DISTRIB::_d = nullptr;

DISTRIB* DISTRIB::get_distrib()
{
   if (!_d)
   {
      err_adv(debug, "DISTRIB.C::get_distrib(): Instantiating DISTRIB...");
      _d = new DISTRIB();
   }

   return _d;
}

/////////////////////////////////////////////////////////////////
// Methods
/////////////////////////////////////////////////////////////////


/////////////////////////////////////
// Constructor
/////////////////////////////////////
DISTRIB::DISTRIB()
{
   // Creating the following objects is done for the side effect
   // (in FUNC_ITEM constructor) of adding them to the DECODER
   // hash table:
   new JOTupdate_geom(GELptr());
   new JOTsend_geom(GELptr());
   new JOTclip_info();
   new JOTrender_mode();
   new JOThash     (GELptr(), nullptr);
   new JOTxform    (GEOMptr());
   new JOTcreate   (GELptr());
   new JOTcam      (CAMdataptr(), this);
   new JOTwin      (nullptr);
   new JOTio       (nullptr);
   new JOTview     (VIEWptr());
   new JOTgrab     (GELptr(), 0);
   new JOTcolor    (GEOMptr());
   new JOTdisplay  (GELptr());
   new JOTdestroy  (GELptr());
   new JOTtransp   (GEOMptr());
   new JOTtexture  (GEOMptr());

   disp_obs();
   exist_obs();
   save_obs();
   load_obs();
}

/////////////////////////////////////
// interpret()
/////////////////////////////////////
//
// Decodes input stream until JOTdone() is received
//
int
DISTRIB::interpret(
   NETenum    code, 
   STDdstream *sender
   )
{
   int ret = 0;
   switch (code) 
   {
      case NETcontext: 
      {
         static int print_errs = (Config::get_var_bool("PRINT_ERRS",false,true) != 0);
         do 
         {
            DATA_ITEM *di = DATA_ITEM::Decode(*sender);
            // Should print error w/ problem class name if we can't decode
            // (How to get class name nicely?)
            if (di) 
            {
               err_adv((print_errs>0), 
                  "DISTRIB::interpret: Decoded: '%s'", di->class_name().c_str());
               if (di->class_name() == JOTdone().class_name())  break;
            }
            //XXX - Try something new -- bail on failure and return failure code...
            else
            {
               err_msg("DISTRIB::interpret: Could not decode input. Aborting.");
               ret = 1;
               break;
            }
            sender->check_end_delim(); // forces an eof() if done
         } while (!sender->eof());
     }
     brcase NETswap_ack: // only master wall receives NETswap_ack's
     {
        // Used to be CAVE related stuff here...
     }
     brcase NETadd_connection : 
     { 
     
     }
     brdefault: 
     {
         ret = 1;
         err_msg(
            "DISTRIB::interpret: Code: '%d' from '%s' is not NETcontext",
               int(code), sender->name().c_str());
     }
   }
   return ret;
}

/////////////////////////////////////
// load()
/////////////////////////////////////
//
// Load data from a STDdstream
//
// Return true if failed, false if successful
bool
DISTRIB::load(STDdstream &cli)
{

   static int print_errs = (Config::get_var_bool("PRINT_ERRS",false,true) != 0);

   DRAWN.flush();
   DRAWN.buffer();

   bool retval = false;

   err_adv((print_errs>0), 
      "DISTRIB::load: About to check stream...");

   // just start interpreting the stream ...
   err_adv((print_errs>0),
      "DISTRIB::load: About to interpret...");
   retval = !!interpret(NETcontext, &cli);

   DRAWN.flush();
   return retval;
}

/////////////////////////////////////
// load_stream()
/////////////////////////////////////
LOADobs::load_status_t
DISTRIB::load_stream(
   STDdstream &s,
   bool       full_scene)
{
   bool ret;

   LOADobs::load_status_t result;

   string header;

   if (s.iostr())
   {
      err_adv(debug, "DISTRIB::load_stream: Loading ASCII stream...");

      //Should begin with a '#jot' header...
      s >> header;

      if (header == "#jot")
      {
         err_adv(debug,
                 "DISTRIB::load_stream: Founder expected header: '%s'",
                 header.c_str());
         err_adv(debug,
                 "DISTRIB::load_stream: Attempting conventional load...");
         ret = load(s);
         if (!ret)
         {
            result = LOADobs::LOAD_ERROR_NONE;
            err_adv(debug, "DISTRIB::load_stream: ...load successful!");
         }
         else
         {
            result = LOADobs::LOAD_ERROR_JOT;
            err_msg("DISTRIB::load_stream: *LOAD FAILED*");
         }
      }
      else
      {
         err_msg("DISTRIB::load_stream: Not '#jot' header: '%s'", header.c_str());
         err_msg("DISTRIB::load_stream: Attempting to use auxillary LOADERs...");

         ret = LOADER::load(s.name());

         if (!ret)
         {
            err_adv(debug, "DISTRIB::load_stream: Auxillary LOADERs failed!!!");
            err_msg("DISTRIB::load_stream: *LOAD FAILED*");
            result = LOADobs::LOAD_ERROR_READ;
         }
         else
         {
            err_msg("DISTRIB::load_stream: Auxillary LOADERs succeeded.");
            err_adv(debug, "DISTRIB::load_stream: ...load successful!");
            result = LOADobs::LOAD_ERROR_AUX;
         }
      }
   }
   else
   {
      err_msg("DISTRIB::load_stream: **LOAD FAILED** No valid stream or handle!!");
      result = LOADobs::LOAD_ERROR_STREAM;
   }

   return result;
}


/////////////////////////////////////
// notify_load()
/////////////////////////////////////
void      
DISTRIB::notify_load(
   STDdstream     &s,
   load_status_t &status,
   bool           full_scene
   )
{
   if (status == LOADobs::LOAD_ERROR_NONE)
   {
      status = load_stream(s, full_scene);
   }
   else
   {
      err_adv(debug, "DISTRIB::notify_load: Load procedure already in error state. Aborting...");      
   }
}

/////////////////////////////////////
// notify_save()
/////////////////////////////////////
void      
DISTRIB::notify_save(
   STDdstream     &s,
   save_status_t &status,
   bool           full_scene
   )
{
   if (status == SAVEobs::SAVE_ERROR_NONE)
   {
      status = save_stream(s, full_scene);
   }
   else
   {
      err_adv(debug, "DISTRIB::notify_save: Save procedure already in error state. Aborting...");      
   }
}

/////////////////////////////////////
// save_stream()
/////////////////////////////////////
SAVEobs::save_status_t
DISTRIB::save_stream(
   STDdstream   &s,
   bool         full_scene
   )
{
   bool ret;

   SAVEobs::save_status_t result;

   if (s.iostr())
   {
      err_adv(debug, "DISTRIB::save_stream: Saving ASCII stream...");

      ret = save(s, full_scene);

      if (ret)
      {
         result = SAVEobs::SAVE_ERROR_NONE;
         err_adv(debug, "DISTRIB::save_stream: ...save succeeded.");
      }
      else
      {
         result = SAVEobs::SAVE_ERROR_WRITE;
         err_msg("DISTRIB::save_stream: **SAVE FAILED**");
      }
   }
   else
   {
      err_msg("DISTRIB::save_stream: **SAVE FAILED** No valid stream or handle!!");
      result = SAVEobs::SAVE_ERROR_STREAM;
   }
   
   return result;
}

/////////////////////////////////////
// save()
/////////////////////////////////////
//
// Save data to a STDdstream
//
bool
DISTRIB::save(
   STDdstream& s,
   bool       full_scene
   )
{
   int i;

   // Figure out what to save...

   // Get a list of all GEOMs
   GEOMlist geoms;
   for (i = 0; i < EXIST.num(); i++) {
      GEOM* geom = GEOM::upcast(EXIST[i]);
      if (geom) {
         geoms += geom;
      } 
   }

   // Get list of undisplayed objects
   // If no object depends on an undisplayed object, then
   // we don't save it to a file
   CGELlist &drawn = DRAWN;
   GELlist   filter;
   for (i = geoms.num() - 1; i >= 0; i--) {
      if (NETWORK.get(geoms[i]) &&    // object is networked
          !drawn.contains(geoms[i])) { // object isn't drawn
         // Don't save out this object
         filter += geoms[i];
         if (Config::get_var_bool("DEBUG_NET_STREAM",false)) {
            cerr << "DISTRIB::save: filtered out: "
                 << geoms[i]->name()
                 << "\n  networked: "
                 << (NETWORK.get(geoms[i]) ? "yes" : "no")
                 << ", DRAWN index: "
                 << drawn.get_index(geoms[i])
                 << endl;
         }
      }
   }

   // Now save things...

   s << "#jot\n";

   //Bail out early if the IO already failed...
   if (s.fail())
   {
      return false;
   }

   if (full_scene)
   {
      s << JOTio(IOManager::instance());

      // GEOMS
      for (i = 0; i < geoms.num(); i++) {
         if (NETWORK.get(geoms[i]) &&
             !filter.contains(geoms[i])) {
            if (debug) {
               cerr << "DISTRIB::save: Writing GEOM: "
                    << geoms[i]->name() << endl;
            }
            s << JOTsend_geom(geoms[i]) << JOTcreate(geoms[i]); 
            err_adv(debug, "DISTRIB::save: ...finished writing GEOM.");
         } 
      }

      // Other stuff
      s << JOTcam(VIEW::peek()->cam()->data());
      s << JOTwin(VIEW::peek()->win());
      s << JOTview(VIEW::peek());
   } else {
      // GEOMS
      for (i = geoms.num() - 1; i >= 0 ; i--) {
         if (NETWORK.get(geoms[i]) &&
             filter.get_index(geoms[i]) == BAD_IND) {
            err_adv(debug, "DISTRIB::save: Writing GEOM Update'%s'..." ,
                    geoms[i]->name().c_str());
            s << JOTupdate_geom(geoms[i]); 
            err_adv(debug, "DISTRIB::save: ...finished writing GEOM Update.");
         } 
      }

      s << JOTcam(VIEW::peek()->cam()->data());
   }
#ifndef WIN32
   // the following is needed on linux/mac; but on windows
   // its sole effect is to print out '3':
   s << JOTdone();
#endif
   return !s.fail();
}

/////////////////////////////////////
// notify_exist()
/////////////////////////////////////
void      
DISTRIB::notify_exist(
   CGELptr &g,
   int      f
   )
{
   if (f)  {
      if (NETWORK.get(g)) {
         DATA_ITEM::add_decoder(g->name(), (DATA_ITEM *)&*g, 0);
      }
   }
}

/////////////////////////////////////
// notify()
/////////////////////////////////////
void      
DISTRIB::notify(
   CGELptr &g,
   int      flag
   )
{
   int i;
   if (flag) {
      for (i = 0; i < VIEWS.num(); i++)
         VIEWS[i]->display(g);
   } else {
      for (i = 0; i < VIEWS.num(); i++)
         VIEWS[i]->undisplay(g);
   }
}

