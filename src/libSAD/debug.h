#ifdef DEBUG
#define DEBUG_MSG(f,x) {printf("debug: "f, x);}
#else
#define DEBUG_MSG(f,x) {}
#endif
