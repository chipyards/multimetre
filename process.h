#define QFIFO		(1<<10)
#define FIFOMASK	(QFIFO-1)

#define QBUF 		(1<<12)
#define BUFMASK 	(QBUF-1)

#define QMSG		16

class process {
public:

// buffer pour lecture port serie
unsigned char rxbuf[64];
int rxcnt;

// fifo pour port serie
unsigned char fifobuf[QFIFO];
unsigned int fifoWI;
unsigned int fifoRI;

// buffer pour message extrait du fifo
unsigned char msgbuf[QMSG];

// callback pour affichage textuel
void (*serial_msg_call)( char * );

// buffers pour le graphique
float Lbuf[QBUF];


// constructeur
process() : fifoWI(0), fifoRI(0), serial_msg_call(NULL) {
	for	( int i = 0; i < QBUF; ++i )
		Lbuf[i] = 0.0;
	};

// methodes
int step();

// la partie du process en relation avec jluplot
void prep_layout( gpanel * panneau );
void connect_layout( gpanel * panneau );
};

