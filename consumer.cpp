#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <bits/stdc++.h>
#include <signal.h>
using namespace std;

// Define files pathes used for keys
#define shmkey "files/shmkey"
#define psemkey "files/psemkey"
#define csemkey "files/csemkey"
#define msemkey "files/msemkey"

// map used for commodities to index
map <string, int> mp;

// functions used in main
void handler_function(int sig);
void fill(vector<deque<double>>& v, string str);
void initMemSem();
void end();
void printCon(vector<deque<double>> v);

// Global variables
union semun
{
	int val;
	struct semid_ds *buf;
	ushort array [1];
} sem_attr;

char* str;
int mutex_sem, con_sem, prod_sem, shmid, memsize, BOUND;
struct sembuf asem [1];
key_t key;

int main(int argc, char *argv[])
{
    // initialize map
    mp = {{"ALUMINIUM",0},
			{"COPPER",1},
			{"COTTON",2},
			{"CRUDEOIL",3},
			{"GOLD",4},
			{"LEAD",5},
			{"MENTHAOIL",6},
			{"NATURALGAS",7},
			{"NICKEL",8},
			{"SILVER",9},
			{"ZINC",10}};
	// Catch ctrl C
	signal(SIGINT, handler_function);
	// initialize vector of deques used for printing
	vector<deque<double>> v(11);
	// Get max number of items on buffer and initialize memsize
	BOUND = stoi(argv[1]);
	memsize = BOUND * 32;
	// initialize semaphores buffer struct
	asem [0].sem_num = 0;
	asem [0].sem_op = 0;
	asem [0].sem_flg = 0;
	// Initialize memory and semaphores
	initMemSem();
	while(true) {
		// Check if you can take an item from the buffer
		asem[0].sem_op=-1;
		if (semop (con_sem, asem, 1) == -1) {
	   		perror ("semop: con_sem"); exit (1);
        	}
		// Get Mutual Execlusion
		asem[0].sem_op=-1;
		if (semop (mutex_sem, asem, 1) == -1) {
	   		perror ("semop: mutex_sem"); exit (1);
        	}
		// Critical Section Begin
		string tmp = str;
		string tmp2 = tmp.substr(0, tmp.find("\n"));
		tmp.erase(0, tmp.find("\n") + 1);
		strcpy(str, tmp.c_str());
		// Critical Section End
		// Release Mutual Execlusion
		asem[0].sem_op=1;
		if (semop (mutex_sem, asem, 1) == -1) {
	   		perror ("semop: mutex_sem"); exit (1);
        	}
        	// Add empty space on buffer
        	asem[0].sem_op=1;
		if (semop (prod_sem, asem, 1) == -1) {
	   		perror ("semop: prod_sem"); exit (1);
        	}
        // Fill and print vector of deques
		fill(v, tmp2);
		printCon(v);
	}
    return 0;
}

void initMemSem(){
	// ftok to generate unique key for shared memory
	if ((key = ftok(shmkey,65)) == -1) {
       		perror ("ftok"); exit(1);
    	}
	// shmget returns an identifier in shmid
	if ( (shmid = shmget(key,memsize,0666|IPC_CREAT)) < 0 ) {
		perror("shmget"); exit(1);
	}
	// put shared memory in str
	str= (char*) shmat(shmid,(void*)0,0);
	// ftok to generate unique key for Mutex
	if ((key = ftok(msemkey,65)) == -1) {
       		perror ("ftok Mutex"); exit(1);
    	}
    	// Create  Mutex Semaphore
	if ((mutex_sem = semget (key, 1, 0666 | IPC_CREAT)) == -1) {
        	perror ("semget Mutex"); exit (1);
    	}
    	// Giving initial value.
	sem_attr.val = 1;
	if (semctl (mutex_sem, 0, SETVAL, sem_attr) == -1) {
		perror ("semctl Mutex SETVAL"); exit (1);
	}
	// ftok to generate unique key for consumer
	if ((key = ftok(csemkey,65)) == -1) {
       		perror ("ftok Con"); exit(1);
    	}
    	// Create  consumer Semaphore ( Indicates number of available items on the buffer )
	if ((con_sem = semget (key, 1, 0666 | IPC_CREAT)) == -1) {
        	perror ("semget Con"); exit (1);
    	}
    	// Giving initial value.
	sem_attr.val = 0;
	if (semctl (con_sem, 0, SETVAL, sem_attr) == -1) {
		perror ("semctl Con SETVAL"); exit (1);
	}
	// ftok to generate unique key for producer
	if ((key = ftok(psemkey,65)) == -1) {
       		perror ("ftok Prod"); exit(1);
    	}
    	// Create  producer Semaphore ( Indicates number of available places on the buffer )
	if ((prod_sem = semget (key, 1, 0666 | IPC_CREAT)) == -1) {
        	perror ("semget Prod"); exit (1);
    	}
    	// Giving initial value.
	sem_attr.val = BOUND;
	if (semctl (prod_sem, 0, SETVAL, sem_attr) == -1) {
		perror ("semctl Prod SETVAL"); exit (1);
	}
}

// function to make sure only 6 items on deque ( first 1-5 for current average ---- 2-6 for previous average )
void only4(vector<deque<double>>& v){
	for (auto& deq : v)
		while ( deq.size() > 6 )
			deq.pop_back();
}

// tokenize and fill deque
void fill(vector<deque<double>>& v, string str){
    	string s = str;
    	string key = "";
    	string tmp= "";
    	int i = 0;
    	while(s[i] != ',') {
    		key += s[i];
    		i++;
    	}
    	i++;
    	while(i < s.length()) {
    		tmp += s[i];
    		i++;
    	}
    	double c = stod(tmp);
    	v[mp[key]].push_front(c);
    	only4(v);
}

// Helper function to print spaces
string printSpace(int w) {
	string s = "";
	while(w--) s+=" ";
	return s;
}

// Function to count digite
int count_digit(int number) {
   int count = 0;
   if (number == 0) return 1;
   while(number != 0) {
      number = number / 10;
      count++;
   }
   return count;
}

// Function to calculate average
double getAvg(deque<double> vals, int k) {
	int count = 0;
	double sum = 0;
	if ( k == 5 && vals.size() < 6) k = 6;
	for (int i = 0; i < vals.size(); i++) {
		if ( i == k ) continue;
		count++;
		sum += vals[i];
	}
	if ( count == 0 ) return 0.00;
	return sum/count;
}

// print function using previous helper functions
void printCon(vector<deque<double>> v) {
    system("clear");
	string up = "↑";
	string down = "↓";
	double c, avg, prevAvg, prev;
	//╚╝╬═╩╠╣╦╔╗║
	cout << "╔═══════════════╦══════════╦══════════╗" << endl;
	cout << "║ Currency      ║  Price   ║ AvgPrice ║" << endl;
	cout << "╠═══════════════╬══════════╬══════════╣" << endl;
	for ( auto i : mp ) {
		cout << "║ " << i.first << printSpace(14 - i.first.size()) << "║";
		int ind = i.second;
		avg = getAvg(v[ind],5);
		prevAvg = getAvg(v[ind],0);
		c = v[ind].size()>0?v[ind][0]:0.00;
		prev =v[ind].size()>1?v[ind][1]:0.00;
		if ( c > prev ) {
			int w = 3 - count_digit((int) c);
	    		cout << printSpace(w/2 + w%2);
	    		printf("\033[32m%7.2lf%s \033[0m",c,up.c_str());
	    		cout << "║";
		}
		else if ( c < prev ) {
			int w = 3 - count_digit((int) c);
	    		cout << printSpace(w/2 + w%2);
	    		printf("\033[31m%7.2lf%s \033[0m",c,down.c_str());
	    		cout << "║";
		}
		else {
			int w = 4 - count_digit((int) c);
	    		cout << printSpace(w/2);
	    		printf("%7.2lf",c);
	    		cout << printSpace(w/2 + w%2) << "║";
		}

		if ( avg > prevAvg ) {
			int w = 3 - count_digit((int) avg);
	    		cout << printSpace(w/2 + w%2);
	    		printf("\033[32m%7.2lf%s \033[0m",avg,up.c_str());
	    		cout << "║" <<endl;
		}
		else if ( avg < prevAvg ) {
			int w = 3 - count_digit((int) avg);
	    		cout << printSpace(w/2 + w%2);
	    		printf("\033[31m%7.2lf%s \033[0m",avg,down.c_str());
	    		cout << "║" <<endl;
		}
		else {
			int w = 4 - count_digit((int) avg);
	    		cout << printSpace(w/2);
	    		printf("%7.2lf",c);
	    		cout << printSpace(w/2 + w%2) << "║" <<endl;
		}
	}
	cout << "╚═══════════════╩══════════╩══════════╝" << endl;


}

// Handles ctrl C
void handler_function(int sig){
	printf("\nRequest to TERMINATE initiated....\n ");
	printf("Releaseing all Semaphores....\n ");
	end();
}

// Check if user wants to deleter shared memory or semaphores
void end(){
	shmdt(str);
	printf("DELETE SEMAPHORES?? [Y/N] : ");
	char c;
	cin >> c;
	if ( c == 'Y' || c == 'y' ) {
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
	}
	printf("DELETE SHARED MEMORY?? [Y/N] : ");
	cin >> c;
	if ( c == 'Y' || c == 'y' ) {
		shmctl(shmid,IPC_RMID,0);
	}
	printf("\n TERMINATING...\n");
	exit(0);
}

