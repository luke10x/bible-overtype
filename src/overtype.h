#ifndef overtype_h__
#define overtype_h__

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/ioctl.h>


// These also need to be repeaded in the implementation file
typedef struct overtype_t overtype_t;

overtype_t *ovt_create(uint8_t *blob);


#endif
