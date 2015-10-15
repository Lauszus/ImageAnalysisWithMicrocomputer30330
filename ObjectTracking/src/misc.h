#ifndef __misc_h__
#define __misc_h__

#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))

enum Connected {
    CONNECTED_4 = 0,
    CONNECTED_6,
    CONNECTED_8,
};

#endif
