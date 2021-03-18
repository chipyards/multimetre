class glostru {
public:
GtkWidget * wmain;
GtkWidget * vmain;
GtkWidget *   vpans;   
GtkWidget *     vpan1;
GtkWidget *       darea1;
GtkWidget *       zarea1;
GtkWidget *     darea2;
GtkWidget *   hbut;
GtkWidget *     brun;
GtkWidget *     slay;
GtkWidget *     erpX;
GtkWidget *     erpY;

gpanel panneau1;	// panneau1 dans darea1
gzoombar zbar;		// sa zoombar
gpanel panneau2;	// panneau2 dans darea2

// options
int COMport;		// COM serial port number, 0 ==> automatic
int option_power;
double k_power;
int option_swap;

// buffer pour lecture port serie
char rxbuf[64];
int rxcnt;

// fifo pour port serie
#define QFIFO		(1<<10)
#define FIFOMASK	(QFIFO-1)
char fifobuf[QFIFO];
unsigned int fifoWI;
unsigned int fifoRI;

// buffer pour message extrait du fifo
#define QMSG	16
char msgbuf[QMSG];

// buffers pour les layers X(t), Y(t)
#define QBUF 	(1<<11)
#define BUFMASK (QBUF-1)
float Xbuf[8][QBUF];
float Ybuf[8][QBUF];
unsigned int wri;
int curlag;

int recording_chan;	// normal       : 8 channels, recording XY layer index = recording_chan
			// option_power : 4 channels, recording layer pair { recording_chan*2, recording_chan*2+1 }  

int running;		// scroll continu
double Uspan;		// etendue fenetre scroll continu

char plot_fnam[128];
char plot_desc[128];

#define MAX_LAG		38
private:
// timestamps et valeurs precedents
unsigned short oldtimeX;
unsigned short oldtimeY;
double valX;
double valY;

public:
// constructeur
glostru() : COMport(0), option_power(0), k_power(1.0), option_swap(0),
	fifoWI(0), fifoRI(0), wri(0), curlag(0), recording_chan(0), running(1), Uspan(500.0),
	oldtimeX(0), oldtimeY(0), valX(0.0), valY(0.0) {
	snprintf( plot_fnam, sizeof(plot_fnam), "aimtti1.pdf" );
	snprintf( plot_desc, sizeof(plot_desc), "?" );
	};

// methodes

int step();	// fontion appelee periodiquement
void clearXY();
void clearXYall();
void set_point( int abs_lag, float Xv, float Yv );
void layout();

};

