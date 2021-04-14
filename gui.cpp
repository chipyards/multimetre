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
#include "cli_parse.h"
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
static const GdkColor verde = { 0, 0x8000, 0xFF00, 0x8000 };
static const GdkColor amare = { 0, 0xE000, 0xFF00, 0x0000 };
static const GdkColor laran = { 0, 0xFF00, 0xB000, 0x8000 };
static const GdkColor verme = { 0, 0xFF00, 0x3000, 0x3000 };

ticks++;

// acquisition de donnees en temps reel
if	( ( ticks % 3 ) == 0 )
	glo->step();

if	( glo->running )
	{
	// cadrages avec moderations
	if	( ( ticks % 6 ) == 0 )
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
		int layer_index = (glo->option_power)?(glo->recording_chan*2):(glo->recording_chan);
		layer_f_param * lelay2 = (layer_f_param *)glo->panneau2.bandes[0]->courbes[layer_index];
		lelay2->scan();
		// scan minimal sur le layer power, juste pour effacer le cadrage par defaut
		if	( ( glo->option_power ) && ( glo->wri < 3 ) )
			{
			layer_index = (glo->recording_chan*2) + 1;
			lelay2 = (layer_f_param *)glo->panneau2.bandes[0]->courbes[layer_index];
			lelay2->scan();
			}
		glo->panneau2.fullMN(); glo->panneau2.force_repaint = 1;
		}
	}

if	( ( ticks % 6 ) == 3 )
	{
	if	( glo->running )
		{
		int lag = glo->curlag;
		const GdkColor * cor;
		if	( lag < 5 )
			cor = &verde;
		else if	( lag < 10 )
			cor = &amare;
		else if	( lag < 20 )
			cor = &laran;
		else	cor = &verme;
		gtk_widget_modify_bg( glo->brun, GTK_STATE_NORMAL, cor );	
		gtk_widget_modify_bg( glo->brun, GTK_STATE_PRELIGHT, cor );	
		}
	else	gtk_widget_modify_bg( glo->brun, GTK_STATE_NORMAL, NULL );
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

// le spin button du recording channel
void xy_layer_call( GtkWidget *widget, glostru * glo )
{
unsigned int layer_index;
glo->recording_chan = gtk_spin_button_get_value_as_int( GTK_SPIN_BUTTON(widget) );
if	( glo->option_power )
	{
	layer_index = glo->recording_chan * 2;
	printf("recording XY layers set to %d and %d (power)\n", layer_index, layer_index+1 ); fflush(stdout);
	if	( (layer_index+1) < glo->panneau2.bandes[0]->courbes.size() )
		{
		glo->panneau2.set_layer_vis( 0, layer_index, 1 );
		glo->panneau2.set_layer_vis( 0, layer_index+1, 1 );
		}
	}
else	{
	layer_index = glo->recording_chan;
	printf("recording XY layer set to %d\n", layer_index ); fflush(stdout);
	if	( layer_index < glo->panneau2.bandes[0]->courbes.size() )
		glo->panneau2.set_layer_vis( 0, layer_index, 1 );
	}
glo->wri = 0;
}

// les codes 'g', 'u', 'v', 'c' pour piloter les AimTTi 1604

void send_g_call( GtkWidget *widget, glostru * glo )
{
char c = 'g';
printf("tx '%c'\n", c ); fflush(stdout);
nb_serial_write( &c, 1 );
}

void send_u_call( GtkWidget *widget, glostru * glo )
{
char c = 'u';
printf("tx '%c'\n", c ); fflush(stdout);
nb_serial_write( &c, 1 );
}

void send_v_call( GtkWidget *widget, glostru * glo )
{
char c = 'v';
printf("tx '%c'\n", c ); fflush(stdout);
nb_serial_write( &c, 1 );
}

void send_c_call( GtkWidget *widget, glostru * glo )
{
char c = 'c';
printf("tx '%c'\n", c ); fflush(stdout);
nb_serial_write( &c, 1 );
}


/** ============================ GLUPLOT call backs =============== */

void clic_call_back1( double M, double N, void * vglo )
{
printf("clic M N %g %g\n", M, N );
// glostru * glo = (glostru *)vglo;
}

void key_call_back_common( int v, void * vglo )
{
glostru * glo = (glostru *)vglo;
switch	( v )
	{
	// le dump, aussi utile pour faire un flush de stdout
	case 'd' :
		glo->panneau1.dump(); fflush(stdout);
		glo->panneau2.dump(); fflush(stdout);
		break;
	// les codes 'g', 'u', 'v', 'c' pour piloter les AimTTi 1604
	case 'g' :	// on/off
	case 'u' :	// connect
	case 'v' :	// disconnect
	case 'c' :	// auto range
		{
		char c = (char)v;
		printf("tx '%c'\n", c ); fflush(stdout);
		nb_serial_write( &c, 1 );
		}
		break;
	case ' ' :
		run_call( NULL, glo );
		break;
	}
}


void key_call_back1( int v, void * vglo )	// panneau1
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
	// experience sur le subticks
	case 't' :
		glo->panneau1.bandes[0]->subtk *= 2.0;
		glo->panneau1.force_repaint = 1;
		glo->panneau1.force_redraw = 1;		// necessaire pour panneau1 a cause de offscreen_flag
		break;
	// zoom en mode running
	case 'Z' :
		glo->Uspan *= 2;
		break;
	case 'z' :
		glo->Uspan *= 0.5;
		break;
	// PDF
	case 'p' :
		modpop_entry( "PDF plot", "---- nom du fichier ----", glo->plot_fnam, sizeof(glo->plot_fnam), GTK_WINDOW(glo->wmain) );
		modpop_entry( "PDF plot", "--------------------------- description --------------------------",
				glo->plot_desc, sizeof(glo->plot_desc), GTK_WINDOW(glo->wmain) );
		glo->panneau1.pdfplot( glo->plot_fnam, glo->plot_desc );
		break;
	default : key_call_back_common( v, vglo );
	}
}

void key_call_back2( int v, void * vglo )
{
glostru * glo = (glostru *)vglo;
switch	( v )
	{
	case GDK_KEY_KP_0 :
	case '0' : glo->panneau2.toggle_vis( 0, 0 ); break;
	case GDK_KEY_KP_1 :
	case '1' : glo->panneau2.toggle_vis( 0, 1 ); break;
	case GDK_KEY_KP_2 :
	case '2' : glo->panneau2.toggle_vis( 0, 2 ); break;
	case GDK_KEY_KP_3 :
	case '3' : glo->panneau2.toggle_vis( 0, 3 ); break;
	case GDK_KEY_KP_4 :
	case '4' : glo->panneau2.toggle_vis( 0, 4 ); break;
	case GDK_KEY_KP_5 :
	case '5' : glo->panneau2.toggle_vis( 0, 5 ); break;
	case GDK_KEY_KP_6 :
	case '6' : glo->panneau2.toggle_vis( 0, 6 ); break;
	case GDK_KEY_KP_7 :
	case '7' : glo->panneau2.toggle_vis( 0, 7 ); break;
	// changer le layer pour ecriture N.B. la callback du bouton va se charger de gerer le changement
	case '>' :
		if	( glo->recording_chan < ((glo->option_power)?3:7) )
			{ 
			gtk_spin_button_set_value( GTK_SPIN_BUTTON(glo->slay), glo->recording_chan + 1);
			} break;
	case '<' :
		if	( glo->recording_chan > 0 )
			{
			gtk_spin_button_set_value( GTK_SPIN_BUTTON(glo->slay), glo->recording_chan - 1);
			} break;
	// restart le layer selectionne en ecriture
	case 'R' : glo->clearXY(); break;
	// restart tous les layers
	case 'A' : glo->clearXYall(); break;
	// PDF
	case 'p' :
		modpop_entry( "PDF plot", "---- nom du fichier ----", glo->plot_fnam, sizeof(glo->plot_fnam), GTK_WINDOW(glo->wmain) );
		modpop_entry( "PDF plot", "--------------------------- description --------------------------",
				glo->plot_desc, sizeof(glo->plot_desc), GTK_WINDOW(glo->wmain) );
		glo->panneau2.pdfplot( glo->plot_fnam, glo->plot_desc );
		break;
	default : key_call_back_common( v, vglo );
	}
}

/** ============================ l'enrichissement de menu Y ======= */
// enrichissement du menu Y du strip 1 "XY"
// callbacks des boutons radio

static void change_kr( gpanel * panneau, double lekr )
{
strip * lestrip = panneau->bandes[panneau->selected_strip];
lestrip->kr = lekr;	// un zoom sur place est necessaire pour recalculer les ticks
lestrip->zoomY( 0.0, (double)lestrip->ndy );
panneau->force_repaint = 1;
}

static void Y_menu_x100_call( GtkWidget *widget,  gpanel * panneau  )
{
if	( gtk_check_menu_item_get_active( GTK_CHECK_MENU_ITEM(widget) ) )
	change_kr( panneau, 0.01 );
}
static void Y_menu_x10_call( GtkWidget *widget,  gpanel * panneau  )
{
if	( gtk_check_menu_item_get_active( GTK_CHECK_MENU_ITEM(widget) ) )
	change_kr( panneau, 0.1 );
}
static void Y_menu_x1_call( GtkWidget *widget,  gpanel * panneau  )
{
if	( gtk_check_menu_item_get_active( GTK_CHECK_MENU_ITEM(widget) ) )
	change_kr( panneau, 1.0 );
}
static void Y_menu_x01_call( GtkWidget *widget,  gpanel * panneau  )
{
if	( gtk_check_menu_item_get_active( GTK_CHECK_MENU_ITEM(widget) ) )
	change_kr( panneau, 10.0 );
}
static void Y_menu_x001_call( GtkWidget *widget,  gpanel * panneau  )
{
if	( gtk_check_menu_item_get_active( GTK_CHECK_MENU_ITEM(widget) ) )
	change_kr( panneau, 100.0 );
}

static void enrich_Y_menu( gpanel * panneau, int istrip  )
{
GtkWidget * curmenu;
GtkWidget * curitem;
GSList *group = NULL;

curmenu = ((gstrip *)panneau->bandes[istrip])->smenu_y;
if	( curmenu == NULL )
	return;

curitem = gtk_separator_menu_item_new();
gtk_menu_shell_append( GTK_MENU_SHELL( curmenu ), curitem );
gtk_widget_show ( curitem );

curitem = gtk_radio_menu_item_new_with_label( group, "X 100");
g_signal_connect( G_OBJECT( curitem ), "activate",
		  G_CALLBACK( Y_menu_x100_call ), (gpointer)panneau );
gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(curitem), FALSE );
gtk_menu_shell_append( GTK_MENU_SHELL( curmenu ), curitem );
gtk_widget_show ( curitem );

group = gtk_radio_menu_item_get_group( GTK_RADIO_MENU_ITEM(curitem) );
curitem = gtk_radio_menu_item_new_with_label( group, "X 10");
g_signal_connect( G_OBJECT( curitem ), "activate",
		  G_CALLBACK( Y_menu_x10_call ), (gpointer)panneau );
gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(curitem), FALSE );
gtk_menu_shell_append( GTK_MENU_SHELL( curmenu ), curitem );
gtk_widget_show ( curitem );

group = gtk_radio_menu_item_get_group( GTK_RADIO_MENU_ITEM(curitem) );
curitem = gtk_radio_menu_item_new_with_label( group, "X 1");
g_signal_connect( G_OBJECT( curitem ), "activate",
		  G_CALLBACK( Y_menu_x1_call ), (gpointer)panneau );
gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(curitem), TRUE );
gtk_menu_shell_append( GTK_MENU_SHELL( curmenu ), curitem );
gtk_widget_show ( curitem );

group = gtk_radio_menu_item_get_group( GTK_RADIO_MENU_ITEM(curitem) );
curitem = gtk_radio_menu_item_new_with_label( group, "X 0.1");
g_signal_connect( G_OBJECT( curitem ), "activate",
		  G_CALLBACK( Y_menu_x01_call ), (gpointer)panneau );
gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(curitem), FALSE );
gtk_menu_shell_append( GTK_MENU_SHELL( curmenu ), curitem );
gtk_widget_show ( curitem );

group = gtk_radio_menu_item_get_group( GTK_RADIO_MENU_ITEM(curitem) );
curitem = gtk_radio_menu_item_new_with_label( group, "X 0.01");
g_signal_connect( G_OBJECT( curitem ), "activate",
		  G_CALLBACK( Y_menu_x001_call ), (gpointer)panneau );
gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(curitem), FALSE );
gtk_menu_shell_append( GTK_MENU_SHELL( curmenu ), curitem );
gtk_widget_show ( curitem );
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
			{
			XYflag = 1 + c - 'X';
			if	( option_swap )
				XYflag ^= 3;	// permuter 1 et 2
			}
		// ici on a XYflag = 0 (invalid), 1 (X) ou 2 (Y)
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
			short int X_period, X_lag, Y_period, Y_lag;
			double val;
			char gui_msg[128];
			msgbuf[QMSG-1] = 0;
			if	( extract_timestamp( msgbuf, &time ) )
				{ printf("%c ]%s[ BAD TIMESTAMP\n", 'W' + XYflag, msgbuf ); fflush(stdout); }
			else if	( extract_val( msgbuf, &val ) )
				{ printf("%c ]%s[ BAD VAL\n", 'W' + XYflag, msgbuf ); fflush(stdout); }
			else	{
				if	( XYflag == 1 )
					{
					// calcul timing : "lag" = retard du message courant par rapport au precedent de l'autre instrument
					X_period = time - oldtimeX; X_lag = time - oldtimeY;
					oldtimeX = time; valX = val;
					// extraction d'un point
					if	( X_lag < MAX_LAG )
						{
						curlag = (int)X_lag;	// pour retour visuel
						set_point( (int)X_lag, valX, valY );
						}
					// affichage textuel dans le GUI
					snprintf( gui_msg, sizeof(gui_msg), "%d/%d   %g", X_lag, X_period, val );
					X_status_show( gui_msg );
					}
				else	{
					// calcul timing
					Y_period = time - oldtimeY; Y_lag = time - oldtimeX;
					oldtimeY = time; valY = val;
					// extraction d'un point
					if	( Y_lag < MAX_LAG )
						{
						curlag = (int)Y_lag;	// pour retour visuel
						set_point( (int)Y_lag, valX, valY );
						}
					// affichage textuel dans le GUI
					snprintf( gui_msg, sizeof(gui_msg), "%d/%d   %g", Y_lag, Y_period, val );
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
int layer_index;
layer_f_param * lelay;
if	( option_power )
	{
	layer_index = recording_chan * 2;
	lelay = (layer_f_param *)panneau2.bandes[0]->courbes[layer_index];
	lelay->qu = 0;
	lelay = (layer_f_param *)panneau2.bandes[0]->courbes[layer_index+1];
	lelay->qu = 0;
	}
else	{
	layer_index = recording_chan;
	lelay = (layer_f_param *)panneau2.bandes[0]->courbes[layer_index];
	lelay->qu = 0;
	}
wri = 0;
}

void glostru::clearXYall()
{
int layer_index;
layer_f_param * lelay;
for	( layer_index = 0; layer_index < 8; layer_index++ )
	{
	lelay = (layer_f_param *)panneau2.bandes[0]->courbes[layer_index];
	lelay->qu = 0;
	gtk_spin_button_set_value( GTK_SPIN_BUTTON(slay), 0.0 ); 
	panneau2.set_layer_vis( 0, layer_index,
		( ( layer_index == 0 ) || ( ( layer_index == 1 ) && ( option_power ) ) )?1:0 );
	}
recording_chan = 0;
wri = 0;
}


void glostru::set_point( int abs_lag, float Xv, float Yv )
{
printf("wri %4d : lag %4d : %8.3f %8.3f\n", wri, abs_lag, Xv, Yv ); fflush(stdout);
if	( !running )
	return;
layer_f_fifo  * lelay0 = (layer_f_fifo  *)panneau1.bandes[0]->courbes[0];
layer_f_fifo  * lelay1 = (layer_f_fifo  *)panneau1.bandes[0]->courbes[1];

// action sur le graphe fifo X(t) Y(t)
lelay0->push(Xv);
lelay1->push(Yv);

// action sur le graphe Y(X)
int layer_index;
layer_f_param * lelay;

if	( option_power )
	{
	layer_index = recording_chan * 2;
	Xbuf[layer_index][wri&(QBUF-1)] = Xv;
	Ybuf[layer_index][wri&(QBUF-1)] = Yv;
	Xbuf[layer_index+1][wri&(QBUF-1)] = Xv;
	Ybuf[layer_index+1][wri&(QBUF-1)] = Xv * Yv * k_power;
	wri++;
	if	( wri <= QBUF )
		{
		lelay = (layer_f_param *)panneau2.bandes[0]->courbes[layer_index];
		lelay->qu = wri;
		lelay = (layer_f_param *)panneau2.bandes[0]->courbes[layer_index+1];
		lelay->qu = wri;
		}
	}
else	{
	layer_index = recording_chan;
	Xbuf[layer_index][wri&(QBUF-1)] = Xv;
	Ybuf[layer_index][wri&(QBUF-1)] = Yv;
	wri++;
	if	( wri <= QBUF )
		{
		lelay = (layer_f_param *)panneau2.bandes[0]->courbes[layer_index];
		lelay->qu = wri;
		}
	}
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
curbande->add_layer( curcour, "ValX" );

// configurer le layer
curcour->set_km( 1.0 );
curcour->set_m0( 0.0 );
curcour->set_kn( 1.0 );
curcour->set_n0( 0.0 );
curcour->fgcolor.set( 0.75, 0.0, 0.0 );

// creer un layer
curcour = new layer_f_fifo(12);
curbande->add_layer( curcour, "ValY" );

// configurer le layer
curcour->set_km( 1.0 );
curcour->set_m0( 0.0 );
curcour->set_kn( 1.0 );
curcour->set_n0( 0.0 );
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
curbande->bgcolor.set( 1.0, 1.0, 1.0 );
curbande->Ylabel = "XY";
curbande->optX = 1;
curbande->subtk = 10;
enrich_Y_menu( &panneau2, 0 );

// creer 8 layers
layer_f_param * curcour2;
char lab[32];
for	( int i = 0; i < 8; i++ )
	{
	// creer un layer
	curcour2 = new layer_f_param;
	if	( option_power )
		{
		if	( i & 1 )
			snprintf( lab, sizeof(lab), "Pwr #%d", i/2 );
		else	snprintf( lab, sizeof(lab), "Y(X) #%d", i/2 );
		}
	else	{
		snprintf( lab, sizeof(lab), "Y(X) #%d", i );
		}
	curbande->add_layer( curcour2, lab );
	if	( option_power )
		{
		if	(i/2)
			panneau2.set_layer_vis( 0, i, 0 );
		}
	else	{
		if	(i)
			panneau2.set_layer_vis( 0, i, 0 );
		}

	// configurer le layer
	curcour2->set_km( 1.0 );
	curcour2->set_m0( 0.0 );
	curcour2->set_kn( 1.0 );
	curcour2->set_n0( 0.0 );
	curcour2->fgcolor.arc_en_ciel( i % 8 );

	// connexion layout - data
	curcour2 = (layer_f_param *)panneau2.bandes[0]->courbes[i];
	curcour2->U = Xbuf[i];
	curcour2->V = Ybuf[i];
	curcour2->qu = 0;
	}

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
gtk_widget_set_size_request( curwidg, 800, 620 );
glo->vpans = curwidg;

// creer boite verticale pour panel et sa zoombar
curwidg = gtk_vbox_new( FALSE, 0 ); /* spacing ENTRE objets */
gtk_paned_pack1( GTK_PANED(glo->vpans), curwidg, TRUE, FALSE ); // resizable, not shrinkable
glo->vpan1 = curwidg;


/* creer une drawing area resizable depuis la fenetre */
curwidg = gtk_drawing_area_new();
gtk_widget_set_size_request( curwidg, 800, 80 );	// hauteur mini, la hauteur initiale fixee par parent
glo->panneau1.events_connect( GTK_DRAWING_AREA( curwidg ) );
gtk_box_pack_start( GTK_BOX( glo->vpan1 ), curwidg, TRUE, TRUE, 0 );
glo->darea1 = curwidg;

/* creer une drawing area  qui ne sera pas resizee en hauteur par la hbox
   mais quand meme en largeur (par chance !!!) */
curwidg = gtk_drawing_area_new();
glo->zbar.events_connect( GTK_DRAWING_AREA( curwidg ) );
gtk_box_pack_start( GTK_BOX( glo->vpan1 ), curwidg, FALSE, FALSE, 0 );
glo->zarea1 = curwidg;

/* creer une drawing area resizable depuis la fenetre */
curwidg = gtk_drawing_area_new();
gtk_widget_set_size_request( curwidg, 800, 80 );	// hauteur mini, la hauteur initiale fixee par parent
glo->panneau2.events_connect( GTK_DRAWING_AREA( curwidg ) );
gtk_paned_pack2( GTK_PANED(glo->vpans), curwidg, TRUE, FALSE ); // resizable, not shrinkable
glo->darea2 = curwidg;

/* creer boite horizontale */
curwidg = gtk_hbox_new( FALSE, 10 ); /* spacing ENTRE objets */
gtk_container_set_border_width( GTK_CONTAINER (curwidg), 5);
gtk_box_pack_start( GTK_BOX( glo->vmain ), curwidg, FALSE, FALSE, 0 );
glo->hbut = curwidg;

/* simple bouton */
curwidg = gtk_button_new_with_label ("Run/Pause");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( run_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, TRUE, TRUE, 0 );
glo->brun = curwidg;

// spin button pour choisir layer XY a enregistrer
if	( glo->option_power )
	curwidg = gtk_spin_button_new_with_range( 0.0, 3.0, 1.0 );
else	curwidg = gtk_spin_button_new_with_range( 0.0, 7.0, 1.0 );
g_signal_connect( curwidg, "value-changed",
		  G_CALLBACK( xy_layer_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, FALSE, FALSE, 0 );
glo->slay = curwidg;

/* simple bouton */
curwidg = gtk_button_new_with_label ("Restart XY");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( clear_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, TRUE, TRUE, 0 );

// entree non editable
curwidg = gtk_entry_new();
gtk_widget_set_usize( curwidg, 150, 0 );
gtk_entry_set_editable( GTK_ENTRY(curwidg), FALSE );
gtk_entry_set_text( GTK_ENTRY(curwidg), "X" );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, FALSE, FALSE, 0 );
glo->erpX = curwidg;

// entree non editable
curwidg = gtk_entry_new();
gtk_widget_set_usize( curwidg, 150, 0 );
gtk_entry_set_editable( GTK_ENTRY(curwidg), FALSE );
gtk_entry_set_text( GTK_ENTRY(curwidg), "Y" );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, FALSE, FALSE, 0 );
glo->erpY = curwidg;

/* simple bouton */
curwidg = gtk_button_new_with_label ("On/Off (g)");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( send_g_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, TRUE, TRUE, 0 );

/* simple bouton */
curwidg = gtk_button_new_with_label ("Connect(u)");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( send_u_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, TRUE, TRUE, 0 );

/* simple bouton */
curwidg = gtk_button_new_with_label ("Disconn(v)");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( send_v_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, TRUE, TRUE, 0 );

/* simple bouton */
curwidg = gtk_button_new_with_label ("Auto(c)");
gtk_signal_connect( GTK_OBJECT(curwidg), "clicked",
                    GTK_SIGNAL_FUNC( send_c_call ), (gpointer)glo );
gtk_box_pack_start( GTK_BOX( glo->hbut ), curwidg, TRUE, TRUE, 0 );

// connecter la zoombar au panel et inversement
glo->panneau1.zoombar = &glo->zbar;
glo->panneau1.zbarcall = gzoombar_zoom;
glo->zbar.panneau = &glo->panneau1;

gtk_widget_show_all( glo->wmain );


// 	parsage CLI
cli_parse * lepar = new cli_parse( argc, (const char **)argv, "pw" );
// le parsage est fait, on recupere les args !
const char * val;
if	( ( val = lepar->get( 'p' ) ) )	glo->COMport = atoi( val );
if	( ( val = lepar->get( 'w' ) ) )
	{
	glo->option_power = 1;
	glo->k_power = strtod( val, NULL );
	if	( glo->k_power == 0.0 )
		glo->k_power = 1.0;
	printf("option power : k = %g\n", glo->k_power ); fflush(stdout);
	}
if	( lepar->get( 's' ) )
	{
	glo->option_swap = 1;	
	printf("option swap : permutation of X & Y\n" ); fflush(stdout);
	}
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

#define MY_BAUD_RATE 9600
glo->COMport = nb_open_serial_ro( glo->COMport, MY_BAUD_RATE );
if	( glo->COMport >=1 )
	{
	printf("ouverture COM%d a %d bauds Ok\n", glo->COMport, MY_BAUD_RATE );
	}
else	{
	printf("failed open port COM, code %d\n", glo->COMport );
	GdkColor rouge = { 0, 0xFF00, 0x0000, 0x0000 };
	gtk_widget_modify_base( glo->erpX, GTK_STATE_NORMAL, &rouge );
	gtk_widget_modify_base( glo->erpY, GTK_STATE_NORMAL, &rouge );
	gtk_entry_set_text( GTK_ENTRY(theglo.erpX), "no COM port" );
	gtk_entry_set_text( GTK_ENTRY(theglo.erpY), "please quit" );
	}

fflush(stdout);
	g_timeout_add( 31, (GSourceFunc)(idle_call), (gpointer)glo );

gtk_main();


printf("closing\n");
return(0);
}

