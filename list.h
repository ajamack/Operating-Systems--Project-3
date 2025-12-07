#ifndef LIST_H
#define LIST_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

/********** USER LIST **********/

struct node {
   char username[30];
   int socket;
   struct node *next;
};

// insert node at the first location
struct node* insertFirstU(struct node *head, int socket, char *username);

// find a node with given username
struct node* findU(struct node *head, char* username);

// find a node with given socket fd
struct node* findBySocket(struct node *head, int socket);

// remove a node by socket; returns new head
struct node* removeBySocket(struct node *head, int socket);

// free all user nodes
void freeUserList(struct node *head);


/********** ROOM STRUCTURES **********/

struct room_member {
   struct node *user;              // pointer into user list
   struct room_member *next;
};

struct room {
   char name[30];
   struct room_member *members;    // linked list of members
   struct room *next;
};

// add a room (if it doesn't already exist); returns new head
struct room* addRoom(struct room *head, const char *name);

// find a room by name
struct room* findRoom(struct room *head, const char *name);

// add user to named room (no-op if room or user is NULL or already a member)
void addUserToRoom(struct room *rooms, const char *roomName, struct node *user);

// remove user from named room
void removeUserFromRoom(struct room *rooms, const char *roomName, struct node *user);

// remove user from all rooms
void removeUserFromAllRooms(struct room *rooms, struct node *user);

// do two users share at least one room?
bool shareRoom(struct room *rooms, struct node *a, struct node *b);

// append list of rooms into buffer ("Rooms:\n - room1\n ...")
void listRoomsToBuffer(struct room *rooms, char *buffer, size_t bufsize);

// free all rooms and membership lists
void freeRooms(struct room *rooms);


/********** DM (DIRECT MESSAGE) STRUCTURES **********/

struct dm {
   struct node *u1;
   struct node *u2;
   struct dm *next;
};

// add DM connection between a and b (no duplicates); returns new head
struct dm* addDM(struct dm *head, struct node *a, struct node *b);

// remove DM connection between a and b; returns new head
struct dm* removeDM(struct dm *head, struct node *a, struct node *b);

// remove all DMs involving user
void removeUserFromAllDMs(struct dm **headPtr, struct node *user);

// are two users DM-connected?
bool isDMConnected(struct dm *head, struct node *a, struct node *b);

// free all DMs
void freeDMs(struct dm *head);

#endif
