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
// tablet.C

#if defined(linux) || defined(_AIX)
#include <sys/ioctl.h>
#elif WIN32
// XXX: Temporary until win32 serial support gets figured out
#define ioctl(a,b,c)
#else
#include <sys/filio.h>
#endif

#include "dev.H"
#include "tablet.H"

#include "std/config.H"

using namespace mlib;

#define BIT(N, INT)         ((1 << N) & ((int)(INT)))
#define BITS(SN, EN, INT)    (((1 << EN) -1) & (~((1 << SN)-1)) & ((int)(INT)))

static bool debug = Config::get_var_bool("DEBUG_TABLET",false,true);

TabletDesc::TabletDesc(
   TabletType t
   )
{
   switch (t) {
    case CROSS      : *this = TabletDesc(B19200, 5, "@",     5000);
      brcase LCD        : *this = TabletDesc(B9600,  7,  "",     3162);
      brcase TINY       : *this = TabletDesc(B9600,  7,  "",     3162);
      brcase MULTI_MODE : *this = TabletDesc(B9600,  7, "MU1\n", 15000);
      brcase LARGE      : *this = TabletDesc(B9600,  7,  "",     15000);
      brcase SMALL      : *this = TabletDesc(B9600,  7,  "",     6000);
      brcase INTUOS     : *this = TabletDesc(B9600,  9,  "MT1\nID1\nIT0\n", 16240);
   }
   _type = t;
}

Tablet::Tablet( 
   FD_MANAGER *manager,
   TabletDesc::TabletType  ttype,
   const char *dev,
   const char *name
   ) : TTYfd(manager, dev, name), _styl_buttons( &_stylus ), _desc(ttype)
{
   if (!_dev[0])
      strcpy(_dev, DEV_DFLT_SERIAL_PORT);
}

int 
Tablet::activate()
{
   int ret = TTYfd::activate();
   if (ret) {
      set_speed   (_desc._baud);
      set_stopbits(1);
      set_charsize(CS8);
      set_parity  (TTY_NONE);

      if (_desc._init_str != str_ptr()) {
         if (write(**_desc._init_str, strlen(**_desc._init_str), 20) < 0)
            perror("initializing tablet");
      }
   }

   return ret;
}

// Read the "tablet setting" from the Wacom Tablet using the ~R command.
// modify bit number 'bit_num' to be 1 or 0 based on 'value'.  Then
// write the "tablet setting" using the ~W command.
void
Tablet::mod_setting_bit( int bit_num, int value )
{
   // check that it's a wacom tablet..
   if (_desc._type == TabletDesc::TINY       ||
       _desc._type == TabletDesc::SMALL      ||
       _desc._type == TabletDesc::LARGE      ||
       _desc._type == TabletDesc::MULTI_MODE ||
       _desc._type == TabletDesc::LCD        ||
       _desc._type == TabletDesc::INTUOS) {
      char buffer[256];

      if (write("~R\n", 3, 20) < 0)
         perror("Tablet::mod_setting_bit(..)");

      nread(buffer, strlen("~Rdddddddd,ddd,dd,dddd,dddd\n"), 20);

      cerr << "buffer = '" << buffer << "'" << endl;

      int byte = (bit_num / 8);
      int bit  = (bit_num % 8);

      // only allow modification of 1st 8 bytes
      // (does it make sense to modify other bytes?)
      assert(byte >= 0 && byte < 8);

      if (value) {
         // (plus 2 because of the 2 command chars (i.e., "~R") )
         buffer[byte+2] |=  (1 << bit); 
      } else {
         buffer[byte+2] &= ~(1 << bit);
      }

      buffer[1] = '*'; // cmd to use so new settings take effect immediately
      cerr << "buffer = '" << buffer << "'" << endl;
//       write(buffer, strlen("~*dddddddd,ddd,dd,dddd,dddd\n"), 20);
   } else {
      cerr << "*** don't know how to 'mod_setting_bit' "
           << "for tablet in use.." << endl;
   }
}

TabletEvent::TabletEvent( 
   char        *buf, 
   TabletDesc  *desc
   ):_desc(desc), _is_valid(1)
{
   double yres = double(_desc->_yres);
   // Try to make sure this record is aligned properly-- that is the 7th
   // bit of the 1st byte should be set to `1`.
   assert(BIT(7, buf[0]));
  
   // Generate appropriate event for byte contents of 'buf'
   // Compute values for pos[0], pos[1], and buttons
   double rawx, rawy;

   if (_desc->_type == TabletDesc::CROSS) {
      rawx = (((buf[2] & 0x7f)<<7) | (buf[1] & 0x7f));// X coord from byte 1 & 2
      rawy = (((buf[4] & 0x7f)<<7) | (buf[3] & 0x7f));// Y coord from byte 4 & 5
      _pos[0]  = 2*(rawx/ 7000.0 - 0.5);
      _pos[1]  = 2*(rawy/ yres - 0.5);
      _buttons = buf[0] & 3;
      _device  = STYLUS;
      _touching_tablet = BIT(5,buf[0]);    // (is device touching the tablet?)

   } else if(_desc->_type == TabletDesc::INTUOS) {
      _device = STYLUS;
      int dev = BIT(0,buf[0]);
      static int device_type[2];
      _is_eraser = ((device_type[dev]&0x7ff) != 0x0022);

      if((buf[0] & 0xfc) == 0xc0) {
         /* device id packet */
         device_type[dev] = ((buf[1]&0x7f) << 5) | ((buf[2]&0x7c) >> 2);
         _is_valid = 0;
      }
      else if((buf[0] & 0xfe) == 0x80) {
         /* out of proximity packet */
         _touching_tablet = 0;
         _is_valid = 0;         /* hack to make jot work */
      }
      else if((buf[0]&0xb8) == 0xa0) {
         /* pen packet */
         rawx = (((buf[1] & 0x7f) << 9) | ((buf[2] & 0x7f) << 2)
                 | ((buf[3] & 0x60) >> 5));
         rawy = (((buf[3] & 0x1f) << 11) | ((buf[4] & 0x7f) << 4)
                 | ((buf[5] & 0x78) >> 3));


         // XXX - for 8x6 inch INTUOS tablet, investigations indicate
         // that the x-resolution is 20320:
         double sizex = 20320;
         double sizey = yres;

         // XXX - Temporary hack for 4x5 inch size.
         // event:
         static bool small_tablet = Config::get_var_bool("TABLET_SIZE_SMALL",false,true);
         
         if (small_tablet) {
            sizex=12700.0;
            sizey=10360.0;
         }

         _pos[0] = 2*(rawx/sizex - 0.5);
         _pos[1] = -2*(rawy/sizey - 0.5);
         _stylus_pressure = (((buf[5]&0x7)<<7) | (buf[6]&0x7f)) - 512;
         _buttons = ((buf[0] & 0x6) | (_stylus_pressure >= -480));
         _touching_tablet = BIT(6,buf[0]);

         if (Config::get_var_bool("DBUG_PRESSURE",false,true))
            cerr << _stylus_pressure << endl;

      }
      else {
         _is_valid = 0;
         fprintf(stderr, "Unknown tablet packet %02X\n", buf[0]);
      }
   } else {

      // Bit #5 of the first byte indicates which cursor's button and position
      // information is being reported.
      _device = (BIT(5,buf[0])) ? STYLUS : PUCK;

      rawx = (((buf[1] & 0x7f)<<7) | (buf[2] & 0x7f));
      rawy = (((buf[4] & 0x7f)<<7) | (buf[5] & 0x7f));
      if (_desc->_type == TabletDesc::LCD ||
          _desc->_type == TabletDesc::TINY) { 
         _pos[0] =  2*(rawx/ (yres * (211.2/158.4)) - 0.5);
         _pos[1] = -2*(rawy/  yres - 0.5);
      } else {
         if (yres > 7000)
            _pos[0] = 2*(rawx/ yres - 0.5);
         else _pos[0] = 2*(rawx/ yres - 0.675);
         _pos[1] = -2*(rawy/ yres - 0.5);
      }
      _buttons = (buf[3] >> 3) & 15; // Eraser & button in byte 3
      
      _touching_tablet = BIT(6,buf[0]);    // (is device touching the tablet?)
   
      _stylus_pressure = (int(buf[6]) & 0x3f) << 1 | ((int(buf[3]) >> 2) & 0x01);
      if (!BIT(6,buf[6]))
         _stylus_pressure += 127;

      if (Config::get_var_bool("DBUG_PRESSURE",false,true))
         cerr << _stylus_pressure << endl;

      if (_device == PUCK) {         // offset the position of the puck so your
         _pos[0] += 0.2;             // hand isn't "in the way"
         _pos[1] += 0.2;
      }
   }

   _pos[0] += Config::get_var_dbl("TABLET_OFFSET_X",0.0,true);
   _pos[1] += Config::get_var_dbl("TABLET_OFFSET_Y",0.0,true);
}

void 
TabletQueue::do_enqueue( 
   TabletEvent &event 
   )
{
   _queue[_tail] = event;

   if (_tail == _length-1)
      _tail = 0;
   else
      _tail++;

   if (_tail==_head)   // This should never happen!
      cerr << "*** ERROR: Queue overflow!" << char(7) << endl;
}

void 
TabletQueue::enqueue( 
   TabletEvent &event
   )
{
   if (empty()) // If there are no events in the queue, always enqueue this one.
      do_enqueue(event);
   else {
      // Determine the index to the last event (we know the queue is not empty)
      int last_event_index = ((_tail - 1) == -1) ? _length-1 : (_tail-1);

      // If the button state of this event is different from the last
      // event, always enqueue the event.
      if ( (event.get_buttons() != _queue[last_event_index].get_buttons()) )
         do_enqueue(event);
      else {
         // If the button state of this event is the same as the last event,
         // replace the last event with the current one.
         if (event.touching_tablet() != 
             _queue[last_event_index].touching_tablet())
            do_enqueue(event);
         else
            _queue[last_event_index] = event;
      }
   }
}

TabletEvent 
TabletQueue::dequeue()
{
   assert(!empty());

   TabletEvent event = _queue[_head];

   if( _head == _length-1 )
      _head = 0;
   else
      _head++;

   return event;
}

TabletMultimode::TabletMultimode( 
   FD_MANAGER              *mgr,
   TabletDesc::TabletType   ttype,
   const char              *tty,
   const char              *name
   ) : Tablet(mgr, ttype, tty, name), _puck_buttons(&_puck), 
   _using_stylus_eraser(0)
{
   _num_bytes_left_from_last_read = 0;

   _stylus_queue.reset();
   _puck_queue  .reset();
}

void 
TabletMultimode::enqueue_available_tablet_events()
{
   // Read all data in input buff & combine w/ any remaining data from
   // last read
   int bytes_read = nread(_tablet_data_buffer + _num_bytes_left_from_last_read,
//			  4096 - _num_bytes_left_from_last_read, // NO! read only 24bytes ahead
			  24 - _num_bytes_left_from_last_read,
			  _timeout);

   if ( bytes_read < 0 ) {
      // Could not read any bytes!? (... maybe no events were sent.)
      if (debug) {
         cerr << "** WARNING: TabletMultimode could not read any"  << endl;
         cerr << "            data from the tablet."               << endl;
         cerr << "            (bytes_read = " << bytes_read << ")" << endl;
      }
      return;
   }

   //cerr << "bytes_in_buffer = " << _num_bytes_left_from_last_read << "\t";
   // What is the total number of bytes we have read?
   int bytes_in_buffer = (bytes_read + _num_bytes_left_from_last_read);
   //cerr << "(" << bytes_in_buffer << ")" << endl;

   // Be sure we are synchronized with the tablet records-- if not, then
   // throw away the first few bytes until we are.
   if (! BIT(7, _tablet_data_buffer[0]) ) {
      // this code should only hapeen once, MAYBE, the first loop through this
      // method. Otherwise, we are losing bytes from the tablet or ... ???
      int offset = 1;
//       for (; !BIT(7, _tablet_data_buffer[offset]) && offset < _desc._rec_len; offset++) 
      for (; !BIT(7, _tablet_data_buffer[offset]) && offset < bytes_in_buffer; offset++) 
         ;

      if (offset == _desc._rec_len) {
         cerr << "** ERROR: Cannot find start of tablet record!" << endl;
//          assert(0);
      }

      for (int i = offset; i < bytes_in_buffer; i++)
         _tablet_data_buffer[i-offset] = _tablet_data_buffer[i];

      bytes_in_buffer -= offset;

      if (debug)
         cerr << "** Finding tablet record start (skipped "
             << offset << " bytes)" << endl;
   }

   // How many complete records are there in the input buffer?
   int num_complete_records = (bytes_in_buffer / _desc._rec_len);

   // Add all the events we know about to the queue of events to handle.
   for (int i=0; i < num_complete_records; i++) {
      if (BIT(7, _tablet_data_buffer[i*_desc._rec_len])) {
         // Create a new "TabletEvent"
         TabletEvent event(_tablet_data_buffer+i*_desc._rec_len, &_desc);

         if(event.is_valid()) {
            // Insert it into the queue of events for the right input device
            if (event.get_device() == TabletEvent::STYLUS)
               _stylus_queue.enqueue(event);
            else _puck_queue  .enqueue(event);
         }
      } else
         if (debug)
            cerr << "** WARNING: bad record read from tablet (#" <<i<<" )"
                 << endl;
   }

   // If an incomplete record has started to be read (ie. we read an uneven 
   // multiple of _rec_len  bytes) move those bytes to the front of the input 
   // buffer & handle them the next time this method is called.
   _num_bytes_left_from_last_read = (bytes_in_buffer % _desc._rec_len);
   for (int b=0; b<_num_bytes_left_from_last_read; b++)
      _tablet_data_buffer[b] = _tablet_data_buffer[bytes_in_buffer-
                                                  _num_bytes_left_from_last_read + b];
}

void 
TabletMultimode::sample()
{
   // Read in & enqueue all available tablet events
   enqueue_available_tablet_events();

   TabletEvent event;

   // Alternate between reporting stylus and puck events (if both queues 
   // have events waiting to be reported)
   if (!_stylus_queue.empty() && !_puck_queue.empty()) {
      if (_device_to_report == PUCK) {
         _device_to_report = STYLUS;

         event = _puck_queue.dequeue();
      }
      else {
         _device_to_report = PUCK;

         event = _stylus_queue.dequeue();
      }
   }
   else {
      if (!_stylus_queue.empty())
         event = _stylus_queue.dequeue();
      else if( !_puck_queue.empty() )
         event = _puck_queue.dequeue();
      else     // All queues are empty-- nothing to report!
         return;
   }

   Evd::DEVmod     mods       = DEVmod_gen::mods();
   DEVice_buttons &old_buttons= event.get_device() == TabletEvent::PUCK ? 
      _puck_buttons : _styl_buttons;

   if(_desc._type == TabletDesc::INTUOS) {
      /* Currently we only support the stylus */
      int bit;
      if(event.is_eraser()) bit = 3;
      else bit = 0;
      if(BIT(0, event.get_buttons()) != old_buttons.get(bit)) {
         if(BIT(0, event.get_buttons())) {
            old_buttons.event(bit, DEVice_buttons::DOWN, mods);
         }
         else old_buttons.event(bit, DEVice_buttons::UP, mods);
      }
      for(bit = 1; bit < 3; bit++) {
         int state = BIT(bit, event.get_buttons()) ? 1 : 0;
         if(state != old_buttons.get(bit)) {
            if(state) old_buttons.event(bit, DEVice_buttons::DOWN, mods);
            else old_buttons.event(bit, DEVice_buttons::UP, mods);
         }
      }
      if(event.is_eraser()) bit = 7;
      else bit = 8;
      if(event.touching_tablet() != old_buttons.get(bit)) {
         if(event.touching_tablet()) {
            old_buttons.event(bit, DEVice_buttons::DOWN, mods);
         }
         else old_buttons.event(bit, DEVice_buttons::UP, mods);
      }
      if (event.get_device() == TabletEvent::PUCK) {
         _puck  .event(XYpt(event.get_pos()[0],
                            event.get_pos()[1]), mods);
      }
      else {
         _stylus_pressure = event.stylus_pressure();
         _stylus.set_pressure((_stylus_pressure + 511)/1024.0);
         _stylus.event(XYpt(event.get_pos()[0],
                            event.get_pos()[1]), mods);
      }
   }
   else {
      // If the stylus is touching the tablet AND the 3rd button is down AND
      // the last event showed the stylus was *not* touching... then we're
      // using the eraser.
      //
      if (event.get_device() == TabletEvent::STYLUS &&
          BIT(2,event.get_buttons()) &&   // button 3 is pressed
          !old_buttons.get(2) &&          // and button 3 wasn't pressed before
          event.touching_tablet() &&      // and the stylus is touching now
          !old_buttons.get(8)) {          // but it wasn't before
         _using_stylus_eraser = true;
         old_buttons.event(7, DEVice_buttons::DOWN, mods);
         for (int i = 0; i < 2; i++) { // check the state of each button
            int state = BIT(i,event.get_buttons()) ? 1 : 0;
            if (state != old_buttons.get(i)) {
               if (state) {
                  if (i != 0) {
                     old_buttons.event(i, DEVice_buttons::DOWN, mods);
                  }
               } else {
                  old_buttons.event(i, DEVice_buttons::UP  , mods);
               }
            }
         }
      }

      // set new button state & generate button events
      if (event.get_device() == TabletEvent::STYLUS && _using_stylus_eraser) {
         // If button #1 (bit 0 of 'event.get_buttons()') is set, then
         // set button #4 (bit 3) of the stylus' buttons indicating the eraser
         // is being pressed on the tablet surface.
         const int eraser_bit = 3;

         int state = BIT(0, event.get_buttons()) ? 1 : 0;
         if (state != old_buttons.get(eraser_bit)) {
            if (state)
               old_buttons.event(eraser_bit, DEVice_buttons::DOWN, mods);
            else old_buttons.event(eraser_bit, DEVice_buttons::UP  , mods);
         }
      }
      else {
         if (event.touching_tablet()) {   // only generate button events when 
            //   touching the tablet
            for (int i = 0; i < 3; i++) { // check the state of each button
               int state = BIT(i,event.get_buttons()) ? 1 : 0;
               if (state != old_buttons.get(i)) {
                  if (state) 
                     old_buttons.event(i, DEVice_buttons::DOWN, mods);
                  else old_buttons.event(i, DEVice_buttons::UP  , mods);
               }
            }
         }
      }
     
      // set new 2d device position & generate events
      int touching(event.touching_tablet() ? 1 : 0);
      if (touching != old_buttons.get(8)) {
         if (touching)
            old_buttons.event(8, DEVice_buttons::DOWN, mods);
         else old_buttons.event(8, DEVice_buttons::UP  , mods);
      }

      if (event.get_device() == TabletEvent::PUCK) {
         _puck  .event(XYpt(event.get_pos()[0],
                            event.get_pos()[1]), mods);
      } else {
         _stylus_pressure = event.stylus_pressure();
         _stylus.event(XYpt(event.get_pos()[0],
                            event.get_pos()[1]), mods);
      }

      // If the stylus has been lifted from the tablet & we were drawing
      // with the eraser, then we're not using the eraser anymore.
      // NOTE: this must be done *after* buttons are set since they are
      // dependent on the value of '_using_stylus_eraser'.
      if ((!event.touching_tablet() || !BIT(2,event.get_buttons())) && 
          _using_stylus_eraser) {
         old_buttons.event(7, DEVice_buttons::UP, mods);
         _using_stylus_eraser = false;
      }
   }
}
