
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
#include "nb_serial.h"

#include "process.h"

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

// fontion appelee periodiquement
int process::step()
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
					if	( X_status_call )
						X_status_call( gui_msg );
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
					if	( Y_status_call )
						Y_status_call( gui_msg );
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

void process::set_point( int abs_lag, float Xv, float Yv )
{
printf("wri %4d : lag %4d : %8.3f %8.3f\n", wri, abs_lag, Xv, Yv ); fflush(stdout);
if	( ( run ) && ( *run ) )
	{
	Xbuf[wri&BUFMASK] = Xv;
	Ybuf[wri&BUFMASK] = Yv;
	wri++;
	}
}

void process::prep_layout( gpanel * panneau )
{
strip * curbande;
layer_f_circ * curcour;
panneau->offscreen_flag = 1;	// 1 par defaut
// marge pour les textes
panneau->mx = 60;
panneau->kq = 2.5;	// echelles en secondes, Te = 400ms
panneau->kq = 1.0;	// echelles en samples
// creer le strip
curbande = new strip;	// strip avec drawpad
panneau->bandes.push_back( curbande );
// configurer le strip
curbande->bgcolor.dR = 1.0;
curbande->bgcolor.dG = 0.9;
curbande->bgcolor.dB = 0.8;
curbande->Ylabel = "Mes";
curbande->optX = 1;
curbande->optretX = 1;
curbande->kr = 1.0;
// creer un layer float circulaire (oscilloscope)
curcour = new layer_f_circ;	// wave a pas uniforme
curbande->courbes.push_back( curcour );
// parentizer a cause des fonctions "set"
panneau->parentize();
// configurer le layer (style=0 par defaut)
curcour->set_km( 1.0 );
curcour->set_m0( 0.0 );
curcour->set_kn( 1.0 );
curcour->set_n0( 0.0 );
curcour->label = "Xval";
curcour->fgcolor.dR = 0.0;
curcour->fgcolor.dG = 0.0;
curcour->fgcolor.dB = 0.75;
// second layer sur le meme strip
curcour = new layer_f_circ;	// wave a pas uniforme
curbande->courbes.push_back( curcour );
// parentizer a cause des fonctions "set"
panneau->parentize();
// configurer le layer
curcour->set_km( 1.0 );
curcour->set_m0( 0.0 );
curcour->set_kn( 1.0 );
curcour->set_n0( 0.0 );
curcour->label = "Yval";
curcour->fgcolor.dR = 0.0;
curcour->fgcolor.dG = 0.6;
curcour->fgcolor.dB = 0.0;
}

// connexion du layout aux data
void process::connect_layout( gpanel * panneau )
{
// pointeurs locaux sur les 2 layers d'oscilloscope
layer_f_circ * lay_oscX = NULL;
layer_f_circ * lay_oscY = NULL;
// connecter les layers de ce layout sur les buffers existants
lay_oscX = (layer_f_circ *)panneau->bandes[0]->courbes[0];
lay_oscY = (layer_f_circ *)panneau->bandes[0]->courbes[1];
lay_oscX->V = Xbuf;
lay_oscY->V = Ybuf;
lay_oscX->qu = QBUF;
lay_oscY->qu = QBUF;
// lay_oscX->scan();
lay_oscX->Vmin = -10.0;
lay_oscX->Vmax = 10.0;	// test prelim @ 10V
printf("end connect layout, %d strips\n\n", panneau->bandes.size() ); fflush(stdout);
}

