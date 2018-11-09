#include <sys/types.h>//socket
#include <sys/socket.h>//socket
#include <sys/wait.h>
#include <netinet/in.h>//INADDR_ANY
#include <arpa/inet.h>
#include <bits/stdc++.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
using namespace std;

#define NUM_OF_WORKERS 500
#define MAXSZ 256

// shared variables
map <int, string> keyValue;        // empty map container
int cnt = 0;
int worker_thread_id[NUM_OF_WORKERS]={0};
pthread_t worker_thread[NUM_OF_WORKERS];
int ret;
// locks and condition variables
pthread_mutex_t my_mutex = PTHREAD_MUTEX_INITIALIZER;

struct arg_struct {
    int arg1;
    int arg2;
};
struct arg_struct args[NUM_OF_WORKERS];
void error(char *msg);

int sockfd;//to create socket
struct sockaddr_in clientAddress;//server writes to client on this address
socklen_t clientAddressLength = sizeof(clientAddress);

void setup_server_sock(int argc, char *argv[]);

void process(char *str,int thread_n, int newsockfd){
    string command = string(str);
    // cout<<command<<"\n";
    stringstream ss(command);
    string operation;
    ss>>operation;
    int key;
    ss>>key;

    // pthread_mutex_lock(&my_mutex);
    map<int, string>::iterator it = keyValue.find(key);
    int n;
    if(operation.compare("create")==0){
        // cout<<"Thread_"<<thread_n<<" "<<str<<endl;
        int value_size;
        string value;
        ss>>value_size;
        ss>>value;
        if(it != keyValue.end()){   //element found;
            n = write(newsockfd,"Key-Value already present.", sizeof("Key-Value already present."));
            // cout<<n<<"bytes sent"<<endl;
            return;
        }

        keyValue.insert(pair <int, string> (key, value));
        n = write(newsockfd,"OK", sizeof("OK"));
        // cout<<n<<"bytes sent"<<endl;
        // pthread_mutex_unlock(&my_mutex);
        return;
    }
    if(operation.compare("read")==0){
        // cout<<"Thread_"<<thread_n<<" "<<str<<endl;
        // // cout<<"key----------------->"<<key<<endl;
        if(it == keyValue.end())
        {   //element found;
            n = write(newsockfd,"No value for this key exists.", sizeof("No value for this key exists."));
            // cout<<n<<"bytes sent"<<endl;
            return;
        }
        string msg = string(it->second);
        n = write(newsockfd,msg.c_str(), msg.length());
        // cout<<n<<" bytes sent"<<endl;
        return;
    }
    if(operation.compare("update")==0){
        // pthread_mutex_lock(&my_mutex);
        if(it == keyValue.end()){
            n = write(newsockfd,"No value for this key exists.", sizeof("No value for this key exists."));
            // cout<<n<<"bytes sent"<<endl;
            return;
        }

        // cout<<"Thread_"<<thread_n<<" "<<str<<endl;
        int value_size;//= atoi(strtok(NULL, " "));
        string value;// = strtok(NULL, " ");
        ss>>value_size;
        ss>>value;

        keyValue[key]=value;
        n = write(newsockfd, "OK", sizeof("OK"));
        // cout<<n<<"bytes sent"<<endl;
        // // cout<<"updated"<<endl;
        // pthread_mutex_unlock(&my_mutex);
        return;
    }

    if(operation.compare("delete")==0){
        // pthread_mutex_lock(&my_mutex);
        // cout<<"Thread_"<<thread_n<<" "<<str<<endl;
        // // cout<<"key----------------->"<<key<<endl;
        if(it == keyValue.end()){
            //element not found;
            n = write(newsockfd,"No value for this key exists.", sizeof("No value for this key exists."));
            // cout<<n<<"bytes sent"<<endl;
            return;
        }
        int num = keyValue.erase(key);
        if(num<1) {
            n = write(newsockfd, "Unable to delete", sizeof("Unable to delete"));
            // cout<<n<<"bytes sent"<<endl;
        }
        else {
            n = write(newsockfd, "OK", sizeof("OK"));
            // cout<<n<<"bytes sent"<<endl;
        }
        // pthread_mutex_unlock(&my_mutex);
    }
    return;
}

void* thread_fun(void*(data)){
    // pthread_mutex_unlock(&my_mutex);
    // pthread_detach(pthread_self());
    struct arg_struct *args = (struct arg_struct *)data;
    int newsockfd = args->arg1;
    int thread_n = args->arg2;
    printf("thread_%d connected to client: %s\n",thread_n, inet_ntoa(clientAddress.sin_addr));
    char str[10];
    sprintf(str, "%d", thread_n);
    int bytes_trans = write(newsockfd, str, 10);
    // // cout<<bytes_trans<<"bytes sent"<<endl;
    while(1){

        char msg[MAXSZ];
        bzero(msg,MAXSZ);
        int n=read(newsockfd,msg,MAXSZ);
        if(n<=1)  continue;
        // cout<<n<<"bytes received"<<endl;

        pthread_mutex_lock(&my_mutex);
        if(strcmp(msg,"disconnect")==0){
            // pthread_mutex_lock(&my_mutex);
            cnt--;
            int n_write = write(newsockfd,"OK:disconnected from server", sizeof("OK:disconnected from server"));
            // cout<<n_write<<"bytes sent"<<endl;
            close(newsockfd);
            // cout<<"Thread_"<<thread_n<<" disconnected"<<endl;
            worker_thread_id[thread_n] = 0;
            pthread_mutex_unlock(&my_mutex);
            // return NULL;
            pthread_exit(NULL);
        }
        else {
            process(msg, thread_n, newsockfd);
        }
        pthread_mutex_unlock(&my_mutex);
    }
}

void* master_thread_fun(void *){
    int newsockfd[NUM_OF_WORKERS] = {0};
    int index=0;

    cout<<"Server is open to accept "<<NUM_OF_WORKERS<<" connections.\n";
    while(1){

        cout<<index<<endl;
        newsockfd[index] = accept(sockfd,(struct sockaddr*)&clientAddress,&clientAddressLength);
        if(newsockfd[index]>0){
            pthread_mutex_lock(&my_mutex);
            // cout<<"New request arrived";
            if(cnt<NUM_OF_WORKERS){
                int bytes_trans = write(newsockfd[index],"1",2); // 1 = successfully connected
                // cout<<bytes_trans<<"bytes sent"<<endl;
                cnt++;
                int i;
                // create worker threads
                // for(i = 0; i < NUM_OF_WORKERS; i++){
                    // if(worker_thread_id[i]==0){
                        worker_thread_id[index]=1;

                        args[index].arg1 = newsockfd[index];
                        args[index].arg2 = index;
                        // printf("index %d\n", args[index].arg2);

                        // cout<<"Allocated to thread "<<i<<endl;
                        pthread_create(&worker_thread[index], NULL, thread_fun, (void*)&args[index]);
                        // pthread_mutex_unlock(&my_mutex);
                        //break;
                    //}
                //}
                // cout<<"Returned to listening\n";
            }
            else{
                int bytes_trans = write(newsockfd[index],"0",2);
                // cout<<bytes_trans<<"bytes sent"<<endl;
                if(close(newsockfd[index])==0)  cout<<"Can't accept anymore connections.\n";
            }
            index++;
            pthread_mutex_unlock(&my_mutex);
        }

        for(int i=0;i<NUM_OF_WORKERS;i++){
          // pthread_cond_broadcast(&empty);
          // pthread_join(worker_thread[i],NULL);
        }
    }
}
int main(int argc, char *argv[]){
    setup_server_sock(argc, argv);
    //cout<<"enter the number of threads to be spawned:\t";
    int master_thread_id = 0;
    pthread_t master_thread;
    //create master thread
    pthread_create(&master_thread, NULL, master_thread_fun, (void *)&master_thread_id);
    pthread_join(master_thread, NULL);
    printf("Program ended\n");
    return 0;
}

//below are the final functions
void error(char *msg){
    perror(msg);
    exit(1);
}

void setup_server_sock(int argc, char *argv[]){
    if (argc < 3) {
		fprintf(stderr,"ERROR, too low arguments\n");
		exit(1);
	}
    //create socket
    sockfd=socket(AF_INET,SOCK_STREAM,0);
    if(sockfd<0) error("unable to create server socket");
    int opt=1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        error("setsockopt");
    }
    //initialize the socket addresses
    struct sockaddr_in serverAddress;//server receive on this address
    bzero((char*)&serverAddress, sizeof(serverAddress));
    serverAddress.sin_family=AF_INET;
    serverAddress.sin_addr.s_addr= inet_addr(argv[1]);
    // serverAddress.sin_addr.s_addr=htonl(INADDR_ANY);
    serverAddress.sin_port=htons(atoi(argv[2]));
    //bind the socket with the server address and port
    if(bind(sockfd,(struct sockaddr *)&serverAddress, sizeof(serverAddress))<0) error("error on binding");
    //listen for connection from client
    listen(sockfd,NUM_OF_WORKERS+1);
}
