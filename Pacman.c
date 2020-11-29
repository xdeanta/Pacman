#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <ncurses.h>

/*
	Pacman en ncurses
	Xavier De Anta.
	Johan Velasquez.
*/

typedef struct _Char{
	char id;
	int posX;
	int posY; //Struct para los personajes
} t_char;
/* Prototipos de las funciones */
// -Hilos- //
void* jugador(); //Hilo jugador
void* fantasma(void*); //Hilo fantasma
// --Funciones del hilo principal-- //
void getposchars(char**, int, int); //funcion para localizar personajes en el archivo mapa
char** init_buffer(FILE*,char*, int, int); //buffer del mapa leido del archivo y lo retorna como arreglo bidimensional de caracteres
// --Funciones de los otros hilos -- //
// ---Funciones del hilo jugador --- //
void movechar(char ,int ,int ,int*); //Mueve al jugador a traves del mapa
void refreshS(WINDOW*, int); //Muestra el puntaje del jugador
int checkpos(int ,int ,int*); //Verifica si la posicion que se mueve el jugador es valida
// ---Funciones de los hilos fantasmas --- //
void movefan(char*,int, int, int, int); //Similar a movechar pero con los fantasmas
int checkposfan(int, int); //Similar a checkpos pero para los fantasmas
// -Variables Globales //
t_char chars[5]; //Arreglo de personajes
WINDOW* map; //La ventana que muestra el mapa
int thread_counter=0; //Esto es el indice de los hilos fantasmas
int game_status=0; //Status del juego. 1 si el jugador gana, -1 si el jugador pierde
int dot_map=0; //Contador de puntos en el buffer del mapa
sem_t sem_map, sem_ptID, sem_move_ff, sem_move_f, sem_status;
/* Semaforos: */
/* sem_map: proteje el acceso al mapa
   sem_ptID: proteje la variable thread_counter
   sem_move_ff: proteje el movimiento entre los fantasmas
   sem_move_f: proteje el movimiento entre el jugador y los fantasmas
   sem_status: proteje el acceso y modificacion de la variable game_status
*/
int main(int argc, char** argv){
	FILE* fmap; //Archivo del mapa
	char** bufmap; //Buffer del mapa
	int bufmap_x, bufmap_y,i;
/*
	bufmap_x: numero de columnas de la matriz
	bufmap_y: numero de filas de la matriz
	i: indice para los for
*/
	pthread_t IA[4];
	pthread_t user;
	bufmap_y=atoi(argv[1]); //numero de filas de la matriz por parametro del programa
	bufmap_x=atoi(argv[2]); //numero de columnas
	bufmap=init_buffer(fmap,argv[3],bufmap_y,bufmap_x); //Creo el buffer del mapa
	initscr(); //Inicializacion de ncurses
	cbreak();  //Entrada directa del teclado
	noecho();
	curs_set(0); //Oculta la posicion del cursor de la pantalla
	map=newwin(bufmap_y,bufmap_x,1,0); //Creo una ventana del tamano del mapa
//  Inicializacion de los semaforos
	sem_init(&sem_map,0,1);
	sem_init(&sem_ptID,0,1);
	sem_init(&sem_move_f,0,0);
	sem_init(&sem_move_ff,0,1);
	sem_init(&sem_status,0,1);
//	Crea el mapa en la pantalla
	for(i=0; i < bufmap_y; i++){
		waddstr(map, bufmap[i]);
	}
	wrefresh(map);
//  Borra el buffer del mapa
	for(i=0; i < bufmap_y; i++){
		free(bufmap[i]);
	}
	free(bufmap);
//	Creo los hilos fantasma
	for(i=0; i < 4; i++){
		pthread_create(&IA[i], NULL, fantasma, NULL);
	}
//	Creo el hilo jugador
	pthread_create(&user, NULL, jugador, NULL);
//	Espero por el hijo jugador
	pthread_join(user, NULL);
	//wgetch(map);
//	Al finalizar el juego, Limpio la pantalla y muestra un mensaje dependiendo el status del juego
	wclear(map);
	if(game_status==1){
		wprintw(map,"Felicitaciones\n");
		wrefresh(map);
		wprintw(map,"Presione una tecla para salir\n");
		wrefresh(map);
		wgetch(map);
	}
	if(game_status==-1){
		wprintw(map,"Game Over\n");
		wrefresh(map);
		wprintw(map,"Presione una tecla para salir\n");
		wrefresh(map);
		wgetch(map);
	}
//	Destruyo los semaforos antes de finalizar el programa
	sem_destroy(&sem_map);
	sem_destroy(&sem_ptID);
	sem_destroy(&sem_move_ff);
	sem_destroy(&sem_move_f);
	sem_destroy(&sem_status);
//	Destruyo la pantalla ncurses
	endwin();
//	Fin del programa
	return 0;
}

char** init_buffer(FILE* a,char* arg, int y, int x){
	char** array; //Buffer del mapa
	int i;
	a=fopen(arg, "r");
	if(a==NULL){
		fprintf(stderr, "Error al abrir el archivo\n");
		exit(1);
	}
// 	Creo el buffer del mapa
	array=(char**)malloc(y*sizeof(char*));
	for(i=0; i < y; i++){
		array[i]=(char*)malloc(x*sizeof(char));
	}
	i=0;
	while(i<y){
//	Extraigo del archivo el mapa
		fgets(array[i],x+1,a);
		i++;
	}
	fclose(a);
//	Llama a la funcion que localiza los personajes del mapa
	getposchars(array,y,x);
//	Retorno el arreglo
	return array;
}

void getposchars(char** array, int y, int x){
	int i, j;
	char charx;
	for(i=0;i<y;i++){
		for(j=0;j<x;j++){
			charx=array[i][j];
			switch(charx){
				case 'P':
				//Posicion del Jugador
					chars[0].id=charx;
					chars[0].posX=i;
					chars[0].posY=j;
					
				break;
				//Posiciones de los fantasmas
				case '1':
					chars[1].id=charx;
					chars[1].posX=i;
					chars[1].posY=j;
				break;
				case '2':
					chars[2].id=charx;
					chars[2].posX=i;
					chars[2].posY=j;
				break;
				case '3':
					chars[3].id=charx;
					chars[3].posX=i;
					chars[3].posY=j;
				break;
				case '4':
					chars[4].id=charx;
					chars[4].posX=i;
					chars[4].posY=j;
				break;
				//Contador de puntos del mapa para comparar con el puntaje del jugador
				case '.':
					dot_map++;
				break;
			}
		}
	}
	dot_map=dot_map+4; //Sumo 4 porque en las posiciones iniciales de los fantasmas hay puntos
}
// Funcion del Hilo jugador
void* jugador(){
	WINDOW* score; //Ventana asociada al jugador que muestra el puntaje acumulado
	int i; //indice para un for
	int iscore=0; //Acumulador de puntaje
	int nposx, nposy; //Pocision del jugador
	char ch; //Caracter leido de la consola
	score=newwin(1,10, 0,0); //Inicializacion de la ventana del puntaje
	wprintw(score,"Score:%d\n",iscore); //Puntaje inicial
	wrefresh(score);;
	while(1){
		//Verifico primero el estatus del juego
		//Semaforo de status, para el caso que otro hilo este modificando la variable
		//SECCION CRITICA: STATUS
		sem_wait(&sem_status);
		if(dot_map==iscore){
			game_status=1;
		}
		//Condicional que rompe el ciclo infinito si el juego termino
		if(game_status!=0){
			sem_post(&sem_status);
			break;
		}
		//FIN DE SECCION CRITICA
		sem_post(&sem_status);
		nposx=chars[0].posX;
		nposy=chars[0].posY;
		//Leo un caracter de la consola
		ch=wgetch(map);
		if(ch=='q'){
			break;
		}else{
			//Funcion que permite mover al jugador
			movechar(ch,nposx,nposy,&iscore);
			//Refresco la ventana del puntaje
			refreshS(score,iscore);
		}
		//Este for es para indicar a los fantasmas que se pueden mover. Hilo Productor
		for(i=0;i<4;i++){
			sem_post(&sem_move_f);
		}
	}
	pthread_exit(NULL);
}

void* fantasma(void* args){
	//Indice del hilo
	int ptID;
	//Movimiento del fantasma
	int move;
	//Posicion del fantasma
	int nposx, nposy;
	//Caracter previo al movimiento del fantasma
	char prevchar='.';
	//SECCION CRITICA: thread_counter
	sem_wait(&sem_ptID);
		thread_counter++;
		//Guardo el indice del hilo
		ptID=thread_counter;
	//FIN DE SECCION CRITICA
	sem_post(&sem_ptID);
		while(1){
			//Espera por el jugador se haya movido de posicion. Hilo Consumidor
			sem_wait(&sem_move_f);
			//SECCION CRITICA: movimiento entre fantasmas
			sem_wait(&sem_move_ff);
			//SECCION CRITICA: Status del juego
			sem_wait(&sem_status);
			//Condicional: Si el juego termino, rompo el ciclo
			if(game_status!=0){
				sem_post(&sem_status);
				break;
			}
			//FIN DE SECCION CRITICA: Status
			sem_post(&sem_status);
			nposx=chars[ptID].posX;
			nposy=chars[ptID].posY;
			//Obtiene el movimiento a realizar: Movimiento Aleatorio
			srand(time(NULL));
			//Rango de rand(): 1 - 4
			move=rand() % 4 + 1;
			movefan(&prevchar,move,nposx,nposy, ptID);
			//FIN DE SECCION CRITICA: Movimiento entre fantasmas
			sem_post(&sem_move_ff);
		}
	pthread_exit(NULL);
}

void movechar(char a, int x, int y, int *piscore){
	//Movimientos Temporales
	//checkpos Verifica si la posicion a la cual me voy a mover es valida
	//Si es valida retorna 1, sino retorna 0
	int tempX, tempY;
	switch(a){
		case 'w':
			tempX=x-1;
			if(checkpos(tempX,y,piscore)==1){
				//SECCION CRITICA: Acceso a la ventana del mapa
				sem_wait(&sem_map);
					mvwaddch(map,x,y,' ');
					wrefresh(map);
					mvwaddch(map,tempX,y,'P');
					wrefresh(map);
				//FIN DE SECCION CRITICA
				sem_post(&sem_map);
				//Actualizo la posicion actual del jugador
				chars[0].posX=tempX;
			}
		break;
		case 's':
			tempX=x+1;
			if(checkpos(tempX,y,piscore)==1){
				//SECCION CRITICA: Acceso a la ventana del mapa
				sem_wait(&sem_map);
					mvwaddch(map,x,y,' ');
					wrefresh(map);
					mvwaddch(map,tempX,y,'P');
					wrefresh(map);
				//FIN DE SECCION CRITICA
				sem_post(&sem_map);
				//Actualizo la posicion actual del jugador
				chars[0].posX=tempX;
			}
		break;
		case 'a':
			tempY=y-1;
			if(checkpos(x,tempY,piscore)==1){
				//SECCION CRITICA: Acceso a la ventana del mapa
				sem_wait(&sem_map);
					mvwaddch(map,x,y,' ');
					wrefresh(map);
					mvwaddch(map,x,tempY,'P');
					wrefresh(map);
				//FIN DE SECCION CRITICA
				sem_post(&sem_map);
				//Actualizo la posicion actual del jugador
				chars[0].posY=tempY;
			}
		break;
		case 'd':
			tempY=y+1;
			if(checkpos(x,tempY,piscore)==1){
				//SECCION CRITICA: Acceso a la ventana del mapa
				sem_wait(&sem_map);
					mvwaddch(map,x,y,' ');
					wrefresh(map);
					mvwaddch(map,x,tempY,'P');
					wrefresh(map);
				//FIN DE SECCION CRITICA
				sem_post(&sem_map);
				//Actualizo la posicion actual del jugador
				chars[0].posY=tempY;
			}
		break;
	}
}

int checkpos(int x, int y,int* piscore){
	char text;
	/*Obtengo el caracter de la ventana en la posicion x,y, retorna 1 si es valida
	  Sino retorna 0, actualiza el puntaje si la posicion x,y hay un punto y cambia
	  el status del juego si el jugador se mueve en una posicion hay un fantasma*/
	text=mvwinch(map, x, y) & A_CHARTEXT;
	switch(text){
		case '.':
			*piscore=*piscore + 1;
			return 1;
			break;
		case '*':
			return 0;
			break;
		case '1':
			sem_wait(&sem_status);
			game_status=-1;
			sem_post(&sem_status);
			return 1;
			break;
		case '2':
			sem_wait(&sem_status);
			game_status=-1;
			sem_post(&sem_status);
			return 1;			
			break;
		case '3':
			sem_wait(&sem_status);
			game_status=-1;;
			sem_post(&sem_status);
			return 1;			
			break;
		case '4':
			sem_wait(&sem_status);
			game_status=-1;
			sem_post(&sem_status);
			return 1;			
			break;
		case ' ':
			return 1;
			break;

	}
}

void refreshS(WINDOW* a, int s){
	//Refresca la pantalla de puntaje
	wclear(a);
	wprintw(a, "Score:%d\n",s);
	wrefresh(a);
}

void movefan(char* pchar,int move, int x, int y,int fid){
	//Similar a movechar
	int tempX, tempY;
	char prevchar;
	prevchar=*pchar;
	switch(move){
		case 1:
			tempX=x-1;
			if(checkposfan(tempX,y)==1){
				*pchar=mvwinch(map,tempX,y) & A_CHARTEXT;
				sem_wait(&sem_map);
					mvwaddch(map,x,y,prevchar);
					wrefresh(map);
					mvwaddch(map,tempX,y,chars[fid].id);
					wrefresh(map);
				sem_post(&sem_map);
				chars[fid].posX=tempX;
			}
			break;
		case 2:
		tempX=x+1;
		if(checkposfan(tempX,y)==1){
			*pchar=mvwinch(map,tempX,y) & A_CHARTEXT;
			sem_wait(&sem_map);
				mvwaddch(map,x,y,prevchar);
				wrefresh(map);
				mvwaddch(map,tempX,y,chars[fid].id);
				wrefresh(map);
			sem_post(&sem_map);
			chars[fid].posX=tempX;
		}
		break;
		case 3:
			tempY=y-1;
			if(checkposfan(x,tempY)==1){
				*pchar=mvwinch(map,x,tempY) & A_CHARTEXT;
				sem_wait(&sem_map);
					mvwaddch(map,x,y,prevchar);
					wrefresh(map);
					mvwaddch(map,x,tempY,chars[fid].id);
					wrefresh(map);
				sem_post(&sem_map);
				chars[fid].posY=tempY;
			}
			break;
		case 4:
			tempY=y+1;
			if(checkposfan(x,tempY)==1){
				*pchar=mvwinch(map,x,tempY) & A_CHARTEXT;
				sem_wait(&sem_map);
					mvwaddch(map,x,y,prevchar);
					wrefresh(map);
					mvwaddch(map,x,tempY,chars[fid].id);
					wrefresh(map);
				sem_post(&sem_map);
				chars[fid].posY=tempY;
			}
			break;
	}
}

int checkposfan(int x, int y){
	//Similar a checkpos
	char text;
	text=mvwinch(map,x,y);
	switch(text){
		case '.':
			return 1;
			break;
		case '*':
			return 0;
			break;
		case '1':
			return 0;
			break;
		case '2':
			return 0;
			break;
		case '3':
			return 0;
			break;
		case '4':
			return 0;
			break;
		case ' ':
			return 1;
			break;
		case 'P':
			sem_wait(&sem_status);
			game_status=-1;
			sem_post(&sem_status);
			return 1;
			break;

	}
}