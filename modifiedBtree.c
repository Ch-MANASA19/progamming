#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define ORDER 5   // B-Tree order (max children per node)

// Node structure of the B-Tree
struct BTreeNode {
    int keys[ORDER - 1];              // Array to store keys
    struct BTreeNode *children[ORDER]; // Array of child pointers
    bool leaf;                         // Is leaf node?
    int num_keys;                      // Number of keys in node
};

// Function to create a new B-Tree node
struct BTreeNode *createNode(bool leaf) {
    struct BTreeNode *newNode = (struct BTreeNode *)malloc(sizeof(struct BTreeNode));
    newNode->leaf = leaf;
    newNode->num_keys = 0;
    for (int i = 0; i < ORDER; i++) {
        newNode->children[i] = NULL;
    }
    return newNode;
}

// Function prototypes
bool search(struct BTreeNode *root, int key);
void insert(struct BTreeNode **root, int key);
void insertNonFull(struct BTreeNode *node, int key);
void splitChild(struct BTreeNode *parent, int i);
void deleteKey(struct BTreeNode **root, int key);
void deleteKeyHelper(struct BTreeNode *node, int key);
void removeFromLeaf(struct BTreeNode *node, int idx);
int getPredecessor(struct BTreeNode *node, int idx);
int getSuccessor(struct BTreeNode *node, int idx);
void mergeChildren(struct BTreeNode *node, int idx);
void fill(struct BTreeNode *node, int idx);
void borrowFromPrev(struct BTreeNode *node, int idx);
void borrowFromNext(struct BTreeNode *node, int idx);
void printTree(struct BTreeNode *root, int level);

// Function to search a key in the B-Tree
bool search(struct BTreeNode *root, int key) {
    if (!root) return false;

    int i = 0;
    while (i < root->num_keys && key > root->keys[i]) i++;

    if (i < root->num_keys && key == root->keys[i])
        return true;

    if (root->leaf) return false;

    return search(root->children[i], key);
}

// Function to insert a key into the B-Tree
void insert(struct BTreeNode **root, int key) {
    struct BTreeNode *node = *root;

    if (node == NULL) {
        *root = createNode(true);
        (*root)->keys[0] = key;
        (*root)->num_keys = 1;
    } else {
        if (node->num_keys == ORDER - 1) {
            struct BTreeNode *newRoot = createNode(false);
            newRoot->children[0] = node;
            *root = newRoot;
            splitChild(newRoot, 0);
            insertNonFull(newRoot, key);
        } else {
            insertNonFull(node, key);
        }
    }
}

// Insert into non-full node
void insertNonFull(struct BTreeNode *node, int key) {
    int i = node->num_keys - 1;

    if (node->leaf) {
        while (i >= 0 && key < node->keys[i]) {
            node->keys[i + 1] = node->keys[i];
            i--;
        }
        node->keys[i + 1] = key;
        node->num_keys++;
    } else {
        while (i >= 0 && key < node->keys[i]) i--;

        i++;
        if (node->children[i]->num_keys == ORDER - 1) {
            splitChild(node, i);
            if (key > node->keys[i]) i++;
        }
        insertNonFull(node->children[i], key);
    }
}

// Split child node
void splitChild(struct BTreeNode *parent, int i) {
    struct BTreeNode *child = parent->children[i];
    struct BTreeNode *newChild = createNode(child->leaf);

    int mid = ORDER / 2;
    newChild->num_keys = mid - 1;

    for (int j = 0; j < mid - 1; j++)
        newChild->keys[j] = child->keys[j + mid];

    if (!child->leaf) {
        for (int j = 0; j < mid; j++)
            newChild->children[j] = child->children[j + mid];
    }

    child->num_keys = mid - 1;

    for (int j = parent->num_keys; j >= i + 1; j--)
        parent->children[j + 1] = parent->children[j];

    parent->children[i + 1] = newChild;

    for (int j = parent->num_keys - 1; j >= i; j--)
        parent->keys[j + 1] = parent->keys[j];

    parent->keys[i] = child->keys[mid - 1];
    parent->num_keys++;
}

// Delete key
void deleteKey(struct BTreeNode **root, int key) {
    if (*root == NULL) return;

    deleteKeyHelper(*root, key);

    if ((*root)->num_keys == 0) {
        struct BTreeNode *tmp = *root;
        if ((*root)->leaf)
            *root = NULL;
        else
            *root = (*root)->children[0];
        free(tmp);
    }
}

// Helper for delete
void deleteKeyHelper(struct BTreeNode *node, int key) {
    int i = 0;
    while (i < node->num_keys && key > node->keys[i]) i++;

    if (i < node->num_keys && node->keys[i] == key) {
        if (node->leaf) {
            removeFromLeaf(node, i);
        } else {
            if (node->children[i]->num_keys >= ORDER / 2) {
                int pred = getPredecessor(node, i);
                node->keys[i] = pred;
                deleteKeyHelper(node->children[i], pred);
            } else if (node->children[i + 1]->num_keys >= ORDER / 2) {
                int succ = getSuccessor(node, i);
                node->keys[i] = succ;
                deleteKeyHelper(node->children[i + 1], succ);
            } else {
                mergeChildren(node, i);
                deleteKeyHelper(node->children[i], key);
            }
        }
    } else {
        if (node->leaf) return;

        bool flag = (i == node->num_keys);
        if (node->children[i]->num_keys < ORDER / 2) fill(node, i);

        if (flag && i > node->num_keys)
            deleteKeyHelper(node->children[i - 1], key);
        else
            deleteKeyHelper(node->children[i], key);
    }
}

// Remove from leaf
void removeFromLeaf(struct BTreeNode *node, int idx) {
    for (int i = idx + 1; i < node->num_keys; i++)
        node->keys[i - 1] = node->keys[i];
    node->num_keys--;
}

// Get predecessor
int getPredecessor(struct BTreeNode *node, int idx) {
    struct BTreeNode *curr = node->children[idx];
    while (!curr->leaf)
        curr = curr->children[curr->num_keys];
    return curr->keys[curr->num_keys - 1];
}

// Get successor
int getSuccessor(struct BTreeNode *node, int idx) {
    struct BTreeNode *curr = node->children[idx + 1];
    while (!curr->leaf)
        curr = curr->children[0];
    return curr->keys[0];
}

// Merge children
void mergeChildren(struct BTreeNode *node, int idx) {
    struct BTreeNode *child = node->children[idx];
    struct BTreeNode *sibling = node->children[idx + 1];

    child->keys[ORDER / 2 - 1] = node->keys[idx];
    for (int i = 0; i < sibling->num_keys; i++)
        child->keys[i + ORDER / 2] = sibling->keys[i];

    if (!child->leaf) {
        for (int i = 0; i <= sibling->num_keys; i++)
            child->children[i + ORDER / 2] = sibling->children[i];
    }

    child->num_keys += sibling->num_keys + 1;

    for (int i = idx + 1; i < node->num_keys; i++)
        node->keys[i - 1] = node->keys[i];

    for (int i = idx + 2; i <= node->num_keys; i++)
        node->children[i - 1] = node->children[i];

    node->num_keys--;
    free(sibling);
}

// Fill child if it has less than t-1 keys
void fill(struct BTreeNode *node, int idx) {
    if (idx != 0 && node->children[idx - 1]->num_keys >= ORDER / 2)
        borrowFromPrev(node, idx);
    else if (idx != node->num_keys && node->children[idx + 1]->num_keys >= ORDER / 2)
        borrowFromNext(node, idx);
    else {
        if (idx != node->num_keys)
            mergeChildren(node, idx);
        else
            mergeChildren(node, idx - 1);
    }
}

// Borrow from prev
void borrowFromPrev(struct BTreeNode *node, int idx) {
    struct BTreeNode *child = node->children[idx];
    struct BTreeNode *sibling = node->children[idx - 1];

    for (int i = child->num_keys - 1; i >= 0; i--)
        child->keys[i + 1] = child->keys[i];

    if (!child->leaf) {
        for (int i = child->num_keys; i >= 0; i--)
            child->children[i + 1] = child->children[i];
    }

    child->keys[0] = node->keys[idx - 1];

    if (!child->leaf)
        child->children[0] = sibling->children[sibling->num_keys];

    node->keys[idx - 1] = sibling->keys[sibling->num_keys - 1];

    child->num_keys++;
    sibling->num_keys--;
}

// Borrow from next
void borrowFromNext(struct BTreeNode *node, int idx) {
    struct BTreeNode *child = node->children[idx];
    struct BTreeNode *sibling = node->children[idx + 1];

    child->keys[child->num_keys] = node->keys[idx];

    if (!child->leaf)
        child->children[child->num_keys + 1] = sibling->children[0];

    node->keys[idx] = sibling->keys[0];

    for (int i = 1; i < sibling->num_keys; i++)
        sibling->keys[i - 1] = sibling->keys[i];

    if (!sibling->leaf) {
        for (int i = 1; i <= sibling->num_keys; i++)
            sibling->children[i - 1] = sibling->children[i];
    }

    child->num_keys++;
    sibling->num_keys--;
}

// Print the B-Tree
void printTree(struct BTreeNode *root, int level) {
    if (root != NULL) {
        printf("Level %d [", level);
        for (int i = 0; i < root->num_keys; i++) {
            printf(" %d", root->keys[i]);
        }
        printf(" ]\n");
        if (!root->leaf) {
            for (int i = 0; i <= root->num_keys; i++) {
                printTree(root->children[i], level + 1);
            }
        }
    }
}

// Main function
int main() {
    struct BTreeNode *root = NULL;

    int keys[] = {10, 20, 5, 6, 12, 30, 7, 17, 3, 9};
    int n = sizeof(keys) / sizeof(keys[0]);

    for (int i = 0; i < n; i++) {
        insert(&root, keys[i]);
        printf("\nInserted %d\n", keys[i]);
        printTree(root, 0);
    }

    printf("\nSearching...\n");
    int searchKeys[] = {6, 17, 8};
    for (int i = 0; i < 3; i++) {
        printf("Key %d %s\n", searchKeys[i], search(root, searchKeys[i]) ? "found" : "not found");
    }

    printf("\nDeleting 6...\n");
    deleteKey(&root, 6);
    printTree(root, 0);

    printf("\nDeleting 13...\n");
    deleteKey(&root, 13);
    printTree(root, 0);

    printf("\nDeleting 7...\n");
    deleteKey(&root, 7);
    printTree(root, 0);

    return 0;
}
