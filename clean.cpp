#include <iostream>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <bits/stdc++.h>
#include <sys/sem.h>
using namespace std;
#define shmkey "files/shmkey"
#define psemkey "files/psemkey"
#define csemkey "files/csemkey"
#define msemkey "files/msemkey"

int mutex_sem, con_sem, prod_sem, shmid;
key_t key;

int delete_segment(int seg_id){
    if ((shmctl(seg_id,IPC_RMID,0))==-1){
    std::cout<<" ERROR(C++)with shmctl(IPC_RMID): "<<strerror(errno)<<std::endl;
    return -1;
    }else//on success
        return 0;
}

void memDel(){

    struct shmid_ds shm_info;
    struct shmid_ds shm_segment;
    int max_id = shmctl(0,SHM_INFO,&shm_info);
    if (max_id>=0){
        for (int i=0;i<=max_id;++i) {
                int shm_id = shmctl(i , SHM_STAT , &shm_segment);
                if (shm_id<=0)
                    continue;
                else if (shm_segment.shm_nattch==0){
                    delete_segment(shm_id);
                }
        }
    }
    return;
}

int main()
{	
	//signal(SIGINT, handler_function);
    // ftok to generate unique key
	key = ftok(shmkey,65);
  
    // shmget returns an identifier in shmid
    int shmid = shmget(key,0,0);
    if ( shmid < 0 ) {
    	cout << "Shared Memory doesn't Exist" << endl;
    }
    // Destroy the shared memory  
    shmctl(shmid,IPC_RMID,NULL);
    
    // ftok to generate unique key for Mutex
	if ((key = ftok(msemkey,65)) == -1) {
   		perror ("ftok Mutex"); exit(1);
	}
	// Create  Mutex Semaphore
	if ((mutex_sem = semget (key, 1, 0666 | IPC_CREAT)) == -1) {
    	perror ("semget Mutex"); exit (1);
	}
	// ftok to generate unique key for consumer
	if ((key = ftok(csemkey,65)) == -1) {
   		perror ("ftok Con"); exit(1);
	}
	// Create  consumer Semaphore ( Indicates number of available items on the buffer )
	if ((con_sem = semget (key, 1, 0666 | IPC_CREAT)) == -1) {
    	perror ("semget Con"); exit (1);
	}
	// ftok to generate unique key for producer
	if ((key = ftok(psemkey,65)) == -1) {
   		perror ("ftok Prod"); exit(1);
	}
	// Create  producer Semaphore ( Indicates number of available places on the buffer )
	if ((prod_sem = semget (key, 1, 0666 | IPC_CREAT)) == -1) {
    	perror ("semget Prod"); exit (1);
	}
    
    
    // remove semaphores
    if (semctl (mutex_sem, 0, IPC_RMID) == -1) {
        perror ("semctl IPC_RMID"); exit (1);
    }
    if (semctl (prod_sem, 0, IPC_RMID) == -1) {
        perror ("semctl IPC_RMID"); exit (1);
    }
    if (semctl (con_sem, 0, IPC_RMID) == -1) {
        perror ("semctl IPC_RMID"); exit (1);
    }
	memDel();
    return 0;
}
