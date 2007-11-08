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

#include "glui.h"

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
  int orig;

  if ( !glui )
    return;

	orig = set_to_glut_window();


	draw_emboss_box(	0, 
							w,
							GLUI_GRAPH_NAME_TOP_BORDER +
							GLUI_GRAPH_FONT_HEIGHT - 1 - 
							GLUI_GRAPH_FONT_MID_HEIGHT, 
							h );

	draw_bkgd_box( GLUI_GRAPH_NAME_INDENT-1, 
						GLUI_GRAPH_NAME_INDENT + 
						string_width(name.string) + 
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

	restore_window(orig);

	draw_active_box(	GLUI_GRAPH_NAME_INDENT, 
							GLUI_GRAPH_NAME_INDENT +
							string_width(name.string) + 
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
				string_width(name.string) + 
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
	int orig,state;
	int win_h; 
	int win_w;

	if ( !glui )
		return;

	orig = set_to_glut_window();

	win_h = glutGet(GLUT_WINDOW_HEIGHT);
	win_w = glutGet(GLUT_WINDOW_WIDTH);

	if (front) state = glui->set_front_draw_buffer();

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

	if (front) glui->restore_draw_buffer(state);  

	restore_window(orig);
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

GLUI_Graph::GLUI_Graph( void ) 
{
	sprintf( name, "Graph: %p", this );

	type							= GLUI_CONTROL_GRAPH;
	w								= GLUI_GRAPH_WIDTH;
	h								= GLUI_GRAPH_HEIGHT;
	can_activate				= true;
	live_type					= GLUI_LIVE_NONE;
	alignment					= GLUI_ALIGN_CENTER;

	pressed						= false;

	event							= GLUI_GRAPH_EVENT_NONE;
	event_key					= -1;
	event_mod					= -1;
	event_x						= -1;
	event_y						= -1;

	graph_data					= NULL;
   graph_data_num          = 0;

	graph_w						= GLUI_GRAPH_WIDTH;
	graph_h						= GLUI_GRAPH_HEIGHT;

   min_x                   = 0.0;
   min_y                   = 0.0;
   max_x                   = 1.0;
   max_y                   = 1.0;

   background[0]           = 0.8;
   background[1]           = 0.8;
   background[2]           = 0.8;
   background[3]           = 1.0;
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

