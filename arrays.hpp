#pragma once

/* Arrays is implementation of simple `dynamic` arrays in C++. 
    Usage examples:
    array<int, 128> a;
    a.add(1);
    a.add(2);
    a.add(3);
    a.remove_at(1); // same result as remove(2);

   Implementation expects following macros to be defined before it is included:
    #define null ((void*)0) // or nullptr
    #define countof(a) (sizeof((a))/sizeof(*(a)))
    #define array_alloc(n) malloc(n) 
    #define array_realloc(p, n) realloc(p, n)
    #define array_free free
    #define array_assert(e, ...) if (!(e)) { ... }
    Macro ARRAYS_IMPLEMENT_INTERFACE may be defined to allow all array to implement 
    an 'array_i' interface. 
    WARNING: ARRAYS_IMPLEMENT_INTERFACE results in *significant* performance penalty 
    because all methods will become virtual and thus not inlined.
*/

#pragma push_macro("null") // // https://gcc.gnu.org/onlinedocs/gcc/Push_002fPop-Macro-Pragmas.html
#pragma push_macro("countof")

#ifndef null
#   define null ((void*)0)
#endif
#ifndef countof
#   define countof(a) (sizeof((a)) / sizeof(*(a)))
#endif

#ifndef array_alloc
#   define array_alloc(n) malloc(n)
#   define array_realloc(p, n) realloc(p, n)
#   define array_free(p) free(p)
#endif
#ifndef array_assertion
#   if defined(DEBUG) || defined(_DEBUG)
#       define array_assertion(e, ...) if (!(e)) { printf("%s:%d array_assertion failed: %s ", __FILE__, __LINE__, #e); printf(__VA_ARGS__); printf("\n"); breakpoint(); }
#   else
#       define array_assertion(e, ...) 
#   endif
#endif

// All array classes only supports pointers, atomic types and constructor-less plain data objects "t".
// By design: no copy constructor, not assignable, no shrinkig, double capacity realloc for heap.
// Arrays are "heavy" - absence of copy constructor and assign operator ensure 
// that arrays are not passed by value as function parameters by mistake.

#ifdef __cplusplus

#ifdef ARRAYS_IMPLEMENT_INTERFACE

template <typename t> struct array_i {
    virtual t& operator[](const int i) = 0;
    virtual bool add(const t &e) = 0; // returns true if added, false if array overflows
    virtual int index_of(const t &e) const = 0; // returns -1 if not found
    virtual void remove_at(const int i) = 0; 
    virtual bool remove(const t &e) = 0; // returns false if not found
    virtual bool reserve(int k) = 0;
    virtual bool resize(int k) = 0;
    virtual int size() const = 0;
    virtual void clear() = 0;
    virtual t* cast() = 0; // array::cast() may modify content of array
    // cast() for "vec" and "array" pointer can only be used before any subsequent call to add()
};

#define implements_array_i(t) : public array_i<t>
#else
#define implements_array_i(t) 
#endif

template <typename t, int n> struct array implements_array_i(t) {

    array() : count(0) { array_assertion(n > 0, "array size cannot be less then 1"); }

    const t& operator[](const int i) const { array_assertion(0 <= i && i < count, "%d out of range [0..%d]", i, count);  return a[i]; }
    t& operator[](const int i) { array_assertion(0 <= i && i < count, "%d out of range [0..%d]", i, count);  return a[i]; }

    bool add(const t &e) { 
        if (count == n) {
            return false;
        } else {
            a[count++] = e;
            return true;
        }
    }

    int index_of(const t &e) const {
        for (int i = 0; i < count; i++) { if (memcmp(&a[i], &e, sizeof(t)) == 0) { return i; } }
        return -1;
    }

    void remove_at(const int i) {
        array_assertion(0 <= i && i < count, "%d out of range [0..%d]", i, count);
        if (i < count - 1) { memcpy(&a[i], &a[i + 1], (count - 1 - i) * sizeof(t)); }
        count--;
    }

    bool remove(const t &e) {
        const int i = index_of(e);
        if (i >= 0) { remove_at(i); }
        return i >= 0;
    }

    bool reserve(const int k) {
        array_assertion(0 <= k && k < countof(a), "%d out of range [0..%d]", k, countof(a));
        return 0 <= k && k < countof(a);
    }

    bool resize(const int k) {
        array_assertion(0 <= k && k < countof(a), "%d out of range [0..%d]", k, countof(a));
        bool b = 0 <= k && k < countof(a);
        if (b) { count = k; }
        return b;
    }

    void clear() { count = 0; }

    int size() const { return count; }
    t* cast() { return a; }
    operator const t*() const { return a; }
    operator t*() const { return a; }

private:
    array(const array&) : a((t*)null), count(0) { array_assertion(false, "not implemented"); }
    const array& operator=(const array&) {  array_assertion(false, "not implemented"); return this; }
    t a[n];
    int count;
};

template <typename t> struct heap_array implements_array_i(t) {

    heap_array() : a((t*)null), count(0), capacity(0) {}
    ~heap_array() { array_free(a); }

    const t& operator[](const int i) const { array_assertion(0 <= i && i < count, "%d out of range [0..%d]", i, count);  return a[i]; }
    t& operator[](const int i) { array_assertion(0 <= i && i < count, "%d out of range [0..%d]", i, count);  return a[i]; }

    bool reserve(const int c) {
        if (c > capacity) {
            t* n = (t*)array_realloc(a, c * sizeof(t));
            if (n != null) { 
                a = n;
                capacity = c;
            }
            return n != null;
        }
        return true;
    }
    
    bool resize(const int k) {
        bool b = reserve(k); 
        if (b) { count = k; }
        return b;
    }

    bool add(const t &e) { // returns true if added, false if out of memory
        bool b = count < capacity || reserve(capacity == 0 ? 16 : capacity + capacity);
        if (b) {
            a[count++] = e;
        }
        return b;
    }
    
    int index_of(const t &e) const {
        for (int i = 0; i < count; i++) { if (memcmp(&a[i], &e, sizeof(t)) == 0) { return i; } }
        return -1;
    }

    void remove_at(const int i) {
        array_assertion(0 <= i && i < count, "%d out of range [0..%d]", i, count);
        if (i < count - 1) { memcpy(&a[i], &a[i + 1], (count - 1 - i) * sizeof(t)); }
        count--;
    }

    bool remove(const t &e) {
        const int i = index_of(e);
        if (i >= 0) { remove_at(i); }
        return i >= 0;
    }

    void clear() { count = 0; }

    int size() const { return count; }

    t* cast() { return a; }
    operator const t*() const { return a; }
    operator t*() const { return a; }

private:
    heap_array(const heap_array&) : a((t*)null), count(0), capacity(0) { array_assertion(false, "not implemented"); }
    const heap_array& operator=(const heap_array&) {  array_assertion(false, "not implemented"); return this; }
    t* a;
    int count;
    int capacity;
};

template <typename t, int n = 16> struct nc_array implements_array_i(t) { // non-contiguous array - first "n" elements are not heap allocated 

    nc_array() {}

    const t& operator[](int i) const { array_assertion(0 <= i && i < size(), "%d out of range [0..%d]", i, size());  return i < a.size() ? a[i] : v[i - a.size()]; }
    t& operator[](int i) { array_assertion(0 <= i && i < size(), "%d out of range [0..%d]", i, size());  return i < a.size() ? a[i] : v[i - a.size()]; }

    bool add(const t &e) { 
        return a.size() > 0 || size() >= n ? v.add(e) : a.add(e);
    }

    int index_of(const t &e) const {
        const int i = a.index_of(e);
        if (i >= 0) { return i; }
        const int j = v.index_of(e);
        return j < 0 ? j : j + a.size();
    }

    void remove_at(int i) {
        array_assertion(0 <= i && i < size(), "index=%d out of range [0..%d]", i, size());
        if (v.size() == 0) {
//          printf("a.remove_at(%d)\n", i);
            a.remove_at(i);
        } else if (i < a.size()) {
//          printf("a.remove_at(%d) + v.remove_at(0)\n", i);
            a.remove_at(i);
            a.add(v[0]);
            v.remove_at(0);
        } else {
//          printf("v.remove_at(%d)\n", i - a.size());
            v.remove_at(i - a.size());
        }
    }

    bool remove(const t &e) {
        const int i = index_of(e);
        if (i >= 0) { remove_at(i); }
        return i >= 0;
    }

    bool reserve(const int c) {
        array_assertion(c > 0, "capacity must be positive = %d", c);
        if (a.size() == 0) {
            return v.reserve(c);
        } else if (c > n) { 
            return v.reserve(c - n);
        } else {
            return true;
        }
    }

    bool resize(const int c) {
        if (a.size() == 0) { // "cast()" migrated array to "heap"
            return v.resize(c);
        } else if (c > n) {
            return v.resize(c - n); 
        } else { 
            v.clear(); 
            return a.resize(c); 
        }
    }

    void clear() { a.clear(); v.clear(); }

    int size() const { return a.size() + v.size(); }

    t* cast() {
        if (v.size() == 0) { 
            return &a[0]; 
        } else if (a.size() > 0) { 
            int k = size();
            v.reserve(k);
            if (v.size() - a.size() > 0) {
                memmove(&v[a.size()], &v[0], (v.size() - a.size()) * sizeof(t));
            }                
            memcpy(&v[0], &a[0], a.size() * sizeof(t));
            a.clear();
            v.resize(k);
        }            
        return &v[0];         
    }

private:
    nc_array(const nc_array&)  { array_assertion(false, "not implemented"); }
    const nc_array& operator=(const nc_array&) { array_assertion(false, "not implemented"); return this; }
    operator const t*() const { array_assertion(false, "not implemented"); return null; }
    operator t*() const { array_assertion(false, "not implemented"); return null; }
    array<t,n> a;
    heap_array<t> v;
};

#endif // __cplusplus

#pragma pop_macro("countof")
#pragma pop_macro("null")
