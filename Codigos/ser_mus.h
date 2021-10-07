#define MAX_BUF 1024
#define PORT 6012
#define FILES_PATH	"files"

#define ST_INIT	0
#define ST_USER	1
#define ST_ROOT	2
#define ST_LERR	3

#define COM_PASS	0
#define COM_UPLO	1
#define COM_RENM	2
#define COM_DELE	3
#define COM_LIST	4
#define COM_DOWN	5
#define COM_EXIT	6

#define MAX_UPLOAD_SIZE	10*1024*1024	// 10 MB
#define SPACE_MARGIN		50*1024*1024	// 50 MB

struct Nodo *canciones = NULL;

char * COMANDOS[] = {"PSW","SUB","MOD","ELM","LST","DWN","EXT",NULL};
char * FICHEROS[] ={};
char * passwords[] = {"root"};
int estado;

void sesion(int s);
void renombrar(int s, char *buf);
int readline(int stream, char *buf, int tam);
int busca_string(char *string, char **strings);
int busca_substring(char *string, char **strings);
void inesperado(int s);
void enviar_lista_fich(int s,char *com);
unsigned long espacio_libre();
int no_oculto(const struct dirent *entry);

struct Nodo{
	char info[40];
	struct Nodo * next;
};