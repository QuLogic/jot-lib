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
#ifndef COMMAND_H_IS_INCLUDED
#define COMMAND_H_IS_INCLUDED

/*!
 *  \file command.H
 *  \brief Contains the definition of the COMMAND class.  A class
 *  representing a command that can be done and undone.  Some derived
 *  classes are also defined here.
 *
 *  \sa command.C
 *
 */

#include "geom/geom.H"
#include "disp/view.H"
#include "std/ref.H" // for REFcounter

class COMMAND;
typedef const COMMAND CCOMMAND;
MAKE_PTR_BASEC(COMMAND);
typedef const COMMANDptr CCOMMANDptr;

/*!
 *  \brief Base class for "commands" that can be executed 
 *  (COMMAND::doit()) and undone (COMMAND::undoit()).
 *
 *  A COMMAND can be in one of 3 possible states:
 *  
 *  - done:   doit() was executed.
 *  - undone: undoit() was executed after doit().
 *  - clear:  neither doit() nor undoit() have been called.
 *  
 *  For many types of commands, there is no difference
 *  between 'undone' and 'clear'. An example of where the two
 *  WOULD be different is a command that generates a portion
 *  of mesh in doit(), but in undoit() simply hides the
 *  portion of mesh instead of destroying it. That way,
 *  redoing the command by calling doit() after undoit()
 *  would be relatively lightweight: the portion of mesh
 *  would be unhidden rather than regenerated.
 *  
 *  The virtual method COMMAND::clear() is used to transition
 *  from state 'undone' to state 'clear'; e.g. the mesh
 *  generation command in the example above would delete the
 *  region of mesh in clear(), returning the command (and the
 *  mesh) to its initial state before the first doit() was
 *  executed. Most derived classes will not need to override
 *  COMMAND::clear().
 *  
 *  The following are the actions that can be taken from
 *  each state:
 *  
 *    state: 'clear'
 *      doit()   changes state to 'done'
 *      undoit() is a no-op
 *      clear()  is a no-op
 *  
 *    state: 'done'
 *      doit()   is a no-op
 *      undoit() changes state to 'undone'
 *      clear()  is an invalid action
 *  
 *    state: 'undone'
 *      doit()   changes state to 'done'
 *      undoit() is a no-op
 *      clear()  changes state to 'clear'
 *  
 */
class COMMAND : public REFcounter {
   
 public:

   //! \name Run-Time Type Id
   //@{
      
   DEFINE_RTTI_METHODS_BASE("COMMAND", CCOMMAND*);
      
   //@}
      
   //! \name Constructors
   //@{
   
   COMMAND(bool done=false): _is_done(done), _is_undone(false) {}
      
   //@}
   
   //! \name Accessors
   //@{
   
   //! \brief Neither doit() nor undoit() have been called.
   bool is_clear()      const   { return !(_is_done || _is_undone); }
   
   //! \brief doit() was called most recently.
   bool is_done()       const   { return _is_done; } 
   
   //! \brief undoit() was called most recently, after doit().
   bool is_undone()     const   { return _is_undone; }
      
   //@}
   
   //! \name Command Virtual Methods
   //@{
   
   //! \brief Execute the command.
   //! \return \c true on success.
   //! If the command is already done, this is a no-op and still returns \c true.
   virtual bool doit();
   
   //! \brief Undo the command.
   //! \return \c true on success.
   //! If the command is already undone, this is a no-op and still returns \c true.
   virtual bool undoit();
   
   //! \brief After doit() and undoit() were called, restore things to the way
   //! they were before (may be a no-op for most derived classes).
   virtual bool clear();
      
   //@}
   
   //! \name Diagnostic
   //@{
   
   virtual void print() const { cerr << class_name() << " "; }
      
   //@}

 protected:
      
   bool _is_done;       //!< \brief \c true if doit() was executed most recently.
   bool _is_undone;     //!< \brief \c true if undoit() was executed most recently.
      
};

MAKE_PTR_SUBC(UNDO_CMD,COMMAND);

/*!
 *  \brief Given some command, UNDO_CMD performs the reverse operation.
 *
 */
class UNDO_CMD : public COMMAND {
   
 public:
   
   //! \name Constructors
   //@{
   
   UNDO_CMD(const COMMANDptr& c) : _cmd(c) {}
      
   //@}
   
   //! \name Run-Time Type Id
   //@{
   
   DEFINE_RTTI_METHODS3("UNDO_CMD", UNDO_CMD*, COMMAND, CCOMMAND*);
      
   //@}
   
   //! \name Command Virtual Methods
   //@{
   
   virtual bool doit();
   virtual bool undoit();
   virtual bool clear();
      
   //@}
   
   //! \name Diagnostic
   //@{
   
   virtual void print() const {
      cerr << class_name() << ": { ";
      if (_cmd) _cmd->print();
      cerr << " } ";
   }
      
   //@}
   
 protected:
      
   COMMANDptr   _cmd;
   
};

MAKE_PTR_SUBC(DISPLAY_CMD,COMMAND);

/*!
 *  \brief Put a GEL or GELs in the WORLD's display list.
 *
 *  \question Should this class be moved to a different header (since it is a
 *  concrete COMMAND rather than an abstract command)?
 *
 */
class DISPLAY_CMD : public COMMAND {
   
 public:
   
   //! \name Constructors
   //@{
   
   DISPLAY_CMD(CGELptr&  gel ) : _gels(gel)  {}
   DISPLAY_CMD(CGELlist& gels) : _gels(gels) {}
      
   //@}
   
   //! \name Run-Time Type Id
   //@{
   
   DEFINE_RTTI_METHODS3("DISPLAY_CMD", DISPLAY_CMD*, COMMAND, CCOMMAND*);
      
   //@}
   
   //! \name Command Virtual Methods
   //@{
   
   virtual bool doit();
   virtual bool undoit();
      
   //@}
   
   //! \name Diagnostic
   //@{
   
   virtual void print() const {
      cerr << class_name() << " { ";
      for (int i=0; i<_gels.num(); i++)
         cerr << _gels[i]->class_name() << " ";
      cerr << " } ";
   }
      
   //@}
   
 protected:
      
   GELlist  _gels;

};

MAKE_PTR_SUBC(UNDISPLAY_CMD, UNDO_CMD);

/*!
 *  \brief Remove a GEL or GELs from the WORLD's display list.
 *
 *  \question Should this class be moved to a different header (since it is a
 *  concrete COMMAND rather than an abstract command)?
 *
 */
class UNDISPLAY_CMD : public UNDO_CMD {
 public:

   //! \name Constructors
   //@{

   UNDISPLAY_CMD(CGELptr&  gel ) : UNDO_CMD(new DISPLAY_CMD(gel))  {}
   UNDISPLAY_CMD(CGELlist& gels) : UNDO_CMD(new DISPLAY_CMD(gels)) {}
   
   //@}

   //! \name Run-Time Type Id
   //@{

   DEFINE_RTTI_METHODS3("UNDISPLAY_CMD", UNDISPLAY_CMD*, UNDO_CMD, CCOMMAND*);
   
   //@}

};

MAKE_PTR_SUBC(MULTI_CMD,COMMAND);

/*!
 *  \brief Perform multiple commands at once.
 *
 */
class MULTI_CMD : public COMMAND {
   
 public:

   //! \name Run-Time Type Id
   //@{
   
   DEFINE_RTTI_METHODS3("MULTI_CMD", MULTI_CMD*, COMMAND, CCOMMAND*);
      
   //@}
      
   //! \name COMMAND List Operations
   //@{
   
   CLIST<COMMANDptr>& commands() const  { return _commands; }
   bool               is_empty() const  { return _commands.empty(); }
   int                     num() const  { return _commands.num(); }
   
   void         add(CCOMMANDptr& cmd)   { _commands += cmd; }
   void         rem(CCOMMANDptr& cmd)   { _commands -= cmd; }
   COMMANDptr   pop()                   { return _commands.pop(); }
   
   CCOMMANDptr& last()          const   { return _commands.last(); }
      
   //@}
   
   //! \name Command Virtual Methods
   //@{
   
   virtual bool doit();
   virtual bool undoit();
   virtual bool clear();
      
   //@}
   
   //! \name Diagnostic
   //@{
   
   virtual void print() const {
      cerr << class_name() << " { ";
      for (int i=0; i<_commands.num(); i++)
         _commands[i]->print();
      cerr << " } ";
   }
      
   //@}

 protected:
      
   LIST<COMMANDptr> _commands;

};

#endif // COMMAND_H_IS_INCLUDED

/* end of file command.H */
