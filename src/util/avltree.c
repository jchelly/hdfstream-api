#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "avltree.h"

AVLTree *avltree_new(int (*cmpfunc) (const void *val1, const void *val2))
{
  AVLTree *tree  = malloc(sizeof(AVLTree));
  if(!tree)return NULL;
  tree->root     = NULL;
  tree->cmpfunc  = cmpfunc;
  tree->prev     = NULL;
  tree->first    = NULL;
  return tree;
}

static Node *new_node(AVLTree *tree)
{
  Node *node   = malloc(sizeof(Node));
  node->left   = NULL;
  node->right  = NULL;
  node->parent = NULL;
  node->val    = NULL;
  node->key    = NULL;
  node->next = NULL;
  node->prev = NULL;
  if(tree->prev)
    {
      tree->prev->next = node;
      node->prev = tree->prev;
    }
  tree->prev = node;

  if(!tree->first)tree->first = node;

  return node;
}

static Node *sibling(Node *node)
{
  /*
    Return a pointer to the next sibling of node 'node',
    or NULL if there isn't one.
  */

  if(!node->parent)return NULL;

  if(node->parent->left == node)
    return node->parent->right;
  else
    return NULL;
}

static Node *first_child(Node *node)
{
  /*
    Return a pointer to the 'leftmost' child of node -
    i.e. left child if there is one, otherwise right child.
    Returns NULL if there are no child nodes.
  */
  if(node->left)
    return node->left;
  else if(node->right)
    return node->right;
  else
    return NULL;
}

static void rotate_left(AVLTree *tree, Node *root)
{
  Node *pivot  = root->right;
  Node *parent = root->parent;

  if(root == tree->root)tree->root = pivot;

  root->right   = pivot->left;
  if(pivot->left)pivot->left->parent = root;

  pivot->left = root;
  root->parent = pivot;

  pivot->parent = parent;
  if(parent)
    {
      if(parent->left == root)
	parent->left = pivot;
      else
      	parent->right = pivot;
    }
}

static void rotate_right(AVLTree *tree, Node *root)
{
  Node *pivot  = root->left;
  Node *parent = root->parent;

  if(root == tree->root)tree->root = pivot;

  root->left   = pivot->right;
  if(pivot->right)pivot->right->parent = root;

  pivot->right = root;
  root->parent = pivot;

  pivot->parent = parent;
  if(parent)
    {
      if(parent->right == root)
	parent->right = pivot;
      else
      	parent->left = pivot;
    }
}

static int node_height(Node *root)
{
  /*
    Returns the height of the tree rooted at node.

    TODO - speed this up by storing heights in the tree.

    If root is NULL,         returns 0
    If root has no children, returns 1
    Otherwise returns height of tree.
  */

  if(!root)return 0;

  Node *node = first_child(root);
  if(node)
    {
      int height    = 2;
      int maxheight = 2;
      while(node != root)
	{
	  while(first_child(node))
	    {
	      node = first_child(node);
	      height += 1;
	      if(height > maxheight)maxheight=height;
	    }
	  while((!sibling(node)) && (node != root))
	    {
	      node   = node->parent;
	      height = height - 1;
	    }
	  if(node != root)
	    node = sibling(node);
	}
      return maxheight;
    }
  else
    {
      return 1;
    }
}


static void rebalance(AVLTree *tree, Node *node)
{
  /*
     Check tree is balanced, starting at node 'node' and working
     back towards the root
  */
  Node *root = node->parent;
  while(root)
    {
      Node *next = root->parent;
      int balance = node_height(root->right) - node_height(root->left);
      if(balance == 2)
	{
	  Node *r = root->right;
	  int  rb = node_height(r->right) - node_height(r->left);
	  if(rb==1)
	    {
	      rotate_left(tree,root);
	    }
	  else if(rb==-1)
	    {
	      rotate_right(tree,root->right);
	      rotate_left(tree,root);
	    }
	}
      else if(balance == -2)
	{

	  Node *l = root->left;
	  int  lb = node_height(l->right) - node_height(l->left);
	  if(lb==-1)
	    {
	      rotate_right(tree,root);
	    }
	  else if(lb==1)
	    {
	      rotate_left(tree,root->left);
	      rotate_right(tree,root);
	    }
	}
      root = next;
    }
}


void *avltree_lookup(AVLTree *tree, const void *key)
{
  Node *node = tree->root;

  while(node)
    {
      int iswitch = (*(tree->cmpfunc)) (key, node->key);
      if(iswitch<0)
	node = node->left;
      else if(iswitch>0)
	node = node->right;
      else
        return node->val;
    }
  return NULL;
}

int avltree_add_node(AVLTree *tree, void *key, void *val)
{
  if(tree->root)
    {
      /* Already have some nodes - insert a new one */
      Node *node = tree->root;
      int depth = 0;
      while(1)
	{
	  depth = depth + 1;
	  int iswitch = (*(tree->cmpfunc)) (key, node->key);
	  if(iswitch<0)
	    {
	      /* New key is smaller */
	      if(node->left)
		node = node->left;
	      else
		{
		  node->left = new_node(tree);
		  node->left->parent = node;
		  node = node->left;
		  break;
		}
	    }
	  else if(iswitch>0)
	    {
	      /* New key is larger */
	      if(node->right)
		node = node->right;
	      else
		{
		  node->right = new_node(tree);
		  node->right->parent = node;
		  node = node->right;
		  break;
		}
	    }
	  else
	    {
	      /* Strings are the same */
	      return 1;
	    }
	}
      node->key = key;
      node->val = val;

      /* Check tree is balanced */
      rebalance(tree,node);
    }
  else
    {
      /* This is the first node so its the root */
      Node *node = new_node(tree);
      node->key = key;
      node->val = val;
      tree->root = node;
    }

  return 0;
}


static void dealloc_node(AVLTree *tree, Node *node)
{
  if(node==tree->first) {
    /* Removing first node in linked list */
    tree->first = node->next;
  }
  if(node==tree->prev) {
    /* Removing last node in linked list */
    tree->prev = node->prev;
  }
  if(node->prev)
      node->prev->next = node->next;
  if(node->next)
    node->next->prev = node->prev;
  free(node);
}

void *avltree_delete_node(AVLTree *tree, const void *key)
{

  /* Find node to remove */
  Node *target = NULL;
  Node *node   = tree->root;
  void *val = NULL;
  while(node)
    {
      int iswitch = (*(tree->cmpfunc)) (key, node->key);
      if(iswitch<0)
	node = node->left;
      else if(iswitch>0)
	node = node->right;
      else
	{
	  /* Found it! */
	  target = node;
          val = node->val;
	  break;
	}
    }
  if(!target)return NULL;

  /* Find replacement node */
  Node *replace = NULL;
  Node *parent  = target->parent;
  if(target->left)
    {
      /* Largest in left subtree */
      replace = target->left;
      while(replace->right)
	replace = replace->right;

      /* Replace target node with replacement node */
      target->key = replace->key;
      target->val = replace->val;

      /* Remove replacement node */
      if(replace->parent == target)
	replace->parent->left  = replace->left;
      else
	replace->parent->right = replace->left;
      if(replace->left)
	replace->left->parent = replace->parent;
      dealloc_node(tree, replace);
    }
  else if(target->right)
    {
      /* Smallest in right subtree */
      replace = target->right;
      while(replace->left)
	replace = replace->left;

      /* Replace target node with replacement node by copying
	 key and value */
      target->key = replace->key;
      target->val = replace->val;

      /* Remove replacement node */
      if(replace->parent == target)
	replace->parent->right  = replace->right;
      else
	replace->parent->left = replace->right;
      if(replace->right)
	replace->right->parent = replace->parent;
      dealloc_node(tree, replace);
    }
  else
    {
      /* Leaf node, so no subtrees but this might be the root */
      if(target != tree->root) {
        if(target->parent->left == target)
          target->parent->left = NULL;
        else
          target->parent->right = NULL;
      } else {
        tree->root = NULL;
      }
      dealloc_node(tree, target);
    }

  if(parent)
    rebalance(tree, parent);

  return val;
}


void avltree_free(AVLTree *tree)
{
  /* Deallocate all nodes */
  Node *node = tree->first;
  while(node)
    {
      Node *next = node->next;
      free(node);
      node = next;
    }
  free(tree);
}
