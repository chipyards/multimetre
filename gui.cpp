#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <cairo-pdf.h>
#include <locale.h>
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
#include "gluplot.h"

#include "layer_f_fifo.h"
#include "layer_f_param.h"

#include "modpop3.h"
#include "gui.h"
#include "nb_serial.h"

// unique variable globale exportee pour gasp() de modpop3
GtkWindow * global_main_window = NULL;

// le contexte de l'appli
static glostru theglo;

/** ============================ GTK call backs ======================= */
int idle_call( glostru * glo )
{
static unsigned int ticks = 0;
ticks++;

// acquisition de donnees en temps reel
if	( ( ticks % 3 ) == 0 )
	glo->step();

if	( glo->running )
	{
	// cadrages avec moderations
	if	( ( ticks % 7 ) == 0 )
		{		// strip du fifo en scroll continu
		layer_f_fifo * lelay0 = (layer_f_fifo *)glo->panneau1.bandes[0]->courbes[0];
		layer_f_fifo * lelay1 = (layer_f_fifo *)glo->panneau1.bandes[0]->courbes[1];
		// cadrage vertical
		lelay0->scan();
		lelay1->scan();
		glo->panneau1.bandes[0]->fullN();
		// cadrage horizontal, fenetre de largeur constante Uspan
		double U0, U1;
		U1 = (double)lelay0->Ue + 0.05 * glo->Uspan;
		U0 = U1 - glo->Uspan;
		glo->panneau1.zoomM( U0, U1 );
		glo->panneau1.force_repaint = 1;
				// strip XY
		layer_f_param * lelay2 = (layer_f_param *)glo->panneau2.bandes[0]->courbes[0];
		lelay2->scan();
		glo->panneau2.fullMN(); glo->panneau2.force_repaint = 1;
		}
	}

// moderateur de drawing
if	( glo->panneau1.force_repaint )
	glo->panneau1.paint();
	
if	( glo->panneau2.force_repaint )
	glo->panneau2.paint();

return( -1 );
}

gint close_event_call( GtkWidget *widget, GdkEvent *event, gpointer data )
{
gtk_main_quit();
return (TRUE);		// ne pas destroyer tout de suite
}

void quit_call( GtkWidget *widget, glostru * glo )
{
gtk_main_quit();
}

// callbacks de widgets
void run_call( GtkWidget *widget, glostru * glo )
{
glo->running ^= 1;
if	( glo->running == 0 )
	{
	glo->panneau1.fullM();	// pour que la zoombar soit utilisable
	glo->panneau1.force_repaint = 1;
	glo->panneau1.force_redraw = 1;		// necessaire pour panneau1 a cause de offscreen_flag
	}
}

void clear_call( GtkWidget *widget, glostru * glo )
{
glo->clearXY();
}

/** ============================ GLUPLOT call backs =============== */

void clic_call_back1( double M, double N, void * vglo )
{
printf("clic M N %g %g\n", M, N );
// glostru * glo = (glostru *)vglo;
}

void key_call_back1( int v, void * vglo )
{
glostru * glo = (glostru *)vglo;
switch	( v )
	{
	// la visibilite
	case GDK_KEY_KP_0 :
	case '0' : glo->panneau1.toggle_vis( 0, 0 ); break;
	case GDK_KEY_KP_1 :
	case '1' : glo->panneau1.toggle_vis( 0, 1 ); break;
	// l'option offscreen drawpad
	case 'o' : glo->panneau1.offscreen_flag = 1; break;
	case 'n' : glo->panneau1.offscreen_flag = 0; break;
	// le dump, aussi utile pour faire un flush de stdout
	case 'd' :
		glo->panneau1.dump(); fflush(stdout);
		break;
	//
	case 't' :
		glo->panneau1.bandes[0]->subtk *= 2.0;
		glo->panneau1.force_repaint = 1;
		glo->panneau1.force_redraw = 1;		// necessaire pour panneau1 a cause de offscreen_flag
		break;
	//
	case 'Z' :
		glo->Uspan *= 2;
		break;
	case 'z' :
		glo->Uspan *= 0.5;
		break;
	//
	case 'p' :
		char fnam[32], capt[128];
		snprintf( fnam, sizeof(fnam), "demo2.1.pdf" );
		modpop_entry( "PDF plot", "nom du fichier", fnam, sizeof(fnam), GTK_WINDOW(glo->wmain) );
		snprintf( capt, sizeof(capt), "plot FIFO" );
		modpop_entry( "PDF plot", "description", capt, sizeof(capt), GTK_WINDOW(glo->wmain) );
		glo->panneau1.pdfplot( fnam, capt );
		break;
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

void key_call_back2( int v, void * vglo )
{
glostru * glo = (glostru *)vglo;
switch	( v )
	{
	// le dump, aussi utile pour faire un flush de stdout
	case 'd' :
		glo->panneau2.dump(); fflush(stdout);
		break;
	//
	case 'p' :
		char fnam[32], capt[128];
		snprintf( fnam, sizeof(fnam), "demo2.2.pdf" );
		modpop_entry( "PDF plot", "nom du fichier", fnam, sizeof(fnam), GTK_WINDOW(glo->wmain) );
		snprintf( capt, sizeof(capt), "plot XY" );
		modpop_entry( "PDF plot", "description", capt, sizeof(capt), GTK_WINDOW(glo->wmain) );
		glo->panneau2.pdfplot( fnam, capt );
		break;
	}
}

/** ============================ l'application ==================== */

static void X_status_show( char * msg )
{
gtk_entry_set_text( GTK_ENTRY(theglo.erpX), msg );
}

static void Y_status_show( char * msg )
{
gtk_entry_set_text( GTK_ENTRY(theglo.erpY), msg );
}


// message typique de la passerelle STM32F1 (16 bytes)
// 0123456789abcdef
// X2f3b kS 0.4218
// X			X ou Y : id du multimetre
//  2f2b		hex timestamp
//	 k		mode : V, v, A, a, k, R
//	  S		S = autoScale
//         -		signe ou ~
//          0.4218	valeur
//                LF	LineFeed

// extraction du timestamp (rend 0 si Ok)
static int extract_timestamp( const char * msg, unsigned short * time )
{
char * tail;
*time = (unsigned short)strtol( msg+1, &tail, 16 );
if	( tail != ( msg + 5 ) )
	return 1;
if	( *tail != ' ' )
	return 2;
return 0;
}

// extraction de la mesure (rend 0 si Ok)
// Note : pour que cela marche il faut : setlocale( LC_ALL, "C" ); // kill the frog, AFTER gtk_init
static int extract_val( const char * msg, double * val )
{
char * tail;
if	( msg[8] == '-' )
	*val = strtod( msg+8, &tail ); 
else	*val = strtod( msg+9, &tail ); 
if	( tail != ( msg + 15 ) )
	return 1;
if	( *tail != 0 )	// Note : le LF a ete remplace par 0 dans step()
	return 2;
return 0;
}

// la partie du process en relation avec jluplot

// fontion appelee periodiquement
int glostru::step()
{
int i; unsigned int cnt;
rxcnt = nb_serial_read( rxbuf, sizeof(rxbuf)-1 );
// copier les nouvelles donnees dans le FIFO
if	( rxcnt <= 0 )
	return 0;

for	( i = 0; i < rxcnt; ++i )
	{
	//printf("%c", rxbuf[i] );	// debug byte par byte
	fifobuf[ (fifoWI++) & FIFOMASK ] = rxbuf[i];
	}
// printf("#");

// sonder le fifo
cnt = fifoWI - fifoRI;

// printf("{%d %d}", rxcnt, cnt );

while	( cnt >= 16 )
	{
	unsigned int i, j = 0, c, XYflag = 0;
	for	( i = 0; i < cnt; )
		{
		c = fifobuf[ (fifoRI+(i++)) & FIFOMASK ];
		if	( ( c == 'X' ) || ( c == 'Y' ) )
			XYflag = 1 + c - 'X';
		if	( XYflag )
			{
			msgbuf[j++] = c;
			if	( ( c == 10 ) || ( j >= QMSG ) )
				break;
			}
		}
	// printf("->c=%d, i=%d, j=%d\n", c, i, j );
	// on va laisser dans le fifo un message qui serait trop court mais sans le LF,
	// car il pourra etre fini au prochain tour
	if	( ( j < QMSG ) && ( c != 10 ) )
		break;	// sortir du while
	// dans tous les autres cas le message est retire (marque comme lu) et eventuellement accepte
	else	{
		fifoRI += i;		// le message est marque lu
		cnt = fifoWI - fifoRI;
		if	( j == QMSG )	// si en plus il a pile la bonne longueur, on l'accepte
			{
			unsigned short time;
			short unsigned int X_period, X_lag, Y_period, Y_lag;
			double val;
			char gui_msg[128];
			msgbuf[QMSG-1] = 0;
			if	( extract_timestamp( msgbuf, &time ) )
				{ printf("]%s[ BAD TIMESTAMP\n", msgbuf ); fflush(stdout); }
			else if	( extract_val( msgbuf, &val ) )
				{ printf("]%s[ BAD VAL\n", msgbuf ); fflush(stdout); }
			else	{
				if	( XYflag == 1 )
					{
					// calcul timing : "lag" = retard du message courant par rapport au precedent de l'autre instrument
					X_period = time - oldtimeX; X_lag = time - oldtimeY;
					oldtimeX = time; valX = val;
					// extraction d'un point
					if	( X_lag < MAX_LAG )
						{
						set_point( (int)X_lag, valX, valY );
						}
					// affichage textuel dans le GUI
					snprintf( gui_msg, sizeof(gui_msg), "%5u %5u  %g", X_period, X_lag, val );
					X_status_show( gui_msg );
					}
				else	{
					// calcul timing
					Y_period = time - oldtimeY; Y_lag = time - oldtimeX;
					oldtimeY = time; valY = val;
					// extraction d'un point
					if	( Y_lag < MAX_LAG )
						{
						set_point( (int)Y_lag, valX, valY );
						}
					// affichage textuel dans le GUI
					snprintf( gui_msg, sizeof(gui_msg), "%5u %5u  %g", Y_period, Y_lag, val );
					Y_status_show( gui_msg );
					}

				// affichage textuel dans la console
				//if	( abs_lag >= 0 )
				//	{ printf("]%s[ %d\n", msgbuf, abs_lag ); fflush(stdout); }
				//else	{ printf("]%s[\n",    msgbuf ); fflush(stdout); }
				}
			}
		}
	}
return 0;
}

// la partie du process en relation avec jluplot

void glostru::clearXY()
{
layer_f_param * lelay2 = (layer_f_param *)panneau2.bandes[0]->courbes[0];
wri = 0;
lelay2->qu = 0;
}

void glostru::set_point( int abs_lag, float Xv, float Yv )
{
printf("wri %4d : lag %4d : %8.3f %8.3f\n", wri, abs_lag, Xv, Yv ); fflush(stdout);
if	( !running )
	return;

layer_f_fifo  * lelay0 = (layer_f_fifo  *)panneau1.bandes[0]->courbes[0];
layer_f_fifo  * lelay1 = (layer_f_fifo  *)panneau1.bandes[0]->courbes[1];
layer_f_param * lelay2 = (layer_f_param *)panneau2.bandes[0]->courbes[0];

// action sur le graphe fifo X(t) Y(t)
lelay0->push(Xv);
lelay1->push(Yv);

// action sur le graphe Y(X)
Xbuf[wri&(QBUF-1)] = Xv;
Ybuf[wri&(QBUF-1)] = Yv; 
wri++;
if	( wri <= QBUF )
	lelay2->qu = wri;
}

void glostru::layout()
{

// layout jluplot

// marge pour les textes
// panneau1.mx = 60;
// panneau1.offscreen_flag = 0;

// creer le strip pour les waves
gstrip * curbande;
curbande = new gstrip;
panneau1.add_strip( curbande );

// configurer le strip
curbande->bgcolor.set( 0.90, 0.95, 1.0 );
curbande->Ylabel = "val";
curbande->optX = 1;
curbande->subtk = 1;

// creer un layer
layer_f_fifo * curcour;
curcour = new layer_f_fifo(12);
curbande->add_layer( curcour );

// configurer le layer
curcour->set_km( 1.0 );
curcour->set_m0( 0.0 );
curcour->set_kn( 1.0 );
curcour->set_n0( 0.0 );
curcour->label = string("ValX");
curcour->fgcolor.set( 0.75, 0.0, 0.0 );

// creer un layer
curcour = new layer_f_fifo(12);
curbande->add_layer( curcour );

// configurer le layer
curcour->set_km( 1.0 );
curcour->set_m0( 0.0 );
curcour->set_kn( 1.0 );
curcour->set_n0( 0.0 );
curcour->label = string("ValY");
curcour->fgcolor.set( 0.0, 0.0, 0.8 );

// connexion layout - data
// sans objet pour layer_f_fifo : il possede les data

// layout jluplot pour panneau2
panneau2.offscreen_flag = 0;

// creer le strip
curbande = new gstrip;
panneau2.add_strip( curbande );

panneau2.pdf_DPI = 100;	// defaut est 72

// configurer le strip
curbande->bgcolor.set( 1.0, 0.95, 0.85 );
curbande->Ylabel = "XY";
curbande->optX = 1;
curbande->subtk = 10;

// creer un layer
layer_f_param * curcour2;
curcour2 = new layer_f_param;
curbande->add_layer( curcour2 );

// configurer le layer
curcour2->set_km( 1.0 );
curcour2->set_m0( 0.0 );
curcour2->set_kn( 1.0 );
curcour2->set_n0( 0.0 );
curcour2->label = string("Lissajoux");
curcour2->fgcolor.set( 0.75, 0.0, 0.0 );

// connexion layout - data
curcour2 = (layer_f_param *)panneau2.bandes[0]->courbes[0];
curcour2->U = Xbuf;
curcour2->V = Ybuf;
curcour2->qu = 0;

}


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

gtk_window_set_title( GTK_WINDOW (curwidg), "JAW");
gtk_container_set_border_width( GTK_CONTAINER( curwidg ), 10 );
glo->wmain = curwidg;
global_main_window = (GtkWindow *)curwidg;

// creer boite verticale pour : en haut plot panels, en bas boutons et entries
curwidg = gtk_vbox_new( FALSE, 5 ); /* spacing ENTRE objets */
gtk_container_add( GTK_CONTAINER( glo->wmain ), curwidg );
glo->vmain = curwidg;

// paire verticale "paned" pour : en haut panel X(t) Y(t) avec zoombar, en bas panel Y(X) 
curwidg = gtk_vpaned_new ();
gtk_box_pack_start( GTK_BOX( glo->vmain ), curwidg, TRUE, TRUE, 0 );
// gtk_container_set_border_width( GTK_CONTAINER( curwidg ), 5 );	// le tour exterieur
gtk_widget_set_size_request( curwidg, 800, 700 );
glo->vpans = curwidg;

// creer boite verticale pour panel et sa zoombar
curwidg = gtk_vbox_new( FALSE, 0 ); /* spacing ENTRE objets */
gtk_paned_pack1( GTK_PANED(glo->vpans), curwidg, TRUE, FALSE ); // resizable, not shrinkable
glo->vpan1 = curwidg;

/* creer une drawing area resizable depuis la fenetre */
curwidg = glo->panneau1.layout( 800, 80 );	// hauteur mini, la hauteur initiale fixee par parent
gtk_box_pack_start( GTK_BOX( glo->vpan1 ), curwidg, TRUE, TRUE, 0 );
glo->darea1 = curwidg;

/* creer une drawing area  qui ne sera pas resizee en hauteur par la hbox
   mais quand meme en largeur (par chance !!!) */
curwidg = glo->zbar.layout( 800 );
gtk_box_pack_start( GTK_BOX( glo->vpan1 ), curwidg, FALSE, FALSE, 0 );
glo->zarea1 = curwidg;

/* creer une drawing area resizable depuis la fenetre */
curwidg = glo->panneau2.layout( 800, 80 );	// hauteur mini, la hauteur initiale fixee par parent
gtk_paned_pack2( GTK_PANED(glo->vpans), curwidg, TRUE, FALSE ); // resizable, not shrinkable
glo->darea2 = curwidg;

/* creer boite horizontale */
curwidg = gtk_hbox_new( FALSE, 10 ); /* spacing ENTRE objets */
gtk_container_set_border_width( GTK_CONTAINER (curwidg), 5);
gtk_box_pack_start( GTK_BOX( glo->vmain ), curwidg, FALSE, FALSE, 0 );
glo->hbut = curwidg;

/* simple bouton */
curwidg = gtk_button_new_with_label (" Run/Pause ");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( run_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, TRUE, TRUE, 0 );
glo->brun = curwidg;

/* simple bouton */
curwidg = gtk_button_new_with_label (" Restart ");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( clear_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, TRUE, TRUE, 0 );
glo->braz = curwidg;

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

// connecter la zoombar au panel et inversement
glo->panneau1.zoombar = &glo->zbar;
glo->panneau1.zbarcall = gzoombar_zoom;
glo->zbar.panneau = &glo->panneau1;

gtk_widget_show_all( glo->wmain );

int portnum = 0;
if	( argc >= 2 )
	portnum = atoi(argv[1] );

#define MY_BAUD_RATE 9600
portnum = nb_open_serial_ro( portnum, MY_BAUD_RATE );
if	( portnum >=1 )
	{ printf("ouverture COM%d a %d bauds Ok\n", portnum, MY_BAUD_RATE ); fflush(stdout);  }
else	gasp("failed open port COM, code %d", portnum );

glo->panneau1.clic_callback_register( clic_call_back1, (void *)glo );
glo->panneau1.key_callback_register( key_call_back1, (void *)glo );
glo->panneau2.key_callback_register( key_call_back2, (void *)glo );

glo->layout();

// forcer un full initial pour que tous les coeffs de transformations soient a jour
glo->panneau1.full_valid = 0;
glo->panneau2.full_valid = 0;
// refaire un configure car celui appele par GTK est arrive trop tot
glo->panneau1.configure();
glo->zbar.configure();
glo->panneau2.configure();

g_timeout_add( 31, (GSourceFunc)(idle_call), (gpointer)glo );

fflush(stdout);

gtk_main();


printf("closing\n");
return(0);
}

