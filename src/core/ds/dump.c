#include "ds/dump.h"
#include "logger/logger.h"
#undef rootDump
#undef nodeDump

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "utils/utils.h"
#include "ds/queue/queue.h"

/// Global variable for graph dumps, 
/// so that i dont have to pass the file pointer around, 
/// since this is a debug feature anyway
FILE* GRAPH_DUMP = NULL;

static uint CALL_COUNT = 0;

//these aren't static consts but macros because of how C treats
//constant-variable sized arrays (for some reason mistakes them for VLAs)
#define DOT_PATH_BUF_SZ  128
#define HTML_PATH_BUF_SZ 128
#define IMG_PATH_BUF_SZ  128
#define DOT_CMD_BUF_SZ   512

_unused static const char* BG_COLOR      = "#FFFFFF";
_unused static const char* BAD_OUTLINE   = "#602222";
_unused static const char* BAD_FILL      = "#F02222";
_unused static const char* DEFAULT_CELL  = "#F02222";
_unused static const char* OP_CELL       = "#A6E3A1";
_unused static const char* NUM_CELL      = "#6CA1F9";
_unused static const char* RAW_ID_CELL   = "#FF81B4";
_unused static const char* SYMBOL_CELL   = "#B581F4";
_unused static const char* CTRL_CELL     = "#F9E2AF";
_unused static const char* TYPE_CELL     = "#FAB387";
_unused static const char* TABLE_OUTLINE = "#101510";
_unused static const char* TABLE_FILL    = "#10151034";
_unused static const char* ADDRESS_FILL  = "#10151034";
_unused static const char* LEFT_FILL     = "#10151034";
_unused static const char* RIGHT_FILL    = "#10151034";
_unused static const char* PARENT_FILL   = "#10151034";
_unused static const char* VALUE_FILL    = "#10151034";
_unused static const char* TYPE_FILL     = "#10151034";
_unused static const char* EXC_FILL      = "#10151034";
_unused static const char* OK_EDGE       = "#2222E0";
_unused static const char* BAD_EDGE      = "#E02222";
_unused static const char* ROOT_OUTLINE  = "#666666";
_unused static const char* ROOT_FILL     = "#DDDDDD";
_unused static const char* LIST_CELL     = "#8673BA";
_unused static const char* NEXT_FILL     = "#10151034";
_unused static const char* PREV_FILL     = "#10151034";
_unused static const char* INDEX_FILL    = "#10151034";
_unused static const char* FREE_EDGE     = "#20B412";
_unused static const char* FREE_OUTLINE  = "#26721F";
_unused static const char* FREE_FILL     = "#64BD6C";
_unused static const char* TAIL_OUTLINE  = "#666666";
_unused static const char* TAIL_FILL     = "#DDDDDD";
_unused static const char* HEAD_OUTLINE  = "#666666";
_unused static const char* HEAD_FILL     = "#DDDDDD";

static Error listTextDump(FILE* f, List* lst,
                          const char* commentary, 
                          const char* filename, int line);
static Error listGraphDump(FILE* f, List* lst);
static Error hashTableTextDump(FILE* f, HashTable* table,
                               const char* commentary, 
                               const char* filename, int line);
static Error hashTableGraphDump(FILE* f, HashTable* table);
static Error rootTextDump(FILE* f, TreeRoot* root,
                          const char* commentary, const char* filename, int line);
static Error rootGraphDump(FILE* f, TreeRoot* root);
static void populateDot(FILE* dot, TreeNode* node);
static void declareNode(FILE* dot, TreeNode* node, bool bondFailed);
static void declareRank(FILE* dot, TreeNode* node, Queue** queue);
static void executeDot(FILE* f, char* dotPath);

#define WARNING_PREFIX(condition) (condition) ? "<b><body><font color=\"red\">[!]</font></body></b>" : ""

#define DOT_HEADER_INIT(file)                                                               \
  fprintf(file,                                                                             \
          "digraph G {\n"                                                                   \
          "rankdir=TB;\n"                                                                   \
          "graph [bgcolor=\"%s\", pad=0.25, nodesep=0.55, "                                 \
                 "ranksep=0.9, splines=ortho, ordering=\"in\"];\n"                          \
          "node [shape=hexagon, style=\"filled\", color=\"%s\", penwidth=1.4, "             \
                "fillcolor=\"%s\", fontname=\"monospace\", fontsize=30];\n"                 \
          "edge [color=\"%s\", penwidth=2.5, weight = 0, arrowsize=0.8, arrowhead=vee];\n", \
          BG_COLOR,                                                                         \
          BAD_OUTLINE, BAD_FILL,                                                            \
          BG_COLOR);

Error listDump_(FILE* f, List* lst, const char* commentary, 
    const char* filename, int line) {
  if (!f || !filename || !commentary)
    return BadArgs;

  ++CALL_COUNT;

  listVerify(lst);

  if (listTextDump(f, lst, commentary, 
                   filename, line))
    return OK;

  fputs("Graphical Dump:\n", f);
  return listGraphDump(f, lst);
}

Error hashTableDump_(FILE* f, HashTable* table, const char* commentary, 
                     const char* filename, int line) {
  if (!f || !filename || !commentary)
    return BadArgs;

  ++CALL_COUNT;

  hashTableVerify(table);

  if (hashTableTextDump(f, table, commentary, 
                        filename, line))
    return OK;

  return hashTableGraphDump(f, table);
}

static Error hashTableTextDump(FILE* f, HashTable* table,
                               const char* commentary, const char* filename, 
                               int line) {
  assert(f);
  assert(filename);
  assert(commentary);

  if (!table) {
    fprintf(f,
            "%s\n"
            "HashTable Dump #%u called from %s:%d\n"
            "Textual Dump:\n"
            "HashTable [NULL] {}\n",
            commentary,
            CALL_COUNT, filename, line);
    return Fail;
  }

  fprintf(f,
          "%s\n"
          "HashTable Dump #%u called from %s:%d\n"
          "Textual Dump:\n"
          "HashTable [%p] {\n"
          "\t...\n"
          "\t%sbuckets = %p\n"
          "\tbucketCount = %zu\n"
          "\t%shashFunc = %p\n"
          "\t%sinitialized = %s\n"
          "}\n",
          commentary,
          CALL_COUNT, filename, line,
          table,
          WARNING_PREFIX(!table->buckets), table->buckets,
          table->bucketCount,
          WARNING_PREFIX(!table->hashFunc), table->hashFunc,
          WARNING_PREFIX(!table->initialized), table->initialized ? "true" : "false");

  return table->buckets ? OK : Fail;
}

static Error hashTableGraphDump(FILE* f, HashTable* table) {
    assert(f);
    assert(table);
    assert(table->buckets);

    Error err = OK;
    ++CALL_COUNT;
    if ((err = listGraphDump(f, &table->values))) 
      return err;

    for (size_t i = 0; i < table->bucketCount; i++) {
      ++CALL_COUNT;
      if ((err = listGraphDump(f, table->buckets + i))) 
        return err;
    }

    return OK;
}

static Error listTextDump(FILE* f, List* lst,
                          const char* commentary, const char* filename, 
                          int line) {
    assert(f);
    assert(filename);
    assert(commentary);

    if (!lst) {
        fprintf(f,
            "%s\n"
            "List Dump #%u called from %s:%d\n"
            "Textual Dump:\n"
            "List [NULL] {}\n",
            commentary,
            CALL_COUNT, filename, line);
        return Fail;
    }

    bool isDataNull = !lst->data;
    bool isNextNull = !lst->next;
    bool isPrevNull = !lst->prev;

    fprintf(f,
            "%s\n"
            "List Dump #%u called from %s:%d\n"
            "Textual Dump:\n"
            "List [%p] {\n"
            "\tcapacity = %lu\n"
            "\tisDoubleLinked = %s\n"
            "\thead = %lu\n"
            "\ttail = %lu\n"
            "\tfree = %lu\n"
            "\tfreeTail = %lu\n"
            "\t%sstatus = %d\n"
            "\t%sdata = %p\n"
            "\t%snext = %p\n"
            "\t%sprev = %p\n"
            "\t<b>|     index     |     next     |     prev     |     data     |\n",
            commentary,
            CALL_COUNT, filename, line,
            lst,
            lst->capacity,
            lst->isDoubleLinked ? "true" : "false",
            lst->next[0],
            lst->prev[0],
            lst->free,
            lst->freeTail,
            WARNING_PREFIX(lst->status), lst->status,
            WARNING_PREFIX(isDataNull),  lst->data,
            WARNING_PREFIX(isNextNull),  lst->next,
            WARNING_PREFIX(isPrevNull && lst->isDoubleLinked),  lst->prev);

    if (isDataNull)
        return Fail;

    for (ListIndex i = 0; i < lst->capacity; i++) {
      fprintf(f,
              "\t| "LIST_INDEX_FMT" | "LIST_INDEX_FMT" | "LIST_INDEX_FMT" |",
              i, isNextNull ? 0 : lst->next[i], isPrevNull ? 0 : lst->prev[i]);

      if (i != 0)
        lst->printfFunc(f, listGet(lst, i));
      else 
        fprintf(f, "%.*s", (int)lst->itemSize, listGet(lst, i));

      fprintf(f, "\n\t----------------------------------------------\n");
    }
    fprintf(f, "</b>}\n");

    return OK;
}

static Error listGraphDump(FILE* f, List* lst) {
    assert(f);
    assert(lst);

    if ((!lst->prev && lst->isDoubleLinked) || !lst->next) {
        loglnTraced(ERROR, "Prev or Next are NULL in a list that needs them to dump!");
        return Fail;
    }

    char dotPath[DOT_PATH_BUF_SZ] = {};
    if (snTimestampedFilename(dotPath, DOT_PATH_BUF_SZ, ".log/dot-", ".txt", CALL_COUNT)) {
        loglnTraced(ERROR, "Dot file name composition for this graph dump");
        return Fail;
    }

    FILE* dot = fopen(dotPath, "w");
    if (!dot) {
        loglnTraced(ERROR, "Dot file open failed for this graph dump");
        return Fail;
    }

    DOT_HEADER_INIT(dot);
    fprintf(dot, "free [shape=box, style=\"filled\", color=\"%s\", "
                       "fillcolor=\"%s\", label=\"FREE\", fontsize=28]\n"
                 "freeTail [shape=box, style=\"filled\", color=\"%s\", "
                       "fillcolor=\"%s\", label=\"FREE TAIL\", fontsize=28]\n"
                 "head [shape=box, style=\"filled\", color=\"%s\", "
                       "fillcolor=\"%s\", label=\"HEAD\", fontsize=28]\n"
                 "tail [shape=box, style=\"filled\", color=\"%s\", "
                       "fillcolor=\"%s\", label=\"TAIL\", fontsize=28]\n",
                 FREE_OUTLINE, FREE_FILL,
                 FREE_OUTLINE, FREE_FILL,
                 HEAD_OUTLINE, HEAD_FILL,
                 TAIL_OUTLINE, TAIL_FILL);

    bool* isFree = (bool*)calloc(lst->capacity, sizeof(bool));
    if (!isFree) {
        loglnTraced(ERROR, "Failed to allocate isFree for this graph dump!");
        return Fail;
    }
    for (ListIndex i = lst->free; i < lst->capacity && i != 0; i = lst->next[i]) {
        if (!lst->isDoubleLinked || (!lst->prev[i] && i != lst->next[0])) {
            if (isFree[i])
                break;
            isFree[i] = 1;
        } else {
            break;
        }
    }

    for (ListIndex i = 0; i < lst->capacity; i++) {
        if (lst->isDoubleLinked) {
            fprintf(dot,
                    "node%lu"
                    "[shape=box, style=\"rounded, filled\", color=\"%s\", fillcolor=\"%s\", penwidth=2.1, fontsize=14, label="
                    "<<table border=\"0\" cellborder=\"1\" cellspacing=\"0\" cellpadding=\"4\" color=\"%s\">"
                    "<tr>"
                        "<td bgcolor=\"%s\"><b>prev:</b> %lu</td>"
                        "<td bgcolor=\"%s\"><b>idx:</b> %lu</td>"
                        "<td bgcolor=\"%s\"><b>next:</b> %lu</td>"
                    "</tr>"
                    "<tr>"
                        "<td colspan=\"5\" bgcolor=\"%s\"><b>value:</b>",
                    i,
                    TABLE_OUTLINE,
                    i == 0 ? BG_COLOR : isFree[i] ? FREE_FILL : LIST_CELL,
                    TABLE_OUTLINE,
                    PREV_FILL,    lst->prev[i],
                    INDEX_FILL,   i,
                    NEXT_FILL,    lst->next[i],
                    VALUE_FILL);
        } else {
            fprintf(dot,
                    "node%lu"
                    "[shape=box, style=\"rounded, filled\", color=\"%s\", fillcolor=\"%s\", penwidth=2.1, fontsize=14, label="
                    "<<table border=\"0\" cellborder=\"1\" cellspacing=\"0\" cellpadding=\"4\" color=\"%s\">"
                    "<tr>"
                        "<td bgcolor=\"%s\"><b>idx:</b> %lu</td>"
                        "<td bgcolor=\"%s\"><b>next:</b> %lu</td>"
                    "</tr>"
                    "<tr>"
                        "<td colspan=\"5\" bgcolor=\"%s\"><b>value:</b>",
                    i,
                    TABLE_OUTLINE,
                    i == 0 ? BG_COLOR : isFree[i] ? FREE_FILL : LIST_CELL,
                    TABLE_OUTLINE,
                    INDEX_FILL,   i,
                    NEXT_FILL,    lst->next[i],
                    VALUE_FILL);
        }

        if (i != 0)
          lst->printfFunc(dot, listGet(lst, i));
        else 
          fprintf(dot, "%.*s", (int)lst->itemSize, listGet(lst, i));

        fprintf(dot,
                   "</td>"
                "</tr>"
                "<tr>"
                   "<td colspan=\"5\" bgcolor=\"%s\"><b>addr:</b> %p</td>"
                "</tr>"
                "</table>"
                ">];\n",
                ADDRESS_FILL, listGet(lst, i));
    }

    fprintf(dot, "{rank=same; ");
    for (ListIndex i = 0; i < lst->capacity; ++i)
        fprintf(dot, "node%lu; ", i);
    fprintf(dot, "}\n");

    // invisible arrows
    for (ListIndex i = 0; i + 1 < lst->capacity; i++)
        fprintf(dot, "node%lu -> node%lu [minlen=1, maxlen=1, weight=10, penwidth=0, "
                                         "arrowhead=none, constraint=true]\n",
                     i, i + 1);

    bool* isBroken = (bool*)calloc(lst->capacity, sizeof(bool));
    if (!isBroken) {
        loglnTraced(ERROR, "Failed to allocate isBroken for this graph dump!");
        free(isFree);
        return Fail;
    }

    // main
    for (ListIndex i = 1; i < lst->capacity; i++) {
        if (isFree[i]) {
            if (lst->next[i])
                fprintf(dot, "node%lu -> node%lu [color=\"%s\"]\n", i, lst->next[i],
                        lst->next[i] < lst->capacity && isFree[lst->next[i]] ? FREE_EDGE : BAD_EDGE);
            continue;
        }
        if (lst->isDoubleLinked && !((!lst->prev[i]) == (i == lst->next[0]))) {
            fprintf(dot, "node%lu -> node%lu [color=\"%s\"]\n", i, lst->prev[i], BAD_EDGE);
        }
        if (!lst->next[i] && i == lst->prev[0]) continue;
        if (!lst->isDoubleLinked) {
            fprintf(dot, "node%lu -> node%lu [color=\"%s\"]\n", i, lst->next[i],
                    lst->next[i] < lst->capacity && lst->next[i] && !isFree[lst->next[i]]
                    ? OK_EDGE : BAD_EDGE);
        } else if (lst->next[i] < lst->capacity && i == lst->prev[lst->next[i]]) {
            fprintf(dot, "node%lu -> node%lu [dir=both arrowtail=vee color=\"%s\"]\n", i, lst->next[i], OK_EDGE);
        } else {
            isBroken[i] = 1;
            fprintf(dot, "node%lu -> node%lu [color=\"%s\"]\n", i, lst->next[i], BAD_EDGE);
            if (lst->next[i] < lst->capacity && lst->prev[lst->next[i]])
                fprintf(dot, "node%lu -> node%lu [color=\"%s\"]\n", lst->next[i], lst->prev[lst->next[i]], BAD_EDGE);
        }
    }

    // broken ones
    if (lst->isDoubleLinked) {
        for (ListIndex i = 1; i < lst->capacity; i++) {
            if (lst->prev[i] < lst->capacity && isBroken[lst->prev[i]])
                fprintf(dot, "node%lu -> node%lu [color=\"%s\"]\n", i, lst->prev[i], BAD_EDGE);
        }
    }

    free(isFree);
    free(isBroken);
    isBroken = NULL;
    isFree = NULL;

    fprintf(dot, "free     -> node%lu [color=\"%s\"]\n", lst->free, FREE_EDGE);
    fprintf(dot, "freeTail -> node%lu [color=\"%s\"]\n", lst->freeTail, FREE_EDGE);
    fprintf(dot, "head     -> node%lu [color=\"%s\"]\n", lst->next[0], OK_EDGE);
    fprintf(dot, "tail     -> node%lu [color=\"%s\"]\n", lst->prev[0], OK_EDGE);

    fprintf(dot, "}\n");
    fclose(dot);

    executeDot(f, dotPath);
    return OK;
}

void rootDump(FILE* f, TreeRoot* root, 
              const char* commentary, const char* filename, int line) {
  assert(f);
  assert(filename);
  assert(commentary);

  ++CALL_COUNT;
  logln(INFO, "rootDump (overall dump #%u) started", CALL_COUNT);

  if (rootTextDump(f, root, commentary, filename, line))
    return;

  rootGraphDump(f, root);
  logln(INFO, "rootDump (overall dump #%u) ended", CALL_COUNT);
}

void nodeDump(FILE* f, TreeNode* node, 
              const char* commentary, const char* filename, int line) {
  assert(f);
  assert(filename);
  assert(commentary);

  ++CALL_COUNT;

  logln(INFO, "nodeDump (overall dump #%u) started", CALL_COUNT);

  if (!node) {
    fprintf(f,
            "%s\n"
            "Textual Dump:\n"
            "TreeNode Dump #%u called from %s:%d\n"
            "TreeNode [NULL] {}\n",
            commentary,
            CALL_COUNT, filename, line);
    return;
  }
  fprintf(f,
          "%s\n"
          "TreeNode Dump #%u called from %s:%d\n",
          commentary,
          CALL_COUNT, filename, line);
  char dotPath[DOT_PATH_BUF_SZ] = {};
  if (snTimestampedFilename(dotPath, DOT_PATH_BUF_SZ,
                            ".log/dot-", ".txt", CALL_COUNT)) {
    loglnTraced(ERROR, "Dot file name composition failed for graph dump");
    return;
  }
  FILE* dot = fopen(dotPath, "w");
  if (!dot) {
    loglnTraced(ERROR, "Dot file open failed for this graph dump");
    return;
  }
  DOT_HEADER_INIT(dot);
  populateDot(dot, node);
  fputs("}\n", dot);
  fclose(dot);
  fputs("Graphical Dump:\n", f);
  executeDot(f, dotPath);

  logln(INFO, "nodeDump (overall dump #%u) ended", CALL_COUNT);
}

FILE* openHtmlLogFile(const char* path) {
  if (!path)
    return NULL;

  char name[HTML_PATH_BUF_SZ] = {}; 
  if (snTimestampedFilename(name, HTML_PATH_BUF_SZ,
                            path, ".html", 0))
    return NULL;

  FILE* f = fopen(name, "w");
  if (!f)
    return NULL;

  fprintf(f,
          "<pre><h1>%s</h1>\n",
          name);
  return f;
}

void closeHtmlLogFile(FILE* f) {
  if (f)
    fclose(f);
} 

static Error rootTextDump(FILE* f, TreeRoot* root,
                          const char* commentary, const char* filename, 
                          int line) {
  if (!root) {
    fprintf(f,
            "%s\n"
            "Textual Dump:\n"
            "TreeRoot Dump #%u called from %s:%d\n"
            "TreeRoot [NULL] {}\n",
            commentary,
            CALL_COUNT, filename, line);
    return Fail;
  }

  fprintf(f,
          "%s\n"
          "TreeRoot Dump #%u called from %s:%d\n"
          "Textual Dump:\n"
          "TreeRoot [%p] {\n"
          "\tnodeCount = %lu\n"
          "\t%srootNode = %p\n"
          "}\n",
          commentary,
          CALL_COUNT, filename, line,
          root,
          root->nodeCount,
          WARNING_PREFIX(!root->rootNode), root->rootNode);

  return !root->rootNode ? Fail : OK;
}

static Error rootGraphDump(FILE* f, TreeRoot* root) {
  assert(f);
  assert(root);

  char dotPath[DOT_PATH_BUF_SZ] = {};
  if (snTimestampedFilename(dotPath, DOT_PATH_BUF_SZ,
                            ".log/dot-", ".txt", CALL_COUNT)) {
    loglnTraced(ERROR, "Dot file name composition failed for graph dump");
    return Fail;
  }

  FILE* dot = fopen(dotPath, "w");
  if (!dot) {
    loglnTraced(ERROR, "Dot file open failed for graph dump");
    return Fail;
  }

  DOT_HEADER_INIT(dot);

  fprintf(dot,
          "root [shape=box, style=\"filled\", color=\"%s\", "
          "fillcolor=\"%s\", fontsize=14, label="
          "<<table border=\"0\" cellborder=\"1\" cellspacing=\"0\" cellpadding=\"4\" color=\"%s\">"
          "<tr>"
              "<td bgcolor=\"%s\"><b>ROOT</b></td>"
          "</tr>"
          "<tr>"
          "<td bgcolor=\"%s\"><b>nodeCount:</b> %lu</td>"
          "</tr>"
          "<tr>"
              "<td bgcolor=\"%s\"><b>address:</b> %p</td>"
          "</tr>"
          "<tr>"
              "<td bgcolor=\"%s\"><b>rootNode:</b> %p</td>"
          "</tr>"
          "</table>"
          ">];\n",
          ROOT_OUTLINE, ROOT_FILL,
          TABLE_OUTLINE,
          TABLE_FILL,
          TABLE_FILL, root->nodeCount,
          TABLE_FILL, root,
          TABLE_FILL, root->rootNode);

  populateDot(dot, root->rootNode);

  fprintf(dot, "root -> node%p [color=\"%s\"]\n", root->rootNode, OK_EDGE);
  fputs("}\n", dot);
  fclose(dot);

  fputs("Graphical Dump:\n", f);
  executeDot(f, dotPath);

  return OK;
}

#undef DOT_HEADER_INIT

static void populateDot(FILE* dot, TreeNode* node) {
  assert(dot);
  assert(node);

  declareNode(dot, node, false);
  Queue* queue = NULL;
  declareRank(dot, node, &queue);
}

#define DECLARE_CHILD_NODE(child)                                                 \
   {                                                                              \
   if (child) {                                                                   \
     if (child->parent == node) {                                                 \
       fprintf(dot, "node%p -> node%p [color=\"%s\", arrowtail=vee, dir=both]\n", \
               node, child, OK_EDGE);                                             \
       declareNode(dot, child, false);                                            \
     } else {                                                                     \
       fprintf(dot, "node%p -> node%p [color=\"%s\"]\n", node, child, BAD_EDGE);  \
       declareNode(dot, child, true);                                             \
     }                                                                            \
   }                                                                              \
   }

static void declareNode(FILE* dot, TreeNode* node, bool bondFailed) {
  assert(dot);
  if (!node)
    return;
  
  const char* nodeStr = getNodeTypeStr(node->data.type);
  const char* nodeColor = DEFAULT_CELL;
  switch (node->data.type) {
    case NUM_TYPE:       nodeColor = NUM_CELL;     break;
    case OP_TYPE:        nodeColor = OP_CELL;      break;
    case SYMBOL_TYPE:    nodeColor = SYMBOL_CELL;  break;
    case RAW_IDENT_TYPE: nodeColor = RAW_ID_CELL;  break;
    case CTRL_TYPE:      nodeColor = CTRL_CELL;    break;
    case VAR_TYPE_TYPE:  nodeColor = TYPE_CELL;    break;
    default: logln(ERROR, "Bad node type in declareNode(...)"); break;
  }

#ifdef SIMPLIFIED_NODES
  fprintf(dot,
          "node%p"
          "[shape=box, style=\"rounded, filled\", color=\"%s\", fillcolor=\"%s\", penwidth=2.1, fontsize=14, label="
          "<<table border=\"0\" cellborder=\"1\" cellspacing=\"0\" cellpadding=\"4\" color=\"%s\">"
          "<tr>"
              "<td colspan=\"6\" bgcolor=\"%s\"><b>exceptionCount: </b>%lu</td>"
          "</tr>"
          "<tr>"
              "<td colspan=\"6\" bgcolor=\"%s\"><b>type:</b>%s</td>"
          "</tr>",
          node,
          TABLE_OUTLINE,
          nodeColor,
          TABLE_OUTLINE,
          node->data.exceptionCount ? BAD_FILL : EXC_FILL,
          node->data.exceptionCount,
          nodeStr ? TYPE_FILL     : BAD_FILL,  
          nodeStr ? nodeStr : "ERROR: no info for such NodeType");
#else
  fprintf(dot,
          "node%p"
          "[shape=box, style=\"rounded, filled\", color=\"%s\", fillcolor=\"%s\", penwidth=2.1, fontsize=14, label="
          "<<table border=\"0\" cellborder=\"1\" cellspacing=\"0\" cellpadding=\"4\" color=\"%s\">"
          "<tr>"
              "<td colspan=\"6\" bgcolor=\"%s\"><b>parent:</b> %p</td>"
          "</tr>"
          "<tr>"
              "<td colspan=\"6\" bgcolor=\"%s\"><b>exceptionCount: </b>%lu</td>"
          "</tr>"
          "<tr>"
              "<td colspan=\"6\" bgcolor=\"%s\"><b>type:</b> %s</td>"
          "</tr>",
          node,
          TABLE_OUTLINE,
          nodeColor,
          TABLE_OUTLINE,
          PARENT_FILL,  node->parent,
          node->data.exceptionCount ? BAD_FILL : EXC_FILL,
          node->data.exceptionCount,
          nodeStr ? TYPE_FILL     : BAD_FILL,  
          nodeStr ? nodeStr : "ERROR: no info for such NodeType");
#endif
  
  switch (node->data.type) {
    case OP_TYPE:
      {
        const OpTypeInfo* opInfo = parseOpType(node->data.value.op);
        fprintf(dot,
                "<tr>"
                  "<td colspan=\"6\" bgcolor=\"%s\"><b>value:</b> %s</td>"
                "</tr>",
                opInfo ? VALUE_FILL  : BAD_FILL, 
                opInfo ? opInfo->str : "ERROR: no info for such OpType");
      }
      break;
    case RAW_IDENT_TYPE:
      {
        StringView var = node->data.value.rawId;
        if (var.data) 
          fprintf(dot,
                  "<tr>"
                    "<td colspan=\"6\" bgcolor=\"%s\"><b>value:</b> %.*s</td>"
                  "</tr>",
                  VALUE_FILL,
                  (int)var.size, var.data);
        else
          fprintf(dot,
                  "<tr>"
                    "<td colspan=\"6\" bgcolor=\"%s\"><b>value:</b> %s</td>"
                  "</tr>",
                  BAD_FILL,
                  "NULL");
      }
      break;
    case SYMBOL_TYPE:
      {
        SymbolIndex i = node->data.value.sym;
        fprintf(dot,
                "<tr>"
                 "<td colspan=\"6\" bgcolor=\"%s\"><b>value:</b> bucketIndex = %lu </td>"
                "</tr>"
                "<tr>"
                 "<td colspan=\"6\" bgcolor=\"%s\"><b>value:</b> listIndex   = %lu </td>"
                "</tr>",
                VALUE_FILL, i.bucketIndex, 
                VALUE_FILL, i.listIndex);
      }
      break;
    case NUM_TYPE:
      fprintf(dot,
              "<tr>"
                "<td colspan=\"6\" bgcolor=\"%s\"><b>value:</b> %ld</td>"
              "</tr>",
              VALUE_FILL, node->data.value.num);
      break;
    case CTRL_TYPE:
      {
        const char* str = getCtrlTypeStr(node->data.value.ctrl);
        fprintf(dot,
                "<tr>"
                  "<td colspan=\"6\" bgcolor=\"%s\"><b>value:</b> %s</td>"
                "</tr>",
                str ? VALUE_FILL : BAD_FILL, 
                str ? str : "ERROR: no info for such CtrlType");
      }
      break;
    case VAR_TYPE_TYPE:
      {
        const char* str = getVarTypeStr(node->data.value.varType);
        fprintf(dot,
                "<tr>"
                  "<td colspan=\"6\" bgcolor=\"%s\"><b>value:</b> %s</td>"
                "</tr>",
                str ? VALUE_FILL : BAD_FILL, 
                str ? str : "ERROR: no info for such VarType");
      }
      break;
    case UNKNOWN_TYPE:
    default:
      break;
  }

#ifdef SIMPLIFIED_NODES
  fprintf(dot,
          "</table>"
          ">];\n");
#else
  fprintf(dot,
          "<tr>"
              "<td colspan=\"6\" bgcolor=\"%s\"><b>address:</b> %p</td>"
          "</tr>"
          "<tr>"
              "<td bgcolor=\"%s\"><b>left:</b> %p</td>"
              "<td bgcolor=\"%s\"><b>right:</b> %p</td>"
          "</tr>"
          "</table>"
          ">];\n",
          ADDRESS_FILL, node,
          !IS_OP(node) && !IS_CTRL(node) && node->left
          ? DEFAULT_CELL
          : LEFT_FILL, node->left,
          !IS_OP(node) && !IS_CTRL(node) && node->right
          ? DEFAULT_CELL
          : RIGHT_FILL, node->right);
#endif

  if (node->parent && bondFailed)
    fprintf(dot, "node%p -> node%p [color=\"%s\"]\n", node, node->parent, BAD_EDGE);
  DECLARE_CHILD_NODE(node->left);
  DECLARE_CHILD_NODE(node->right);
}

#undef DECLARE_CHILD_NODE

static void declareRank(FILE* dot, TreeNode* node, Queue** queue) {
  assert(dot);
  assert(node);
  assert(queue);

  fputs("{ rank = same; ", dot);
  Queue* newQueue = NULL;
  TreeNode* cur = node;
  do {
    fprintf(dot, "node%p; ", cur);
    if (cur->left)
      enqueue(&newQueue, cur->left);
    if (cur->right)
      enqueue(&newQueue, cur->right);
  } while (!dequeue(queue, &cur));
  fputs("}\n", dot);
  if (!dequeue(&newQueue, &cur))
    declareRank(dot, cur, &newQueue);
}

static void executeDot(FILE* f, char* dotPath) {
  char cmd[DOT_CMD_BUF_SZ] = {};
  char imgPath[IMG_PATH_BUF_SZ] = {};
  if (snTimestampedFilename(imgPath, IMG_PATH_BUF_SZ,
                            ".log/graph-", 
                            ".svg", CALL_COUNT)) {
    loglnTraced(ERROR, "Image file path composition failed for graph dump!");
    return;
  }
  snprintf(cmd, DOT_CMD_BUF_SZ, "dot -T svg \"%s\" -o \"%s\"", dotPath, imgPath);
  system(cmd);

  fprintf(f, "<img src=\"./%s\"></img>\n", imgPath + strlen(".log/"));
}

#undef WARNING_PREFIX
