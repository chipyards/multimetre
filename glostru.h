/* singleton pseudo global storage ( allocated in main()  )
   JLN's GTK widget naming chart :
   w : window
   f : frame
   h : hbox
   v : vbox
   b : button
   l : label
   e : entry
   s : spin adj
   m : menu
   o : option
 */


typedef struct
{
GtkWidget * wmain;
GtkWidget * vmain;
GtkWidget *   farea;
GtkWidget *   darea;
GtkWidget *   sarea;
GtkWidget *   hbut;
GtkWidget *     bpla;
GtkWidget *     brew;
GtkWidget *     esta;
GtkWidget *     erpX;
GtkWidget *     erpY;
GtkWidget *     bqui;
int darea_queue_flag;

gpanel panneau;		// panneau principal (wav), dans darea
gzoombar zbar;		// sa zoombar

process pro;		// le process : reception serial, calcul amps, preparation layout

int running;		// scroll continu

int darea_expose_cnt;
int idle_profiler_cnt;
int idle_profiler_time;

} glostru;

