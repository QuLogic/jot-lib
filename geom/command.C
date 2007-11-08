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
#include "geom/world.H"
#include "geom/command.H"

/*****************************************************************
 * COMMAND:
 *****************************************************************/
bool
COMMAND::doit()
{
   _is_done = true;
   _is_undone = false;
   return true;
}

bool
COMMAND::undoit()
{
   _is_done = false;
   _is_undone = true;
   return true;
}

bool
COMMAND::clear()
{
   if (is_done())
      return false;

   _is_done = _is_undone = false;
   return true;
}

/*****************************************************************
 * UNDO_CMD:
 *****************************************************************/
bool
UNDO_CMD::doit()
{
   if (_cmd && _cmd->undoit())
      return COMMAND::doit();   // update state in COMMAND
   return false;
}

bool
UNDO_CMD::undoit()
{
   if (_cmd && _cmd->doit())
      return COMMAND::undoit(); // update state in COMMAND
   return false;
}

bool
UNDO_CMD::clear()
{
   if (!COMMAND::clear())       // update state in COMMAND
      return false;

   if (_cmd)
      _cmd->clear();
   return true;
}

/*****************************************************************
 * DISPLAY_CMD:
 *****************************************************************/
bool
DISPLAY_CMD::doit()
{
   for (int i = 0; i < _gels.num(); i++)
      WORLD::display (_gels[i], false);

   return COMMAND::doit();      // update state in COMMAND
}

bool
DISPLAY_CMD::undoit()
{
   // Go in reverse order, in case it matters:
   for (int i = _gels.num()-1; i >= 0 ; i--)
      WORLD::undisplay(_gels[i], false);

   return COMMAND::undoit();    // update state in COMMAND
}

/*****************************************************************
 * MULTI_CMD:
 *****************************************************************/
bool 
MULTI_CMD::doit()
{
   for (int i=0; i <_commands.num(); i++) 
      _commands[i]->doit();

   return COMMAND::doit();      // update state in COMMAND
}

bool
MULTI_CMD::undoit() 
{
   // undo in reverse order
   for (int i=_commands.num()-1; i >= 0; i--) 
      _commands[i]->undoit();

   return COMMAND::undoit();    // update state in COMMAND
}

bool
MULTI_CMD::clear() 
{
   if (!COMMAND::clear())       // update state in COMMAND
      return false;

   // Clear in reverse order
   for (int i=_commands.num()-1; i >= 0; i--) 
      _commands[i]->clear();
   return true;
}

/* end of file command.C */
