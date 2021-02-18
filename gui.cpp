// jluplot demo
#include <locale.h>
#include <gtk/gtk.h>
#include <cairo-pdf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>

using namespace std;
#include <string>
#include <iostream>
#include <vector>

#include "jluplot.h"
#include "layers.h"
#include "gluplot.h"
#include "process.h"
#include "glostru.h"
#include "modpop2.h"
#include "nb_serial.h"

// unique variable globale exportee pour gasp() de modpop2
GtkWindow * global_main_window = NULL;

// le contexte de l'appli
static glostru theglo;

/** ============================ serial call back ======================= */

void update_status_X( char * msg )
{
gtk_entry_set_text( GTK_ENTRY(theglo.erpX), msg );
}

void update_status_Y( char * msg )
{
gtk_entry_set_text( GTK_ENTRY(theglo.erpY), msg );
}

/** ============================ GTK call backs ======================= */

int idle_call( glostru * glo )
{
glo->pro.step();	// recuperer les data du port serie
++glo->idle_profiler_cnt;
// la partie graphique
unsigned int bufstep = glo->pro.bufspan / glo->panneau.ndx;	// samples par pixel
// moderation : on n'agit que si le nombre de samples produit au moins 1 pixel de scroll
if	( ( glo->running ) && ( ( glo->pro.wri - glo->pro.rdi ) > bufstep ) )
	{
	// preparer le scroll
	int u0, u1;
	u1 = ( glo->pro.wri - 2 ) & BUFMASK;
	glo->pro.rdi = glo->pro.wri;
	//u1 += QBUF;	// pour ne voir que des abcisses positives
	u0 = u1 - glo->pro.bufspan;
	glo->panneau.zoomM( double(u0), double(u1) );

	glo->panneau.force_redraw = 1;
	glo->panneau.force_repaint = 1;
	}

// la partie textuelle
if	( ( glo->idle_profiler_cnt % 32 ) == 0 )
	{
	fflush(stdout);
	}

// moderateur de drawing
if	( ( glo->panneau.queue_flag ) || ( glo->panneau.force_repaint ) )
	{
	// gtk_widget_queue_draw( glo->darea );
	glo->panneau.queue_flag = 0;
	glo->panneau.paint();
	}

return( -1 );
}

gint close_event_call( GtkWidget *widget,
                        GdkEvent  *event,
                        gpointer   data )
{
gtk_main_quit();
return (TRUE);		// ne pas destroyer tout de suite
}


void quit_call( GtkWidget *widget, glostru * glo )
{
gtk_main_quit();
}

void run_call( GtkWidget *widget, glostru * glo )
{
glo->running = 1;
}
void pause_call( GtkWidget *widget, glostru * glo )
{
glo->running = 0;
}

void cmd_call( GtkWidget *widget, glostru * glo )
{
// const char * cmd;
// cmd = gtk_entry_get_text( GTK_ENTRY(widget) );
}

/** ============================ GLUPLOT call backs =============== */

void clic_call_back( double M, double N, void * vglo )
{
// glostru * glo = (glostru *)vglo;
// printf("youtpi %g %g\n", M, N );
}

void key_call_back( int v, void * vglo )
{
glostru * glo = (glostru *)vglo;
switch	( v )
	{
	case '0' : glo->panneau.toggle_vis( 0, -1 ); break;
	case 'x' : glo->panneau.toggle_vis( 0, 0 ); break;
	case 'y' : glo->panneau.toggle_vis( 0, 1 ); break;
	case 'Z' : if	( glo->pro.bufspan < (QBUF/2) )	// zoom horizontal
			glo->pro.bufspan *= 2;
		break;
	case 'z' : if	( glo->pro.bufspan > 16 )	// zoom horizontal
			glo->pro.bufspan /= 2;
		break;
	case 'V' :			// zoom vertival courbes 0 et 1 full sur partie visible (bufspan)
		{			// N.B. le full de l'axe vertical prendrait la session entiere
		unsigned int i, u; float v, Vmin, Vmax;
		u = glo->panneau.bandes[0]->courbes[0]->u0;
		Vmin = Vmax = glo->pro.Xbuf[u&BUFMASK];		// on initialise avec le premier element
		for	( i = 0; i < glo->pro.bufspan; ++i )
			{
			v = glo->pro.Xbuf[(u+i)&BUFMASK];
			if ( v < Vmin ) Vmin = v;
			if ( v > Vmax ) Vmax = v;
			v = glo->pro.Ybuf[(u+i)&BUFMASK];
			if ( v < Vmin ) Vmin = v;
			if ( v > Vmax ) Vmax = v;
			}
		// normalement il faudrait utiliser NdeV() mais ici N=V, alors...
		glo->panneau.bandes[0]->zoomN( double(Vmin), double(Vmax) );
		glo->panneau.force_redraw = 1;
		glo->panneau.force_repaint = 1;
		} break;
	case 'd' :
		{
		glo->panneau.dump();
		fflush(stdout);
		} break;
	// codes pour les multimetres 1604
	case 'g' :	// on/off
	case 'u' :	// connect
	case 'v' :	// disconnect
		{
		char c = (char)v;
		printf("tx '%c'\n", c ); fflush(stdout);
		nb_serial_write( &c, 1 );
		}
		break;
	}
}

/** ============================ context menus ======================= */


/** ============================ main, quoi ======================= */


int main( int argc, char *argv[] )
{
glostru * glo = &theglo;
GtkWidget *curwidg;

gtk_init(&argc,&argv);

setlocale( LC_ALL, "C" );       // kill the frog, AFTER gtk_init

curwidg = gtk_window_new( GTK_WINDOW_TOPLEVEL );

gtk_signal_connect( GTK_OBJECT(curwidg), "delete_event",
                    GTK_SIGNAL_FUNC( close_event_call ), NULL );
gtk_signal_connect( GTK_OBJECT(curwidg), "destroy",
                    GTK_SIGNAL_FUNC( gtk_main_quit ), NULL );

gtk_window_set_title( GTK_WINDOW (curwidg), "AimTTi 1604");
gtk_container_set_border_width( GTK_CONTAINER( curwidg ), 10 );
glo->wmain = curwidg;
global_main_window = (GtkWindow *)curwidg;

/* creer boite verticale */
curwidg = gtk_vbox_new( FALSE, 5 ); /* spacing ENTRE objets */
gtk_container_add( GTK_CONTAINER( glo->wmain ), curwidg );
glo->vmain = curwidg;

/* creer une drawing area resizable depuis la fenetre */
glo->darea = glo->panneau.layout( 800, 600 );
gtk_box_pack_start( GTK_BOX( glo->vmain ), glo->darea, TRUE, TRUE, 0 );

/* creer une drawing area  qui ne sera pas resizee en hauteur par la hbox
   mais quand meme en largeur (par chance !!!) */
glo->sarea = glo->zbar.layout( 640 );
gtk_box_pack_start( GTK_BOX( glo->vmain ), glo->sarea, FALSE, FALSE, 0 );


/* creer boite horizontale */
curwidg = gtk_hbox_new( FALSE, 10 ); /* spacing ENTRE objets */
gtk_container_set_border_width( GTK_CONTAINER (curwidg), 5);
gtk_box_pack_start( GTK_BOX( glo->vmain ), curwidg, FALSE, FALSE, 0 );
glo->hbut = curwidg;

/* simple bouton */
curwidg = gtk_button_new_with_label (" Run ");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( run_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, TRUE, TRUE, 0 );
glo->bpla = curwidg;
//*/
/* simple bouton */
curwidg = gtk_button_new_with_label (" Pause ");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( pause_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, TRUE, TRUE, 0 );
glo->brew = curwidg;
//*/
// entree editable
curwidg = gtk_entry_new();
gtk_widget_set_usize( curwidg, 100, 0 );
gtk_entry_set_editable( GTK_ENTRY(curwidg), TRUE );
g_signal_connect( curwidg, "activate",
                  G_CALLBACK( cmd_call ), (gpointer)glo );
gtk_entry_set_text( GTK_ENTRY(curwidg), "" );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, FALSE, FALSE, 0 );
glo->esta = curwidg;

// entree non editable
curwidg = gtk_entry_new();
gtk_widget_set_usize( curwidg, 160, 0 );
gtk_entry_set_editable( GTK_ENTRY(curwidg), FALSE );
gtk_entry_set_text( GTK_ENTRY(curwidg), "X" );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, FALSE, FALSE, 0 );
glo->erpX = curwidg;

// entree non editable
curwidg = gtk_entry_new();
gtk_widget_set_usize( curwidg, 160, 0 );
gtk_entry_set_editable( GTK_ENTRY(curwidg), FALSE );
gtk_entry_set_text( GTK_ENTRY(curwidg), "Y" );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, FALSE, FALSE, 0 );
glo->erpY = curwidg;


/* simple bouton */
curwidg = gtk_button_new_with_label (" Quit ");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( quit_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, TRUE, TRUE, 0 );
glo->bqui = curwidg;

glo->panneau.offscreen_flag = 0;

// connecter la zoombar au panel et inversement
glo->panneau.zoombar = &glo->zbar;
glo->panneau.zbarcall = gzoombar_zoom;
glo->zbar.panneau = &glo->panneau;

gtk_widget_show_all( glo->wmain );

int portnum = 0;
if	( argc >= 2 )
	portnum = atoi(argv[1] );

#define MY_BAUD_RATE 9600
portnum = nb_open_serial_ro( portnum, MY_BAUD_RATE );
if	( portnum >=1 )
	{ printf("ouverture COM%d a %d bauds Ok\n", portnum, MY_BAUD_RATE ); fflush(stdout);  }
else	gasp("failed open port COM, code %d", portnum );

glo->idle_profiler_cnt = 0;
glo->running = 0;

// preparer le layout
glo->pro.X_status_call = update_status_X;
glo->pro.Y_status_call = update_status_Y;
glo->pro.run = &glo->running;
glo->pro.prep_layout( &glo->panneau );
glo->pro.connect_layout( &glo->panneau );

fflush(stdout);

glo->panneau.clic_callback_register( clic_call_back, (void *)glo );
glo->panneau.key_callback_register( key_call_back, (void *)glo );


// forcer un full initial pour que tous les coeffs de transformations soient a jour
glo->panneau.full_valid = 0;
// refaire un configure car celui appele par GTK est arrive trop tot
glo->panneau.configure();
//glo->panneau.dump();

g_timeout_add( 31, (GSourceFunc)(idle_call), (gpointer)glo );

fflush(stdout);
gtk_main();


printf("closing\n");
return(0);
}
