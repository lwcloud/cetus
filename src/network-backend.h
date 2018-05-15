/* $%BEGINLICENSE%$
 Copyright (c) 2007, 2012, Oracle and/or its affiliates. All rights reserved.

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License as
 published by the Free Software Foundation; version 2 of the
 License.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 02110-1301  USA

 $%ENDLICENSE%$ */

#ifndef _BACKEND_H_
#define _BACKEND_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "network-conn-pool.h"
#include "chassis-mainloop.h"

#include "network-exports.h"
#ifdef HAVE_OPENSSL
#include <openssl/rsa.h>
#endif
typedef enum {
    BACKEND_STATE_UNKNOWN,
    BACKEND_STATE_UP,
    BACKEND_STATE_DOWN,
    BACKEND_STATE_MAINTAINING,
    BACKEND_STATE_DELETED,
} backend_state_t;

#define NO_PREVIOUS_STATE -1

typedef enum {
    BACKEND_TYPE_UNKNOWN,
    BACKEND_TYPE_RW,
    BACKEND_TYPE_RO,
} backend_type_t;

typedef enum {
    BACKEND_ALGO_ROUND_ROBIN,
    BACKEND_ALGO_RANDOM,
    BACKEND_ALGO_FIRST,
} backend_algo_t;

typedef struct backend_config {
    GString *default_username;
    GString *default_db;

    int max_conn_pool;
    int mid_conn_pool;

    gchar charset;

} backend_config;

typedef struct {
    network_address *addr;
    GString *server_group;
    GString *address;           /* original address, might be domain name or ip */

    backend_state_t state;   /**< UP or DOWN */
    backend_type_t type;     /**< ReadWrite or ReadOnly */

    GTimeVal state_since;    /**< timestamp of the last state-change */

    network_connection_pool *pool; /**< the pool of open connections */

    /**< number of open connections to this backend for SQF */
    int connected_clients;

    /**< the UUID of the backend */
    GString *uuid;

    backend_config *config;
    GPtrArray *challenges;
    time_t last_check_time;
    int slave_delay_msec;       /* valid if this is a ReadOnly slave */
} network_backend_t;

NETWORK_API network_backend_t *network_backend_new();
NETWORK_API void network_backend_free(network_backend_t *b);
NETWORK_API int network_backend_conns_count(network_backend_t *b);
NETWORK_API int network_backend_init_extra(network_backend_t *b, chassis *chas);
void network_backend_save_challenge(network_backend_t *b, const network_mysqld_auth_challenge *, gboolean);
network_mysqld_auth_challenge *network_backend_get_challenge(network_backend_t *b);

typedef struct {
    unsigned int ro_server_num;
    unsigned int read_count;
    GPtrArray *backends;
#ifdef HAVE_OPENSSL
    RSA *rsa;
#endif
    /* GHashTable *ip_table; */
    GTimeVal backend_last_check;
    GPtrArray *groups;          /* GPtrArray<network_group_t *> */
} network_backends_t;

NETWORK_API network_backends_t *network_backends_new();
NETWORK_API void network_backends_free(network_backends_t *);
NETWORK_API int network_backends_add(network_backends_t *, const gchar *, backend_type_t, backend_state_t, void *);
NETWORK_API int network_backends_remove(network_backends_t *bs, guint index);
NETWORK_API int network_backends_check(network_backends_t *bs);
NETWORK_API int network_backends_modify(network_backends_t *, guint, backend_type_t, backend_state_t, backend_state_t);
NETWORK_API network_backend_t *network_backends_get(network_backends_t *bs, guint ndx);
NETWORK_API guint network_backends_count(network_backends_t *bs);
NETWORK_API gboolean network_backends_load_user_profile(network_backends_t *, chassis *);

NETWORK_API gboolean network_backends_load_config(network_backends_t *, chassis *);

/* get backend index by ip:port string */
int network_backends_find_address(network_backends_t *bs, const char *);

network_mysqld_auth_challenge *network_backends_get_challenge(network_backends_t *b, int back_ndx);

#define MAX_GROUP_SLAVES 4

typedef struct network_group_t {
    GString *name;
    network_backend_t *master;
    network_backend_t *slaves[MAX_GROUP_SLAVES];
    int nslaves;
    unsigned int slave_visit_cnt;
} network_group_t;

network_group_t *network_backends_get_group(network_backends_t *, const GString *name);

/* round-robin pick a slave from group */
network_backend_t *network_group_pick_slave_backend(network_group_t *);

void network_group_get_slave_names(network_group_t *, GString *);

int network_backends_get_ro_ndx(network_backends_t *, backend_algo_t);

int network_backends_get_rw_ndx(network_backends_t *);

int network_backends_idle_conns(network_backends_t *);
int network_backends_used_conns(network_backends_t *);

#endif /* _BACKEND_H_ */
