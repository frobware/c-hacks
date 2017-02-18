#ifndef INC_chacks_btree_H
#define INC_chacks_btree_H

struct btree_node {
	void *data;
	struct btree_node *l;
	struct btree_node *r;
};

typedef int (*BTREE_NODE_EQUALS) (const void *a, const void *b);

/*
 * Find node which matches data.
 */
extern int btree_find(const struct btree_node *n, const void *data,
		      BTREE_NODE_EQUALS fn);

extern struct btree_node *btree_insert(struct btree_node *n, void *data);

#endif				/* INC_chacks_btree_H */
