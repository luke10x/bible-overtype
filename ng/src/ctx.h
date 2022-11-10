#ifndef ctx_h__

#define ctx_h__

#include "activity.h"

#define CTX_TYPE_WASM 1 

typedef struct ctx_t ctx_t;

ctx_t* ctx_wasm_ctor(activity_t* activity);

void ctx_virtual_init(ctx_t* ctx);

#endif
