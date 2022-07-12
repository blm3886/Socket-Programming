# Socket-Programming

A simple tcp client server application that describes the communication between the client and a server. 

project done as part of coursework:

In this project a simplified version of a blockchain system has been implimented that'll help to understand 
how cryptocurrency transactions work. For this scenario, we will have three nodes on the blockchain where 
each stores a group of transactions. These will be represented by backend servers. While in the blockchain
the transaction protocol deals with updating the digital wallet of each user, for this project there will be a 
main server in charge of running the calculations and updating the wallets for each user. 
Each transaction reported in the blockchain will include, in the following order, the transaction number, sender, 
receiver and amount being transferred.

The client issues a request for finding their current amount of txcoins in
their account, transfers them to another client and provides a file statement with all the
transactions in order. These requests will be sent to a Central Server which in turn
interacts with three other backend servers for pulling information and data processing.

client.cpp 
    The client.cpp program communicates with the serverM.cpp using TCP connection.
    The client program accepts command line args and request the main server 2 operations.
    1. CHECK WALLET (./clinet username)   
            The program converts this input to the form '"1_"+clientName'
            which is read by the serverM and iterpreted as a check wallet operation.

    2. TXCOINS (./clinet username1 username2 amount)
            The program converts this input to the form '"2_username1 username2 amount"'
            which is read by the serverM and iterpreted as a txcoin operation.

monitor.cpp
    The monitor.cpp program communicates with the serverM.cpp using TCP connection.
    The monitor program accepts command line args and request the main server 1 operations.
    1. TXLIST (.monitor TXLIST)   
            The program converts this input to the form "3_TXLIST"
            which is read by the serverM and iterpreted as a TXLIST operation.

serverM.cpp 
    -The main serverM acts as the client and communicates to three backened 
     servers (serverA serverB and serverC) over UDP connection.
    - Ansd receives requests from client and monitor programs over TCP.
     1. CHECK WALLET 
               -The server queries all the three backend servers (A, B and C)
               for the transcations details of a given user.
               -If user is found the sent and recived txcoins of the user is obatianed 
               and used to wards calculating the balance.
               - else the messge - "ABSENT" is obatined.
     
    2. TXCOINS 
            -checks if a valid transcation is possible, 
                -if yes then the serverM:
                    1.queries the all the backedn servers to send the 
                       max value of serial number in their block files and uses this to find the 
                       serial number of the next transcation. (No print messages are displayed 
                       When the main server does this query).
                    2. selects any one server at random and records the tranasction.
                - if transaction is not possible then appripriate message is displayed to the client.    
    
    3. TXLIST    
            The program requests the backend server to send all the data in the block file.
            on receiving the all the tranactions the main server decrypts all the tranastion logs 
            and writes the data to a file.

serverA.cpp
    - serverA.cpp has access to transactions in block1.txt.
serverB.cpp
    - serverB.cpp has access to transactions in block2.txt.
serverC.cpp
     - serverB.cpp has access to transactions in block2.txt.

    - All the three backend servers (A , B and C ) perform the same operation 
      using different block files. 
    - it provides the main server 3 types of data on request.
        1. Check Wallet - When this operation is request by the serverM the serverA 
                          responds with the sent and received txcoins associated with 
                          the user else responds with ABSENT.
                        -request format from serverM -> "1_clientName".
                        -response fromat from serverA -> "S_(sentAmounts)_R_(receivedAmounts)".

        2. Get Max serial number so far - gets the max serail number present in block1.txt file.
                        -request format from serverM -> "2_(..data..)".
                        -response format from serverA -> max serial number in string form.

        3. Add successful transctions
                         - the successful transaction is written to the file.
                         -request format from serverM -> "3_(..data..)".
                         -response fromat from serverA -> "logged".
        
        4.ServerM requesting all the transactions recorded in the block.txt files.
                         -request format from serverM -> "4_(..data..)".
                         -response format from serverA -> sends each line of the files.
                                                        after sending all the lines,
                                                        sends an "**END***" text at the end, if indictates 
                                                        the serverM that all the tranactions have been obatined from this file.
