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

// buffers pour le graphique
float Lbuf[QBUF];

// timestamp et valeur precedents
unsigned short oldtimeX;
unsigned short oldtimeY;
double valX;
double valY;

// constructeur
process() : fifoWI(0), fifoRI(0), X_status_call(NULL), Y_status_call(NULL),
	    oldtimeX(0), oldtimeY(0), valX(0.0), valY(0.0) {
	for	( int i = 0; i < QBUF; ++i )
		Lbuf[i] = 0.0;
	};

// methodes
int step();

// la partie du process en relation avec jluplot
void prep_layout( gpanel * panneau );
void connect_layout( gpanel * panneau );
};

