#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "list.h"

/////////////////// USERLIST //////////////////////////

// insert link at the first location
struct node* insertFirstU(struct node *head, int socket, char *username) {
   if(findU(head, username) == NULL) {
       struct node *link = (struct node*) malloc(sizeof(struct node));

       link->socket = socket;
       strcpy(link->username, username);

       link->next = head;
       head = link;
   }
   else {
       printf("Duplicate: %s\n", username);
   }
   return head;
}

// find a link with given user
struct node* findU(struct node *head, char* username) {
   struct node* current = head;

   if(head == NULL) {
      return NULL;
   }

   while(current != NULL && strcmp(current->username, username) != 0) {
      current = current->next;
   }

   return current;
}

// find a node with given socket fd
struct node* findBySocket(struct node *head, int socket) {
    struct node* current = head;

    if (head == NULL) {
        return NULL;
    }

    while (current != NULL && current->socket != socket) {
        current = current->next;
    }

    return current;  // NULL if not found
}

// remove a node by socket; returns new head
struct node* removeBySocket(struct node *head, int socket) {
    struct node *current = head;
    struct node *prev = NULL;

    while (current != NULL && current->socket != socket) {
        prev = current;
        current = current->next;
    }

    if (!current) {
        return head; // not found
    }

    if (!prev) {
        head = current->next;
    } else {
        prev->next = current->next;
    }

    free(current);
    return head;
}

// free all user nodes
void freeUserList(struct node *head) {
    struct node *cur = head;
    while (cur != NULL) {
        struct node *next = cur->next;
        free(cur);
        cur = next;
    }
}


/////////////////// ROOMS //////////////////////////

// add a room (if it doesn't already exist); returns new head
struct room* addRoom(struct room *head, const char *name) {
    struct room *r = head;

    while (r != NULL) {
        if (strcmp(r->name, name) == 0) {
            return head; // already exists
        }
        r = r->next;
    }

    r = (struct room*)malloc(sizeof(struct room));
    strcpy(r->name, name);
    r->members = NULL;
    r->next = head;

    return r;
}

// find a room by name
struct room* findRoom(struct room *head, const char *name) {
    struct room *r = head;
    while (r != NULL) {
        if (strcmp(r->name, name) == 0) {
            return r;
        }
        r = r->next;
    }
    return NULL;
}

// add user to named room
void addUserToRoom(struct room *rooms, const char *roomName, struct node *user) {
    if (!rooms || !user) return;

    struct room *r = findRoom(rooms, roomName);
    if (!r) return;

    struct room_member *m = r->members;
    while (m != NULL) {
        if (m->user == user) {
            return; // already a member
        }
        m = m->next;
    }

    struct room_member *nm = (struct room_member*)malloc(sizeof(struct room_member));
    nm->user = user;
    nm->next = r->members;
    r->members = nm;
}

// remove user from named room
void removeUserFromRoom(struct room *rooms, const char *roomName, struct node *user) {
    if (!rooms || !user) return;

    struct room *r = findRoom(rooms, roomName);
    if (!r) return;

    struct room_member *cur = r->members;
    struct room_member *prev = NULL;

    while (cur != NULL && cur->user != user) {
        prev = cur;
        cur = cur->next;
    }

    if (!cur) return;

    if (!prev) {
        r->members = cur->next;
    } else {
        prev->next = cur->next;
    }

    free(cur);
}

// remove user from all rooms
void removeUserFromAllRooms(struct room *rooms, struct node *user) {
    struct room *r = rooms;
    while (r != NULL) {
        removeUserFromRoom(rooms, r->name, user);
        r = r->next;
    }
}

// do two users share at least one room?
bool shareRoom(struct room *rooms, struct node *a, struct node *b) {
    if (!rooms || !a || !b) return false;

    struct room *r = rooms;
    while (r != NULL) {
        bool hasA = false, hasB = false;
        struct room_member *m = r->members;

        while (m != NULL) {
            if (m->user == a) hasA = true;
            if (m->user == b) hasB = true;
            m = m->next;
        }

        if (hasA && hasB) return true;

        r = r->next;
    }

    return false;
}

// append list of rooms into buffer
void listRoomsToBuffer(struct room *rooms, char *buffer, size_t bufsize) {
    (void)bufsize; // not strictly used for bounds in this simple implementation

    buffer[0] = '\0';
    strcat(buffer, "Rooms:\n");

    struct room *r = rooms;
    while (r != NULL) {
        strcat(buffer, " - ");
        strcat(buffer, r->name);
        strcat(buffer, "\n");
        r = r->next;
    }
}

// free all rooms and membership lists
void freeRooms(struct room *rooms) {
    struct room *r = rooms;
    while (r != NULL) {
        struct room *nextR = r->next;

        struct room_member *m = r->members;
        while (m != NULL) {
            struct room_member *nextM = m->next;
            free(m);
            m = nextM;
        }

        free(r);
        r = nextR;
    }
}


/////////////////// DMs //////////////////////////

// add DM connection between a and b (no duplicates); returns new head
struct dm* addDM(struct dm *head, struct node *a, struct node *b) {
    if (!a || !b) return head;

    struct dm *cur = head;
    while (cur != NULL) {
        if ((cur->u1 == a && cur->u2 == b) || (cur->u1 == b && cur->u2 == a)) {
            return head; // already exists
        }
        cur = cur->next;
    }

    struct dm *d = (struct dm*)malloc(sizeof(struct dm));
    d->u1 = a;
    d->u2 = b;
    d->next = head;

    return d;
}

// remove DM connection between a and b; returns new head
struct dm* removeDM(struct dm *head, struct node *a, struct node *b) {
    struct dm *cur = head;
    struct dm *prev = NULL;

    while (cur != NULL) {
        if ((cur->u1 == a && cur->u2 == b) || (cur->u1 == b && cur->u2 == a)) {
            if (!prev) {
                head = cur->next;
            } else {
                prev->next = cur->next;
            }
            free(cur);
            return head;
        }
        prev = cur;
        cur = cur->next;
    }

    return head;
}

// are two users DM-connected?
bool isDMConnected(struct dm *head, struct node *a, struct node *b) {
    struct dm *cur = head;

    while (cur != NULL) {
        if ((cur->u1 == a && cur->u2 == b) || (cur->u1 == b && cur->u2 == a)) {
            return true;
        }
        cur = cur->next;
    }

    return false;
}

// remove all DMs involving user
void removeUserFromAllDMs(struct dm **headPtr, struct node *user) {
    if (!headPtr || !user) return;

    struct dm *cur = *headPtr;
    struct dm *prev = NULL;

    while (cur != NULL) {
        if (cur->u1 == user || cur->u2 == user) {
            struct dm *toFree = cur;
            if (!prev) {
                *headPtr = cur->next;
                cur = cur->next;
            } else {
                prev->next = cur->next;
                cur = cur->next;
            }
            free(toFree);
        } else {
            prev = cur;
            cur = cur->next;
        }
    }
}

// free all DMs
void freeDMs(struct dm *head) {
    struct dm *cur = head;
    while (cur != NULL) {
        struct dm *next = cur->next;
        free(cur);
        cur = next;
    }
}
