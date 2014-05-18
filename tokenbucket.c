#include<math.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h> 
#include<sys/time.h>
#include<pthread.h>
#include<unistd.h>
#include<signal.h>
#include<fcntl.h>
#include"my402list.h"


#define DEFAULT_LAMBDA 0.5
#define DEFAULT_P 3
#define DEFAULT_B 10
#define DEFAULT_MU 0.35
#define DEFAULT_R 1.5
#define DEFAULT_N 20
#define MAX_VAL 2147483647

typedef struct packetStruct{
	int packetID;
	int tokensRequired;
	float transmissionTime;
	struct timeval arrivalTimeQ1;
	struct timeval departureTimeQ1;
	struct timeval arrivalTimeQ2;
	struct timeval departureTimeQ2;
} Packet;


typedef struct inputVal{
	int lambdaInverse;
	int muInverse;
	int P;
	int n;
} Input;

pthread_t packetArrivalThread;
pthread_t tokenDepositingThread;
pthread_t serverThread;
pthread_t signalHandle;

float lambda = 0.5, mu = 0.35, r = 1.5;
int B = 10, P = 3, n = 20;
char *t = NULL;

int traceFileFlag = 0;

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t out = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condQ2Packet = PTHREAD_COND_INITIALIZER;
sigset_t intSignal;

My402List *arrivalQueue = NULL;
My402List *serverQueue = NULL;
My402List *completedPacketsQueue = NULL;
int tokenBucketCount = 0;

My402List *argList = NULL;

int alltokenCount = 0;
int droptokenCount = 0;
int packetCounter = 0;
int dropPacketCounter = 0;
int removedPacketCounter = 0;
long double totalSystemTime = 0;
double totalServiceTime = 0;
double totalInterarrivalTime = 0;
double totalTimeInQ1 = 0;
double totalTimeInQ2 = 0;
double totalTimeInS = 0;
long double totalSystemTimeSqrd = 0; 

struct timeval emulationStartTime;
struct timeval emulationEndTime;
double emulationDuration;

int endArrivalThread = 0;
int endTokenThread = 0;
int endServerThread = 0;
int signalIntFlag = 0;

/**
*	Function converts struct timeval value obtained from
*	gettimeofday to microseconds
*/
double converttoMicroSeconds(struct timeval temp){
	return ((temp.tv_sec * 1000000) + temp.tv_usec);
}

/**
*	Function converts struct timeval value obtained from
*	gettimeofday to milliseconds
*/
double converttoMilliSeconds(struct timeval temp){
	return ((temp.tv_sec * 1000000) + temp.tv_usec)/1000;
}


/**
*	Function calculates in microseconds, the difference 
*	between the current time and the emulation start time
*/
double calcDisplayTime(){
	struct timeval currentTime;
	if(gettimeofday(&currentTime, 0) == -1){
		//error getting time of the day
		fprintf(stderr, "ERROR - Unable to get current system time\n");
        exit(1);
	}
	return (converttoMicroSeconds(currentTime) - converttoMicroSeconds(emulationStartTime))/1000;
}

/**
*	Function reads a line from input tsfile if -t option 
*	is present in th command line input
*/
Input* readLine(FILE *file, int line){
	int P, lambdaInverse, muInverse;
	char buf[100];
	
	if(fgets(buf, 100, file) != NULL){
		char *saveptr, *val;
		Input *data = (Input *)malloc(sizeof(Input));
		int i;
		buf[strlen(buf) - 1] = '\0';
		if(line == 1){
			for(i = 0; i < strlen(buf); i++){
				if(buf[i] == '\t' || buf[i] == ' '){
					fprintf(stderr, "ERROR1 - Invalid file format\n");
					exit(1);	
				}
			}
			n = atoi(buf);
			data -> n = n;
			data -> P = 0;
			data -> lambdaInverse = 0;
			data -> muInverse = 0;
		}
		else{
			val = strtok_r(buf, " \t", &saveptr);
			if(val != NULL){
				lambdaInverse = atoi(val);
				val = strtok_r(NULL, " \t", &saveptr);
				if(val != NULL){
					P = atoi(val);
					val = strtok_r(NULL, " \t", &saveptr);
					if(val != NULL){ 
						muInverse = atoi(val);
						val = strtok_r(NULL, " \t", &saveptr);
						if(val == NULL){
								
							data -> n = n;
							data -> P = P;
							//data -> lambda = 1000.0/((float)lambdaInverse);
							data -> lambdaInverse = lambdaInverse;
							//data -> mu = 1000.0/((float)muInverse);
							data -> muInverse = muInverse;
							//My402ListAppend(argList, data);
						}
						else{
							//error in file, leading or trailing spaces or tabs
							fprintf(stderr, "ERROR - Invalid file format\n");
							exit(1);
						}
					}
					else{
						//error in file, leading or trailing spaces or tabs
						fprintf(stderr, "ERROR - Invalid file format\n");
						exit(1);
					}
				}
				else{
				//error in file, leading or trailing spaces or tabs
					fprintf(stderr, "ERROR - Invalid file format\n");
					exit(1);
				}
			}
			else{
				//error in file, leading or trailing spaces or tabs
				fprintf(stderr, "ERROR - Invalid file format\n");
				exit(1);
			}	
		}
		return data;
	
	}
	else{
		//error in file, leading or trailing spaces or tabs
		fprintf(stderr, "ERROR - Unable to read file lines\n");
		exit(1);
	}
}

/**
*	Function parses input arguments supplied from commandline 
*	to get values of all the options and check for errors
*/
void readInputArguments(int argc, char *argv[]){
	int i;
	for(i = 0; i < argc - 1; i++){
		if(strcmp(argv[i], "-lambda") == 0){
			lambda = atof(argv[i+1]);
			if(lambda < 0){
				fprintf(stderr, "ERROR - lambda must be a positive real number\n");
	        	exit(1);
			}
				//argerror
			if((1/lambda) > 10)
				lambda = 0.1;
		}
		if(strcmp(argv[i], "-mu") == 0){
			mu = atof(argv[i+1]);
			if(mu < 0){
				fprintf(stderr, "ERROR - mu must be a positive real number\n");
	        	exit(1);
			}
				//argerror
			if((1/mu) > 10)
				mu = 0.1;
		}
		if(strcmp(argv[i], "-B") == 0)
			B = atoi(argv[i+1]);
			if(B < 0 || B > MAX_VAL){
				fprintf(stderr, "ERROR - B must be a positive integer and smaller than 2147483647\n");
	        	exit(1);
			}
				//argerror
		if(strcmp(argv[i], "-P") == 0)
			P = atoi(argv[i+1]);
			if(P < 0 || P > MAX_VAL){
				fprintf(stderr, "ERROR - P must be a positive integer and smaller than 2147483647\n");
	        	exit(1);
			}
				//argerror
		if(strcmp(argv[i], "-r") == 0){
			r = atof(argv[i+1]);
			if(r < 0){
				fprintf(stderr, "ERROR - r must be a positive real number\n");
	        	exit(1);
			}
				//argerror
			if((1/r) > 10)
				r = 0.1;
		}
		if(strcmp(argv[i], "-n") == 0)
			n = atoi(argv[i+1]);
			if(n < 0 || n > MAX_VAL){
				fprintf(stderr, "ERROR - n must be a positive integer and smaller than 2147483647\n");
	        	exit(1);
			}
				//argerror	
		if(strcmp(argv[i], "-t") == 0){
			traceFileFlag = 1;
			//printf("%s|%s\n", argv[i], argv[i+1]);
			t = strdup(argv[i+1]);	
			if(t == NULL){
				fprintf(stderr, "ERROR - No file name supplied\n");
	        	exit(1);
			}
				//argerror
		}
				
	}
									
}


/**
*	Function displays input option values parsed  
* 	from command line on stdout
*/
void displayInputArguments(){

	FILE *file1 = fopen(t, "r");	
	Input *data = NULL;
	if(traceFileFlag){
		data = readLine(file1, 1);
		n = data -> n;
		fclose(file1);
	}
	printf("\nEmulation Parameters:\n");
	printf("\n\tnumber to arrive = %d", n);
	if(!traceFileFlag)
		printf("\n\tlambda = %.3g", lambda);
	if(!traceFileFlag)
		printf("\n\tmu = %.3g", mu);
	printf("\n\tr = %.3g", r);
	printf("\n\tB = %d", B);
	if(!traceFileFlag)
		printf("\n\tP = %d", P);
	if(traceFileFlag)
		printf("\n\ttsfile = %s", t);
	printf("\n\n");	
	
	
}

/**
*	Function allocates memory and initializes  
* 	Arrival Queue and Server Queue
*/
void initQueues(){
	arrivalQueue = (My402List *)malloc(sizeof(My402List));
	if(arrivalQueue == NULL){
		fprintf(stderr, "ERROR - Unable to allocate memory from the heap\n");
        exit(1);
	}
	else
		My402ListInit(arrivalQueue);
	
	serverQueue = (My402List *)malloc(sizeof(My402List));
	if(serverQueue == NULL){
		//error, malloc failed
		fprintf(stderr, "ERROR - Unable to allocate memory from the heap\n");
        exit(1);
	}
	else
		My402ListInit(serverQueue);
}

/**
*	Function initializes packet with an ID, number of 
* 	tokens required and the service/transmission time
*/
void initPacket(Packet *newPacket, int id, int tokensRequired, double transmissionTime){
	newPacket -> packetID = id;
	newPacket -> tokensRequired = tokensRequired;	
	newPacket -> transmissionTime = transmissionTime;		
}

/**
*	Function adds packet to arrival queue 
*
*/
void addToArrivalQueue(Packet *newPacket){
	if(arrivalQueue != NULL){
		My402ListAppend(arrivalQueue, newPacket);
	}
}

/**
*	Function checks if packet can move from Arrival 
* 	queue to Server queue by comparing the number of tokens 
*	in the bucket and the number required by the packet
*/
int checkMoveToQueue2(){
	if(arrivalQueue != NULL){
		if(!My402ListEmpty(arrivalQueue)){
			My402ListElem *first = My402ListFirst(arrivalQueue);
			if(((Packet *)(first -> obj)) -> tokensRequired <= tokenBucketCount)
				return 1;
		}
		else
			return 0;
	}
	return 0;
}

/**
*	Function transfers packet from Arrival queue to 
*	Server queue 
*/
void transferPacketToQ2(){
	if(arrivalQueue != NULL && My402ListEmpty(arrivalQueue) != 1){
		My402ListElem *firstPacket = My402ListFirst(arrivalQueue);
		if(serverQueue != NULL)
			My402ListAppend(serverQueue, firstPacket -> obj);
		My402ListUnlink(arrivalQueue, firstPacket);
	}
}

/**
*	Sets arrival queue departure time and server queue 
*	arrival time for packet being transferred
*/
void setDepartureArrivalTime(struct timeval temp){
	if(arrivalQueue != NULL && My402ListEmpty(arrivalQueue) != 1){
		My402ListElem *firstPacket = My402ListFirst(arrivalQueue);
		((Packet *)(firstPacket -> obj)) -> departureTimeQ1 = temp;
		((Packet *)(firstPacket -> obj)) -> arrivalTimeQ2 = temp;
	}
}

/**
*	Thread function for the packet arrival thread 
*	
*/
void *processArrivalQueue(void *arg){
	int tokensRequired;
	double interarrivalTime, transmissionTime, timeToSleep = 0, displayTime, measuredInterarrival;
	struct timeval currentTime, startPrevious, finishPrevious, timeAfterSleep; 
	long double timeInQ1;
	int lineCount = 0;
	FILE *file = fopen(t, "r");	
	Input *data = NULL;
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);
	if(traceFileFlag){
		data = readLine(file, 1);
		n = data -> n;
	}
	lineCount = n;
	while(!signalIntFlag){
		if(traceFileFlag){
			if(lineCount > 0){
				data = readLine(file, 2);
				interarrivalTime = data -> lambdaInverse;
				tokensRequired = data -> P;
				transmissionTime = data -> muInverse;
				lineCount --;
			}
		}
		else{
			interarrivalTime = 1000.0/(float)lambda;
			tokensRequired = P;
			transmissionTime = 1000.0/(float)mu;
		}	
				
		if(gettimeofday(&finishPrevious, 0) == -1){
			fprintf(stderr, "ERROR - Unable to get current system time\n");
	        exit(1);
		}
		
		//call usleep
		if(packetCounter != 0){
			timeToSleep = interarrivalTime * 1000 - (converttoMicroSeconds(finishPrevious) - converttoMicroSeconds(startPrevious));
			if(timeToSleep < 0)
				timeToSleep = 0;
			usleep(timeToSleep);
		}
		if(packetCounter == 0){
			timeToSleep = interarrivalTime * 1000;
			usleep(timeToSleep);
		}
		if(gettimeofday(&timeAfterSleep, 0) == -1){
			//error getting time of the day
			fprintf(stderr, "ERROR - Unable to get current system time\n");
	        exit(1);
		}	
		if(packetCounter == 0){
			measuredInterarrival = interarrivalTime;
		}
		else
			measuredInterarrival = (converttoMicroSeconds(timeAfterSleep) - converttoMicroSeconds(startPrevious))/1000;
		
		if(packetCounter + 1 > n){
			endArrivalThread = 1;
			return 0;
		}
		totalInterarrivalTime += measuredInterarrival;
		Packet *p = (Packet *)malloc(sizeof(Packet));
		if(p == NULL){
			fprintf(stderr, "ERROR - Unable to allocate memory from the heap\n");
        	exit(1);
		}
		packetCounter ++;
		initPacket(p, packetCounter, tokensRequired, transmissionTime);
		if(gettimeofday(&currentTime, 0) == -1){
			//error getting time of the day
			fprintf(stderr, "ERROR - Unable to get current system time\n");
	        exit(1);
		}
		if(p -> tokensRequired > B)
		{
			dropPacketCounter ++;
			displayTime = calcDisplayTime();
			flockfile(stdout);
			printf("%012.3fms: p%d arrives, needs %d tokens, inter-arrival time = %.3lfms, dropped\n", displayTime, p -> packetID, p -> tokensRequired, measuredInterarrival);
			funlockfile(stdout);
			continue;
			
		}
		pthread_mutex_lock(&m);
		
		p -> arrivalTimeQ1 = currentTime;
		
		displayTime = calcDisplayTime();
		flockfile(stdout);
		printf("%012.3fms: p%d arrives, needs %d tokens, inter-arrival time = %.3lfms\n", displayTime, p -> packetID, p -> tokensRequired, measuredInterarrival);
		funlockfile(stdout);
		if(gettimeofday(&startPrevious, 0) == -1){
			//error getting time of the day
			fprintf(stderr, "ERROR - Unable to get current system time\n");
	        exit(1);
		}
		addToArrivalQueue(p);
		if(gettimeofday(&currentTime, 0) == -1){
			//error getting time of the day
			fprintf(stderr, "ERROR - Unable to get current system time\n");
	        exit(1);
		}
		displayTime = calcDisplayTime();
		flockfile(stdout);
		printf("%012.3fms: p%d enters Q1\n", displayTime, p -> packetID);
		funlockfile(stdout);
		if(checkMoveToQueue2()){
			displayTime = calcDisplayTime();
			transferPacketToQ2();
			//set departure and arrival time for packet being transferred. 
			
			if(gettimeofday(&currentTime, 0) == -1){
				//error getting time of the day
				fprintf(stderr, "ERROR - Unable to get current system time\n");
		        exit(1);
			}
			p -> departureTimeQ1 = currentTime;
			p -> arrivalTimeQ2 = currentTime;
			timeInQ1 = ((converttoMicroSeconds(p -> departureTimeQ1)) - (converttoMicroSeconds(p -> arrivalTimeQ1)))/1000;
			tokenBucketCount -= p -> tokensRequired;
			flockfile(stdout);
			printf("%012.3fms: p%d leaves Q1, time in Q1 = %.3Lfms, token bucket now has %d tokens\n", displayTime, p -> packetID, timeInQ1, tokenBucketCount);
			displayTime = calcDisplayTime();
			printf("%012.3fms: p%d enters Q2\n", displayTime, p -> packetID);
			funlockfile(stdout);
			
			totalTimeInQ1 += timeInQ1;
			pthread_cond_signal(&condQ2Packet);
		}
		
		pthread_mutex_unlock(&m);	
	}
	return 0;
}

/**
*	Thread function for the token depositing thread
*	
*/
void *processTokenBucket(void *arg){
	
	double timeToSleep, displayTime;
	long double timeInQ1;
	double tokenArrivalTime = 1000.0/(float)r;
	struct timeval finishPrevious, startPrevious, currentTime;
	
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);
	
	while(!signalIntFlag){
		
		if(endArrivalThread == 1){
			if(My402ListEmpty(arrivalQueue)){
				endTokenThread = 1;
				pthread_cond_signal(&condQ2Packet);
				return 0;
			}
		}
		if(gettimeofday(&finishPrevious, 0) == -1){
			//error getting time of the day
			fprintf(stderr, "ERROR - Unable to get current system time\n");
	        exit(1);
		}
		
		if(alltokenCount != 0){
			timeToSleep = tokenArrivalTime * 1000 - (converttoMicroSeconds(finishPrevious) - converttoMicroSeconds(startPrevious));
			if(timeToSleep < 0)
					timeToSleep = 0;
			usleep(timeToSleep);
		}
		if(alltokenCount == 0){
			timeToSleep = tokenArrivalTime * 1000;
			usleep(timeToSleep);
		}
		pthread_mutex_lock(&m);
		if(gettimeofday(&startPrevious, 0) == -1){
			//error getting time of the day
			fprintf(stderr, "ERROR - Unable to get current system time\n");
	        exit(1);
		}
		alltokenCount ++;
		displayTime = calcDisplayTime();
		if(tokenBucketCount + 1 <= B){
			tokenBucketCount ++;
			flockfile(stdout);
			printf("%012.3fms: token t%d arrives, token bucket now has %d tokens\n", displayTime, alltokenCount, tokenBucketCount);
			funlockfile(stdout);
		}
		else{
			flockfile(stdout);
			printf("%012.3fms: token t%d arrives, dropped\n", displayTime, alltokenCount);
			funlockfile(stdout);
			droptokenCount ++;
		}
		if(checkMoveToQueue2()){
			displayTime = calcDisplayTime();
			if(gettimeofday(&currentTime, 0) == -1){
				//error getting time of the day
				fprintf(stderr, "ERROR - Unable to get current system time\n");
		        exit(1);
			}
			
			My402ListElem *firstPacket = My402ListFirst(arrivalQueue);
			timeInQ1 = ((converttoMicroSeconds(currentTime)) - (converttoMicroSeconds(((Packet *)(firstPacket -> obj)) -> arrivalTimeQ1)))/1000;
			totalTimeInQ1 += timeInQ1;
			tokenBucketCount -= ((Packet *)(firstPacket -> obj)) -> tokensRequired;
			flockfile(stdout);
			printf("%012.3fms: p%d leaves Q1, time in Q1 = %.3Lfms, token bucket now has %d tokens\n",displayTime, ((Packet *)(firstPacket -> obj)) -> packetID, timeInQ1, tokenBucketCount);
			displayTime = calcDisplayTime();
			printf("%012.3fms: p%d enters Q2\n", displayTime, ((Packet *)(firstPacket -> obj)) -> packetID);
			funlockfile(stdout);
					
			setDepartureArrivalTime(currentTime);
			transferPacketToQ2();
			pthread_cond_signal(&condQ2Packet);
			
		}
		
		pthread_mutex_unlock(&m);	
	}
	
	return 0;
}

/**
*	Function to unlink packet being serviced from Q2 
*	
*/
void transferPacketToServer(){
	if(serverQueue != NULL && My402ListEmpty(serverQueue) != 1){
		My402ListElem *firstPacket = My402ListFirst(serverQueue) -> obj;
		My402ListUnlink(serverQueue, firstPacket);
	}
}

/**
*	Thread function for the server processing
*	
*/
void *processServerQueue(void *arg){
	double timeToSleep = 0, timeInQ2, timeInSystem, displayTime, serviceTime;
	struct timeval currentTime;
	
	while(1){
		
		pthread_mutex_lock(&m);
		while(My402ListEmpty(serverQueue)){
			if(signalIntFlag)
				return 0;
			if(endTokenThread){
				endServerThread = 1;
				return 0;
			}
				
			pthread_cond_wait(&condQ2Packet, &m);
			//wait for signal
		}
		
		if(endTokenThread){
			if(My402ListEmpty(serverQueue)){
				endServerThread = 1;
				return 0;
			}
		}
		
		Packet *first = (Packet *)(My402ListFirst(serverQueue) -> obj);
		timeToSleep = first -> transmissionTime;
		if(gettimeofday(&currentTime, 0) == -1){
			//error getting time of the day
			fprintf(stderr, "ERROR - Unable to get current system time\n");
	        exit(1);
		}
		first -> departureTimeQ2 = currentTime;
		displayTime = calcDisplayTime();
		timeInQ2 = (converttoMicroSeconds(first -> departureTimeQ2) - converttoMicroSeconds(first -> arrivalTimeQ2))/1000;
		totalTimeInQ2 += timeInQ2;
		flockfile(stdout);
		printf("%012.3fms: p%d leaves Q2, time in Q2 = %.3lfms, begin service at S\n", displayTime, first -> packetID, timeInQ2);
		funlockfile(stdout);
		
		pthread_mutex_unlock(&m);
		usleep((timeToSleep) * 1000);
		if(gettimeofday(&currentTime, 0) == -1){
			//error getting time of the day
			fprintf(stderr, "ERROR - Unable to get current system time\n");
	        exit(1);
		}
		pthread_mutex_lock(&m);
		serviceTime = (converttoMicroSeconds(currentTime) - converttoMicroSeconds(first -> departureTimeQ2))/1000;
		timeInSystem = (converttoMicroSeconds(currentTime) - converttoMicroSeconds(first -> arrivalTimeQ1))/1000;
		displayTime = calcDisplayTime();
		flockfile(stdout);
		printf("%012.3fms: p%d departs from S, service time = %.3lfms, time in system = %.3lfms\n", displayTime, first -> packetID, serviceTime, timeInSystem);
		funlockfile(stdout);
		totalSystemTime += timeInSystem;
		totalSystemTimeSqrd += timeInSystem * timeInSystem;
		totalServiceTime += serviceTime;
		//throw out packet
		My402ListUnlink(serverQueue, My402ListFirst(serverQueue));
		
		pthread_mutex_unlock(&m);
		if(signalIntFlag){
			return 0;
			
		}
	}
	return 0;
}

void removePacketFromServer(){
	if(completedPacketsQueue != NULL && My402ListEmpty(completedPacketsQueue) != 1){
		My402ListElem *firstPacket = My402ListFirst(completedPacketsQueue) -> obj;
		My402ListUnlink(serverQueue, firstPacket);
	}
}

/**
*	Function to calculate required statistics as per the spec
*	
*/
void calcStatists(){

	double tokenDropProbability, packetDropProbability;
	double avgTimeSpentInSystem, avgServiceTime, avgInterarrivalTime;
	double avgNumberPacketsQ1, avgNumberPacketsQ2, avgNumberPacketsS; 
	double val1, val2, val3, val4, stdDeviationSystemTime;
	
	tokenDropProbability = (double)droptokenCount / (double)alltokenCount;
	packetDropProbability = (double)dropPacketCounter / (double)packetCounter;
	avgTimeSpentInSystem = (totalSystemTime / 1000) / (packetCounter - dropPacketCounter - removedPacketCounter);
	avgServiceTime = (totalServiceTime / 1000) / (packetCounter - dropPacketCounter - removedPacketCounter);
	avgInterarrivalTime = (totalInterarrivalTime / 1000) / packetCounter;
	avgNumberPacketsQ1 = (totalTimeInQ1) / emulationDuration;
	avgNumberPacketsQ2 = (totalTimeInQ2) / emulationDuration;
	avgNumberPacketsS = (totalServiceTime) / emulationDuration;
	
	val1 = totalSystemTimeSqrd/(1000 * 1000);
	val2 = packetCounter - dropPacketCounter - removedPacketCounter;
	val3 = val1 / val2;
	val4 = avgTimeSpentInSystem * avgTimeSpentInSystem;
	stdDeviationSystemTime = sqrt(val3 - val4);
	
	printf("\nStatistics:\n");
	if(packetCounter == 0)
		printf("\taverage packet inter-arrival time = n/a (no packet arrived in the system)\n");	
	else
		printf("\taverage packet inter-arrival time = %.6g\n", avgInterarrivalTime);
	
	if((packetCounter - dropPacketCounter - removedPacketCounter) == 0)
	{
		if(packetCounter == 0)
			printf("\taverage packet service time = n/a (no packet arrived in the system)\n");	
		else
			printf("\taverage packet service time = n/a (no packet reached the server)\n");	
	}
	else
		printf("\taverage packet service time = %.6g\n", avgServiceTime);
	
	if(emulationDuration == 0){
		printf("\n\taverage number of packets in Q1 = n/a (emulation time is zero)\n");
		printf("\taverage number of packets in Q2 = n/a (emulation time is zero)\n");
		printf("\taverage number of packets in S = n/a (emulation time is zero)\n");
	}
	else{
		printf("\n\taverage number of packets in Q1 = %.6g\n", avgNumberPacketsQ1);
		printf("\taverage number of packets in Q2 = %.6g\n", avgNumberPacketsQ2);
		printf("\taverage number of packets in S = %.6g\n", avgNumberPacketsS);
	}	
	
	
	if((packetCounter - dropPacketCounter - removedPacketCounter) == 0)
	{
		if(packetCounter == 0)
			printf("\taverage time a packet spent in system = n/a (no packet arrived in the system)\n");	
		else
			printf("\taverage time a packet spent in system = n/a (no packet reached the server)\n");	
	}
	else
		printf("\n\taverage time a packet spent in system = %.6g\n", avgTimeSpentInSystem);
		
	if((packetCounter - dropPacketCounter - removedPacketCounter) == 0)
	{
		if(packetCounter == 0)
			printf("\tstandard deviation for time spent in system = n/a (no packet arrived in the system)\n");	
		else
			printf("\tstandard deviation for time spent in system = n/a (no packet reached the server)\n");	
	}
	else
		printf("\tstandard deviation for time spent in system = %.6g\n", stdDeviationSystemTime);
		
	if(alltokenCount == 0)
		printf("\ttoken drop probability = n/a (no token arrived in the system)\n");	
	else	
		printf("\n\ttoken drop probability = %.6g\n", tokenDropProbability);
		
	if(packetCounter == 0)
		printf("\tpacket drop probability = n/a (no packet arrived in the system)\n");	
	else
		printf("\tpacket drop probability = %.6g\n\n", packetDropProbability);
	
}

/**
*	Thread function for handling the SIGINT interrupt 
*	
*/

void *signalHandler(void * arg){
	int sig;
	sigwait(&intSignal, &sig);
	My402ListElem *i = NULL;
	double displayTime;
	int a;
	flockfile(stdout);
	printf("\n");
	funlockfile(stdout);
	signalIntFlag = 1;
	endArrivalThread = 1;
	endTokenThread = 1;
	endServerThread = 1;
	pthread_cancel(packetArrivalThread);
	pthread_cancel(tokenDepositingThread);
	
	pthread_mutex_lock(&m);
	
	if(My402ListLength(serverQueue) == 0)
		pthread_cond_signal(&condQ2Packet);
	removedPacketCounter = My402ListLength(arrivalQueue);
	a = My402ListLength(arrivalQueue);
	i = My402ListFirst(arrivalQueue);
	while(a != 0){
		displayTime = calcDisplayTime();
		flockfile(stdout);
		printf("%012.3fms: p%d removed from Q1\n", displayTime, ((Packet *)(i -> obj)) -> packetID);
		funlockfile(stdout);
		i = My402ListNext(arrivalQueue, i);
		a--;
	}
	
	
	pthread_mutex_unlock(&m);
	return 0;
}

/**
*	Function for creation of threads
*	
*/
void createThreads(){
	int errorNumber, a;
	double displayTime;
	My402ListElem *i = NULL;
	
	if(gettimeofday(&emulationStartTime, 0) == -1){
		//error in getting time
		fprintf(stderr, "ERROR - Unable to get current system time\n");
        exit(1);
	}
	
	printf("%012.3fms: emulation begins\n", 0.0f);
	
	
	sigemptyset(&intSignal);
	sigaddset(&intSignal, SIGINT);
	pthread_sigmask(SIG_BLOCK, &intSignal, 0);
	
	errorNumber = pthread_create(&signalHandle, 0, signalHandler, 0);
	if(errorNumber != 0){
		//error
		fprintf(stderr, "ERROR - Unable to create thread\n");
        exit(1);
	}
	errorNumber = pthread_create(&packetArrivalThread, 0, processArrivalQueue, 0);
	if(errorNumber != 0){
		//error
		fprintf(stderr, "ERROR - Unable to create thread\n");
        exit(1);
	}
	errorNumber = pthread_create(&tokenDepositingThread, 0, processTokenBucket, 0);
	if(errorNumber != 0){
		//error
		fprintf(stderr, "ERROR - Unable to create thread\n");
        exit(1);
	}
	errorNumber = pthread_create(&serverThread, 0, processServerQueue, 0);
	if(errorNumber != 0){
		//error
		fprintf(stderr, "ERROR - Unable to create thread\n");
        exit(1);
	}
	
	pthread_join(packetArrivalThread, 0);
	pthread_join(tokenDepositingThread, 0);
	pthread_join(serverThread, 0);
	
	a = My402ListLength(serverQueue);
	i = My402ListFirst(serverQueue);
	
	while(a != 0){
		displayTime = calcDisplayTime();
		
		printf("%012.3fms: p%d removed from Q2\n", displayTime, ((Packet *)(i -> obj)) -> packetID);
		i = My402ListNext(serverQueue, i);
		a--;
	}
	
	displayTime = calcDisplayTime();
	if(gettimeofday(&emulationEndTime, 0) == -1){
		//error in getting time
		fprintf(stderr, "ERROR - Unable to get current system time\n");
        exit(1);
	}
	printf("%012.3fms: emulation ends\n", displayTime);
	emulationDuration = displayTime;
	
}


int main(int argc, char *argv[]){
	readInputArguments(argc, argv);
	displayInputArguments();
	initQueues();
	createThreads();
	calcStatists();
	return 0;
}






























