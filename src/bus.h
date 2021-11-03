#define MIN(_a, _b) ((_a) > (_b) ? (_b) : (_a))
#define MAX(_a, _b) ((_a) < (_b) ? (_b) : (_a))

#define INRANGE(_num, _x, _y) ((_num >= MIN(_x, _y)) && (_num <= MAX(_x, _y)))

