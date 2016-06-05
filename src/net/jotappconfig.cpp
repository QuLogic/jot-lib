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
#include "std/config.H"
#include "std/file.H"
#include <fstream>
#include "data_item.H"

/**********************************************************************
 * BaseJOTappConfig
 **********************************************************************/

//Use these macros when adding new variables that serialize to .cfg files

#define INTEGER_VARIABLE(X,Y) TAG_meth<X>(Y, &X::put_integer_var, &X::get_integer_var, 1)
#define DOUBLE_VARIABLE(X,Y)  TAG_meth<X>(Y, &X::put_double_var,  &X::get_double_var,  1)
#define STRING_VARIABLE(X,Y)  TAG_meth<X>(Y, &X::put_string_var,  &X::get_string_var,  1)
#define BOOL_VARIABLE(X,Y)    TAG_meth<X>(Y, &X::put_bool_var,    &X::get_bool_var,    1)

//Use these macros when deprecating old variables... The variables will be parsed
//during loading, but have no effect, and will not be re-saved.

#define OLD_VARIABLE(X,Y)     TAG_meth<X>(Y, &X::put_old_var,     &X::get_old_var,     1)

class BaseJOTappConfig : public DATA_ITEM, public Config
{

   /******** STATIC MEMBER VARIABLES ********/
 private:
   static TAGlist*   _bja_tags;

 public:
   /******** STATIC MEMBER METHODS ********/

   /******** MEMBER VARIABLES ********/

 public:
   /******** CONSTRUCTOR/DECONSTRUCTOR *******/

   BaseJOTappConfig(const string &j) : Config(j) { assert(_instance == this); }

   virtual ~BaseJOTappConfig()   { }

 protected:
   /******** MEMBER METHODS ********/

   /******** Config METHODS ********/

   virtual bool         save(const string &);
   virtual bool         load(const string &);

   /******** DATA_ITEM METHODS ********/
 public:
   DEFINE_RTTI_METHODS_BASE("BaseJOTappConfig", CDATA_ITEM *);
   virtual DATA_ITEM*   dup() const  { return new BaseJOTappConfig(""); }
   virtual CTAGlist&    tags() const;

 protected:
   /******** I/O Methods ********/
   virtual void   get_string_var (TAGformat &d);
   virtual void   put_string_var (TAGformat &d) const;

   virtual void   get_integer_var (TAGformat &d);
   virtual void   put_integer_var (TAGformat &d) const;

   virtual void   get_double_var (TAGformat &d);
   virtual void   put_double_var (TAGformat &d) const;

   virtual void   get_bool_var (TAGformat &d);
   virtual void   put_bool_var (TAGformat &d) const;

   virtual void   get_old_var (TAGformat &d);
   virtual void   put_old_var (TAGformat &d) const;
};

/*****************************************************************
 * BaseJOTappConfig
 *****************************************************************/

//////////////////////////////////////////////////////
// BaseJOTappConfig Static Variables Initialization
//////////////////////////////////////////////////////

TAGlist*             BaseJOTappConfig::_bja_tags   = nullptr;

//////////////////////////////////////////////////////
// BaseJOTappConfig Methods
//////////////////////////////////////////////////////

////////////////////////////////////
// tags()
/////////////////////////////////////
CTAGlist &
BaseJOTappConfig::tags() const
{
   if (!_bja_tags) {
      _bja_tags = new TAGlist;

      //_bja_tags->push_back(new INTEGER_VARIABLE(BaseJOTappConfig,"FOO_INT"));
      //_bja_tags->push_back(new DOUBLE_VARIABLE(BaseJOTappConfig,"FOO_DBL"));
      //_bja_tags->push_back(new STRING_VARIABLE(BaseJOTappConfig,"FOO_STR"));
      //_bja_tags->push_back(new BOOL_VARIABLE(BaseJOTappConfig,"FOO_BOOL"));
      //_bja_tags->push_back(new OLD_VARIABLE(BaseJOTappConfig,"FOO_BOOL"));
   }

   return *_bja_tags;
}

/////////////////////////////////////
// save()
/////////////////////////////////////
bool
BaseJOTappConfig::save(const string &filename)
{
   assert(_instance);

   fstream fout;
   fout.open(filename.c_str(), ios::out);

   if (!fout) {
      cerr << "BaseJOTappConfig::save() - ERROR! Could not open file '" << filename << "'\n";
      return false;
   }

   STDdstream stream(&fout);
   ((BaseJOTappConfig*)_instance)->format(stream);

   fout.close();

   return true;
}


/////////////////////////////////////
// load()
/////////////////////////////////////
bool
BaseJOTappConfig::load(const string &filename)
{
   assert(_instance);
   if (filename == "") {
      cerr << "BaseJOTappConfig::load: error: filename is empty" << endl;
      return false;
   }

   fstream fin;
#if (defined (WIN32) && defined(_MSC_VER) && (_MSC_VER <=1300)) /*VS 6.0*/

   fin.open(filename.c_str(), ios::in | ios::nocreate);
#else
   fin.open(filename.c_str(), ios::in);
#endif

   if (!fin) {
      // It is routine to try opening jot.cfg before falling back to
      // jot-default.cfg, so we don't report failure here.
      return false;
   }

   STDdstream stream(&fin);

   string class_name;
   stream >> class_name;

   if (class_name != ((BaseJOTappConfig*)_instance)->class_name()) {
      cerr << "BaseJOTappConfig::load: loaded class name '"
           << class_name << "' is not correct config class '"
           << ((BaseJOTappConfig*)_instance)->class_name() << endl;
      return false;
   }

   ((BaseJOTappConfig*)_instance)->decode(stream);

   return true;
}


/////////////////////////////////////
// get_string_var()
/////////////////////////////////////
void
BaseJOTappConfig::get_string_var(TAGformat &d)
{
   //cerr << "BaseJOTappConfig::get_string_var() [" << d.name() << "]\n";

   string str_val = (*d).get_string_with_spaces();

   //cerr << "BaseJOTappConfig::get_string_var() [" << d.name() << "] - Loaded string: '"  << str_val << "'\n";

   if (str_val == "NULL_STR")
      str_val = "";

   set_var_str(d.name(), str_val);

}

/////////////////////////////////////
// put_string_var()
/////////////////////////////////////
void
BaseJOTappConfig::put_string_var(TAGformat &d) const
{
   //cerr << "BaseJOTappConfig::put_string_var() [" << d.name() << "]\n";

   if (get_var_is_set(d.name())) {
      string str_val;

      str_val = get_var_str(d.name());

      if (str_val == "")
         str_val = "NULL_STR";

      cerr << "BaseJOTappConfig::put_string_var() [" << d.name() << "] - Writing string '" << str_val << "'\n";

      d.id();
      *d << str_val.c_str() << " ";
      d.end_id();
   } else {
      cerr << "BaseJOTappConfig::put_string_var() [" << d.name() << "] - Not set...\n";
   }
}


/////////////////////////////////////
// get_old_var()
/////////////////////////////////////
void
BaseJOTappConfig::get_old_var(TAGformat &d)
{
   //cerr << "BaseJOTappConfig::get_old_var() [" << d.name() << "]\n";

   string str_val = (*d).get_string_with_spaces();

   cerr << "BaseJOTappConfig::get_old_var() [" << d.name() << "] - **Deprecated Variable** Loaded string: '"  << str_val << "'\n";

   // Don't do anything with this value...

}

/////////////////////////////////////
// put_old_var()
/////////////////////////////////////
void
BaseJOTappConfig::put_old_var(TAGformat &) const
{
   //Do nothing...
}

/////////////////////////////////////
// get_integer_var()
/////////////////////////////////////
void
BaseJOTappConfig::get_integer_var(TAGformat &d)
{
   //cerr << "BaseJOTappConfig::get_integer_var() [" << d.name() << "]\n";

   int int_val;
   *d >> int_val;

   //cerr << "BaseJOTappConfig::get_integer_var() [" << d.name() << "] - Loaded integer: '"  << int_val << "'\n";

   set_var_int(d.name(), int_val);

}

/////////////////////////////////////
// put_integer_var()
/////////////////////////////////////
void
BaseJOTappConfig::put_integer_var(TAGformat &d) const
{
   //cerr << "BaseJOTappConfig::put_integer_var() [" << d.name() << "]\n";

   if (get_var_is_set(d.name())) {
      int int_val;

      int_val = get_var_int(d.name());

      cerr << "BaseJOTappConfig::put_integer_var() [" << d.name() << "] - Writing integer '" << int_val << "'\n";

      d.id();

      *d << int_val;

      d.end_id();
   } else {
      cerr << "BaseJOTappConfig::put_integer_var() [" << d.name() << "] - Not set...\n";
   }
}

/////////////////////////////////////
// get_double_var()
/////////////////////////////////////
void
BaseJOTappConfig::get_double_var(TAGformat &d)
{
   //cerr << "BaseJOTappConfig::get_double_var() [" << d.name() << "]\n";

   double dbl_val;
   *d >> dbl_val;

   //cerr << "BaseJOTappConfig::get_double_var() [" << d.name() << "] - Loaded double: '"  << dbl_val << "'\n";

   set_var_dbl(d.name(), dbl_val);
}

/////////////////////////////////////
// put_double_var()
/////////////////////////////////////
void
BaseJOTappConfig::put_double_var(TAGformat &d) const
{
   //cerr << "BaseJOTappConfig::put_double_var() [" << d.name() << "]\n";

   if (get_var_is_set(d.name())) {
      double dbl_val;

      dbl_val = get_var_dbl(d.name());

      cerr << "BaseJOTappConfig::put_double_var() [" << d.name() << "] - Writing double '" << dbl_val << "'\n";

      d.id();

      *d << dbl_val;

      d.end_id();
   } else {
      cerr << "BaseJOTappConfig::put_double_var() [" << d.name() << "] - Not set...\n";
   }
}

/////////////////////////////////////
// get_bool_var()
/////////////////////////////////////
void
BaseJOTappConfig::get_bool_var(TAGformat &d)
{
   //cerr << "BaseJOTappConfig::get_bool_var() [" << d.name() << "]\n";

   string foo;
   *d >> foo;

   bool bool_val;
   if (foo == "true") {
      bool_val = true;
      //cerr << "BaseJOTappConfig::get_bool_var() [" << d.name() << "] - Loaded bool: '"  << foo << "'\n";
   } else if (foo == "false") {
      bool_val = false;
      //cerr << "BaseJOTappConfig::get_bool_var() [" << d.name() << "] - Loaded bool: '"  << foo << "'\n";
   } else {
      bool_val = false;
      cerr << "BaseJOTappConfig::get_bool_var() [" << d.name() << "] - ERROR!! Loaded bool: '"  << foo << "', but should be 'true' or 'false'. Assuming 'false'...\n";
   }

   set_var_bool(d.name(), bool_val);
}

/////////////////////////////////////
// put_bool_var()
/////////////////////////////////////
void
BaseJOTappConfig::put_bool_var(TAGformat &d) const
{
   //cerr << "BaseJOTappConfig::put_bool_var() [" << d.name() << "]\n";

   if (get_var_is_set(d.name())) {

      bool bool_val;
      bool_val = get_var_bool(d.name());

      string foo;
      foo = bool_val?"true":"false";

      cerr << "BaseJOTappConfig::put_bool_var() ["
           << d.name() << "] - Writing bool '" << foo << "'\n";

      d.id();
      *d << foo.c_str() << " ";
      d.end_id();
   } else {
      cerr << "BaseJOTappConfig::put_bool_var() ["
           << d.name() << "] - Not set...\n";
   }
}

/**********************************************************************
 * JOTappConfig Class
 **********************************************************************/

class JOTappConfig : public BaseJOTappConfig
{
 private:
   /******** STATIC MEMBER VARIABLES ********/
   static TAGlist*            _ja_tags;

 public:
   /******** STATIC MEMBER METHODS ********/
   //static void       init();

   /******** MEMBER VARIABLES ********/

 public:
   /******** CONSTRUCTOR/DECONSTRUCTOR *******/

   JOTappConfig(const string& j) : BaseJOTappConfig(j) { assert(_instance == this);  }

   virtual ~JOTappConfig() { }

 protected:
   /******** MEMBER METHODS ********/

   /******** DATA_ITEM METHODS ********/
 public:
   DEFINE_RTTI_METHODS2("JOTappConfig", BaseJOTappConfig, CDATA_ITEM *);
   virtual DATA_ITEM*   dup() const  { return new JOTappConfig(""); }
   virtual CTAGlist&    tags() const;

};

/*****************************************************************
 * JOTappConfig Static Data
 *****************************************************************/

TAGlist*    JOTappConfig::_ja_tags = nullptr;

/*****************************************************************
 * JOTappConfig Methods
 *****************************************************************/

/////////////////////////////////////
// tags()
/////////////////////////////////////
CTAGlist &
JOTappConfig::tags() const
{
   if (!_ja_tags) {
      _ja_tags = new TAGlist(BaseJOTappConfig::tags());

      // Tags suitable for public consumption....

      // XXX - needs work! many of these should be removed,
      //       and a few others added...

      _ja_tags->push_back(new STRING_VARIABLE(JOTappConfig,"REF_IMAGE_SIZE"));
      _ja_tags->push_back(new STRING_VARIABLE(JOTappConfig,"BGCOLOR"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"START_WITH_LINE_PEN"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"START_WITH_HATCHING_PEN"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"START_WITH_BASECOAT_PEN"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"SUPPRESS_NPR"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"ENABLE_FFS"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"SHOW_JOT_APP"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"JOT_RECENTER"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"ANTIALIAS_SILS"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"ANTIALIAS_WIREFRAME"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"DRAW_BACKFACING"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"GRAB_ALPHA"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"GOOCH_NO_LINE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"SIGMA_ONE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"FIT_SIGMA"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"FIT_PHASE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"FIT_INTERPOLATE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"COVER_MAJORITY"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"COVER_ONE_TO_ONE"));
      _ja_tags->push_back(new INTEGER_VARIABLE(JOTappConfig,"SIL_DEBUG_W"));
      _ja_tags->push_back(new INTEGER_VARIABLE(JOTappConfig,"SIL_DEBUG_H"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"FIT_PIX"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"WEIGHT_FIT"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"WEIGHT_SCALE"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"WEIGHT_DISTORT"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"WEIGHT_HEAL"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"HACK_EYE_POS"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"REALLY_HACK_EYE_POS"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"PRINT_ERRS"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"JOT_REPORT_SIL_STATS"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"JOT_NO_FACE_CULLING"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"GRID_USE_UV"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"ZX_NO_DOTS"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"ZX_NO_COLOR"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"ZX_CLOSED"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"SILS_IGNORE_MESH_XFORM"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"JOT_SAVE_XFORMED_MESH"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"JOT_USE_NEW_BFACE_IO"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"SUPPRESS_CREASES"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"ALLOW_CHECK_ALL"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"PRINT_MESH_SIZE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"NO_RANDOM_SILS"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"JOT_CREASE_THRESH"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"UV_RESOLUTION"));
      _ja_tags->push_back(new INTEGER_VARIABLE(JOTappConfig,"CHECK_ALL_UNDER"));
      _ja_tags->push_back(new INTEGER_VARIABLE(JOTappConfig,"RANDOMIZED_MIN_FACES"));
      _ja_tags->push_back(new INTEGER_VARIABLE(JOTappConfig,"RANDOMIZED_MIN_EDGES"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"PRINT_TRIS_PER_STRIP"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"NO_GL_EXT_compiled_vertex_array"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"NO_GL_NV_vertex_program"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"NO_GL_ARB_multitexture"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"NO_GL_NV_register_combiners"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"NO_GL_ARB_vertex_program"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"NO_GL_ARB_fragment_program"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"NO_GL_ATI_fragment_shader"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"SCREEN_BOX"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"ZX_NEW_BRANCH"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"COUNT_STROKES"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"ADD_CURVE_PLOT"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"HACK_ID_UPDATE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"ADD_GRID_SHADER"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"OPAQUE_COMPOSITE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"HATCHING_GROUP_SLIDE_FIT"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"NPR_SELECT_ALPHA"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"BMESH_OFFSET_FACTOR"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"BMESH_OFFSET_UNITS"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"SIL_VIS_SAMPLE_SPACING"));
      _ja_tags->push_back(new INTEGER_VARIABLE(JOTappConfig,"VIS_ID_RAD"));
      _ja_tags->push_back(new INTEGER_VARIABLE(JOTappConfig,"INVIS_ID_RAD"));
      _ja_tags->push_back(new INTEGER_VARIABLE(JOTappConfig,"MAX_LUBO_STEPS"));
      _ja_tags->push_back(new INTEGER_VARIABLE(JOTappConfig,"LUBO_SAMPLE_STEP"));
      _ja_tags->push_back(new INTEGER_VARIABLE(JOTappConfig,"SIL_VIS_PATH_WIDTH"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"DRAW_PROPAGATION"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"USE_OLD_LENGTH_ENCODING"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"LONG_LUBO"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"NO_BOX_CHECK"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"VERIFY_IDREF_RANGE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"HACK_NOISE"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"SIL_TO_STROKE_PIX_SAMPLING"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"HEAL_DRAG_PIX_THRESH"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"HEAL_JOIN_PIX_THRESH"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"SPARSE_FACTOR"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"MIN_FRAC_PER_GROUP"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"MIN_PIX_PER_GROUP"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"MIN_PATH_PIX"));
      _ja_tags->push_back(new INTEGER_VARIABLE(JOTappConfig,"MIN_VOTES_PER_GROUP"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"BASE_STROKE_REPAIR_INTERSECTIONS"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"FIX_CREASE_OFFSETS"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"CREASE_EXACT_VISIBILITY"));
      _ja_tags->push_back(new INTEGER_VARIABLE(JOTappConfig,"REFINE_LIMIT_HACK"));
      _ja_tags->push_back(new INTEGER_VARIABLE(JOTappConfig,"JOT_WINDOW_WIDTH"));
      _ja_tags->push_back(new INTEGER_VARIABLE(JOTappConfig,"JOT_WINDOW_HEIGHT"));
      _ja_tags->push_back(new INTEGER_VARIABLE(JOTappConfig,"JOT_WINDOW_X"));
      _ja_tags->push_back(new INTEGER_VARIABLE(JOTappConfig,"JOT_WINDOW_Y"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"GEST_ADD_MIN_DIST"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"TRIM_DIST"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"TRIM_ANGLE"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"ELLIPSE_ERR_FACTOR"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"ELLIPSE_LO_FACTOR"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"ELLIPSE_HI_FACTOR"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"EPC_RATIO"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"SELECT_EDGE_CHAIN_PT_THRESH"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"SHININESS"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"KEYLINE_SIL_WIDTH"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"MAX_CURVATURE"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"CURV_REMAP_SCALE"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"ANTI_DISTORT_TIP_FACTOR"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"FADE_DUR"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"HATCHING_PIX_SAMPLING"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"PIX_RES"));
      _ja_tags->push_back(new STRING_VARIABLE(JOTappConfig,"JOT_WINDOW_NAME"));
      _ja_tags->push_back(new STRING_VARIABLE(JOTappConfig,"JOT_RENDER_STYLE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"KBD_NAVIGATION"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"FADE_TEXTURES"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"HATCHING_KEEP_OLD_TRANSITIONS"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"HATCHING_GROUP_SLIDE_FIT"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"NO_CONNECT_ERRS"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"DO_TCP_DELAY"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"FLIP_CAM_MANIP"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"JOT_PRINT_LINE_WIDTHS"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"JOT_PRINT_POINT_SIZES"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"DONT_QUIT_ALL"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"QUIT_ALL"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"DISTRIB_CAMERA"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"JOT_DL_PER_VIEW"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"TABLET_SIZE_SMALL"));
      _ja_tags->push_back(new INTEGER_VARIABLE(JOTappConfig,"JOT_NUM_WINS"));
      _ja_tags->push_back(new INTEGER_VARIABLE(JOTappConfig,"TTY_TIMEOUT"));
      _ja_tags->push_back(new INTEGER_VARIABLE(JOTappConfig,"MAX_TRY"));
      _ja_tags->push_back(new INTEGER_VARIABLE(JOTappConfig,"RADJUST"));
      _ja_tags->push_back(new INTEGER_VARIABLE(JOTappConfig,"LADJUST"));
      _ja_tags->push_back(new INTEGER_VARIABLE(JOTappConfig,"GLUT_WAIT"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"TABLET_OFFSET_X"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"TABLET_OFFSET_Y"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"IOD"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"DISPLAY_ASPECT"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"PIXEL_WARP_FACTOR"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"JOT_GAMMA"));
      _ja_tags->push_back(new STRING_VARIABLE(JOTappConfig,"DISPLAY"));
      _ja_tags->push_back(new STRING_VARIABLE(JOTappConfig,"DOT_TEXTURE"));
      _ja_tags->push_back(new INTEGER_VARIABLE(JOTappConfig,"JOT_GLUT_REDRAW_TIMEOUT_MS"));
      _ja_tags->push_back(new INTEGER_VARIABLE(JOTappConfig,"JOT_GLUT_REDRAW_SLEEP_MS"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"GEST_STARTUP_DIST_THRESH"));


      //Tags that shouldn't get seen by g.q.public... Right now, they're
      //treated the same, but that will change before the next release.
      //Please add any tags you want 'secret' below... I'll implement
      //'secret' version of the tag macros soon...

      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"DEBUG_EASELS"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"PRINT_SIMPLEX_BITS"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"DEBUG_TABLET"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"DEBUG_CAM_HISTORY"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"DEBUG_CONNECT_EDIT_INIT"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"DEBUG_CONNECT_EDIT_ALL"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"JOT_MINIMAL_RENDER_STYLES"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"DEBUG_DTAP_GUARD"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"DEBUG_EDGE_SELECT"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"GEST_DEBUG_TRIM"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"DEBUG_GEST_STROKE_DRAW"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"DEBUG_GEST_DRAWER"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"DEBUG_INFLATE_ALL"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"DOUG"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"DEBUG_GESTURES"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"DEBUG_SELECT_WIDGET_INIT"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"DEBUG_SELECT_WIDGET_ALL"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"ALWAYS_UPDATE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"DEBUG_SWEEP_ALL"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"SUG_VALLEY"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"DEBUG_MOUSE_BUTTON_CALLBACK"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"DEBUG_CONTROL_FRAME"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"ENABLE_SUGCON"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"SUG_THRESHGRAD"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"SUG_THRESHGRAD2"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"SUG_THRESHANGLE"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"SUG_THRESHANGLE2"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"DEBUG_LPATCH"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"SKIN_INHIBIT_UV"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"DEBUG_ADAPTIVE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"DEBUG_BMESH_DESTRUCTOR"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"DEBUG_BUILD_ZX_STRIPS"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"DEBUG_CLEAN_PATCHES"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"FORCE_FACE_PICK"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"DEBUG_SUBDIVISION"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"DEBUG_GL_EXTENSIONS"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"DEBUG_HATCH_FIXED"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"ADD_DITHER"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"HATCHING_DEBUG_VIS"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"PUT_VIEW_STUFF_IN_SM"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"DEBUG_STROKES"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"DEBUG_ZX_STROKES"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"DEBUG_LUBO"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"NEW_IDREF_METHOD"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"HACK_FIX_LOOP_POS"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"HACK_USES_MINTEST"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"SUGKEYLINE_SIL_WIDTH"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"DEBUG_BASE_STROKES"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"DEBUG_BCURVE_RES_LEVEL"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"DEBUG_EXTRUDE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"DEBUG_EXTENDER"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"DEBUG_PANEL"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"DEBUG_RSURF"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"DEBUG_TESSELLATE"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"CONTROL_FRAME_RATIO"));
      _ja_tags->push_back(new DOUBLE_VARIABLE(JOTappConfig,"CTRL_FRAME_TOP_THICKNESS"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"USE_STEREO"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"ASYNCH"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"BIRD_CAM"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"ORTHO_CAM"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"JOT_DEBUG_THREADS"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"JOT_MULTITHREAD"));

      _ja_tags->push_back(new INTEGER_VARIABLE(JOTappConfig,"JOT_WARNING_LEVEL"));

      _ja_tags->push_back(new STRING_VARIABLE(JOTappConfig,"DEBUG_GLSL_SHADER"));
      _ja_tags->push_back(new STRING_VARIABLE(JOTappConfig,"DEBUG_TEXTURE"));
      
      //karol's dev tags
      _ja_tags->push_back(new STRING_VARIABLE(JOTappConfig,"ENABLE_MARBLE"));
      _ja_tags->push_back(new STRING_VARIABLE(JOTappConfig,"ENABLE_HALFTONE_EX"));
      _ja_tags->push_back(new STRING_VARIABLE(JOTappConfig,"ENABLE_DOTS_EX"));
      _ja_tags->push_back(new STRING_VARIABLE(JOTappConfig,"ENABLE_DOTS_TX"));

      _ja_tags->push_back(new STRING_VARIABLE(JOTappConfig,"SHOW_SKYBOX"));
      _ja_tags->push_back(new STRING_VARIABLE(JOTappConfig,"SHOW_HALOS"));

      _ja_tags->push_back(new STRING_VARIABLE(JOTappConfig,"DO_VIEW_HALOS"));
      _ja_tags->push_back(new STRING_VARIABLE(JOTappConfig,"DO_REF_HALOS"));
      _ja_tags->push_back(new STRING_VARIABLE(JOTappConfig,"USE_SMALLER_HALOS"));
      _ja_tags->push_back(new STRING_VARIABLE(JOTappConfig,"ENABLE_SKYBOX_TEX"));
      _ja_tags->push_back(new STRING_VARIABLE(JOTappConfig,"SUPRESS_NPR"));
      _ja_tags->push_back(new STRING_VARIABLE(JOTappConfig,"ENABLE_PROXY"));
      _ja_tags->push_back(new STRING_VARIABLE(JOTappConfig,"DEBUG_DOTS"));

      //Mikes timed transition stuff
      _ja_tags->push_back(new STRING_VARIABLE(JOTappConfig,"USE_TIMED_TRANSITIONS"));


      //Deprecated tags go here using the 'OLD_VARIABLE' tag so they're be
      //quietly read, but not written back out...

      _ja_tags->push_back(new OLD_VARIABLE(JOTappConfig,"DEBUG_DOUBLE_TAP"));
      _ja_tags->push_back(new OLD_VARIABLE(JOTappConfig,"DEBUG_WIN_PICK"));
      _ja_tags->push_back(new OLD_VARIABLE(JOTappConfig,"DEBUG_UV_INTERSECT"));
      _ja_tags->push_back(new OLD_VARIABLE(JOTappConfig,"DEBUG_SLASH_CB"));


      // john: for testing serialization
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"WRITE_SUBDIV_MESHES"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"WRITE_BNODES_INFO"));

      //other stuff 2007

      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig,"ENABLE_PATCH_D2D"));

      // debug flags

      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_ACTION"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_APPLY_SKEL"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_BASECOAT_SHADER"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_BBASE_RES_LEVEL"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_BBASE_SUBDIV_GEN"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_BBASE_UPDATE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_BBASE_XFORM"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_BCURVE_BOUNDARY"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_BCURVE_CONSTRUCTOR"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_BCURVE_DELETE_ELS"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_BCURVE_EMBEDDED"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_BCURVE_MAP_CONSTRUCTOR"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_BCURVE_ON_SKIN"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_BCURVE_ON_VERTS"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_BCURVE_RESAMPLE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_BFACE_LIST_IS_CONNECTED"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_BFACE_LIST_SUBDIVISION"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_BINARY_IMAGE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_BLUR_SHADER"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_BNODE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_BNODE_DESTRUCTOR"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_BNODE_INVALIDATE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_BNODE_TICK"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_BNODE_TOPO_SORT"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_BNODE_UPDATE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_BNODE_XFORM"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_BPOINT_FRAME"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_BPOINT_MEME"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_BPOINT_MOVE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_BUILD_CURVE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_BUILD_REVOLVE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_BUILD_TUBE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_CAM_FOCUS"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_CAM_FSA"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_CELL"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_CHECK_PLANE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_CIRCLE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_CIRCLE_WIDGET_ALL"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_CIRCLE_WIDGET_GO"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_CIRCLE_WIDGET_INIT"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_CLEAN_ON_EXIT"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_CONNECTED_LIST"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_CREASE_WIDGET"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_CREATE_CURVE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_CREATE_RECT"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_CREATE_RIBBONS"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_CRV_BOUNDARIES"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_CRV_SKETCH_INIT"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_CRV_SKETCH_OVERSKETCH"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_CRV_SKETCH_STROKE_CB"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_CRV_SKETCH_UPDATE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_CS_DRAW_SPANS"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_CURSOR3D"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_CURVE_BINORMS"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_CURVE_NORMS"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_CURVE_PTS"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_D2D_SAMPLES"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_DATA_ITEM"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_DEBUG_UV"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_DISK_MAP"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_DISTRIB"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_DOTS"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_DRAW_MANIP"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_DT"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_EDGE_SWAP"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_EDIT_LEVEL_CHANGED"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_ELLIPSE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_EXTENDER_CIRCLE_CB"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_EXTENDER_STROKE_CB"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_EXTENDER_SWEEP_BALL"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_FIX_ORIENTATION"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_FLOOR_REALIGN"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_FPS"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_GEST_INT"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_GET_DRAW_PLANE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_GET_PRIMITIVE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_GET_SUBDIV_CHAIN"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_GLSL_HALO"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_GLSL_HATCHING"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_GLSL_MARBLE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_GLSL_NORMAL"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_GLSL_PAPER"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_GLSL_SHADER"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_GLSL_TOON"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_GLSL_XTOON"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_GLUT_WACOM"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_GLUT_WINSSYS"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_GRID"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_HALFTONE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_HALO_SPHERE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_HALO_UPDATE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_HATCHING"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_HISTOGRAM"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_ICON2D"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_IMAGE_LINE_SHADER"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_INC_SEC_DISK"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_INC_SEC_QUAD"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_INC_SEC_TRI"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_INSET_QUAD"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_IO"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_JOT_CMD"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_JOT_GLUT_IDLE_CB"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_LAYER_BASE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_LINE_CB"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_LMESH_FIT"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_LMESH_SUBDIV_INPUTS"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_MAP2D3D"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_MATCH_SPANS"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_MEME_DEMOTION"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_MEME_DESTRUCTOR"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_MEMES"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_MEME_SUBDIV_GEN"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_MESH_MERGE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_MESH_OPS"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_MIN_DISTORT"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_MOVE_CMD"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_MSLD"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_MULTI_LIGHTS_TONE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_NAME_LOOKUP"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_NET_STREAM"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_NON_MANIFOLD"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_OBJ2SM"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_OBJ_READER"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_OVERSKETCH"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_P2H"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_PAINTERLY"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_PANEL_CELLS"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_PANEL_OP"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_PAPER_DOLL"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_PATCH_BLEND_WEIGHTS"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_PATCH_FOCUS"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_PATCH_PEN"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_PATCH_READ_STREAM"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_PICK_FACE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_PLY2SM"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_PNG2SM"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_PRIMITIVE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_PRIMITIVE_CREATE_BALL"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_PRIMITIVE_FIND_SKEL_CURVE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_PRIMITIVE_INIT"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_PROFILE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_PROXY_SURFACE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_PROXY_TEXTURE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_PUNCH"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_PUSH_FACES"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_QUAD_PANEL"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_REF_IMAGES"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_RE_TESS"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_RIDGE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_ROOF"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_ROOF_STROKE_CB"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_SCHEDULER"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_SELECT_SKEL_CURVE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_SIMPLE_TUBE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_SKIN"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_SKIN_CURVE_MAP_CONSTRUCTOR"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_SKIN_CURVE_SETUV"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_SKIN_MEMES"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_SKIN_UPDATE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_SKYBOX"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_SLASH"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_SLASH_TAP"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_SMALL_CIRCLE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_SPECIFY_UPPER"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_SPLIT_TRI"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_SPS"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_SSUBRAMA"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_STROKE_CB"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_SUBDIV_UPDATER"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_SURF_CURVE_MAP_CONSTRUCTOR"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_SURF_CURVE_MAP_RECOMPUTE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_SURF_CURVE_SETUV"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_SWEEP_BASE_EXTEND"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_SWEEP_HIT_BOUNDARY"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_SWEEP_LINE_CB"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_SWEEP_SETUP"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_SWEEP_TRIM"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_TAP"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_TEX_USE_MIPMAP"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_TEXBODY_XFORM"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_TEXTURE"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_TONE_SHADER"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_TRI_PANEL"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_UV_DATA"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_UV_INTERSECT"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_UV_SURF_SMOOTH_UVS"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_VERT_MAPPER"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_VIS_REF"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_VOLUME_PRESERVATION"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_WEAK_EDGES"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_WIREFRAME"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_XFORM_CMD"));
      _ja_tags->push_back(new BOOL_VARIABLE(JOTappConfig, "DEBUG_XFORM_PEN"));
   }

   return *_ja_tags;
}

/**********************************************************************
 * main_config()
 **********************************************************************/
void
main_config(bool init)
{
   static JOTappConfig* config = nullptr;

   if (init) {
      //Variables that shouldn't raise warnings if they
      //are accessed prior to a load, or are overridden, etc.

      //This is queried in err_mesg, that raises warning because
      //some err_mesg's appear before the config file is loaded.
      //The wraning employ err_mesg, and so an infinite function
      //recursion happens!

      Config::no_warn("JOT_WARNING_LEVEL");  

      //Set JOT_ROOT env. var...
      if (!getenv("JOT_ROOT")) {
         err_msg("main_config: JOT_ROOT environment variable NOT found...");
         err_msg("main_config: Will use current working directory instead...");
         string cwd = getcwd_();
         if (cwd != "") {
            setenv("JOT_ROOT", cwd.c_str(), 1);
         } else {
            err_msg("main_config: failed retrieving current working directory!");
            exit(0);
         }
      }

      // Setup config class, and JOT_ROOT global variable...


      config = new JOTappConfig(string(getenv("JOT_ROOT")) + "/");
      assert(config);

      if (!(Config::load_config(Config::JOT_ROOT() + "jot.cfg") ||
            Config::load_config(Config::JOT_ROOT() + "jot-default.cfg"))) {
         err_msg("main_config: *FAILED* config load from %s or %s",
                 (Config::JOT_ROOT() + "jot.cfg").c_str(),
                 (Config::JOT_ROOT() + "jot-default.cfg").c_str());
      }
   } else {
      err_mesg(ERR_LEV_SPAM, "main_config: Cleanup...");
      assert(config);
      delete config;
      config = nullptr;
   }
}

// end of file jotappconfig.C
