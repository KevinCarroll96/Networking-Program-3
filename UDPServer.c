/* 
    Compilation: gcc -o DHCPserver DHCPserver.c
    Execution  : ./DHCPserver <port_number> [eg. port_number = 5000, where port_number is the DHCP server port number]
*/

#include <stdio.h> 
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
 
#define BUFLEN 512  //Max length of buffer
 
void die(char *s)
{
    perror(s);
    exit(1);
}
 
int main(int argc, char *argv[])
{
    struct sockaddr_in si_me, si_other;
    struct timeval timeout={0,0};
    fd_set readfds;
    int select_ret;
    int x = 9, s, i, j = 0, slen = sizeof(si_other), recv_len, portno;
    int ctransID = 0, lifeTime = 3600;
    char buf[BUFLEN], message[1024];
    char cyiaddr[BUFLEN] = "";
    char fileGetIPAddr[50];
    char stringLine[50], rmLine1[50], ipCount[5];
    FILE *fileStream, *fileTemp;
    time_t start, end;
    double elapsed = 0;
     
    //create a UDP socket
    if((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        die("socket");
     
    // zero out the structure
    memset((char *) &si_me, 0, sizeof(si_me));
    portno = atoi(argv[1]); //The port on which to listen for incoming data
     
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(portno);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
     
    //bind socket to port
    if(bind(s, (struct sockaddr *)&si_me, sizeof(si_me)) == -1)
        die("bind");
    
    system("clear");
    printf("...This is DHCP server...\n\n");

    //keep listening for data
    while(1)
    {
        do
        {
            FD_ZERO(&readfds); //Zero out socket set
            FD_SET(s, &readfds); //Add socket to listen to
            select_ret = select(s+1, &readfds, NULL, NULL, &timeout);

            fflush(stdout);

            //Begin sequence for DHCP discover - Receive broadcast message from client
            printf("\nBegin DHCP 4-Handshake - Discover client arrives\n");
            if((recv_len = recvfrom(s, &cyiaddr, sizeof(cyiaddr), 0, (struct sockaddr *) &si_other, &slen)) == -1)
                die("recvfrom()");
            if((recv_len = recvfrom(s, &ctransID, sizeof(ctransID), 0, (struct sockaddr *) &si_other, &slen)) == -1)
                die("recvfrom()");
            printf("Broadcast from client from %s, port number:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
            printf("yiaddr: %s\n", cyiaddr);
            printf("Transaction ID: %d\n", ctransID);
			
			if(x<0)
			{
				strcpy(cyiaddr, "0.0.0.0");
				if(sendto(s, &cyiaddr, sizeof(cyiaddr), 0, (struct sockaddr *) &si_other, slen) == -1)
					die("sendto()");
				goto LOOP;
			}

            //Begin sequence for DHCP offer
            fileStream = fopen("IPaddress.txt", "r"); //Open the IPaddress file for reading
            fgets(fileGetIPAddr, 50, fileStream); //Get the IP Address from the file to assign to the client
            strcpy(cyiaddr, fileGetIPAddr); //Assign it to the client
            printf("\nAssigned yiaddr is: %s", cyiaddr); 
            printf("For Transaction ID: %d\n", ctransID);
            printf("Assigned Lifetime: 3600 secs\n\n"); //Assign the Lifetime of the IP address for 1 hour, timer not set yet
            //Send to the server, print error if it doesn't work
            if(sendto(s, &cyiaddr, sizeof(cyiaddr), 0, (struct sockaddr *) &si_other, slen) == -1)
                die("sendto()");
            if(sendto(s, &ctransID, sizeof(ctransID), 0, (struct sockaddr *) &si_other, slen) == -1)
                die("sendto()");
            if(sendto(s, &lifeTime, sizeof(lifeTime), 0, (struct sockaddr *) &si_other, slen) == -1)
                die("sendto()");

            //Begin sequence for DHCP request - Receive request/confirmation message from client
            if((recv_len = recvfrom(s, &cyiaddr, sizeof(cyiaddr), 0, (struct sockaddr *) &si_other, &slen)) == -1)
                die("recvfrom()");
            if((recv_len = recvfrom(s, &ctransID, sizeof(ctransID), 0, (struct sockaddr *) &si_other, &slen)) == -1)
                die("recvfrom()");
            if((recv_len = recvfrom(s, &lifeTime, sizeof(lifeTime), 0, (struct sockaddr *) &si_other, &slen)) == -1)
                die("recvfrom()");
            printf("Request from client from %s, port number:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
            printf("Confirmed yiaddr: %s", cyiaddr);
            printf("Transaction ID: %d\n", ctransID);
            printf("Lifetime: %d\n\n", lifeTime);

            //Begin sequence for DHCP acknowledge - Send acknowledgment message to client
            printf("Sending ACK message to client:%s, on port number:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
            printf("yiaddr: %s", cyiaddr);
            printf("Transaction ID: %d\n", ctransID);
            printf("Lifetime: %d\n\n", lifeTime);
            //Send ACK message to client
            if(sendto(s, &cyiaddr, sizeof(cyiaddr), 0, (struct sockaddr *) &si_other, slen) == -1)
                die("sendto()");
            if(sendto(s, &ctransID, sizeof(ctransID), 0, (struct sockaddr *) &si_other, slen) == -1)
                die("sendto()");
            if(sendto(s, &lifeTime, sizeof(lifeTime), 0, (struct sockaddr *) &si_other, slen) == -1)
                die("sendto()");

            //Start the Lifetime timer here
            time(&start);

            //Update the IPaddress file
            fileTemp = fopen("File_copy.txt", "w");  //Temporary file

            i = 0;
            while(i < x)
            {
                fscanf(fileStream, "%s", stringLine); //Scan the line of the file at line i
				fprintf(fileTemp, "%s\n", stringLine); //Copy the next line to the temporary file File_copy.txt
				i++;
			}
			x--;
            sprintf(ipCount, "%d", j);
            j++; //Update the counter for next IP Address in list to use
			fclose(fileStream);
            fclose(fileTemp);

            system("rm IPaddress.txt"); //Remove old IPaddress.txt
            system("mv File_copy.txt IPaddress.txt"); //Rename File_copy.txt to IPaddress.txt

            //Grab the end time then show elapsed for Lifetime counter if it were a requirement
            //it would be implemented with something like this...
            sleep(5);
            time(&end);
            elapsed = difftime(end, start);
            printf("Elapsed time for IPaddress: %s is - %.2lf secs\n\n", rmLine1, elapsed);

        }while(select_ret > 0);
		
            //If we run out of IP addresses to assign to the clients
            LOOP: printf("There are no more IP Addresses!!!!!!\n");
    }
 
    close(s);
    return 0;
}