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


#ifndef IBIS_H_
#define IBIS_H_


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string>
#include <vector>
#include <map>
using namespace std;

#include "ibis_types.h"
#include "infiniband/ibutils/memory_pool.h"

#define IB_SLT_UNASSIGNED 255

struct node_addr_t {
    direct_route_t m_direct_route;
    u_int16_t      m_lid;
    bool operator < (const node_addr_t& y) const
    {
        if (m_lid != y.m_lid)
            return m_lid < y.m_lid;
        if (m_direct_route.length != y.m_direct_route.length)
            return m_direct_route.length < y.m_direct_route.length;
        return memcmp(m_direct_route.path.BYTE, y.m_direct_route.path.BYTE,
                      m_direct_route.length) < 0;
    }
};

/*
struct less_node_addr {
  bool operator() (const node_addr_t* x, const node_addr_t* y) const
  {
      if (x->m_lid != y->m_lid)
          return x->m_lid < y->m_lid;
      if (x->m_direct_route.length != y->m_direct_route.length)
          return x->m_direct_route.length < y->m_direct_route.length;
      return memcmp(x->m_direct_route.path.BYTE, y->m_direct_route.path.BYTE,
                    x->m_direct_route.length) < 0;
  }
};
*/

struct transaction_data_t;

struct pending_mad_data_t {
    u_int8_t* m_umad;
    unsigned int m_umad_size;
    u_int8_t m_mgmt_class;
    transaction_data_t * m_transaction_data;

    pending_mad_data_t():m_umad(NULL),m_transaction_data(NULL) {}

    ~pending_mad_data_t() {
        delete[] m_umad;
    }

    int init();
};

typedef list<pending_mad_data_t *>  pending_mads_on_node_t;

struct transaction_data_t {
    u_int8_t           m_data_offset;
    unpack_data_func_t m_unpack_class_data_func;
    dump_data_func_t   m_dump_class_data_func;
    unpack_data_func_t m_unpack_attribute_data_func;
    dump_data_func_t   m_dump_attribute_data_func;
    bool               m_is_smp;
    clbck_data_t       m_clbck_data;
    pending_mads_on_node_t *m_pending_mads;

    transaction_data_t() :
            m_data_offset(0),
            m_unpack_class_data_func(NULL),
            m_dump_class_data_func(NULL),
            m_unpack_attribute_data_func(NULL),
            m_dump_attribute_data_func(NULL),
            m_is_smp(false),
            m_pending_mads(NULL) {}

    int init(){return 0;} //for MemoryPool usage

    void Set(u_int8_t data_offset,
             const unpack_data_func_t unpack_class_data_func,
             const dump_data_func_t   dump_class_data_func,
             const unpack_data_func_t unpack_attribute_data_func,
             const dump_data_func_t   dump_attribute_data_func,
             bool is_smp,
             const clbck_data_t &clbck_data,
             pending_mads_on_node_t *pending_mads)
    {
        m_data_offset = data_offset;
        m_unpack_class_data_func = unpack_class_data_func;
        m_dump_class_data_func = dump_class_data_func;
        m_unpack_attribute_data_func = unpack_attribute_data_func;
        m_dump_attribute_data_func = dump_attribute_data_func;
        m_is_smp = is_smp;
        m_clbck_data = clbck_data;
        m_pending_mads = pending_mads;
    }



    /*
    transaction_data_t(
        u_int8_t data_offset,
        const unpack_data_func_t unpack_class_data_func,
        const dump_data_func_t   dump_class_data_func,
        const unpack_data_func_t unpack_attribute_data_func,
        const dump_data_func_t   dump_attribute_data_func,
        bool is_smp,
        const clbck_data_t &clbck_data,
        pending_mads_on_node_t *pending_mads) :
            m_data_offset(data_offset),
            m_unpack_class_data_func(unpack_class_data_func),
            m_dump_class_data_func(dump_class_data_func),
            m_unpack_attribute_data_func(unpack_attribute_data_func),
            m_dump_attribute_data_func(dump_attribute_data_func),
            m_is_smp(is_smp),
            m_clbck_data(clbck_data),
            m_pending_mads(pending_mads) {}
            */

    /*
    transaction_data_t& operator=(const transaction_data_t& transaction_data)
    {
        m_data_offset = transaction_data.m_data_offset;
        m_unpack_class_data_func = transaction_data.m_unpack_class_data_func;
        m_dump_class_data_func = transaction_data.m_dump_class_data_func;
        m_unpack_attribute_data_func = transaction_data.m_unpack_attribute_data_func;
        m_dump_attribute_data_func = transaction_data.m_dump_attribute_data_func;
        m_is_smp = transaction_data.m_is_smp;
        m_handle_data_func = transaction_data.m_handle_data_func;
        m_p_obj = transaction_data.m_p_obj;
        m_p_key = transaction_data.m_p_key;
        return *this;
    }
    */

};

typedef map < u_int32_t, transaction_data_t *>  transactions_map_t;
typedef map < node_addr_t, pending_mads_on_node_t >  mads_on_node_map_t;

/****************************************************/
const char * get_ibis_version();


/****************************************************/
class Ibis {
private:
    ////////////////////
    //members
    ////////////////////
    string dev_name;
    phys_port_t port_num;

    enum {NOT_INITILIAZED, NOT_SET_PORT, READY} ibis_status;
    string last_error;

    void *p_umad_buffer_send;       /* buffer for send mad - sync buffer */
    void *p_umad_buffer_recv;       /* buffer for recv mad - sync buffer */
    u_int8_t *p_pkt_send;           /* mad pkt to send */
    u_int8_t *p_pkt_recv;           /* mad pkt was received */
    u_int64_t mads_counter;         /* number of mads we sent already */

    int umad_port_id;									/* file descriptor returned by umad_open() */
    int umad_agents_by_class[IBIS_IB_MAX_MAD_CLASSES];  /* array to map class --> agent */
    int timeout;
    int retries;

    vector<uint8_t > PSL;          /* PSL table (dlid->SL mapping) of this node */
    bool usePSL;

    //asynchronouse invocation
    MemoryPool<transaction_data_t> m_transaction_data_pool;
    transactions_map_t transactions_map;
    u_int32_t m_pending_gmps;
    u_int32_t m_pending_smps;
    u_int32_t m_max_gmps_on_wire;
    u_int32_t m_max_smps_on_wire;

    MemoryPool<pending_mad_data_t> m_pending_mads_pool;
    mads_on_node_map_t      m_mads_on_node_map;

    ////////////////////
    //methods
    ////////////////////
    void SetLastError(const char *fmt, ...);
    bool IsLegalMgmtClass(int mgmt_class);

    // Returns: 0 - success / 1 - error
    int Bind();
    int RecvMad(int mgmt_class, int umad_timeout);
    int SendMad(int mgmt_class, int umad_timeout, int umad_retries);
    int RecvAsyncMad(int umad_timeout);

    int AsyncRec(bool &retry, pending_mad_data_t *&next_pending_mad_data);
    void MadRecTimeoutAll();
    void TimeoutAllPendingMads();

    /*
     * Returns: mad status[bits: 0-15] / IBIS_MAD_STATUS_SEND_FAILED / IBIS_MAD_STATUS_RECV_FAILED /
     *  IBIS_MAD_STATUS_TIMEOUT / IBIS_MAD_STATUS_GENERAL_ERR / IBIS_MAD_STATUS_SUCCESS
     */
    int DoRPC(int mgmt_class);

    /*
     * Returns: mad status[bits: 0-15] / IBIS_MAD_STATUS_SEND_FAILED / IBIS_MAD_STATUS_GENERAL_ERR
     *  IBIS_MAD_STATUS_SUCCESS
     */
    int DoAsyncSend(int mgmt_class);

    /*
     * Returns: mad status[bits: 0-15] / IBIS_MAD_STATUS_RECV_FAILED /
     *  IBIS_MAD_STATUS_TIMEOUT / IBIS_MAD_STATUS_GENERAL_ERR / IBIS_MAD_STATUS_SUCCESS
     */
    int DoAsyncRec();

    /*
     * remove the current pending mad data from the list and free it
     * set next_pending_mad_data to the next element in the list.
     *
     * Returns: IBIS_MAD_STATUS_GENERAL_ERR / IBIS_MAD_STATUS_SUCCESS
     */
    int GetNextPendingData(transaction_data_t * p_transaction_data,
                           pending_mad_data_t *&next_pending_mad_data);

    /*
     * Send the current mad
     * May recive mad and send pending mad on the received mad address
     * Returns: IBIS_MAD_STATUS_GENERAL_ERR / IBIS_MAD_STATUS_SUCCESS
     */
    int AsyncSendAndRec(int mgmt_class,
                        transaction_data_t *p_transaction_data,
                        pending_mad_data_t *pending_mad_data);

    ////////////////////
    //methods mads
    ////////////////////
    void DumpReceivedMAD();
    void DumpMadData(dump_data_func_t dump_func, void *mad_obj);
    void CommonMadHeaderBuild(struct MAD_Header_Common *mad_header,
            u_int8_t mgmt_class,
            u_int8_t method,
            u_int16_t attribute_id,
            u_int32_t attribute_modifier);
    u_int8_t GetMgmtClassVersion(u_int8_t mgmt_class);

    ////////////////////
    //smp class methods
    ////////////////////
    void SMPHeaderDirectRoutedBuild(struct MAD_Header_SMP_Direct_Routed *smp_direct_route_header,
            u_int8_t method,
            u_int16_t attribute_id,
            u_int32_t attribute_modifier,
            u_int8_t direct_path_len);

    ////////////////////
    //mellanox methods
    ////////////////////
    bool IsSupportIB(void *type);
    bool IsIBDevice(void *arr, unsigned arr_size, u_int16_t dev_id);

public:
    ////////////////////
    //methods
    ////////////////////
    Ibis();
    ~Ibis();

    inline bool IsInit() { return (this->ibis_status != NOT_INITILIAZED); };
    inline bool IsReady() { return (this->ibis_status == READY); };
    inline void SetTimeout(int timeout_value) { this->timeout = timeout_value; };
    inline void SetNumOfRetries(int retries_value) { this->retries = retries_value; };
    inline u_int64_t GetNewTID() { return ++this->mads_counter;	}
    inline u_int8_t *GetSendMadPktPtr() { return this->p_pkt_send; };
    inline u_int8_t *GetRecvMadPktPtr() { return this->p_pkt_recv; };

    const char* GetLastError();
    void ClearLastError() {last_error.clear();}
    bool IsFailed() {return !last_error.empty();}

    string ConvertDirPathToStr(direct_route_t *p_curr_direct_route);
    string ConvertMadStatusToStr(u_int16_t status);

    // Returns: 0 - success / 1 - error
    int Init();
    int SetPort(const char* device_name, phys_port_t port_num);
    int SetPort(u_int64_t port_guid);   //guid is BE
    int SetSendMadAddr(int d_lid, int d_qp, int sl, int qkey);
    int GetAllLocalPortGUIDs(OUT local_port_t local_ports_array[IBIS_MAX_LOCAL_PORTS],
            OUT u_int32_t *p_local_ports_num);

    int SetPSLTable(const vector<uint8_t > &PSLTable);
    uint8_t getPSLForLid(u_int16_t lid);
    void setPSLForLid(u_int16_t lid, u_int16_t maxLid, uint8_t sl);

    void SetMaxMadsOnWire(u_int16_t max_gmps_on_wire, u_int8_t max_smps_on_wire)
    {
        m_max_gmps_on_wire = max_gmps_on_wire;
        m_max_smps_on_wire = max_smps_on_wire;
    }

    ////////////////////
    //methods mads
    ////////////////////
    /*
     * Returns: mad status[bits: 0-15] / IBIS_MAD_STATUS_SEND_FAILED / IBIS_MAD_STATUS_RECV_FAILED /
     *  IBIS_MAD_STATUS_TIMEOUT / IBIS_MAD_STATUS_GENERAL_ERR
     */
    int MadGetSet(u_int16_t lid,
            u_int32_t d_qp,
            u_int8_t sl,
            u_int32_t qkey,
            u_int8_t mgmt_class,
            u_int8_t method,
            u_int16_t attribute_id,
            u_int32_t attribute_modifier,
            u_int8_t data_offset,
            void *p_class_data,
            void *p_attribute_data,
            const pack_data_func_t pack_class_data_func,
            const unpack_data_func_t unpack_class_data_func,
            const dump_data_func_t dump_class_data_func,
            const pack_data_func_t pack_attribute_data_func,
            const unpack_data_func_t unpack_attribute_data_func,
            const dump_data_func_t dump_attribute_data_func,
            const clbck_data_t *p_clbck_data = NULL);

    void MadRecAll();

    ////////////////////
    //smp class methods
    ////////////////////
    /*
     * Returns: mad status[bits: 0-15] / IBIS_MAD_STATUS_SEND_FAILED / IBIS_MAD_STATUS_RECV_FAILED /
     *  IBIS_MAD_STATUS_TIMEOUT / IBIS_MAD_STATUS_GENERAL_ERR
     */
    int SMPMadGetSetByLid(u_int16_t lid,
            u_int8_t method,
            u_int16_t attribute_id,
            u_int32_t attribute_modifier,
            void *p_smp_attribute_data,
            const pack_data_func_t smp_pack_attribute_data_func,
            const unpack_data_func_t smp_unpack_attribute_data_func,
            const dump_data_func_t smp_dump_attribute_data_func,
            const clbck_data_t *p_clbck_data = NULL);
    int SMPMadGetSetByDirect(direct_route_t *p_direct_route,
            u_int8_t method,
            u_int16_t attribute_id,
            u_int32_t attribute_modifier,
            void *p_smp_attribute_data,
            const pack_data_func_t smp_pack_attribute_data_func,
            const unpack_data_func_t smp_unpack_attribute_data_func,
            const dump_data_func_t smp_dump_attribute_data_func,
            const clbck_data_t *p_clbck_data = NULL);

    /*
     * Returns: mad status[bits: 0-7] / IBIS_MAD_STATUS_SEND_FAILED / IBIS_MAD_STATUS_RECV_FAILED /
     *  IBIS_MAD_STATUS_TIMEOUT / IBIS_MAD_STATUS_GENERAL_ERR
     */
    int SMPPortInfoMadGetByLid(u_int16_t lid,
            phys_port_t port_number,
            struct SMP_PortInfo *p_port_info,
            const clbck_data_t *p_clbck_data = NULL);
    int SMPMlnxExtPortInfoMadGetByLid(u_int16_t lid,
            phys_port_t port_number,
            struct SMP_MlnxExtPortInfo *p_mlnx_ext_port_info,
            const clbck_data_t *p_clbck_data = NULL);
    int SMPSwitchInfoMadGetByLid(u_int16_t lid,
            struct SMP_SwitchInfo *p_switch_info,
            const clbck_data_t *p_clbck_data = NULL);
    int SMPNodeInfoMadGetByLid(u_int16_t lid,
            struct SMP_NodeInfo *p_node_info,
            const clbck_data_t *p_clbck_data = NULL);
    int SMPNodeDescMadGetByLid(u_int16_t lid,
            struct SMP_NodeDesc *p_node_desc,
            const clbck_data_t *p_clbck_data = NULL);
    int SMPSMInfoMadGetByLid(u_int16_t lid,
            struct SMP_SMInfo *p_sm_info,
            const clbck_data_t *p_clbck_data = NULL);
    int SMPLinearForwardingTableGetByLid(u_int16_t lid,
            u_int32_t lid_to_port_block_num,
            struct SMP_LinearForwardingTable *p_linear_forwarding_table,
            const clbck_data_t *p_clbck_data = NULL);
    int SMPMulticastForwardingTableGetByLid(u_int16_t lid,
            u_int8_t port_group,
            u_int32_t lid_to_port_block_num,
            struct SMP_MulticastForwardingTable *p_multicast_forwarding_table,
            const clbck_data_t *p_clbck_data = NULL);
    int SMPPkeyTableGetByLid(u_int16_t lid,
            u_int16_t port_num,
            u_int16_t block_num,
            struct SMP_PKeyTable *p_pkey_table,
            const clbck_data_t *p_clbck_data = NULL);
    int SMPGUIDInfoTableGetByLid(u_int16_t lid,
            u_int32_t block_num,
            struct SMP_GUIDInfo *p_guid_info,
            const clbck_data_t *p_clbck_data = NULL);
    int SMPSLToVLMappingTableGetByLid(u_int16_t lid,
            phys_port_t out_port_number,
            phys_port_t in_port_number,
            struct SMP_SLToVLMappingTable *p_slvl_mapping,
            const clbck_data_t *p_clbck_data = NULL);

    int SMPPortInfoMadGetByDirect(direct_route_t *p_direct_route,
            phys_port_t port_number,
            struct SMP_PortInfo *p_port_info,
            const clbck_data_t *p_clbck_data = NULL);
    int SMPMlnxExtPortInfoMadGetByDirect(direct_route_t *p_direct_route,
            phys_port_t port_number,
            struct SMP_MlnxExtPortInfo *p_mlnx_ext_port_info,
            const clbck_data_t *p_clbck_data = NULL);
    int SMPSwitchInfoMadGetByDirect(direct_route_t *p_direct_route,
            struct SMP_SwitchInfo *p_switch_info,
            const clbck_data_t *p_clbck_data = NULL);
    int SMPNodeInfoMadGetByDirect(direct_route_t *p_direct_route,
            struct SMP_NodeInfo *p_node_info,
            const clbck_data_t *p_clbck_data = NULL);
    int SMPNodeDescMadGetByDirect(direct_route_t *p_direct_route,
            struct SMP_NodeDesc *p_node_desc,
            const clbck_data_t *p_clbck_data = NULL);
    int SMPSMInfoMadGetByDirect(direct_route_t *p_direct_route,
            struct SMP_SMInfo *p_sm_info,
            const clbck_data_t *p_clbck_data = NULL);
    int SMPLinearForwardingTableGetByDirect(direct_route_t *p_direct_route,
            u_int32_t lid_to_port_block_num,
            struct SMP_LinearForwardingTable *p_linear_forwarding_table,
            const clbck_data_t *p_clbck_data = NULL);
    int SMPMulticastForwardingTableGetByDirect(direct_route_t *p_direct_route,
            u_int8_t port_group,
            u_int32_t lid_to_port_block_num,
            struct SMP_MulticastForwardingTable *p_multicast_forwarding_table,
            const clbck_data_t *p_clbck_data = NULL);
    int SMPPKeyTableGetByDirect(direct_route_t *p_direct_route,
            u_int16_t port_num,
            u_int16_t block_num,
            struct SMP_PKeyTable *p_pkey_table,
            const clbck_data_t *p_clbck_data = NULL);
    int SMPGUIDInfoTableGetByDirect(direct_route_t *p_direct_route,
            u_int32_t block_num,
            struct SMP_GUIDInfo *p_guid_info,
            const clbck_data_t *p_clbck_data = NULL);
    int SMPSLToVLMappingTableGetByDirect(direct_route_t *p_direct_route,
            phys_port_t out_port_number,
            phys_port_t in_port_number,
            struct SMP_SLToVLMappingTable *p_slvl_mapping,
            const clbck_data_t *p_clbck_data = NULL);

    ////////////////////
    //pm class methods
    ////////////////////
    /*
     * Returns: mad status[bits: 0-15] / IBIS_MAD_STATUS_SEND_FAILED / IBIS_MAD_STATUS_RECV_FAILED /
     *  IBIS_MAD_STATUS_TIMEOUT / IBIS_MAD_STATUS_GENERAL_ERR
     */
    int PMMadGetSet(u_int16_t lid,
            u_int8_t method,
            u_int16_t attribute_id,
            u_int32_t attribute_modifier,
            void *p_pm_attribute_data,
            const pack_data_func_t pm_pack_attribute_data_func,
            const unpack_data_func_t pm_unpack_attribute_data_func,
            const dump_data_func_t pm_dump_attribute_data_func,
            const clbck_data_t *p_clbck_data = NULL);

    /*
     * Returns: mad status[bits: 0-7] / IBIS_MAD_STATUS_SEND_FAILED / IBIS_MAD_STATUS_RECV_FAILED /
     *  IBIS_MAD_STATUS_TIMEOUT / IBIS_MAD_STATUS_GENERAL_ERR
     */
    int PMClassPortInfoGet(u_int16_t lid,
            struct IB_ClassPortInfo *p_ib_class_port_info,
            const clbck_data_t *p_clbck_data = NULL);
    int PMPortCountersGet(u_int16_t lid,
            phys_port_t port_number,
            struct PM_PortCounters *p_port_counters,
            const clbck_data_t *p_clbck_data = NULL);
    int PMPortCountersSet(u_int16_t lid,
            struct PM_PortCounters *p_port_counters,
            const clbck_data_t *p_clbck_data = NULL);
    int PMPortCountersClear(u_int16_t lid,
            phys_port_t port_number,
            const clbck_data_t *p_clbck_data = NULL);
    int PMPortCountersExtendedGet(u_int16_t lid,
            phys_port_t port_number,
            struct PM_PortCountersExtended *p_port_counters,
            const clbck_data_t *p_clbck_data = NULL);
    int PMPortCountersExtendedSet(u_int16_t lid,
            struct PM_PortCountersExtended *p_port_counters,
            const clbck_data_t *p_clbck_data = NULL);
    int PMPortCountersExtendedClear(u_int16_t lid,
            phys_port_t port_number,
            const clbck_data_t *p_clbck_data = NULL);
    int PMPortExtendedSpeedsCountersGet(u_int16_t lid,
            phys_port_t port_number,
            struct PM_PortExtendedSpeedsCounters *p_port_counters,
            const clbck_data_t *p_clbck_data = NULL);
    //undefined behavior
    //int PMPortExtendedSpeedsCountersSet(u_int16_t lid,
    //    struct PM_PortExtendedSpeedsCounters *p_port_counters);
    int PMPortExtendedSpeedsCountersClear(u_int16_t lid,
            phys_port_t port_number,
            const clbck_data_t *p_clbck_data = NULL);


    ////////////////////
    //vs class methods
    ////////////////////
    /*
     * Returns: mad status[bits: 0-15] / IBIS_MAD_STATUS_SEND_FAILED / IBIS_MAD_STATUS_RECV_FAILED /
     *  IBIS_MAD_STATUS_TIMEOUT / IBIS_MAD_STATUS_GENERAL_ERR
     */
    int VSMadGetSet(u_int16_t lid,
            u_int8_t method,
            u_int16_t attribute_id,
            u_int32_t attribute_modifier,
            void *p_vs_attribute_data,
            const pack_data_func_t vs_pack_attribute_data_func,
            const unpack_data_func_t vs_unpack_attribute_data_func,
            const dump_data_func_t vs_dump_attribute_data_func,
            const clbck_data_t *p_clbck_data = NULL);

    /*
     * Returns: mad status[bits: 0-7] / IBIS_MAD_STATUS_SEND_FAILED / IBIS_MAD_STATUS_RECV_FAILED /
     *  IBIS_MAD_STATUS_TIMEOUT / IBIS_MAD_STATUS_GENERAL_ERR
     */
    int VSGeneralInfoGet(u_int16_t lid, struct VendorSpec_GeneralInfo *p_general_info,
                         const clbck_data_t *p_clbck_data = NULL);

    /*
     * Returns: mad status[bits: 0-7] / IBIS_MAD_STATUS_SEND_FAILED / IBIS_MAD_STATUS_RECV_FAILED /
     *  IBIS_MAD_STATUS_TIMEOUT / IBIS_MAD_STATUS_GENERAL_ERR
     */
    int VSPortLLRStatisticsGet(u_int16_t lid,
            phys_port_t port_number,
            struct VendorSpec_PortLLRStatistics *p_port_llr_statistics,
            bool get_symbol_errors,
            const clbck_data_t *p_clbck_data = NULL);

    /*
     * Returns: mad status[bits: 0-7] / IBIS_MAD_STATUS_SEND_FAILED / IBIS_MAD_STATUS_RECV_FAILED /
     *  IBIS_MAD_STATUS_TIMEOUT / IBIS_MAD_STATUS_GENERAL_ERR
     */
    int VSPortLLRStatisticsClear(u_int16_t lid,
        phys_port_t port_number,
        bool clear_symbol_errors,
        const clbck_data_t *p_clbck_data = NULL);

    /*
     * Returns: mad status[bits: 0-7] / IBIS_MAD_STATUS_SEND_FAILED / IBIS_MAD_STATUS_RECV_FAILED /
     *  IBIS_MAD_STATUS_TIMEOUT / IBIS_MAD_STATUS_GENERAL_ERR
     */
    int VSPortLLRStatisticsSet(u_int16_t lid,
        phys_port_t port_number,
        struct VendorSpec_PortLLRStatistics *p_port_llr_statistics,
        bool set_symbol_errors,
        const clbck_data_t *p_clbck_data = NULL);

    ////////////////////
    //cc class methods
    ////////////////////
    /*
     * Returns: mad status[bits: 0-15] / IBIS_MAD_STATUS_SEND_FAILED / IBIS_MAD_STATUS_RECV_FAILED /
     *  IBIS_MAD_STATUS_TIMEOUT / IBIS_MAD_STATUS_GENERAL_ERR
     */
    int CCMadGetSet(u_int16_t lid,
            u_int8_t sl,
            u_int8_t method,
            u_int16_t attribute_id,
            u_int32_t attribute_modifier,
            u_int64_t cc_key,
            void *p_cc_log_attribute_data,
            void *p_cc_mgt_attribute_data,
            const pack_data_func_t cc_pack_attribute_data_func,
            const unpack_data_func_t cc_unpack_attribute_data_func,
            const dump_data_func_t cc_dump_attribute_data_func,
            const clbck_data_t *p_clbck_data = NULL);

    /*
     * Returns: mad status[bits: 0-7] / IBIS_MAD_STATUS_SEND_FAILED / IBIS_MAD_STATUS_RECV_FAILED /
     *  IBIS_MAD_STATUS_TIMEOUT / IBIS_MAD_STATUS_GENERAL_ERR
     */
    int CCClassPortInfoGet(u_int16_t lid,
            u_int8_t sl,
            u_int64_t cc_key,
            struct IB_ClassPortInfo *p_ib_class_port_info,
            const clbck_data_t *p_clbck_data = NULL);
    int CCClassPortInfoSet(u_int16_t lid,
            u_int8_t sl,
            u_int64_t cc_key,
            struct IB_ClassPortInfo *p_ib_class_port_info,
            const clbck_data_t *p_clbck_data = NULL);
    int CCCongestionInfoGet(u_int16_t lid,
            u_int8_t sl,
            u_int64_t cc_key,
            struct CC_CongestionInfo *p_cc_congestion_info,
            const clbck_data_t *p_clbck_data = NULL);
    int CCCongestionKeyInfoGet(u_int16_t lid,
            u_int8_t sl,
            u_int64_t cc_key,
            struct CC_CongestionKeyInfo *p_cc_congestion_key_info,
            const clbck_data_t *p_clbck_data = NULL);
    int CCCongestionKeyInfoSet(u_int16_t lid,
            u_int8_t sl,
            u_int64_t cc_key,
            struct CC_CongestionKeyInfo *p_cc_congestion_key_info,
            const clbck_data_t *p_clbck_data = NULL);
    int CCCongestionLogSwitchGet(u_int16_t lid,
            u_int8_t sl,
            u_int64_t cc_key,
            struct CC_CongestionLogSwitch *p_cc_congestion_log_sw,
            const clbck_data_t *p_clbck_data = NULL);
    int CCCongestionLogCAGet(u_int16_t lid,
            u_int8_t sl,
            u_int64_t cc_key,
            struct CC_CongestionLogCA *p_cc_congestion_log_ca,
            const clbck_data_t *p_clbck_data = NULL);
    int CCSwitchCongestionSettingGet(u_int16_t lid,
            u_int8_t sl,
            u_int64_t cc_key,
            struct CC_SwitchCongestionSetting *p_cc_sw_congestion_setting,
            const clbck_data_t *p_clbck_data = NULL);
    int CCSwitchCongestionSettingSet(u_int16_t lid,
            u_int8_t sl,
            u_int64_t cc_key,
            struct CC_SwitchCongestionSetting *p_cc_sw_congestion_setting,
            const clbck_data_t *p_clbck_data = NULL);
    int CCSwitchPortCongestionSettingGet(u_int16_t lid,
            u_int8_t sl,
            u_int64_t cc_key,
            u_int8_t block_idx,
            struct CC_SwitchPortCongestionSetting *p_cc_sw_port_congestion_setting,
            const clbck_data_t *p_clbck_data = NULL);
    int CCSwitchPortCongestionSettingSet(u_int16_t lid,
            u_int8_t sl,
            u_int64_t cc_key,
            u_int8_t block_idx,
            struct CC_SwitchPortCongestionSetting *p_cc_sw_port_congestion_setting,
            const clbck_data_t *p_clbck_data = NULL);
    int CCCACongestionSettingGet(u_int16_t lid,
            u_int8_t sl,
            u_int64_t cc_key,
            struct CC_CACongestionSetting *p_cc_ca_congestion_setting,
            const clbck_data_t *p_clbck_data = NULL);
    int CCCACongestionSettingSet(u_int16_t lid,
            u_int8_t sl,
            u_int64_t cc_key,
            struct CC_CACongestionSetting *p_cc_ca_congestion_setting,
            const clbck_data_t *p_clbck_data = NULL);
    int CCCongestionControlTableGet(u_int16_t lid,
            u_int8_t sl,
            u_int64_t cc_key,
            u_int8_t block_idx,
            struct CC_CongestionControlTable *p_cc_congestion_control_table,
            const clbck_data_t *p_clbck_data = NULL);
    int CCCongestionControlTableSet(u_int16_t lid,
            u_int8_t sl,
            u_int64_t cc_key,
            u_int8_t block_idx,
            struct CC_CongestionControlTable *p_cc_congestion_control_table,
            const clbck_data_t *p_clbck_data = NULL);
    int CCTimeStampGet(u_int16_t lid,
            u_int8_t sl,
            u_int64_t cc_key,
            struct CC_TimeStamp *p_cc_time_stamp,
            const clbck_data_t *p_clbck_data = NULL);

    ////////////////////
    //mellanox methods
    ////////////////////
    bool IsVenMellanox(u_int32_t vendor_id);
    //switch
    bool IsDevAnafa(u_int16_t dev_id);
    bool IsDevShaldag(u_int16_t dev_id);
    bool IsDevSwitchXIB(u_int16_t dev_id);
    //bridge
    bool IsDevBridgeXIB(u_int16_t dev_id);
    //hca
    bool IsDevTavor(u_int16_t dev_id);
    bool IsDevSinai(u_int16_t dev_id);
    bool IsDevArbel(u_int16_t dev_id);
    bool IsDevConnectX_1IB(u_int16_t dev_id);
    bool IsDevConnectX_2IB(u_int16_t dev_id);
    bool IsDevConnectX_3IB(u_int16_t dev_id);
    bool IsDevConnectXIB(u_int16_t dev_id);
    bool IsDevGolan(u_int16_t dev_id);
};

#endif	/* IBIS_H_ */

