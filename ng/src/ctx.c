#include <stdlib.h>

#include "ctx.h"

typedef struct ctx_t ctx_t;

struct ctx_t {
    activity_t* activity;
    int type;
};

ctx_t* ctx_wasm_ctor(activity_t* activity) {
  ctx_t* ctx = malloc(sizeof(ctx_t));
  ctx->activity = activity;
  ctx->type = CTX_TYPE_WASM;
  
  return ctx;
}

void ctx_wasm_init(ctx_t* ctx) {
  // TODO something in js land
}

void ctx_virtual_init(ctx_t* ctx) {
  if (ctx->type == CTX_TYPE_WASM) {
    ctx_wasm_init(ctx);
  }
}

