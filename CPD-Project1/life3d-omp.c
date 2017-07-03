#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <omp.h>
#include "life3d-omp-functions.c"

int main(int argc, char *argv[]){

   double start = omp_get_wtime();
   
   int SIZE_TABLE = 103001;   //103001
   
   int x, y, z, i, temp, neighbours, iter, SIZE_CUBE;
   
   //Cell tables initialization
   cell * table = malloc(SIZE_TABLE*sizeof(cell));
   memset(table, -1, SIZE_TABLE*sizeof(cell));
   cell * aux_table = malloc(SIZE_TABLE*sizeof(cell));
   memset(aux_table, -1, SIZE_TABLE*sizeof(cell));
   cell * aux;
   
   
   char line[100];
   char * points;

   if (argv[1] == NULL) {
        printf("Error: file pointer is null.");
        exit(1);
   } 
          
   FILE* file = fopen (argv[1], "r");
   fgets(line, sizeof(line), file);
   SIZE_CUBE = atoi(line);

   
   while (fgets(line, sizeof(line), file)) {
        points = strtok(line," ");
        x= atoi(points);
        points = strtok (NULL, " ");
        y= atoi(points);
        points = strtok (NULL, " ");
        z = atoi(points);
        insert(aux_table, x, y, z,SIZE_TABLE);
   } 

   if (argv[2] == NULL) {
        printf("Error: Number of Iterations not specified.");
        exit(1);
   }
   
   fclose (file);

   iter=atoi(argv[2]); 
   
   //Create locks to lock positions in the array
   omp_lock_t lock_aux[SIZE_TABLE];
   int h;
   for (h=0; h<SIZE_TABLE; h++)
        omp_init_lock(&(lock_aux[h]));


   //Main Cycle
   
   
   
   for (i=0; i<iter; i++){
   
	aux=table;
	table=aux_table;
	aux_table=aux;
	
	free_list(aux_table, SIZE_TABLE); 
	
	#pragma omp parallel // indicar ao compilador que o codigo dentro desta região irá correr em paralelo!x
	
	{

	#pragma omp for private (neighbours, temp) // the variables neighbours and temp are now private for each thread
	for(temp=0; temp<SIZE_TABLE; temp++){ 	// Cada thread vai receber um grupo de iterações do loop equivalente
		cell current = table[temp];
		if(current.key!=-1){
			neighbours= num_alive_neighbours_a(table, aux_table, current.x, current.y, current.z, SIZE_TABLE, SIZE_CUBE, lock_aux);
			if (neighbours>=2 && neighbours<=4){
	
			        
		        omp_set_lock(&(lock_aux[temp])); //fazer lock de um determinada posição da tabela para que duas threads nao acedam á mesma celula ao mesmo tempo.
				insert(aux_table, current.x, current.y, current.z, SIZE_TABLE); 
				omp_unset_lock(&(lock_aux[temp])); // fazer unlock para desbloquear o acesso aquela celula!
				
				
			}
			while (current.next != NULL) {
				current=*current.next;
				neighbours = num_alive_neighbours_a(table, aux_table, current.x, current.y, current.z, SIZE_TABLE, SIZE_CUBE, lock_aux);
				if (neighbours>=2 && neighbours<=4){		                
			                
		                omp_set_lock(&(lock_aux[temp]));
				        insert(aux_table, current.x, current.y, current.z, SIZE_TABLE); 
				        omp_unset_lock(&(lock_aux[temp]));
					
				}
				
			}	
		}
	}
	// quando acaba o loop encontra-se aqui uma barreira implicita para juntar de novo as threads
	
	} //end parallel
   }
   

   
   //Create ordered linked list
   
   cell * order[SIZE_CUBE];
   int l;
   for (l = 0; l<SIZE_CUBE; l++)
        order[l] = NULL;
   
   
   ordered_list(aux_table, order, SIZE_TABLE, SIZE_CUBE);
   
   //print ordered linked list
   
   double end = omp_get_wtime();
   
   print_list(order, SIZE_CUBE);
   
   
}
