
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

// fontion appelee periodiquement
int process::step()
{
int i; unsigned int cnt;
rxcnt = nb_serial_read( (char *)rxbuf, sizeof(rxbuf)-1 );
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
	unsigned int i, j = 0, c, flag = 0;
	for	( i = 0; i < cnt; )
		{
		c = fifobuf[ (fifoRI+(i++)) & FIFOMASK ];
		if	( ( c == 'X' ) || ( c == 'Y' ) )
			flag = 1;
		if	( flag )
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
			msgbuf[QMSG-1] = 0;
			// affichage textuel dans le GUI
			serial_msg_call( (char *)msgbuf );
			// affichage textuel dans la console
			printf("%s\n", msgbuf); fflush(stdout);
			}
		}
	}
return 0;
}


// la partie du process en relation avec jluplot

// layout pour les waveforms - doit etre fait apres lecture wav data et calcul spectres
void process::prep_layout( gpanel * panneau )
{
strip * curbande;
layer_f_circ * curcour;
panneau->offscreen_flag = 1;	// 1 par defaut
// marge pour les textes
panneau->mx = 60;
panneau->kq = 200.0;	// echelles en secondes, 200 ech/s
// creer le strip
curbande = new strip;	// strip avec drawpad
panneau->bandes.push_back( curbande );
// creer le layer
curcour = new layer_f_circ;	// wave a pas uniforme
curbande->courbes.push_back( curcour );
// parentizer a cause des fonctions "set"
panneau->parentize();

// configurer le strip
curbande->bgcolor.dR = 1.0;
curbande->bgcolor.dG = 0.9;
curbande->bgcolor.dB = 0.8;
curbande->Ylabel = "log";
curbande->optX = 1;
curbande->optretX = 1;
curbande->kr = (4096*4)/3.315;
// configurer le layer
curcour->set_km( 1.0 );
curcour->set_m0( 0.0 );
curcour->set_kn( 1.0 );
curcour->set_n0( 0.0 );
curcour->label = "log";
curcour->fgcolor.dR = 0.0;
curcour->fgcolor.dG = 0.0;
curcour->fgcolor.dB = 0.75;
}

// connexion du layout aux data
void process::connect_layout( gpanel * panneau )
{
// pointeurs locaux sur les layers
layer_f_circ * layLOG = NULL;
// connecter les layers de ce layout sur les buffers existants
layLOG = (layer_f_circ *)panneau->bandes[0]->courbes[0];
layLOG->V = Lbuf;
layLOG->qu = QBUF;
// layLOG->scan();
layLOG->Vmin = 0.0;
layLOG->Vmax = 16384.0;	// data 14 bits
printf("end connect layout, %d strips\n\n", panneau->bandes.size() ); fflush(stdout);
}

