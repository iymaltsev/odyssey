#ifndef OD_SERVER_POOL_H
#define OD_SERVER_POOL_H

/*
 * odissey.
 *
 * PostgreSQL connection pooler and request router.
*/

typedef struct od_serverpool od_serverpool_t;

typedef int (*od_serverpool_cb_t)(od_server_t*, void*);

struct od_serverpool
{
	od_list_t active;
	od_list_t idle;
	od_list_t expire;
	int       count_active;
	int       count_idle;
	int       count_expire;
	od_list_t link;
};

void od_serverpool_init(od_serverpool_t*);
void od_serverpool_free(od_serverpool_t*);
void od_serverpool_set(od_serverpool_t*, od_server_t*,
                       od_serverstate_t);

od_server_t*
od_serverpool_next(od_serverpool_t*, od_serverstate_t);

od_server_t*
od_serverpool_foreach(od_serverpool_t*, od_serverstate_t,
                      od_serverpool_cb_t, void*);

#endif /* OD_SERVER_POOL_H */