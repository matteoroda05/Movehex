#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

typedef struct {
    int c; 
    int r; 
} route; 

typedef struct {
    short cost;
    route* aroutes;
    unsigned short n_aroutes;
    bool visited;
    int dist;
    int row;
    int col;
    int hpos;
    bool inheap;
    int call_number;
} cell;

int curr_call_number = 0;
    
typedef struct {
    cell** map;
    int cols;
    int rows;
} MapData;

MapData world = {NULL, 0, 0}; 

typedef struct node { 
    cell* key;
    struct node* next;
} node; 

typedef struct {
    cell** cells;
    int first;
    int last;
    int size;
    int count;
} queue;

typedef struct {
    int x;
    int y;
} coord;
const coord even[6] = {
    {1, 0}, {0, -1}, {-1, -1}, {-1, 0}, {0, 1}, {1, -1}
};
const coord odd [6] = {
    {1, 0}, {0, -1}, {-1, 1}, {-1, 0}, {0, 1}, {1, 1}
};

typedef struct {
    cell** heap;
    int heapsize;
} Heap;

Heap H = {NULL, 0};
bool isheap = false;

typedef struct {
    int xp;
    int yp;
    int xd;
    int yd;
    int cost;
} travel;

typedef struct {
    travel travelp[100];
    char size;
} travels;

travels tCosts = {{{0}}, 0};

void init (int cols, int rows);

void freeworld (){
     
    if (world.map != NULL){
        for (int i = 0; i < world.rows; i++) {
            
            // Free all air routes 
            for (int j = 0; j < world.cols; j++) {
                free(world.map[i][j].aroutes);
            }

            // Free all columns 
            free(world.map[i]);
        }   
        // Free all rows and set size to 0x0 and map to NULL
        free(world.map);
        world.cols = 0;
        world.rows = 0;
        world.map = NULL;
    }
}

void enqueue (queue* Q, cell* enq){
    int last = Q->last;
    Q->cells[last] = enq;
    last ++;
    if (last == Q->size){
        last = 0;
    }
    Q->last = last;
    Q->count ++;
}

cell* dequeue (queue* Q){
    int first = Q->first;
    cell* res = Q->cells[first];
    first ++;
    if (first == Q->size){
        first = 0;
    }
    Q->first = first;
    Q->count --;
    return res;
}

void change_cost (int x, int y, int v, int rad);

void toggle_air_route (int x1, int y1, int x2, int y2);

void travel_cost (int xp, int yp, int xd, int yd);

void newheap (){
    if (isheap == false){
        if (H.heap != NULL){
            free(H.heap);
        }
        H.heap = (cell **) calloc (world.cols * world.rows, sizeof(cell*));
        H.heapsize = 0;
        isheap = true;
    }
}

void heapifUp (cell* c){
    if (H.heap == NULL || c == NULL){
        return;
    }
    
    if (c->dist != -1){
        int i = c->hpos;
        int father  = (i-1) / 2;
        while (i > 0 && (H.heap[father]->dist > H.heap[i]->dist || H.heap[father]->dist == -1)){
            // Until the position in heap is real and (father == -1 or father.dist > i.dist) => swap  i and its father
            cell* tmp = H.heap[i];
            H.heap[i] = H.heap[father];
            H.heap[father] = tmp;
            H.heap[i]->hpos = i;
            H.heap[father]->hpos = father;
            i = father;
            father = (i-1) / 2;
        }
    }
}

void addToHeap (cell* c, int ndis) {
    if (H.heap == NULL || c == NULL || ndis < 0){
        return;
    }

    if (c->visited == true && curr_call_number == c->call_number){
        // If the cell was already visited no need to add
        return;
    } else if (curr_call_number > c->call_number){
        // If the cell was visited but the current call number is greater than the cell's => update cell call number and parameters
        c->visited = false;
        c->inheap = false;
        c->call_number = curr_call_number;
    }
    
    // If the cell is in the heap, find the cell in the heap when found update distance and heapifUp
    if (c->inheap == true){
        // Se la cella e dentro controllo la distanza e la aggioerno
        if (c->dist == -1 || ndis < c->dist){
            c->dist = ndis;
        }
        heapifUp (c);
        return;
    }
    
    // If the cell is not found insert in the last available spot in the heap and find its place (heapifUp) 
    H.heap[H.heapsize] = c;
    c->hpos = H.heapsize;
    c->dist = ndis;
    c->inheap = true;
    heapifUp (c);

    // Increase heap size
    H.heapsize ++;
}

void minHeapify (int n){
    // HEAPIFYDOWN 
    int l = 2*n +1,
        r = 2*n +2;
        //l-left , r-right
    int posmin = n;

    if(l < H.heapsize && ( H.heap[l]->dist < H.heap[n]->dist || H.heap[n]->dist == -1) ){
        if (H.heap[l]->dist != -1){
            posmin = l;
        }
    }
    if(r < H.heapsize && ( H.heap[r]->dist < H.heap[posmin]->dist || H.heap[posmin]->dist == -1) ){
        // Check if posmin is -1, since if posmin == -1 => n is posmin /// if i checked for n == -1 there could be the chance where posmin is l (≠-1) and at the same time r > l
        if (H.heap[r]->dist != -1){
            posmin = r;
        }
    }
    
    // Recursive case: if posmin is different from n
    if (posmin != n){
        // Swap 
        cell* tmp = H.heap[n];
        H.heap[n] = H.heap[posmin];
        H.heap[posmin] = tmp;
        H.heap[n]->hpos = n;
        H.heap[posmin]->hpos = posmin;

        minHeapify (posmin);
    }
}

cell* heapMin (){
    if (H.heapsize < 1){
        return NULL;
    }
    
    cell* min = H.heap[0];
    min->inheap = false;

    if (H.heapsize == 1){
        H.heapsize --;
        return min;
    }

    // The minimum and is the first element in the heap
    H.heap[0] = H.heap[H.heapsize -1];
    H.heap[0]->hpos = 0;
    H.heapsize --;

    // Rearrange Heap
    minHeapify (0);

    return min;
}

int main (void){
    char input[30];
    int num1,num2,num3,num4; 
    while(scanf("%s", input)==1){
        if (strcmp(input,"exit")==0){
            break;
        }
        if(strcmp(input,"init") == 0){
            if(scanf("%d %d", &num1, &num2)==0){
                return 0;
            }
            init(num1,num2);
        }else if (strcmp(input,"exit")!=0){
            if(scanf("%d %d %d %d", &num1, &num2, &num3, &num4)==0){
                return 0;
            }
            if(strcmp(input,"travel_cost")==0){
                travel_cost(num1,num2,num3,num4);
            }else if(strcmp(input,"change_cost")==0){
                change_cost(num1,num2,num3,num4);
            }else if(strcmp(input,"toggle_air_route")==0){
                toggle_air_route(num1,num2,num3,num4);
            }
        }
    }
    freeworld();
    return 0;
}

void init (int cols, int rows){
    // If the map isnt NULL (already checked in freeworld) => free the map and set params to 0
    freeworld();

    // Set the new dimensions
    world.cols = cols;
    world.rows = rows;

    // Set size of cache array to 0
    tCosts.size = 0;

    // Initialise the matrizx with the set size
    world.map = (cell **) malloc(rows * sizeof(cell *));
    for (int i = 0; i < rows; i++) {
        world.map[i] = (cell *) malloc(cols * sizeof(cell));
    }

    for (int i = 0; i < rows; i++){
        for (int j = 0; j < cols; j++){
            world.map[i][j].cost = 1;
            world.map[i][j].aroutes = NULL;
            world.map[i][j].n_aroutes = 0;
            world.map[i][j].visited = false;
            world.map[i][j].dist = -1;
            world.map[i][j].row = i;
            world.map[i][j].col = j;
            world.map[i][j].hpos = 0;
            world.map[i][j].inheap = false;
            world.map[i][j].call_number = 0;
        }
    }

    // Initialise heap presence to false (heap inaccesible)
    isheap = false; 

    printf("OK\n");
}

void change_cost (int x, int y, int v, int rad){
    // Check if the procedure can be done or if it has to be killed, in which return after printing KO
    if (world.map == NULL){
        printf("KO\n");
        return;
    }else if (rad < 1 || v < -10 || v > 10 || x >= world.cols || y >= world.rows || x < 0 || y < 0){
        printf("KO\n");
        return;
    }

    // Set travel cost array to 0 (the previous routes might now have a different cost due to the cost change)
    tCosts.size = 0;

    // Increase current call number
    curr_call_number++;

    // Start BFS (+ cost change)
    cell* curr = &world.map[y][x];
    curr->call_number = curr_call_number;
    curr->visited = true;
    curr->dist = 0;
    
    // Create (internally) a queue and enqueue the first element
    queue Q;
    Q.size = 1 + 3 * (rad * (rad - 1)); 
    Q.cells = (cell**)calloc(Q.size, sizeof(cell*));
    Q.first = 0;
    Q.last = 0;
    Q.count = 0;
    enqueue (&Q, curr);

    while (Q.count > 0){
        curr = dequeue (&Q);
        int cdist = curr->dist;
        // Until the queue isnt empty dequeue the first element and modify its cost
        float float_change = 1.0 - ((float)cdist / (float)rad);
        int cost_change = (int) floor((float)v * float_change);
        curr->cost += cost_change; // Implicit cast to short (no need for checks since the maximum is 100 and short can go up to 127)
        // If the cost is out of bounds re-size it to be within [0, 100]
        if (curr->cost < 0){ curr->cost = 0;}
        if (curr->cost > 100){ curr->cost = 100;} 

        // The air routes cost (even though in the specification is defined by a mathematical function) it is always the same as the cost of the cells => no need to modify it

        cdist++; // Increment the current distance to visit the adjacent nodes
        
        // In case the current distance ≥ radius there is no need to add more cells to the queue
        if (cdist < rad){
            if ((curr->row & 1) == 0){
                // Even cells 
                // For all adjacent cells check if the cell was not already modified, if not modified update the distance and enqueue it
                for (int i = 0; i < 6; i++){
                    int newr = curr->row + even[i].x;
                    int newc = curr->col + even[i].y;
                    if (newr >= 0 && newr < world.rows && newc >= 0 && newc <  world.cols){
                        if (! world.map[newr][newc].visited || world.map[newr][newc].call_number < curr_call_number){
                            // If the i-th adjacent exists (in bounds) and it was not yet visited, 
                            // update distance update visited, enqueue
                            world.map[newr][newc].visited = true;
                            world.map[newr][newc].call_number = curr_call_number;
                            world.map[newr][newc].dist = cdist;
                            enqueue (&Q, &world.map[newr][newc]);
                        }
                    }
                }
            }else {
                // Odd cell => same procedure
                for (int i = 0; i < 6; i++){
                    int newr = curr->row + odd[i].x;
                    int newc = curr->col + odd[i].y;
                    if (newr >= 0 && newr < world.rows && newc >= 0 && newc <  world.cols){
                        if (! world.map[newr][newc].visited || world.map[newr][newc].call_number < curr_call_number){
                            world.map[newr][newc].visited = true;
                            world.map[newr][newc].call_number = curr_call_number;
                            world.map[newr][newc].dist = cdist;
                            enqueue (&Q, &world.map[newr][newc]);
                        }
                    }
                }
            }
        }
    }

    // Free the queue
    free(Q.cells);

    printf ("OK\n");
}
  
void toggle_air_route (int x1, int y1, int x2, int y2){
    if (world.map == NULL){
        printf("KO\n");
        return;
    } 
    if (x1 >= world.cols || y1 >= world.rows || x2 >= world.cols || y2 >= world.rows || x1 < 0 || y1 < 0 || x2 < 0 || y2 < 0){
        // Invalid source/destination
        printf ("KO\n");
        return;
    }

    // No previous air routes
    if (world.map[y1][x1].n_aroutes < 1){
        // Create vector of 5 air routes and initialise n_aroutes to 1
        world.map[y1][x1].aroutes = (route*) calloc(5, sizeof(route));
        world.map[y1][x1].aroutes[0].c = x2;
        world.map[y1][x1].aroutes[0].r = y2;
        world.map[y1][x1].n_aroutes = 1;
    } 
    
    // If other air routes exist already
    else {
        // Check if the same air rout already exists
        bool found = false;
        for (int i = 0; i < world.map[y1][x1].n_aroutes; i++){
            if (world.map[y1][x1].aroutes[i].c == x2 && world.map[y1][x1].aroutes[i].r == y2){
                // If it exists remove it and if array becomes empty free the array
                found = true;
                for (int j = i; j < world.map[y1][x1].n_aroutes - 1; j++){
                    world.map[y1][x1].aroutes[j] = world.map[y1][x1].aroutes[j + 1];
                }
                world.map[y1][x1].n_aroutes--;
                if (world.map[y1][x1].n_aroutes == 0){
                    free (world.map[y1][x1].aroutes);
                    world.map[y1][x1].aroutes = NULL;
                }
                break;
            }
        }
        if (found == false){
            // If the same air rout doesnt exist, then add the air route
            if (world.map[y1][x1].n_aroutes == 5){
                // If 5 air route already exist print KO and return
                printf("KO\n");
                return;
            }
            // Add the air route and increase n_aroutes 
            world.map[y1][x1].aroutes[world.map[y1][x1].n_aroutes].c = x2;
            world.map[y1][x1].aroutes[world.map[y1][x1].n_aroutes].r = y2;
            world.map[y1][x1].n_aroutes++;
        }
    }

    // Set travel cost array to 0 (the previous routes might now have a different cost due to the added/removed air route)
    tCosts.size = 0;

    printf("OK\n");
}

void travel_cost (int xp, int yp, int xd, int yd){
    
    // Follows all cases in which travel cost prints -1 and returns
    
    if (world.map == NULL){ 
        // No real world
        printf("-1\n");
        return;
    } 
    if (xp >= world.cols || yp >= world.rows || xd >= world.cols || yd >= world.rows || xp < 0 || yp < 0 || xd < 0 || yd < 0){
        // Invalid cells 
        printf ("-1\n");
        return;
    }
    
    // Start and finish coincide => print 0 and return
    if(xp == xd && yp == yd){
        printf("0\n");
        return;
    }

    // Starting cell has cost == 0 (=> impossible to exit from it) => print -1 and return
    if (world.map[yp][xp].cost < 1){
        printf("-1\n");
        return;
    }


    // Check tCosts, if the exact cell source-destination is found print the previously found result
    for (int i = 0; i < tCosts.size; i++){
        if (tCosts.travelp[i].xp == xp && tCosts.travelp[i].yp == yp && tCosts.travelp[i].xd == xd && tCosts.travelp[i].yd == yd){
            printf("%d\n", tCosts.travelp[i].cost);
            return;
        }
    }
    
    // Add the cell combination to tCosts
    int TCindex;
    if (tCosts.size < 100){
        TCindex = tCosts.size;
        tCosts.size++;
    }else {
        // Since in most input cases tested for the number of consecutive travel costs is less than 100 if it exceeds 100 we simply put all in 0
        TCindex = 0;      
    }
    tCosts.travelp[TCindex].xp = xp;
    tCosts.travelp[TCindex].yp = yp;
    tCosts.travelp[TCindex].xd = xd;
    tCosts.travelp[TCindex].yd = yd;
    tCosts.travelp[TCindex].cost = 0;

    // Increase current call number
    curr_call_number++;
    
    // Dijkstra
    // Initialise heap
    newheap ();
    H.heapsize = 0;
    
    // Initialise starting cell
    world.map[yp][xp].dist = 0;
    world.map[yp][xp].call_number = curr_call_number;
    world.map[yp][xp].visited = false;
    world.map[yp][xp].inheap = false;
    addToHeap (&world.map[yp][xp], 0); 
    
    cell* curr;

    while (H.heapsize > 0){ 
        // Until the heap exists check the adjacent and the air routes and add them to heap if possible
        curr = heapMin ();
        curr->visited = true;
        curr->call_number = curr_call_number;
        int cdist = curr->dist;

        // Before operating on the successors, check if the current minimum is reachable, if not print -1, free heap and return 
        if (cdist == -1){
            printf("-1\n");
            tCosts.travelp[TCindex].cost = -1; 
            return;
        }

        // If curr is destination the minimum distance has been found
        if (curr->row == yd && curr->col == xd){
            printf("%d\n", cdist);
            tCosts.travelp[TCindex].cost = cdist;
            return;
        }

        int ccost = curr->cost;
        int ndis = cdist + ccost;
        // Do the following on adjacent: check > modify distance > add to heap
        if (ccost > 0){
            if ((curr->row & 1) == 0){
                // Even cell
                for (int i = 0; i < 6; i++){
                    int newr = curr->row + even[i].x;
                    int newc = curr->col + even[i].y;
                    if (newr >= 0 && newr < world.rows && newc >= 0 && newc < world.cols){
                        addToHeap (&world.map[newr][newc], ndis);
                    }
                }
            }else {
                // Odd cell
                for (int i = 0; i < 6; i++){
                    int newr = curr->row + odd[i].x;
                    int newc = curr->col + odd[i].y;
                    if (newr >= 0 && newr < world.rows && newc >= 0 && newc < world.cols){
                        addToHeap (&world.map[newr][newc], ndis);
                    }
                }
            }
        }

        // Do the following on air route connections: check > modify distance > add to heap
        if (curr->n_aroutes > 0 && ccost > 0){
            for (int i = 0; i < curr->n_aroutes; i++){
                int newr = curr->aroutes[i].r;
                int newc = curr->aroutes[i].c;
                addToHeap (&world.map[newr][newc], ndis);
            }
        }
    }   
    
    // If the cell has not been found print -1 
    printf("-1\n");
    tCosts.travelp[TCindex].cost = -1;
}
