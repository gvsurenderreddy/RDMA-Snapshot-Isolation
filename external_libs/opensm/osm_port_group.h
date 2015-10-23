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
 *	copyright notice, this list of conditions and the following
 *	disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *	copyright notice, this list of conditions and the following
 *	disclaimer in the documentation and/or other materials
 *	provided with the distribution.
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
 *      Port Group manager
 */
#ifndef OSM_PORT_GROUP_H
#define OSM_PORT_GROUP_H

#include <iba/ib_types.h>
#include <regex.h>
#include <complib/cl_qmap.h>

#define LIST_DELIMITER ","
#define PG_ALL "ALL"
#define PG_ALL_SW "ALL_SWITCHES"
#define PG_ALL_CA "ALL_CAS"
#define NONE_STR  ""
#define MAX_GUID_LEN 18
#define MAX_RULE_LENGTH 256
#define MAX_PORT_LEN 3

struct osm_opensm;
struct osm_port;
struct osm_physp;
struct osm_subn;

typedef enum osm_port_group_rule_type {
	OSM_PORT_GROUP_NO_RULE = 0,
	OSM_PORT_GROUP_GUID_RULE,
	OSM_PORT_GROUP_GUID_RANGE_RULE,
	OSM_PORT_GROUP_GUID_NAME_RULE,
	OSM_PORT_GROUP_GUID_REGEX_RULE,
	OSM_PORT_GROUP_GUID_UNION_RULE,
	OSM_PORT_GROUP_GUID_SUBTRACT_RULE,
	OSM_PORT_GROUP_PREDEFINED_RULE
} osm_port_group_rule_type_t;

typedef struct osm_port_group_member {
	cl_map_item_t map_item;
	struct osm_port *port;
	boolean_t *physp_members;
} osm_port_group_member_t;

typedef struct osm_port_group {
	cl_list_item_t list_item;
	char *use;
	char *name;
	char *list;
	char *hostname;
	char *regex_str;
	char *mask;		/*used to store list|hostname|regex|mask */
	uint64_t min_guid;
	uint64_t max_guid;
	uint8_t port_num;
	char *src_port_grp_1;
	char *src_port_grp_2;
	cl_qmap_t ports;
	osm_port_group_rule_type_t rule_type;
	boolean_t applied;
	regex_t regex;
} osm_port_group_t;

typedef struct osm_port_group_mgr {
	cl_qlist_t port_groups;
} osm_port_group_mgr_t;

/****f* OpenSM: Port Group/osm_port_grp_mgr_construct
* NAME
*	osm_port_grp_mgr_construct
*
* DESCRIPTION
*	This function constructs a Port Group Manager object.
*
* SYNOPSIS
*/
void osm_port_grp_mgr_construct(IN osm_port_group_mgr_t * p_mgr);
/*
* PARAMETERS
*	p_mgr
*		Pointer to a port manager to construct
*
* RETURN VALUE
*	none
*/

/****f* OpenSM: Port Group/osm_port_grp_mgr_create
* NAME
*	osm_port_grp_mgr_create
*
* DESCRIPTION
*	This function populates a Port Group Manager object.
*
* SYNOPSIS
*/
boolean_t osm_port_grp_mgr_create(IN struct osm_opensm *p_osm);
/*
* PARAMETERS
*	p_osm
*		Pointer to osm of the port group manager.
*
* RETURN VALUE
*	success indication (TRUE = Success, FALSE = Fail)
*/

/****f* OpenSM: Port Group/osm_port_grp_mgr_destroy
* NAME
*	osm_port_grp_mgr_destroy
*
* DESCRIPTION
*	This function destroys a Port Group Manager object.
*
* SYNOPSIS
*/
void osm_port_grp_mgr_destroy(IN osm_port_group_mgr_t * p_mgr);
/*
* PARAMETERS
*	p_mgr
*		Pointer to a port group manager to destroy.
*
* RETURN VALUE
*	None
*/

/****f* OpenSM: Port Group/osm_get_port
* NAME
*	osm_get_port
*
* DESCRIPTION
*	This function find and get a port in a port group by its port guid
*
* SYNOPSIS
*/
struct osm_port *osm_pgrp_get_port(IN osm_port_group_t * p_group,
				   IN ib_net64_t port_guid);
/*
* PARAMETERS
*	p_group
*		Pointer to a port group to search the port in.
*	port_guid
*		Port guid to search.
*
* RETURN VALUE
*	Pointer to the port if found or NULL if port not found.
*/

/****f* OpenSM: Port Group/osm_get_member
* NAME
*	osm_get_member
*
* DESCRIPTION
*	This function find and get a port group member from a group by its
*	port guid
*
* SYNOPSIS
*/
osm_port_group_member_t *osm_pgrp_get_member(IN osm_port_group_t * p_group,
					     IN ib_net64_t port_guid);
/*
* PARAMETERS
*	p_group
*		Pointer to a port group to search the port in.
*	port_guid
*		Port guid to search.
*
* RETURN VALUE
*	Pointer to the port group member if found or NULL if not found.
*/

/****f* OpenSM: Port Group/osm_get_port_grp
* NAME
*	osm_get_port_grp
*
* DESCRIPTION
*	This function find and get port group by its name.
*
* SYNOPSIS
*/
osm_port_group_t *osm_get_port_grp(IN struct osm_opensm *p_osm,
				   IN const char *group_name);
/*
* PARAMETERS
*	p_osm
*		Pointer to a open_sm in which to search for the port group.
*	group_name
*		name of port group to search.
*
* RETURN VALUE
*	Pointer to the port group if found or NULL if not found.
*/

/****f* OpenSM: Port Group/osm_physp_in_grp
* NAME
*	osm_physp_in_grp
*
* DESCRIPTION
*	This function find and get a physical port from a port group.
*
* SYNOPSIS
*/
boolean_t osm_physp_in_grp(IN osm_port_group_t * p_grp, IN ib_net64_t guid,
			   IN uint8_t port_num);
/*
* PARAMETERS
*	p_grp
*		Pointer to a port group to search the physical port in.
*	guid
*		Port guid to search.
*	port_num
*		Number of required physical port.
*
* RETURN VALUE
*	Pointer to physical port if found or NULL if not found.
*/

/****f* OpenSM: Port Group/osm_ports_grp_count
* NAME
*	osm_ports_grp_count
*
* DESCRIPTION
*	This function return the number of ports port groups in a port group
*	manager.
*
* SYNOPSIS
*/
uint32_t osm_ports_grp_count(IN struct osm_opensm *p_osm);
/*
* PARAMETERS
*	p_osm
*		Pointer to a open_sm in which its port groups manager groups
*		shall be counted
*
* RETURN VALUE
*	Number of groups in the port group manager.
*/

/************************************************************************
 * API for other modules who would like to add ports to port groups
 * If successful return pointer to the member that was added
 * if not return NULL
 ***********************************************************************/

/****f* OpenSM: Port Group/osm_set_port_in_grp
* NAME
*	osm_set_port_in_grp
*
* DESCRIPTION
*	This function set a port in a port group with required value (the
*	value is set on all physical ports of the port)
*
* SYNOPSIS
*/
boolean_t osm_set_port_in_grp(IN struct osm_subn *p_subn,
			      IN osm_port_group_t * p_grp,
			      IN struct osm_port *p_port, IN boolean_t val);
/*
* PARAMETERS
*	p_subn
*		Pointer to a osm_subn object.
*	p_grp
*		Pointer to the port group to which the port shall be added
*	p_port
*		Pointer to the port required to be added to the group
*	val
*		Value to set on all physical ports of the port.
*
* RETURN VALUE
*	True - Success
*	False - Failure
*
* NOTES
*	If port found in group its physical ports value shall be set, if not
*	found it shall be added.
*/

/****f* OpenSM: Port Group/osm_set_physp_in_grp_by_num
* NAME
*	osm_set_physp_in_grp_by_num
*
* DESCRIPTION
*	This function set a specific physical port in a port group with required
*	value.
*
* SYNOPSIS
*/
boolean_t osm_set_physp_in_grp_by_num(IN struct osm_subn *p_subn,
				      IN osm_port_group_t * p_grp,
				      IN struct osm_port *p_port,
				      IN uint8_t port_num, IN boolean_t val);
/*
* PARAMETERS
*	p_subn
*		Pointer to a osm_subn object.
*	p_grp
*		Pointer to the port group to which the port shall be added
*	p_port
*		Pointer to the port required to be added to the group
*	port_num
*		Physical port number to set.
*	val
*		Value to set.
*
* RETURN VALUE
*	True - Success
*	False - Failure
*
* NOTES
*	If port found in group its physical port value shall be set, if not
*	found it shall be added.
*/

/****f* OpenSM: Port Group/osm_set_physp_in_grp
* NAME
*	osm_set_physp_in_grp
*
* DESCRIPTION
*	This function set a specific physical port in a port group with required
*	value (same as osm_set_physp_in_grp_by_num but instead of getting port
*	and physical port number this method get the actual physical port pointer
*	and add it only if port is already in the group).
*
* SYNOPSIS
*/
boolean_t osm_set_physp_in_grp(IN struct osm_subn *p_subn,
			       IN osm_port_group_t * p_grp,
			       IN struct osm_physp *p_physp, IN boolean_t val);
/*
* PARAMETERS
*	p_subn
*		Pointer to a osm_subn object.
*	p_grp
*		Pointer to the port group to which the port shall be added
*	p_physp
*		Pointer to the physical port required to be added to the group
*	val
*		Value to set.
*
* RETURN VALUE
*	True - Success
*	False - Failure
*/

/****f* OpenSM: Port Group/osm_remove_port_from_grp
* NAME
*	osm_remove_port_from_grp
*
* DESCRIPTION
*	This function remove a port from port group.
*
* SYNOPSIS
*/
void osm_remove_port_from_grp(IN osm_port_group_t * p_grp,
			      IN struct osm_port *p_port);
/*
* PARAMETERS
*	p_grp
*		Pointer to the port group
*	p_port
*		Pointer to the port required to be removed
*
* RETURN VALUE
*	None
*/

/****f* OpenSM: Port Group/osm_destroy_port_grp
* NAME
*	osm_destroy_port_grp
*
* DESCRIPTION
*	This function destroys a port group.
*
* SYNOPSIS
*/
void osm_destroy_port_grp(osm_port_group_t * port_group);
/*
* PARAMETERS
*	port_group
*		Pointer to the port group
*
* RETURN VALUE
*	None
*/

/*****************************
 * API To Be Used By Parser
 ****************************/

/****f* OpenSM: Port Group/osm_create_port_grp_by_list
* NAME
*	osm_create_port_grp_by_list
*
* DESCRIPTION
*	Create new port group according to guid list rule
*
* SYNOPSIS
*/
void osm_create_port_grp_by_list(IN struct osm_opensm *p_osm, IN char *name,
				 IN char *use, IN char *list);
/*
* PARAMETERS
*	p_osm
*		Pointer to a osm_opensm object (to the port group mgr
*		on this opensm the group will be added)
*	name
*		Name of new group
*	use
*		Use comment for new group
*	list
*		guid list rule
*/

/****f* OpenSM: Port Group/osm_create_port_grp_by_hostname
* NAME
*	osm_create_port_grp_by_hostname
*
* DESCRIPTION
*	Create new port group according to hostname rule
*
* SYNOPSIS
*/
void osm_create_port_grp_by_hostname(IN struct osm_opensm *p_osm, IN char *name,
				     IN char *use, IN char *hostname);
/*
* PARAMETERS
*	p_osm
*		Pointer to a osm_opensm object (to the port group mgr
*		on this opensm the group will be added)
*	name
*		Name of new group
*	use
*		Use comment for new group
*	hostname
*		hostname rule
*/

/****f* OpenSM: Port Group/osm_create_port_grp_by_range
* NAME
*	osm_create_port_grp_by_range
*
* DESCRIPTION
*	Create new port group according to guid range rule
*
* SYNOPSIS
*/
void osm_create_port_grp_by_range(IN struct osm_opensm *p_osm, IN char *name,
				  IN char *use, IN char *mask, IN uint64_t min,
				  IN uint64_t max);
/*
* PARAMETERS
*	p_osm
*		Pointer to a osm_opensm object (to the port group mgr
*		on this opensm the group will be added)
*	name
*		Name of new group
*	use
*		Use comment for new group
*	mask
*		range mask (currently not used)
*	min
*		min guid in range
*	max
*		max guid in range
*/

/****f* OpenSM: Port Group/osm_create_port_grp_by_regex
* NAME
*	osm_create_port_grp_by_regex
*
* DESCRIPTION
*	Create new port group according to hostname regex rule
*
* SYNOPSIS
*/
void osm_create_port_grp_by_regex(IN struct osm_opensm *p_osm, IN char *name,
				  IN char *use, IN char *regex,
				  IN uint8_t port_num);
/*
* PARAMETERS
*	p_osm
*		Pointer to a osm_opensm object (to the port group mgr
*		on this opensm the group will be added)
*	name
*		Name of new group
*	use
*		Use comment for new group
*	regex
*		hostname regex rule
*	port_num
*		physical port number to add to the group (0 = ALL)
*/

/****f* OpenSM: Port Group/osm_create_port_grp_by_union
* NAME
*	osm_create_port_grp_by_union
*
* DESCRIPTION
*	Create new port group according to groups union rule
*
* SYNOPSIS
*/
void osm_create_port_grp_by_union(IN struct osm_opensm *p_osm, IN char *name,
				  IN char *use, IN char *first_group,
				  IN char *second_group);
/*
* PARAMETERS
*	p_osm
*		Pointer to a osm_opensm object (to the port group mgr
*		on this opensm the group will be added)
*	name
*		Name of new group
*	use
*		Use comment for new group
*	first_group
*		name of first group to unite
*	second_group
*		name of second group to unite
*/

/****f* OpenSM: Port Group/osm_create_port_grp_by_subtract
* NAME
*	osm_create_port_grp_by_subtract
*
* DESCRIPTION
*	Create new port group according to groups subtract rule
*
* SYNOPSIS
*/
void osm_create_port_grp_by_subtract(IN struct osm_opensm *p_osm, IN char *name,
				     IN char *use, IN char *included_group,
				     IN char *subtracted_group);
/*
* PARAMETERS
*	p_osm
*		Pointer to a osm_opensm object (to the port group mgr
*		on this opensm the group will be added)
*	name
*		Name of new group
*	use
*		Use comment for new group
*	included_group
*		name of group to be included in new group
*	subtracted_group
*		name of group to subtract from new group
*/

/****f* OpenSM: Port Group/osm_create_empty_grp
* NAME
*	osm_create_empty_grp
*
* DESCRIPTION
*	Create new empty port group without using any rule
*
* SYNOPSIS
*/
osm_port_group_t *create_empty_grp(IN struct osm_opensm *p_osm, IN char *name);
/*
* PARAMETERS
*	p_osm
*		Pointer to a osm_opensm object (to the port group mgr
*		on this opensm the group will be added)
*	name
*		Name of new group
*/

/****f* OpenSM: Port Group/osm_create_port_grp_member
 *
* NAME
*	create_port_group_member
*
* DESCRIPTION
*	Create port group memeber
*
* SYNOPSIS
*/
osm_port_group_member_t *osm_create_port_grp_member(struct osm_opensm * p_osm,
						    struct osm_port * p_port,
						    boolean_t
						    physp_def_val);
/*
* PARAMETERS
*	p_osm
*		Pointer to a osm_opensm object
*	p_port
*		Pointer to osm_port object
*	physp_def_val
*		Default Value to set in all physps
*
* RETURN VALUE
*	Pointer to new port group member
*/

/****f* OpenSM: Port Group/osm_destroy_port_grp_member
 *
* NAME
*	osm_destroy_port_grp_member
*
* DESCRIPTION
*	Destroy port group memeber
*
* SYNOPSIS
*/
void osm_destroy_port_grp_member(osm_port_group_member_t * grp_mem);
/*
* PARAMETERS
*	grp_mem
*		Pointer to group member to destroy
*
* RETURN VALUE
*	none
*/

/****f* OpenSM: Port Group/osm_set_physp_reliable
 *
* NAME
*	osm_set_physp_reliable
*
* DESCRIPTION
*	Set specific port number value inside a port group memeber
*
* SYNOPSIS
*/
boolean_t osm_set_physp_reliable(struct osm_opensm * p_osm,
				 osm_port_group_member_t * p_member,
				 uint8_t port_num, boolean_t val);
/*
* PARAMETERS
*	p_osm
*		Pointer to a osm_opensm object
*	p_member
*		Pointer to group member object
*	port_num
*		Port number to set
*	val
*		Value to set
*
* RETURN VALUE
*	Boolean success/fail indication
*/
#endif
