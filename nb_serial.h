#ifdef  __cplusplus
extern "C" {
#endif

// ro = read only
// si com_port_number est nul, cherche le premier port après COM2
// rend le numero du port ouvert, 0 si pas trouvé, <0 en cas d'erreur
int nb_open_serial_ro( int com_port_number, unsigned int baudrate );

// rend le nombre d'octets lus (eventuelement zero)
// -1 en cas d'erreur
int nb_serial_read( char * rxbuf, unsigned int rxsize );

int nb_serial_write( char * txbuf, unsigned int txsize );

#ifdef  __cplusplus
}
#endif
