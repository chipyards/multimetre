#define QFIFO		(1<<10)
#define FIFOMASK	(QFIFO-1)

#define QBUF 		(1<<12)
#define BUFMASK 	(QBUF-1)

#define QMSG		16

#define MAX_LAG		38
class process {
public:

// buffer pour lecture port serie
char rxbuf[64];
int rxcnt;

// fifo pour port serie
char fifobuf[QFIFO];
unsigned int fifoWI;
unsigned int fifoRI;

// buffer pour message extrait du fifo
char msgbuf[QMSG];

// callbacks pour affichage status
void (*X_status_call)( char * );
void (*Y_status_call)( char * );
// on/off switch
int *run;

// buffers pour le graphique
float Xbuf[QBUF];
float Ybuf[QBUF];
unsigned int wri;	// write index
unsigned int rdi;	// read index (sert a moderer les redraws)
unsigned int bufspan;	// largeur de fenetre en mode scroll continu (en samples)

private:
// timestamp et valeur precedents
unsigned short oldtimeX;
unsigned short oldtimeY;
double valX;
double valY;

public:
// constructeur
process() : fifoWI(0), fifoRI(0), X_status_call(NULL), Y_status_call(NULL), run(NULL),
	    wri(0), rdi(0), bufspan(200),
	    oldtimeX(0), oldtimeY(0), valX(0.0), valY(0.0)
	{
	for	( int i = 0; i < QBUF; ++i )
		{ Xbuf[i] = 0.0; Ybuf[i] = 0.0; }
	};

// methodes
int step();

// la partie du process en relation avec jluplot
void prep_layout( gpanel * panneau );
void connect_layout( gpanel * panneau );
private:
void set_point( int abs_lag, float Xv, float Yv );
};

