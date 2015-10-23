/*
 * Copyright (c) 2004-2012 Mellanox Technologies LTD. All rights reserved.
 *
 * This software is available to you under the terms of the
 * OpenIB.org BSD license included below:
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


#ifndef IBDIAG_H
#define IBDIAG_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string>
#include <list>
#include <map>
using namespace std;

#include <infiniband/ibdm/Fabric.h>
#include <infiniband/ibis/ibis.h>

#include "ibdiag_ibdm_extended_info.h"
#include "ibdiag_fabric_errs.h"
#include "ibdiag_types.h"

#define CHECK_EXT_SPEEDS_COUNTERS        0x01
#define CHECK_EXT_SPEEDS_COUNTERS_ON_HCA 0x02
#define PRINT_LLR_COUNTERS               0x04

/****************************************************/
string get_ibdiag_version();


/*****************************************************/
class IBDiag {
private:
    ////////////////////
    //members
    ////////////////////
    IBFabric discovered_fabric;
    Ibis ibis_obj;
    IBDMExtendedInfo fabric_extended_info;

    enum {NOT_INITILIAZED, NOT_SET_PORT, READY} ibdiag_status;
    enum {DISCOVERY_SUCCESS, DISCOVERY_NOT_DONE, DISCOVERY_DUPLICATED_GUIDS} ibdiag_discovery_status;
    string last_error;
    bool check_duplicated_guids;

    ////////////////////
    //discovery members
    ////////////////////
    list_p_direct_route bfs_list;               //this list supposed to be empty at the end of discovery
    list_p_direct_route good_direct_routes;
    list_p_bad_direct_route bad_direct_routes;
    list_p_direct_route loop_direct_routes;
    list_string duplicated_guids_detection_errs;
    list_p_fabric_general_err errors; //MAYDO replace separete error per RetrieceInfo

    map_guid_list_p_direct_route bfs_known_node_guids;
    map_guid_list_p_direct_route bfs_known_port_guids;

    IBNode * root_node;
    u_int8_t root_port_num;
    progress_bar_nodes_t discover_progress_bar_nodes;
    progress_bar_ports_t discover_progress_bar_ports;

    bool no_mepi; // do not send SMPMlnxExtPortInfoMad
    u_int8_t llr_active_cell_size; //active LLR given by user

    ////////////////////
    //methods
    ////////////////////
    void SetLastError(const char *fmt, ...);
    void CleanUpInternalDB();

    // Returns: SUCCESS_CODE / ERR_CODE_DB_ERR
    int GetReverseDirectRoute(INOUT direct_route_t *p_reverse_direct_route, IN direct_route_t *p_direct_route);

    // Returns: SUCCESS_CODE / ERR_CODE_EXCEEDS_MAX_HOPS
    int ConcatDirectRoutes(IN direct_route_t *p_direct_route1,
            IN direct_route_t *p_direct_route2,
            OUT direct_route_t *p_direct_route_result);

    //Returns: SUCCESS_CODE / ERR_CODE_IO_ERR
    int OpenFile(const char *file_name, ofstream& sout, bool to_append = false, bool add_header = false);

    ////////////////////
    //duplicated guids  methods
    ////////////////////
    void AddDupGUIDDetectError(IN direct_route_t *p_direct_route_checked_node,
            IN u_int64_t checked_node_guid,
            IN u_int8_t checked_node_type,
            IN direct_route_t *p_direct_route_got_err,
            IN bool no_response_err,
            IN bool max_hops_err,
            IN string err_desc);

    // Returns: SUCCESS_CODE / ERR_CODE_DB_ERR / ERR_CODE_DUPLICATED_GUID / ERR_CODE_FABRIC_ERROR
    int CheckIfSameCADevice(IN direct_route_t *p_new_direct_route,
            IN direct_route_t *p_old_direct_route,
            IN struct SMP_NodeInfo *p_new_node_info,
            OUT IbdiagBadDirectRoute_t *p_bad_direct_route_info);

    // Returns: SUCCESS_CODE / ERR_CODE_DB_ERR / ERR_CODE_DUPLICATED_GUID / ERR_CODE_FABRIC_ERROR / ERR_CODE_EXCEEDS_MAX_HOPS
    int CheckIfSameSWDevice(IN direct_route_t *p_new_direct_route,
            IN direct_route_t *p_old_direct_route,
            IN struct SMP_NodeInfo *p_new_node_info,
            OUT IbdiagBadDirectRoute_t *p_bad_direct_route_info);

    // Returns: SUCCESS_CODE / ERR_CODE_DB_ERR
    int IsDuplicatedGuids(IN direct_route_t *p_new_direct_route,
            IN struct SMP_NodeInfo *p_new_node_info,
            OUT bool *duplicated_node_guid,
            OUT bool *duplicated_port_guid,
            OUT bool *is_visited_node_already,
            OUT bool *is_visited_port_already,
            OUT direct_route_t **p_old_direct_route,
            OUT IbdiagBadDirectRoute_t *p_bad_direct_route_info);

    ////////////////////
    //discovery methods
    ////////////////////
    inline bool IsBFSKnownPortGuid(IN u_int64_t guid);
    inline bool IsBFSKnownNodeGuid(IN u_int64_t guid);
    inline void MarkNodeGuidAsBFSKnown(IN u_int64_t guid, IN direct_route_t *p_direct_route);
    inline void MarkPortGuidAsBFSKnown(IN u_int64_t guid, IN direct_route_t *p_direct_route);

    inline list_p_direct_route& GetDirectRoutesByNodeGuid(IN u_int64_t guid) { return this->bfs_known_node_guids[guid]; }
    inline list_p_direct_route& GetDirectRoutesByPortGuid(IN u_int64_t guid) { return this->bfs_known_port_guids[guid]; }

    inline void BFSPushPath(IN direct_route_t *p_direct_route);
    inline direct_route_t * BFSPopPath();

    inline void AddGoodPath(IN direct_route_t *p_direct_route);
    inline int  AddBadPath(IN IbdiagBadDirectRoute_t *bad_direct_route, IN direct_route_t *direct_route);
    inline void AddLoopPath(IN direct_route_t *p_direct_route);

    void PostDiscoverFabricProcess();

    // Returns: SUCCESS_CODE / ERR_CODE_INCORRECT_ARGS / ERR_CODE_NO_MEM
    int QueryForMlnxExtSpeedAndLLRActive(u_int8_t port_phy_state,
                                         u_int16_t dev_id,
                                         direct_route_t *p_direct_route,
                                         IBPort *p_port,
                                         bool speed_query);

    // Returns: SUCCESS_CODE / ERR_CODE_FABRIC_ERROR / ERR_CODE_IBDM_ERR / ERR_CODE_INCORRECT_ARGS / ERR_CODE_NO_MEM
    int DiscoverFabricOpenCAPorts(IN IBNode *p_node,
            IN direct_route_t *p_direct_route,
            IN SMP_NodeInfo *p_node_info,
            IN bool is_root,
            OUT IbdiagBadDirectRoute_t *p_bad_direct_route_info);
    int DiscoverFabricOpenSWPorts(IN IBNode *p_node,
            IN direct_route_t *p_direct_route,
            IN SMP_NodeInfo *p_node_info,
            IN bool is_root,
            OUT IbdiagBadDirectRoute_t *p_bad_direct_route_info);
    int DiscoverFabricBFSOpenPorts(IN direct_route_t * p_direct_route,
            IN IBNode *p_node,
            IN struct SMP_NodeInfo *p_node_info,
            IN bool is_visited_node,
            IN bool is_root,
            OUT IbdiagBadDirectRoute_t *p_bad_direct_route_info);

    // Returns: SUCCESS_CODE / ERR_CODE_DB_ERR / ERR_CODE_IBDM_ERR / ERR_CODE_TRY_TO_DISCONNECT_CONNECTED_PORT
    int DiscoverFabricBFSCreateLink(IN direct_route_t * p_direct_route,
            IN IBPort * p_port);

    // Returns: SUCCESS_CODE / ERR_CODE_FABRIC_ERROR / ERR_CODE_IBDM_ERR / ERR_CODE_INCORRECT_ARGS / ERR_CODE_NO_MEM
    int DiscoverFabricBFSOpenNode(IN direct_route_t *p_direct_route,
            IN bool is_root,
            OUT IBNode **p_pnode,
            OUT struct SMP_NodeInfo *p_node_info,
            OUT bool *is_visited_node,
            IN progress_func_discovered_t discover_progress_func,
            OUT IbdiagBadDirectRoute_t *p_bad_direct_route_info);

    bool IsValidNodeInfoData(IN struct SMP_NodeInfo *p_node_info,
                             OUT string &additional_info);

    ////////////////////
    //db file methods
    ////////////////////
    void DumpCSVNodesTable(ofstream &sout);

    //Returns: SUCCESS_CODE / ERR_CODE_DB_ERR
    int DumpCSVPortsTable(ofstream &sout);
    int DumpCSVLinksTable(ofstream &sout);

    ////////////////////
    //pm methods
    ////////////////////
    void ResetAppData();

    void DumpPortCounters(ofstream &sout,
                          u_int32_t check_counters_bitset,
                          bool en_per_lane_cnts);
    int ReadCapMask(list_p_fabric_general_err& pm_errors,
                    IBNode *p_node, IBPort *p_port,
                    u_int16_t *&p_cap_mask, u_int32_t *&p_port_info_cap_mask);

    ////////////////////
    //sm methods
    ////////////////////
    void DumpSMInfo(ofstream &sout);

    ////////////////////
    // vs methods
    ////////////////////
    void DumpNodesInfo(ofstream &sout);

    //Returns: SUCCESS_CODE / IBDIAG_ERR_CODE_FABRIC_ERROR
    int CheckVSGeneralInfo(IBNode *p_curr_node, struct VendorSpec_GeneralInfo *p_curr_general_info);

    ////////////////////
    //routing methods
    ////////////////////
    int ReportNonUpDownCa2CaPaths(IBFabric *p_fabric, list_pnode rootNodes, string& output);

    //Returns: SUCCESS_CODE / ERR_CODE_DB_ERR
    int DumpUCFDBSInfo(ofstream &sout);
    int DumpMCFDBSInfo(ofstream &sout);
    int DumpSLVLFile(ofstream &sout,
                     list_p_fabric_general_err& retrieve_errors,
                     progress_func_nodes_t progress_func);
    ////////////////////
    ////pkey methods
    ////////////////////
    void DumpPartitionKeys(ofstream &sout);

    ////////////////////
    ////aguid methods
    ////////////////////
    void DumpAliasGUID(ofstream &sout);


public:
    ////////////////////
    //methods
    ////////////////////
    IBDiag();
    ~IBDiag();

    inline string ConvertDirPathToStr(direct_route_t *p_direct_route) { return (this->ibis_obj.ConvertDirPathToStr(p_direct_route)); }

    inline bool IsInit() { return (this->ibdiag_status != NOT_INITILIAZED); };
    inline bool IsReady() { return (this->ibdiag_status == READY); };
    inline bool IsDiscoveryDone() { return this->ibdiag_discovery_status == DISCOVERY_SUCCESS; }
    inline bool IsDuplicatedGuidsFound() { return this->ibdiag_discovery_status == DISCOVERY_DUPLICATED_GUIDS; }

    list_p_fabric_general_err &GetErrors() {return errors;}

    inline IBFabric * GetDiscoverFabricPtr() { return &(this->discovered_fabric); }
    inline Ibis * GetIbisPtr() { return &(this->ibis_obj); }

    inline progress_bar_nodes_t * GetDiscoverProgressBarNodesPtr() { return &this->discover_progress_bar_nodes; }
    inline progress_bar_ports_t * GetDiscoverProgressBarPortsPtr() { return &this->discover_progress_bar_ports; }

    inline list_string& GetRefToDupGuidsDetectionErrors() {return this->duplicated_guids_detection_errs; }
    inline bool& GetRefToCheckDupGuids() { return this->check_duplicated_guids; }

    const char* GetLastError();

    // Returns: SUCCESS_CODE / ERR_CODE_INIT_FAILED
    int Init();
    int SetPort(const char* device_name, phys_port_t port_num);
    int SetPort(u_int64_t port_guid);

    // Returns: SUCCESS_CODE / ERR_CODE_DB_ERR
    int GetLocalPortState(OUT u_int8_t& state);

    IBNode * GetNodeByDirectRoute(IN const direct_route_t *p_direct_route);
    direct_route_t * GetDirectRouteByNodeGuid(IN u_int64_t guid);
    direct_route_t * GetDirectRouteByPortGuid(IN u_int64_t guid);
    IBPort * GetRootPort();
    int GetAllLocalPortGUIDs(OUT local_port_t local_ports_array[IBIS_MAX_LOCAL_PORTS],
            OUT u_int32_t *p_local_ports_num);

    ////////////////////
    //duplicated guids methods
    ////////////////////
    //Returns: SUCCESS_CODE / ERR_CODE_CHECK_FAILED / ERR_CODE_NO_MEM / ERR_CODE_DB_ERR
    int CheckDuplicatedGUIDs(list_p_fabric_general_err& guids_errors);

    // Returns: SUCCESS_CODE / ERR_CODE_DB_ERR
    int PrintNodesDuplicatedGuids();
    int PrintPortsDuplicatedGuids();

    void PrintDupGuidsDetectionErrors();

    ////////////////////
    //discovery methods
    ////////////////////
    void GetGoodDirectRoutes(OUT list_string& good_direct_routes);
    void GetLoopDirectRoutes(OUT list_string& loop_direct_routes);
    void GetBadDirectRoutes(OUT list_string& bad_direct_routes);
    void PrintAllRoutes();

    // Returns: SUCCESS_CODE / ERR_CODE_FABRIC_ERROR / ERR_CODE_IBDM_ERR / ERR_CODE_INCORRECT_ARGS / ERR_CODE_NO_MEM / ERR_CODE_DB_ERR / ERR_CODE_TRY_TO_DISCONNECT_CONNECTED_PORT / ERR_CODE_INCORRECT_ARGS
    int DiscoverFabric(IN progress_func_discovered_t discover_progress_func = NULL,
            IN u_int8_t max_hops = IBDIAG_MAX_HOPS);
    //Returns: SUCCESS_CODE / ERR_CODE_DISCOVERY_NOT_SUCCESS / ERR_CODE_DB_ERR / ERR_CODE_NO_MEM / ERR_CODE_FABRIC_ERROR
    int BuildSwitchInfoDB(list_p_fabric_general_err& retrieve_errors, progress_func_nodes_t progress_func);

    int ParsePSLFile(const string & file_name, string& output);

    inline void SetNoMEPI() {
        this->no_mepi = true;
    }

    inline void SetLLRActiveCell(const string &active_cell_size) {
        unsigned long int cell_size = strtoul(active_cell_size.c_str(), NULL, 10);

        switch (cell_size) {
        case 128:
            this->llr_active_cell_size = LLR_ACTIVE_CELL_128;
            break;
        case 64: default:
            this->llr_active_cell_size = LLR_ACTIVE_CELL_64;
            break;
        }
    }

    ////////////////////
    //checks methods
    ////////////////////
    //Return: SUCCESS_CODE / ERR_CODE_DISCOVERY_NOT_SUCCESS / ERR_CODE_INCORRECT_ARGS / ERR_CODE_NO_MEM / ERR_CODE_CHECK_FAILED / ERR_CODE_DB_ERR
    int CheckLinkWidth(list_p_fabric_general_err& width_errors, string expected_link_width_str = "");
    int CheckLinkSpeed(list_p_fabric_general_err& speed_errors, string expected_link_speed = "");

    //Return: SUCCESS_CODE / ERR_CODE_DISCOVERY_NOT_SUCCESS / ERR_CODE_NO_MEM / ERR_CODE_CHECK_FAILED
    int CheckDuplicatedNodeDescription(list_p_fabric_general_err& nodes_errors);
    int CheckLinks(list_p_fabric_general_err& links_errors);
    int CheckLids(list_p_fabric_general_err& lids_errors);

    ////////////////////
    //db file methods
    ////////////////////
    //Returns: SUCCESS_CODE / ERR_CODE_DISCOVERY_NOT_SUCCESS / ERR_CODE_IO_ERR / ERR_CODE_DB_ERR
    int DumpInternalDBCSVTable(ofstream& sout);

    //Returns: SUCCESS_CODE / ERR_CODE_DISCOVERY_NOT_SUCCESS / ERR_CODE_IBDM_ERR
    int WriteLSTFile(const char *file_path);

    ////////////////////
    //pm methods
    ////////////////////
    void CopyPMInfoObjVector(OUT vector_p_pm_info_obj& new_pm_obj_info_vector);
    void CleanPMInfoObjVector(IN vector_p_pm_info_obj& curr_pm_obj_info_vector);

    //Returns: SUCCESS_CODE / ERR_CODE_DISCOVERY_NOT_SUCCESS / ERR_CODE_DB_ERR / ERR_CODE_NO_MEM / ERR_CODE_FABRIC_ERROR
    int BuildClassPortInfoDB(list_p_fabric_general_err& pm_errors);

    //Returns: SUCCESS_CODE / ERR_CODE_DISCOVERY_NOT_SUCCESS / ERR_CODE_DB_ERR / ERR_CODE_NO_MEM / ERR_CODE_FABRIC_ERROR
    int BuildPortCountersDB(list_p_fabric_general_err& pm_errors,
                            progress_func_nodes_t progress_func,
                            u_int32_t check_counters_bitset);
    int ResetPortCounters(list_p_fabric_general_err& ports_errors,
                          progress_func_nodes_t progress_func,
                          u_int32_t check_counters_bitset);

    //Returns: SUCCESS_CODE / ERR_CODE_DISCOVERY_NOT_SUCCESS
    int DumpPortCountersCSVTable(ofstream &sout, u_int32_t check_counters_bitset);

    //Returns: SUCCESS_CODE / IBDIAG_ERR_CODE_NO_MEM / ERR_CODE_INCORRECT_ARGS / IBDIAG_ERR_CODE_CHECK_FAILED
    int CalcCounters(vector_p_pm_info_obj& prev_pm_info_obj_vec,
                     double diff_time_between_samples,
                     list_p_fabric_general_err& pm_errors);

    //Returns: SUCCESS_CODE / IBDIAG_ERR_CODE_DB_ERR / ERR_CODE_NO_MEM / ERR_CODE_CHECK_FAILED / ERR_CODE_INCORRECT_ARGS
    int CheckAllPMValues(list_p_fabric_general_err& pm_errors,
                         map_str_uint64& counters_to_threshold_map,
                         u_int32_t check_counters_bitset);

    //Returns: SUCCESS_CODE / ERR_CODE_NO_MEM / ERR_CODE_CHECK_FAILED / ERR_CODE_INCORRECT_ARGS
    int CheckCountersDiff(vector_p_pm_info_obj& prev_pm_info_obj_vec,
            list_p_fabric_general_err& pm_errors);

    // return IBDIAG_SUCCESS_CODE
    int CalcLLRBER(vector_p_pm_info_obj& prev_pm_info_obj_vec,
                   u_int32_t curr_indx,
                   IBPort *p_curr_port,
                   u_int8_t active_cell,
                   long double &ber_value_reciprocal_val);

    int CalcBER(vector_p_pm_info_obj& prev_pm_info_obj_vec,
                u_int32_t curr_indx,
                IBPort *p_curr_port,
                double sec_between_samples,
                list_p_fabric_general_err& ber_errors,
                long double &ber_value_reciprocal_val);

    int CalcBERErrors(vector_p_pm_info_obj& prev_pm_info_obj_vec,
            u_int64_t ber_threshold_opposite_val,
            double sec_between_samples,
            list_p_fabric_general_err& ber_errors);

    list_string GetListOFPMNames();

    //Returns: SUCCESS_CODE / ERR_CODE_DISCOVERY_NOT_SUCCESS / ERR_CODE_IO_ERR
    int WritePMFile(const char *file_name,
                    u_int32_t check_counters_bitset,
                    bool en_per_lane_cnts);

    ////////////////////
    //sm methods
    ////////////////////
    //Returns: SUCCESS_CODE / ERR_CODE_DISCOVERY_NOT_SUCCESS / ERR_CODE_DB_ERR / ERR_CODE_NO_MEM / ERR_CODE_FABRIC_ERROR
    int BuildSMInfoDB(list_p_fabric_general_err& ports_errors);

    //Returns: SUCCESS_CODE / ERR_CODE_DISCOVERY_NOT_SUCCESS / ERR_CODE_NO_MEM / ERR_CODE_CHECK_FAILED
    int CheckSMInfo(list_p_fabric_general_err& sm_errors);

    //Returns: SUCCESS_CODE / ERR_CODE_DISCOVERY_NOT_SUCCESS / ERR_CODE_IO_ERR
    int WriteSMFile(const char *file_name);

    //Returns: SUCCESS_CODE / ERR_CODE_DISCOVERY_NOT_SUCCESS
    int DumpSMInfoCSVTable(ofstream &sout);

    ////////////////////
    //vs methods
    ////////////////////
    //Returns: SUCCESS_CODE / ERR_CODE_DISCOVERY_NOT_SUCCESS / ERR_CODE_DB_ERR / ERR_CODE_NO_MEM / ERR_CODE_FABRIC_ERROR
    int BuildNodeInfoDB(list_p_fabric_general_err& nodes_errors, progress_func_nodes_t progress_func);

    //Returns: SUCCESS_CODE / ERR_CODE_DISCOVERY_NOT_SUCCESS / ERR_CODE_IO_ERR
    int WriteNodesInfoFile(const char *file_name);

    //Returns: SUCCESS_CODE / ERR_CODE_DISCOVERY_NOT_SUCCESS
    int DumpNodesInfoCSVTable(ofstream &sout);

    //Returns: SUCCESS_CODE / ERR_CODE_DISCOVERY_NOT_SUCCESS / ERR_CODE_DB_ERR / ERR_CODE_CHECK_FAILED
    int CheckFWVersion(list_p_fabric_general_err& fw_errors);

    ////////////////////
    //routing methods
    ////////////////////
    //Returns: SUCCESS_CODE / ERR_CODE_DISCOVERY_NOT_SUCCESS / ERR_CODE_DB_ERR / ERR_CODE_NO_MEM / ERR_CODE_FABRIC_ERROR
    int RetrieveUCFDBSInfo(list_p_fabric_general_err& retrieve_errors, progress_func_nodes_t progress_func);
    int RetrieveMCFDBSInfo(list_p_fabric_general_err& retrieve_errors, progress_func_nodes_t progress_func);

    //Returns: SUCCESS_CODE / ERR_CODE_DISCOVERY_NOT_SUCCESS / ERR_CODE_IO_ERR / ERR_CODE_DB_ERR
    int WriteUCFDBSFile(const char *file_name);
    int WriteMCFDBSFile(const char *file_name);
    int WriteSLVLFile(const char *file_name,
                      list_p_fabric_general_err& retrieve_errors,
                      progress_func_nodes_t progress_func);
    int ParseSLVLFile(const string & file_name, string& output);


    //Returns: SUCCESS_CODE / ERR_CODE_IBDM_ERR / ERR_CODE_DISCOVERY_NOT_SUCCESS
    int ReportFabricQualities(string& output, const char *outDir);
    int ReportCreditLoops(string& output, bool is_fat_tree);

        ////////////////////
        ////pkey methods
        ////////////////////
    //Returns: SUCCESS_CODE / ERR_CODE_DISCOVERY_NOT_SUCCESS / ERR_CODE_DB_ERR / ERR_CODE_NO_MEM / ERR_CODE_FABRIC_ERROR
    int BuildPartitionKeysDB(list_p_fabric_general_err &pkey_errors,
            progress_func_nodes_t progress_func);

    //Returns: SUCCESS_CODE / ERR_CODE_NO_MEM / ERR_CODE_CHECK_FAILED
    int CheckPartitionKeys(list_p_fabric_general_err& pkey_errors);

    //Returns: SUCCESS_CODE / ERR_CODE_DISCOVERY_NOT_SUCCESS / ERR_CODE_IO_ERR
    int WritePKeyFile(const char *file_name);

    //Returns: SUCCESS_CODE / ERR_CODE_DISCOVERY_NOT_SUCCESS
    int DumpPartitionKeysCSVTable(ofstream &sout);

        ////////////////////
        ////aguid methods
        ////////////////////
    //Returns: SUCCESS_CODE / ERR_CODE_DISCOVERY_NOT_SUCCESS / ERR_CODE_DB_ERR / ERR_CODE_NO_MEM / ERR_CODE_FABRIC_ERROR
    int BuildAliasGuidsDB(list_p_fabric_general_err &aguid_errors,
            progress_func_nodes_t progress_func);

    //Returns: SUCCESS_CODE / ERR_CODE_NO_MEM / ERR_CODE_CHECK_FAILED
    int CheckDuplicatedAliasGuids(list_p_fabric_general_err& aguid_errors);

    //Returns: SUCCESS_CODE / ERR_CODE_DISCOVERY_NOT_SUCCESS / ERR_CODE_IO_ERR
    int WriteAliasGUIDFile(const char *file_name);

    //Returns: SUCCESS_CODE / ERR_CODE_DISCOVERY_NOT_SUCCESS
    int DumpAliasGUIDCSVTable(ofstream &sout);

};


#endif          /* IBDIAG_H */

