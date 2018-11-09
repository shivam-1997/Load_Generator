//CLIENT
#include <bits/stdc++.h>
#include <sys/time.h>
#include <stdio.h>
#include <sys/types.h>//socket
#include <sys/socket.h>//socket
#include <string.h>//memset
#include <stdlib.h>//sizeof
#include <netinet/in.h>//INADDR_ANY
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fstream>
#include <string>

using namespace std;


// #define SERVER_IP "127.0.0.1"
#define MAXSZ 256
// #define NUM_OF_WORKERS 4
int NUM_OF_WORKERS;
// int *requests;
// double *responseTime;
int requests[200];
double responseTime[200];
void error(char *msg);


struct arg_struct {
    int thread_id;
};

char thread_n[10];

int connect_fun(char *server_ip,int server_port, int &sockfd, struct sockaddr_in &serverAddress, struct hostent *(&server)){
    //create socket
    sockfd=socket(AF_INET,SOCK_STREAM,0);
    //initialize the socket addresses
    memset(&serverAddress,0,sizeof(serverAddress));
    server = gethostbyname(server_ip);
    if(server == NULL){
        fprintf(stderr, "No such host\n");
        exit(0);
    }

    serverAddress.sin_family=AF_INET;

    // serverAddress.sin_addr.s_addr=inet_addr(server->);
    bcopy((char*)server->h_addr,    (char*)&serverAddress.sin_addr.s_addr,  server->h_length);
    serverAddress.sin_port=htons(server_port);
    //client  connect to server on port
    if(connect(sockfd,(struct sockaddr *)&serverAddress,sizeof(serverAddress)) != 0) return 0;
    // cout<<"Connection request successfully sent"<<endl;
    char msg[3];
    bzero(msg,3);
    int bytes_read = read(sockfd,msg,2);
    // if(bytes_read>0)
    // cout<<"connection established. "<<msg<<endl;
    // else
    // cout<<"connection not established. "<<endl;
    return atoi(msg);
}
void disconnect_fun(int &sockfd){
  int n_write = write(sockfd, "disconnect", sizeof("disconnect"));
  char msg[MAXSZ];
  int n_read = read(sockfd, msg, MAXSZ);
  msg[n_read] = 0;
  cout<<msg<<" thread "<< thread_n<<endl;
  close(sockfd);
  // else cout<<"Error, unable to disconnect from client side"<<endl;

}

double executeCommand(std::string command, int &sockfd, struct sockaddr_in &serverAddress, struct hostent *(&server)){
    clock_t t;  // in seconds
    struct timeval begin, end;
    // cout<<command<<endl;
    // Used to split string around spaces.
    int total_length = command.length();
    std::stringstream ss(command);
    string operation;
    ss>>operation;
    int response_t = 0;
    try {
      if(operation.compare("connect")==0){

        string server_ip_string, server_port;
        ss>>server_ip_string;
        ss>>server_port;
        int n = server_ip_string.length();
        char server_ip[n+1];
        strcpy(server_ip, server_ip_string.c_str());

        // t = clock();
        gettimeofday(&begin, NULL);
        if(connect_fun(server_ip, stoi(server_port), sockfd, serverAddress, server)==1){
             // cout<<"Waiting for thread id"<<endl;
            // t = clock() - t;
            gettimeofday(&end, NULL);
            int n = read(sockfd, thread_n,10);
            cout<<"OK-connected to thread "<<thread_n<<endl;

            return double((end.tv_sec - begin.tv_sec) * 1000000.00 + (end.tv_usec - begin.tv_usec)) / 1000000.00;
        }
        else perror("Unable to connect to server");
      }
      else if(operation.compare("disconnect")==0){

          // t = clock();
          gettimeofday(&begin, NULL);
          disconnect_fun( sockfd);
          // t = clock() - t;
          gettimeofday(&end, NULL);
          return double((end.tv_sec - begin.tv_sec) * 1000000.00 + (end.tv_usec - begin.tv_usec)) / 1000000.00;
      }
      else if((operation.compare("create")==0) || (operation.compare("delete")==0)
            ||(operation.compare("update")==0) || (operation.compare("read")==0)){

        int n = command.length();
        char msg[MAXSZ];
        /*writeing process starts */
        // cout<<n<<" character command to be sent"<<endl;
        bzero(msg,MAXSZ);
        int temp = n;
        strcpy(msg, command.c_str());
        msg[MAXSZ-1]=0;

        // t = clock();
        gettimeofday(&begin, NULL);
        // do{
        temp -= write(sockfd, msg, MAXSZ);
        // }while(temp>0);
        // cout<<"Complete command with "<<n<<" characters sent."<<endl;
        /*writeing process ends*/

        /* receiving process starts */
        bzero(msg,MAXSZ);
        int n_transfer = read(sockfd, msg, MAXSZ);
        // t = clock() - t;
        gettimeofday(&end, NULL);
        return double ((end.tv_sec - begin.tv_sec) * 1000000.00 + (end.tv_usec - begin.tv_usec)) / 1000000.00;
        // cout<<n_transfer<<" Bytes received"<<endl;
        msg[n_transfer] = 0;
        cout<<msg<<endl;
        /*receiving process ends*/
      }
      else cout<<"Invalid command."<<endl;
    }
    catch (const char* msg){
      cerr << msg << endl;
    }
    // return (end.tv_usec - begin.tv_usec) * 1000;
}


void handle_sigint(int sig);

void* testThread( void* data){
    // signal(SIGINT, handle_sigint);

    int sockfd;//to create socket
    struct sockaddr_in serverAddress;//client will connect on this
    struct hostent *server;

    int *thread_id = (int*) data;
    int n = *thread_id;
    requests[n] = 0;
    int key;
    string command;
    cout<<"testing started for thread "<<n<<"\n";
    try{
    executeCommand("connect 127.0.0.1 5500", sockfd, serverAddress, server);
    requests[n]++;
    while(1){

        key = rand() % 10000;
        command = "create "+ to_string(key) + " 10 abcdefghij";
        responseTime[n] += executeCommand( command, sockfd, serverAddress, server);
        requests[n]++;

        key = rand() % 10000;
        command = "read "+ to_string(key);
        responseTime[n] += executeCommand( command, sockfd, serverAddress, server);
        requests[n]++;

        key = rand() % 10000;
        command = "update " + to_string(key) + " 10 abcdefghij";
        responseTime[n] += executeCommand( command, sockfd, serverAddress, server);
        requests[n]++;

        key = rand() % 10000;
        command = "delete "+ to_string(key);
        responseTime[n] += executeCommand( command, sockfd, serverAddress, server);
        requests[n]++;

    }
    // cout<<endl;
    executeCommand("disconnect", sockfd, serverAddress, server);
    requests[n]++;
}
    catch(const char *msg){
        cout<<"Error inside thread "<<n<<endl;
    }
}

int calTotalRequests(int *requests, int NUM_OF_WORKERS);
double evaluateThroughput( int totalRequests, int time);
double evaluateResponseTime( double *responseTime, int totalRequests, int NUM_OF_WORKERS);

int main(int argc, char *argv[]){
    cout<<"Enter the number of threads: ";
    cin>>NUM_OF_WORKERS;
    // requests = (int*)malloc( sizeof(int) * NUM_OF_WORKERS);
    // responseTime = (double*)malloc( sizeof(double) * NUM_OF_WORKERS);
    // NUM_OF_WORKERS = argv[0];
    int timeSec;
    cout<<"Enter the time in seconds: ";
    cin>>timeSec;
    // timeSec = argv[1];
    int worker_thread_id[NUM_OF_WORKERS]={0};
    for(int i = 0; i < NUM_OF_WORKERS; i++){
        worker_thread_id[i] = i;
    }

    pthread_t worker_thread[NUM_OF_WORKERS];

    try{
        for(int i = 0; i < NUM_OF_WORKERS; i++){
            cout<<i<<endl;
            pthread_create(&worker_thread[i], NULL, testThread, (void*)&worker_thread_id[i]);
        }
    }catch(const char* msg){
        // cerr << msg << endl;
        cout<<"Error in creating thread"<<endl;
    }

    int timeMilli = timeSec*1000;
    sleep(timeSec);

    cout<<"Time completed."<<endl;
    // free(responseTime);
    // free(requests);

    // pthread_cancel(master_thread,NULL);
    for(int i=0;i<NUM_OF_WORKERS;i++){
        try{
            pthread_cancel(worker_thread[i]);
        }
        catch(const char* msg){
            // cerr << msg << endl;
            cout<<"Error in killing thread"<<endl;
        }
    }
    cout<<"*************************************************"<<endl;
    int totalRequests = calTotalRequests(requests, NUM_OF_WORKERS);
    cout<<"Total requests generated: "<< totalRequests<<endl;
    cout<<"Throughput is "<<evaluateThroughput( totalRequests, timeSec)<<" req/sec"<<endl;
    cout<<"Average response time is "<<evaluateResponseTime( responseTime, totalRequests, NUM_OF_WORKERS)<<endl;
    cout<<"*************************************************"<<endl;
    printf("Program ended\n");
    return 0;
}

int calTotalRequests(int *requests, int NUM_OF_WORKERS){
    int sum = 0;
    for ( int i = 0; i< NUM_OF_WORKERS; i++){
        sum += requests[i];
    }
    return sum;
}
double evaluateThroughput( int totalRequests, int time){
    return double( totalRequests/ time);
}
double evaluateResponseTime( double *responseTime, int totalRequests, int NUM_OF_WORKERS){
    double sum = 0.0;
    for ( int i = 0; i< NUM_OF_WORKERS; i++){
        sum += responseTime[i];
    }
    return double(sum/totalRequests);
}
void error(char *msg){
    perror(msg);
    exit(0);
}
