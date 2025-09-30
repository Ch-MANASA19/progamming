#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#define T 3  // Minimum degree. Change T to adjust tree branching. T=3 => max keys = 2*T-1 = 5

// B-Tree node structure
typedef struct BTreeNode {
    int keys[2 * T - 1];
    struct BTreeNode *children[2 * T];
    int n;           // current number of keys
    bool leaf;
} BTreeNode;

// Create a new B-Tree node
BTreeNode *bt_create_node(bool leaf) {
    BTreeNode *node = (BTreeNode *)malloc(sizeof(BTreeNode));
    if (!node) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    node->leaf = leaf;
    node->n = 0;
    for (int i = 0; i < 2 * T; ++i) node->children[i] = NULL;
    return node;
}

// Search key in subtree rooted with node
bool bt_search(BTreeNode *node, int k) {
    if (!node) return false;
    int i = 0;
    while (i < node->n && k > node->keys[i]) i++;
    if (i < node->n && node->keys[i] == k) return true;
    if (node->leaf) return false;
    return bt_search(node->children[i], k);
}

// Split child y of x at index i (y is full)
void bt_split_child(BTreeNode *x, int i) {
    BTreeNode *y = x->children[i];
    BTreeNode *z = bt_create_node(y->leaf);
    z->n = T - 1; // z will take last T-1 keys from y

    // copy last T-1 keys of y to z
    for (int j = 0; j < T - 1; j++)
        z->keys[j] = y->keys[j + T];

    // copy last T children of y to z if not leaf
    if (!y->leaf) {
        for (int j = 0; j < T; j++)
            z->children[j] = y->children[j + T];
    }

    // reduce number of keys in y
    y->n = T - 1;

    // create space in x for new child
    for (int j = x->n; j >= i + 1; j--)
        x->children[j + 1] = x->children[j];

    x->children[i + 1] = z;

    // move keys in x to make space for median
    for (int j = x->n - 1; j >= i; j--)
        x->keys[j + 1] = x->keys[j];

    // put median key of y into x
    x->keys[i] = y->keys[T - 1];
    x->n += 1;
}

// Insert when root is not full
void bt_insert_nonfull(BTreeNode *x, int k) {
    int i = x->n - 1;
    if (x->leaf) {
        // shift keys to make room
        while (i >= 0 && k < x->keys[i]) {
            x->keys[i + 1] = x->keys[i];
            i--;
        }
        x->keys[i + 1] = k;
        x->n += 1;
    } else {
        // find child to descend into
        while (i >= 0 && k < x->keys[i]) i--;
        i++;
        if (x->children[i]->n == 2 * T - 1) {
            bt_split_child(x, i);
            if (k > x->keys[i]) i++;
        }
        bt_insert_nonfull(x->children[i], k);
    }
}

// Insert key into B-Tree
BTreeNode *bt_insert(BTreeNode *root, int k) {
    if (!root) {
        root = bt_create_node(true);
        root->keys[0] = k;
        root->n = 1;
        return root;
    }
    if (root->n == 2 * T - 1) {
        // root is full, need new root
        BTreeNode *s = bt_create_node(false);
        s->children[0] = root;
        bt_split_child(s, 0);
        int i = (s->keys[0] < k) ? 1 : 0;
        bt_insert_nonfull(s->children[i], k);
        return s;
    } else {
        bt_insert_nonfull(root, k);
        return root;
    }
}

// Utility to get predecessor (max key in subtree rooted at node->children[idx])
int bt_get_predecessor(BTreeNode *node, int idx) {
    BTreeNode *cur = node->children[idx];
    while (!cur->leaf) cur = cur->children[cur->n];
    return cur->keys[cur->n - 1];
}

// Utility to get successor (min key in subtree rooted at node->children[idx+1])
int bt_get_successor(BTreeNode *node, int idx) {
    BTreeNode *cur = node->children[idx + 1];
    while (!cur->leaf) cur = cur->children[0];
    return cur->keys[0];
}

// Merge children idx and idx+1 of node. Pull down key[idx] into merged child.
void bt_merge(BTreeNode *node, int idx) {
    BTreeNode *child = node->children[idx];
    BTreeNode *sibling = node->children[idx + 1];

    // pull key from node down to child
    child->keys[T - 1] = node->keys[idx];

    // copy keys from sibling to child
    for (int i = 0; i < sibling->n; ++i)
        child->keys[i + T] = sibling->keys[i];

    // copy children as well
    if (!child->leaf) {
        for (int i = 0; i <= sibling->n; ++i)
            child->children[i + T] = sibling->children[i];
    }

    child->n += sibling->n + 1;

    // shift keys and children in node
    for (int i = idx + 1; i < node->n; ++i)
        node->keys[i - 1] = node->keys[i];
    for (int i = idx + 2; i <= node->n; ++i)
        node->children[i - 1] = node->children[i];

    node->n--;

    free(sibling);
}

// Borrow from previous sibling
void bt_borrow_from_prev(BTreeNode *node, int idx) {
    BTreeNode *child = node->children[idx];
    BTreeNode *sibling = node->children[idx - 1];

    // shift child's keys and children right by 1
    for (int i = child->n - 1; i >= 0; --i)
        child->keys[i + 1] = child->keys[i];

    if (!child->leaf) {
        for (int i = child->n; i >= 0; --i)
            child->children[i + 1] = child->children[i];
    }

    // put key from node down to child
    child->keys[0] = node->keys[idx - 1];

    if (!child->leaf)
        child->children[0] = sibling->children[sibling->n];

    // move sibling's last key up to node
    node->keys[idx - 1] = sibling->keys[sibling->n - 1];

    child->n += 1;
    sibling->n -= 1;
}

// Borrow from next sibling
void bt_borrow_from_next(BTreeNode *node, int idx) {
    BTreeNode *child = node->children[idx];
    BTreeNode *sibling = node->children[idx + 1];

    // node's key moves to child's last key
    child->keys[child->n] = node->keys[idx];

    if (!child->leaf)
        child->children[child->n + 1] = sibling->children[0];

    // sibling's first key moves up to node
    node->keys[idx] = sibling->keys[0];

    // shift keys and children in sibling left by 1
    for (int i = 1; i < sibling->n; ++i)
        sibling->keys[i - 1] = sibling->keys[i];
    if (!sibling->leaf) {
        for (int i = 1; i <= sibling->n; ++i)
            sibling->children[i - 1] = sibling->children[i];
    }

    child->n += 1;
    sibling->n -= 1;
}

// Ensure child idx has at least T-1 keys
void bt_fill(BTreeNode *node, int idx) {
    if (idx != 0 && node->children[idx - 1]->n >= T)
        bt_borrow_from_prev(node, idx);
    else if (idx != node->n && node->children[idx + 1]->n >= T)
        bt_borrow_from_next(node, idx);
    else {
        if (idx != node->n)
            bt_merge(node, idx);
        else
            bt_merge(node, idx - 1);
    }
}

// Remove key k from subtree rooted with node
void bt_remove_from_node(BTreeNode *node, int k);

// Remove key present in leaf node at idx
void bt_remove_from_leaf(BTreeNode *node, int idx) {
    for (int i = idx + 1; i < node->n; ++i)
        node->keys[i - 1] = node->keys[i];
    node->n--;
}

// Remove key present in non-leaf node at idx
void bt_remove_from_nonleaf(BTreeNode *node, int idx) {
    int k = node->keys[idx];
    // If the child before idx has at least T keys, find predecessor
    if (node->children[idx]->n >= T) {
        int pred = bt_get_predecessor(node, idx);
        node->keys[idx] = pred;
        bt_remove_from_node(node->children[idx], pred);
    }
    // Else if child after idx has at least T keys, find successor
    else if (node->children[idx + 1]->n >= T) {
        int succ = bt_get_successor(node, idx);
        node->keys[idx] = succ;
        bt_remove_from_node(node->children[idx + 1], succ);
    } else {
        // Merge children and then remove k from merged child
        bt_merge(node, idx);
        bt_remove_from_node(node->children[idx], k);
    }
}

void bt_remove_from_node(BTreeNode *node, int k) {
    int idx = 0;
    while (idx < node->n && node->keys[idx] < k) idx++;

    if (idx < node->n && node->keys[idx] == k) {
        if (node->leaf)
            bt_remove_from_leaf(node, idx);
        else
            bt_remove_from_nonleaf(node, idx);
    } else {
        // Key not present in this node
        if (node->leaf) return; // key not found
        bool flag = (idx == node->n);
        if (node->children[idx]->n < T)
            bt_fill(node, idx);

        if (flag && idx > node->n)
            bt_remove_from_node(node->children[idx - 1], k);
        else
            bt_remove_from_node(node->children[idx], k);
    }
}

// Remove key from B-Tree; adjust root if necessary
BTreeNode *bt_remove(BTreeNode *root, int k) {
    if (!root) return NULL;
    bt_remove_from_node(root, k);
    if (root->n == 0) {
        BTreeNode *tmp = root;
        if (root->leaf) {
            free(root);
            root = NULL;
        } else {
            root = root->children[0];
            free(tmp);
        }
    }
    return root;
}

// Print tree structure (preorder) with indentation
void bt_print(BTreeNode *root, int level) {
    if (!root) return;
    for (int i = 0; i < level; ++i) printf("  ");
    printf("[");
    for (int i = 0; i < root->n; ++i) {
        printf("%d", root->keys[i]);
        if (i + 1 < root->n) printf(" ");
    }
    printf("]\n");
    if (!root->leaf) {
        for (int i = 0; i <= root->n; ++i)
            bt_print(root->children[i], level + 1);
    }
}

// Utility: generate N unique random numbers in range [0..maxv)
void gen_unique_randoms(int *arr, int N, int maxv) {
    if (N > maxv) {
        fprintf(stderr, "Not enough unique values available\n");
        exit(EXIT_FAILURE);
    }
    // simple reservoir-like / shuffle approach
    int *pool = malloc(sizeof(int) * maxv);
    for (int i = 0; i < maxv; ++i) pool[i] = i + 1; // 1..maxv
    // Fisher-Yates shuffle first N positions
    for (int i = 0; i < N; ++i) {
        int j = i + rand() % (maxv - i);
        int tmp = pool[i]; pool[i] = pool[j]; pool[j] = tmp;
        arr[i] = pool[i];
    }
    free(pool);
}

int main(void) {
    srand((unsigned)time(NULL));

    // prepare 100 unique random elements
    const int N = 100;
    int arr[N];
    gen_unique_randoms(arr, N, 1000); // choose unique numbers from 1..1000

    BTreeNode *root = NULL;

    printf("Inserting 100 random keys into B-Tree (T=%d)...\n", T);
    for (int i = 0; i < N; ++i) {
        root = bt_insert(root, arr[i]);
    }

    printf("\nB-Tree structure after inserts:\n");
    bt_print(root, 0);

    // Demonstrate search
    printf("\nSearch demo:\n");
    int to_search[5] = {arr[0], arr[10], arr[20], 9999, arr[99]}; // include a not-present value 9999
    for (int i = 0; i < 5; ++i) {
        int k = to_search[i];
        printf("Searching %d -> %s\n", k, bt_search(root, k) ? "FOUND" : "NOT FOUND");
    }

    // Demonstrate deletions: remove 10 keys (first 10 inserted)
    printf("\nDeleting 10 keys (first 10 inserted):\n");
    for (int i = 0; i < 10; ++i) {
        int key = arr[i];
        printf("Deleting %d\n", key);
        root = bt_remove(root, key);
    }

    printf("\nB-Tree structure after deletions:\n");
    bt_print(root, 0);

    // free tree would be nice; skipping for brevity (program ends)
    return 0;
}
