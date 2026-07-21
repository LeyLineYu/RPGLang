#ifndef DUMP_H
#define DUMP_H

#include <stdio.h>
#include "ds/tree/root.h"
#include "ds/hashtable/hashtable.h"

extern FILE* GRAPH_DUMP; 

// TODO: symbol offset is dumped incorrectly

/*
Accounted situations:
    -Premature next 0
    -Postmature prev 0
    -next/prev link to out-of-capacity non-existing-node
    -next/prev link from main to free element
    -next/prev link from free to main element
    -next/prev mixup in main (wrongly linked, no back-to-back linking)
    -prev at head isnt 0
    -next at tail isnt 0
*/
Error listDump_(FILE* f, List* lst, const char* commentary, 
                const char* filename, int line);

Error hashTableDump_(FILE* f, HashTable* table, const char* commentary, 
                     const char* filename, int line);

#define listDump(f, lst, commentary) \
        listDump_(f, lst, commentary, __FILE__, __LINE__)
#define hashTableDump(f, table, commentary) \
        hashTableDump_(f, table, commentary, __FILE__, __LINE__)

void rootDump(FILE* html, TreeRoot* root,
              const char* commentary, const char* filename, int line);
void nodeDump(FILE* html, TreeNode* node, 
              const char* commentary, const char* filename, int line);

#define rootDump(file, root, commentary) \
        rootDump(file, root, commentary, __FILE__, __LINE__)
#define nodeDump(file, node, commentary) \
        nodeDump(file, node, commentary, __FILE__, __LINE__)

/// if the directory in the dirPath doesn't exist, will return NULL
FILE* openHtmlLogFile(const char* dirPath);
void  closeHtmlLogFile(FILE* html);

#endif
