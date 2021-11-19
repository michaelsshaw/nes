#ifndef NES_UTIL_H_
#define NES_UTIL_H_

/*! @file util.h
 * A bunch of preprocessor macros that i find useful
 */

#define INRANGE(_num, _x, _y)        ((_num) >= (_x) && (_num) <= (_y))
#define IFINRANGE(__num, __xx, __yy) if (INRANGE((__num), (__xx), (__yy)))

#endif // NES_UTIL_H_
