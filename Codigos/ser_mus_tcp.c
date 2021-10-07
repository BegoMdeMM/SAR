#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/statvfs.h>
#include <stdbool.h>

#include "ser_mus.h"


int main()
{
	int sock, dialogo;
	struct sockaddr_in dir_serv;
	socklen_t tam_dir;

	// Crear el socket
	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Error al crear el socket");
		exit(1);
	}
	
	memset(&dir_serv, 0, sizeof(dir_serv));
	dir_serv.sin_family = AF_INET;
	dir_serv.sin_addr.s_addr = htonl(INADDR_ANY);
	dir_serv.sin_port = htons(PORT);
	
	// Asignar direccion al socket
	if(bind(sock, (struct sockaddr *) &dir_serv, sizeof(dir_serv)) < 0)
	{
		perror("Error al asignar una direccion al socket");
		exit(1);
	}

	// Preparar el socket como socket de escucha
	if(listen(sock,5) < 0)
	{
		perror("Error al definir el socket como socket de escucha");
		exit(1);
	}

	// Ignorar la senyal enviada cuando un proceso hijo acaba, para que no quede como zombi
	signal(SIGCHLD, SIG_IGN);
	while(1)
	{
		// Aceptar peticion de conexion de cliente y crear socket de dialogo
		if((dialogo = accept(sock, NULL, NULL)) < 0)
		{
			perror("Error al aceptar la conexion");
			exit(1);
		}

		// Crear proceso hijo para que se encargue de la comunicacion con el cliente
		switch(fork())
		{
			case 0:
				close(sock);
				sesion(dialogo);
				close(dialogo);
				exit(0);
			default:
				close(dialogo);
		}
	}
}

void sesion(int s){ 
	char buf[MAX_BUF], file_path[MAX_BUF], file_name[MAX_BUF];
	int n, pass, comando, error;
	FILE *fp;
	struct stat file_info;
	unsigned long file_size, leidos;
	char * sep;

	//poner el estado inicial;
	estado = ST_INIT;
	while(1)
	{
		//leer  lo que envial el cliente
		if((n=readline(s,buf,MAX_BUF)) <= 0)
			return;
		//mirar si el comando esta en nuestra lista de comandos
		if((comando=busca_substring(buf,COMANDOS)) < 0)
		{
			inesperado(s);
			continue;// rompe la iteracion del ciclo pero sique con el proceso
		}
		//casuistica por comando
		switch(comando){
			//iniciar sesion
			case COM_PASS:
			printf("%s\n",buf+4);
				if((estado == ST_LERR) || (estado == ST_INIT)){
					buf[n-2] = 0;
					if(strlen(buf) == 4){
						estado = ST_USER;
						printf("somos usuario estandar\n");	
						write(s,"OK\r\n",5);
					}
					else if((pass = busca_substring(buf+4/*QUITA EL COMANDO*/, passwords)) < 0){
							if(estado == ST_INIT){
								write(s,"ER1\r\n",5);
								estado = ST_LERR;
							}else if(estado == ST_LERR){
								close(s);
							}
						}else{
							estado = ST_ROOT;	
							printf("somos root\n");	
							write(s,"OK\r\n",5);				
					}
				}else{
					write(s,"ERX\r\n",5);//algun error no especificado
				}
				continue;
			break;
			//cerrar conexion
			case COM_EXIT:
				write(s,"OK\r\n",5);
				close(s);
			break;
			//listado de canciones
			case COM_LIST:
				if((estado == ST_INIT) || (estado == ST_LERR)){
					write(s,"ERX\r\n",5);
				}else{
					buf[n-2] = 0;
					if(enviar_listado(s,buf+4) < 0) write(s,"ER2\r\n",5);
				}
			break;
			//RENOMBRAR, FALTA DECIR COMO SE SEPARA UNO DEL OTRO
			case COM_RENM:
				if(estado == ST_ROOT){
					buf[n-2] = 0;
					renombrar(s,buf+4);
				}else{write(s,"ERX\r\n",5);}
			break;
			case COM_DELE:
				if(estado==ST_ROOT){
					buf[n-2] = 0;
					sprintf(file_path,"%s/%s",FILES_PATH,buf+4);
					if(unlink(file_path) < 0)
						write(s,"ER9\r\n",6);
					else
						write(s,"OK\r\n",4);
				}else{
					write(s,"ERX\r\n",5);
				}
			break;
			case COM_UPLO:
				if((estado == ST_ROOT) || (estado== ST_USER)){
					buf[n-2] = 0;
					if((sep = strchr(buf,'&')) == NULL){
						write(s,"ERX\r\n",5);
						return;
					}
					*sep = 0;
					sep++;
					strcpy(file_name,buf+4); // Nombre del fichero
					file_size = atoi(sep);
					if(espacio_libre() < file_size + SPACE_MARGIN){
						write(s,"ER3\r\n",5);
						return;
					}


					struct dirent ** nombre_ficheros;
					int i,num_ficheros;
					struct stat file_info;

					num_ficheros = scandir(FILES_PATH, &nombre_ficheros, no_oculto,alphasort);
					for(i=0;i<num_ficheros;i++)
					{
						if(strcmp(nombre_ficheros[i]->d_name,file_name)==0){
							write(s,"ER4\r\n",5);
							break;
						}
					}
					write(s,"OK\r\n",5);

					sprintf(file_path,"%s/%s",FILES_PATH,file_name);
					// Crear el fichero nuevo
					fp=fopen(file_path,"w");
					error = (fp == NULL);
					while(leidos < file_size)
					{
						// Recibir un trozo de fichero
						if((n=read(s,buf,MAX_BUF)) <= 0){
							close(s);
							return;
						}
						if(!error){
							if(fwrite(buf, 1, n, fp) < n){
								fclose(fp);
								unlink(file_path);
								error = 1;
							}
						}
						leidos += n;
					}
					fclose(fp);
				}else{
					write(s,"ERX\r\n",5);
				}
			break;
			case COM_DOWN:
			if((estado == ST_USER) || (estado == ST_ROOT)){
				buf[n-2]=0;
				sprintf(file_path,"%s/%s",FILES_PATH,buf+4);
				if(stat(file_path, &file_info) < 0)
					write(s,"ER5\r\n",5);
				else{
					sprintf(buf,"OK?%ld\r\n", file_info.st_size);
					write(s, buf, strlen(buf));
				}
				if((fp=fopen(file_path,"r")) == NULL){
					//write(s,"ER5\r\n",5);
				}
				else{
					// Leer el primer trozo de fichero
					if((n=fread(buf,1,MAX_BUF,fp))<MAX_BUF && ferror(fp) != 0)
						write(s,"ER5\r\n",5);
					else{
						do
						{
							write(s,buf,n);
						}while((n=fread(buf,1,MAX_BUF,fp))==MAX_BUF);
						if(ferror(fp) != 0)
							{
								return;
							}else if(n>0)
								write(s,buf,n);
								// Enviar ultimotrozo
							}
							fclose(fp);
					}
			}else{
				write(s,"ERX\r\n",5);
			}

			break;
			default: 
				printf("eleccion por defecto, no tendriamos que entrar nunca\n");
			break;
		}
	}
}


 void renombrar(int s, char *buf){
 	char * nuevo;
 	char  original[MAX_BUF];

	if((nuevo = strchr(buf,'&')) == NULL){
		write(s,"ER7\r\n",5);
	}else{
			*nuevo = 0;
			nuevo++;
			/*if(strlen(nuevo)==1){
				write(s,"ER7\r\n",5);
				return;
			}*/
			strcpy(original,buf); // Nombre del fichero
			struct dirent ** nombre_ficheros;
			int i,num_ficheros;
			struct stat file_info;
			bool enc=false;
			bool dupli=false;

			num_ficheros = scandir(FILES_PATH, &nombre_ficheros, no_oculto,alphasort);
			for(i=0;i<num_ficheros;i++)
			{
				if(strcmp(nombre_ficheros[i]->d_name,original)==0){
					enc=true;
				}
				if(strcmp(nombre_ficheros[i]->d_name,nuevo) ==0){
					dupli=true;
				}
			}
		if(!enc){
			write(s,"ER6\r\n",5);
		}else if(dupli){
			write(s,"ER8\r\n",5);
		}else{
			write(s,"OK\r\n",5);
		}
	}
/*	num_ficheros = scandir(FILES_PATH, &nombre_ficheros, no_oculto,alphasort);
	if(num_ficheros < 0)
		return -1;
	if(write(s,"OK\r\n",4) < 4)
		return -1;
	for(i=0;i<num_ficheros;i++)
	{
		sprintf(path_fichero,"%s/%s",FILES_PATH, nombre_ficheros[i]->d_name);
		if(stat(path_fichero,&file_info) < 0)
			tam_fichero=-1L;
		else
			tam_fichero = file_info.st_size;
		sprintf(buf,"%s&mp3&%ld\r\n",nombre_ficheros[i]->d_name, tam_fichero);
		if((strcmp(nombre_ficheros[i]->d_name,nombre)==0) || (strlen(nombre)==0)){
			if(write(s, buf, strlen(buf)) < strlen(buf))
				return i;
		}
	}*/
 }

int readline(int stream, char *buf, int tam)
{
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
		else if(c=='\r')
			cr = 1;
		else
			cr = 0;
	}
	return -2;
}

//BUSCAR EL COMANDO
int busca_substring(char *string, char **strings)
	{
	int i=0;
	while(strings[i] != NULL)
	{
		if(!strncmp(string,strings[i],strlen(strings[i])))
		return i;
		i++;
	}
	return -1;
}

//tratamiento del un comando inesperado
void inesperado(int s)
	{
	if(estado == ST_INIT)
	{
		estado = ST_LERR;
		write(s,"ER1\r\n",5);
	}
	else if(estado == ST_LERR)
	{
		estado = ST_INIT;
		close(s);
	}
	write(s,"ERX\r\n",5);
}

int enviar_listado(int s, char *nombre)
	{
	struct dirent ** nombre_ficheros;
	int i,num_ficheros;
	long tam_fichero;
	char buf[MAX_BUF], path_fichero[MAX_BUF];
	struct stat file_info;

	num_ficheros = scandir(FILES_PATH, &nombre_ficheros, no_oculto,alphasort);
	if(num_ficheros < 0)
		return -1;
	if(write(s,"OK\r\n",4) < 4)
		return -1;
	for(i=0;i<num_ficheros;i++)
	{
		sprintf(path_fichero,"%s/%s",FILES_PATH, nombre_ficheros[i]->d_name);
		if(stat(path_fichero,&file_info) < 0)
			tam_fichero=-1L;
		else
			tam_fichero = file_info.st_size;
		sprintf(buf,"%s&mp3&%ld\r\n",nombre_ficheros[i]->d_name, tam_fichero);
		if((strcmp(nombre_ficheros[i]->d_name,nombre)==0) || (strlen(nombre)==0)){
			if(write(s, buf, strlen(buf)) < strlen(buf))
				return i;
		}
		write(s,"\r\n",2);
	}
	return num_ficheros;
}

int no_oculto(const struct dirent *entry){
	if (entry->d_name[0]== '.')
		return (0);
	else
		return (1);
}

unsigned long espacio_libre()
{
	struct statvfs info;
	if(statvfs(FILES_PATH,&info)<0)
		return -1;
	return info.f_bsize * info.f_bavail;
}
