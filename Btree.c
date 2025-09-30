//Construct B-Tree an order of 5 with a set of 100 random elements stored in array. Implement searching, insertion and deletion operations.

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#define ORDER 5
// Node structure of the B-Tree
struct BTreeNode {
    int keys[ORDER - 1]; // Array to store keys
    struct BTreeNode *children[ORDER]; // Array of child pointers
    bool leaf; // Flag to indicate if node is a leaf or not
    int num_keys; // Number of keys currently in the node
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
void printTree(struct BTreeNode *root);
// Function to search a key in the B-Tree
bool search(struct BTreeNode *root, int key) {
    int i = 0;
    while (i < root->num_keys && key > root->keys[i]) {
        i++;
    }
    if (i < root->num_keys && key == root->keys[i]) {
        return true; // Key found
    }
    if (root->leaf) {
        return false; // Key not found
    }
    return search(root->children[i], key); // Recursively search in the appropriate child
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
// Function to insert a key into a non-full B-Tree node
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
        while (i >= 0 && key < node->keys[i]) {
            i--;
        }
        i++;
        if (node->children[i]->num_keys == ORDER - 1) {
            splitChild(node, i);
            if (key > node->keys[i]) {
                i++;
            }
        }
        insertNonFull(node->children[i], key);
    }
}
// Function to split a child of a B-Tree node
void splitChild(struct BTreeNode *parent, int i) {
    struct BTreeNode *child = parent->children[i];
    struct BTreeNode *newChild = createNode(child->leaf);
    newChild->num_keys = ORDER / 2 - 1;
    for (int j = 0; j < ORDER / 2 - 1; j++) {
        newChild->keys[j] = child->keys[j + ORDER / 2];
    }
    if (!child->leaf) {
        for (int j = 0; j < ORDER / 2; j++) {
            newChild->children[j] = child->children[j + ORDER / 2];
        }
    }
    child->num_keys = ORDER / 2 - 1;
    for (int j = parent->num_keys; j >= i + 1; j--) {
        parent->children[j + 1] = parent->children[j];
    }
    parent->children[i + 1] = newChild;
    for (int j = parent->num_keys - 1; j >= i; j--) {
        parent->keys[j + 1] = parent->keys[j];
    }
    parent->keys[i] = child->keys[ORDER / 2 - 1];
    parent->num_keys++;
}
// Function to delete a key from the B-Tree
void deleteKey(struct BTreeNode **root, int key) {
    struct BTreeNode *node = *root;
    deleteKeyHelper(node, key);
    if (node->num_keys == 0 && !node->leaf) {
        *root = node->children[0];
        free(node);
    }
}
// Function to delete a key from a B-Tree node
void deleteKeyHelper(struct BTreeNode *node, int key) {
    int i = 0;
    while (i < node->num_keys && key > node->keys[i]) {
        i++;
    }
    if (i < node->num_keys && key == node->keys[i]) {
        if (node->leaf) {
            removeFromLeaf(node, i);
        } else {
            if (node->children[i]->num_keys >= ORDER / 2) {
                int predecessor = getPredecessor(node, i);
                node->keys[i] = predecessor;
                deleteKeyHelper(node->children[i], predecessor);
            } else if (node->children[i + 1]->num_keys >= ORDER / 2) {
                int successor = getSuccessor(node, i);
                node->keys[i] = successor;
                deleteKeyHelper(node->children[i + 1], successor);
            } else {
                mergeChildren(node, i);
                deleteKeyHelper(node->children[i], key);
            }
        }
    } else {
        if (node->leaf) {
            return;
        }
        if (node->children[i]->num_keys < ORDER / 2) {
            fill(node, i);
        }
        deleteKeyHelper(node->children[i], key);
    }
}
// Function to remove a key from a leaf node
void removeFromLeaf(struct BTreeNode *node, int idx) {
    for (int i = idx + 1; i < node->num_keys; i++) {
        node->keys[i - 1] = node->keys[i];
    }
    node->num_keys--;
}
// Function to get predecessor key in a B-Tree node
int getPredecessor(struct BTreeNode *node, int idx) {
    struct BTreeNode *curr = node->children[idx];
    while (!curr->leaf) {
        curr = curr->children[curr->num_keys];
    }
    return curr->keys[curr->num_keys - 1];
}
// Function to get successor key in a B-Tree node
int getSuccessor(struct BTreeNode *node, int idx) {
    struct BTreeNode *curr = node->children[idx + 1];
    while (!curr->leaf) {
        curr = curr->children[0];
    }
    return curr->keys[0];
}
// Function to merge a child node with its sibling
void mergeChildren(struct BTreeNode *node, int idx) {
    struct BTreeNode *child = node->children[idx];
    struct BTreeNode *sibling = node->children[idx + 1];
    child->keys[ORDER / 2 - 1] = node->keys[idx];
    for (int i = 0; i < sibling->num_keys; i++) {
        child->keys[i + ORDER / 2] = sibling->keys[i];
    }
    if (!child->leaf) {
        for (int i = 0; i <= sibling->num_keys; i++) {
            child->children[i + ORDER / 2] = sibling->children[i];
        }
    }
    child->num_keys += sibling->num_keys + 1;
    free(sibling);
    for (int i = idx + 1; i < node->num_keys; i++) {
        node->keys[i - 1] = node->keys[i];
    }
    for (int i = idx + 2; i <= node->num_keys; i++) {
        node->children[i - 1] = node->children[i];
    }
    node->num_keys--;
}
// Function to fill a B-Tree node that has less than (ORDER/2) keys
void fill(struct BTreeNode *node, int idx) {
    if (idx > 0 && node->children[idx - 1]->num_keys >= ORDER / 2) {
        borrowFromPrev(node, idx);
    } else if (idx < node->num_keys && node->children[idx + 1]->num_keys >= ORDER / 2) {
        borrowFromNext(node, idx);
    } else {
        if (idx < node->num_keys) {
            mergeChildren(node, idx);
        } else {
            mergeChildren(node, idx - 1);
        }
    }
}
// Function to borrow a key from the previous child node
void borrowFromPrev(struct BTreeNode *node, int idx) {
    struct BTreeNode *child = node->children[idx];
    struct BTreeNode *sibling = node->children[idx - 1];
    for (int i = child->num_keys - 1; i >= 0; i--) {
        child->keys[i + 1] = child->keys[i];
    }
    if (!child->leaf) {
        for (int i = child->num_keys; i >= 0; i--) {
            child->children[i + 1] = child->children[i];
        }
    }
    child->keys[0] = node->keys[idx - 1];
    if (!child->leaf) {
        child->children[0] = sibling->children[sibling->num_keys];
    }
    node->keys[idx - 1] = sibling->keys[sibling->num_keys - 1];
    child->num_keys++;
    sibling->num_keys--;
}
// Function to borrow a key from the next child node
void borrowFromNext(struct BTreeNode *node, int idx) {
    struct BTreeNode *child = node->children[idx];
    struct BTreeNode *sibling = node->children[idx + 1];
    child->keys[child->num_keys] = node->keys[idx];
    if (!child->leaf) {
        child->children[child->num_keys + 1] = sibling->children[0];
    }
    node->keys[idx] = sibling->keys[0];
    for (int i = 1; i < sibling->num_keys; i++) {
        sibling->keys[i - 1] = sibling->keys[i];
    }
    if (!sibling->leaf) {
        for (int i = 1; i <= sibling->num_keys; i++) {
            sibling->children[i - 1] = sibling->children[i];
        }
    }
    child->num_keys++;
    sibling->num_keys--;
}
// Function to print the B-Tree
void printTree(struct BTreeNode *root) {
    if (root != NULL) {
        for (int i = 0; i < root->num_keys; i++) {
            printf("%d ", root->keys[i]);
        }
        printf("\n");
        if (!root->leaf) {
            for (int i = 0; i <= root->num_keys; i++) {
                printTree(root->children[i]);
            }
        }
    }
}
// Main function for testing
int main() {
    struct BTreeNode *root = NULL;

    // Inserting keys into the B-Tree
    int keys[] = {10, 20, 5, 6, 12, 30, 7, 17, 3, 9};
    int num_keys = sizeof(keys) / sizeof(keys[0]);

    for (int i = 0; i < num_keys; i++) {
        insert(&root, keys[i]);
        printf("Inserted key %d: \n", keys[i]);
        printTree(root);
        printf("\n");
    }
    // Searching for keys in the B-Tree
    int search_keys[] = {6, 17, 8};
    int num_search_keys = sizeof(search_keys) / sizeof(search_keys[0]);

    for (int i = 0; i < num_search_keys; i++) {
        if (search(root, search_keys[i])) {
            printf("Key %d found in the B-Tree.\n", search_keys[i]);
        } else {
            printf("Key %d not found in the B-Tree.\n", search_keys[i]);
        }
    }
    // Deleting keys from the B-Tree
   
    return 0;
