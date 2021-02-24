#define INFO(...) \
    fprintf(stderr, "################ info: %s:%d:%s: ", __FILE__, __LINE__, __func__); \
    fprintf(stderr, __VA_ARGS__);

#define INFOP(...) \
    fprintf(stderr, __VA_ARGS__);

