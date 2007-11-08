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
#include "std/support.H"
#include "glew/glew.H"

#include "dev/devpoll.H"
#include "disp/colors.H"
#include "geom/body.H"
#include "geom/text2d.H"
#include "geom/line3d.H"
#include "geom/world.H"
#include "geom/winsys.H"

#include "std/config.H"
#include "std/thread.H"
#include "std/time.H"
#include "std/run_avg.H"

using namespace mlib;

WORLD*  WORLD::_w       = 0;
bool    WORLD::_is_over = false;

bool debug_threads = Config::get_var_bool("JOT_DEBUG_THREADS") != 0;

LOADER *LOADER::_all;

LOADER::LOADER()
{
   _next = _all;
   _all  = this;
}

bool
LOADER::load(
   Cstr_ptr &path
   )
{
   bool loaded = 0;
   for (LOADER *l = _all; l != 0 && !loaded; l = l->_next) {
      loaded = l->try_load(path);
   }
   return loaded;
}

//
//  When a stream encounters a data object identified by a name
// that doesn't match the class name of the object registered to
// decode it, then this function is called to resolve the conflict.  
//
// It's possible the registered object actually *is* the object
// to be decoded in which case this verifies the match and returns
// the object.   (In this case, the object to be decoded will only
// have its name on the stream and no additional data).
//
// Alternatively, it's possible that the object to be decoded does 
// exist in the EXIST database, in which case nothing needs to 
// explicitly decode the object -- just return the object from EXIST.
// (Again, the only data on the stream will be the object's name).
//
// If these two fail, then the object can't be decoded.
//
DATA_ITEM*
WORLD::_default_decoder(
   STDdstream& , 
   Cstr_ptr&  name, 
   DATA_ITEM* hash
   )
{
   // Use the object that's registered to decode 'name' only if that 
   // object's name is 'name' also.
   GELptr gel = GEL::upcast(hash);
   if (gel && gel->name() == name)
      return hash;

   // If nothing was registered to decode 'name', then check to see
   // if an object called 'name' exists and return it if it does.
   gel = EXIST.lookup(name);
   if (gel)
      return gel;

   // Report failure:
   cerr << "WORLD::_default_decoder: can't find object named "
        << name << endl;
   if (hash)
      cerr << hash->class_name() << " vs. " << name << endl;

   return 0;
}

//
//
// WORLD
//
//
WORLD::WORLD() : _hash(10)
{
   // Set up the clean up routine
   atexit(WORLD::Clean_On_Exit);

   DATA_ITEM::set_default_decoder(_default_decoder);

#ifdef USE_PTHREAD
   _tsync = 0;
   _threads = 0;
   _doMultithread = Config::get_var_bool("JOT_MULTITHREAD",false,true);
#endif
}

// call 'exec_poll()' on all pollable device objects
void
WORLD::poll(void)
{
   ARRAY<DEVpoll *> &p = DEVpoll::pollable();
   for(int i=0;i<p.num();i++)
      p[i]->exec_poll();
}

#ifdef USE_PTHREAD
class RenderThread : public Thread {
 public:
   void set_view_num(int view_num) { _view_num = view_num; }
 protected:
   int _view_num;
   void threadProc();
};

void
RenderThread::threadProc()
{
   ThreadSync *sync = WORLD::get_world()->get_threadsync();
   bool need_set = true;
#ifdef sgi
   // FIXME - should make sure that there is a cpu per VIEW
   int retval = 0;
   if ((retval = pthread_setrunon_np(_view_num+1)) != 0) {
      // On the SGI errno is cleared, so we have to use strerror
      err_msg("RenderThread::threadProc - pthread_setrunon_np: %s", 
              strerror(retval));
   }
#endif

   for (;;) {
      if (debug_threads) fprintf(stderr, "slave-%d syncing once\n", _view_num);
      sync->wait();
      if (need_set) {
         ThreadObs::notify_render_thread_obs(VIEWS[_view_num]);
         need_set = false;
      }
      if (debug_threads) fprintf(stderr, "slave-%d painting\n", _view_num);
      VIEWS[_view_num]->paint();
      // this second sync shouldn't be needed on a multipipe IR
      if (debug_threads) fprintf(stderr, "slave-%d syncing twice\n", _view_num);
      sync->wait();
      if (debug_threads) fprintf(stderr, "slave-%d swapping\n", _view_num);
      VIEWS[_view_num]->impl()->swap_buffers();
      if (debug_threads) fprintf(stderr,"slave-%d syncing thrice\n", _view_num);
      sync->wait();
   }
}
#endif

void
WORLD::draw(void)
{
#ifdef USE_PTHREAD
   if (_doMultithread) {
      if (!_threads) {
         _tsync = new ThreadSync(VIEWS.num()+1);
         _threads = new RenderThread[VIEWS.num()];
         pthread_setconcurrency(VIEWS.num());
         for (int i=0; i < VIEWS.num(); i++) {
            _threads[i].set_view_num(i);
            _threads[i].start();
         }
      }

      if (debug_threads) fprintf(stderr, "master unlocking\n");
      VIEWS[0]->win()->unlock();
      VIEWS[0]->win()->unlock();
      if (debug_threads) fprintf(stderr, "master syncing once\n");
      _tsync->wait();
      // render threads paint
      if (debug_threads) fprintf(stderr, "master syncing twice\n");
      _tsync->wait();
      // render threads swap
      if (debug_threads) fprintf(stderr, "master syncing thrice\n");
      _tsync->wait();
      // need to wait until swaps finish so we can grab the display again
      if (debug_threads) fprintf(stderr, "master locking\n");
      VIEWS[0]->win()->lock();
      VIEWS[0]->win()->lock();
   } else
#endif
      {
         for (int i =0; i < VIEWS.num(); i++) {
            VIEWS[i]->paint();
         }
      }
}

// Deactivate all our connects, turn off stereo, and quit
void
WORLD::clean_on_exit() const
{
   static bool debug = Config::get_var_bool("DEBUG_CLEAN_ON_EXIT",false);
   err_adv(debug, "WORLD::clean_on_exit");

   // Kill off all the ref counted GELs in the EXIST list to
   // ensure they properly die off while all the required static
   // observed HASHes are still around. See draw/floor.H for an
   // example where one must explicitly add another clean_on_exit
   // routine via onexit() to ensure extra static ref counted
   // GEL pointers are killed off to ensure object destruction
   // prior to total termination.

   while (!EXIST.empty()) {
      GELptr g = EXIST.pop();
      if (debug)
         cerr << " Trashing '" << g->name() << "' "
              << "(" << g->class_name() << ")" << endl;
      destroy(g, false);
   }

   int i;
   for (i = 0; i<_fds.num(); i++)
      _fds[i]->deactivate();
   for (i = 0; i < VIEWS.num(); i++)
      VIEWS[i]->stereo(VIEWimpl::NONE);
}

void
WORLD::quit() const
{
   // Report on silhouette statistics if asked
   if (Config::get_var_bool("JOT_REPORT_SIL_STATS",false,true)) {
      extern RunningAvg<double> rand_secs;  // secs per randomized extraction
      extern RunningAvg<double> brute_secs; // secs per brute-force extraction
      extern RunningAvg<double> zx_secs;    // secs per zx extraction (rand)
      extern RunningAvg<double> rand_sils;  // avg num sils found randomized
      extern RunningAvg<double> brute_sils; // avg num sils found brute-force
      extern RunningAvg<double> zx_sils;    // avg num sils found zx (rand) 
      extern RunningAvg<double> all_edges;  // avg number of total mesh edges

      if (!(isZero( all_edges.val()) &&
            isZero(brute_secs.val()) &&
            isZero( rand_secs.val()) &&
            isZero(   zx_secs.val()))) {

         err_msg("\n**** Silhouette statistics ****\n");
         err_msg("Total number of edges:  %d", (int)all_edges.val());

         if (!isZero(brute_secs.val())) {
            err_msg("Brute force:");
            err_msg("  Avg sils extracted:     %d", (int)brute_sils.val());
            err_msg("  Seconds per extraction: %f", brute_secs.val());
            err_msg("  Extractions per second: %f", 1.0/brute_secs.val());
         }

         if (!isZero(rand_secs.val() > 0)) {
            err_msg("Randomized:");
            err_msg("  Avg sils extracted:     %d", (int)rand_sils.val());
            err_msg("  Seconds per extraction: %f", rand_secs.val());
            err_msg("  Extractions per second: %f", 1.0/rand_secs.val());
            err_msg("Speedup: %2.1f", brute_secs.val()/rand_secs.val());
         }

         if (!isZero(brute_secs.val())) {
            err_msg("ZX:");
            err_msg("  Avg segments extracted: %d", (int)zx_sils.val());
            err_msg("  Seconds per extraction: %f",      zx_secs.val());
            err_msg("  Extractions per second: %f",  1.0/zx_secs.val());
         }

         err_msg("\n*******************************\n");
      }
   }

   exit(0);
}

GEOMptr
map_obj(
   CGEOMptr  &name,
   CGEOMlist &mapfrom,
   CGEOMlist &mapto
   )
{
   for (int n = 0; n < mapfrom.num(); n++)
      if (name == mapfrom[n])
         return mapto[n];

   return GEOMptr();
}


GEOMptr
WORLD::lookup(
   Cstr_ptr &s 
   )
{   
   GELptr g = EXIST.lookup(s);
   if (GEOM::isa(g))
      return GEOMptr((GEOM *) &* g);
   return GEOMptr();
}

GELptr
WORLD::show(CWpt& p, double width, CCOLOR& c, double alpha, bool depth_test)
{
   LINE3Dptr line = new LINE3D;
   line->add(p);
   line->set_width(width);
   line->set_color(c);
   line->set_alpha(alpha);
   line->set_no_depth(!depth_test);
   create(line,false);
   return line;
}

GELptr 
WORLD::show_pts(
   CWpt_list& pts, double width, CCOLOR& col, double alpha, bool depth_test
   )
{
   // XXX - Should have dedicated GEL class for this.
   //       This is done in a hurry to get something that
   //       works but is not efficient.

   for (int i=0; i<pts.num(); i++)
      show(pts[i], width, col, alpha, depth_test);

   return 0;
}

GELptr
WORLD::show(
   CWpt& a, CWpt& b, double width, CCOLOR& c, double alpha, bool depth_test
   )
{
   LINE3Dptr line = new LINE3D;
   line->add(a);
   line->add(b);
   line->set_width(width);
   line->set_color(c);
   line->set_alpha(alpha);
   line->set_no_depth(!depth_test);
   create(line,false);
   return line;
}

GELptr 
WORLD::show_polyline(
   CWpt_list& pts, double width, CCOLOR& col, double alpha, bool depth_test
   )
{
   LINE3Dptr line = new LINE3D;
   line->add(pts);
   line->set_width(width);
   line->set_color(col);
   line->set_alpha(alpha);
   line->set_no_depth(!depth_test);
   create(line,false);

   return 0;
}

/*******************************************************
 * XF_DRAW:
 * 
 *   Draws the coordinate frame described by a Wtransf.
 ********************************************************/
class XF_DRAW : public GEOM {
 public:
   //******** MANAGERS ********
   XF_DRAW(CWtransf& xf, double axis_length) :
      _xf(xf), _axis_length(axis_length) {}
   
   //******** GEOM METHODS ********
   virtual int draw(CVIEWptr &v) {
      if (!_xf.origin().in_frustum())
         return 0;

      // Draw x, y, and z axes in yellow, blue, and tan

      glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_CURRENT_BIT);
      glDisable(GL_LIGHTING);   // GL_ENABLE_BIT
      glDisable(GL_DEPTH_TEST); // GL_ENABLE_BIT
      glLineWidth(2);           // GL_LINE_BIT

      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();
      glMultMatrixd(_xf.transpose().matrix());

      double s = world_length(_xf.origin(), _axis_length);

      glBegin(GL_LINES);

      glColor3dv(Color::blue.data());  // GL_CURRENT_BIT
      glVertex3d(0,0,0);
      glVertex3d(s,0,0);

      glColor3dv(Color::yellow.data());// GL_CURRENT_BIT
      glVertex3d(0,0,0);
      glVertex3d(0,s,0);

      glColor3dv(Color::red.data());   // GL_CURRENT_BIT
      glVertex3d(0,0,0);
      glVertex3d(0,0,s);

      glEnd();

      glPopMatrix();

      glPopAttrib();

      return 1; 
   }

 protected:
   Wtransf _xf;
   double  _axis_length;     // Target screen length for axes
};

GELptr
WORLD::show(CWtransf& xf, double axis_length)
{
   GELptr ret = new XF_DRAW(xf, axis_length);
   create(ret,false);
   return ret;
}

class REF_CLASS(WMSG) : public FRAMEobs {
  protected:
   TEXT2Dptr    _text;
   double       _end_time;
   static WMSG *_msg;

  public:
   WMSG(CTEXT2Dptr &msg) : _text(msg), _end_time(0) { _msg = this;}
   int tick() {
      if (_end_time && the_time() > _end_time) {
         _text->set_string(NULL_STR);
         _msg = 0;
         return -1;
      } else return 1;
   }
   void elapsed(double sec) {_end_time = sec > 0 ? the_time() + sec : 0;}
   static WMSG *msg() {return _msg;}
};

WMSG *WMSG::_msg=0;
TEXT2Dptr msgtext;

void
WORLD::_Message(
   Cstr_ptr &str,
   double   secs,
   CXYpt    &pos
   )
{   
   _Multi_Message(0);
   if (!msgtext) {
      msgtext = new TEXT2D(unique_name("msg"), str, pos);
      GEOMptr g = msgtext;
      g->set_color(Color::firebrick);
      NETWORK.set(g, 0);
      create(g, false);
   } else {
      msgtext->set_string(str);
      msgtext->set_loc   (pos);
   }

   if (pos == XYpt(0,.9))
      msgtext->centered() = true;
   else
      msgtext->centered() = false;

   if (!WMSG::msg()) {
      new WMSG(msgtext);
      FRAMEobsptr m(WMSG::msg());
      _w->schedule(m);
   }
   WMSG::msg()->elapsed(secs);
}

/*multiple messages can be displayed on multiple lines.  They are formatted
  to fit the screen*/
const int MAXLINES = 6;
class REF_CLASS(WMMSG) : public FRAMEobs {
  protected:
   TEXT2Dptr    _text[MAXLINES];
   double       _end_time;
   int          _num_msgs;
   static WMMSG *_msg;

  public:
   WMMSG(TEXT2Dptr msg[]) : _end_time(0) {
      int i;
      for (i = 0; ((msg[i])&&(i < MAXLINES)); i++) {
         _text[i] = *(new TEXT2Dptr(msg[i])); 
      }
      _num_msgs = i;          
      _msg = this;
   } 

   int tick() {
      if (_end_time && the_time() > _end_time) {
         //clear away all messages begin displayed
         str_list list;
         list += "";
         WORLD::multi_message(list);
         for (int i = 0; i < _num_msgs; i++) 
            _text[i]->set_string(NULL_STR);

         _msg = 0;
         return -1;
      } else return 1;
   }
   void elapsed(double sec) {_end_time = sec > 0 ? the_time() + sec : 0;}
   static WMMSG *msg() {return _msg;}
};

WMMSG *WMMSG::_msg=0;
TEXT2Dptr mmsgtext[MAXLINES];

//returns the location of the next instance of chr in the string
int
WORLD::get_next(Cstr_ptr &str, int loc, char chr) {
   while (str[loc] != '\0') {
      loc++;
      if ((str[loc] == chr)||(str[loc] == '\0'))
         return loc;
   }
   return 1000;
}

/*given a string, returns an array such that each of its elements contains
  a maximum of line_length characters*/
str_list
WORLD::format_str(Cstr_ptr &str, const int line_length)
{
   int i = 0, len = 0;
   char *line = new char[line_length];
   str_list formatted;
   while (str[len] != '\0') {
      if (str[i] == ' ') len++; //go on to next character if new line
      for (i = len; i < len + line_length; i++) {
         //conditions that end the line
         if (((str[i] == ' ')&&(get_next(str, i, ' ') >= len+line_length))||
             (str[i] == '\0'))
            break;            
         //add a good charcter to the output string
         line[i - len] = str[i];
      }
      line[i - len] = '\0';
      formatted += line;
      //this many characters formatted
      len = i;
   } 
   delete [] line;
   return formatted;
}

void
WORLD::_Multi_Message(
   Cstr_list &str,
   double     secs,
   CXYpt     &pos
   )
{
   int i, w, h;
   //read each string into a single array
   str_list formatted;
   TEXT2Dptr msg;
   VIEW::peek_size(w, h);
   int line_length = w / 10;
   for (i = 0; i < str.num(); i++) 
      formatted.operator+=(format_str(str[i], line_length));


   //delete old messages
   if (msgtext) 
      msgtext->set_string(NULL_STR);
   for (i = 0; i < MAXLINES; i++)
      if (mmsgtext[i])
         mmsgtext[i]->set_string(NULL_STR);

   XYpt pos2 = pos, jVec(0, -.08);

   for (i = 0; ((i < formatted.num())&&(i < MAXLINES)); i++) 
      if (!mmsgtext[i]) {
         msg = new TEXT2D(unique_name("msg"), formatted[i], pos2);
         GEOMptr g = msg;
         g->set_color(Color::firebrick);
         NETWORK.set(g, 0);
         create(g);
         msg->centered() = true;
         mmsgtext[i] = msg;
         pos2 += jVec;
      } else {    //move text if it is currently being shown 
         mmsgtext[i]->set_string(formatted[i]);
         mmsgtext[i]->set_loc   (pos2);
         pos2 += jVec;
      }

   if (!WMMSG::msg()) {
      new WMMSG(mmsgtext);
      FRAMEobsptr m(WMMSG::msg());
      _w->schedule(m);
   }
   WMMSG::msg()->elapsed(secs);
}

// Calls doit() on a command, adds it to the undo list,
// and clears the redo list.
// 
void
WORLD::_add_command(COMMANDptr c)
{
   if (!c) {
      err_msg("WORLD::_add_command: null command");
      return;
   }
   if (Config::get_var_bool("DEBUG_JOT_CMD")) {
      cerr << "Adding command: ";
      c->print();
      cerr << endl;
   }

   _clear_redoable();
   c->doit();
   _undoable += c;
}

// Pops a command from the undoable list, calls
// undo on it, and appends it to the redoable list.
//
void
WORLD::_undo()
{
   // if there are no commands to undo, notify user and return
   if (_undoable.empty()) {
      WORLD::message("Can't undo anymore");
      return;
   } else {
      WORLD::message("Undo");
   }
   
   COMMANDptr c = _undoable.pop(); 
   if (!c->undoit()) {
      
   }
   _redoable += c;

   if (Config::get_var_bool("DEBUG_JOT_CMD")) {
      cerr << "Undoing command: ";
      c->print();
      cerr << endl;
   }
}

// Pops a command from the redoable list, calls
// redo on it, and appends it to the undoable list.
//
void
WORLD::_redo()
{
   // if there are no commands to redo, notify user and return
   if (_redoable.empty()) {
      WORLD::message("Can't redo anymore");
      return;
   } else {
      WORLD::message("Redo");
   }

   COMMANDptr c = _redoable.pop(); 
   c->doit();
   _undoable += c;

   if (Config::get_var_bool("DEBUG_JOT_CMD")) {
      cerr << "Redoing command: ";
      c->print();
      cerr << endl;
   }
}

// Clears the redoable list
//
void
WORLD::_clear_redoable()
{
   while( !_redoable.empty() )
      _redoable.pop()->clear();
   _redoable.clear();
}

// print the current command stack (for debugging)
//
void
WORLD::_print_command_list()
{
   COMMANDptr c;
   for (int i=0; i<_undoable.num(); i++) {
      _undoable[i]->print();
      cerr << endl;
   }
}



void       
WORLD::create(CGELptr &o, bool display_undoable)  
{ 
   if (NETWORK.is_default(o))
      NETWORK.set(o,1);
  
   if (! EXIST.contains(o) )
      EXIST.add(o);
   EXISTobs::notify_exist_obs(o,1);
   display(o, display_undoable); 
}

void
WORLD::destroy(CGELptr &o, bool undoable)  
{ 
   undisplay(o, undoable); 
   EXIST.rem(o);
   EXISTobs::notify_exist_obs(o,0);
}

void
WORLD::destroy(CGELlist &gels, bool undoable)  
{
   // XXX - should make a single undo command here
   for (int i=0; i<gels.num(); i++)
      destroy(gels[i], undoable);
}

bool
WORLD::is_displayed(CGELptr& o)
{
   return o && DRAWN.contains(o);
}

void
WORLD::display(CGELptr &o, bool undoable)  
{
   if (!o) 
      return;

   if (undoable) {
      add_command( new DISPLAY_CMD(o) );
   } else {
      DRAWN.add(o); 
   }
}

void
WORLD::display_gels(CGELlist& gels, bool undoable)  
{
   if (gels.empty()) 
      return;

   if (undoable) {
      add_command( new DISPLAY_CMD(gels) );
   } else {
      for (int i=0; i<gels.num(); i++)
         DRAWN.add(gels[i]); 
   }
}

void
WORLD::undisplay(CGELptr &o, bool undoable)  
{ 
   if (!o) 
      return;

   if (undoable) {
      add_command( new UNDISPLAY_CMD(o) );
   } else {
      DRAWN.rem(o); 
   }
}

void
WORLD::undisplay_gels(CGELlist& gels, bool undoable)  
{ 
   if (gels.empty()) 
      return;

   if (undoable) {
      add_command( new UNDISPLAY_CMD(gels) );
   } else {
      for (int i=0; i<gels.num(); i++)
         DRAWN.rem(gels[i]); 
   }
}

int
WORLD::toggle_display(CGELptr &o, bool undoable)  
{ 
   if (DRAWN.contains(o)){ 
      undisplay(o, undoable);
      return 0; 
   } 
  
   display(o, undoable);
   return 1;
}
