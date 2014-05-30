
// samiv, ensisoft, im adding this header so that pyconfig.h is found
// correctly  and the client code doesn't need to worry about that.
// PC/pyconfig.h hand written (by python folks) for msvc, watcom and other windows compilers.
// linux/pyconfig.h is shit spit out by configure
#if defined(_WIN32)
#  include "../PC/pyconfig.h"
#else
#  include "../linux/pyconfig.h"
#endif

