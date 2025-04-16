/*
 * 
 * explicit free list ashiglan hiilee.
 * gehdee minii kod 64bit uildliin system deer l ajillah baih daa (haygaa 8B-d hadgalaad yvchihsan hha)
 * 
 * 
 * Free: 
 * | HEADER | PREV      | NEXT      | OPTIONAL PADDING  | FOOTER|
 * allocated: 
 * | HEADER | PAYLOAD               | FOOTER|
 *           pointer ni payload-aa zaadag.
 * 
 * ashiglagdaj bui block ni (header(4B), payload, footer(4B) ) -s burdene => 8B +? => alignment 8 geheer blockin hemjee bagadaa 16
 * chuluutei block ni { header(4B), prev*(8B), next*(8B), footer(4B) } -s burdene => 24B
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "Tesoro",
    /* First member's full name */
    "Bayarjavkhlan Baatarchuluun",
    /* First member's email address */
    "21b1num2718@stud.num.edu.mn",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8
#define HALF_ALIGNMENT 4

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
#define SIZE_ALIGN(size) (((size) + (HALF_ALIGNMENT - 1)) & ~0x3)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
#define NUMBER_SIZE (SIZE_ALIGN(sizeof(size_t)))
// pointer-n hemjee (64 bit / 8B baih daa)
#define P_SIZE (ALIGN(sizeof(void*)))

// block-n hamgiin baga hemjee ni 24B baina.
#define MINBLOCKSIZE 24
// #define WSIZE 4 // int-n hemjee (word)
// #define DSIZE 8 // haygiin hemjee (double word)

// plus one buyu 1-tei or hiine.
#define p1(size) (size | 1)
#define MAX(x, y) ((x) > (y)? (x) : (y))
// ene hediig huulchihloo
#define GET(p)        (*(size_t *)(p))
#define PUT(p, val)   (*(size_t *)(p) = (val))
#define GET_SIZE(p)  (GET(p) & ~0x1)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define HDRP(bp)     ((void *)(bp) - NUMBER_SIZE)
#define FTRP(bp)     ((void *)(bp) + GET_SIZE(HDRP(bp)) - P_SIZE)
// header-g ni ashiglaad footer oloh
#define FTRP_H(p)     ((void *)(p) + GET_SIZE(p) - NUMBER_SIZE)

// block pointer ugugduhud daraagiin/umnuh block-n haygiig olno
#define NEXT_BLKP(hp) ((char *)(hp) + GET_SIZE(hp))
#define PREV_BLKP(hp) ((char *)(hp) - GET_SIZE(hp - NUMBER_SIZE))

#define NEXT_FREE(hp) (*((void **)((char*)(hp)+NUMBER_SIZE+P_SIZE)))
#define PREV_FREE(hp) (*((void **)((char*)(hp)+ NUMBER_SIZE)))

#define PRINT_INFO(hp) (printf("%d. %p, alloc: %d, size = %d, nextF = %p, prevF = %p\n",_count, hp, GET_ALLOC(hp), GET_SIZE(hp), NEXT_FREE(hp), PREV_FREE(hp) ))
// heap-n ehleliig zaana
void* heap_start = NULL;
// hamgiin anhni free block;
void* free_start = NULL;
int _count = -1;

static void* find_free(size_t size);
static void* find_free_split(size_t size);
static void change_prev_free(void* hp, void* prevFree);
static void change_next_free(void* hp, void* nextFree);
static void split(void* hp, size_t size);
static char coalesce(void* hp);
// static void heap_checker();
/* 
 * mm_init - initialize the malloc package.
 * ppt deer baisan shig zaawal ehend bolon hoino ni hil bolgoj 2 block tawih shaardlaga baina uu? => bas 16B ni haa ym be, te
 */
int mm_init(void)
{
    // printf("----------------- init ajillaj baina\n");
    // _count = -1;
    heap_start = NULL;
    free_start = NULL;
    size_t tmp = 0;
    if(NUMBER_SIZE % ALIGNMENT) {
        tmp = ALIGNMENT - NUMBER_SIZE;
    }
    heap_start = mem_sbrk(MINBLOCKSIZE + tmp); // 
    if(heap_start == (void*)(-1)) return -1;

    heap_start = (char*) heap_start + tmp;
    free_start = heap_start;

    PUT(heap_start, MINBLOCKSIZE); // initial header
    change_prev_free(heap_start, NULL); 
    change_next_free(heap_start, NULL);
    PUT((void*)((char*)heap_start + NUMBER_SIZE + P_SIZE + P_SIZE), MINBLOCKSIZE) ; // initial footer 

    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    // printf("-------------%d.malloc: size = %d\n", ++_count, size);

    size_t new_size = MAX(ALIGN(size + NUMBER_SIZE*2 ), MINBLOCKSIZE);
    // chuluutei zai olood, tuuniigee ynzalj baigaa..
    void* p = find_free_split(new_size);
    
    if(p == NULL) { 
    // heap ni tomorch chadahgui bol ingeh uu
        return NULL;
    }

    // printf("mm_malloc: find_free = %p, new_size = %d\n", p, new_size);

    // heap_checker();
    return (void *)((char *)p + NUMBER_SIZE);
}

/*
 * mm_free
 */
void mm_free(void *ptr)

{
    // block-n header, footer-t free block bolohig temdeglej baina.
    //  neeree footer-n hemjeeg uurhcluh ym baina shd
    void* hdr = HDRP(ptr);
    size_t size = GET_SIZE(hdr);
    // printf("-------------%d.mm_free: hdr = %p, size = %d \n", ++_count, hdr, size);
    void* ftr = FTRP_H(hdr);
    PUT(hdr, size);
    PUT(ftr, size);
    change_next_free(hdr, NULL);
    change_prev_free(hdr, NULL);

    if(free_start == NULL) {
        free_start = hdr; 
        // heap_checker(); 
        return;
    }
    if(free_start > hdr) {
        change_next_free(hdr, free_start);

        change_prev_free(free_start, hdr);
        free_start = hdr;
        // heap_checker();
        return;
    }

    if(coalesce(hdr) == 0) {

        // umnuh bolon daraah 2 block ni free bish baisan tul free list-d oruulah heregtei bolno.
        // sanah oin daraallaar free list dotroo oruulchihy
        ptr = free_start;
        // free list dotroo sanah oin daraallaar oruulah 
        while(1) {
            ftr = NEXT_FREE(ptr); // 
            if(ftr == NULL) {
                change_next_free(ptr, hdr);
                change_prev_free(hdr, ptr);
                break; // eswel return
            }
            if(hdr > ptr && hdr < ftr) {
                change_next_free(ptr, hdr);
                change_prev_free(ftr, hdr);
                change_next_free(hdr, ftr);
                change_prev_free(hdr, ptr);
                break;
            }
            ptr = ftr;
        }
    }

    // heap_checker();

    //coalesce -g hiichihwel testelj bolno doo
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = HDRP(ptr);
    // printf("-------------%d.mm_realloc: size = %d, ptr = %p\n",++_count, size, oldptr);
    void *newptr;
    size_t newsize, copySize, diff;

    if(ptr == NULL) return mm_malloc(size); // ptr null baiwal malloc gesen ug

    if(size == 0) {mm_free(ptr); return NULL;}

    copySize = GET_SIZE(oldptr);
    newsize = MAX(ALIGN(size + 2 * NUMBER_SIZE), MINBLOCKSIZE);

    // herwee odoo baigaa hemjee ni shine hemjeenees ih baiwal
    if(newsize <= copySize) {
        // printf("shrink:)\n");
        diff = copySize - newsize;
        if(diff <= MINBLOCKSIZE) {
            return ptr;
        }

        newptr = (void*) ((char*)oldptr + diff);
        PUT(newptr, p1(diff));
        PUT(FTRP_H(newptr), GET(newptr));
        _count--;
        mm_free(newptr);

        PUT(oldptr, newsize);
        PUT(FTRP_H(oldptr), newsize);
        return ptr;

    }

    // odoogiinhoos tom hemjeetei baiwal.
    diff = newsize - copySize;
    if(NEXT_BLKP(oldptr) == NEXT_FREE(oldptr)) {
        newptr = NEXT_FREE(oldptr);
        // yg daraagiin block ni hangalttai chuluutei hemjeetei
        if(GET_SIZE(newptr) >= diff) {
            // printf("expand\n");
            copySize = GET_SIZE(newptr);
            if(copySize - diff >= MINBLOCKSIZE) {
                // split hiih heregtei
                split(newptr, diff) ;
                PUT(oldptr, p1(newsize));
                PUT(HDRP(oldptr), p1(newsize));
                
                return ptr;
            }
            // daraagiin block-g split hiih shaardlagagui
            change_next_free(PREV_FREE(newptr), NEXT_FREE(newptr));
            change_prev_free(NEXT_FREE(newptr), PREV_FREE(newptr));
            PUT(oldptr, newsize);
            PUT(HDRP(oldptr), newsize);
            return ptr;
            
        }
    }

    // zaawal shineer alloc hiih heregtei bolno
    // printf("new mallll\n");
    _count--;
    newptr = mm_malloc(size);
    // printf("re_alloc: oldptr = %p\n", oldptr);

    if (newptr == NULL)
      return NULL;
    memcpy(newptr, ptr, copySize);
    // printf("re_alloc: oldptr = %p\n", oldptr);
    _count--;
    mm_free(ptr);
    return newptr;
}

/**
 * chuluulugduj bui block-g umnuh bolon daraagiin blocktoi ni niiluuleh gej oroldono
 */
static char coalesce(void* hp) {
    // printf("coalesce has been called\n");
    void* prev_b = PREV_BLKP(hp);
    if(hp <= heap_start) prev_b = NULL;
    void* next_b = NEXT_BLKP(hp);
    if(next_b >= mem_heap_hi()) next_b = NULL;
    char flag = 0;

    if(prev_b!=NULL && !GET_ALLOC(prev_b)) {
        // printf("umnuhtei ni niiluulj baina\n");
        // umnuh block ni free
        PUT(prev_b, GET_SIZE(prev_b)+GET_SIZE(hp)); // shuud umnuh block-nh ni hemjeeg uurchluhud bolno
        PUT((FTRP_H(prev_b)), GET_SIZE(prev_b));
        flag=1;
    } else prev_b = hp; // daraagiin if dotor prev_b -g l ashiglah geed ingelee.
    if(next_b!=NULL && !GET_ALLOC(next_b)) {
        // printf("daraagiinhtai ni niiluulj baina\n");
        // daraagiin block ni free
        PUT(prev_b, GET_SIZE(prev_b)+GET_SIZE(next_b)); 
        PUT((FTRP_H(prev_b)), GET_SIZE(prev_b));

        change_next_free(prev_b, NEXT_FREE(next_b)); //next_free-g ni ugaasaa uurchilj taarna.
        change_prev_free(NEXT_FREE(next_b), prev_b);

        if(flag == 0) { // zuwhun daraagiinhtai ni niiluulj baigaa tohioldol umnuh block gej baihgui tul 2uulang ni huuldag.s
            change_prev_free(prev_b, PREV_FREE(next_b));
            // change_prev_free(NEXT_FREE(next_b), prev_b);
            change_next_free(PREV_FREE(next_b), prev_b);
        }
        flag+=2;
    }

    return flag; // ali alintai ni niileegui bol 0-g butsaana
}

/**
 * ugugdsun hemjeeg bagtaah chuluutei block haidag function. 
 * first_fit ashiglasan.
 * 
 */
static void* find_free(size_t size) {
    void* f = free_start; // ene anhni free-n header baigaa.
    void* tmp = NULL;
    // free block baihgui bol urgutgunu
    if(f == NULL) { 

        if ((tmp = mem_sbrk(size)) == (void *)-1)
            return NULL;

        PUT(tmp, p1(size));
        PUT((void*)(FTRP_H(tmp)) , p1(size));
        change_next_free(tmp, NULL);
        change_prev_free(tmp, NULL);

        return tmp;
    }
    while(f) {
        if(GET_SIZE(f) >= size) {
            PUT(f, p1(GET_SIZE(f)));
            PUT(FTRP_H(f), (GET(f)));
            return f;
        }
        // tmp = *((void**)((char*)f + NUMBER_SIZE + P_SIZE)); // next free block
        tmp = NEXT_FREE(f);
        // !!! null mun bilu
        if(tmp == NULL) break; // next baihgui bol break hiine.
        f = tmp;
    }

    if((void*)NEXT_BLKP(f) <= mem_heap_hi()) {
        // printf("find_free: suuliin free block ni dunduur baigaa tul heap ee tomruulj baina\n");
        // heapee size hemjeegeer urgutguh heregtei.
        if ((tmp = mem_sbrk(size)) == (void *)-1)
            return NULL;

        PUT(tmp, p1(size));
        PUT((void*)(FTRP_H(tmp)) , p1(size));
        change_next_free(tmp, NULL);
        change_prev_free(tmp, NULL);

        return tmp;
    }

    // suuld baigaa ni free block buguud hemjee hurehgui bol heap-ee tomruuldag.

    tmp = mem_sbrk(size - GET_SIZE(f));
    // tmp = increase_heap(size);

    PUT(f, p1(size)); // 
    PUT((FTRP_H(f)) , p1(size));
    change_next_free(PREV_FREE(f), NULL);

    if(f == free_start) free_start = NULL; 

    return f;
    
}
/**
 * ugugdsun hemjeeg bagtaah block haij olood tuuniigee huwaah ymda
 */
static void* find_free_split(size_t size) {
    void* hp = find_free(size);
    if(hp == NULL) return NULL; 
    // gue er ni endee header footer ee hiichihwel zugeer ym baina.
    // split hiihguigeer.
    // printf("find_free_split: hp = %p, get_size(hp) = %d, is_allocated = %d\n",hp, GET_SIZE(hp), GET_ALLOC(hp));


    if(GET_SIZE(hp) - size < MINBLOCKSIZE){ 
        if(hp == free_start) {
            free_start = NEXT_FREE(hp);
        }
        change_next_free(PREV_FREE(hp), NEXT_FREE(hp));
        change_prev_free(NEXT_FREE(hp), PREV_FREE(hp));
        return hp; // block-n haygiig yvuulna daa
    } 
    // split hiij huwaaj baina.
    split(hp, size);
    return hp;    
}

/**
 * free block-n next_free pointer-g shinechilne.
 * solih free block
 */
static void change_next_free(void* hp, void* nextFree) {
    // printf("change_next_free: hp = %p, nextFree = %p\n", hp, nextFree);
    if(hp == NULL) return;
    // void *prev = PREV_FREE(hp);
    *((void**)(((char*) (hp) + NUMBER_SIZE+P_SIZE))) = nextFree;
}

/**
 * free block-n prev_free pointer-g shinechilne.
 * hp => solih free block
 */
static void change_prev_free(void* hp, void* prevFree) {
    // printf("change_prev_free: hp = %p, prevFree = %p\n", hp, prevFree);
    if(hp == NULL) return;

    *((void**)(((char*) (hp) + NUMBER_SIZE))) = prevFree;
}

/**
 * ugugdsun free block-g ugugdsun hemjeegeer huwaagaad
 * ali alinih ni prev bolon next free block-g zaaj ugnu.
 */
static void split(void *hp, size_t size) {

    // printf("split is called\n");
    void* new_free = ((char*) hp + size);
    size_t new_size = GET_SIZE(hp) - size;
    if(new_size < MINBLOCKSIZE) return;
    // umnuh free block-d uurchlult
    change_next_free(PREV_FREE(hp), new_free);
    // shine free block-n 2 pointer-g zaaj baina.
    change_prev_free(new_free, PREV_FREE(hp));
    change_next_free(new_free, NEXT_FREE(hp));
    change_prev_free(NEXT_FREE(new_free), new_free);

    if(free_start == hp) {
        free_start = new_free; 
    }

    PUT(hp, p1(size));
    PUT(FTRP_H(hp), p1(size));
    PUT(new_free, (new_size));
    PUT(FTRP_H(new_free), (new_size));    
}

// static void heap_checker() {
//     if(_count  < -3) {
//         exit(0);
    
//     void* st = heap_start;
//     void* tmp = NULL;
//     int count = 0;
//     void* hlast = mem_heap_hi();
//     // printf("****************** heap_chekcer***************\n");
//     while(st != NULL && st <= hlast) {
//         if(!GET_ALLOC(st)) count++;
//         PRINT_INFO(st);
//         if(GET_SIZE(st) < 0) {
//             // printf("size<0 gene u? %p \n", st);
//             exit(0);
//             break;
//         }
//         if(NEXT_BLKP(st) == st) {
//             // printf("ene yu bilee: %p, size = %d\n", st, GET_SIZE(st));
//             exit(0);
//             break;
//         }
//         st = NEXT_BLKP(st);

//     }
//     // printf("there are %d free blocks\n", count);
//     // printf("free start = %p\n", free_start);
//     tmp = free_start;
//     if(count!=0) count --;
//     if(tmp != NULL ) st = NEXT_FREE(tmp);
//     else st = NULL;
//     if(tmp != NULL && GET_ALLOC(tmp)) {
//         // printf("free_start ni allocated baina gene u??\n");
//         exit(0);
//     }

//     while(st != NULL) {
//         if(GET_ALLOC(st)) {
//             // printf("allocated block free list dotor baina : %p\n", st);
//             exit(0);
//         }
//         count--;
//         if(st <= tmp) {
//             // printf("next_free was lesser than the prev_free %d, %p, %p\n", count, st, tmp); 
//             exit(0);
//         }
//         if(PREV_FREE(st) != tmp) {
//             // printf("next_free, prev_free zuruutei baina shd %d\n", count); 
//             exit(0);
//         }
//         tmp = st;
//         st = NEXT_FREE(st);
//     }
//     if(count) {
//         // printf("free block-uud hoorondoo tasaldsan baina count=%d\n", count); exit(0);
//     }
// }

// }




