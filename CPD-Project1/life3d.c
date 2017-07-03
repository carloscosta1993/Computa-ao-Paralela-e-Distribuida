#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <omp.h>
#include "life3d_functions.c"

int main(int argc, char *argv[]){

   double start = omp_get_wtime();
   
   int SIZE_TABLE = 103001;
   
   int x, y, z, i, state, neighbours, iter, SIZE_CUBE;
   
   //Cell tables initialization
   cell * table = malloc(SIZE_TABLE*sizeof(cell));
   memset(table, -1, SIZE_TABLE*sizeof(cell));
   cell * aux_table = malloc(SIZE_TABLE*sizeof(cell));
   memset(aux_table, -1, SIZE_TABLE*sizeof(cell));
   cell * aux;
   cell current;
   
   
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
        insert(aux_table, x, y, z,SIZE_TABLE); //vai inserir cada celula viva do ficheiro na hash table
   } 

   if (argv[2] == NULL) {
        printf("Error: Number of Iterations not specified.");
        exit(1);
   }
   
   fclose (file);

   iter=atoi(argv[2]); 


   //Main Cycle
   
   for (i=0; i<iter; i++){
   
	aux=table;
	table=aux_table; //a table passa a ser a tabela da geração anterior ja com as celulas calculadas
	aux_table=aux; // a tabela auxiliar passa a ser a tabela principal sem qualquer motivo, apenas para ser limpa
	
	free_list(aux_table, SIZE_TABLE); //limpar a tabela aux_table para servir de colocação das celulas que vao ser calculadas. Esta tabela é a que vai ser usada na proxima geração.	
	
	int temp;
	for(temp=0; temp<SIZE_TABLE; temp++){ //vai percorrer cada celula
		current = table[temp]; //celula que vai ser analisada
		if(current.key!=-1){ //se a celula nao estiver vazia
			neighbours = num_alive_neighbours_a(table, aux_table, current.x, current.y, current.z, SIZE_TABLE, SIZE_CUBE); // recebe o numero de vizinhos vivos
			if (neighbours>=2 && neighbours<=4){ //se o numero de vizinhos vivos estiver dentro destes limites, entao passa a estar viva na proxima geraçao
				insert(aux_table, current.x, current.y, current.z, SIZE_TABLE); 
			}
			while (current.next != NULL) { // se dentro da mesma celula existir uma linked list com mais celulas
				current=*current.next; // apontador para a proxima celula na linked list
				neighbours = num_alive_neighbours_a(table, aux_table, current.x, current.y, current.z, SIZE_TABLE, SIZE_CUBE ); // faz a mesma verificação que em cima
				if (neighbours>=2 && neighbours<=4){
					insert(aux_table, current.x, current.y, current.z, SIZE_TABLE); 
				}
				
			}	
		}
	}
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
