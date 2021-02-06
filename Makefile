# directories
GTKBASE= F:/Appli/msys64/mingw64

# listes
SOURCESC= modpop2.c 
SOURCESCPP= layers.cpp gluplot.cpp jluplot.cpp process.cpp nb_serial.c gui.cpp
HEADERS= glostru.h gluplot.h jluplot.h layers.h modpop2.h process.h nb_serial.h

EXE= gui.exe

OBJS= $(SOURCESC:.c=.o) $(SOURCESCPP:.cpp=.o)

# maintenir les libs et includes dans l'ordre alphabetique SVP

LIBS= `pkg-config --libs gtk+-2.0`
LUBS= -L$(GTKBASE)/lib \
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
INCS= -Wall -O2 -mms-bitfields -Wno-deprecated-declarations \
-I$(GTKBASE)/include/atk-1.0 \
-I$(GTKBASE)/include/cairo \
-I$(GTKBASE)/include/gdk-pixbuf-2.0 \
-I$(GTKBASE)/include/glib-2.0 \
-I$(GTKBASE)/include/gtk-2.0 \
-I$(GTKBASE)/include/pango-1.0 \
-I$(GTKBASE)/lib/glib-2.0/include \
-I$(GTKBASE)/lib/gtk-2.0/include \
-I$(GTKBASE)/include/harfbuzz
# cibles

ALL : $(OBJS)
	g++ -o $(EXE) -s $(OBJS) $(LIBS)

clean : 
	rm *.o

.cpp.o: 
	g++ $(INCS) -c $<

.c.o: 
	gcc $(INCS) -c $<

# dependances
modpop2.o : ${HEADERS}
layers.o : ${HEADERS}
gluplot.o : ${HEADERS}
jluplot.o : ${HEADERS}
process.o : ${HEADERS}
gui.o : ${HEADERS}
nb_serial.O : ${HEADERS}
