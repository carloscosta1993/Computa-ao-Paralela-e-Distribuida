typedef struct node {
    int x;
    int y;
    int z;
    int key;
    struct node * next;
} cell;

cell* push(cell * head, int x, int y, int z);

//Generate a key for the given triplet. Collisions can occour.
unsigned long gen_key(int x, int y, int z)
{   
    unsigned long hash = (x*677 + y)*971 + z;
    
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    
    return hash;
}

//Compute modulus
int mod(int a, int b)
{
    int r = a % b;
    return r < 0 ? r + b : r;
}

//Generate index for insertion in the hashtable
int hashcode(int key, int SIZE_TABLE)
{
        return mod(key, SIZE_TABLE);
}

//Insert a live cell in the hashtable
void insert(cell * table, int x, int y, int z, int SIZE_TABLE)
{
        unsigned long key = gen_key(x, y, z);
        int hash = hashcode(key, SIZE_TABLE);
        cell new_cell =  {x, y, z, 1, NULL};
        
        
        if (table[hash].key == -1)
        {
                table[hash] = new_cell;
        }
        else
        {
                new_cell.next = malloc(sizeof(cell));
                *new_cell.next = table[hash];
                table[hash] = new_cell;
        }
        
}

//This function search in the hashtable the given cell and give back the cell status (0 or 1)
int search(cell * table, int x, int y, int z, int SIZE_TABLE)
{
        unsigned long key = gen_key(x,y,z);
        int hash = hashcode(key, SIZE_TABLE);
        
        if (table[hash].key == -1)
        {
                return 0;
        }
        else
        {
                cell current = table[hash];
                while (current.next != NULL && (current.x != x || current.y != y || current.z != z)){
                        current = *current.next;
                }
                
                if (current.x == x && current.y == y && current.z == z)
                        return 1;
                else
                        return 0;
                
        
        }
}


//This function count the number of alive neighbours of a given dead cell

int num_alive_neighbours_d(cell * table, cell * aux_table, int x, int y, int z, int SIZE_TABLE, int SIZE_CUBE, omp_lock_t * lock_aux)
{
    int num_live = 0;
    
    int pos = hashcode(gen_key(x,y,z), SIZE_TABLE);
    
    omp_set_lock(&(lock_aux[pos]));
    int find = search(aux_table, x, y, z, SIZE_TABLE);
    omp_unset_lock(&(lock_aux[pos]));
    
    if(!find) {   
	    num_live += search(table, mod((x+1),SIZE_CUBE), y, z, SIZE_TABLE);
	    num_live += search(table, mod((x-1),SIZE_CUBE), y, z, SIZE_TABLE);
	    num_live += search(table, x, mod((y+1),SIZE_CUBE), z, SIZE_TABLE);
	    num_live += search(table, x, mod((y-1),SIZE_CUBE), z, SIZE_TABLE);
	    num_live += search(table, x, y, mod((z+1),SIZE_CUBE), SIZE_TABLE);
	    num_live += search(table, x, y, mod((z-1),SIZE_CUBE), SIZE_TABLE);

	    if(num_live==2 || num_live==3){
	        omp_set_lock(&(lock_aux[pos]));
		insert(aux_table, x, y, z, SIZE_TABLE); 
		omp_unset_lock(&(lock_aux[pos]));

	    }
    }

    return num_live;
}


//This function count the number of alive neighbours of a given alive cell
int num_alive_neighbours_a(cell * table, cell * aux_table, int x, int y, int z, int SIZE_TABLE, int SIZE_CUBE, omp_lock_t * lock_aux)
{
    int num_live = 0;
  
    if(!search(table, mod((x+1),SIZE_CUBE), y, z, SIZE_TABLE)) 
        num_alive_neighbours_d(table, aux_table, mod((x+1),SIZE_CUBE), y, z, SIZE_TABLE, SIZE_CUBE, lock_aux);
    else num_live++;
    
    if(!search(table, mod((x-1),SIZE_CUBE), y, z, SIZE_TABLE)) 
        num_alive_neighbours_d(table, aux_table, mod((x-1),SIZE_CUBE), y, z, SIZE_TABLE, SIZE_CUBE, lock_aux);
    else num_live++;
    
    if(!search(table, x, mod((y+1),SIZE_CUBE), z, SIZE_TABLE)) 
        num_alive_neighbours_d(table, aux_table, x, mod((y+1),SIZE_CUBE), z, SIZE_TABLE, SIZE_CUBE, lock_aux);
    else num_live++;
    
    if(!search(table, x, mod((y-1),SIZE_CUBE), z, SIZE_TABLE)) 
        num_alive_neighbours_d(table, aux_table, x, mod((y-1),SIZE_CUBE), z, SIZE_TABLE, SIZE_CUBE, lock_aux);
    else num_live++;
    
    if(!search(table, x, y, mod((z+1),SIZE_CUBE), SIZE_TABLE)) 
        num_alive_neighbours_d(table, aux_table, x, y, mod((z+1),SIZE_CUBE), SIZE_TABLE, SIZE_CUBE, lock_aux);
    else num_live++;
    
    if(!search(table, x, y, mod((z-1),SIZE_CUBE), SIZE_TABLE)) 
        num_alive_neighbours_d(table, aux_table, x, y, mod((z-1),SIZE_CUBE), SIZE_TABLE, SIZE_CUBE, lock_aux);
    else num_live++;

    return num_live;
}

// This function frees the memory in the hashtable and set everything to -1 again
void free_list(cell * table, int SIZE_TABLE)
{   
    int temp;    
    #pragma omp parallel for private(temp)
    for(temp=0; temp<SIZE_TABLE; temp++){
        
        cell test;
        cell current;
        current = table[temp];
	    if(current.key!=-1){
	    	while (current.next != NULL) {
	    	        test=current;
	    	        current=*current.next;
	    	        free(test.next);
	  	    }   
        }
    }
    
    memset(table, -1, SIZE_TABLE*sizeof(cell));
}


//Scan the hash table and insert the cells, in order, in a linked list
void ordered_list(cell * table, cell ** order,  int SIZE_TABLE, int SIZE_CUBE)
{   
    
   omp_lock_t lock_list[SIZE_CUBE];
   int h;
   for (h=0; h<SIZE_CUBE; h++)
        omp_init_lock(&(lock_list[h]));

    int temp = 0;
    
    #pragma omp parallel for private(temp)
    for(temp=0; temp<SIZE_TABLE; temp++){
        cell * current = &table[temp];
	    if(current->key != -1){	                
		    while (current->next != NULL) {
		    
		          omp_set_lock(&(lock_list[current->x]));  
		    
		          if (order[current->x] == NULL){
		            cell * new_head = malloc(sizeof(cell));
		            new_head->next = NULL;
		            new_head->x = current->x;
		            new_head->y = current->y;
		            new_head->z = current->z;
		            new_head->key = current->key;
	                order[current->x] = new_head;
	                    
	                omp_unset_lock(&(lock_list[current->x]));
	                    
	                current = current->next;        
	              } else {
		                cell * head = order[current->x];
                        head = push(head, current->x, current->y, current->z);
                        order[current->x] = head;
                        omp_unset_lock(&(lock_list[current->x]));
                        current = current->next;
                  }
                      
            }
		
	       omp_set_lock(&(lock_list[current->x]));
	       
		      if (order[current->x] == NULL){
		        cell * new_head = malloc(sizeof(cell));
		        new_head->next = NULL;
		        new_head->x = current->x;
		        new_head->y = current->y;
		        new_head->z = current->z;
		        new_head->key = current->key;
	            order[current->x] = new_head;
	                
	            omp_unset_lock(&(lock_list[current->x]));
	                       
	          } else {
		                cell * head = order[current->x];
                        head = push(head, current->x, current->y, current->z);
                        order[current->x] = head;
                        omp_unset_lock(&(lock_list[current->x]));
              }
    	}
    }
}


//Accessory function to support the push of new elements in the ordered linked list
cell * push(cell * head, int x, int y, int z) {     
    cell * current = head;
    
    if ((x < current->x) || (x == current->x && y < current->y) || (x == current->x && y == current->y && z < current->z))
    {
        cell * new_head = malloc(sizeof(cell));
        new_head->next = head;
        new_head->x = x;
        new_head->y = y;
        new_head->z = z;
        new_head->key = 1;
        
        return new_head;
    }

    
    while (current->next != NULL && ((current->next->x < x) || (current->next->x == x && current->next->y < y) || (current->next->x == x && current->next->y == y && current->next->z < z))) {
        current = current->next;
    }
    
    if (x == current->x && y==current->y && z==current->z)
        return head;
    
        
    if (current->next == NULL)
    {
        current->next = malloc(sizeof(cell));
        current->next->x = x;
        current->next->y = y;
        current->next->z = z;
        current->next->next = NULL;
    }
    else
    {
        cell * tmp = current->next;
        current->next = malloc(sizeof(cell));
        current->next->x = x;
        current->next->y = y;
        current->next->z = z;
        current->next->next = tmp;
    }
    
    return head;
}


//Print the cells of the given linked list
void print_list(cell ** order, int SIZE_CUBE)
{
     int i;
     for (i = 0; i < SIZE_CUBE; i++){
        if (order[i] != NULL){
                cell * current =  order[i];
                while (current->next != NULL)
                {
                        printf("%d %d %d\n", current->x, current->y, current->z);
                        current = current->next;
                }
        
        printf("%d %d %d\n", current->x, current->y, current->z);
        }
     }
}


//Accessory function: count the number of empty buckets in the hash table
int void_cells(cell * table, int SIZE_TABLE){
        int count = 0;
        int i;
        for (i = 0; i < SIZE_TABLE; i++){
                if (table[i].key == -1){
                        count ++;
                }
        }

        return count;
}




