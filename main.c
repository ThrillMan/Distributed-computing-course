#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>

#define REQUEST_TAG 1
#define OK_TAG 2
#define RELEASE_TAG 3
#define PAR_REQUEST_TAG 4
#define PAR_ACCEPT_TAG 5
#define PAR_DENY_TAG 6
#define PAR_END_TAG 7

enum State { RELEASED, WANTED, HELD };

typedef struct {
    int rank;
    int size;
    int clock;
    int req_clock;
    enum State state;
    int *deferred_queue;
    int deferred_count;
    bool is_artist;
    int *pair_counts; // dla A
    bool paired;      // dla G
} Process;

int max(int a, int b) {
    return a > b ? a : b;
}

void init_process(Process *p, int rank, int size, int a_num) {
    p->rank = rank;
    p->size = size;
    p->clock = 0;
    p->req_clock = 0;
    p->state = RELEASED;
    p->deferred_queue = malloc(size * sizeof(int));
    p->deferred_count = 0;
    p->is_artist = (rank < a_num);
    
    if (p->is_artist) {
        p->pair_counts = calloc(size - a_num, sizeof(int));
    } else {
        p->paired = false;
    }
}

// artysci
void artist_logic(Process *p, int a_num) {
    MPI_Status status;
    
    while (1) {
        sleep(rand() % 3 + 1);
        
        // chce wejsc do sekcji krytycznej
        p->state = WANTED;
        p->clock++;
        p->req_clock=p->clock;
        
        printf("A%d: Zadanie dostepu (clock: %d)\n", p->rank, p->clock);
        
        // wysylanie zadan do pozostalych procesow A
        for (int dest = 0; dest < a_num; dest++) {
            if (dest != p->rank) {
                MPI_Send(&p->clock, 1, MPI_INT, dest, REQUEST_TAG, MPI_COMM_WORLD);
            }
        }
        
        int ok_received = 0;
        while (ok_received < a_num-1) {
            int flag;
            MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
            
            if (flag) {
                if(p->rank != status.MPI_SOURCE){
                    if (status.MPI_TAG == REQUEST_TAG) {

                        int incoming_clock;
                        MPI_Recv(&incoming_clock, 1, MPI_INT, status.MPI_SOURCE, REQUEST_TAG, MPI_COMM_WORLD, &status);
                        p->clock = max(p->clock, incoming_clock) + 1;
                        
                        
                        if ((p->req_clock < incoming_clock)){
                            printf("---A%d: Odraczam przez czas A%d\n", p->rank,status.MPI_SOURCE);
                            p->deferred_queue[p->deferred_count++] = status.MPI_SOURCE;
                        }
						
                        else if((p->req_clock == incoming_clock)&&(p->rank<status.MPI_SOURCE)){
                            printf("---A%d: Odraczam przez ID A%d\n", p->rank,status.MPI_SOURCE);
                            p->deferred_queue[p->deferred_count++] = status.MPI_SOURCE;
							
                        } else {
                            MPI_Send(&p->clock, 1, MPI_INT, status.MPI_SOURCE, OK_TAG, MPI_COMM_WORLD);
                        }
                    } else if (status.MPI_TAG == OK_TAG) {
                        int temp;
                        MPI_Recv(&temp, 1, MPI_INT, status.MPI_SOURCE, OK_TAG, MPI_COMM_WORLD, &status);
                        ok_received++;
                        printf("A%d otrzymalem zgode od: A%d\n",p->rank,status.MPI_SOURCE);
                        printf("A%d mam zgode: %d\n",p->rank,ok_received);

                    }
                }
            }
        }
        
        // wejscie do sekcji krytycznej
        p->state = HELD;
        printf("!@# A%d: W sekcji krytycznej\n", p->rank);
        

        for (int g = a_num; g < p->size; g++) {
            if (p->pair_counts[g - a_num] == 0) {
                printf("A%d: Probuje dobrac sie w pare z G%d\n", p->rank, g);
                MPI_Send(&p->rank, 1, MPI_INT, g, PAR_REQUEST_TAG, MPI_COMM_WORLD);
                
                int response;
                MPI_Recv(&response, 1, MPI_INT, g, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                
                if (status.MPI_TAG == PAR_ACCEPT_TAG) {
                    printf("A%d-G%d: Utworzono pare\n", p->rank, g);
                    p->pair_counts[g - a_num]++;
                    sleep(1);
                    
                    // zakonczenie pary
                    MPI_Send(&p->rank, 1, MPI_INT, g, PAR_END_TAG, MPI_COMM_WORLD);
                    break;
                }
            }
        }
        sleep(rand() % 3 + 1);
        // wyjscie z sekcji krytycznej
        p->state = RELEASED;
        printf("#@! A%d: Opuscilem sekcje krytyczna\n", p->rank);
        for (int i = 0; i < p->deferred_count; i++) {
            MPI_Send(&p->clock, 1, MPI_INT, p->deferred_queue[i], OK_TAG, MPI_COMM_WORLD);
        }
        p->deferred_count = 0;
    }
}

// procesy G (geoinz)
void engineer_logic(Process *p) {
    MPI_Status status;
    
    while (1) {
        int request;
        MPI_Recv(&request, 1, MPI_INT, MPI_ANY_SOURCE, PAR_REQUEST_TAG, MPI_COMM_WORLD, &status);
        
        if (!p->paired) {
            p->paired = true;
            MPI_Send(&p->rank, 1, MPI_INT, status.MPI_SOURCE, PAR_ACCEPT_TAG, MPI_COMM_WORLD);
            
            MPI_Recv(&request, 1, MPI_INT, status.MPI_SOURCE, PAR_END_TAG, MPI_COMM_WORLD, &status);
            p->paired = false;
            printf("G%d: Zakonczylem pare z A%d\n", p->rank, status.MPI_SOURCE);
        } else {
            MPI_Send(&p->rank, 1, MPI_INT, status.MPI_SOURCE, PAR_DENY_TAG, MPI_COMM_WORLD);
        }
    }
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    srand(rank * time(NULL));
    
    int a_num = size / 2;
    Process p;
    init_process(&p, rank, size, a_num);
    printf("Ilosc art:%d Ilosc inz:%d\n",a_num,size-a_num);
    
    if (p.is_artist) {
        artist_logic(&p, a_num);
    } else {
        engineer_logic(&p);
    }
    
    MPI_Finalize();
    return 0;
}