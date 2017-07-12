// Copyright (c) 2016-2017, NetApp, Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>

#include <warpcore/warpcore.h>

#include "conn.h"
#include "quic.h"
#include "stream.h"


int32_t stream_cmp(const struct q_stream * const a,
                   const struct q_stream * const b)
{
    return (int32_t)a->id - (int32_t)b->id;
}


SPLAY_GENERATE(stream, q_stream, node, stream_cmp)


struct q_stream * get_stream(struct q_conn * const c, const uint32_t id)
{
    struct q_stream which = {.id = id};
    return SPLAY_FIND(stream, &c->streams, &which);
}


struct q_stream * new_stream(struct q_conn * const c, const uint32_t id)
{
    ensure(get_stream(c, id) == 0, "stream already %u exists", id);

    struct q_stream * const s = calloc(1, sizeof(*s));
    ensure(c, "could not calloc q_stream");
    s->c = c;
    STAILQ_INIT(&s->o);
    STAILQ_INIT(&s->i);
    STAILQ_INIT(&s->r);
    s->id = id;
    if (id)
        c->next_sid += 2;
    SPLAY_INSERT(stream, &c->streams, s);
    warn(info, "reserved new str %u on conn %" PRIx64 " as %s", id, c->id,
         is_set(CONN_FLAG_CLNT, c->flags) ? "client" : "server");
    return s;
}


void free_stream(struct q_stream * const s)
{
    warn(info, "freeing str %u on conn %" PRIx64 " as %s", s->id, s->c->id,
         is_set(CONN_FLAG_CLNT, s->c->flags) ? "client" : "server");

    w_free(w_engine(s->c->sock), &s->o);
    w_free(w_engine(s->c->sock), &s->i);
    w_free(w_engine(s->c->sock), &s->r);

    SPLAY_REMOVE(stream, &s->c->streams, s);
    free(s);
}
