#include <string>
#include <cstring>
#include <iostream>
#include <cstdlib>
#include <queue>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include "serializer.h"
using namespace std;

// Queue for requests to do
queue< string > toSend;
int closing = 0;
int awaitingMessages = 0;

void * sendLines(void * arg) {

    int sockfd = *(int*) arg;

	while( ! closing ) {
		// If queue is nonempty, send messages
		if ( ! toSend.empty() ) {
			// Pop first message
			string val = toSend.front();
			toSend.pop();

			// Pack message
			char msg[val.length()+5];

			packlen(val.length()+1, msg);

			// Pack rest of message
			memcpy((void*) (msg+4), val.c_str(), val.length()+1);

			// Send message to server
            write(sockfd, msg, val.length()+5);

            awaitingMessages++;
			sleep(2);
		} else {
			// Else queue is empty, give it some time to fill
			//sleep(1);
		}
	}

	// Empty Queue
    while ( ! toSend.empty() ) {
        			// Pop first message
			string val = toSend.front();
			toSend.pop();

			// Pack message
			char msg[val.length()+5];

			packlen(val.length()+1, msg);

			// Pack rest of message
			memcpy((void*) (msg+4), val.c_str(), val.length()+1);

			// Send message to server
            write(sockfd, msg, val.length()+5);

            awaitingMessages++;
			sleep(2);
    }
}

void * receiveLines(void * arg) {

    int sockfd = *(int*) arg;

	while ( ! closing ) {
        int packetlen;
        char num[4];
		// Listen on socket for returning data
        read(sockfd, num, 4);

        packetlen = unpacklen(num);

        char msg[packetlen];
        read(sockfd, msg, packetlen);

		// Print it
		cout << "Server: " << msg << endl;

        // Decrement counter
        awaitingMessages--;
	}

    // When closing, we want to print messages already sent to the server
	for (; awaitingMessages > 0; awaitingMessages--) {
        // Rcv and print message from server
        int packetlen;
        char num[4];
		// Listen on socket for returning data
        read(sockfd, num, 4);

        packetlen = unpacklen(num);

        char msg[packetlen];
        read(sockfd, msg, packetlen);

		// Print it
		cout << msg << endl;

	}
}

int main() {
	// Get Server name and port
	char * servername = getenv("SERVER_ADDRESS");
	char * portnumber = getenv("SERVER_PORT");

	addrinfo * serverInfo, *sp;
	addrinfo hint;
    int sockfd;

    memset(&hint, 0, sizeof(hint));
    hint.ai_protocol = IPPROTO_TCP;

    // Get server address
    if (getaddrinfo(servername, portnumber, &hint, &serverInfo) != 0) {
        cerr << "getAddrInfo() failed" << endl;
        exit(EXIT_FAILURE);
    }

    // Connect to server
    for(sp = serverInfo; sp != NULL; sp = sp->ai_next){
        if ((sockfd = socket(sp->ai_family, sp->ai_socktype, sp->ai_protocol)) < 0) {
            cerr << "Could not open socket " << errno << endl;
            close(sockfd);
        } else {

            if (connect (sockfd, (sockaddr *) sp->ai_addr, sp->ai_addrlen) < 0) {
                cerr << "Connection failed" << errno << endl;
            } else {
                break;
            }
        }
    }

	// Se1cond thread will send lines
    // Third thread will receive and print them
    pthread_t senderID, receiverID;
    pthread_create(&senderID, NULL, &sendLines, (void *) &sockfd);
    pthread_create(&receiverID, NULL, &receiveLines, (void *) &sockfd);

	while( ! cin.fail() ) {
		// Read Line
		string in;

		getline(cin, in);

		// Queue line
		toSend.push(in);
	}

	// Cleanup
	// Kill child threads
	closing = 1;

	while (awaitingMessages != 0) {}

	// Close socket
	close(sockfd);
}
