#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/stat.h>
#include <unistd.h>

#include "cli_ficherosMusica.h"



int sock, sesion=0, n, cont, status, opcion;
char buf[MAX_BUF], param[MAX_BUF];
long tam_fich, leidos;
struct stat file_info;
FILE *fp;
struct hostent *hp;




int main(int argc, char *argv[])
{

// Comprobar el paso de parametros
	if(argc != 2)
	{
		fprintf(stderr, "Uso: %s <DireccionIPv4>\n", argv[0]);
		exit(1);
	}
/*
Crear un socket y enviar peticion de conexion al servidor.
*/
	struct sockaddr_in dir_serv;
	sock = socket(AF_INET, SOCK_STREAM, 0);
	dir_serv.sin_family = AF_INET;
	dir_serv.sin_port = htons(PORT);

	if((hp = gethostbyname(argv[1]))==NULL)
	{
		fprintf(stderr,"Error en la direccion IP: %s\n", argv[1]);
		exit(1);
	}
	memcpy(&dir_serv.sin_addr, hp->h_addr_list[0],hp->h_length);
	printf("Direccion IP: %s\n", inet_ntoa(dir_serv.sin_addr));
	connect(sock, (struct sockaddr *) &dir_serv, sizeof(dir_serv));
/*
Leer de la entrada estandar hasta leer EOF o encontrar algun error.
Lo leido sera guardado en la variable 'buf' y contendra el caracter de fin de string.
¿Que ocurrira si la linea leida tiene mas caracteres que la capacidad del buffer, MAX_BUF? Pon un valor mas pequenio para MAX_BUF y haz la prueba.
*/
	abrirSesion();
	do{
//Menu principal:
		opcion=menu();
		switch(opcion){
			case OP_LISTFICH:
				solicitarLista();
			break;

			case OP_UPFICH:
				subirFichero();
			break;

			case OP_DOWNFICH:
				bajarFichero();
			break;

			case OP_MODNOM:
				if(sesion!=2){
					printf("No eres administrador asi que no puedes acceder aqui\n");
				}else	
					modificarNombre();
			break;

			case OP_DEL:
				if(sesion!=2){
					printf("No eres administrador asi que no puedes acceder aqui\n");
				}else	
					eliminarFichero();		
			break;

			case OP_EXIT:
				salir();
			break;
			
		}
	}while(1);
	
/*
Cerrar conexion y liberar el socket.
*/
	close(sock);
}

//**************************************************************************************
//**************************************************************************************

//MENU

int menu(){
	char buf[MAX_BUF];
	int opcion;
	printf("\n");
	printf("\t\t\t\t*********************************************\n");
	printf("\t\t\t\t*********************************************\n");
	printf("\t\t\t\t**                                         **\n");
	printf("\t\t\t\t**       1. Mostrar Lista de ficheros      **\n");
	printf("\t\t\t\t**       2. Subir fichero                  **\n");
	printf("\t\t\t\t**       3. Bajar fichero		   **\n");
	printf("\t\t\t\t**       4. Modificar fichero              **\n");
	printf("\t\t\t\t**       5. Borrar fichero                 **\n");
	printf("\t\t\t\t**       6. Terminar                       **\n");
	printf("\t\t\t\t**                                         **\n");
	printf("\t\t\t\t*********************************************\n");
	printf("\t\t\t\t*********************************************\n");
	while(1)
		{
			fgets(buf,MAX_BUF,stdin);
			opcion = atoi(buf);
			if(opcion > 0 && opcion < 7)
				break;
			printf("\t\t\t\tOpcion erronea. Intentalo de nuevo: \n");
		}
	printf("\n");
	return opcion;

}

//**************************************************************************************
//**************************************************************************************

//Abrir sesion
/*
	Pedir al usuario que introduzca la pass para logearse.
		Si pass==Ø ->  Abre sesion.
		Si pass==passAdmin -> Abre sesion admin.
		si no -> error y volvera al menu.
	Llamar a sesion(char* pass) que lo que hará será abrir la sesión.
*/

void abrirSesion(){
	printf("Usuario: \n");
	fgets(param,MAX_BUF,stdin);
	param[strlen(param)-1]=0;
	if(strlen(param)!=0){
		int i=1;
		do{
			
			sprintf(buf,"%s?%s\r\n",COMANDOS[COM_PSW],param);
			write(sock, buf, strlen(buf));
			//readline(sock, buf, MAX_BUF);
			read(sock,buf,MAX_BUF);
			status = parse(buf);
			if(status != 0)
			{
				fprintf(stderr,"Error: \n");
				fprintf(stderr,"%s\n",MSG_ERR[status]);
				i--;
				continue;
			}
			printf("Te has logeado como administrador\n");
			sesion=2;
			i=0;
		}while(i!=0);
	}else{
		sesion=1;
		printf("Te has logeado como usuario\n");
		sprintf(buf,"%s?%s\r\n",COMANDOS[COM_PSW],param);
		write(sock,buf,strlen(buf));
		if(status != 0)
			{
				fprintf(stderr,"Error: \n");
				fprintf(stderr,"%s\n",MSG_ERR[status]);
			}
	}
}

//**************************************************************************************
//**************************************************************************************

// Solicitud de listado de ficheros
/*
	Se le pedira al usuario un nombre.
		- Si no inserta nada, se enviara la peticion al servidor para que le envie la lista de los nombres de los ficheros.
		- Si introduce un nombre, se enviara la peticion al servidor para que le envie la lista de los nombres de los ficheros que abrirSesio contienen el nombre puesto.
*/

void solicitarLista(){
	struct timeval timer;
	fd_set rset;
	printf("Si quieres la lsita de los objetos que contengan un nombre escribelo si no dale a enter\n");
	fgets(param,MAX_BUF,stdin);
	param[strlen(param)-1]=0;
	sprintf(buf,"%s?%s\r\n",COMANDOS[COM_LST],param);
	write(sock,buf,strlen(buf)); // Enviar comando
	//n = readline(sock, buf, MAX_BUF); // Recibir respuesta
	n=read(sock, buf,MAX_BUF);
	status = parse(buf);
	if(status != 0)
	{
		fprintf(stderr,"Error: \n");
		fprintf(stderr,"%s\n",MSG_ERR[status]);
	}
	else
	{
// Recibir lista de ficheros y mostrarla linea a linea
		int kop = 0;	//  Para controlar el numero de ficheros
		printf("Lista de ficheros recibida del servidor:\n");
		printf("------------------------------------------\n");
		//n = readline(sock, buf, MAX_BUF);
		n=read(sock, buf,MAX_BUF);	
		status = parse(buf);
		if(status != 0)
		{
			fprintf(stderr,"Error: \n");
			fprintf(stderr,"%s\n",MSG_ERR[status]);
		}else{	
			while(n > 2)
			{
				buf[n-2] = 0;
				char nombre[MAX_BUF],formato[MAX_BUF];
				memcpy(nombre,strtok(buf,"&"),n-5);
				memcpy(formato,strtok(NULL,"&"),3);
				printf("%s.%s\n",nombre,formato);
				tam_fich = atol(strtok(NULL,"&"));
				if(tam_fich < 0)
					printf("Tamaño desconocido\n");
				else
				{
					if(tam_fich < 1024)
						printf("% 5ld B\n", tam_fich);
					else if(tam_fich < 1024*1024)
						printf("%5.1f KB\n", tam_fich/1024.0);
					else if(tam_fich < 1024*1024*1024)
						printf("%5.1f MB\n", tam_fich/(1024.0*1024));
					else
						printf("%5.1f GB\n", tam_fich/(1024.0*1024*1024));
				}
				kop++;
				//n = readline(sock, buf, MAX_BUF);
				timer.tv_sec=2;
				timer.tv_usec=0;
				FD_ZERO(&rset);
				FD_SET(sock, &rset);
				if(n=select(sock+1,&rset,NULL,NULL,&timer)<0)
					printf("Error desconocido\n");
				else if(n>0)
					n=read(sock, buf,MAX_BUF);
		
				
			}
			if(kop > 0)
			{
				printf("------------------------------------------\n");
				printf("Total ficheros disponibles: %d.\n",kop);
			}
			else
				printf("No hay ficheros disponibles.\n");
			printf("------------------------------------------\n");
		}	
	}
}

//**************************************************************************************
//**************************status************************************************************

// Subir fichero
/*
	Se le pedirá al usuario el nombre del fichero que quiere subir. Se enviara al servidor que se prepare para la transeferencia de un archivo. 
		-Si el servidor tiene espacio suficiente y no existe ningun archivo con ese nombre se enviara el archivo.
		-Si no error y volvera al menu.
*/

void subirFichero(){
	printf("Indica el nombre del fichero que quieres subir: \n");
	fgets(param,MAX_BUF,stdin);
	param[strlen(param)-1] = 0;
	if(stat(param, &file_info) < 0)	// Obtener el tamanyo del fichero
		fprintf(stderr,"El fichero %s no se ha encontrado.\n",param);
	else
	{
		sprintf(buf,"%s?%s&%ld\r\n",COMANDOS[COM_SUB], param, file_info.st_size);
		write(sock,buf,strlen(buf)); // Enviar comando
		//n = readline(sock, buf, MAX_BUF); // Recibir respuesta
		n=read(sock, buf,MAX_BUF);
		status = parse(buf);
		if(status != 0)
		{
			fprintf(stderr,"Error: \n");
			fprintf(stderr,"%s\n",MSG_ERR[status]);
		}
		else
		{
			
			if((fp = fopen(param,"r")) == NULL) // Abrir fichero
			{
				fprintf(stderr,"Error al abrir el fichero %s.\n",param);
				exit(1);
			}
			while((n=fread(buf,1,MAX_BUF,fp))==MAX_BUF) // Enviar fichero a trozos
				write(sock,buf,MAX_BUF);
			if(ferror(fp)!=0)
			{
				fprintf(stderr,"Error al enviar el fichero.\n");
				exit(1);
			}
			write(sock,buf,n); // Enviar el ultimo trozo de fichero
			//n = readline(sock, buf, MAX_BUF); // Recibir respuesta
			n=read(sock, buf,MAX_BUF);
			status = parse(buf);
			if(status != 0)
			{
				fprintf(stderr,"Error: \n");
				fprintf(stderr,"%s\n",MSG_ERR[status]);
			}
			else
				printf("El fichero %s ha sido enviado correctamente.\n", param);
		}
	}
	
}

//**************************************************************************************
//**************************************************************************************

// Descargar fichero
/*
	Se pedira al usuario que introduzca el nombre del fichero que quiere descargar.
		- Si existe ese archivo se recibira.
		- Si no error y vuelve al menu.
*/

void bajarFichero(){
	printf("Indica el nombre del fichero que quieres bajar: \n");
	fgets(param,MAX_BUF,stdin);
	param[strlen(param)-1] = 0;
	sprintf(buf,"%s?%s\r\n",COMANDOS[COM_DWN], param);
	write(sock,buf,strlen(buf)); // Enviar comando
	//n = readline(sock, buf, MAX_BUF); // Recibir respuesta
	n=read(sock, buf,MAX_BUF);
	status = parse(buf);
	if(status != 0)
	{
		fprintf(stderr,"Error: \n");
		fprintf(stderr,"%s\n",MSG_ERR[status]);
	}
	else
	{
		buf[n-2] = 0; // Borra el final de linea (EOL)
		tam_fich = atol(buf+2);
		//n = readline(sock, buf, MAX_BUF); // Recibir respuesta
		n=read(sock, buf,MAX_BUF);
		status = parse(buf);
		leidos = 0;
		if((fp = fopen(param,"w")) == NULL) // Crear fichero
		{
			perror("No se puede guardar el fichero en el disco local\n");
			exit(1);
		}
		while(leidos < tam_fich) // Recibir fichero y guardar en disco
		{
			n = read(sock, buf, MAX_BUF);
			if(fwrite(buf, 1, n, fp) < 0)
			{
				perror("Error al guardar el fichero en el disco local\n");
				exit(1);
			}
			leidos += n;
		}
		fclose(fp);
		printf("El fichero %s ha sido recibido correctamente.\n",param);
		
	}
}

//**************************************************************************************
//**************************************************************************************

// Modificar el nombre de un fichero
/*
	***SOLO POSIBLE SI LOGEADO COMO ADMIN***
	Se le pediran al usuario 2 cosas:
		- El nombre del fichero que quiere modificar
		- El nombre nuevo.
*/

void modificarNombre(){
	char param2[MAX_BUF];
	printf("Indica el nombre del fichero que quieres modificar: \n");
	fgets(param,MAX_BUF,stdin);
	param[strlen(param)-1]=0;
	printf("Indica el nuevo nombre para el fichero: \n");
	fgets(param2,MAX_BUF,stdin);
	sprintf(buf,"%s?%s&%s\r\n",COMANDOS[COM_ELM], param, param2);
	write(sock,buf,strlen(buf));
	//n=readline(sock,buf,MAX_BUF);
	n=read(sock, buf,MAX_BUF);
	status = parse(buf);
	if(status !=0 )
	{
		fprintf(stderr,"Error: \n");
		fprintf(stderr,"%s\n",MSG_ERR[status]);
	}else{
		printf("El nombre del archivo se ha cambiado correctamente.\n");
	}
}

//**************************************************************************************
//**************************************************************************************

// Eliminar un fichero
/*
	***SOLO POSIBLE SI LOGEADO COMO ADMIN***
	Se pedira al usuario que introduzca el nombre del fichero que quiere eliminar.
		- Si existe ese archivo se eliminara.
		- Si no error y vuelve al menu.
*/

void eliminarFichero(){
	printf("Indica el nombre del fichero que quieres borrar: \n");
	fgets(param,MAX_BUF,stdin);
	param[strlen(param)-1] = 0;
	sprintf(buf,"%s?%s\r\n",COMANDOS[COM_ELM], param);
	write(sock,buf,strlen(buf)); // Enviar comando
	//n = readline(sock, buf, MAX_BUF); // Recibir respuesta
	n=read(sock, buf,MAX_BUF);
	status = parse(buf);
	if(status != 0)
	{
		fprintf(stderr,"Error: ");
		fprintf(stderr,"%s",MSG_ERR[status]);
	}
	else
		printf("El fichero %s ha sido borrado.\n", param);
}

//**************************************************************************************
//**************************************************************************************

// Cerrar sesion
/*
	Cerrara la conexion con el servidor y terminara el programa.
*/

void salir(){
	sprintf(buf,"%s\r\n",COMANDOS[COM_EXT]);
	write(sock,buf,strlen(buf)); // Enviar comando
	close(sock);
	exit(1);
}

//**************************************************************************************
//**************************************************************************************


int parse(char *status)
{
	if(strncmp(status,"OK",2)==0)
		return 0;
	else if(strncmp(status,"ER",2)==0)
		return(atoi(status)+2);
	else
	{
		fprintf(stderr,"Respuesta inesperada.\n");
		exit(1); 
	}
}


int readline(int stream, char *buf, int tam)
{
	/*
		Atencion! Esta implementacion es simple, pero no es nada eficiente.
	*/
	char c;
	int total=0;
	int cr = 0;

	while(total<tam)
	{
		int n = read(stream, &c, 1);
		if(n == 0)
		{
			if(total == 0)
				return 0;
			else
				return -1;
		}
		if(n<0)
			return -3;
		buf[total++]=c;
		if(cr && c=='\n')
			return total;
		else if(c=='\n')
			cr = 1;
		else
			cr = 0;
	}
	return -2;
}

