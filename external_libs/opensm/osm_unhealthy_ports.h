/*
 * Copyright (c) 2013 Mellanox Technologies LTD. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

/*
 * Abstract:
 *	Declaration of osm_unhealthy_port_t.
 */

#ifndef _OSM_UNHEALTHY_PORTS_H_
#define _OSM_UNHEALTHY_PORTS_H_

#include <sys/time.h>
#include <complib/cl_fleximap.h>
#include <iba/ib_types.h>
#include <opensm/osm_base.h>
#include <opensm/osm_path.h>
#include <opensm/osm_subnet.h>

#ifdef __cplusplus
#  define BEGIN_C_DECLS extern "C" {
#  define END_C_DECLS   }
#else				/* !__cplusplus */
#  define BEGIN_C_DECLS
#  define END_C_DECLS
#endif				/* __cplusplus */

BEGIN_C_DECLS

struct osm_physp;
struct osm_node;
struct osm_sm;

typedef struct osm_unhealthy_port {
	cl_fmap_item_t fmap_item;
	struct osm_unhealthy_port *p_obj;
	ib_net64_t guid;
	ib_net64_t peer_guid;
	uint64_t cond_mask;
	struct timeval timestamp;
	uint32_t set_err_hist;
	uint32_t flapping_hist;
	uint32_t no_resp_hist;
	uint32_t illegal_count;
	uint8_t port_num;
	char *node_desc;
	uint8_t node_type;
	uint8_t peer_port_num;
	char *peer_node_desc;
	boolean_t isolate;
	boolean_t answered_last_ls;
} osm_unhealthy_port_t;

void osm_hm_set_by_node(IN struct osm_sm *sm, IN struct osm_node *p_node,
		        IN osm_hm_cond_type_enum cond);

static inline boolean_t osm_hm_is_cond_set(IN osm_unhealthy_port_t *p_port,
				    IN osm_hm_cond_type_enum cond)
{
	if (p_port)
		return (p_port->cond_mask & 1 << cond);
	else
		return FALSE;
}

void osm_hm_set_by_physp(IN struct osm_sm *sm, IN struct osm_physp *p_physp,
			 IN osm_hm_cond_type_enum cond);

void osm_hm_set_by_dr_path(IN struct osm_sm *sm, IN uint8_t hop_count,
			   IN uint8_t *path, IN osm_hm_cond_type_enum cond);

void osm_unhealthy_port_delete(IN OUT osm_unhealthy_port_t
			       **pp_unhealthy_port);

void osm_unhealthy_port_remove(IN struct osm_sm *sm,
			       IN OUT osm_unhealthy_port_t **pp_unhealthy_port);

void osm_hm_mgr_process(IN struct osm_sm * sm);

boolean_t lookup_isolated_port(IN struct osm_sm *sm, IN struct osm_node * p_node,
			       IN const uint8_t port_num);

boolean_t osm_hm_set_port_answer_ls(IN struct osm_sm *sm,
				    IN struct osm_node *p_node,
				    IN const uint8_t port_num,
				    IN boolean_t answered);

osm_unhealthy_port_t *osm_hm_get_unhealthy_port(IN struct osm_sm *sm,
						IN struct osm_node *p_node,
						IN const uint8_t port_num);

END_C_DECLS
#endif				/* _OSM_UNHEALTHY_PORTS_H_ */
