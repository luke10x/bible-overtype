#include <stdint.h>
#include <stdlib.h> 

typedef struct overtype_t overtype_t;

struct overtype_t {

};

overtype_t *ovt_create(uint8_t *blob) {
  overtype_t *self = malloc(sizeof(overtype_t));
  return self;
}
