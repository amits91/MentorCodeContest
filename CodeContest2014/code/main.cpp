#include "sys/time.h"
#include "stdio.h"
#include "stdlib.h"
#include "assert.h"
#include "string.h"
#include <stdint.h>
#include <string>
#include <vector>
#include <list>
//#include <bitset>
#include <boost/dynamic_bitset.hpp>
#include <limits.h>
#include <errno.h>
#include <emmintrin.h> // Define SSE2 intrinsic functions
#include <nmmintrin.h> // Define SSE2 intrinsic functions


using namespace std;
double wtime(void);
//#define PROFILE
#ifdef PROFILE
#define INIT_TIME_SEG initTimeSeg()
#define RECORD_TIME_SEGMENT(x) recordTimeSeg((x))
double initTimeSeg()
{
    static double startTime = 0; 
    double lastStartTime = startTime;
    startTime = wtime();
    return lastStartTime;
}

void recordTimeSeg(const char* msg)
{
    static int cnt = 0;
    printf("#### %10d: Time(%20s): %f\n", cnt++, (msg?msg:""), wtime()-initTimeSeg());
    fflush(stdout);
}
#else
#define INIT_TIME_SEG
#define RECORD_TIME_SEGMENT(x)
#endif

#ifdef DEBUG
#define DEBUG_PRINTF(...) printf( __VA_ARGS__)
#else
#define DEBUG_PRINTF(...) 
#endif


void* CALLOC( size_t sz )
{
    void* a = malloc(sz);
    memset(a, 0, sz);
    return a;
}

#define OTHERS 0
#define SME 1
#define NEW_HIRE 2
#define FIRST_NEW_HIRE 3
#define FIRST_SME 4

#define ALIGND(X) X __attribute__((aligned(8)))
#define PRE_FETCH_T0( x, y )  _mm_prefetch(((char *)(x))+y,_MM_HINT_T0) 
//#define PRE_FETCH_T0( x, y )  
#define PRE_FETCH_NTA( x, y )  _mm_prefetch(((char *)(x))+y,_MM_HINT_NTA) 

#define FILE_BUFF_SZ 4*1024*1024
//#define FILE_BUFF_SZ 64
#define RESET_LBUFF \
    lbuff[0] = '\0';\
    llen     = 0

#define RESET_FILE_BUFF \
    fbuff[0] = '\0';\
    flen     = 0

#define DUMP_TO_DISK \
    fbuff[flen] = '\0'; \
    fwrite(fbuff, sizeof(char), flen*sizeof(char), f); \
    RESET_FILE_BUFF; \
    //RECORD_TIME_SEGMENT("After Dump");
    //fflush(f); \
    //fsync(fileno(f)); \
    //RECORD_TIME_SEGMENT("Before Dump");\


#define LIST_ITERATE( iter, l ) \
    for (std::list<FriendNodeT*>::iterator (iter) = (l)->begin(); \
            (iter) != (l)->end(); \
            ++(iter))
#define ROOT_LIST_ITERATE( iter, l ) \
    for (std::list<RootDataT*>::iterator (iter) = (l)->begin(); \
            (iter) != (l)->end(); \
            ++(iter))

#define SME_BITSOF(x) (*(x)->data->smeBits)
#define CONN_BITS(x) (*(x)->data->connBits)
#define CONN_CHECKED(x) (*(x)->data->connChecked)

#define NH_SM_BITSOF(x) (*(x)->nh_sm_bits)

typedef struct _FriendNodeDataS {
    int localId;
    ALIGND(char idStr[16]);
    uint8_t len;
    char personKind;
    list<struct _FriendNodeS*> *smeConns;
    //list<struct _FriendNodeS*> *firstConns;
    boost::dynamic_bitset<> *smeBits;
    //boost::dynamic_bitset<> *connBits;
    //boost::dynamic_bitset<> *connChecked;
} FriendNodeDataT;

typedef struct _FriendNodeS {
    int id;
    struct _FriendNodeS* root;
    struct _rootDataS* rootData;
    int size;
    FriendNodeDataT* data;
} FriendNodeT;

#define personKindOf(x) (x && x->data ? x->data->personKind : 0)
#define localIdOf(x)    (x && x->data ? x->data->localId : -1)

#ifdef SAFE
#define idStrOf(x) (x && x->data ? x->data->idStr : "")
#define strlenOf(x) (x && x->data ? x->data->len : -1)
#define firstConnsOf(x) (x && x->data ? x->data->firstConns : NULL)
#else
#define idStrOf(x)      (x->data->idStr)
#define strlenOf(x)     (x->data->len)
#define firstConnsOf(x) (x->data->firstConns)
#endif
#define smeConnsOf(x) (x && x->data ? x->data->smeConns : NULL)

#define NODE_IS_SME(x) (personKindOf((x)) == SME)
#define NODE_IS_NEW_HIRE(x) (personKindOf((x)) == NEW_HIRE)
#define NODE_IS_NEW_HIRE_OR_SME(x) (NODE_IS_SME((x)) || NODE_IS_NEW_HIRE((x)))


typedef struct _FriendsS {
    FriendNodeT **peopleArr;
    int arrCnt;
    int noPeople;
    int impPersons[5];
    list<struct _rootDataS*> rootDataList;
    FriendNodeT** sme_arr;
    list<FriendNodeT*> nh_list;
} FriendsT;

typedef struct _rootDataS {
    boost::dynamic_bitset<> *nhBits;
    boost::dynamic_bitset<> *smBits;
    list<pair<FriendNodeT *, FriendNodeT*> >*nhList;
} RootDataT;

static inline RootDataT* create_RootDataT(FriendsT* fr)
{
    RootDataT* r = (RootDataT*)malloc(sizeof(RootDataT));
    r->nhBits = new boost::dynamic_bitset<>(fr->impPersons[NEW_HIRE]);
    r->smBits = new boost::dynamic_bitset<>(fr->impPersons[SME]);
    r->nhList = new list<pair<FriendNodeT*, FriendNodeT*> >;
    fr->rootDataList.push_back(r);
    return r;
}

static inline RootDataT* merge_RootDataT(RootDataT* r1, RootDataT* r2)
{
    if( r1 == NULL ) return r2;
    if( r2 == NULL ) return r1;
    *r1->nhBits |= *r2->nhBits;
    *r1->smBits |= *r2->smBits;
    list<pair<FriendNodeT*, FriendNodeT*> >::iterator it = r1->nhList->end();
    r1->nhList->splice(it, *r2->nhList);
    delete r2->nhBits;
    delete r2->smBits;
    r2->nhBits = NULL;
    delete r2->nhList;
    return r1;
}

static inline pair<FriendNodeT*, FriendNodeT*>* build_fn_arr(FriendsT* fr, RootDataT* r)
{
    pair<FriendNodeT*, FriendNodeT*>* fn_arr = (pair<FriendNodeT*, FriendNodeT*>*)CALLOC(sizeof(pair<FriendNodeT*, FriendNodeT*>)*fr->impPersons[NEW_HIRE]);
    list<pair<FriendNodeT*, FriendNodeT*> >::iterator it;
    for(it = r->nhList->begin(); it != r->nhList->end(); ++it) {
        FriendNodeT* nh = it->first;
        FriendNodeT* fn = it->second;
//        if (fn_arr[localIdOf(nh)].first == NULL)
        fn_arr[localIdOf(nh)] = *it;
    }
    return fn_arr;
}

static inline void update_RootDataT(FriendsT* fr, RootDataT* rd, FriendNodeT* nh, FriendNodeT* fn, FriendNodeT* sm)
{
    if (nh) {
        (*rd->nhBits)[localIdOf(nh)] = 1;
        pair<FriendNodeT*, FriendNodeT*> p;
        p.first = nh;
        p.second = fn;
        rd->nhList->push_back(p);
    }
    if (sm) {
        (*rd->smBits)[localIdOf(sm)] = 1;
    }
}

void print_RootDataT(RootDataT* r)
{
    string buffer;
//    to_string(*r->nhBits, buffer);
    printf("\n NH Bits: %d, %s", r->nhBits->count(), buffer.c_str());
//    to_string(*r->smBits, buffer);
    printf("\n SM Bits: %d, %s", r->smBits->count(), buffer.c_str());
    printf("\n List cnt: %d", r->nhList->size());
    printf("\n");
}


static inline void create_FriendNodeDataT(FriendsT* fr, FriendNodeT* f, char kind)
{
    assert(kind != OTHERS);
    if (f->data == NULL) {
        FriendNodeDataT* d = (FriendNodeDataT*)malloc(sizeof(FriendNodeDataT));
        f->data        = d;
        d->localId     = fr->impPersons[kind]++;
        d->personKind  = kind;
        d->smeConns    = new list<FriendNodeT*>();
        d->smeBits     = NULL;
#if 0
        if (kind != SME) {
            d->connBits = new boost::dynamic_bitset<>(fr->impPersons[SME]);
        }
#endif
        sprintf(f->data->idStr, "%d", f->id);
        f->data->len = strlen(f->data->idStr);
        //fr->impPersonsList[kind].push_back(f);
        if (kind == NEW_HIRE)
            fr->nh_list.push_back(f);
    }
}

static inline FriendNodeT* create_FriendNodeT(FriendsT* fr, int i, char kind)
{
    FriendNodeT* f = NULL;
    f           = (FriendNodeT*)malloc(sizeof(FriendNodeT));
    f->id       = i;
    f->root     = f;
    f->rootData = NULL;
    f->size     = 1;
    f->data     = NULL;
    if (kind > 0) {
        create_FriendNodeDataT(fr, f, kind);
    }
    return f;
}

void print_FriendNodeT( FriendNodeT* f )
{
    const char* kind = NULL;
    switch(personKindOf(f)) {
        case NEW_HIRE       : kind = "NEW_HIRE"; break;
        case SME            : kind = "SME"; break;
        case FIRST_NEW_HIRE : kind = "FIRST_NEW_HIRE"; break;
        case FIRST_SME      : kind = "FIRST_SME"; break;
        default             : kind = "OTHERS"; break;
    }
    printf( "(%d, %s, %d)", f->id, kind, localIdOf(f) );
    fflush(stdout);
}

FriendsT *init_FriendsT( FriendsT* f )
{
    f->peopleArr = (FriendNodeT**)CALLOC(2*sizeof(FriendNodeT));
    f->arrCnt    = 2;
    f->noPeople  = 0;
    f->sme_arr   = NULL;
    //f->nh_sm_bits = NULL;
    memset(f->impPersons,0,sizeof(f->impPersons));
}

void printPeople(FriendsT* f)
{
    printf("\nArray size: %d", f->arrCnt);
    printf("\nTotal Persons: %d\n", f->noPeople);
    /*
    for (int i = 0; i < f->arrCnt; i++) {
        if (f->peopleArr[i] != NULL) {
            printf("  Person: " );
            print_FriendNodeT(f->peopleArr[i]);
            printf("\n" );
        }
    }
    */
    int cnt = 0;
    ROOT_LIST_ITERATE(it, &f->rootDataList) {
        RootDataT* r = *it;
        if (r->nhBits == NULL) continue;
        if (r->nhBits->any() && r->smBits->any()) {
            print_RootDataT(r);
            if (r->nhBits->count() != r->nhList->size()) {
                pair<FriendNodeT*, FriendNodeT*>* fn_arr = build_fn_arr(f, r);
                for (int j = 0; j < f->impPersons[NEW_HIRE]; j++) {
                    pair<FriendNodeT*, FriendNodeT*> p = fn_arr[j];
                    FriendNodeT* nh = p.first;
                    FriendNodeT* fn = p.second;
                    if (fn) {
                        for (int k = 0; k < f->impPersons[SME]; k++) {
                            if (((*r->smBits)[k] == 1) ) {
                                if ( nh->data->smeBits && (nh->data->smeBits->test(k) == false)) {
                                    SME_BITSOF(nh)[k] = 1;
                                } else {
                                    nh->data->smeBits = new boost::dynamic_bitset<>(f->impPersons[SME]);
                                    SME_BITSOF(nh)[k] = 1;
                                }
                            }
                        }
                    }

                }
            }
            cnt++;
        }
    }
    printf("\nTotal Roots List: %d", f->rootDataList.size());
    printf("\nTotal Roots: %d\n", cnt);
}
static inline void printOneConn(FriendsT* fr,
        FriendNodeT* nh, FriendNodeT* fn, FriendNodeT* sm,
        char* fbuff, int* flenp, FILE* fl ) __attribute__((always_inline));

#define RESET_FILE_BUFF_NEW \
    fbuff[0] = '\0';\
    *flenp   = 0
#define DUMP_TO_DISK_NEW \
    fbuff[*flenp] = '\0'; \
    fwrite(fbuff, sizeof(char), (*flenp)*sizeof(char), fl); \
    RESET_FILE_BUFF_NEW;
    //fflush(f); \
    //fsync(fileno(f)); \


void dumpConns(FriendsT* f) 
{
    FriendNodeT* nh = NULL;
    ALIGND(char  fbuff[FILE_BUFF_SZ]);
    int   flen;
    FILE *fl    = fopen("output", "wb");
    int* flenp = &flen;
    RESET_FILE_BUFF;
#if 1
    LIST_ITERATE(nh_iter, &f->nh_list) {
        nh = *nh_iter;
        LIST_ITERATE(jt, smeConnsOf(nh)) {
            FriendNodeT* sme = *jt;
            int k = localIdOf(sme);
            if ( nh->data->smeBits && (nh->data->smeBits->test(k) == false)) {
                SME_BITSOF(nh)[k] = 1;
                printOneConn(f, nh, NULL, sme, fbuff, &flen, fl);
            } else if (nh->data->smeBits == NULL) {
                nh->data->smeBits = new boost::dynamic_bitset<>(f->impPersons[SME]);
                SME_BITSOF(nh)[k] = 1;
                printOneConn(f, nh, NULL, sme, fbuff, &flen, fl);
            }
        }
    }
#endif
    ROOT_LIST_ITERATE(it, &f->rootDataList) {
        RootDataT* r = *it;
        if (r->nhBits == NULL) continue;
        if (r->nhBits->any() && r->smBits->any()) {
//            print_RootDataT(r);
            if (r->nhBits->count() != r->nhList->size()) {
                pair<FriendNodeT*, FriendNodeT*>* fn_arr = build_fn_arr(f, r);
                for (int j = 0; j < f->impPersons[NEW_HIRE]; j++) {
                    pair<FriendNodeT*, FriendNodeT*> p = fn_arr[j];
                    nh = p.first;
                    FriendNodeT* fn = p.second;
                    if (fn) {
                        for (int k = 0; k < f->impPersons[SME]; k++) {
                            if ((r->smBits->test(k)) ) {
                                FriendNodeT* sme = f->sme_arr[k];
                                if ( nh->data->smeBits && (nh->data->smeBits->test(k) == false)) {
                                    SME_BITSOF(nh)[k] = 1;
                                    printOneConn(f, nh, fn, sme, fbuff, &flen, fl);
                                } else if (nh->data->smeBits == NULL) {
                                    nh->data->smeBits = new boost::dynamic_bitset<>(f->impPersons[SME]);
                                    SME_BITSOF(nh)[k] = 1;
                                    printOneConn(f, nh, fn, sme, fbuff, &flen, fl);
                                }
                            }
                        }
                    }

                }
                free(fn_arr);
            } else {
                list<pair<FriendNodeT*, FriendNodeT*> >::iterator it;
                for(it = r->nhList->begin(); it != r->nhList->end(); ++it) {
                    FriendNodeT* nh = it->first;
                    FriendNodeT* fn = it->second;
                    for (int k = 0; k < f->impPersons[SME]; k++) {
                        if (r->smBits->test(k)) {
                            FriendNodeT* sme = f->sme_arr[k];
                            if ( nh->data->smeBits && (nh->data->smeBits->test(k) == false)) {
                                SME_BITSOF(nh)[k] = 1;
                                printOneConn(f, nh, fn, sme, fbuff, &flen, fl);
                            } else if (nh->data->smeBits == NULL) {
                                nh->data->smeBits = new boost::dynamic_bitset<>(f->impPersons[SME]);
                                SME_BITSOF(nh)[k] = 1;
                                printOneConn(f, nh, fn, sme, fbuff, &flen, fl);
                            }
                        }
                    }
                }
            }
        }
    }
    if (flen > 0) {
        DUMP_TO_DISK_NEW;
    }
    fclose(fl);
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
        f->peopleArr[id] = create_FriendNodeT(f, id, type);
        f->noPeople++;
    }
    return f->peopleArr[id];
}


FriendNodeT* findRoot( FriendsT* fr, FriendNodeT* i)
{
    FriendNodeT* root = i;

    while (root->id != root->root->id) {
        root = root->root;
    }
    // Path compression
    FriendNodeT* p = i;
    while (p != root) {
        FriendNodeT* n = p->root;
        p->root = root;
        p = n;
    }
    return root;
}

//static int conn_cnt = 0;
bool connected(FriendsT* fr, FriendNodeT* p, FriendNodeT* q)
{
    //conn_cnt++;
    return findRoot(fr, p) == findRoot(fr, q);
}

void create_conn(FriendsT* fr, FriendNodeT* p, FriendNodeT* q)
{
    FriendNodeT* rootP = findRoot(fr, p);
    FriendNodeT* rootQ = findRoot(fr, q);
    if (rootP == rootQ) return;
    // make smaller root point to larger one to keep it balanced
    if (rootP->size < rootQ->size) {
        rootP->root = rootQ;
        rootQ->size += rootP->size; 
        rootQ->rootData = merge_RootDataT(rootQ->rootData, rootP->rootData);
    } else {
        rootQ->root = rootP; rootP->size += rootQ->size; 
        rootP->rootData = merge_RootDataT(rootP->rootData, rootQ->rootData);
    }
}

void update_terminal_conn(FriendsT* fr, FriendNodeT* lf, FriendNodeT* rf)
{
    if (NODE_IS_NEW_HIRE_OR_SME(lf)) {
        //firstConnsOf(lf)->push_back(rf);
        if (NODE_IS_NEW_HIRE(lf) && !NODE_IS_NEW_HIRE_OR_SME(rf)) {
            create_FriendNodeDataT(fr, rf, FIRST_NEW_HIRE);
            FriendNodeT* root = findRoot(fr, rf);
            if (root->rootData == NULL) {
                root->rootData = create_RootDataT(fr);
            }
            update_RootDataT(fr, root->rootData, lf, rf, NULL);
        } else if (NODE_IS_SME(lf) && !NODE_IS_NEW_HIRE_OR_SME(rf)) {
            create_FriendNodeDataT(fr, rf, FIRST_SME);
            FriendNodeT* root = findRoot(fr, rf);
            if (root->rootData == NULL) {
                root->rootData = create_RootDataT(fr);
            }
            update_RootDataT(fr, root->rootData, NULL, NULL, lf);
        }
    } 
    if (NODE_IS_NEW_HIRE_OR_SME(rf)) {
        //firstConnsOf(rf)->push_back(lf);
        if (NODE_IS_NEW_HIRE(rf) && !NODE_IS_NEW_HIRE_OR_SME(lf)) {
            create_FriendNodeDataT(fr, lf, FIRST_NEW_HIRE);
            FriendNodeT* root = findRoot(fr, lf);
            if (root->rootData == NULL) {
                root->rootData = create_RootDataT(fr);
            }
            update_RootDataT(fr, root->rootData, rf, lf, NULL);
        } else if (NODE_IS_SME(rf) && !NODE_IS_NEW_HIRE_OR_SME(lf)) {
            create_FriendNodeDataT(fr, lf, FIRST_SME);
            FriendNodeT* root = findRoot(fr, lf);
            if (root->rootData == NULL) {
                root->rootData = create_RootDataT(fr);
            }
            update_RootDataT(fr, root->rootData, NULL, NULL, rf);
        }
    }
    if (NODE_IS_SME(lf)) {
        fr->sme_arr[localIdOf(lf)] = lf;
    }
    if (NODE_IS_SME(rf)) {
        fr->sme_arr[localIdOf(rf)] = rf;
    }
    if (NODE_IS_NEW_HIRE(lf) && NODE_IS_SME(rf)) {
        smeConnsOf(lf)->push_back(rf);
    } else if (NODE_IS_SME(lf) && NODE_IS_NEW_HIRE(rf)) {
        smeConnsOf(rf)->push_back(lf);
    }
}

void parseLine(char* buf, FriendsT* fr, int type, bool isgraph, int lines)
{
    char* endPtr = NULL;
    char* str = buf;
    PRE_FETCH_T0( buf, 128);
    if (isgraph == false) {
        int id = 0;
        errno = 0;
        id = strtol(str, &endPtr, 10);
        if (errno == 0) {
            FriendNodeT* p = addPerson(fr, id, type);
        }
    } else {
        int l = -1;
        int r = -1;
        if (lines == 1) return;
        //if (strstr(buf, "}")) return;
        if (str[0] == '}') return;
        errno = 0;
        l = strtol(str, &endPtr, 10);
        str = endPtr + 2;
        errno = 0;
        r = strtol(str, &endPtr, 10);
        if (errno == 0) {
            FriendNodeT* lf = addPerson( fr, l, 0 );
            FriendNodeT* rf = addPerson( fr, r, 0 );
            // Terminal Nodes
            if (NODE_IS_NEW_HIRE_OR_SME(lf) || NODE_IS_NEW_HIRE_OR_SME(rf)) {
                update_terminal_conn(fr, lf, rf);
            } else {
                // Internal Nodes
                create_conn(fr, lf, rf);
            }
        }
    }
}

char* readEntireFile( FILE* f, long* size )
{
    long lSize;
    char * buffer;
    size_t result;

    // obtain file size:
    fseek (f , 0 , SEEK_END);
    lSize = ftell (f);
    rewind (f);
    // allocate memory to contain the whole file:
    buffer = (char*) _mm_malloc(sizeof(char)*lSize, sizeof(char));

    // copy the file into the buffer:
    result = fread (buffer,1,lSize,f);
    if (result != lSize) {fputs ("Reading error",stderr); exit (3);}
    *size = lSize;

    return buffer;
}

void parseFile(char* fileName, FriendsT* fr, int type, bool isgraph)
{
    FILE* file;
    char* buf;
    int lines =0;
    char* allFileContent = NULL;
    char* nextLine = NULL;
    long size = 0;

    file = fopen(fileName, "r");
    assert(file);
    allFileContent = readEntireFile(file, &size);
    fclose(file);
    RECORD_TIME_SEGMENT("File Read");

    nextLine  = allFileContent;
    buf       = nextLine;
    if (buf) {
        nextLine  = strchr(buf,'\n');
        if (nextLine) {
            *nextLine = '\0';
            nextLine++;
        }
    }
    while (buf) {
        if (*buf!='\0') {
            lines++;
            parseLine(buf, fr, type, isgraph, lines);
        }
        buf = nextLine;
        if (buf) {
            nextLine  = strchr(buf,'\n');
            if (nextLine) {
                *nextLine = '\0';
                nextLine++;
            }
        }
    }
    RECORD_TIME_SEGMENT("File Parse");
    _mm_free(allFileContent);
}

// SSE2 optimized memcpy()
void *MemCpySse2(void *__restrict b, const void *__restrict a, size_t n)
{
    char *s1 = (char*)b;
    const char *s2 = (const char*)a;
    for(; 0<n; --n)*s1++ = *s2++;
    return b;
}

//#define MEMCPY(x,y,z) MemCpySse2( x, y, z)
#define MEMCPY(x,y,z) memcpy( x, y, z)

static inline void printOneConn(FriendsT* fr,
        FriendNodeT* nh, FriendNodeT* fn, FriendNodeT* sm,
        char* fbuff, int* flenp, FILE* fl )
{
    int len = 0;
    char* lstr = fbuff + *flenp;
    PRE_FETCH_T0( lstr, 128);
    if (fn) {
        PRE_FETCH_T0( idStrOf(nh), 16);
        PRE_FETCH_T0( idStrOf(fn), 16);
        PRE_FETCH_T0( idStrOf(sm), 16);
        len = strlenOf((nh)); MEMCPY(lstr, idStrOf(nh), len); lstr += len; (*flenp) += len;
        *lstr = '-'; lstr++; (*flenp)++;
        *lstr = '-'; lstr++; (*flenp)++;
        len = strlenOf((fn)); MEMCPY(lstr, idStrOf(fn), len); lstr += len; (*flenp) += len;
        *lstr = '-'; lstr++; (*flenp)++;
        *lstr = '-'; lstr++; (*flenp)++;
        len = strlenOf((sm)); MEMCPY(lstr, idStrOf(sm), len); lstr += len; (*flenp) += len;
    } else {
        PRE_FETCH_T0( idStrOf(nh), 16);
        PRE_FETCH_T0( idStrOf(sm), 16);
        len = strlenOf((nh)); MEMCPY(lstr, idStrOf(nh), len); lstr += len; (*flenp) += len;
        *lstr = '-'; lstr++; (*flenp)++;
        *lstr = '-'; lstr++; (*flenp)++;
        len = strlenOf((sm)); MEMCPY(lstr, idStrOf(sm), len); lstr += len; (*flenp) += len;
    }
    *lstr = '\n'; lstr++; (*flenp)++;
    *lstr = '\0';
    if ((*flenp) > (FILE_BUFF_SZ - 128 )) {
        DUMP_TO_DISK_NEW;
    }
}

#if 0
bool isSMEConnected(FriendsT* fr, FriendNodeT* node, FriendNodeT* sme)
{
    if (NODE_IS_SME(sme) && node->data->connBits) {
        if (CONN_BITS(node)[localIdOf(sme)] == 1)
            return true;
    } 
    return false;
}
bool endNodesConnected(FriendsT* fr, FriendNodeT* start, FriendNodeT* end, FriendNodeT** first)
{
    assert (personKindOf(start) != personKindOf(end));
    if (isSMEConnected(fr, start, end)) {
        *first = NULL;
        return true;
    }
    LIST_ITERATE(it, firstConnsOf(start)) {
        FriendNodeT* nh = *it;
#ifdef DEBUG
        static FriendNodeT* pn = start;
        static int cnt = 0;
        if ( pn == start) {
            cnt++;
        } else {
            pn = start;
            cnt = 1;
        }
        static FriendNodeT* cn = nh;
        static int cc = 0;
        if ( cn == nh) {
            cc++;
        } else {
            cn = nh;
            cc = 1;
        }
//        DEBUG_PRINTF("\n==%10d.%d) %10d:%10d:%10d", cnt, cc, start->id, nh->id, end->id);
#endif
        if (nh->data->connChecked && CONN_CHECKED(nh)[localIdOf(end)] == 1) {
            if (CONN_BITS(nh)[localIdOf(end)] == 1) {
                *first = nh;
                DEBUG_PRINTF("\n==%10d.%d) %10d:%10d:%10d", cnt, cc, start->id, nh->id, end->id);
                DEBUG_PRINTF(" True");
                return true;
            } else {
                DEBUG_PRINTF("\n==%10d.%d) %10d:%10d:%10d", cnt, cc, start->id, nh->id, end->id);
                DEBUG_PRINTF(" False");
                return false;
            }

        } else {
            // Check connections between first nodes of newHier and sme
            LIST_ITERATE(jt, firstConnsOf(end)) {
                FriendNodeT* sm = *jt;
                // Skip connections which are newhiers or smes
                if (NODE_IS_NEW_HIRE_OR_SME(nh) || NODE_IS_NEW_HIRE_OR_SME(sm)) {
                    continue;
                } else if ((sm == nh) || connected(fr, nh, sm)) {
                    *first = nh;
//                    DEBUG_PRINTF(" TRUE");
                    update_sme_on_first_sme(fr, nh, end, 1);
                    return true;
                }
            }
            update_sme_on_first_sme(fr, nh, end, 0);
        }
    }
//    DEBUG_PRINTF(" FALSE");
    return false;
}

// Collecting a buffer and then writing it in the end
void printConnections(FriendsT* fr)
{
    char  lbuff[128];
    char  fbuff[FILE_BUFF_SZ];
    int   flen;
    int   llen;
    FILE *f    = fopen("output", "wb");
    assert (f);
    RESET_FILE_BUFF;
    RESET_LBUFF;
//#pragma omp parallel private(lbuff, llen)
    LIST_ITERATE(newHier, &fr->impPersonsList[NEW_HIRE]) {
//#pragma omp parallel private(lbuff)
        LIST_ITERATE(sme, &fr->impPersonsList[SME]) {
            FriendNodeT* nh = *newHier;
            FriendNodeT* sm = *sme;
            FriendNodeT* fn = NULL;
            if (endNodesConnected(fr, nh, sm, &fn)) {
                char* lstr = lbuff;
                int len = 0;
                RESET_LBUFF;
                if (fn) {
                    len = strlen(idStrOf(nh)); memcpy(lstr, idStrOf(nh), len); lstr += len; llen += len;
                    *lstr = '-'; lstr++; llen++;
                    *lstr = '-'; lstr++; llen++;
                    len = strlen(idStrOf(fn)); memcpy(lstr, idStrOf(fn), len); lstr += len; llen += len;
                    *lstr = '-'; lstr++; llen++;
                    *lstr = '-'; lstr++; llen++;
                    len = strlen(idStrOf(sm)); memcpy(lstr, idStrOf(sm), len); lstr += len; llen += len;
                } else {
                    len = strlen(idStrOf(nh)); memcpy(lstr, idStrOf(nh), len); lstr += len; llen += len;
                    *lstr = '-'; lstr++; llen++;
                    *lstr = '-'; lstr++; llen++;
                    len = strlen(idStrOf(sm)); memcpy(lstr, idStrOf(sm), len); lstr += len; llen += len;
                }
                *lstr = '\n'; lstr++; llen++;
                *lstr = '\0';
                //#pragma omp critical
                if ((flen + llen) < FILE_BUFF_SZ) {
                    memcpy(fbuff + flen, lbuff, llen);
                    flen += llen;
                } else {
                    DUMP_TO_DISK;
                    memcpy(fbuff + flen, lbuff, llen);
                    flen += llen;
                }
            }
        }
        DEBUG_PRINTF("\n");
        RECORD_TIME_SEGMENT("After 1 New Hire");
    }
    if (flen > 0) {
        DUMP_TO_DISK;
    }
    fclose(f);
}


void test_bitsets(FriendsT* f)
{
    int si = 0;
    int ni = 0;
    int fs = 0;
    int fn = 0;
    boost::dynamic_bitset<> sme_bits(f->impPersons[SME]);
    boost::dynamic_bitset<> nh_bits (f->impPersons[NEW_HIRE]);
    boost::dynamic_bitset<> fs_bits (f->impPersons[FIRST_SME]);
    boost::dynamic_bitset<> fn_bits (f->impPersons[FIRST_NEW_HIRE]);
    string buffer;
    to_string(sme_bits, buffer);
    printf("\n SME Bits: %s\n", buffer.c_str());
    LIST_ITERATE( nt, &f->impPersonsList[NEW_HIRE]) {
        nh_bits[ni++] = 1;
        si = 0;
        LIST_ITERATE( st, &f->impPersonsList[SME]) {
            sme_bits[si++] = 1;
        }
    }
    assert( ni == f->impPersons[NEW_HIRE]);
    assert( si == f->impPersons[SME]);
    to_string(sme_bits, buffer);
    printf("\n SME Bits: %s\n", buffer.c_str());
    LIST_ITERATE( nt, &f->impPersonsList[FIRST_NEW_HIRE]) {
        fn_bits[fn++] = 1;
        fs = 0;
        LIST_ITERATE( st, &f->impPersonsList[FIRST_SME]) {
            fs_bits[fs++] = 1;
        }
    }
    assert( fn == f->impPersons[FIRST_NEW_HIRE]);
    assert( fs == f->impPersons[FIRST_SME]);
    RECORD_TIME_SEGMENT("Only Traversal");

}
#endif


int myMain(int argc, char* argv[])
{
    // Start your code here
    FriendsT f = {0};
    init_FriendsT(&f);
    parseFile(argv[2], &f, NEW_HIRE, false);
    RECORD_TIME_SEGMENT("Parse New Hire");
    parseFile(argv[3], &f, SME, false);
    RECORD_TIME_SEGMENT("Parse SME");
    f.sme_arr    = (FriendNodeT**) CALLOC(sizeof(FriendNodeT*)*f.impPersons[SME]);
    //f.nh_sm_bits = new boost::dynamic_bitset<>(f.impPersons[NEW_HIRE]);
    parseFile(argv[1], &f, OTHERS, true);
    RECORD_TIME_SEGMENT("Parse Input Graph");
#ifdef DEBUG
    DEBUG_PRINTF("\n");
    DEBUG_PRINTF("Total NEW_HIRE       : %d\n", f.impPersons[NEW_HIRE]);
    DEBUG_PRINTF("Total SME            : %d\n", f.impPersons[SME]);
    DEBUG_PRINTF("Total FIRST_NEW_HIRE : %d\n", f.impPersons[FIRST_NEW_HIRE]);
    DEBUG_PRINTF("Total FIRST_SME      : %d\n", f.impPersons[FIRST_SME]);
    DEBUG_PRINTF("Total OTHERS         : %d\n", f.impPersons[OTHERS]);
    printPeople(&f);
    RECORD_TIME_SEGMENT("Print Info");
#endif
//    test_bitsets(&f);
    dumpConns(&f);
    if (argc == 4) {
//        printConnections(&f);
        //printf("\n No. of connected(): %d\n", conn_cnt);
        //printf("\n No. of compressed(): %d\n", cnt_cn);
    }
    RECORD_TIME_SEGMENT("printConnections");
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
    INIT_TIME_SEG;
    startTime = wtime();
    return lastStartTime;
}

void recordTime()
{
    printf("#### Recorded Time:%f\n",wtime()-initTime());
    fflush(stdout);
}

int main(int argc, char* argv[])
{
    initTime();
    myMain(argc, argv);
    recordTime();
    return 0;
}
/*******************************************************************/
