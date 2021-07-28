#pragma once

typedef struct ListItem {
  struct ListItem* prev;
  struct ListItem* next;
} ListItem;

typedef struct ListHead {
  ListItem* first;
  ListItem* last;
  int size;
} ListHead;

void List_init(ListHead* head);     // initializes the list head
void ListItem_init(ListItem* item); // sets prev and next to null. Basically useless sugar.

ListItem* List_find(ListHead* head, ListItem* item); // finds if any an element
ListItem* List_insert(ListHead* head, ListItem* previous, ListItem* item); // inserts item after previous
ListItem* List_detach(ListHead* head, ListItem* item); // detaches (not destroy) an element in the list

