#include <stdlib.h>

#include "activity.h"

typedef struct activity_t activity_t;

struct activity_t {
    activity_t* next;
    activity_t* prev;
};

activity_t* activity_welcome_ctor() {
  activity_t* activity = malloc(sizeof(activity_t));
  return activity;
}

