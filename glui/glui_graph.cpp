/****************************************************************************

  GLUI User Interface Toolkit
  ---------------------------

     glui_graph - GLUI_Graph control class


          --------------------------------------------------

  Copyright (c) 1998 Paul Rademacher

  This program is freely distributable without licensing fees and is
  provided without guarantee or warrantee expressed or implied. This
  program is -not- in the public domain.

*****************************************************************************/

#include "glui/glui_jot.H"
#include "glui_internal.h"
#include "glui_internal_control.h"
#include <cassert>

#define GLUI_GRAPH_FONT_HEIGHT					9
#define GLUI_GRAPH_FONT_DROP						3
#define GLUI_GRAPH_FONT_FULL_HEIGHT			(GLUI_GRAPH_FONT_HEIGHT + GLUI_GRAPH_FONT_DROP)
#define GLUI_GRAPH_FONT_MID_HEIGHT				4

#define GLUI_GRAPH_NAME_INDENT					6
#define GLUI_GRAPH_NAME_SIDE_BORDER			   2
#define GLUI_GRAPH_NAME_TOP_BORDER				2
#define GLUI_GRAPH_NAME_BOTTOM_BORDER			0
#define GLUI_GRAPH_IMG_TOP_BORDER				1
#define GLUI_GRAPH_IMG_BOTTOM_BORDER			2
#define GLUI_GRAPH_IMG_LEFT_BORDER				2
#define GLUI_GRAPH_IMG_RIGHT_BORDER				2


#define VALID_I(A)   { assert((A)>=0); assert((A)<graph_data_num); }

/********************** GLUI_GraphSeries::GLUI_GraphSeries() ******/

GLUI_GraphSeries::GLUI_GraphSeries(void)
{
   name[0]     = 0;
   type        = GLUI_GRAPH_SERIES_DOT;
   size        = 1.5;
   color[0]    = 0.0; color[1] = 0.0; color[2] = 0.0; color[3] = 1.0;
   data_num    = 0;
   data_x      = NULL;      data_y      = NULL;
}

/********************** GLUI_GraphSeries::~GLUI_GraphSeries() ******/

GLUI_GraphSeries::~GLUI_GraphSeries(void)
{
   clear_data();
}

/********************** GLUI_GraphSeries::clear_data() ******/

void  GLUI_GraphSeries::clear_data(void)
{
   if (data_num != 0)
   {
      delete[] data_x; delete[] data_y;
      data_num = 0; data_x = NULL; data_y = NULL;
   }
}

/********************** GLUI_GraphSeries::set_name() ******/

void  GLUI_GraphSeries::set_name(const char *n)
{
   int i = 0;
   for ( ; i<GLUI_GRAPH_SERIES_NAME_LENGTH-1; i++)
      if (n[i] != 0)
         name[i] = n[i];
      else
         break;
   name[i] = 0;
}

/********************** GLUI_GraphSeries::set_color() ******/

void  GLUI_GraphSeries::set_color(const double *c)
{
   for (int i=0; i<4; i++)
      color[i]=c[i];
}

/********************** GLUI_GraphSeries::set_data() ******/

void  GLUI_GraphSeries::set_data (int n, const double *x, const double *y)
{
   if (n > 0)
   {
      if (data_num != n)
      {
               clear_data(); data_num = n;
               data_x = new double[n]; data_y = new double[n];
               assert(data_y); assert(data_x);
      }
      for (int i=0; i<data_num; i++) { data_x[i] = x[i]; data_y[i] = y[i]; }
   }
}

/********************** GLUI_Graph::mouse_down_handler() ******/

int    GLUI_Graph::mouse_down_handler( int local_x, int local_y )
{
	int graph_x, graph_y;

	graph_x = local_x - x_abs - ((w+1)-graph_w)/2;
	if (graph_x < 0) graph_x = 0;
	if (graph_x >= graph_w) graph_x = graph_w-1;
	graph_y = (h - GLUI_GRAPH_IMG_BOTTOM_BORDER - 2) - (local_y - y_abs);
	if (graph_y < 0) graph_y = 0;
	if (graph_y >= graph_h) graph_y = graph_h-1;

	pressed = true;

	//printf("%d %d\n",graph_x, graph_y);

	event = GLUI_GRAPH_EVENT_MOUSE_DOWN;
	event_x = graph_x;
	event_y = graph_y;
	event_key = -1;
	event_mod = -1;

	execute_callback();

	return false;
}


/**************************** GLUI_Graph::mouse_up_handler() */

int    GLUI_Graph::mouse_up_handler( int local_x, int local_y, int inside )
{
	int graph_x, graph_y;

	graph_x = local_x - x_abs - ((w+1)-graph_w)/2;
	if (graph_x < 0) graph_x = 0;
	if (graph_x >= graph_w) graph_x = graph_w-1;
	graph_y = (h - GLUI_GRAPH_IMG_BOTTOM_BORDER - 2) - (local_y - y_abs);
	if (graph_y < 0) graph_y = 0;
	if (graph_y >= graph_h) graph_y = graph_h-1;

	pressed = false;

	//printf("%d %d\n",graph_x, graph_y);

	event = GLUI_GRAPH_EVENT_MOUSE_UP;
	event_x = graph_x;
	event_y = graph_y;
	event_key = -1;
	event_mod = -1;

	execute_callback();

	return false;
}


/****************************** GLUI_Graph::mouse_held_down_handler() ******/

int    GLUI_Graph::mouse_held_down_handler( int local_x, int local_y, int inside)
{

	int graph_x,graph_y;

	graph_x = local_x - x_abs - ((w+1)-graph_w)/2;
	if (graph_x < 0) graph_x = 0;
	if (graph_x >= graph_w) graph_x = graph_w-1;
	graph_y = (h - GLUI_GRAPH_IMG_BOTTOM_BORDER - 2) - (local_y - y_abs);
	if (graph_y < 0) graph_y = 0;
	if (graph_y >= graph_h) graph_y = graph_h-1;

	//printf("%d %d\n",graph_x, graph_y);

	event = GLUI_GRAPH_EVENT_MOUSE_MOVE;
	event_x = graph_x;
	event_y = graph_y;
	event_key = -1;
	event_mod = -1;

	execute_callback();

	return false;
}

/****************************** GLUI_Graph::special_handler() **********/

int    GLUI_Graph::special_handler( int key,int mods )
{

	//printf("%d %d\n",key,mods);

	event = GLUI_GRAPH_EVENT_KEY;
	event_x = -1;
	event_y = -1;
	event_key = key;
	event_mod = mods;

	execute_callback();

	return false;
}


/****************************** GLUI_Graph::draw() **********/

void    GLUI_Graph::draw( int x, int y )
{

  GLUI_DRAWINGSENTINAL_IDIOM;

	draw_emboss_box(	0,
							w,
							GLUI_GRAPH_NAME_TOP_BORDER +
							GLUI_GRAPH_FONT_HEIGHT - 1 -
							GLUI_GRAPH_FONT_MID_HEIGHT,
							h );

	draw_bkgd_box( GLUI_GRAPH_NAME_INDENT-1,
						GLUI_GRAPH_NAME_INDENT +
						string_width(name) +
						2*GLUI_GRAPH_NAME_SIDE_BORDER - 1,
						0,
						0 +
						GLUI_GRAPH_FONT_FULL_HEIGHT +
						GLUI_GRAPH_NAME_TOP_BORDER +
						GLUI_GRAPH_NAME_BOTTOM_BORDER);

	draw_name(	GLUI_GRAPH_NAME_INDENT +
					GLUI_GRAPH_NAME_SIDE_BORDER,
					GLUI_GRAPH_FONT_HEIGHT-1 +
					GLUI_GRAPH_NAME_TOP_BORDER);

	draw_active_area();

	draw_active_box(	GLUI_GRAPH_NAME_INDENT,
							GLUI_GRAPH_NAME_INDENT +
							string_width(name) +
							2*GLUI_GRAPH_NAME_SIDE_BORDER - 1,
							0,
							GLUI_GRAPH_FONT_FULL_HEIGHT +
							GLUI_GRAPH_NAME_TOP_BORDER +
							GLUI_GRAPH_NAME_BOTTOM_BORDER - 1);

}


/************************************ GLUI_Graph::update_size() **********/

void   GLUI_Graph::update_size( void )
{
	int name_w, img_w;

	if ( !glui )
		return;

	h =	GLUI_GRAPH_NAME_TOP_BORDER +
			GLUI_GRAPH_FONT_FULL_HEIGHT +
			GLUI_GRAPH_NAME_BOTTOM_BORDER +
			GLUI_GRAPH_IMG_TOP_BORDER +
			graph_h +
			GLUI_GRAPH_IMG_BOTTOM_BORDER +
			2 -
			1;

	name_w =	GLUI_GRAPH_NAME_INDENT +
				string_width(name) +
				2*GLUI_GRAPH_NAME_SIDE_BORDER +
				GLUI_GRAPH_NAME_INDENT -
				1;

	img_w = GLUI_GRAPH_IMG_LEFT_BORDER + GLUI_GRAPH_IMG_RIGHT_BORDER + graph_w + 2 + 2 - 1;

	if (img_w >= name_w)
		w = img_w;
	else
	{
		w = img_w + 2*((name_w - img_w + 1)/2);
	}
}


/****************************** GLUI_Graph::draw_active_area_translated() **********/

void    GLUI_Graph::draw_translated_active_area( int front )
{
	int win_h;
	int win_w;

	GLUI_DRAWINGSENTINAL_IDIOM;

	win_h = glutGet(GLUT_WINDOW_HEIGHT);
	win_w = glutGet(GLUT_WINDOW_WIDTH);

	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();
	glLoadIdentity();
	glTranslatef( (float) win_w/2.0, (float) win_h/2.0, 0.0 );
	glRotatef( 180.0, 0.0, 1.0, 0.0 );
	glRotatef( 180.0, 0.0, 0.0, 1.0 );
	glTranslatef( (float) -win_w/2.0, (float) -win_h/2.0, 0.0 );
	glTranslatef( (float) this->x_abs + .5, (float) this->y_abs + .5, 0.0 );

	draw_active_area();

	glMatrixMode( GL_MODELVIEW );
	glPopMatrix();

}

/****************************** GLUI_Graph::draw_active_area() **********/

void    GLUI_Graph::draw_active_area( void )
{

   double ox = 2 + GLUI_GRAPH_IMG_LEFT_BORDER;
   double oy = h - GLUI_GRAPH_IMG_BOTTOM_BORDER - 2;


	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();

   glPushAttrib( GL_ENABLE_BIT | GL_POINT_BIT | GL_LINE_BIT|
                  GL_HINT_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);


   //Draw in the bkg
   glClear(GL_STENCIL_BUFFER_BIT);
   glStencilFunc(GL_ALWAYS, 1, 1);
   glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);

   glEnable(GL_STENCIL_TEST);

   //Pixel space
   glTranslated(ox,oy,0.0);
   glScaled(1.0,-1.0,1.0);

   //Background
   glColor4dv(background);
   glBegin(GL_QUADS);
      glVertex2d( 0, -1);
      glVertex2d( graph_w, -1);
      glVertex2d( graph_w, graph_h-1);
      glVertex2d( 0, graph_h-1);
   glEnd();
/*
   //Erase the text
   glStencilFunc(GL_ALWAYS, 0, 1);
   glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);

   char buf[20];
   //XXX - Make labels optional
   sprintf(buf,"%#3.3g",max_y);
   draw_bkgd_box(1-GLUI_GRAPH_IMG_LEFT_BORDER,1-GLUI_GRAPH_IMG_LEFT_BORDER+string_width(buf),
                  graph_h-GLUI_GRAPH_FONT_HEIGHT, graph_h-GLUI_GRAPH_FONT_HEIGHT + GLUI_GRAPH_FONT_HEIGHT);
   glRasterPos2i(1-GLUI_GRAPH_IMG_LEFT_BORDER,graph_h-GLUI_GRAPH_FONT_HEIGHT);
   glColor4d(0.,0.,0.,0.);
   draw_string(buf);
   sprintf(buf,"%3.3g",min_y);
   glRasterPos2i(1-GLUI_GRAPH_IMG_LEFT_BORDER,0);
   glColor4d(0.,0.,0.,0.);
   draw_string(buf);
   sprintf(buf,"%3.3g",min_x);
   glRasterPos2i(0,2-GLUI_GRAPH_IMG_BOTTOM_BORDER);
   glColor4d(0.,0.,0.,0.);
   draw_string(buf);
   sprintf(buf,"%3.3g",max_x);
   glRasterPos2i(graph_w-string_width(buf),2-GLUI_GRAPH_IMG_BOTTOM_BORDER);
   glColor4d(0.,0.,0.,0.);
   draw_string(buf);
*/
   //Cull the series
   glStencilFunc(GL_EQUAL, 1, 1);
   glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

   //Axis space
   glScaled((graph_w-1.0)/(max_x-min_x),(graph_h-1.0)/(max_y-min_y),1.0);
   glTranslated(-min_x,-min_y,0.0);

   glEnable(GL_BLEND);                                   // GL_ENABLE_BIT
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);    // GL_COLOR_BUFFER_BIT

   glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);              // GL_HINT_BIT
   glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);               // GL_HINT_BIT

   //Series
   for (int i=0; i<graph_data_num; i++)
   {
      glColor4dv(graph_data[i].color);

      if (graph_data[i].type == GLUI_GRAPH_SERIES_DOT)
      {
         glPointSize(graph_data[i].size);                // GL_POINT_BIT
         glEnable(GL_POINT_SMOOTH);                      // GL_ENABLE_BIT
         glBegin(GL_POINTS);
      }
      else if (graph_data[i].type == GLUI_GRAPH_SERIES_LINE)
      {
         glLineWidth(graph_data[i].size);                // GL_LINE_BIT
         glEnable(GL_LINE_SMOOTH);                       // GL_ENABLE_BIT
         glBegin(GL_LINE_STRIP);
      }
      else assert(0);

      for (int j=0; j<graph_data[i].data_num; j++)
      {
               glVertex2d( graph_data[i].data_x[j], graph_data[i].data_y[j]);
      }
      glEnd();
   }

   glPopAttrib();
   glPopMatrix();
}

/****************************** GLUI_BitmapBox::redraw() **********/
void    GLUI_Graph::redraw()
{
  if ( NOT can_draw() ) return;

   draw_translated_active_area(true);
}


/****************************** GLUI_Graph::set_graph_size() **********/

void    GLUI_Graph::set_graph_size( int g_w, int g_h )
{
	if ((g_w>0) && (g_h>0))
	{

		graph_w = g_w;
		graph_h = g_h;

		update_size();

		if( glui ) glui->refresh();

	}
	else
	{
		fprintf(stderr,"GLUI_Graph::set_img_size - Bad dimensions.\n");
	}

}

/****************************** GLUI_Graph::get_num_series() **********/

int     GLUI_Graph::get_num_series( void )
{
   return graph_data_num;
}

/****************************** GLUI_Graph::set_num_series() **********/

void    GLUI_Graph::set_num_series(int n)
{
   clear_series();

   if (n>0)
   {
      graph_data_num = n;
      graph_data = new GLUI_GraphSeries[graph_data_num];
      assert(graph_data);
   }
}

/****************************** GLUI_Graph::clear_series() **********/

void    GLUI_Graph::clear_series()
{
   if (graph_data_num > 0)
   {
      delete[] graph_data;
      graph_data = NULL;
      graph_data_num = 0;
   }
}

/****************************** GLUI_Graph::set_series_name() **********/

void    GLUI_Graph::set_series_name(int i, const char *n)
{
   assert(i>=0 && i<graph_data_num);
   graph_data[i].set_name(n);
}

/****************************** GLUI_Graph::set_series_type() **********/

void    GLUI_Graph::set_series_type(int i, int t)
{
   assert(i>=0 && i<graph_data_num);
   graph_data[i].type = t;
}

/****************************** GLUI_Graph::set_series_size() **********/

void    GLUI_Graph::set_series_size(int i, double s)
{
   assert(i>=0 && i<graph_data_num);
   graph_data[i].size = s;
}

/****************************** GLUI_Graph::set_series_color() **********/

void    GLUI_Graph::set_series_color(int i, const double *c)
{
   assert(i>=0 && i<graph_data_num);
   graph_data[i].set_color(c);
}

/****************************** GLUI_Graph::set_series_data() **********/

void    GLUI_Graph::set_series_data(int i, int n, const double *x, const double *y)
{
   assert(i>=0 && i<graph_data_num);
   graph_data[i].set_data(n,x,y);
}

/****************************** GLUI_Graph::execute_callback() **********/

void    GLUI_Graph::execute_callback( void )
{

	GLUI_Control::execute_callback();

	event							= GLUI_GRAPH_EVENT_NONE;
	event_key					= -1;
	event_mod					= -1;
	event_x						= -1;
	event_y						= -1;

}

/************** GLUI_Graph::GLUI_Graph() ********************/

GLUI_Graph::GLUI_Graph(GLUI_Node *parent, const char *name,
                       int id, GLUI_CB cb)
{
   common_init();
   user_id = id;
   callback = cb;
   set_name( name );

   parent->add_control( this );
}

/************** GLUI_Graph::~GLUI_Graph() ********************/

GLUI_Graph::~GLUI_Graph( void )
{
   clear_series();
}


/************** GLUI_Graph::set_min_x() ********************/
void
GLUI_Graph::set_min_x( bool autom, double val)
{
   double maxx;
   if (!autom)
   {
      min_x = val;
   }
   else
   {
      bool init = false;
      // If val!=0, it should be used
      // as a %age by which to enlarge the range
      for (int i=0; i<graph_data_num; i++)
      {
         if ((!init)&&(graph_data[i].data_num !=0))
         {
            maxx = min_x = graph_data[i].data_x[0];
            init = true;
         }
         for (int j=0; j<graph_data[i].data_num; j++)
         {
            if (graph_data[i].data_x[j] < min_x)
            {
               min_x = graph_data[i].data_x[j];
            }
            if (graph_data[i].data_x[j] > maxx)
            {
               maxx = graph_data[i].data_x[j];
            }
         }
      }
      min_x = min_x - ((maxx - min_x)?(maxx - min_x):(min_x)) * (val);
   }
}

/************** GLUI_Graph::get_min_x() ********************/
double
GLUI_Graph::get_min_x()
{
   return min_x;
}

/************** GLUI_Graph::set_min_y() ********************/
void
GLUI_Graph::set_min_y( bool autom, double val)
{
   double maxy;
   if (!autom)
   {
      min_y = val;
   }
   else
   {
      bool init = false;
      // If val!=0, it should be used
      // as a %age by which to enlarge the range
      for (int i=0; i<graph_data_num; i++)
      {
         if ((!init)&&(graph_data[i].data_num !=0))
         {
            maxy = min_y = graph_data[i].data_y[0];
            init = true;
         }
         for (int j=0; j<graph_data[i].data_num; j++)
         {
            if (graph_data[i].data_y[j] < min_y)
            {
               min_y = graph_data[i].data_y[j];
            }
            if (graph_data[i].data_y[j] > maxy)
            {
               maxy = graph_data[i].data_y[j];
            }
         }
      }
      min_y = min_y - ((maxy - min_y)?(maxy - min_y):(min_y)) * (val);
   }
}

/************** GLUI_Graph::get_min_y() ********************/
double
GLUI_Graph::get_min_y()
{
   return min_y;
}

/************** GLUI_Graph::set_max_x() ********************/
void
GLUI_Graph::set_max_x( bool autom, double val)
{
   double minx;
   if (!autom)
   {
      max_x = val;
   }
   else
   {
      bool init = false;
      // If val!=0, it should be used
      // as a %age by which to enlarge the range
      for (int i=0; i<graph_data_num; i++)
      {
         if ((!init)&&(graph_data[i].data_num !=0))
         {
            minx = max_x = graph_data[i].data_x[0];
            init = true;
         }
         for (int j=0; j<graph_data[i].data_num; j++)
         {
            if (graph_data[i].data_x[j] > max_x)
            {
               max_x = graph_data[i].data_x[j];
            }
            if (graph_data[i].data_x[j] < minx)
            {
               minx = graph_data[i].data_x[j];
            }
         }
      }
      max_x = max_x + ((max_x - minx)?(max_x - minx):(max_x)) * (val);
   }
}

/************** GLUI_Graph::get_max_x() ********************/
double
GLUI_Graph::get_max_x()
{
   return max_x;
}

/************** GLUI_Graph::set_max_y() ********************/
void
GLUI_Graph::set_max_y( bool autom, double val)
{
   double miny;
   if (!autom)
   {
      max_y = val;
   }
   else
   {
      bool init = false;
      // If val!=0, it should be used
      // as a %age by which to enlarge the range
      for (int i=0; i<graph_data_num; i++)
      {
         if ((!init)&&(graph_data[i].data_num !=0))
         {
            miny = max_y = graph_data[i].data_y[0];
            init = true;
         }
         for (int j=0; j<graph_data[i].data_num; j++)
         {
            if (graph_data[i].data_y[j] > max_y)
            {
               max_y = graph_data[i].data_y[j];
            }
            if (graph_data[i].data_y[j] < miny)
            {
               miny = graph_data[i].data_y[j];
            }
         }
      }
      max_y = max_y + ((max_y - miny)?(max_y - miny):(max_y)) * (val);
   }
}

/************** GLUI_Graph::get_max_y() ********************/
double
GLUI_Graph::get_max_y()
{
   return max_y;
}

/****************************** GLUI_Graph::get_event() **********/

int   GLUI_Graph::get_event( void )
{
   return event;
}

/****************************** GLUI_Graph::get_event_key() **********/

int   GLUI_Graph::get_event_key( void )
{
   return event_key;
}

/****************************** GLUI_Graph::get_event_mod() **********/

int   GLUI_Graph::get_event_mod( void )
{
   return event_mod;
}

/****************************** GLUI_Graph::get_event_x() **********/

int   GLUI_Graph::get_event_x( void )
{
   return event_x;
}

/****************************** GLUI_Graph::get_event_y() **********/

int   GLUI_Graph::get_event_y( void )
{
   return event_y;
}

/****************************** GLUI_Graph::get_graph_w() **********/

int   GLUI_Graph::get_graph_w( void )
{
   return graph_w;
}

/****************************** GLUI_Graph::get_graph_h() **********/

int   GLUI_Graph::get_graph_h( void )
{
   return graph_h;
}

/****************************** GLUI_Graph::get_series_num() **********/

int    GLUI_Graph::get_series_num (int i)
{
   VALID_I(i);
   return graph_data[i].data_num;
}

/****************************** GLUI_Graph::get_series_name() **********/

const char *GLUI_Graph::get_series_name(int i)
{
   VALID_I(i);
   return graph_data[i].name;
}

/****************************** GLUI_Graph::get_series_type() **********/

int     GLUI_Graph::get_series_type(int i)
{
   VALID_I(i);
   return graph_data[i].type;
}

/****************************** GLUI_Graph::get_series_size() **********/

double  GLUI_Graph::get_series_size(int i)
{
   VALID_I(i);
   return graph_data[i].size;
}

/****************************** GLUI_Graph::get_series_color() **********/

const double *GLUI_Graph::get_series_color(int i)
{
   VALID_I(i);
   return graph_data[i].color;
}

/****************************** GLUI_Graph::get_series_data_x() **********/

const double *GLUI_Graph::get_series_data_x(int i)
{
   VALID_I(i);
   return graph_data[i].data_x;
}

/****************************** GLUI_Graph::get_series_data_y() **********/

const double *GLUI_Graph::get_series_data_y(int i)
{
   VALID_I(i);
   return graph_data[i].data_y;
}

/****************************** GLUI_Graph::set_background() **********/

void    GLUI_Graph::set_background(const double *c)
{
   for (int i=0; i<4; i++)
      background[i]=c[i];
}

/****************************** GLUI_Graph::get_background() **********/

const double *GLUI_Graph::get_background()
{
   return background;
}

