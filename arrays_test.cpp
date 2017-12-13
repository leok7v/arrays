#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>    
#include <errno.h>
#include <string.h>
#include <time.h>
#include <typeinfo>
#include <vector>
#include <algorithm>

#ifdef __MACH__    
#   include <mach/mach.h>
#   include <mach/mach_time.h>
#   include <unistd.h>    
#elif _MSC_VER
#   define NOMINMAX
#   include <Windows.h>
#endif

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#if _MSC_VER
#define breakpoint() __debugbreak() // intrinsic
#else
#include <signal.h>
#define breakpoint() raise(SIGINT)
#endif

#define null ((void*)0)
#define countof(a) (sizeof((a))/sizeof(*(a)))

#define assertion(e, ...) if (!(e)) { printf("%s:%d assertion failed: %s ", __FILE__, __LINE__, #e); printf(__VA_ARGS__); printf("\n"); breakpoint(); }
#define assert(e) assertion(e, "");

#undef ARRAYS_IMPLEMENT_INTERFACE
#define array_alloc(n) malloc(n)
#define array_realloc(p, n) realloc(p, n)
#define array_free(p) free(p)
#if defined(DEBUG) || defined(_DEBUG)
#   define array_assertion(e, ...) assertion(e, __VA_ARGS__);
#else
#   define array_assertion(e, ...)
#endif

#include "arrays.hpp"

static double time_in_milliseconds() {
#ifdef __MACH__    
    uint64_t time = mach_absolute_time();
    static mach_timebase_info_data_t timebase;
    if (timebase.denom == 0 ) {
        mach_timebase_info(&timebase);
    }
    uint64_t nano = time * timebase.numer / timebase.denom;
    return nano / 1000000.0;
#elif _MSC_VER    
    static LARGE_INTEGER frequency;
    if (frequency.QuadPart == 0) {
        QueryPerformanceFrequency(&frequency);
    }
    LARGE_INTEGER pq;
    QueryPerformanceCounter(&pq);
    return pq.QuadPart * 1000.0 / frequency.QuadPart;
#else    
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    return spec.tv_sec * 1000 + spec.tv_nsec / 1000000.0;
#endif    
}

#ifdef __cplusplus

/* "The collective noun for a group of hedgehogs is array."
    https://en.wikipedia.org/wiki/Hedgehog
   "But it doesn't come up much, since hedgehogs are solitary creatures..." 
    http://mentalfloss.com/article/56004/16-fun-facts-about-hedgehogs
*/

typedef struct hedgehog_s { int tag; int spines; } hedgehog_t;

template<typename t, typename e>
static void test_array(t& a, int n, bool expandable) {
    for (int i = 0; i < n; i++) { assert(a.add(i)); assert(a[i] == i); }
    assert(a.size() == n);
    assert(a.index_of(n) == -1);
    for (int i = 0; i < n; i++) { assert(a.index_of(i) == i); }
    if (!expandable) {
        assert(!a.add(n));
        assert(a.size() == n);
    }
    while (a.size() > 0) { int k = a.size(); a.remove_at(k / 2); assert(a.size() == k - 1); }
    for (int i = 0; i < n; i++) { assert(a.add(i)); }
    while (a.size() > 0) { int k = a.size(); a.remove(a[k / 2]); assert(a.size() == k - 1); }
    for (int i = 0; i < n; i++) { assert(a.add(i)); }
    assertion(a.size() == n, "a.size()=%d n=%d", a.size(), n);
    e* p = a.cast();
    for (int i = 0; i < a.size(); i++) { assert(p[i] == a[i]); }    
    if (expandable) {
        assertion(a.size() == n, "a.size()=%d n=%d", a.size(), n);
        for (int i = 0; i < n; i++) { assert(a.add(i + n)); }
        assertion(a.size() == n * 2, "a.size()=%d n * 2 = %d", a.size(), n * 2);
        e* p2 = a.cast();
        for (int i = 0; i < a.size(); i++) { assert(p2[i] == a[i]); }    
    }
    a.clear();
    assert(a.size() == 0);
    printf("done %s\n", typeid(t).name());
}

void cpp_test() {
    {
        const int n = 32;
        array<int, n> s;
        heap_array<int> v;
        nc_array<int, n> a;
        test_array< array<int, n>, int>(s, n, false);
        test_array< heap_array<int>, int>(v, n, true);
        test_array< nc_array<int, n>, int>(a, n, true);
    }
    return;
}

enum {
    VECTORS  = 300, // 300
    ELEMENTS = 700, // 700
};

static int sum0;
static int sum1;
static int sum2;

static void test_std_vector_time(bool reserve) {
    double min_time = 99999;
    for (int j = 0; j < 20; j++) {
        double time = time_in_milliseconds();
        for (int cycle = 0; cycle < 100; cycle++) {
            std::vector<hedgehog_t> vectors[VECTORS];
            sum0 = 0;
            for (int i = 0; i < countof(vectors); i++) {
                if (reserve) { vectors[i].reserve(ELEMENTS); }
                for (int j = 0; j < ELEMENTS; j++) {
                    hedgehog_t hog = {j, j * j};
                    vectors[i].push_back(hog);
                    sum0 += vectors[i][j].spines;
                }        
            }
        }            
        min_time = std::min(time, time_in_milliseconds() - time);
    }
    printf("std::vector %d x %d time=%.3f milliseconds %s\n",  VECTORS, ELEMENTS, min_time / 100, reserve ? "(reserved)" : "");
}

static void test_heap_array_time(bool reserve) {
    double min_time = 99999;
    for (int k = 0; k < 20; k++) {
        double time = time_in_milliseconds();
        for (int cycle = 0; cycle < 100; cycle++) {
            heap_array<hedgehog_t> vectors[VECTORS];
            sum1 = 0;
            for (int i = 0; i < countof(vectors); i++) {
                if (reserve) { vectors[i].reserve(ELEMENTS); }
                for (int j = 0; j < ELEMENTS; j++) {
                    hedgehog_t hog = {j, j * j};
                    vectors[i].add(hog);
                    sum1 += vectors[i][j].spines; 
                }        
            }
        }
        min_time = std::min(time, time_in_milliseconds() - time);
    }
    printf("heap_array %d x %d time=%.3f milliseconds\n", VECTORS, ELEMENTS, min_time / 100);
}

static void test_nc_array_time() {
    double min_time = 99999;
    for (int k = 0; k < 20; k++) {
        double time = time_in_milliseconds();
        for (int cycle = 0; cycle < 100; cycle++) {
            nc_array<hedgehog_t, ELEMENTS> vectors[VECTORS];
            sum2 = 0;
            for (int i = 0; i < countof(vectors); i++) {
                for (int j = 0; j < ELEMENTS; j++) {
                    hedgehog_t hog = {j, j * j};
                    vectors[i].add(hog);
                    sum2 += vectors[i][j].spines; 
                }        
            }
        }
        min_time = std::min(time, time_in_milliseconds() - time);
    }
    printf("nc_array %d x %d time=%.3f milliseconds\n", VECTORS, ELEMENTS, min_time / 100);
}

#endif // __cplusplus

int main(int argc, char* argv[]) {
#ifdef __cplusplus
    cpp_test();
    test_std_vector_time(false);
    test_std_vector_time(true);
    test_heap_array_time(true);
    test_nc_array_time();
    printf("sum0=0x%08X sum1=0x%08X sum2=0x%08X sum1-sum0=%d sum2-sum0=%d\n", sum0, sum1, sum2, sum1 - sum0, sum2 - sum0);
#endif    
}
