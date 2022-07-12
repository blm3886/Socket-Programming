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
 #include <vector>
 #include <sstream>
 #include <cstdlib>
 #include <map>
 #include <fstream>
 
 using namespace std;
/** serverM.cpp 
 *  The main server program that communicates with client.cpp and monitor.cpp over TCP
 *  and communicates with the backedn servers (serverA, serverB and serverC) usinh UDP connection.
 *
**/

 #define LOCALHOST "127.0.0.1"       // host address
 #define SERVER_A_PORT 21940         // UDP port (ServerA)
 #define SERVER_B_PORT 22940         // UDP port (ServerB)
 #define SERVER_C_PORT 23940         // UDP port (ServerC)
 #define SERVER_M_UDP_PORT 24940     // UDP port (ServerM)

 #define SERVERM_CLIENT_PORT 25940   // TCP port for Client
 #define SERVERM_MONITOR_PORT 26940  // TCP port for Monitor
 #define BACKLOG 10                  // max number of pending connections.
 #define MAXDATASIZE_CLIENT 1024
 #define MAXDATASIZE 1024             // max data size of buffer.
 #define INITIAL_BALANCE 1000         //Initial wallet amount given to every person.

 //Global variables
 int sockfd_ClientParent, sockfd_clientChildSock;        //Parent and child socket for client. 
 int sockfd_MonitorParent, sockfd_monitorChildSock;      //Parent and child socket for Monitor.
 int sockfd_ServerM_UDP;
 int sockfd_serverM_UDPClient; 

 int sockfd_Server_A;                                    //Socket for server A
 int sockfd_Server_B;                                    //Socket for server B
 int sockfd_Server_C;                                    //Socket for server C
 int option = 1;

 
 socklen_t monitorAddrSize;
 socklen_t clientAddrSize;


//map to store transaction logs before writing to txlist.txt.
map<int,string> transactionLog;        

//Source address
 struct sockaddr_in serverM_Clientaddr;
 struct sockaddr_in serverM_Monitoraddr;
 struct sockaddr_in serverM_UDPaddr;
 struct sockaddr_in serverM_ClientaddrUDP;


 //Destinattion addresses
 struct sockaddr_in client_addr;
 struct sockaddr_in monitor_addr;
 struct sockaddr_in MserverA_addr;
 struct sockaddr_in MserverB_addr;
 struct sockaddr_in MserverC_addr;


 char input_Buffer[MAXDATASIZE_CLIENT];
 char udp_Buffer[MAXDATASIZE];


 //functions for Client connection
 void create_ClientTCP();
 void listenToClient();

//Function for minotor connection
 void create_MonitorTCP();
 void listenToMonitor();

 //Function for UDP socket.
 void createUDPSock_ServerM();
 void connect_ServerA();
 void connect_ServerB();
 void connect_ServerC();

 //Other Functions
 string decryptAmount(string amount);
 string decryptNames(string name);
 
 
/**
 * Function to create TCP socket and bind with port 25942 for listening and communicating with client.
 * This function has been implemented using the refrence from Beej's guide chapter 5.
**/ 
void create_ClientTCP(){
    sockfd_ClientParent = socket(AF_INET,SOCK_STREAM,0);
    if(sockfd_ClientParent == -1){
        perror("Client socket Failed");
        exit(1);
    }

    if(setsockopt(sockfd_ClientParent,SOL_SOCKET,SO_REUSEADDR,&option,sizeof option) == -1){
        perror("setsockopt Client");
        exit(1);
    }
    
        // binding socket to IP/port.
    memset(&serverM_Clientaddr,0,sizeof(serverM_Clientaddr)); // checking if struct is empty.
    serverM_Clientaddr.sin_family = AF_INET;
    serverM_Clientaddr.sin_addr.s_addr = inet_addr(LOCALHOST);
    serverM_Clientaddr.sin_port = htons(SERVERM_CLIENT_PORT);



    if(bind(sockfd_ClientParent,(struct sockaddr*)&serverM_Clientaddr,sizeof serverM_Clientaddr) == -1){
        perror("Server Binding with Client Error:");
        exit(1);
    }
}

/**
 * Function used to listen to request from client.
 * This function has been implemented using the refrence from Beej's guide chapter 5.
**/ 
void listenToClient(){
    if(listen(sockfd_ClientParent, BACKLOG) == -1){
        perror("Error listening Client");
        exit(1);
    }
}


/**
 * Function to create TCP socket and bind over port 26940 for communicating with client.
 * This function has been implemented using the refrence from Beej's guide chapter 5.
**/ 
void create_MonitorTCP(){
    sockfd_MonitorParent = socket(AF_INET,SOCK_STREAM,0);
    if(sockfd_MonitorParent == -1){
        perror("Monitor socket Failed");
        exit(1);
    }

    if(setsockopt(sockfd_MonitorParent,SOL_SOCKET,SO_REUSEADDR,&option,sizeof option) == -1){
        perror("setsockopt Monitor");
        exit(1);
    }

    // binding socket to IP/port.
    memset(&serverM_Monitoraddr,0,sizeof(serverM_Monitoraddr)); // checking if struct is empty.
    serverM_Monitoraddr.sin_family = AF_INET;
    serverM_Monitoraddr.sin_addr.s_addr = inet_addr(LOCALHOST);
    serverM_Monitoraddr.sin_port = htons(SERVERM_MONITOR_PORT);

    if(bind(sockfd_MonitorParent,(struct sockaddr*)&serverM_Monitoraddr,sizeof serverM_Monitoraddr) == -1){
        perror("Server Binding with monitor Error:");
        exit(1);
    }
}

/**
 * Function used to listen to request from client.
 * This function has been implemented using the refrence from Beej's guide chapter 5.
**/ 
void listenToMonitor(){
    if(listen(sockfd_MonitorParent, BACKLOG) == -1){
        perror("Error listening Monitor");
        exit(1);
    }
}

/**
 *  This function stores the transactions in a map when the monitor requests TXLIST option.
 *  The transactio log in decrypted form in stored in mao before writing to file.
 *  (part of TXLIST operation).
**/
void storeTransactionLog(string sr_no, string username1, string username2, string tnsAmt){
    //Decrypting data.
    username1 = decryptNames(username1); 
    username2 = decryptNames(username2);
    string tnsAmt_d =  decryptAmount(tnsAmt);      

    string record = sr_no+" "+username1+" "+username2+" "+tnsAmt_d+"\n";
    //storing the transactions in map.
    transactionLog.insert(pair<int,string>(stoi(sr_no),record));
}


/** 
 *  Function is used to connect to backend serverA, serverB and serverC
 *  using UDP connction to request for all the transactions logs.
 *  (part of TXLIST operation).
 * 
 *  @param port : The port number of the backend server.
**/
void connectServer_TransLog(int port){
  int sockfd;   //UDP socket.
  struct sockaddr_in addr; //server Address
  char buffer[MAXDATASIZE];
  char recBuffer[MAXDATASIZE];
  socklen_t addrSize;

  sockfd = socket(AF_INET,SOCK_DGRAM,0);
    if(sockfd == -1){
      perror("Transaction Log");
      exit(1);
    }

    memset(&addr,0,sizeof(addr)); // checking if struct is empty.
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(LOCALHOST);
    addr.sin_port = htons(port);

    string message ="4_TXLIST";
    bzero(buffer,1024);
    strcpy(buffer,message.c_str());
    sendto(sockfd,buffer,1024,0,(struct sockaddr*)&addr,sizeof(addr));
    
    bzero(recBuffer,1024);
    addrSize = sizeof(addr);
    bool Endflag = true;
    while(Endflag){
        if(recvfrom(sockfd,recBuffer,1024,0,(struct sockaddr*)&addr, &addrSize)== -1){
           perror("Log Response:");
        }
        string data = string(recBuffer);
        
        if(data == "**END***"){
            Endflag = false;
        } 
        else{
          string sr_no_str =  strtok(recBuffer," ");
          string username1 =  strtok(NULL," ");
          string username2 =  strtok(NULL," ");
          string tnsAmt = strtok(NULL,"\n");
          storeTransactionLog(sr_no_str,username1,username2,tnsAmt);
     }
    }
    //cout << "The main server recieved transactions from Server "+server_Name+" using UDP over port "+to_string(port) <<endl;
    close(sockfd); 
}


/**
 * (UDP SERVER A)
 * Function is used to connect to backend serverA using UDP connction.
 * (part of CHECKWALLET and TXCOIN operation).
 * This connection requests the current balance of the user.
 * This function has been implemented using the refrence from Beej's guide chapter 5.
 *
 *  @param message : The message to be sent to the server. (decsribes the type of the operation). 
**/
string connect_ServerA(string message){
  string flag = message.substr(0,1);
  
  int sockfd;   //UDP socket.
  struct sockaddr_in addr; //server Address
  char buffer[1024];
  char recBuffer[1024];
  socklen_t addrSize;

  sockfd = socket(AF_INET,SOCK_DGRAM,0);
    if(sockfd == -1){
      perror("Server A");
      exit(1);
    }

    memset(&addr,0,sizeof(addr)); // checking if struct is empty.
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(LOCALHOST);
    addr.sin_port = htons(SERVER_A_PORT);

    bzero(buffer,1024);
    strcpy(buffer,message.c_str());
    sendto(sockfd,buffer,1024,0,(struct sockaddr*)&addr,sizeof(addr));
    cout << "The main server sent request to server A" << endl;

    bzero(recBuffer,1024);
    addrSize = sizeof(addr);
    
    if(recvfrom(sockfd,recBuffer,1024,0,(struct sockaddr*)&addr, &addrSize)== -1){
           perror("Server A Response:");
    }
    return recBuffer;
}

/**
 * (UDP SERVER B)
 * Function is used to connect to backend serverB using UDP connction.
 * (part of CHECKWALLET and TXCOIN operation).
 * This connection requests the current balance of the user.
 * This function has been implemented using the refrence from Beej's guide chapter 5. 
**/
string connect_ServerB(string message){
  string flag = message.substr(0,1); 
  int sockfd;   //UDP socket.
  struct sockaddr_in addr; //server Address
  char buffer[1024];
  char recBuffer[1024];
  socklen_t addrSize;

  sockfd = socket(AF_INET,SOCK_DGRAM,0);
    if(sockfd == -1){
      perror("Server B (UDP SOCKET CREATE)");
      exit(1);
    }

    memset(&addr,0,sizeof(addr)); // checking if struct is empty.
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(LOCALHOST);
    addr.sin_port = htons(SERVER_B_PORT);

    bzero(buffer,1024);
    strcpy(buffer,message.c_str());
    sendto(sockfd,buffer,1024,0,(struct sockaddr*)&addr,sizeof(addr));
    if(flag == "1"){
    cout << "The main server sent request to server B" << endl;
    }
    
    bzero(recBuffer,1024);
    addrSize = sizeof(addr);
    if(recvfrom(sockfd,recBuffer,1024,0,(struct sockaddr*)&addr, &addrSize)== -1){
       perror("Server B Response:");
    }
   return recBuffer;
}

/**
 * (UDP SERVER C)
 * Function is used to connect to backend serverC using UDP connction.
 * (part of CHECKWALLET and TXCOIN operation).
 * This connection requests the current balance of the user.
 * This function has been implemented using the refrence from Beej's guide chapter 5.
 * 
 * @param message : The message to be sent to the server. (decsribes the type of the operation). 
**/
string connect_ServerC(string message){
  string flag = message.substr(0,1);
  int sockfd;   //UDP socket.
  struct sockaddr_in addr; //server Address
  char buffer[1024];
  char recBuffer[1024];
  socklen_t addrSize;

  sockfd = socket(AF_INET,SOCK_DGRAM,0);
    if(sockfd == -1){
      perror("Server C (UDP SOCKET CREATE)");
      exit(1);
    }

    memset(&addr,0,sizeof(addr)); // checking if struct is empty.
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(LOCALHOST);
    addr.sin_port = htons(SERVER_C_PORT);

    bzero(buffer,1024);
    strcpy(buffer,message.c_str());
    sendto(sockfd,buffer,1024,0,(struct sockaddr*)&addr,sizeof(addr));
    
    if(flag == "1"){        
    cout << "The main server sent request to server C" << endl;
    }

    bzero(recBuffer,1024);
    addrSize = sizeof(addr);
    if(recvfrom(sockfd,recBuffer,1024,0,(struct sockaddr*)&addr, &addrSize)== -1){
       perror("Server C Response:");
    }
   return recBuffer;
}

/**
 * Function to decrypt the name of a user.
 * Accepts encrypted name in string format and return decrypted name in string form.
 * converts each caharcters having ASCII values of  97-122 and 65-90 to 3 places back. 
 *
 * @param name : the name of user in encrypted form.
 * @returns decryptedName : returns the decrypted name of the user in string form. 
**/
string decryptNames(string name){
    string decryptedName="";
    int encryptVal;

    for(int i = 0; i <name.length();i++){
        encryptVal = int(name[i]);
        if(encryptVal >= 97 && encryptVal <= 99 || encryptVal >= 65 && encryptVal <= 67){
            encryptVal = encryptVal+23;
        }
        else if(encryptVal >= 100 && encryptVal <= 122 || encryptVal >= 68 && encryptVal <= 90){
            encryptVal = encryptVal - 3;
        }
        decryptedName = decryptedName+char(encryptVal);
    }

    return decryptedName;
}

/**
 * Function to encrypt the name of a user.
 * Accepts client  name in string format and return encrypted name in string form.
 * offsets each caharcters having ASCII values of  97-122 and 65-90 to 3 places forward. 
 *
 * @param clientName : the name of user.
 * @returns encryptedName : returns the encrypted name of the user in string form. 
**/
string encryptNames(string clientName){
    string encryptedName="";
    int encryptVal;
    for(int i = 0; i <clientName.length();i++){
        encryptVal = int(clientName[i]);
        if(encryptVal >= 88 && encryptVal <= 90 || encryptVal >= 120 && encryptVal <= 122){
           encryptVal = encryptVal-26;
           encryptVal = encryptVal+3;
        }
        else if(encryptVal >= 65 && encryptVal <= 87 || encryptVal >= 97 && encryptVal <= 119){
          encryptVal = encryptVal +3;
        }
        encryptedName = encryptedName + char((encryptVal));
    }
    return encryptedName;
}

/**
 * Function to encrypt the amount of a user.
 * Accepts the amount of a user in string format and returns the encrypted amount in string form.
 * offsets number to 3 places forward. 
 *
 * @param value : the amount in string format.
 * @returns encryptedAmount : returns the encrypted name of the user in string form. 
**/
string encryptAmout(string value){
    string encryptedAmount = "";
    for(int i = 0; i < value.length(); i++){
        int val = value[i] - '0';
        if(val == 7){
            val = 0;
        }
        else if(val ==8){
            val = 1;
        }
        else if(val == 9){
            val = 2;
        } 
        else{
            val = val +3;
        }

        stringstream ss;
        string str;
        ss << val;
        ss >> str;
        encryptedAmount = encryptedAmount+str;
    }
    return encryptedAmount;
}

/**
 * Function to decrypt the amount of a user.
 * Accepts the amount of a user in string format and returns the decrypted amount in string form.
 * offsets number to 3 places back. 
 *
 * @param value : the amount in string format.
 * @returns decryptedAmount : returns the decrypted amount of the user in string form. 
**/
string decryptAmount(string value){
    //cout << "Initial Value" << value << endl;
    string decryptedAmount = "";
    for(int i = 0; i < value.length(); i++){
        int val = value[i] - '0';
        if(val == 0){
            val = 7;
        }
        else if(val == 1){
            val = 8;
        }
        else if(val == 2){
            val = 9;
        }
        else{
        val = val -3;
        }
        stringstream ss;
        string str;
        ss << val;
        ss >> str;
        decryptedAmount = decryptedAmount+str;
   }
    return decryptedAmount;
}

/**
 *  The method converts the data recieved from the serverA, serverB and ServerC in the required format.
 *  The data about sent and received coins are received from the backend server in the encrypted form.
 *  this method calls the other helper function to decrypt the sent and recieved amount of each user
 *   from all the three servers. 
 *
 * @param data : The data receieved from the backedn serverA, serverB and serverC conating sent and receieved amount.
 * @returns data : Returns the sent and received amount of each user in decrypted form. 
**/
string extractCoinsInfo(string data){
    vector<string>sentAmtArray;
    vector<string>recAmtArray;
    /**
    *  The sent and received data from the backend servers are in the format "S_AMT_AMT_R_AMT_AMT"
    *  the following code extracts the each amount from this string and decrypts its to the actual amount. 
    */
    if(data != "ABSENT"){
        bool R_found = false;
        bool endS = true;
        
        if(data[0] == 'S'){
            int strptr = 2;
            int len = data.length();
            for(int j = 2;j< len;j++){
                if(data[j] == '_'){
                    int offset = j - strptr;
                    sentAmtArray.push_back(data.substr(strptr,offset));
                    strptr = j+1;
                }
                else if(data[j] =='R'){
                    R_found = true;
                    data = data.substr(j,len-1);
                    break;
                }
            }
        }
      // RECIEVED AMOUNT  
      if(data[0] == 'R'){
            int strptr = 2;
            int len = data.length();
            for(int j = 2;j< len;j++){
                if(data[j] == '_'){
                    int offset = j - strptr;
                    recAmtArray.push_back(data.substr(strptr,offset));
                    strptr = j+1;
                }            
        }
          
    }


    //Decrypt the amount and caluclate the sent amount.
    int SentAmount = 0;
    for(int i=0; i <sentAmtArray.size();i++){
        SentAmount = SentAmount+ stoi(decryptAmount(sentAmtArray.at(i)));        
    }
   
    //Decrypt the amount and caluclate the recieved amount.
    int recAmount = 0;
    for(int i=0; i <recAmtArray.size();i++){
        recAmount = recAmount+ stoi(decryptAmount(recAmtArray.at(i)));        
    }
  
    string returnBal = to_string(SentAmount)+"_"+to_string(recAmount);  
    return returnBal;
    }
    
return data;

}

/**
 *  The method accepts the input message and the client socketdescriptor 
 *  and sends the appropriavte resposne to the client.cpp.\ 
 *  Extracts the recieved data.
 *  and performs the operation requested by the user.
 *
**/
void sendMessageToCleint(string message, int socketfd){
            char walletBuffer[MAXDATASIZE];
            bzero(walletBuffer,MAXDATASIZE);
            strcpy(walletBuffer,message.c_str());
            if(sendto(socketfd,walletBuffer,MAXDATASIZE,0,(struct sockaddr *)&client_addr,sizeof(client_addr))==-1){
            perror("SERVERM - CLIENT send");
        }
}

/**
 *  The method accepts the input socket connection. 
 *  Extracts the recieved data.
 *  and performs the operation requested by the user.
 *  
 * @param childSocket : The incoming socket connection.
 *
**/
void handle_Connection(int childSocket){
     
     int data = recv(childSocket,input_Buffer,MAXDATASIZE_CLIENT,0);
     if(data == -1){
         perror("Server M (input Buffer)");
         exit(1);
     }

     string recievedData = strtok(input_Buffer,"_");
     /**************************OPERATION 1 -- CHECKING BALANCE***********************************/ 
     if(recievedData == "1"){
        string name = strtok(NULL," ");
        string encyptName = encryptNames(name); 
        cout << "The main server received input=\""+name+"\" from the client using TCP over port 25942"<< endl;


        bool serverA_Check = false;
        bool serverB_Check = false;
        bool serverC_Check = false;
        
        //Variable to store the sent and recieved money from serverA,serverB and serverC.
        int sentMoney_A = 0;
        int recMoney_A = 0;

        int sentMoney_B = 0;
        int recMoney_B = 0;

        int sentMoney_C = 0;
        int recMoney_C = 0;


        //DATA FROM SERVER A
        string balFrom_A = extractCoinsInfo(connect_ServerA("1_"+encyptName));
        cout << "The main server recieved transactions from Server A using UDP over port "+to_string(SERVER_A_PORT)<<endl;
        //if any user data in present in server A the values are stored in the variable else the sent and recieved amount will be 0.
        if(balFrom_A != "ABSENT"){
        serverA_Check = true;
        sentMoney_A = stoi(balFrom_A.substr(0,balFrom_A.find("_")));
        recMoney_A =  stoi(balFrom_A.substr(balFrom_A.find("_")+1,balFrom_A.length()));
        } 

        //DATA FROM SERVER B
        string balFrom_B = extractCoinsInfo(connect_ServerB("1_"+encyptName));
        cout << "The main server recieved transactions from Server B using UDP over port "+to_string(SERVER_B_PORT)<<endl;
        //if any user data in present in server B the values are stored in the variable else the sent and recieved amount will be 0.
        if(balFrom_B != "ABSENT"){
           sentMoney_B = stoi(balFrom_B.substr(0,balFrom_B.find("_")));
           recMoney_B =  stoi(balFrom_B.substr(balFrom_B.find("_")+1,balFrom_B.length())); 
         }

        // DATA FROM SERVER C
        string balFrom_C = extractCoinsInfo(connect_ServerC("1_"+encyptName));
        cout << "The main server recieved transactions from Server C using UDP over port "+to_string(SERVER_C_PORT)<<endl;
        //if any user data in present in server C the values are stored in the variable else the sent and recieved amount will be 0.
        if(balFrom_C != "ABSENT"){        
           sentMoney_C = stoi(balFrom_C.substr(0,balFrom_C.find("_")));
           recMoney_C =  stoi(balFrom_C.substr(balFrom_C.find("_")+1,balFrom_C.length())); 
        }

        // If server A,B and C , respond with "ABSENT" for a user, means user is not present in the network and will have no balance.
        // sending message to client that user is not part of the network.
        if(balFrom_A == "ABSENT" && balFrom_B == "ABSENT" && balFrom_C == "ABSENT"){
            string msg = "ABSENT";
            char walletBuffer[MAXDATASIZE];
            bzero(walletBuffer,MAXDATASIZE);
            strcpy(walletBuffer,msg.c_str());
        
            if(sendto(childSocket,walletBuffer,MAXDATASIZE,0,(struct sockaddr *)&client_addr,sizeof(client_addr))==-1){
                perror("SERVERM - CLIENT send");
            }
        cout << "The main server sent the reply to the client" << endl;
        }
        else{
        // Condition executes when atleast one of (serverA , serverB or serverC) has the username.
        // so current balance is calculated.    
        int totalSENT_AMOUNT = sentMoney_A+sentMoney_B+sentMoney_C;
        int totalRECIEVED_AMOUNT = recMoney_A+recMoney_B+recMoney_C;

        //BALANCE CALCULATION.
        int currentWalletBalance = (INITIAL_BALANCE + totalRECIEVED_AMOUNT) - totalSENT_AMOUNT;
        char walletBuffer[MAXDATASIZE];
        bzero(walletBuffer,MAXDATASIZE);
        strcpy(walletBuffer,to_string(currentWalletBalance).c_str());
        
        //sending the current balance to the client.
        if(sendto(childSocket,walletBuffer,MAXDATASIZE,0,(struct sockaddr *)&client_addr,sizeof(client_addr))==-1){
            perror("SERVERM - CLIENT send");
        }
        cout << "The main server sent the current balance to the client" << endl;
        }
     }
     else if(recievedData[0] == '2'){
        /**************************OPERATION 1 -- TXCOIN OPERATION *****************************************/
        string username1 = strtok(NULL,"_");                  //tansaction out name.
        string username2 = strtok(NULL,"_");                  //transction in name.

        //converting the names to encrypted form.
        string username1_encrypt = encryptNames(username1);   
        string username2_encrypt = encryptNames(username2);
        string tnsamount = strtok(NULL,"_");                  //transcation amount.

        cout << "The main server received from "+username1+" to transfer "+tnsamount+" coins to "+ username2+"using TCP over port 25942" << endl;
        
                             //STEP1: checking if both the user are part of the network. 
        bool USER1_Present = false;
        bool USER2_Present = false;
        
        ////////////// CHECKING FOR USER 1 ////////////////////////
        //boolean varibles set to true if the user is part of the network.
        bool serverA_Check_user1 = false;
        bool serverB_Check_user1 = false;
        bool serverC_Check_user1 = false;

        //variables to store the sent and reieved amount from server A, serverB and server C.
        int user1_sentMoney_A = 0;
        int user1_recMoney_A = 0;

        int user1_sentMoney_B = 0;
        int user1_recMoney_B = 0;

        int user1_sentMoney_C = 0;
        int user1_recMoney_C = 0;

        // if any of the servers respond with "ABSENT" means the user is not part of the network and
        // sent and receieved amount of that user will be 0.
        // if the user is present in any of the three severs then,
        // the appropriate boolean variable(server(A|B|c)_Check) is set to true.
        
        //Data from server A.
        string user1_balFrom_A = extractCoinsInfo(connect_ServerA("1_"+username1_encrypt));
        if(user1_balFrom_A != "ABSENT"){
        serverA_Check_user1 = true;
        user1_sentMoney_A = stoi(user1_balFrom_A.substr(0,user1_balFrom_A.find("_")));
        user1_recMoney_A =  stoi(user1_balFrom_A.substr(user1_balFrom_A.find("_")+1,user1_balFrom_A.length())); 
        }

        //data from server B
        string user1_balFrom_B = extractCoinsInfo(connect_ServerB("1_"+username1_encrypt));
        if(user1_balFrom_B != "ABSENT"){
        serverB_Check_user1 = true;
        user1_sentMoney_B = stoi(user1_balFrom_B.substr(0,user1_balFrom_B.find("_")));
        user1_recMoney_B =  stoi(user1_balFrom_B.substr(user1_balFrom_B.find("_")+1,user1_balFrom_B.length())); 
        }


        ////data from server C.
        string user1_balFrom_C = extractCoinsInfo(connect_ServerC("1_"+username1_encrypt));
        if(user1_balFrom_C != "ABSENT"){
        serverC_Check_user1 = true;
        user1_sentMoney_C = stoi(user1_balFrom_C.substr(0,user1_balFrom_C.find("_")));
        user1_recMoney_C =  stoi(user1_balFrom_C.substr(user1_balFrom_C.find("_")+1,user1_balFrom_C.length())); 
        }

        
        //If the user is present in any one of the three backend servers then the first user is marked as present.
        if(serverA_Check_user1 || serverB_Check_user1 || serverC_Check_user1){
            USER1_Present = true;
        }

        /////////// CHECKING  FOR SECOND USER ///////
        bool serverA_Check_user2 = false;
        bool serverB_Check_user2 = false;
        bool serverC_Check_user2 = false;

        int user2_sentMoney_A = 0;
        int user2_recMoney_A = 0;

        int user2_sentMoney_B = 0;
        int user2_recMoney_B = 0;

        int user2_sentMoney_C = 0;
        int user2_recMoney_C = 0;

        //DATA FROM SERVER A.
        string user2_balFrom_A = extractCoinsInfo(connect_ServerA("1_"+username2_encrypt));
        if(user2_balFrom_A != "ABSENT"){
        serverA_Check_user2 = true;
        user2_sentMoney_A = stoi(user2_balFrom_A.substr(0,user2_balFrom_A.find("_")));
        user2_recMoney_A =  stoi(user2_balFrom_A.substr(user2_balFrom_A.find("_")+1,user2_balFrom_A.length())); 
        }

        //DATA FROM SERVER B.
        string user2_balFrom_B = extractCoinsInfo(connect_ServerB("1_"+username2_encrypt));
        if(user2_balFrom_B != "ABSENT"){
        serverB_Check_user2 = true;
        user2_sentMoney_B = stoi(user2_balFrom_B.substr(0,user2_balFrom_B.find("_")));
        user2_recMoney_B =  stoi(user2_balFrom_B.substr(user2_balFrom_B.find("_")+1,user2_balFrom_B.length())); 
        }

        //DATA FROM SERVER C.
        string user2_balFrom_C = extractCoinsInfo(connect_ServerC("1_"+username2_encrypt));
        if(user2_balFrom_C != "ABSENT"){
        serverC_Check_user2 = true;
        user2_sentMoney_C = stoi(user2_balFrom_C.substr(0,user2_balFrom_C.find("_")));
        user2_recMoney_C =  stoi(user2_balFrom_C.substr(user2_balFrom_C.find("_")+1,user2_balFrom_C.length())); 
        }

        //Printing the results after receiving responses from the server.
        cout << "The main server recieved feedback from Server A using UDP over port "+to_string(SERVER_C_PORT)<<endl;
        cout << "The main server recieved feedback from Server B using UDP over port "+to_string(SERVER_C_PORT)<<endl;
        cout << "The main server recieved feedback from Server C using UDP over port "+to_string(SERVER_C_PORT)<<endl;

        //If the user is present in any one of the three backend servers then the first user is marked as present.
        if(serverA_Check_user2 || serverB_Check_user2 || serverC_Check_user2){
            USER2_Present = true;
        }


        //CONDITION 1: When both the users are absent.
        if((!USER1_Present) && (!USER2_Present)){
           string message = "Unable to proceed with the transction as "+username1+"and"+username2+"are not part of the network.";
            ///SENDING MESSAGE TO CLIENT THAT TRANSCTION NOT POSSIBLE BCUZ OF USER1
            sendMessageToCleint(message,childSocket);
            cout << "The main server sent the resulst of transaction to the client." <<endl;
        } 
        //CONDITION 2: When the user1 is absent.
        else if(!USER1_Present){
            string message = "Unable to proceed with the transction as "+username1+" is not part of the network.";
            ///SENDING MESSAGE TO CLIENT THAT TRANSCTION NOT POSSIBLE BCUZ OF USER1
            sendMessageToCleint(message,childSocket);
            cout << "The main server sent the resulst of transaction to the client." <<endl;
        }
        //CONDITION 3: When the user2 is absent.
        else if(!USER2_Present){
            string message = "Unable to proceed with the transction as "+username2+" is not part of the network.";
            ///SENDING MESSAGE TO CLIENT THAT TRANSCTION NOT POSSIBLE BCUZ OF USER2
            sendMessageToCleint(message,childSocket);
            cout << "The main server sent the resulst of transaction to the client." <<endl;
        }
        //CONDITION 4: When user1 and user2 is present.
        else{
                //STEP 2: checking if a valid transaction is possible.

           //checking if senderBalance after transction is valid.
           int recMONEY = user1_recMoney_A+user1_recMoney_B+user1_recMoney_C;            
           int sentMONEY = user1_sentMoney_A+user1_sentMoney_B+user1_sentMoney_C;

        
           int currBal = ((INITIAL_BALANCE + recMONEY) -  sentMONEY);  // current Balance of the sender.
           int senderBalance = currBal- stoi(tnsamount);               // coins left in the sender account if the transcation were made.
           //CONDITION 1: Tranaction cannot be made, as sender balance too low.
           if(senderBalance < 0){
               //sending transactio unsuccessfull message to the cleint.
               string message = username1+" was unable to transfer "+tnsamount+" txcoins to "+username2+"beacuse of insufficient balance.\nThe current balance of "+username1+" is : "+to_string(currBal)+" txcoins.";
               sendMessageToCleint(message,childSocket);
               cout << "The main server sent the resulst of transaction to the client." <<endl;
           }
           //CONDITION 2: Transcation can be made as sender has valid balance.
           else{
               //TRANSCTION POSSIBLE
               //1.Get transction serial Number
               int sr_No_A;
               int sr_No_B;
               int sr_No_C;

               //Sending request to server A B C to get max transaction number that each server has.
               sr_No_A = stoi(connect_ServerA("2_"));
               sr_No_B = stoi(connect_ServerB("2_"));
               sr_No_C = stoi(connect_ServerC("2_"));

               
               // Computing the serial number for current successful transction.
               int finSerialNum = 0;
               if(sr_No_A >= sr_No_B && sr_No_A >= sr_No_C){
                   finSerialNum = sr_No_A;
               }
               else if(sr_No_B >= sr_No_A && sr_No_B >= sr_No_C){
                   finSerialNum = sr_No_B;
               }
               else{
                   finSerialNum = sr_No_C;
               }

               finSerialNum = finSerialNum + 1;

               //2. Randomly picker one backend server (A|B|C)to record the successful transaction.
                 srand((unsigned) time(NULL));
                 int random = 1+ (rand()%3);
               //3. Converting the data to encrypted form to send to the backend server.
                string trnsactionMessage = to_string(finSerialNum)+" "+username1_encrypt+" "+username2_encrypt+" "+encryptAmout(tnsamount)+"*";
                if(random == 1){
                    //WRITE TRANSCTION LOG TO SERVER A
                    connect_ServerA("3_"+trnsactionMessage);
                }          
                else if(random == 2){
                    //WITE TRANSCTION LOG TO SERVER B
                    connect_ServerB("3_"+trnsactionMessage);
                }
                else {
                    //WRITE TRANSCTION LOG TO SERVER C
                    connect_ServerC("3_"+trnsactionMessage);
                }
            

               //sending the transaction successful message to cleint.
                string message = "\""+username1+"\" successfully transferred "+tnsamount+" txcoins to \""+username2+"\".\nThe cuurent balance of the "+username1+" is : "+to_string(senderBalance)+" txcoins.";
                ///SENDING MESSAGE TO CLIENT THAT TRANSCTION NOT POSSIBLE BCUZ OF USER2
                sendMessageToCleint(message,childSocket);
                cout << "The main server sent the resulst of transaction to the client." <<endl;
           }


        }
     }
     else if(recievedData[0] == '3'){
         /**************************OPERATION 2 -- TXLIST OPERATION *****************************************/
         // The main server communicates over UDP Connection to the 3 backend server to request for the 
         // transcation logs and stores in a map.
         // Then all the transcations are written to file txlist.txt in increasing order.
        
         cout << "The main server received the sorted list request from the monitor using the TCP over port "+to_string(SERVERM_MONITOR_PORT) << endl;
         connectServer_TransLog(SERVER_A_PORT);
         connectServer_TransLog(SERVER_B_PORT);
         connectServer_TransLog(SERVER_C_PORT);

         // WRITING THE DATA TO THE OUTPUT FILE
         ofstream outfile;
         outfile.open("txchain.txt");
           for(auto it = transactionLog.begin(); it != transactionLog.end(); it++){
                string line = it->second; 
                outfile <<  line;       
            } 
         outfile.close(); 

         //Sending confirmation message to minotor after writing the TXCHAIN file.
         string message = "done";
         sendMessageToCleint(message,childSocket); 
       }
       close(childSocket); 
     }
     

 /**
  * The main method is used accpet the socket connectins from client and monitor 
  * in any order and depending on which connection is accepted and which operation 
  * is requested further connections are made in the handelconnection().   
  * 
  * This method is implemented in refrence with : https://stackoverflow.com/questions/15560336/listen-to-multiple-ports-from-one-server 
  **/    
 int main(){
    cout << "Server M is up and Running" << endl;
    create_ClientTCP();
    listenToClient();

    create_MonitorTCP();
    listenToMonitor();
        
        
        int max_socket_so_far = 0;
        fd_set current_sockets, ready_sockets; //Collection of file descriptors.
        //Imitilizing the current_sockets.
        FD_ZERO(&current_sockets);  
        FD_SET(sockfd_ClientParent,&current_sockets); // adding Client Parent socket.
        FD_SET(sockfd_MonitorParent,&current_sockets); // Adding monitor socket.
        max_socket_so_far = max(sockfd_ClientParent,sockfd_MonitorParent)+1;
        while(1){
            //select is destricutive so manking copy of current_sockets.
           ready_sockets = current_sockets;

           if(select(FD_SETSIZE,&ready_sockets,NULL/*Error*/,NULL,NULL/*Timeout*/)<0){
               perror("select error");
               exit(2);
           }

           for(int i =0; i < FD_SETSIZE;i++){
               if(FD_ISSET(i,&ready_sockets)){
                   if(i == sockfd_MonitorParent){
                        //When Monitor is trying to make connection.
                         monitorAddrSize = sizeof(monitor_addr);
                         sockfd_monitorChildSock = accept(sockfd_MonitorParent,(struct sockaddr *)&monitor_addr,&monitorAddrSize);
                             if(sockfd_monitorChildSock == -1){
                            perror("Server failed to connect with Monitor");
                         }
                         FD_SET(sockfd_monitorChildSock,&current_sockets);
                       }                 
                   else if(i == sockfd_ClientParent){
                        //When client is trying to make connection.
                         clientAddrSize = sizeof(client_addr);
                         sockfd_clientChildSock = accept(sockfd_ClientParent,(struct sockaddr *)&client_addr,&clientAddrSize);
                         if(sockfd_clientChildSock == -1){
                            perror("Server failed to connect with Client");
                         }
                         FD_SET(sockfd_clientChildSock,&current_sockets);
                   }
                   else{
                       handle_Connection(i);
                       FD_CLR(i,&current_sockets);  //removing from list after handelling connection.
                   }
               }
           }
 }
   return 0;
 }
    