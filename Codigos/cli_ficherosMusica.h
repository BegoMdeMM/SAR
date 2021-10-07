#define MAX_BUF 1024
#define PORT 6012

#define COM_SUB		0	/*Subir fichero*/
#define COM_MOD		1	/*Modificar nombre de fichero*/	
#define COM_ELM		2	/*Eliminar fichero*/
#define COM_LST		3	/*Solicitar lista de ficheros*/
#define COM_DWN		4	/*Descargar fichero*/
#define COM_PSW		5	/*Introducir password*/
#define COM_EXT		6	/*Cerrar sesion*/


#define OP_LISTFICH		1
#define OP_UPFICH		2
#define OP_DOWNFICH		3
#define OP_MODNOM		4
#define OP_DEL			5
#define OP_EXIT			6

char * COMANDOS[] = {"SUB","MOD","ELM","LST","DWN","PSW","EXT",NULL};
char * MSG_ERR[] =
{
	"Todo correcto.\n",
	"Error en la apertura de sesion: Password incorrecto.\n",
	"Error en la solicitud de la lista.\n",
	"Error al subir fichero: Espacio insuficiente en el servidor.\n",
	"Error al subir fichero: Ya existe un fichero con ese nombre.\n",
	"Error al descargar fichero: Ese fichero no existe (Comprobar nombre).\n",
	"Error al modificar nombre: El fichero no existe.\n",
	"Error al modificar nombre: Nombre de fichero vacio.\n",
	"Error al modificar nombre: Nombre de fichero ya existente.\n",
	"Error al eliminar fichero: Ese fichero no existe.\n"
};

int parse(char *status);
int readline(int stream, char *buf, int tam);
int menu();
void abrirSesion();
void solicitarLista();
void subirFichero();
void bajarFichero();
void modificarNombre();
void eliminarFichero();		
void salir();
