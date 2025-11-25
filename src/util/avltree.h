#ifndef _AVLTREE_H_
#define _AVLTREE_H_

#define ALPHA 0.5

/*
  Tree structure to store (key, value) pairs with fast lookup
  by key. Stores pointers to the keys and values.
*/

struct node
{
  void  *key;
  void  *val;
  struct node *left;
  struct node *right;
  struct node *parent;
  struct node *next;
  struct node *prev;
  int id;
};
typedef struct node Node;

struct tree
{
  int (*cmpfunc) (const void *val1, const void *val2);
  Node *root;
  Node *first;
  Node *prev;
};
typedef struct tree AVLTree;

/* Allocate a new, empty tree */
AVLTree *avltree_new(int (*cmpfunc) (const void *val1, const void *val2));

/* Add a new node, returns 0 on success, 1 on failure (key already exists) */
int avltree_add_node(AVLTree *tree, void *key, void *val);

/* Look up a key. Return pointer to value, or NULL if it doesn't exist */
void *avltree_lookup(AVLTree *tree, const void *key);

/* Remove a node specified by its key. Return pointer to value, or NULL if key doesn't exist. */
void *avltree_delete_node(AVLTree *tree, const void *key);

/* Deallocate a tree */
void avltree_free(AVLTree *tree);

#endif
