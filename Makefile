all: serverM serverA serverB serverC client monitor

serverM: serverM.cpp
	g++ -std=c++11 -oserverM serverM.cpp

serverA: serverA.cpp
	g++ -std=c++11 -oserverA serverA.cpp

serverB: serverB.cpp
	g++ -std=c++11 -oserverB serverB.cpp

serverC: serverC.cpp
	g++ -std=c++11 -oserverC serverC.cpp

client: client.cpp
	g++ -std=c++11 -oclient client.cpp

monitor: monitor.cpp
	g++ -std=c++11 -omonitor monitor.cpp
