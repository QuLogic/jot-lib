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
// io_manager.C

#include "io_manager.H"
#include "std/file.H"

/*****************************************************************
 * IOManager
 *****************************************************************/

/////////////////////////////////////
// Static Variable Initialization
/////////////////////////////////////
TAGlist *      IOManager::_io_tags = NULL;
IOManager *    IOManager::_instance = NULL;

/////////////////////////////////////
// tags()
/////////////////////////////////////
CTAGlist &
IOManager::tags() const
{
   if (!_io_tags) {
      _io_tags = new TAGlist;
 
      *_io_tags += new TAG_meth<IOManager>(
         "basename",
         &IOManager::put_basename,
         &IOManager::get_basename,
         1);
   }
   return *_io_tags;
}

/////////////////////////////////////
// get_basename()
/////////////////////////////////////
void
IOManager::get_basename (TAGformat &d) 
{
   assert(state_() == STATE_SCENE_LOAD);

   str_ptr str;
   *d >> str; 

   if (str == "NULL_STR") 
   {
      _basename = NULL_STR;
      err_mesg(ERR_LEV_SPAM, "IOManager::get_basename() - Loaded NULL string.");
   }
   else
   {
      _basename = str;
      err_mesg(ERR_LEV_SPAM, "IOManager::get_basename() - Loaded string: '%s'", **str);
   }
   
}

/////////////////////////////////////
// put_basename()
/////////////////////////////////////
void
IOManager::put_basename (TAGformat &d) const
{
   assert(state_() == STATE_SCENE_SAVE);

   d.id();
   if (_basename == NULL_STR)
   {
      err_mesg(ERR_LEV_SPAM, "IOManager::put_basename() - Wrote NULL string.");
      *d << str_ptr("NULL_STR");
   }
   else
   {
      *d << _basename;
      err_mesg(ERR_LEV_SPAM, "IOManager::put_basename() - Wrote string: '%s'", **_basename);
   }
   d.end_id();
}

/////////////////////////////////////
// Constructor
/////////////////////////////////////

IOManager::IOManager() : 
   _basename(NULL_STR),
   _cached_cwd_plus_basename(NULL_STR),
   _old_cwd(NULL_STR),
   _old_basename(NULL_STR)
{  
   assert(!_instance); 

   preload_obs();
   postload_obs();
   presave_obs();
   postsave_obs();

   _state.add(STATE_IDLE);

   err_mesg(ERR_LEV_SPAM, "IOManager::IOManager() - Instantiated."); 
}

/////////////////////////////////////
// Destructor
/////////////////////////////////////
IOManager::~IOManager() 
{ 
   //XXX - Never bother to clean up _instance...
   
   assert(0);

   //unobs_preload();
   //unobs_postload();
   //unobs_presave();
   //unobs_postsave();
}

/////////////////////////////////////
// notify_preload
/////////////////////////////////////
void      
IOManager::notify_preload(
   NetStream     &s,
   load_status_t &load_status,
   bool           from_file,
   bool           full_scene
   )
{

   bool ret;
   str_ptr path,file,ext;
   str_ptr cpath,cfile,cext;

   //Actually, this never happens, nevertheless...
   if (!from_file) 
   {
      err_mesg(ERR_LEV_INFO, "IOManager::notify_preload() - *WARNING* Loading from non-file source. Aborting..."); 
      return;
   }


   switch(state_())
   {
      //If we're not already doing some kind of IO, 
      //fire up the desired state...
      case STATE_IDLE:
         _state.add((full_scene)?(STATE_SCENE_LOAD):(STATE_PARTIAL_LOAD));
         err_mesg(ERR_LEV_INFO, "IOManager::notify_preload() - Loading '%s'... [Source: '%s']",
            ((full_scene)?("entire scene"):("scene update")), **s.name());
      break;
      //If we're already saving a entire scene, we might encounter
      //and enmbedded partial load when we are loading and
      //saving the scene update files...
      case STATE_SCENE_SAVE:
         assert(!full_scene);
         _state.add(STATE_PARTIAL_LOAD);
         err_mesg(ERR_LEV_INFO,
            "IOManager::notify_preload() - Loading 'scene update' for subsequent save... [Source: '%s']",
               **s.name());
      break;
     //No other states should reach this callback...
      case STATE_PARTIAL_SAVE:
      case STATE_SCENE_LOAD:
      case STATE_PARTIAL_LOAD:
      default:
         assert(0);
      break;
   }

   if (load_status != LOADobs::LOAD_ERROR_NONE)
   {
      err_mesg(ERR_LEV_INFO, 
         "IOManager::notify_preload() - *WARNING* Load procedure already in error state. Aborting...");
      return;
   }

   if (state_() == STATE_SCENE_LOAD)
   {
      //About to load an entire new scene...The given stream
      //will be interpreted as is.  However, external data files,
      //like .npr or .sm files must have the right cwd set.
      //We'll extract that information from the path part of the
      //stream's name, since that should have been set to the 
      //fullpath used to open the file-based stream!

      //Clear the scene ...
      //XXX - This is hacky, and needs a clearner method...
      // XXX - following is commented out. Referencing draw lib from net lib
      //       breaks the link for stand-along apps in the mesh and convert
      //       directories. Draw_int::do_clear() is now called instead from
      //       Draw_int::do_load():
//       if (Draw_int::get_instance())
//          Draw_int::get_instance()->do_clear();

      //Blank the basename, since old-style files won't have a new name to load...
      //This will be prefixed to all attempts at loading external data files...
      _basename = NULL_STR;

      //Blank out the last successful saved or loaded "cwd+basename prefix"
      _cached_cwd_plus_basename = NULL_STR;
      
      //Save the old cwd in case of failure
      if ((_old_cwd = getcwd_()) == NULL_STR)
      {
         err_msg(
            "IOManager::notify_preload() - *WARNING* Couldn't save current working directory. Aborting...");
         load_status = LOADobs::LOAD_ERROR_CWD;
      }
      //Now, set new cwd...
      //Stream name should be the filename for file streams.
      //Extract path, file and extension... Abort on failure...
      else if (!split_filename(s.name(),path,file,ext))
      {
         err_msg(
            "IOManager::notify_preload() - *WARNING* Couldn't derive a path from stream target: '%s'. Aborting...", 
               **s.name());
         load_status = LOADobs::LOAD_ERROR_CWD;
      }
      else
      {
         //Actually change to the new cwd (Guaranteed to succeed by split_filename)
         ret = chdir_(path); assert(ret);
         err_mesg(ERR_LEV_SPAM, 
            "IOManager::notify_preload() - Set new current working directory: '%s'.", **path);
      }
   }
   else //STATE_PARTIAL_LOAD
   {
      //About to load a partial scene (or update, or something...)

      //These loads should occur in the cached cwd defined by
      //the last full scene loaded (to which this load is
      //an update).  Let's compare...
      
      if (!split_filename(_cached_cwd_plus_basename,cpath,cfile,cext))
      {
         err_msg(
            "IOManager::notify_preload() - *WARNING* Couldn't derive a path from cached path prefix: '%s'. Aborting...",
               **_cached_cwd_plus_basename);
         load_status = LOADobs::LOAD_ERROR_CWD;
      }
      else if (!split_filename(s.name(),path,file,ext))
      {
         err_msg(
            "IOManager::notify_preload() - *WARNING* Couldn't derive a path from stream target: '%s'. Aborting...",
               **s.name());
         load_status = LOADobs::LOAD_ERROR_CWD;
      }
      else if (path != cpath)
      {
         err_msg(
            "IOManager::notify_preload() - *WARNING* Scene update load path doesn't match cached working directory. Aborting...");
         load_status = LOADobs::LOAD_ERROR_CWD;
      }
      else
      {
         err_mesg(ERR_LEV_SPAM, 
            "IOManager::notify_preload() - Maintaining cached working directory: '%s'.", **path);
      }
   }
}


/////////////////////////////////////
// notify_postload
/////////////////////////////////////
void      
IOManager::notify_postload(
   NetStream     &s,
   load_status_t &load_status,
   bool           from_file,
   bool           full_scene
   )
{
   str_ptr path;

   //Actually, this never happens, nevertheless...
   if (!from_file) 
   {
      err_mesg(ERR_LEV_INFO, "IOManager::notify_postload() - *WARNING* Loading from non-file source. Aborting..."); 
      assert(state_() == STATE_IDLE);
      return;
   }

   assert(state_() == ((full_scene)?(STATE_SCENE_LOAD):(STATE_PARTIAL_LOAD)) );

   //Successful load!
   if (load_status == LOADobs::LOAD_ERROR_NONE)
   {
      if (state_() == STATE_SCENE_LOAD)
      {
         //New scene loaded...

         //Keep the new cwd

         //Keep the new basename (NULL_STR, or loaded from file)

         //Set the _cached_cwd_plus_basename so updates can
         //use this to prefix external file references...

         if ((path = getcwd_()) == NULL_STR)
         {
            err_msg(
               "IOManager::notify_postload() - *WARNING* Couldn't cache current working directory. Aborting...");
            load_status = LOADobs::LOAD_ERROR_CWD;
         }
         else
         {
            _cached_cwd_plus_basename = path + "/" + _basename + ((_basename!=NULL_STR)?("--"):(NULL_STR));
            
            //err_msg(
            //   "IOManager::notify_postload() - ...completed 'entire scene' load. [Source: '%s']", **s.name());
            err_mesg(ERR_LEV_INFO, 
               "IOManager::notify_postload() - ...completed 'entire scene' load.");
            err_mesg(ERR_LEV_INFO, 
               "IOManager::notify_postload() - Caching this file load/save prefix: '%s'",
                  **_cached_cwd_plus_basename);
         }
      }
      else //STATE_PARTIAL_LOAD
      {
         //Scene update loaded!
         
         //Keep old _basename
         
         //Keep old _cached_cwd_plus_basename 
         //err_mesg(ERR_LEV_INFO, 
         //   "IOManager::notify_postload() - ...completed 'scene update' load. [Source: '%s']", **s.name());
         err_mesg(ERR_LEV_INFO, 
            "IOManager::notify_postload() - ...completed 'scene update' load.");

      }
   }
   //Failed load!
   else
   {
      if (state_() == STATE_SCENE_LOAD)
      {
         //New scene failed to loaded...

         _basename = NULL_STR;
         _cached_cwd_plus_basename = NULL_STR;

         err_msg(
            "IOManager::notify_postload() - ...failed 'entire scene' load. [Source: '%s']", **s.name());

         if (_old_cwd != NULL_STR)
         {
            if (chdir_(_old_cwd))
            {
               err_mesg(ERR_LEV_INFO, 
                  "IOManager::notify_postload() - Restored old current working directory: '%s'", 
                     **_old_cwd);
            }
            else
            {
               err_msg(
                  "IOManager::notify_postload() - *Warning* Failed restoring old current working directory: '%s'",
                     **_old_cwd);
               //XXX - Set this?!
               load_status = LOADobs::LOAD_ERROR_CWD;
            }
         }
         else
         {
            err_msg(
               "IOManager::notify_postload() - *Warning* No saved current working directory to restore.");
         }
      }
      else //STATE_PARTIAL_LOAD
      {
         //Scene update failed!
         
         //Keep old _basename (shouldn't have changed)

         //Keep old _cached_cwd_plus_basename 
         err_msg(
            "IOManager::notify_postload() - ...failed 'scene update' load. [Source: '%s']", **s.name());
      }
   }
   _state.pop();
   assert(_state.num() > 0);
}

/////////////////////////////////////
// notify_presave
/////////////////////////////////////
void      
IOManager::notify_presave(
   NetStream     &s,
   save_status_t &save_status,
   bool           to_file,
   bool           full_scene
   )
{
   bool ret;
   str_ptr path,file,ext;

   //Actually, this never happens, nevertheless...
   if (!to_file) 
   {
      err_mesg(ERR_LEV_INFO, "IOManager::notify_presave() - *WARNING* Saving to non-file source. Aborting..."); 
      return;
   }

   switch(state_())
   {
      //If we're not already doing some kind of IO, 
      //we only expect a full scene save...
      case STATE_IDLE:
         assert(full_scene);
         _state.add(STATE_SCENE_SAVE);
         err_mesg(ERR_LEV_INFO, 
            "IOManager::notify_presave() - Saving 'entire scene'... [Dest: '%s']",
               **s.name());
      break;
      //If we're already saving a full scene, we might encounter
      //and enmbedded partial save when we are loading and
      //resaving the scene update files...
      case STATE_SCENE_SAVE:
         assert(!full_scene);
         _state.add(STATE_PARTIAL_SAVE);
         err_mesg(ERR_LEV_INFO, 
            "IOManager::notify_presave() - Saving 'scene update'... [Dest: '%s']",
               **s.name());
      break;
      //No other states should reach this callback...
      case STATE_PARTIAL_SAVE:
      case STATE_SCENE_LOAD:
      case STATE_PARTIAL_LOAD:
      default:
         assert(0);
      break;
   }


   if (save_status != SAVEobs::SAVE_ERROR_NONE)
   {
      err_mesg(ERR_LEV_INFO, 
         "IOManager::notify_presave() - *WARNING* Save procedure already in error state. Aborting...");
      return;
   }

   if (state_() == STATE_SCENE_SAVE)
   {
      //About to save new scene... Given stream is used to serialize
      //the scene.  But external files like .npr and .sm need the
      //right cwd to be set so they get saved to the right place.  
      //The cwd and basename prefix attached to all external files will 
      //be derived from the path and filename (sans extension)
      //that was used to open the stream...


      //Keep the last successful saved or loaded "cwd+basename prefix"
      //because we'll want to load old scene update files from this location
      //and then re-save to the new cwd+basename...
      
      //_cached_cwd_plus_basename unchanged

      //Save old cwd and basename in case of save failure...
      _old_basename = _basename;
      if ((_old_cwd = getcwd_()) == NULL_STR)
      {
         err_msg(
            "IOManager::notify_presave() - *WARNING* Couldn't save current working directory. Aborting...");
         save_status = SAVEobs::SAVE_ERROR_CWD;
      }
      else
      {

         //Now, set new cwd and basename...

         //Stream name should be the filename for file streams.
         //Extract path, file and extension... Abort on failure...
         if (!split_filename(s.name(),path,file,ext))
         {
            err_msg(
               "IOManager::notify_presave() - *WARNING* Couldn't derive valid path from target: '%s'. Aborting...", 
                  **s.name());
            save_status = SAVEobs::SAVE_ERROR_CWD;
         }
         else
         {
            //Actually change to the new cwd (Guaranteed to succeed by split_filename)
            _basename = file;
            err_mesg(ERR_LEV_INFO, 
               "IOManager::notify_presave() - Set new current basename file prefix: '%s'.", **file);
            
            ret = chdir_(path); assert(ret);

            err_mesg(ERR_LEV_INFO, 
              "IOManager::notify_presave() - Set new current working directory: '%s'.", **path);
         }
      }
   }
   else //STATE_PARTIAL_SAVE
   {
      //About to save a scene update...

      //These scene update saves should occur in same cwd as defined by
      //the currently occuring full save There should, therefore, be not path
      //component to the filename... Check this!

      if (!split_filename(s.name(),path,file,ext))
      {
         err_msg(
            "IOManager::notify_presave() - *WARNING* Couldn't derive valid path from target: '%s'. Aborting...", 
               **s.name());
         save_status = SAVEobs::SAVE_ERROR_CWD;
      }
      else if (path != getcwd_())
      {
         err_msg(
            "IOManager::notify_presave() - *WARNING* Path for 'scene update' doesn't match current working directory. Aborting...");
         save_status = SAVEobs::SAVE_ERROR_CWD;
      }
      else
      {
         err_mesg(ERR_LEV_INFO, 
           "IOManager::notify_presave() - Maintaing working directory: '%s'.", **path);
      }

   }
}

/////////////////////////////////////
// notify_postsave
/////////////////////////////////////
void      
IOManager::notify_postsave(
   NetStream     &s,
   save_status_t &save_status,
   bool           to_file,
   bool           full_scene
   )
{
 
   str_ptr path;

   //Actually, this never happens, nevertheless...
   if (!to_file) 
   {
      err_mesg(ERR_LEV_INFO, "IOManager::notify_postsave() - *WARNING* Saving to non-file source. Aborting..."); 
      assert(state_() == STATE_IDLE);
      return;
   }

   assert(state_() == ((full_scene)?(STATE_SCENE_SAVE):(STATE_PARTIAL_SAVE)));

   //Successful save!
   if (save_status == SAVEobs::SAVE_ERROR_NONE)
   {
      if (state_() == STATE_SCENE_SAVE)
      {
         //New scene saveed...

         //Keep the new cwd

         //Keep the new basename (derived from stream's filename)

         //Set the _cached_cwd_plus_basename so scene updates can load
         //using this to prefix in all external file references...

         if ((path = getcwd_()) == NULL_STR)
         {
            err_msg(
               "IOManager::notify_postsave() - *WARNING* Couldn't cache current working directory. Aborting...");
            save_status = SAVEobs::SAVE_ERROR_CWD;
         }
         else
         {
            _cached_cwd_plus_basename = path + "/" + _basename + ((_basename!=NULL_STR)?("--"):(NULL_STR));
            //err_msg(
            //   "IOManager::notify_postsave() - ...completed 'entire scene' save. [Dest: '%s']", **s.name());
            err_mesg(ERR_LEV_INFO, 
               "IOManager::notify_postsave() - ...completed 'entire scene' save.");
            err_mesg(ERR_LEV_INFO, 
               "IOManager::notify_postsave() - Caching this file load/save prefix: '%s'",
                  **_cached_cwd_plus_basename);
         }
      }
      else //STATE_PARTIAL_SAVE
      {
         //Scene update saveed!
         
         //Keep old _basename
         
         //Keep old _cached_cwd_plus_basename 
         //err_mesg(ERR_LEV_INFO, 
         //   "IOManager::notify_postsave() - ...completed 'scene update' save. [Dest: '%s']", **s.name());
         err_mesg(ERR_LEV_INFO, 
            "IOManager::notify_postsave() - ...completed 'scene update' save.");
      }
   }

   //Failed save! (Including due to failure directly above...)
   if (save_status != SAVEobs::SAVE_ERROR_NONE)
   {
      if (state_() == STATE_SCENE_SAVE)
      {
         //Scene failed to save...

         //Keep the last successful saved or loaded "cwd+basename prefix"
         //because we'll still want to copy old scene update files from this location
         //until a successful save provides a new location...
         
         //_cached_cwd_plus_basename;

         //Restore old basename prefix
         _basename = _old_basename;

         err_msg(
            "IOManager::notify_postsave() - ...failed 'entire scene' save. [Dest: '%s']", **s.name());

         //Restore old cwd
         if (_old_cwd != NULL_STR)
         {
            if (chdir_(_old_cwd))
            {
               err_mesg(ERR_LEV_INFO, 
                  "IOManager::notify_postsave() - Restoring old current working directory: '%s'",
                     **_old_cwd);
            }
            else
            {
               err_msg(
                  "IOManager::notify_postsave() - *Warning* Failed restoring old current working directory: '%s'",
                     **_old_cwd);
                //XXX - Set this?!
               save_status = SAVEobs::SAVE_ERROR_CWD;
            }
         }
         else
         {
            err_msg(
               "IOManager::notify_postsave() - *Warning* No saved current working directory to restore.");
         }
      }
      else //STATE_PARTIAL_SAVE
      {
         //Failed cene update save!
         
         //Keep old _basename
         
         //Keep old _cached_cwd_plus_basename 

         err_msg(
            "IOManager::notify_postsave() - ...failed 'scene update' save. [Dest: '%s']", **s.name());
      }
   }
   _state.pop();
   assert(_state.num() > 0);
}


/////////////////////////////////////
// load_prefix_()
/////////////////////////////////////
str_ptr
IOManager::load_prefix_()
{
   str_ptr ret;

   switch(state_())
   {
      //If we're not currently doing anything, just use the cwd...
      case STATE_IDLE:
         ret = cwd_();
      break;
      //Full scenes load from the cwd+basename
      case STATE_SCENE_LOAD:
         ret = current_prefix_();
      break;
      //Scenes updates load from the cached_cwd+basename
      case STATE_PARTIAL_LOAD:
         ret = cached_prefix_();
      break;
      //We shouldn't be asking in this state...      
      case STATE_SCENE_SAVE:
      case STATE_PARTIAL_SAVE:
      default:
         assert(0);
      break;
   }
   return ret;
}

/////////////////////////////////////
// save_prefix_()
/////////////////////////////////////
str_ptr
IOManager::save_prefix_()
{
   str_ptr ret;

   switch(state_())
   {
      //If we're not currently doing anything, just use the cwd...
      case STATE_IDLE:
         ret = cwd_();
      break;
      //All saves prefix with the cwd+basename
      case STATE_SCENE_SAVE:
      case STATE_PARTIAL_SAVE:
         ret = current_prefix_();
      break;
      //We shouldn't be asking if these are the state...
      case STATE_SCENE_LOAD:
      case STATE_PARTIAL_LOAD:
      default:
         assert(0);
      break;
   }
   return ret;
}

/////////////////////////////////////
// cwd_()
/////////////////////////////////////
str_ptr
IOManager::cwd_()
{
   str_ptr ret;

   ret = getcwd_();
   if (ret != NULL_STR) ret = ret + "/";

   //XXX - If root directory, will end up with double slashes.
   //      Usually harmless, but should fix...

   return ret;
}

/////////////////////////////////////
// current_prefix_() 
/////////////////////////////////////
str_ptr
IOManager::current_prefix_()
{
   str_ptr ret;

   ret = cwd_();
   if (_basename != NULL_STR) ret = ret + _basename + "--";

   return ret;
}

/////////////////////////////////////
// cached_prefix_()
/////////////////////////////////////
str_ptr
IOManager::cached_prefix_()
{
   return _cached_cwd_plus_basename;
}

/////////////////////////////////////
// split_filename()
/////////////////////////////////////
bool      
IOManager::split_filename(
   Cstr_ptr &fullpath, 
   str_ptr  &path, 
   str_ptr  &file,
   str_ptr  &ext)
{
   bool result;

   //Divide an incoming filename, with optional
   //absolute or relative path component,
   //into an absolute path, filename and file extension strings.
   
   //Relative paths are resolved against the cwd

   //Absoute path "path + '/' + file + extension" will
   //be equivalent to resolving full_filenamne against the cwd

   //The path component must exist... 
   //Returns false on failure and path, file, ext undefined

   //Save this before mucking about...
   str_ptr old_cwd = getcwd_();     
   if (old_cwd == NULL_STR) 
   {
      err_mesg(ERR_LEV_SPAM, 
         "IOManager::split_filename() - *WARNING* Couldn't save old current working directory!!");
   }

   //Grab char version of the fullpath str_ptr
   const char *c_fullpath = **fullpath;

   //Make room for the 3 strings
   char *c_path = new char[strlen(c_fullpath)+2]; assert(c_path);
   char *c_file = new char[strlen(c_fullpath)+2]; assert(c_file);
   char *c_ext  = new char[strlen(c_fullpath)+2]; assert(c_ext);


   //
   //First, grab the path part...
   strcpy(c_path,c_fullpath);

   //Find the last occurance of fwd or rev slash
   char *c_path_slash = max( strrchr(c_path,'/'), strrchr(c_path,'\\') );

   //No slash = no path ... Use CWD
   if (!c_path_slash)
   {
      strcpy(c_path,".");
   }
   //Terminate string after slash
   else 
   {
      ++c_path_slash;
      *c_path_slash = '\0';
   }



   //
   //Then, strip out the path part...

   //Find the last occurance of fwd or rev slash
   //VC 8 /*has*/ HAD a problem with this line 
   //there has been some changes to the standard library by Microsoft
   //strrchr(const char*, const char*) returns a const char* 
   //and is no longer possible to cast into char *
  
	const char *c_fullpath_slash = max( strrchr(c_fullpath,'/'), strrchr(c_fullpath,'\\') );

   //If found, copy everthying after the slash
   if (c_fullpath_slash)
   {
      ++c_fullpath_slash;
      strcpy(c_file, c_fullpath_slash);
   }
   //Else copy everything
   else
   {
      strcpy(c_file, c_fullpath);
   }


   //
   //Finally, deal with the extension

   char *c_file_dot = strrchr(c_file,'.');

   //Non-leading '.' starts extension...
   if (c_file_dot && (c_file_dot != c_file))
   {
      //Copy extension including leading '.'
      strcpy(c_ext, c_file_dot);

      //And lop it off of the file name
      *c_file_dot = '\0';
   }
   //Otherwise there's no extension...
   else
   {
      strcpy(c_ext,"");
   }


   //
   //Now turn the given path into an absolute, verified path...

   if (chdir_(c_path) && ((path = getcwd_()) != NULL_STR) )
   {
      //Success!

      //Copy the file and extension into return vars...
      file = str_ptr(c_file);
      ext  = str_ptr(c_ext);

      result = true;
   }
   else
   {
      //Failed to turn given path into real absolute path...

      result = false;
   }

   if ((old_cwd == NULL_STR) || !chdir_(old_cwd) )
   {
      err_mesg(ERR_LEV_SPAM, 
         "IOManager::split_filename() - *WARNING* Couldn't restore old current working directory!!");
   }

   delete[] c_path;
   delete[] c_file;
   delete[] c_ext;

   return result;
}
