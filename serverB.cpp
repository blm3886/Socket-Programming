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
 #include <cstring>
 #include <fstream>
 #include <sstream>
 #include <map>
 #include <set>
 #include <vector>

 using namespace std;
  /**
  * serverB.cpp
  * Server B has access to file block2.txt that contains intitial transactions.
  * All the data in the block2.txt is in encrypted from.
  * performs operation as per requests made by the main serverM and returns data to the main serverM.
  **/
 #define LOCALHOST "127.0.0.1" // host address
 #define SERVER_B_PORT 22940    // TCP port with Client
 #define BACKLOG 10 // max number of pending connections.
 #define MAXDATASIZE 1024
 #define ERROR -1

 int sockfd_server_B;   //UDP socket.
 struct sockaddr_in server_B_addr; //server Address
 struct sockaddr_in server_M_addr; //Client Address

 socklen_t serverMAddrSize;
 socklen_t clientaddrSize;
 char recBuffer[MAXDATASIZE]; // size of input buffer which is data recieved from the serverM.

//data structures used to store transaction data from block1.txt. 
 set<string> transctionNames;                    //set used to store all the names present in the block2.txt
 map<string,vector<string> > coinsRecieved;      //map which maps each user with coins recieved.
                                                 //key - username and value - vector contaning coins recieved.

 map<string,vector<string> > coinsSent;          //map which maps each user with coins recieved.
                                                 //key - username and value - vector contaning coins recieved.

 set<int> transctionSerial;                      // stores all the serial numbers in the block2.txt.

 /**
 *  This method reads the block2.txt when the program starts.
 *  It extracts the data from the block2.txt and stores in the data structures metioned above.
 *  If the file conatins empty line - the program first create a new file with in correct format
 *  then procees with storing the data.
 **/
 void readBlockFile(){
    string line;
    bool flagCheck = false;

    transctionNames.clear();
    coinsRecieved.clear();
    coinsSent.clear();
    transctionSerial.clear();

    ifstream block2("block2.txt");
    while(getline (block2,line)){
      string transLine[4];    
      string word = "";
      int i =0;
      
     if(line.empty()){
        block2.close();
        //If empty line is found rewrite the file by correctly formatting the blank spaces.
         string tempLine="";

        //Opening a new file in write mode.
        ofstream tempFile("temp.txt");
        ifstream oldBlock2("block2.txt");
        //writing all the contents from block2.txt to temp.txt.
          while(getline (oldBlock2,tempLine)){
             if(!tempLine.empty()){
              tempFile << tempLine+"\n";
              }
     
          }

          oldBlock2.close();
          tempFile.close();

        //Removing the original file.
        remove("block2.txt");
        //Renaming the new file with the block2.txt.
        rename("temp.txt","block2.txt");
        remove("temp.txt");
        flagCheck = true;
        break;
     }
      else{
      for(auto x:line){  
          if(x == ' '){
              transLine[i] = word;
              i++; 
              word = "";
          }
          else{
              word = word + x;
          }
      }
      transLine[i] = word;   
      transctionNames.insert(transLine[1]);
      transctionNames.insert(transLine[2]);

      //ADIING SERIAL NUMBERS 
      transctionSerial.insert(stoi(transLine[0]));
      //Recording sent Transction.
      auto sentValue = coinsSent.find(transLine[1]);
      if(sentValue != coinsSent.end()){
        //Transction names alreay present
        coinsSent[transLine[1]].push_back(transLine[3]);
      }
      else{
        //Transcation name not present, so will add to map.
        coinsSent.insert(pair<string,vector<string>>(transLine[1],{transLine[3]}));
      }
      //Recording recieved Transaction
      auto recValue = coinsRecieved.find(transLine[2]);
      if(recValue != coinsRecieved.end()){
        //Transction names alreay present
        coinsRecieved[transLine[2]].push_back(transLine[3]);
      }
      else{
        //Transcation name not present, so will add to map.
        coinsRecieved.insert(pair<string,vector<string>>(transLine[2],{transLine[3]}));
      }
    }
  }
   if(flagCheck){
    //calling readBlockFile() again to store the file data into the data structures for further processing.
    readBlockFile();
   }
   else{
    block2.close();
   }
}

/**
 * Function to create UDP socket listening and communicating with Server M.
 * This function has been implemented using the refrence from Beej's guide chapter 5.
 **/
void createSocket(){
    sockfd_server_B = socket(AF_INET,SOCK_DGRAM,0);
    if(sockfd_server_B == ERROR){
      perror("Server B");
      exit(1);
    }
}
void connectnBind(){
    memset(&server_B_addr,0,sizeof(server_B_addr)); // checking if struct is empty.
    server_B_addr.sin_family = AF_INET;
    server_B_addr.sin_addr.s_addr = inet_addr(LOCALHOST);
    server_B_addr.sin_port = htons(SERVER_B_PORT);


    if(bind(sockfd_server_B,(struct sockaddr*)&server_B_addr,sizeof(server_B_addr)) == ERROR){
        perror("ServerB binding");
        exit(1);
    }
}

 /**
 * The method recieves the client Name as the Input and returns the sent and the recieved coins 
 * associated with the clientName if the client is present else it return "ABSENT".
 *
 *  @param clientName  : The client Name to search in block2.txt as receievd from serverM.
 *  @return returnData : The sent and recieved coins asociated with client in string format as "S_AMT_AMT_R_AMT_AMT".  
 **/
string checkWallet(string clientName){
   string returnData = "";

   //Checking if valid clientNAME;
   if(transctionNames.find(clientName) == transctionNames.end()){
      //client name not in transaction list;
      returnData = returnData+"ABSENT";
   }
   else{
     //client name valid so extracting sent and recieved amount;
     //Check the client in sent list
     auto sentValue = coinsSent.find(clientName);
     if(sentValue != coinsSent.end()){
        returnData = returnData+"S";  //flag for sent amount.
        vector<string> sentAmounts = sentValue->second;
        string sentAmountValues = "";

        for(int a = 0; a< sentAmounts.size(); a++){ 
          sentAmountValues = sentAmountValues + "_"+sentAmounts.at(a);
       }
        returnData = returnData+sentAmountValues+"_";
     }
     //checking the client name with recieved list.
      auto recValue = coinsRecieved.find(clientName);
      if(recValue != coinsRecieved.end()){
        returnData = returnData+"R";  //flag for sent amount.
        vector<string> recAmounts = recValue->second;
        string recAmountValues = "";
       for(int b = 0; b< recAmounts.size(); b++){
          recAmountValues = recAmountValues + "_"+recAmounts.at(b);
       }
       returnData = returnData+recAmountValues+"_";
     }
   }
   return returnData;
}

/**
  * This methods write the successful transaction sent from the severM to the block1.txt.
  * After writing the data to bock2.txt. the readBlockFile() is called again to  
  * update added transaction in the current state of the program.
 **/
void logTransaction(string logData){
     string filename("block2.txt");
     ofstream file_out;

     file_out.open(filename, ios_base::app);
     file_out << logData+"\n";
     file_out.close();
     readBlockFile();
}

 /**
  * The main method. Listens to the request sent by the serverM on the given port.
  * It provides the required data to the server M.
  *  1 : check wallet operation. 
  *      (message from server is of the from : "1_clientName").
  *      This operation sends the sent and the recieved transaction amount to the main serverM. 
  * 
  * 2: get the max serail number present in block2.txt file.
  *    (message from server is of the from : "2_(..data..)").
  *    This operation sends the max serial number presnt in block2.txt so far to main serverM.
  *
  * 3: write the successful transcation to block2.txt.
  *    (message from server is of the from : "3_(..data..)").
  *    This operation writes the tranaction in block2.txt and responds with done to main serverM.
  *
  * 4: ServerM requesting all the transactions recorded in block2.txt.
  *    (message from server is of the from : "4_(..data..)").
  *    This operation returns all the data from the block2.txt to main serverM.
  **/
int main(){
  readBlockFile();
  cout << "Server B is up and running using UDP port "+to_string(SERVER_B_PORT)+"."<< endl;
  createSocket();
  connectnBind();

       while(1){
        bzero(recBuffer,1024);
        clientaddrSize = sizeof(server_M_addr);
        recvfrom(sockfd_server_B,recBuffer,1024,0,(struct sockaddr*)&server_M_addr,&clientaddrSize);
        string operationType = strtok(recBuffer,"_"); 
        
        if(operationType != "2"){
        cout <<"The ServerB received request from the Main Server."<< endl;
        }
       
        if(operationType == "1"){
            /***************** CHECK WALLET *********************/
            string name = strtok(NULL," ");
            string responseData =  checkWallet(name);
          
            //RETURN THE DATA SEVER M 
            char buffer1[1024];
            bzero(buffer1,1024);
            strcpy(buffer1,responseData.c_str());
            if(sendto(sockfd_server_B,buffer1,1024,0,(struct sockaddr*)&server_M_addr,sizeof(server_M_addr))== ERROR){
            perror("Server B:");
            exit(1);
            }

            cout <<"The ServerB finished the response to the Main Server."<< endl;
       }
       else if(operationType == "2"){
           /***************** GET THE MAX TRANSCTION SERIAL NUMBER IN Block2.txt *********************/
           auto it = transctionSerial.rbegin(); // gets the max value from the set which contains all the serial numers.
           string maxSerialNum = to_string(*it);

            //RETURN MAX SERIAL NUM TO SERVER M 
            //RETURN THE DATA SEVER M 
            char buffer1[MAXDATASIZE];
            bzero(buffer1,MAXDATASIZE);
            strcpy(buffer1,maxSerialNum.c_str());
            if(sendto(sockfd_server_B,buffer1,1024,0,(struct sockaddr*)&server_M_addr,sizeof(server_M_addr))== ERROR){
            perror("ServerB:");
            }
       }
        else if(operationType == "3"){
          /***************** WRITE THE SUCCESSFUL TRANSACTION TO Block2.txt *********************/
            
            //WRITE TO LOG.
            string logMessage = strtok(NULL,"*");
            
            //Writing transaction in the log file.
            logTransaction(logMessage);
          
            char buffer1[MAXDATASIZE];
            bzero(buffer1,MAXDATASIZE);
            string resMessage = "logged";
            strcpy(buffer1,resMessage.c_str());
            if(sendto(sockfd_server_B,buffer1,1024,0,(struct sockaddr*)&server_M_addr,sizeof(server_M_addr))== ERROR){
            perror("ServerB ():");
            }
       } 
       else if(operationType == "4"){
            /***************** TXLIST operation *********************/
           //Send data from log file
            fstream fh;
            fh.open("block2.txt");
               string l;
               while(getline(fh,l)){
            // Sending each line to ServerM
               char buffer1[MAXDATASIZE];
               bzero(buffer1,MAXDATASIZE);
               strcpy(buffer1,l.c_str());
               if(sendto(sockfd_server_B,buffer1,1024,0,(struct sockaddr*)&server_M_addr,sizeof(server_M_addr))== ERROR){
               perror("ServerB:"); 
               }
            }
            string endMsg = "**END***";
            char buffer1[MAXDATASIZE];
            bzero(buffer1,MAXDATASIZE);
            strcpy(buffer1,endMsg.c_str());
            if(sendto(sockfd_server_B,buffer1,1024,0,(struct sockaddr*)&server_M_addr,sizeof(server_M_addr))== ERROR){
               perror("ServerB:");
            }
            fh.close(); 
            cout << "The ServerB finished sending response to the Main Server" << endl;             
       }
    }            
    close(sockfd_server_B);
  return 0;
}

