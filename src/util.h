#ifndef NES_UTIL_H_
#define NES_UTIL_H_

/*! @file util.h
 * A bunch of preprocessor macros that i find useful
 */

#define MIN(_a, _b) ((_a) > (_b) ? (_b) : (_a))
#define MAX(_a, _b) ((_a) < (_b) ? (_b) : (_a))

#define INRANGE(_num, _x, _y)        ((_num >= MIN(_x, _y)) && (_num <= MAX(_x, _y)))
#define IFINRANGE(__num, __xx, __yy) if (INRANGE((__num), (__xx), (__yy)))

#endif // NES_UTIL_H_
