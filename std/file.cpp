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

#include "file.H" 

#ifndef WIN32
#include <dirent.h>
#include <sys/stat.h>
#endif

//////////////////////////////////////////////////////
// rename_()
//////////////////////////////////////////////////////
bool rename_(const string &old_name, const string &new_name)
{   
   int ret;

#ifdef WIN32
   ret = rename(old_name.c_str(), new_name.c_str());
#else
   //XXX - Untested
   ret = rename(old_name.c_str(), new_name.c_str());
#endif
   
   if (ret != 0)
   {
      cerr << "rename_() - ERROR!! Couldn't rename: '" << old_name << "' to: '" << new_name << "'\n";
      return false;
   }
   else
   {
      return true;
   }
}

//////////////////////////////////////////////////////
// remove_()
//////////////////////////////////////////////////////
bool remove_(const string &file)
{   
   int ret;

#ifdef WIN32
   ret = remove(file.c_str());
#else
   //XXX - Untested
   ret = remove(file.c_str());
#endif
   
   if (ret != 0)
   {
      cerr << "remove_() - ERROR!! Couldn't remove: '" << file << "'\n";
      return false;
   }
   else
   {
      return true;
   }
}

//////////////////////////////////////////////////////
// rmdir_()
//////////////////////////////////////////////////////
bool rmdir_(const string &dir)
{   
   int ret;

#ifdef WIN32
   ret = _rmdir(dir.c_str());
#else
   //XXX - Untested
   ret = rmdir(dir.c_str());
#endif
   
   if (ret != 0)
   {
      cerr << "rmdir_() - ERROR!! Couldn't rmdir: '" << dir << "'\n";
      return false;
   }
   else
   {
      return true;
   }
}

//////////////////////////////////////////////////////
// mkdir_()
//////////////////////////////////////////////////////
bool mkdir_(const string &dir)
{   
   int ret;

#ifdef WIN32
   ret = _mkdir(dir.c_str());
#else
   //XXX - Untested
   ret = mkdir(dir.c_str(), S_IRUSR  | S_IWUSR  | S_IXUSR |
                            S_IRGRP/*| S_IWGRP*/| S_IXGRP |
                            S_IROTH/*| S_IWOTH*/| S_IXOTH );
#endif
   
   if (ret != 0)
   {
      cerr << "mkdir_() - ERROR!! Couldn't mkdir: '" << dir << "'\n";
      return false;
   }
   else
   {
      return true;
   }
}

//////////////////////////////////////////////////////
// chdir_()
//////////////////////////////////////////////////////
bool chdir_(const string &new_dir)
{   
   int ret;

#ifdef WIN32
   ret = _chdir(new_dir.c_str());
#else
   //XXX - Untested
   ret = chdir(new_dir.c_str());
#endif
   
   if (ret != 0)
   {
      cerr << "chdir_() - ERROR!! Couldn't CHDIR to: '" << new_dir << "'\n";
      return false;
   }
   else
   {
      return true;
   }
}

#define CWD_BUF 1024
//////////////////////////////////////////////////////
// getcwd_()
//////////////////////////////////////////////////////
string
getcwd_()
{
   char *ret, cwd[CWD_BUF];

#ifdef WIN32
   ret = _getcwd(cwd,CWD_BUF);
#else
   //XXX - Untested
   ret = getcwd(cwd,CWD_BUF);
#endif
   
   if (!ret)
   {
      cerr << "getcwd_() - ERROR!! Couldn't retrieve CWD!\n";
      return "";
   }
   else
   {
      return cwd;
   }
}

//
// Gets a list of files in a certain directory
//
vector<string> dir_list(const string &path)
{
   vector<string> list;
#ifdef WIN32
   WIN32_FIND_DATA file;
   HANDLE hFile;

   const string dot(".");
   const string dotdot("..");
   string current = path+"*";

   if ((hFile = FindFirstFile(current.c_str(), &file)) != INVALID_HANDLE_VALUE) {
      while (true) {
         string fname(file.cFileName);
         if ((fname != dot) && (fname != dotdot)) list.push_back(fname);
         if (!FindNextFile(hFile, &file)) break;
      }
      FindClose(hFile);
   }
#else
   DIR *dir = nullptr;
   struct dirent *direntry;
   if (!path.empty() && (dir = opendir(path.c_str()))) {
      const string dot(".");
      const string dotdot("..");
      struct stat statbuf;
      while ((direntry = readdir(dir))) {
         string file(direntry->d_name);
         if (file != dot && file != dotdot) {
            string path_to_file = path + "/" + file;
            if (!stat(path_to_file.c_str(), &statbuf) && (statbuf.st_mode & S_IFMT) == S_IFREG) {
               list.push_back(file);
            }
         }
      }
      closedir(dir);
   }
#endif

   std::sort(list.begin(), list.end());
   return list;
}

