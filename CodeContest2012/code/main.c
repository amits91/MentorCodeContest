#include<stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <string.h>
//#include <omp.h>
//#include <xmmintrin.h>
#include <emmintrin.h> // Define SSE2 intrinsic functions
#include <nmmintrin.h> // Define SSE2 intrinsic functions
//#define DEBUG

static double wtime(void)
{
  double sec;
  struct timeval tv;

  gettimeofday(&tv,NULL);
  sec = tv.tv_sec + tv.tv_usec/1000000.0;
  return sec;
}

// call wtime twice: before and after your program executes
static int my_main();

static void increase_stack_size()
{
    const rlim_t kStackSize = 64L * 1024L * 1024L;   // min stack size = 64
    struct rlimit rl;
    int result;

    result = getrlimit(RLIMIT_STACK, &rl);
    if (result == 0)
    {
        if (rl.rlim_cur < kStackSize)
        {
            rl.rlim_cur = kStackSize;
            result = setrlimit(RLIMIT_STACK, &rl);
            if (result != 0) {
                fprintf(stderr, "setrlimit returned result = %d\n", result);
            }
        }
    }

}

int main() {

    double time = wtime();
    increase_stack_size();
    my_main();
    time = wtime() - time;
    // report the above time to 5 decimal places while submitting
    printf("\n Total Time: %.5f \n", time );
    return 0;
}

#define CACHE_SZ 4096
#define BIG_SZ 8196
#define SMALL_SZ 16
#define FSMALL "small_picture"
#define FBIG "big_picture"
#define PIXEL_TYPE unsigned char
#define MATCH_THRESHOLD (128)
#define MATCH_ARR_SIZE (BIG_SZ/SMALL_SZ*BIG_SZ/SMALL_SZ)
#define BIG_LOOP_SZ ( BIG_SZ - SMALL_SZ )

//#define ALIGND(X) X __attribute__((aligned(16)))
#define ALIGND(X) X __attribute__((aligned(8)))



//PIXEL_TYPE small_picture[SMALL_SZ][SMALL_SZ] __attribute__((aligned(16)))= {0};
//PIXEL_TYPE big_picture  [BIG_SZ]  [BIG_SZ]   __attribute__((aligned(16)))  = {0};

// Not much benefit by __attribute__ as compiler might already be aligning it
ALIGND( PIXEL_TYPE small_picture[SMALL_SZ][SMALL_SZ] );
ALIGND( PIXEL_TYPE big_picture  [BIG_SZ]  [BIG_SZ]   );

typedef struct _cacheBlock_t {
    unsigned int start_row;
    unsigned int start_col;
    PIXEL_TYPE big_picture[ CACHE_SZ ][ CACHE_SZ ];
    PIXEL_TYPE boundary_col[ CACHE_SZ ][ SMALL_SZ*2 ];
    PIXEL_TYPE boundary_row[ CACHE_SZ ][ SMALL_SZ*2 ];
} cacheBlockT, *cacheBlockP;

#define IDX_CACHE_BLOCK(x) (x)/CACHE_SZ
#define IDX_CACHE(x) (x)%CACHE_SZ
cacheBlockT cache_arr [ BIG_SZ/CACHE_SZ ] [ BIG_SZ/CACHE_SZ ] = {0};


static void file_read()
{
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
#define BUF_SZ 4*BIG_SZ
    char buf[BUF_SZ];
    FILE* pic_file = NULL;
    {
        int row = 0;
        int col = 0;
        pic_file = fopen( FBIG, "r" );
        if( !pic_file )
            printf("\n** Error: Couldn't open file %s", FBIG );

        //while ((read = getline(&line, &len, pic_file)) != -1) {
        while (fgets(buf, BUF_SZ - 1, pic_file) != NULL) {
            int last_num = 0;
            int i = 0;
            while( (buf[i] != '\0') ) {
                if( isdigit( buf[i] ) ) {
                    last_num = last_num*10 + buf[i] - '0';
                } else {
                    big_picture[row][col] = (PIXEL_TYPE)last_num;
                    cache_arr[IDX_CACHE_BLOCK(row)][ IDX_CACHE_BLOCK(col)].big_picture[ IDX_CACHE(row)][IDX_CACHE(col)] = (PIXEL_TYPE)last_num;
                    col++;
                    last_num = 0;
                }
                i++;
            }
            row++;
            col = 0;
        }
        fclose(pic_file);
    }
    {
        int row = 0;
        int col = 0;
        pic_file = fopen( FSMALL, "r" );
        if( !pic_file )
            printf("\n** Error: Couldn't open file %s", FSMALL );

        //while ((read = getline(&line, &len, pic_file)) != -1) {
        while (fgets(buf, BUF_SZ - 1, pic_file) != NULL) {
            int last_num = 0;
            int i = 0;
            //for(i=0; i < len; i++ ) {
            while( buf[i] != '\0' ) {
                if( isdigit( buf[i] ) ) {
                    last_num = last_num*10 + buf[i] - '0';
                } else {
                    small_picture[row][col++] = (PIXEL_TYPE)last_num;
                    last_num = 0;
                }
            }
            row++;
            col = 0;
        }
        fclose(pic_file);
    }
    if (line)
        free(line);
    //#ifdef DEBUG
#if 1
    /* Test File IO */
    {
        int row, col;
        pic_file = fopen ( "bk.small", "w" );
        for( row = 0; row < SMALL_SZ; row++ ) {
            for( col = 0; col < SMALL_SZ; col++ ) {
                fprintf( pic_file, "%d ", small_picture[row][col] );
            }
            fprintf( pic_file, "\n" );
        }
        fclose(pic_file);
        pic_file = fopen ( "bk.big", "w" );
        for( row = 0; row < BIG_SZ; row++ ) {
            for( col = 0; col < BIG_SZ; col++ ) {
                fprintf( pic_file, "%d ", big_picture[row][col] );
            }
            fprintf( pic_file, "\n" );
        }
        fclose(pic_file);
    }
#endif
}

#if 0
static inline int match_pixel( PIXEL_TYPE big_pixel, PIXEL_TYPE small_pixel )
{
    //using 0.8f instead of 0.8 causes performance improvements as typecasting
    //will be done to float rather than default double, which is faster.
    //return ( (small_pixel > 0.8f*big_pixel) && ( small_pixel < 1.2f*big_pixel) ? 1 : 0);

    /* This is faster than above mechanism. Casting to unsigned char is faster
     * than casting to unsigned int.
     */
    //return ( (unsigned char)(small_pixel - 0.8f*big_pixel) < (unsigned char)( 1.2f*big_pixel - 0.8f*big_pixel) ? 1 : 0);
    /* This is slightly more faster */
    return ( (unsigned char)(small_pixel - 0.8f*big_pixel) < (unsigned char)( 1.2f*big_pixel - 0.8f*big_pixel));
    // This is slightly slower than above 16.37 vs 15.72 above need to check
    // why ??
    //return ( (unsigned char)(small_pixel - 0.8f*big_pixel) < (unsigned char)( 0.4f*big_pixel));
}
static int match_block( int row, int col )
{
    int srow;
    int scol;
    unsigned int cnt = 0;
    //#pragma omp parallel for collapse(2) private(srow,scol) reduction( +:cnt)
    for( srow=0; srow < SMALL_SZ; srow++ ) {
        for( scol=0; scol < SMALL_SZ; scol++ ) {
            if( match_pixel(big_picture[row + srow][col+scol], small_picture[srow][scol] ) ) {
                cnt++;
            }
            //#pragma omp critical
            if( cnt > MATCH_THRESHOLD ) {
                //#pragma omp single // This is making this program stuck
                //printf("\nMATCH: %d.%d", row, col );
#if 0
                set_matched_arr_idx( row, col );
#endif
                return 1;
            }
        }
    }
    return 0;
}
#endif
#if 1
// Function to load integer vector from array
static inline __m128i LoadVector( PIXEL_TYPE const * p)
{
    //return _mm_load_si128((__m128i const*)p);// Aligned
    return _mm_loadu_si128((__m128i const*)p);
}
static inline __m128i get_mult( __m128i b, const float f )
{
     __m128i b1;
     __m128i b2;
     __m128i b11;
     __m128i b12;
     __m128i b21;
     __m128i b22;
     __m128i bp8;
     __m128i bp12;
     __m128  fb11;
     __m128  fb12;
     __m128  fb21;
     __m128  fb22;
     __m128  p8;
     __m128  p4;
     __m128  mul11;
     __m128  mul12;
     __m128  mul21;
     __m128  mul22;
     //divide 16
     //  8,8
     b1   = _mm_unpacklo_epi8(b, _mm_set1_epi8(0) );
     b2   = _mm_unpackhi_epi8(b, _mm_set1_epi8(0) );
     //  4,4,4,4
     b11  = _mm_unpacklo_epi16(b1, _mm_set1_epi16(0));
     b12  = _mm_unpackhi_epi16(b1, _mm_set1_epi16(0));
     b21  = _mm_unpacklo_epi16(b2, _mm_set1_epi16(0));
     b22  = _mm_unpackhi_epi16(b2, _mm_set1_epi16(0));

     // Convert four ints to four floats
     fb11 = _mm_cvtepi32_ps(b11);
     fb12 = _mm_cvtepi32_ps(b12);
     fb21 = _mm_cvtepi32_ps(b21);
     fb22 = _mm_cvtepi32_ps(b22);
     p8 = _mm_set1_ps( f );

     mul11 = _mm_mul_ps( fb11, p8 );
     mul12 = _mm_mul_ps( fb12, p8 );
     mul21 = _mm_mul_ps( fb21, p8 );
     mul22 = _mm_mul_ps( fb22, p8 );

     b11 = _mm_cvtps_epi32(mul11);
     b12 = _mm_cvtps_epi32(mul12);
     b21 = _mm_cvtps_epi32(mul21);
     b22 = _mm_cvtps_epi32(mul22);

     b1 = _mm_packs_epi32( b11, b12 );
     b2 = _mm_packs_epi32( b21, b22 );

     bp8 = _mm_packus_epi16( b1, b2 );
     return bp8;
}
// Vectorized matching
#if 1
static inline int match_pixel_vec( __m128i b, __m128i s )
{

    int match_cnt = 0;
    __m128i b0p8 = get_mult( b, 0.8f );
    __m128i b1p2 = get_mult( b, 1.2f );
    __m128i result1 = _mm_cmpgt_epi8(s, b0p8);
    __m128i result2 = _mm_cmplt_epi8(s, b1p2);
    __m128i mask = _mm_and_si128(result1, result2);
    int mask2 = _mm_movemask_epi8(mask);
    match_cnt = _mm_popcnt_u32(mask2);
    return match_cnt;
}
#else
    //return ( (unsigned char)(small_pixel - 0.8f*big_pixel) < (unsigned char)( 1.2f*big_pixel - 0.8f*big_pixel));
    //Testing for impact on performance
static inline int match_pixel_vec( __m128i b, __m128i s )
{

    int match_cnt = 0;
    //__m128i b0p8 = get_mult( b, 0.8f );
    //__m128i b1p2 = get_mult( b, 1.2f );
    //__m128i result2 = _mm_cmpeq_epi8(_mm_sub_epi8(s,b0p8), _mm_sub_epi8(b1p2, b0p8) );
    //__m128i result2 = _mm_cmpeq_epi8( s,  b );
    int mask2 = _mm_movemask_epi8(result2);
    match_cnt = _mm_popcnt_u32(mask2);
    return match_cnt;
}
#endif

#define PRE_FETCH_T0( x, y )  _mm_prefetch(((char *)(x))+y,_MM_HINT_T0) 
#define PRE_FETCH_NTA( x, y )  _mm_prefetch(((char *)(x))+y,_MM_HINT_NTA) 
#define FETCH_DIST 16

static int match_block_vec( int row, int col )
{
    int srow;
    unsigned int cnt = 0;
    //PRE_FETCH_NTA( small_picture, SMALL_SZ*SMALL_SZ );
    //for( srow=0; srow < SMALL_SZ; srow++ ) {
        //PRE_FETCH_T0( &big_picture[ row+srow][col], SMALL_SZ );
    //}
    PRE_FETCH_T0( &big_picture[ row][col], SMALL_SZ );
    //#pragma omp parallel for collapse(2) private(srow,scol) reduction( +:cnt)
    for( srow=0; srow < SMALL_SZ; srow++ ) {
        __m128i big   = LoadVector( &big_picture[row+srow][col] );
        __m128i small = LoadVector( &small_picture[srow][0] );
        PRE_FETCH_T0( &big_picture[ row+srow+1][col], SMALL_SZ );
        //PRE_FETCH_NTA( small_picture, SMALL_SZ );


        cnt += match_pixel_vec( big, small );
#if 0
        //#pragma omp critical
        if( cnt > MATCH_THRESHOLD ) {
            //#pragma omp single // This is making this program stuck
            printf("\nMATCH: %d.%d", row, col );
            return 1;
        }
#endif
    }
    if( cnt > MATCH_THRESHOLD ) {
        //#pragma omp single // This is making this program stuck
        printf("\n(%d.%d)", row, col );
    }
    return 0;
}
#endif

#if 1
#define TILE_SIZE 2048
static void match()
{
    int row1;
    int col1;
    #pragma omp parallel for collapse(2) private(row1, col1) num_threads(12)
    //#pragma omp parallel for collapse(2) private(row1, col1)
    for( row1=0; row1 < BIG_LOOP_SZ; row1+= TILE_SIZE ) {
        for( col1=0; col1 < BIG_LOOP_SZ; col1+= TILE_SIZE ) {
            int row;
            int col;
            //#pragma omp parallel for collapse(2) private(row, col )
            for( row = row1; row < row1 + TILE_SIZE; row++ ) {
                for( col = col1; col < col1 + TILE_SIZE; col++ ) {
                    match_block_vec( row, col );
                }
            }
        }
    }
}
#else
static void match()
{
    int row;
    int col;
#pragma omp parallel for collapse(2) private(row, col) num_threads(8)
    for( row=0; row < BIG_LOOP_SZ; row++ ) {
        for( col=0; col < BIG_LOOP_SZ; col++ ) {
            //Commenting this restricting, This matching is causing slowdown
            //as its restricting parallelization
            //if( !is_pixel_in_matched_range( row, col )) {
            match_block_vec( row, col );
                //}
        }
    }
}
#endif
static int my_main()
{
    file_read();
    //match();
    return 0;
}

