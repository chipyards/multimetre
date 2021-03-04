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
GtkWidget *     braz;
GtkWidget *     erpX;
GtkWidget *     erpY;

gpanel panneau1;	// panneau1 dans darea1
gzoombar zbar;		// sa zoombar
gpanel panneau2;	// panneau2 dans darea2

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
#define QMSG		16
char msgbuf[QMSG];

// buffers pour les layers X(t), Y(t)
#define QBUF 	(1<<11)
#define BUFMASK (QBUF-1)
float Xbuf[QBUF];
float Ybuf[QBUF];
unsigned int wri;

int running;		// scroll continu
double Uspan;		// etendue fenetre scroll continu

#define MAX_LAG		38
private:
// timestamps et valeurs precedents
unsigned short oldtimeX;
unsigned short oldtimeY;
double valX;
double valY;

public:
// constructeur
glostru() : fifoWI(0), fifoRI(0), wri(0), running(1), Uspan(1000.0),
	    oldtimeX(0), oldtimeY(0), valX(0.0), valY(0.0) {};

// methodes

int step();	// fontion appelee periodiquement
void clearXY();
void set_point( int abs_lag, float Xv, float Yv );
void layout();

};

