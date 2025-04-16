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

// heap-n ehleliig zaana
void* heap_start = NULL;
// hamgiin anhni free block;
void* free_start = NULL;
int _count = 0;

static void* find_free(size_t size);
static void* find_free_split(size_t size);
static void change_prev_free(void* hp, void* prevFree);
static void change_next_free(void* hp, void* nextFree);
static void split(void* hp, size_t size);
static char coalesce(void* hp);
static void heap_checker();
/* 
 * mm_init - initialize the malloc package.
 * ppt deer baisan shig zaawal ehend bolon hoino ni hil bolgoj 2 block tawih shaardlaga baina uu? => bas 16B ni haa ym be, te
 */
int mm_init(void)
{
    size_t tmp = 0;
    if(NUMBER_SIZE % ALIGNMENT) {
        tmp = ALIGNMENT - NUMBER_SIZE;
    }
    heap_start = mem_sbrk(MINBLOCKSIZE + tmp); // 
    if(heap_start == (void*)(-1)) return -1;

    heap_start = (char*) heap_start + tmp;
    free_start = heap_start;

    PUT(heap_start, MINBLOCKSIZE); // initial header
    // *((void **)((char *)heap_start + NUMBER_SIZE)) = NULL; // prev pointer
    // *((void **)((char *)heap_start + NUMBER_SIZE + P_SIZE)) = NULL; // next pointer
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
    // printf("--------------------------------\n");
    // printf("%d.malloc\n", _count++);
    if(size == 0) return NULL;
    size_t new_size = MAX(ALIGN(size + NUMBER_SIZE*2 ), MINBLOCKSIZE);
    // chuluutei zai olood, tuuniigee ynzalj baigaa..
    void* p = find_free_split(new_size);
    

    if(p == NULL) { 
    // heap ni tomorch chadahgui bol ingeh uu
        return NULL;
    }

    // printf("mm_malloc: find_free = %p, size = %d\n", p, size);

    return (void *)((char *)p + NUMBER_SIZE);
}

/*
 * mm_free
 */
void mm_free(void *ptr)

{
    // block-n header, footer-t free block bolohig temdeglej baina.
    //  neeree footer-n hemjeeg uurhcluh ym baina shd
    // printf("----------------mm_free has been called\n");
    // printf("%d.free\n", _count++);
    void* hdr = HDRP(ptr);
    // printf("mm_free: block to free = %p\n", hdr);
    size_t size = GET_SIZE(hdr);
    void* ftr = FTRP_H(hdr);
    PUT(hdr, size);
    PUT(ftr, size);
    change_next_free(hdr, NULL);
    change_prev_free(hdr, NULL);

    if(free_start == NULL) {free_start = hdr; return;}
    if(free_start > hdr) {
        change_next_free(hdr, free_start);
        change_prev_free(NEXT_FREE(free_start), hdr);
        free_start = hdr;
        return;
    }

    if(coalesce(hdr) == 0) {

        // printf("mm_free: say free hiisen block ni hajuu 2toigoo niileegui\n");

        // umnuh bolon daraah 2 block ni free bish baisan tul free list-d oruulah heregtei bolno.
        // sanah oin daraallaar free list dotroo oruulchihy
        if(free_start == NULL) {free_start = hdr; return;}
        ptr = free_start;
        // free list dotroo sanah oin daraallaar oruulah 
        while(1) {
            ftr = NEXT_FREE(ptr); // 
            // printf("mm_free: ftr = %p\n", ftr);
            if(ftr == NULL) {
                change_next_free(ptr, hdr);
                break; // eswel return
            }
            if(ftr > ptr) {
                change_next_free(ptr, hdr);
                change_prev_free(ftr, hdr);
                break;
            }
            ptr = ftr;
        }
    }


    //coalesce -g hiichihwel testelj bolno doo
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    printf("mm_realloc: size = %d\n", size);
    void *oldptr = HDRP(ptr);
    void *newptr;
    size_t newsize, copySize, diff;

    if(ptr == NULL) return mm_malloc(size); // ptr null baiwal malloc gesen ug

    if(size == 0) {mm_free(ptr); return NULL;}

    copySize = GET_SIZE(oldptr);
    newsize = MAX(ALIGN(size + 2 * NUMBER_SIZE), MINBLOCKSIZE);

    // herwee odoo baigaa hemjee ni shine hemjeenees ih baiwal
    if(newsize <= copySize) {
        printf("shrink:)\n");
        diff = copySize - newsize;
        if(diff <= MINBLOCKSIZE) {
            printf("%d\n", _count++);
            return ptr;
        }

        newptr = (void*) ((char*)oldptr + diff);
        PUT(newptr, p1(diff));
        PUT(FTRP_H(newptr), GET(newptr));

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
            printf("expand\n");
            printf("%d\n", _count++);
            copySize = GET_SIZE(newptr);
            if(copySize - diff >= MINBLOCKSIZE) {
                // split hiih heregtei
                split(newptr, diff) ;
                PUT(oldptr, newsize);
                PUT(HDRP(oldptr), newsize);
                
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
    
    printf("new mallll\n");
    _count--;
    newptr = mm_malloc(size);

    if (newptr == NULL)
      return NULL;
    memcpy(newptr, ptr, copySize);
    mm_free(oldptr);
    return newptr;
}

/**
 * chuluulugduj bui block-g umnuh bolon daraagiin blocktoi ni niiluuleh gej oroldono
 */
static char coalesce(void* hp) {
    void* prev_b = PREV_BLKP(hp);
    if(hp <= heap_start) prev_b = NULL;
    void* next_b = NEXT_BLKP(hp);
    if(next_b >= mem_heap_hi()) next_b = NULL;
    char flag = 0;
    // printf("coalesce: prevb = %p, nextb = %p\n", prev_b, next_b);

    if(prev_b!=NULL && !GET_ALLOC(prev_b)) {
        // umnuh block ni free
        PUT(prev_b, GET_SIZE(prev_b)+GET_SIZE(hp)); // shuud umnuh block-nh ni hemjeeg uurchluhud bolno
        PUT((FTRP_H(prev_b)), GET_SIZE(prev_b));
        flag=1;
    } else prev_b = hp; // daraagiin if dotor prev_b -g l ashiglah geed ingelee.
    if(next_b!=NULL && !GET_ALLOC(next_b)) {
        // daraagiin block ni free
        PUT(prev_b, GET_SIZE(prev_b)+GET_SIZE(next_b)); 
        PUT((FTRP_H(prev_b)), GET_SIZE(prev_b));

        change_next_free(prev_b, NEXT_FREE(next_b)); //next_free-g ni ugaasaa uurchilj taarna.

        if(flag == 0) { // zuwhun daraagiinhtai ni niiluulj baigaa tohioldol umnuh block gej baihgui tul 2uulang ni huuldag.s
            change_prev_free(prev_b, PREV_FREE(next_b));
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
    // printf("find_free: free_start = %p\n", f);
    // free block baihgui bol urgutgunu
    if(f == NULL) { 
        // printf("find_free: free list is empty\n");

        if ((tmp = mem_sbrk(size)) == (void *)-1)
            return NULL;

        // printf("find_free: tmp after increase_heap = %p\n", tmp);

        // PUT(tmp, p1(size));
        // PUT((void*)(FTRP_H(tmp)) , p1(size));

        return tmp;
    }
    while(f) {
        // printf("hello worlddddd size = %d\n", GET_SIZE(f));
        if(GET_SIZE(f) >= size) return f;
        // tmp = *((void**)((char*)f + NUMBER_SIZE + P_SIZE)); // next free block
        tmp = NEXT_FREE(f);
        // printf("find_free: tmp = next_free(f) = %p, size = %d\n", tmp, 5);
        // !!! null mun bilu
        if(tmp == NULL) break; // next baihgui bol break hiine.
        f = tmp;
    }
    // printf("testing\n");

    if((void*)NEXT_BLKP(f) <= mem_heap_hi()) {
        // heapee size hemjeegeer urgutguh heregtei.
        if ((tmp = mem_sbrk(size)) == (void *)-1)
            return NULL;

        // PUT(tmp, p1(size));
        // PUT((void*)(FTRP_H(tmp)) , p1(size));
        return tmp;
    }

    // suuld baigaa free block-n hemjee hurehgui bol 
    // heap-ee tomruuldag.

    tmp = mem_sbrk(size - GET_SIZE(f));
    // tmp = increase_heap(size);

    // PUT(tmp, (size));
    // PUT(FTRP_H(tmp), (size));


    PUT(f, (size)); // 
    PUT((FTRP_H(f)) , (size));

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
    if(GET_SIZE(hp) - size < MINBLOCKSIZE){ 
        if(hp == free_start) {
            free_start = NEXT_FREE(hp);
        }
        PUT(hp, p1(GET_SIZE(hp))); // header
        // *((size_t*)hp) = *((size_t*)hp)|1;
        PUT(FTRP_H(hp), p1(GET_SIZE(hp)));
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
    if(!hp) return;
    // void *prev = PREV_FREE(hp);
    *((void**)(((char*) (hp) + NUMBER_SIZE+P_SIZE))) = nextFree;
}

/**
 * free block-n prev_free pointer-g shinechilne.
 * hp => solih free block
 */
static void change_prev_free(void* hp, void* prevFree) {
    if(!hp) return;

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

    // umnuh free block-d uurchlult
    change_next_free(PREV_FREE(hp), new_free);
    // shine free block-n 2 pointer-g zaaj baina.
    change_prev_free(new_free, PREV_FREE(hp));
    change_next_free(new_free, NEXT_FREE(hp));
    if(free_start == hp) {
        free_start = new_free; 
    }

    PUT(hp, p1(size));
    PUT(FTRP_H(hp), p1(size));
    PUT(new_free, (new_size));
    PUT(FTRP_H(new_free), (new_size));
}

static void heap_checker() {
    void* st = heap_start;
    void* tmp = NULL;
    int count = 0;
    void* hlast = mem_heap_hi();
    printf("****************** heap_chekcer***************\n");
    // while(st != NULL && st < hlast) {
    //     if(!GET_ALLOC(st)) count++;
    //     printf("(%p , %d, %d, nextF = %p)\n", st,  GET_ALLOC(st), GET_SIZE(st), NEXT_FREE(st));

    //     st = NEXT_BLKP(st);
    // }
    printf("there are %d free blocks\n", count);
    printf("free start = %p\n", free_start);
    tmp = free_start;
    count --;
    if(tmp != NULL ) st = NEXT_FREE(tmp);
    else st = NULL;

    // while(st != NULL) {
    //     count--;
    //     if(PREV_FREE(tmp) != tmp) {printf("next_free, prev_free zuruutei baina shd\n");}
    //     st = NEXT_FREE(st);
    // }
    if(count) printf("free block-uud hoorondoo tasaldsan baina\n");

}




