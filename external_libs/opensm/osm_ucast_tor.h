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

#include <opensm/osm_opensm.h>
#include <opensm/osm_qos_policy.h>

typedef enum tor_typ_dst_
{
	TOR_TYP_DST_DEFAULT  = 0,
	TOR_TYP_DST_ANY      = 1,
	TOR_TYP_DST_INTERNAL = 2,
	TOR_TYP_DST_EXTERNAL = 3,
	TOR_TYP_DST_SPECIFIC = 4
}tor_typ_dst;


typedef enum status_
{
	OK    = 0,
	ERROR = 1
}STATUS;

typedef enum tor_weight_type_
{
	TOR_WEIGHT_PORT_LOAD    = 0,
	TOR_WEIGHT_AVG_LOAD 	= 1,
	TOR_WEIGHT_LAST 	= 2
}tor_weight_type;

typedef struct port_group_info_
{
	char* 					name;
	int						num_of_guids;
	int*					guids_index_list;
	int*					weight_info;
	int						num_of_def_weights;
	int						avg_load_sum;
}tor_port_group_info;

typedef struct pg_index_list_
{
	int						pg_index;
	struct pg_index_list_ * next;
}tor_pg_index_list;

typedef struct network_info_
{
	uint16_t  				pkey;
	int						num_of_guids;
	tor_pg_index_list*		pg_index_list;
	struct network_info_ * 	next;
}tor_network_info;

typedef struct tor_switch_info_
{
	int					guid_index;
	boolean_t			is_edge;
	uint32_t* 			lid_list;
	int*				port_srcgroups;
	uint8_t				num_of_ports;
	uint32_t*			port_weight;
}tor_switch_info;

typedef struct tor_info_
{
	tor_port_group_info* 	port_group_info;
	int				 		num_of_groups;
	ib_net64_t* 	 		guid_array;
	int				 		num_of_guids;
	tor_network_info*	 	network_info;
	tor_switch_info*		switch_info;
	int				 		num_of_switches;
	tor_weight_type			weight_type;

}tor_info;


/* tor structure */
typedef struct tor {
	unsigned 			num_roots;
	osm_opensm_t*		p_osm;
	tor_info			tor_info;
	FILE*				file;

} tor_t;


/* direction */
typedef enum tor_switch_dir
{
	UP = 0,
	DOWN
} tor_switch_dir_t;


struct tor_node {
	cl_list_item_t list;
	osm_switch_t *sw;
	uint64_t id;
	tor_switch_dir_t dir;
	unsigned rank;
	unsigned visited;
};

void osm_switch_count_path_tor(osm_subn_t *			p_subn,
								osm_switch_t * const 	p_sw,
								const uint8_t 			port,
								uint16_t				lid);


void 					tor_delete(void*);
int 					tor_lid_matrices(void *);
void 					delete_tor_node(struct tor_node *);
STATUS 					tor_init(tor_t*);
STATUS 					tor_get_guid_index(tor_t*, ib_net64_t, int*);
STATUS 					tor_get_guid_by_index(tor_t*, ib_net64_t*, int);
void 					tor_dump_info(tor_t*);
void 					tor_print_info(tor_t*);
STATUS 					tor_init_port_groups(tor_t*);
STATUS 					tor_read_qos_match_rules(tor_t*);
STATUS 					tor_parse_use_str(tor_t*, int*, tor_typ_dst*, osm_qos_match_rule_t*);
STATUS 					tor_get_num_of_guids_by_pkey(tor_t*, uint16_t, int*);
void 					tor_fill_weights_specific(tor_port_group_info*, int, int, tor_port_group_info*, boolean_t);
void 					tor_fill_weights_internal(tor_port_group_info*, int, int, boolean_t);
int 					tor_port_group_is_pkey(tor_t*, int, uint16_t);
STATUS 					tor_fill_weights_external_and_any(tor_t*, tor_port_group_info*, tor_typ_dst, uint16_t, int, int, boolean_t);
STATUS 					tor_fill_weights(tor_t*);
STATUS 					tor_init_guid_array(tor_t*);
STATUS 					tor_init_switches(tor_t*);
tor_switch_info* 		tor_find_switch_by_node_guid(osm_switch_t * const, tor_t*);
STATUS 					tor_add_weight(tor_t*, tor_switch_info*, osm_switch_t *, const uint8_t, uint16_t, int*);
STATUS 					tor_update_remote_switch(tor_switch_info*, osm_switch_t * const, const uint8_t, uint16_t, tor_t*, int* );
struct tor_node*		create_tor_node(osm_switch_t *);
STATUS 					tor_update_network_info(uint16_t, tor_t*, int, int);
STATUS 					tor_find_src_name(char*, tor_t*, int*);
int 					tor_get_port_group_index(tor_t*, int, int*);
int 					tor_verify_dir(int* configure, osm_switch_t* p_sw, uint8_t port);
STATUS 					tor_add_srcportgroup(osm_switch_t*, tor_t*, int);
void 					tor_flush(tor_t*);
STATUS 					tor_calloc(void**, int, tor_t*);
