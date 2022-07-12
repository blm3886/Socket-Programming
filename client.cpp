 #include <stdio.h>
 #include <stdlib.h>
 #include <unistd.h>
 #include <errno.h>
 #include <string.h>
 #include <netdb.h>
 #include <sys/types.h>
 #include <netinet/in.h>
 #include <sys/socket.h>
 #include <arpa/inet.h>
 #include <sys/wait.h>
 #include <iostream>

 using namespace std;
 /**
 *  client.cpp.
 *  The client progam communicates with the main server (ServerM.cpp) over TCP connection.
 *  The cleint program accepts command line agruments and has two main operations 
 *  1. CHECK WALLET : This operation requests the current balance of the client from the backend server.
 *                    command line argument -> ./client clientname
 *
 *  2. TXCOINS      : This operation requests to transfer coins from user1's account to user2's account.
 *                    each user has an initial balance of 1000 txcoins.           
 *                    command line argumet -> ./cleint username_transcationOut username_transcationIn transaction_Amount.
 **/

 #define LOCALHOST "127.0.0.1" // host address
 #define serverM_PORT 25940    //Server M port
 #define MAXDATASIZE 1024

 int sockfd;
 struct sockaddr_in serverM_addr;
 char inputBuffer[MAXDATASIZE]; //input buffer for data.

//variables for TXCOINS operations
 string transferOutName; 
 string transferInName; 
 string transferAmt;
 string clientName; 

 //functions to connect to the main serverM.
 void createClientSocket();
 void initilizeConnection_ServerM();
 void connectToServer();

/**
 * Function to create TCP for communicating with ther serverM.
 * This function has been implemented using the refrence from Beej's guide chapter 5.
**/
 void createClientSocket(){
     sockfd = socket(AF_INET,SOCK_STREAM,0);
     if(sockfd == -1){
         perror("Client cannot open socket");
         exit(1);
     }
 }
 void initilizeConnection_ServerM(){
     memset(&serverM_addr,0,sizeof(serverM_addr));
     serverM_addr.sin_family = AF_INET;
     serverM_addr.sin_addr.s_addr = inet_addr(LOCALHOST);
     serverM_addr.sin_port = htons(serverM_PORT);
 }
 void connectToServer(){
    if(connect(sockfd,(struct sockaddr *)&serverM_addr, sizeof(serverM_addr)) == -1){
        cout << "Failed";
    }
    else{
     //cout << "Connect successful";
    }
 }

/**
 * The main program reads the command line arguments and based on the number of inputs 
 * determines the types of operation that has to be performed.
 **/
 int main(int argc, char *argv[]){
    createClientSocket();
    initilizeConnection_ServerM();
    connectToServer();
    cout << "\nThe Client is up and running"<<endl;;

     if(argc == 4){
         /******************************TXCOIN OPERATION*******************************************/
         transferOutName = argv[1];
         transferInName = argv[2];
         transferAmt = argv[3];

         //Preparing input buffer to send to main serverM
         // The message sent to the serverM is of the form : "2_transferOutName_TransferInName_transAMount". 
         // The serverM with decode "2" as  TXCOIN operation. 
          string txCOIN ="2_"+transferOutName+"_"+transferInName+"_"+transferAmt;
          bzero(inputBuffer,MAXDATASIZE); // crearing the inputBuffer
          strcpy(inputBuffer,txCOIN.c_str());

          if(send(sockfd,inputBuffer,sizeof(inputBuffer),0) == -1){
             perror("Cient");
             close(sockfd);
             exit(1);
          }

          cout << transferOutName+" has requested to transfer "+ transferAmt+" txcoins to "+transferInName+"."<< endl;
          char walletBalance[MAXDATASIZE];
          bzero(walletBalance,MAXDATASIZE);
          //response from ther serverM about the transcation status.
          if(recv(sockfd,walletBalance,MAXDATASIZE,0)== -1){
            perror("Receivening from ServerM:");
          }
          cout << walletBalance << endl;
     }
     else{
          /******************************CHECK WALLET OPERATION*******************************************/
         clientName = argv[1];
         //Preparin the inputBuffer which contains the message sent to the serverM
         // the message is of the form "1_clientName"
         // The main serverM would decode the "1" as checkwallet operation.
         string txWALLET ="1_"+clientName;
         bzero(inputBuffer,MAXDATASIZE); // crearing the inputBuffer
         strcpy(inputBuffer,txWALLET.c_str());
         
        if(send(sockfd,inputBuffer,sizeof(inputBuffer),0) == -1){
             perror("Cient");
             close(sockfd);
             exit(1);
         }
         cout << clientName+" sent a balance enquiry request to the main server."<< endl;

         char walletBalance[MAXDATASIZE];
         bzero(walletBalance,MAXDATASIZE);
         //resposne from the serverM about the balance of the user.
         if(recv(sockfd,walletBalance,MAXDATASIZE,0)== -1){
              perror("Receivening from ServerM:");
         }
         // if the user is not part of the network the main server return ABSENT keyword.
         if(string(walletBalance)== "ABSENT"){
            cout << "The input=\""+clientName+"\" is not present in the network."<< endl;
         }
         else{  
            cout << "The current balance of \""+clientName+"\" is "<< walletBalance <<" txcoins."  << endl;
         }
     }
}
