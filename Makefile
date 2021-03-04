# directories
# GTKBASE= F:/Appli/msys64/mingw32
# on ne depend plus d'un F: absolu, mais on doit compiler avec le shell mingw32
GTKBASE= /mingw32

# listes
SOURCESC = modpop3.c nb_serial.c
SOURCESCPP = gluplot.cpp jluplot.cpp \
    layer_f_fifo.cpp layer_f_param.cpp  \
    gui.cpp
HEADERS = gluplot.h jluplot.h \
    layer_f_fifo.h layer_f_param.h \
    modpop3.h nb_serial.h gui.h

OBJS= modpop3.o nb_serial.o gluplot.o jluplot.o \
    layer_f_fifo.o layer_f_param.o \
    gui.o

EXE = gui.exe 
# maintenir les libs et includes dans l'ordre alphabetique SVP

# LIBS= `pkg-config --libs gtk+-2.0`
LIBS= -L$(GTKBASE)/lib \
-latk-1.0 \
-lcairo \
-lgdk-win32-2.0 \
-lgdk_pixbuf-2.0 \
-lglib-2.0 \
-lgmodule-2.0 \
-lgobject-2.0 \
-lgtk-win32-2.0 \
-lpango-1.0 \
-lpangocairo-1.0 \
-lpangowin32-1.0

# -mwindows
# enlever -mwindows pour avoir la console stdout


# options
# INCS= `pkg-config --cflags gtk+-2.0`
INCS= -Wall -Wno-parentheses -Wno-deprecated-declarations -O2 -mms-bitfields \
-I$(GTKBASE)/include \
-I$(GTKBASE)/include/atk-1.0 \
-I$(GTKBASE)/include/cairo \
-I$(GTKBASE)/include/gdk-pixbuf-2.0 \
-I$(GTKBASE)/include/glib-2.0 \
-I$(GTKBASE)/include/gtk-2.0 \
-I$(GTKBASE)/include/harfbuzz \
-I$(GTKBASE)/include/pango-1.0 \
-I$(GTKBASE)/lib/glib-2.0/include \
-I$(GTKBASE)/lib/gtk-2.0/include \
# cibles

ALL : $(OBJS)
	g++ -o $(EXE) -s $(OBJS) $(LIBS)

clean :
	rm *.o

modpop3.o : modpop3.c
	gcc $(INCS) -c modpop3.c
nb_serial.o : nb_serial.c
	gcc $(INCS) -c nb_serial.c
gluplot.o : gluplot.cpp ${HEADERS}
	gcc $(INCS) -c gluplot.cpp
jluplot.o : jluplot.cpp ${HEADERS}
	gcc $(INCS) -c jluplot.cpp
layer_f_fifo.o : layer_f_fifo.cpp ${HEADERS}
	gcc $(INCS) -c layer_f_fifo.cpp
layer_f_param.o : layer_f_param.cpp ${HEADERS}
	gcc $(INCS) -c layer_f_param.cpp
gui.o : gui.cpp ${HEADERS}
	gcc $(INCS) -c gui.cpp

