/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "securec.h"
#include "cert_manager_util.h"
#include "cert_manager_mem.h"
#include "cert_manager_status.h"
#include "cm_log.h"
#include "rbtree.h"

#define RC_OK CMR_OK
#define RC_ERROR CMR_ERROR
#define RC_MEM_ERROR CMR_ERROR_MALLOC_FAIL
#define RC_NOT_FOUND CMR_ERROR_NOT_FOUND
#define RC_BUFFER_TOO_SMALL CMR_ERROR_BUFFER_TOO_SMALL
#define RC_INSUFFICIENT_DATA CMR_ERROR_BUFFER_TOO_SMALL

/* void orintRbTree(struct RbTree *t)
   Implementation of a red-black tree, with serialization
   Reference: Introduction to Algortihms, 3ed edition
   thr following assume each key is a 31-bit integer (uint32_t), adjust if nedded */
#define COLOR_MASK  0x80000000
#define KEY_MASK    0x7fffffff
#define BLACK       0x80000000
#define RED         0

#define COLOR(n)     ((n)->key & COLOR_MASK)
#define IS_RED(n)    (COLOR((n)) == RED)
#define IS_BLACK(n)  (COLOR((n)) == BLACK)

#define SET_COLOR(n, color) ((n)->key = ((n)->key & KEY_MASK) | (color))
#define SET_RED(n)  SET_COLOR((n), RED)
#define SET_BLACK(n) SET_COLOR((n), BLACK)

#define KEY(n) ((n)->key & KEY_MASK)

#ifdef __cplusplus
extern "C" {
#endif

struct EncoderContext {
    uint8_t *buf;
    uint32_t off;
    uint32_t len;
    int status;
    RbTreeValueEncoder enc;
};

RbTreeKey RbTreeNodeKey(const struct RbTreeNode *n)
{
    return n == NULL ? 0 : KEY(n);
}

static int NewNode(struct RbTreeNode **nptr, RbTreeKey key, RbTreeValue value)
{
    struct RbTreeNode *n = CMMalloc(sizeof(struct RbTreeNode));
    if (n == NULL) {
        return CMR_ERROR_MALLOC_FAIL;
    }
    *nptr = n;
    n->key = key;
    n->value = value;
    n->p = NULL;
    n->left = NULL;
    n->right = NULL;
    return CMR_OK;
}

int RbTreeNew(struct RbTree *t)
{
    ASSERT_ARGS(t);

    ASSERT_FUNC(NewNode(&(t->nil), 0, NULL));
    SET_BLACK(t->nil);
    t->root = t->nil;

    return CMR_OK;
}

static void LeftRotate(struct RbTree *t, struct RbTreeNode *x)
{
    struct RbTreeNode *y = x->right;
    x->right = y->left;
    if (y->left != t->nil) {
        y->left->p = x;
    }
    y->p = x->p;
    if (x->p == t->nil) {
        t->root = y;
    } else if (x == x->p->left) {
        x->p->left = y;
    } else {
        x->p->right = y;
    }
    y->left = x;
    x->p = y;
}

static void RightRotate(struct RbTree *t, struct RbTreeNode *x)
{
    struct RbTreeNode *y = x->left;
    x->left = y->right;
    if (y->right != t->nil) {
        y->right->p = x;
    }
    y->p = x->p;
    if (x->p == t->nil) {
        t->root = y;
    } else if (x == x->p->right) {
        x->p->right = y;
    } else {
        x->p->left = y;
    }
    y->right = x;
    x->p = y;
}

static void InsertFixUpRed(struct RbTree *t, struct RbTreeNode **zTreeNode)
{
    struct RbTreeNode *y = NULL;
    struct RbTreeNode *z = *zTreeNode;
    y = z->p->p->right;
    if (IS_RED(y)) {
        SET_BLACK(z->p);
        SET_BLACK(y);
        SET_RED(z->p->p);
        z = z->p->p;
    } else {
        if (z == z->p->right) {
            z = z->p;
            LeftRotate(t, z);
        }
        // case 3
        SET_BLACK(z->p);
        SET_RED(z->p->p);
        RightRotate(t, z->p->p);
    }
    *zTreeNode = z;
}

static void InsertFixUpBlack(struct RbTree *t, struct RbTreeNode **zTreeNode)
{
    struct RbTreeNode *y = NULL;
    struct RbTreeNode *z = *zTreeNode;

    y = z->p->p->left;
    if (IS_RED(y)) {
        SET_BLACK(z->p);
        SET_BLACK(y);
        SET_RED(z->p->p);
        z = z->p->p;
    } else {
        if (z == z->p->left) {
            z = z->p;
            RightRotate(t, z);
        }
        SET_BLACK(z->p);
        SET_RED(z->p->p);
        LeftRotate(t, z->p->p);
    }
    *zTreeNode = z;
}

static void InsertFixUp(struct RbTree *t, struct RbTreeNode *z)
{
    while (IS_RED(z->p)) {
        if (z->p == z->p->p->left) {
            InsertFixUpRed(t, &z);
        } else {
            InsertFixUpBlack(t, &z);
        }
    }

    SET_BLACK(t->root);
}

int RbTreeInsert(struct RbTree *t, RbTreeKey key, const RbTreeValue value)
{
    ASSERT_ARGS(t);

    struct RbTreeNode *z = NULL;
    ASSERT_FUNC(NewNode(&z, key, value));

    struct RbTreeNode *y = t->nil;
    struct RbTreeNode *x = t->root;

    while (x != t->nil) {
        y = x;
        if (KEY(z) < KEY(x)) {
            x = x->left;
        } else {
            x = x->right;
        }
    }

    z->p = y;
    if (y == t->nil) {
        t->root = z;
    } else if (KEY(z) < KEY(y)) {
        y->left = z;
    } else {
        y->right = z;
    }

    z->left = t->nil;
    z->right = t->nil;
    SET_RED(z);

    InsertFixUp(t, z);

    return CMR_OK;
}

static void Transplant(struct RbTree *t, struct RbTreeNode *u, struct RbTreeNode *v)
{
    if (u->p == t->nil) {
        t->root = v;
    } else if (u == u->p->left) {
        u->p->left = v;
    } else {
        u->p->right = v;
    }
    v->p = u->p;
}

static struct RbTreeNode *Minimum(const struct RbTree *t, struct RbTreeNode *x)
{
    while (x->left != t->nil) {
        x = x->left;
    }
    return x;
}

static void DeleteFixUpBlack(struct RbTree *t, struct RbTreeNode **treeNode)
{
    struct RbTreeNode *w = NULL;
    struct RbTreeNode *x = *treeNode;
    w = x->p->right;
    if (IS_RED(w)) {
        SET_BLACK(w);
        SET_RED(x->p);
        LeftRotate(t, x->p);
        w = x->p->right;
    }
    if (IS_BLACK(w->left) && IS_BLACK(w->right)) {
        SET_RED(w);
        x = x->p;
    } else {
        if (IS_BLACK(w->right)) {
            SET_BLACK(w->left);
            SET_RED(w);
            RightRotate(t, w);
            w = x->p->right;
        }

        SET_COLOR(w, COLOR(x->p));
        SET_BLACK(x->p);
        SET_BLACK(w->right);
        LeftRotate(t, x->p);
        x = t->root;
    }
    *treeNode = x;
}

static void DeleteFixUpRed(struct RbTree *t, struct RbTreeNode **treeNode)
{
    struct RbTreeNode *w = NULL;
    struct RbTreeNode *x = *treeNode;

    w = x->p->left;
    if (IS_RED(w)) {
        SET_BLACK(w);
        SET_RED(x->p);
        RightRotate(t, x->p);
        w = x->p->left;
    }
    if (IS_BLACK(w->right) && IS_BLACK(w->left)) {
        SET_RED(w);
        x = x->p;
    } else {
            if (IS_BLACK(w->left)) {
            SET_BLACK(w->right);
            SET_RED(w);
            LeftRotate(t, w);
            w = x->p->left;
        }

        SET_COLOR(w, COLOR(x->p));
        SET_BLACK(x->p);
        SET_BLACK(w->left);
        RightRotate(t, x->p);
        x = t->root;
    }
    *treeNode = x;
}

static void DeleteFixUp(struct RbTree *t, struct RbTreeNode *x)
{
    while (x != t->root && IS_BLACK(x)) {
        if (x == x->p->left) {
            DeleteFixUpBlack(t, &x);
        } else {
            DeleteFixUpRed(t, &x);
        }
    }
    SET_BLACK(x);
}

int RbTreeDelete(struct RbTree *t, struct RbTreeNode *z)
{
    ASSERT_ARGS(t);
    struct RbTreeNode *x = NULL;
    struct RbTreeNode *y = z;
    RbTreeKey yColor = COLOR(y);

    if (z->left == t->nil) {
        x = z->right;
        Transplant(t, z, z->right);
    } else if (z->right == t->nil) {
        x = z->left;
        Transplant(t, z, z->left);
    } else {
        y = Minimum(t, z->right);
        yColor = COLOR(y);
        x = y->right;
        if (y->p == z) {
            x->p = y;
        } else {
            Transplant(t, y, y->right);
            y->right = z->right;
            y->right->p = y;
        }
        Transplant(t, z, y);
        y->left = z->left;
        y->left->p = y;
        SET_COLOR(y, COLOR(z));
    }
    if (yColor == BLACK) {
        DeleteFixUp(t, x);
    }
    CMFree(z);
    return CMR_OK;
}

int RbTreeFindNode(struct RbTreeNode **np, RbTreeKey key, const struct RbTree *t)
{
    ASSERT_ARGS(t && np);

    *np = NULL;

    struct RbTreeNode *n = t->root;
    while (n != t->nil) {
        if (KEY(n) == key) {
            *np = n;
            return CMR_OK;
        } else if (key < KEY(n)) {
            n = n->left;
        } else {
            n = n->right;
        }
    }
    return CMR_ERROR_NOT_FOUND;
}

static void TraverseInOrder(const struct RbTree *t, const struct RbTreeNode *x,
    RbTreeNodeHandler handler, const void *context)
{
    if (x == t->nil) {
        return;
    }
    TraverseInOrder(t, x->left, handler, context);
    handler(KEY(x), x->value, context);
    TraverseInOrder(t, x->right, handler, context);
}

static void TraversePreOrder(const struct RbTree *t, const struct RbTreeNode *x,
    RbTreeNodeHandler handler, const void *context)
{
    if (x == t->nil) {
        return;
    }
    handler(KEY(x), x->value, context);
    TraversePreOrder(t, x->left, handler, context);
    TraversePreOrder(t, x->right, handler, context);
}

static void TraversePostOrder(const struct RbTree *t, const struct RbTreeNode *x,
    RbTreeNodeHandler handler, const void *context)
{
    if (x == t->nil) {
        return;
    }
    TraversePostOrder(t, x->left, handler, context);
    TraversePostOrder(t, x->right, handler, context);
    handler(KEY(x), x->value, context);
}

static void Destroy(struct RbTree *t, struct RbTreeNode *x)
{
    if (x != t->nil) {
        Destroy(t, x->left);
        x->left = t->nil;
        Destroy(t, x->right);
        x->right = t->nil;
        CMFree(x);
    }
}

static int RbTreeDestroy(struct RbTree *t)
{
    ASSERT_ARGS(t);
    Destroy(t, t->root);
    CMFree(t->nil);

    t->nil = NULL;
    t->root = NULL;
    return RC_OK;
}

int RbTreeDestroyEx(struct RbTree *t, RbTreeNodeHandler handler, const void *context)
{
    ASSERT_ARGS(t && handler);
    TraverseInOrder(t, t->root, handler, context);
    return RbTreeDestroy(t);
}

static void Encoder(RbTreeKey key, RbTreeValue value, const void *context)
{
    struct EncoderContext *ctx = (struct EncoderContext *) context;
    if (ctx->status != CMR_OK) {
        /* already failed. do not continue */
        CM_LOG_W("already failed: %d\n", ctx->status);
        return;
    }
    uint32_t keySize = sizeof(RbTreeKey);
    uint32_t valueSize = 0;

    /* get value size */
    int rc = ctx->enc(value, NULL, &valueSize);
    if (rc != CMR_OK) {
        CM_LOG_W("value encoder get length failed: %d\n", rc);
        ctx->status = rc;
        return;
    }

    /* each code is encoded ad (key | encoded_value_len | encoded_value)
       encoded_value_len is uint32_t. */
    uint32_t sz = keySize + sizeof(uint32_t) + valueSize;

    /* if buffer is provided, do the actual encoding */
    if (ctx->buf != NULL) {
        if (ctx->off + sz > ctx->len) {
            ctx->status = CMR_ERROR_BUFFER_TOO_SMALL;
            return;
        }
        uint8_t *buf = ctx->buf + ctx->off;
        if (memcpy_s(buf, ctx->len, &key, keySize) != EOK) {
            return;
        }
        buf += keySize;
        if (memcpy_s(buf, ctx->len, &valueSize, sizeof(uint32_t)) != EOK) {
            return;
        }
        buf += sizeof(uint32_t);
        rc = ctx->enc(value, buf, &valueSize);
        if (rc != CMR_OK) {
            CM_LOG_W("value encoder encoding failed: %d\n", rc);
            ctx->status = rc;
            return;
        }
    }
    /* in any case, updata offset in context. */
    ctx->off += sz;
}


int RbTreeEncode(const struct RbTree *t, RbTreeValueEncoder enc, uint8_t *buf, uint32_t *size)
{
    ASSERT_ARGS(t && t->root && enc && size);

    struct EncoderContext ctx = {
        .buf = buf,
        .off = 0,
        .len = *size,
        .status = CMR_OK,
        .enc = enc,
    };

    TraverseInOrder(t, t->root, Encoder, &ctx);
    if (ctx.status != CMR_OK) {
        return ctx.status;
    }
    *size = ctx.off;
    return CMR_OK;
}

int RbTreeDecode(struct RbTree *t, RbTreeValueDecoder dec, uint8_t *buf, uint32_t size)
{
    ASSERT_ARGS(t && t->root && dec && size);

    uint32_t off = 0;
    while (off < size) {
        uint32_t remaining = size - off;
        uint32_t headerSize = sizeof(RbTreeKey) + sizeof(uint32_t);

        if (remaining < headerSize) {
            /* something is wrong */
            return RC_INSUFFICIENT_DATA;
        }

        uint8_t *s = buf + off;

        RbTreeKey key;
        if (memcpy_s(&key, sizeof(RbTreeKey), s, sizeof(RbTreeKey)) != EOK) {
            return CMR_ERROR_INVALID_OPERATION;
        }
        s += sizeof(RbTreeKey);

        uint32_t valueSize;
        if (memcpy_s(&valueSize, sizeof(RbTreeKey), s, sizeof(uint32_t)) != EOK) {
            return CMR_ERROR_INVALID_OPERATION;
        }
        s += sizeof(uint32_t);

        uint32_t sz = headerSize + valueSize;
        if (remaining < sz) {
            return RC_INSUFFICIENT_DATA;
        }

        RbTreeValue value = NULL;
        int rc = dec(&value, s, valueSize);
        if (rc != CMR_OK) {
            return rc;
        }

        rc = RbTreeInsert(t, key, value);
        if (rc != CMR_OK) {
            return rc;
        }
        off += sz;
    }
    return CMR_OK;
}

#ifdef __cplusplus
}

#endif