#include <cstring>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <cstdlib>
#include "serializer.h"
#include <errno.h>

#include <iostream>
using namespace std;

enum conversion {
	toUpper,
	toLower
};

// Convert given character to upper or lower case as specified by conv parameter
char convert(char toConv, conversion conv) {
	char ret = toConv;
	if (conv == toUpper) {
		if (toConv <= 'z' && toConv >= 'a') {
			ret = toConv - 0x20;
		}
	} else {
		if (toConv <= 'Z' && toConv >= 'A') {
			ret = toConv + 0x20;
		}
	}

	return ret;
}

// Capitalize first character of each word in a string
void convertString (char * str) {
	conversion toDo = toUpper;

	for(int i = 0; i < strlen(str); i++) {
		str[i] = convert(str[i], toDo);

		if (str[i] == ' ') {
			toDo = toUpper;
		} else {
			toDo = toLower;
		}
	}
}

int main() {
    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));

	// Print Server Information
	char servername[1024];
	gethostname(servername , 1024);

	cout << "SERVER_ADDRESS " << servername << endl;

	// Get next port, bind to it announce it
    int listenfd = 0;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(0);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    bind(listenfd, (sockaddr *) &serv_addr, sizeof(serv_addr));

    listen(listenfd, 5);

    socklen_t len = sizeof(serv_addr);

    getsockname(listenfd, (sockaddr*) &serv_addr, &len);

    cout << "SERVER_PORT " << ntohs(serv_addr.sin_port) << endl;

    // Initialize fd_set
    fd_set cxns;
    int nfds = listenfd + 1;
    FD_ZERO(&cxns);
    FD_SET(listenfd, &cxns);

    // We now multiplex between requests and connections from clients

	while (true){
        fd_set workcxns = cxns;

        if (select(nfds, &workcxns, NULL, NULL, NULL) < 0) {
            cerr << "Select() failed" << endl;
            exit(EXIT_FAILURE);
        }

        // Loop through available connections, handle accordingly
        for(int i = 0; i < nfds; i++) {
            // Is i a valid fd?
            if (FD_ISSET(i, &workcxns)) {
                // Is ready fd a cxn req, a request, or a close

                if (i == listenfd) {
                    // Accept Connection, get socket info
                    sockaddr clientaddr;
                    socklen_t len = sizeof(clientaddr);

                    int connfd = accept(listenfd, &clientaddr, &len);

                    if (connfd == -1) {
                        // Client quit, ignore the attempt
                        continue;
                    }

                    // Add new cxn to fd_set
                    if (connfd + 1 > nfds) {
                        nfds = connfd + 1;
                    }
                    FD_SET(connfd, &cxns);
                } else {
                    // Handle request
                    // Read value of 0 indicates connection is closed
                    char num[4];
                    if (read(i, num, 4) == 0) {
                        FD_CLR(i, &cxns);
                        close(i);
                    } else {
                    // Read in the message
                        int packetlen = unpacklen(num);
                        char msg[packetlen];

                        read(i, msg, packetlen);

                        cout << msg << endl;

                        // Convert the message
                        convertString(msg);

                        // Send the message to the client
                        char toSend[packetlen+4];

                        packlen(packetlen, toSend);

                        // Pack rest of message
                        memcpy((void*) (toSend+4), msg, packetlen);

                        write(i, toSend, packetlen+4);
                    }
                }

            }
        }

	}

	close(listenfd);

	return 0;
}
