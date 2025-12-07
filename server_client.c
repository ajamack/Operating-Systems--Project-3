#include "server.h"

#define DEFAULT_ROOM "Lobby"

// USE THESE LOCKS AND COUNTER TO SYNCHRONIZE
extern int numReaders;
extern pthread_mutex_t rw_lock;
extern pthread_mutex_t mutex;

extern struct node *head;
extern struct room *rooms_head;
extern struct dm   *dm_head;

extern char *server_MOTD;


/*
 *Main thread for each client.  Receives all messages
 *and passes the data off to the correct function.  Receives
 *a pointer to the file descriptor for the socket the thread
 *should listen on
 */

char *trimwhitespace(char *str)
{
  char *end;

  // Trim leading space
  while(isspace((unsigned char)*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace((unsigned char)*end)) end--;

  // Write new null terminator character
  end[1] = '\0';

  return str;
}

void *client_receive(void *ptr) {
   int client = *(int *) ptr;  // socket
  
   int received, i;
   char buffer[MAXBUFF], sbuffer[MAXBUFF];  //data buffer of 2K  
   char tmpbuf[MAXBUFF];  //data temp buffer of 1K  
   char cmd[MAXBUFF], username[20];
   char *arguments[80];

   struct node *currentUser;
    
   send(client  , server_MOTD , strlen(server_MOTD) , 0 ); // Send Welcome Message of the Day.

   // Creating the guest user name
   sprintf(username,"guest%d", client);

   // Add user to global list and default room under writer lock
   pthread_mutex_lock(&rw_lock);
   head = insertFirstU(head, client , username);
   struct node *me = findBySocket(head, client);
   addUserToRoom(rooms_head, DEFAULT_ROOM, me);
   pthread_mutex_unlock(&rw_lock);
   
   // Add the GUEST to the DEFAULT ROOM (i.e. Lobby)

   while (1) {
       
      if ((received = read(client , buffer, MAXBUFF)) != 0) {
      
            buffer[received] = '\0'; 
            strcpy(cmd, buffer);  
            strcpy(sbuffer, buffer);
         
            /////////////////////////////////////////////////////
            // we got some data from a client

            // 1. Tokenize the input in buf (split it on whitespace)

            // get the first token 
            arguments[0] = strtok(cmd, delimiters);

            // walk through other tokens 
            i = 0;
            while( arguments[i] != NULL ) {
               arguments[++i] = strtok(NULL, delimiters); 
               strcpy(arguments[i-1], trimwhitespace(arguments[i-1]));
            } 

            // Arg[0] = command
            // Arg[1] = user or room
             
            /////////////////////////////////////////////////////
            // 2. Execute command


            if(strcmp(arguments[0], "create") == 0)
            {
               printf("create room: %s\n", arguments[1]); 
              
               pthread_mutex_lock(&rw_lock);
               rooms_head = addRoom(rooms_head, arguments[1]);
               pthread_mutex_unlock(&rw_lock);
              
               sprintf(buffer, "Room %s created (or already exists).\nchat>", arguments[1]);
               send(client , buffer , strlen(buffer) , 0 ); // send back to client
            }
            else if (strcmp(arguments[0], "join") == 0)
            {
               printf("join room: %s\n", arguments[1]);  

               pthread_mutex_lock(&rw_lock);
               struct room *r = findRoom(rooms_head, arguments[1]);
               if (r == NULL) {
                   pthread_mutex_unlock(&rw_lock);
                   sprintf(buffer, "Room %s does not exist. Use create <room> first.\nchat>", arguments[1]);
                   send(client , buffer , strlen(buffer) , 0 );
               } else {
                   struct node *me2 = findBySocket(head, client);
                   if (me2 != NULL) {
                       addUserToRoom(rooms_head, arguments[1], me2);
                       pthread_mutex_unlock(&rw_lock);
                       sprintf(buffer, "Joined room %s\nchat>", arguments[1]);
                       send(client , buffer , strlen(buffer) , 0 );
                   } else {
                       pthread_mutex_unlock(&rw_lock);
                       sprintf(buffer, "Error: cannot find your user entry.\nchat>");
                       send(client , buffer , strlen(buffer) , 0 );
                   }
               }
            }
            else if (strcmp(arguments[0], "leave") == 0)
            {
               printf("leave room: %s\n", arguments[1]); 

               pthread_mutex_lock(&rw_lock);
               struct node *me2 = findBySocket(head, client);
               if (me2 != NULL) {
                   removeUserFromRoom(rooms_head, arguments[1], me2);
                   pthread_mutex_unlock(&rw_lock);
                   sprintf(buffer, "Left room %s\nchat>", arguments[1]);
                   send(client , buffer , strlen(buffer) , 0 );
               } else {
                   pthread_mutex_unlock(&rw_lock);
                   sprintf(buffer, "Error: cannot find your user entry.\nchat>");
                   send(client , buffer , strlen(buffer) , 0 );
               }

            } 
            else if (strcmp(arguments[0], "connect") == 0)
            {
               printf("connect to user: %s \n", arguments[1]);

               pthread_mutex_lock(&rw_lock);
               struct node *me2 = findBySocket(head, client);
               struct node *other = findU(head, arguments[1]);
               if (me2 == NULL || other == NULL) {
                   pthread_mutex_unlock(&rw_lock);
                   sprintf(buffer, "Error: user %s not found.\nchat>", arguments[1]);
                   send(client , buffer , strlen(buffer) , 0 );
               } else if (me2 == other) {
                   pthread_mutex_unlock(&rw_lock);
                   sprintf(buffer, "Cannot connect to yourself.\nchat>");
                   send(client , buffer , strlen(buffer) , 0 );
               } else {
                   dm_head = addDM(dm_head, me2, other);
                   pthread_mutex_unlock(&rw_lock);
                   sprintf(buffer, "Connected to %s\nchat>", arguments[1]);
                   send(client , buffer , strlen(buffer) , 0 );
               }
            }
            else if (strcmp(arguments[0], "disconnect") == 0)
            {             
               printf("disconnect from user: %s\n", arguments[1]);
               
               pthread_mutex_lock(&rw_lock);
               struct node *me2 = findBySocket(head, client);
               struct node *other = findU(head, arguments[1]);
               if (me2 == NULL || other == NULL) {
                   pthread_mutex_unlock(&rw_lock);
                   sprintf(buffer, "Error: user %s not found.\nchat>", arguments[1]);
                   send(client , buffer , strlen(buffer) , 0 );
               } else {
                   dm_head = removeDM(dm_head, me2, other);
                   pthread_mutex_unlock(&rw_lock);
                   sprintf(buffer, "Disconnected from %s\nchat>", arguments[1]);
                   send(client , buffer , strlen(buffer) , 0 );
               }
            }                  
            else if (strcmp(arguments[0], "rooms") == 0)
            {
               printf("List all the rooms\n");
              
               // READER: acquire read lock
               pthread_mutex_lock(&mutex);
               numReaders++;
               if (numReaders == 1) {
                   pthread_mutex_lock(&rw_lock);
               }
               pthread_mutex_unlock(&mutex);

               listRoomsToBuffer(rooms_head, buffer, sizeof(buffer));

               // release read lock
               pthread_mutex_lock(&mutex);
               numReaders--;
               if (numReaders == 0) {
                   pthread_mutex_unlock(&rw_lock);
               }
               pthread_mutex_unlock(&mutex);
       
               strcat(buffer, "chat>");
               send(client , buffer , strlen(buffer) , 0 ); // send back to client                            
            }   
            else if (strcmp(arguments[0], "users") == 0)
            {
               printf("List all the users\n");

               // READER: acquire read lock (reader-writer pattern)
               pthread_mutex_lock(&mutex);
               numReaders++;
               if (numReaders == 1) {
                  pthread_mutex_lock(&rw_lock);
               }
               pthread_mutex_unlock(&mutex);

               // build list of users in buffer
               buffer[0] = '\0';
               strcat(buffer, "Users:\n");

               struct node *cur = head;
               while (cur != NULL) {
                  strcat(buffer, " - ");
                  strcat(buffer, cur->username);
                  strcat(buffer, "\n");
                  cur = cur->next;
               }

               // release read lock
               pthread_mutex_lock(&mutex);
               numReaders--;
               if (numReaders == 0) {
                  pthread_mutex_unlock(&rw_lock);
               }
               pthread_mutex_unlock(&mutex);

               strcat(buffer, "chat>");
               send(client , buffer , strlen(buffer) , 0 ); // send back to client
            }                           
            else if (strcmp(arguments[0], "login") == 0)
            {
               if (arguments[1] == NULL) {
                   sprintf(buffer, "Usage: login <username>\nchat>");
                   send(client , buffer , strlen(buffer) , 0 );
               } else {
                   // writer: modify shared user list
                   pthread_mutex_lock(&rw_lock);

                   // check if desired username already exists
                   struct node *exists = findU(head, arguments[1]);
                   if (exists != NULL) {
                       pthread_mutex_unlock(&rw_lock);
                       sprintf(buffer, "Username '%s' is already taken.\nchat>", arguments[1]);
                       send(client , buffer , strlen(buffer) , 0 );
                   } else {
                       // find this client by socket and rename
                       struct node *me2 = findBySocket(head, client);
                       if (me2 != NULL) {
                           strcpy(me2->username, arguments[1]);
                           strcpy(username, arguments[1]);  // local cache
                           pthread_mutex_unlock(&rw_lock);

                           sprintf(buffer, "You are now known as %s\nchat>", username);
                           send(client , buffer , strlen(buffer) , 0 );
                       } else {
                           pthread_mutex_unlock(&rw_lock);
                           sprintf(buffer, "Error: could not find your user entry.\nchat>");
                           send(client , buffer , strlen(buffer) , 0 );
                       }
                   }
               }
            } 
            else if (strcmp(arguments[0], "help") == 0 )
            {
                sprintf(buffer, "login <username> - \"login with username\" \ncreate <room> - \"create a room\" \njoin <room> - \"join a room\" \nleave <room> - \"leave a room\" \nusers - \"list all users\" \nrooms -  \"list all rooms\" \nconnect <user> - \"connect to user\" \ndisconnect <user> - \"disconnect from user\" \nexit - \"exit chat\" \n");
                send(client , buffer , strlen(buffer) , 0 ); // send back to client 
            }
            else if (strcmp(arguments[0], "exit") == 0 || strcmp(arguments[0], "logout") == 0)
            {
                //Remove the initiating user from all rooms and direct connections, then close the socket descriptor.
                pthread_mutex_lock(&rw_lock);
                struct node *me2 = findBySocket(head, client);
                if (me2 != NULL) {
                    removeUserFromAllRooms(rooms_head, me2);
                    removeUserFromAllDMs(&dm_head, me2);
                    head = removeBySocket(head, client);
                }
                pthread_mutex_unlock(&rw_lock);

                close(client);
                break;  // exit thread loop
            }                         
            else { 
                 /////////////////////////////////////////////////////////////
                 // 3. sending a message

                 // Reader lock while we inspect the list and sender
                 pthread_mutex_lock(&mutex);
                 numReaders++;
                 if (numReaders == 1) {
                     pthread_mutex_lock(&rw_lock);
                 }
                 pthread_mutex_unlock(&mutex);

                 // find sender's username based on socket
                 struct node *me2 = findBySocket(head, client);
                 const char *from = (me2 != NULL) ? me2->username : "UNKNOWN";

                 // format outgoing message
                 sprintf(tmpbuf, "\n::%s> %s\nchat>", from, sbuffer);
                 strcpy(sbuffer, tmpbuf);

                 // broadcast to users who share a room or are DM-connected
                 currentUser = head;
                 while (currentUser != NULL) {
                     if (client != currentUser->socket) {  // don't send to yourself
                         if (shareRoom(rooms_head, me2, currentUser) || 
                             isDMConnected(dm_head, me2, currentUser)) {
                             send(currentUser->socket , sbuffer , strlen(sbuffer) , 0 ); 
                         }
                     }
                     currentUser = currentUser->next;
                 }

                 // release reader lock
                 pthread_mutex_lock(&mutex);
                 numReaders--;
                 if (numReaders == 0) {
                     pthread_mutex_unlock(&rw_lock);
                 }
                 pthread_mutex_unlock(&mutex);
            }
 
         memset(buffer, 0, sizeof(buffer));
      }
   }
   return NULL;
}
