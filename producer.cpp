#include <sys/ipc.h>
#include <sys/shm.h>
#include <bits/stdc++.h>
#include <unistd.h>
#include <time.h>
#include <sys/sem.h>
#include <signal.h>
using namespace std;

// Define files pathes used for keys
#define shmkey "files/shmkey"
#define psemkey "files/psemkey"
#define csemkey "files/csemkey"
#define msemkey "files/msemkey"

// GLobal variables used in most functions
char* str;
int EXITVAL = 0;
int mutex_sem, con_sem, prod_sem, shmid;
key_t key;
struct sembuf asem [1];

// Functions used in main
void handler_function(int sig);
void initMemSem(); 
void end();

int main(int argc, char *argv[])
{
	// Catch ctrl c signal
	signal(SIGINT, handler_function);
	// struct to get time in seconds
	struct timespec ts;
	char buff[100];
	double mean, sdev;
	// get arguments
	string tmp = argv[1];
	mean = stod(argv[2]);
	sdev = stod(argv[3]);
	//int BOUND = stoi(argv[4]); // Useless for my code
	sdev *= sdev;
	// convert sleep time from ms to seconds
	double slp = stod(argv[4])/1000;
	// generate seed using time
	unsigned seed = chrono::system_clock::now().time_since_epoch().count();
	// initiate generater function using the seed
	default_random_engine generator(seed);
	// Use normal distribution to get prices
	normal_distribution<double> distribution(mean,sdev);
	// initialize semaphore buffer struct with zeros
	asem [0].sem_num = 0;
	asem [0].sem_op = 0;
	asem [0].sem_flg = 0;
	// initialize share memory and semaphores
	initMemSem();
	// infinite loop to run the producer unitl ctrl C is hit
	while(true) {
		// Check if exit is called
		if ( EXITVAL ) end();
		// get price from distribution
		double price = distribution(generator);
		price = abs(price);
		// Get time in gmt and add 2 hours to be in local time
		clock_gettime( CLOCK_REALTIME ,&ts );
		ts.tv_sec += 7200;
		strftime(buff, sizeof buff, "%D %T", gmtime(&ts.tv_sec));
		fprintf(stderr,"[ %s.%03ld ] %s: generatine new value %.2lf\n",buff, ts.tv_nsec,tmp.c_str(),price);
		// Check if you can take an item from the buffer
		asem[0].sem_op=-1;
		if (semop (prod_sem, asem, 1) == -1) {
	   		perror ("semop: con_sem"); exit (1);
       		}
       		// Try to get mutex time and flag
       		clock_gettime( CLOCK_REALTIME ,&ts );
		ts.tv_sec += 7200;
		strftime(buff, sizeof buff, "%D %T", gmtime(&ts.tv_sec));
		fprintf(stderr,"[ %s.%03ld ] %s: Trying to Get MUTEX of share Buffer\n",buff, ts.tv_nsec,tmp.c_str());
		// Get Mutual Execlusion
		asem[0].sem_op=-1;
		if (semop (mutex_sem, asem, 1) == -1) {
	   		perror ("semop: mutex_sem"); exit (1);
       		}
		// Critical Section Begin
		string s = tmp;
		s = s + ',' + to_string(price);
		s = str + s + '\n';
		strcpy(str, s.c_str());
		cout << s << endl;
		// End of Critical Section
		clock_gettime( CLOCK_REALTIME ,&ts );
		ts.tv_sec += 7200;
		strftime(buff, sizeof buff, "%D %T", gmtime(&ts.tv_sec));
		fprintf(stderr,"[ %s.%03ld ] %s: Placing %.2lf on buffer\n",buff, ts.tv_nsec,tmp.c_str(),price);
		// Release Mutual Execlusion
		asem[0].sem_op=1;
		if (semop (mutex_sem, asem, 1) == -1) {
	   		perror ("semop: mutex_sem"); exit (1);
       		}
       		// Add item on buffer
        	asem[0].sem_op=1;
		if (semop (con_sem, asem, 1) == -1) {
	   		perror ("semop: prod_sem"); exit (1);
        	}
        	if ( EXITVAL ) end();
        	clock_gettime( CLOCK_REALTIME ,&ts );
		ts.tv_sec += 7200;
		strftime(buff, sizeof buff, "%D %T", gmtime(&ts.tv_sec));
		fprintf(stderr,"[ %s.%03ld ] %s: Sleeping for %d ms\n",buff, ts.tv_nsec,tmp.c_str(),(int)slp*1000);
		// Sleep
		sleep(slp);
  	}
    return 0;
}

void initMemSem(){
	// ftok to generate unique key for shared memory
	if ((key = ftok(shmkey,65)) == -1) {
   		perror ("ftok"); exit(1);
	}
	// shmget returns an identifier in shmid
	if ( (shmid = shmget(key,0,0)) < 0 ) {
		perror("shmget"); exit(1);
	}
	// shmat to attach to shared memory
	str= (char*) shmat(shmid,(void*)0,0);
	// ftok to generate unique key for Mutex
	if ((key = ftok(msemkey,65)) == -1) {
   		perror ("ftok Mutex"); exit(1);
	}
	// Create  Mutex Semaphore
	if ((mutex_sem = semget (key, 1, 0)) == -1) {
    	perror ("semget Mutex"); exit (1);
	}
	// ftok to generate unique key for consumer semaphore
	if ((key = ftok(csemkey,65)) == -1) {
   		perror ("ftok Con"); exit(1);
	}
	// Create  consumer Semaphore ( Indicates number of available items on the buffer )
	if ((con_sem = semget (key, 1, 0)) == -1) {
    		perror ("semget Con"); exit (1);
	}
	// ftok to generate unique key for producer semaphore
	if ((key = ftok(psemkey,65)) == -1) {
   		perror ("ftok Prod"); exit(1);
	}
	// Create  producer Semaphore ( Indicates number of available places on the buffer )
	if ((prod_sem = semget (key, 1, 0)) == -1) {
    		perror ("semget Prod"); exit (1);
	}
}

void handler_function(int sig){
	// Give user something to know that ctrl C is detected and return unitl all semaphores are released
	printf("\nRequest to TERMINATE initiated....\n ");
	printf("Releaseing all Semaphores....\n ");
	EXITVAL = 1;	
}

void end(){
	//detach from shared memory 
	shmdt(str);
	printf("\n TERMINATING...\n");
	exit(0);
}
