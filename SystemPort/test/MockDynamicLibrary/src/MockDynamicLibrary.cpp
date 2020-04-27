#ifdef _WIN32
#define API __declspec(dllexport)
#else /* POSIX */ 
#define API
#endif

extern "C" API int Foo(int x) {
    return x * x;
}