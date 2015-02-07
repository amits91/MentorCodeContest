#include "sys/time.h"
#include "stdio.h"
#include "stdlib.h"
#include "assert.h"
#include "string.h"
#include <string>
#include <vector>
#include <list>
#include <limits.h>
#include <errno.h>


using namespace std;

void* CALLOC( size_t sz )
{
    void* a = malloc(sz);
    memset(a, 0, sz);
    return a;
}

typedef struct _FriendNodeS {
    int id;
    char idStr[64];
    char personKind;
    int rootId;
    int size;
    list<struct _FriendNodeS*> *firstConns;
} FriendNodeT;

FriendNodeT* create_FriendNodeT(int i, char kind)
{
    FriendNodeT* f = NULL;
    f             = (FriendNodeT*)malloc(sizeof(FriendNodeT));
    f->id         = i;
    f->personKind = kind;
    f->rootId     = i;
    f->size       = 1;
    f->firstConns = NULL;
    f->idStr[0]   = '\0';
    if (kind > 0) {
        f->firstConns = new list<FriendNodeT*>();
        sprintf(f->idStr, "%d", i);
    }
    return f;
}

void print_FriendNodeT( FriendNodeT* f )
{
    const char* kind = NULL;
    switch( f->personKind ) {
        case 2  : kind = "New Hier"; break;
        case 1  : kind = "SME"; break;
        default : kind = ""; break;
    }
    printf( "(%d, %s)", f->id, kind );
}


#define ARR_SIZE 10000;

typedef struct _FriendsS {
    FriendNodeT **peopleArr;
    int arrCnt;
    int noPeople;
    list<FriendNodeT*> newHierList;
    list<FriendNodeT*> smeList;
} FriendsT;

FriendsT *init_FriendsT( FriendsT* f )
{
    f->peopleArr = (FriendNodeT**)CALLOC(2*sizeof(FriendNodeT));
    f->arrCnt    = 2;
    f->noPeople  = 0;
}

void printPeople(FriendsT* f)
{
    printf("\nArray size: %d", f->arrCnt);
    printf("\nTotal Persons: %d", f->noPeople);
    for (int i = 0; i < f->arrCnt; i++) {
        if (f->peopleArr[i] != NULL) {
            printf("  Person: " );
            print_FriendNodeT(f->peopleArr[i]);
            printf("\n" );
        }
    }
}

void resize_arr( FriendsT* f, int capacity)
{
    FriendNodeT** temp;
    int i;

    temp = (FriendNodeT**)CALLOC(capacity*sizeof(FriendNodeT));
    for (i=0; i < f->arrCnt; i++) {
        temp[i] = f->peopleArr[i];
    }
    free(f->peopleArr);
    f->peopleArr = temp;
    f->arrCnt    = capacity;
}

int get_new_size(int id, int curr)
{
    int newsize = curr;
    while (id>(newsize-1)) {
        newsize *= 2;
    }
    return newsize;
}

FriendNodeT* addPerson( FriendsT* f, int id, char type)
{
    if (id > (f->arrCnt - 1)) {
        resize_arr(f, get_new_size(id, f->arrCnt));
    }
    if (f->peopleArr[id] == NULL) {
        f->peopleArr[id] = create_FriendNodeT(id, type);
        f->noPeople++;
    }
    return f->peopleArr[id];
}


FriendNodeT* findRoot( FriendsT* fr, int id)
{
    FriendNodeT* root = fr->peopleArr[id];

    while (root->id != root->rootId) {
        root = fr->peopleArr[root->rootId];
    }
    // Path compression
    FriendNodeT* p = fr->peopleArr[id];
    while (p != root) {
        FriendNodeT* n = fr->peopleArr[p->rootId];
        p->rootId = root->id;
        p = n;
    }
    return root;
}

bool connected(FriendsT* fr, int p, int q)
{
    return findRoot(fr, p) == findRoot(fr, q);
}

void create_conn(FriendsT* fr, int p, int q) 
{
    FriendNodeT* rootP = findRoot(fr, p);
    FriendNodeT* rootQ = findRoot(fr, q);
    if (rootP == rootQ) return;
    // make smaller root point to larger one to keep it balanced
    if (rootP->size < rootQ->size) {
        rootP->rootId = rootQ->id; rootQ->size += rootP->size; 
    } else {
        rootQ->rootId = rootP->id; rootP->size += rootQ->size; 
    }
}

#define BUFF_SIZE 1024

void parseLine(char* buf, FriendsT* fr, int type, bool isgraph, int lines)
{
    char* endPtr = NULL;
    char* str = buf;
    if (isgraph == false) {
        int id = 0;
        //if (sscanf( buf, "%d", &id ) == 1) 
        errno = 0;
        id = strtol(str, &endPtr, 10);
        if (errno == 0) {
            FriendNodeT* p = addPerson(fr, id, type);
            if (type == 1) {
                fr->smeList.push_back(p);
            } else if (type == 2) {
                fr->newHierList.push_back(p);
            }
        }
    } else {
        int l = -1;
        int r = -1;
        if (lines == 1) return;
        //if (strstr(buf, "}")) return;
        if (str[0] == '}') return;
        errno = 0;
        l = strtol(str, &endPtr, 10);
        //if (sscanf(buf, "%d--%d", &l, &r) == 2)
        str = endPtr + 2;
        errno = 0;
        r = strtol(str, &endPtr, 10);
        if (errno == 0) {
            FriendNodeT* lf = addPerson( fr, l, 0 );
            FriendNodeT* rf = addPerson( fr, r, 0 );
            if (lf->personKind > 0 || rf->personKind > 0) {
                if (lf->personKind > 0) {
                    lf->firstConns->push_back(rf);
                    sprintf(rf->idStr, "%d", rf->id);
                } 
                if (rf->personKind > 0) {
                    rf->firstConns->push_back(lf);
                    sprintf(lf->idStr, "%d", lf->id);
                }
            } else {
                create_conn(fr, l, r);
            }
        }
    }
}
void parseFile(char* fileName, FriendsT* fr, int type, bool isgraph)
{
    FILE* file;
    char buf[BUFF_SIZE];
    int lines =0;

    file = fopen(fileName, "r");
    assert(file);
    while (fscanf(file, "%s\n", &buf) == 1) {
        lines++;
        parseLine(buf, fr, type, isgraph, lines);
    }
    fclose(file);
}

bool endNodesConnected(FriendsT* fr, FriendNodeT* start, FriendNodeT* end, FriendNodeT** first)
{
    assert (start->personKind != end->personKind);
    for (std::list<FriendNodeT*>::iterator it = start->firstConns->begin(); it != start->firstConns->end(); ++it) {
        FriendNodeT* nh = *it;
        if (nh == end) {
            *first = NULL;
            return true;
        }
    }
    for (std::list<FriendNodeT*>::iterator it = start->firstConns->begin(); it != start->firstConns->end(); ++it) {
        FriendNodeT* nh = *it;
        // Check connections between first nodes of newHier and sme
        for (std::list<FriendNodeT*>::iterator jt = end->firstConns->begin(); jt != end->firstConns->end(); ++jt) {
            FriendNodeT* sm = *jt;
            // Skip connections which are newhiers or smes
            if (nh->personKind > 0 || sm->personKind > 0) {
                continue;
            } else if (connected(fr, nh->id, sm->id)) {
                *first = nh;
                return true;
            }
        }
    }
    return false;
}

// Collecting a buffer and then writing it in the end
void printConnections(FriendsT* fr)
{
    string  str;
    char    buf[128];
    FILE   *f         = fopen("output", "w");
    assert (f);
//#pragma omp parallel private(str, buf)
    for (std::list<FriendNodeT*>::iterator newHier = fr->newHierList.begin(); newHier != fr->newHierList.end(); ++newHier) {
//#pragma omp parallel private(buf)
        for (std::list<FriendNodeT*>::iterator sme = fr->smeList.begin(); sme != fr->smeList.end(); ++sme) {
            FriendNodeT* nh = *newHier;
            FriendNodeT* sm = *sme;
            FriendNodeT* fn = NULL;
            if (endNodesConnected(fr, nh, sm, &fn)) {
                char* lstr = buf;
                int len = 0;
                if (fn) {
                    len = strlen(nh->idStr); memcpy(lstr, nh->idStr, len); lstr += len;
                    *lstr = '-'; lstr++;
                    *lstr = '-'; lstr++;
                    len = strlen(fn->idStr); memcpy(lstr, fn->idStr, len); lstr += len;
                    *lstr = '-'; lstr++;
                    *lstr = '-'; lstr++;
                    len = strlen(sm->idStr); memcpy(lstr, sm->idStr, len); lstr += len;
                } else {
                    len = strlen(nh->idStr); memcpy(lstr, nh->idStr, len); lstr += len;
                    *lstr = '-'; lstr++;
                    *lstr = '-'; lstr++;
                    len = strlen(sm->idStr); memcpy(lstr, sm->idStr, len); lstr += len;
                }
                *lstr = '\n'; lstr++;
                *lstr = '\0';
//#pragma omp critical
                str += buf;
                buf[0] = '\0';
            }
        }
//#pragma omp critical
        fprintf(f, "%s", str.c_str());
        str.clear();
        buf[0] = '\0';
    }
    fclose(f);
}


int myMain(int argc, char* argv[])
{
  // Start your code here
  FriendsT f = {0};
  init_FriendsT(&f);
  parseFile(argv[2], &f, 2, false);
  parseFile(argv[3], &f, 1, false);
  parseFile(argv[1], &f, 0, true);
//  printPeople(&f);
  printConnections(&f);
}


/******************************************************************
 * Do Not Modify This Section
 ******************************************************************/
double wtime(void)
{
  double sec;
  struct timeval tv;
  gettimeofday(&tv,NULL);
  sec = tv.tv_sec + tv.tv_usec/1000000.0;
  return sec;
}

double initTime()
{
  static double startTime = 0; 
  double lastStartTime = startTime;
  startTime = wtime();
  return lastStartTime;
}

void recordTime()
{
  printf("#### Recorded Time:%f\n",wtime()-initTime());
}

int main(int argc, char* argv[])
{
  initTime();
  myMain(argc, argv);
  recordTime();
  return 0;
}
/*******************************************************************/
