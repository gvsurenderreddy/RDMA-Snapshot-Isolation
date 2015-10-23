/*
 * © Copyright (c) 2004-2013 Mellanox Technologies LTD. All rights reserved
 *
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


#ifndef IBDIAG__CLBCK_H
#define IBDIAG__CLBCK_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string>
#include <list>
#include <map>
using namespace std;

#include "infiniband/ibdiag/ibdiag.h"

//#include <infiniband/ibdm/Fabric.h>
//#include <infiniband/ibis/ibis.h>

//#include "ibdiag_ibdm_extended_info.h"
//#include "ibdiag_fabric_errs.h"
#include "ibdiag_types.h"

#define NOT_SUPPORT_XMIT_WAIT 0x1
#define NOT_SUPPORT_EXT_PORT_COUNTERS 0x2
#define NOT_SUPPORT_EXT_SPEEDS_COUNTERS 0x4
#define NOT_SUPPORT_LLR_COUNTERS 0x8

#define IS_SUPPORT_EXT_PORT_FAILED(appData) \
    (appData & NOT_SUPPORT_EXT_PORT_COUNTERS)
#define IS_SUPPORT_EXT_SPEEDS_FAILED(appData) \
    (appData & NOT_SUPPORT_EXT_SPEEDS_COUNTERS)
#define IS_SUPPORT_LLR_FAILED(appData1) \
    (appData1 & NOT_SUPPORT_LLR_COUNTERS)

// PM
void IBDiagPMCapMaskClbck(const clbck_data_t &clbck_data,
                          int rec_status,
                          void *p_attribute_data);

void IBDiagPMPortCountersGetClbck(const clbck_data_t &clbck_data,
                          int rec_status,
                          void *p_attribute_data);

void IBDiagPMPortCountersClearClbck(const clbck_data_t &clbck_data,
                          int rec_status,
                          void *p_attribute_data);

void IBDiagPMPortCountersExtendedGetClbck(const clbck_data_t &clbck_data,
                          int rec_status,
                          void *p_attribute_data);

void IBDiagPMPortCountersExtendedClearClbck(const clbck_data_t &clbck_data,
                          int rec_status,
                          void *p_attribute_data);

void IBDiagPMPortExtendedSpeedsGetClbck(const clbck_data_t &clbck_data,
                          int rec_status,
                          void *p_attribute_data);

void IBDiagPMPortExtendedSpeedsClearClbck(const clbck_data_t &clbck_data,
                          int rec_status,
                          void *p_attribute_data);

// VS
void IBDiagVSPortLLRStatisticsGetClbck(const clbck_data_t &clbck_data,
                          int rec_status,
                          void *p_attribute_data);

void IBDiagVSPortLLRStatisticsClearClbck(const clbck_data_t &clbck_data,
                          int rec_status,
                          void *p_attribute_data);

void IBDiagVSGeneralInfoGetClbck(const clbck_data_t &clbck_data,
                          int rec_status,
                          void *p_attribute_data);

// SMP
void IBDiagSMPGUIDInfoTableGetClbck(const clbck_data_t &clbck_data,
                          int rec_status,
                          void *p_attribute_data);

void IBDiagSMPPkeyTableGetClbck(const clbck_data_t &clbck_data,
                          int rec_status,
                          void *p_attribute_data);

void IBDiagSMPSMInfoMadGetClbck(const clbck_data_t &clbck_data,
                          int rec_status,
                          void *p_attribute_data);

void IBDiagSMPLinearForwardingTableGetClbck(const clbck_data_t &clbck_data,
                          int rec_status,
                          void *p_attribute_data);

void IBDiagSMPMulticastForwardingTableGetClbck(const clbck_data_t &clbck_data,
                          int rec_status,
                          void *p_attribute_data);

void IBDiagSMPSLToVLMappingTableGetClbck(const clbck_data_t &clbck_data,
                          int rec_status,
                          void *p_attribute_data);

class IBDiagClbck
{
    list_p_fabric_general_err* m_pErrors;
    IBDiag *                   m_pIBDiag;
    IBDMExtendedInfo*          m_pFabricExtendedInfo;
    int                        m_ErrorState;
    string                     m_LastError;
    ofstream *                 m_p_sout;

    void SetLastError(const char *fmt, ...);

public:
    IBDiagClbck():
        m_pErrors(NULL), m_pIBDiag(NULL),
        m_pFabricExtendedInfo(NULL), m_ErrorState(IBDIAG_SUCCESS_CODE),
        m_p_sout(NULL){}

    int GetState() {return m_ErrorState;}
    void ResetState()
    {
        m_LastError.clear();
        m_ErrorState = IBDIAG_SUCCESS_CODE;
    }

    const char* GetLastError();

    void Set(IBDiag * p_IBDiag, IBDMExtendedInfo* p_fabric_extended_info,
             list_p_fabric_general_err* p_errors, ofstream * p_sout = NULL)
    {
        m_pIBDiag = p_IBDiag;
        m_pFabricExtendedInfo = p_fabric_extended_info;
        m_pFabricExtendedInfo = m_pFabricExtendedInfo;
        m_pErrors = p_errors;
        m_ErrorState = IBDIAG_SUCCESS_CODE;
        m_LastError.clear();
        m_p_sout = p_sout;
    }

    void SetOutStream(ofstream * p_sout) {m_p_sout = p_sout;}

    void Reset()
    {
        m_pIBDiag = NULL;
        m_pErrors = NULL;
        m_pFabricExtendedInfo = NULL;
        m_p_sout = NULL;
    }

    // PM
    void PMCapMaskClbck(const clbck_data_t &clbck_data,
                        int rec_status,
                        void *p_attribute_data);

    void PMPortCountersGetClbck(const clbck_data_t &clbck_data,
                        int rec_status,
                        void *p_attribute_data);

    void PMPortCountersClearClbck(const clbck_data_t &clbck_data,
                        int rec_status,
                        void *p_attribute_data);

    void PMPortCountersExtendedGetClbck(const clbck_data_t &clbck_data,
                        int rec_status,
                        void *p_attribute_data);

    void PMPortCountersExtendedClearClbck(const clbck_data_t &clbck_data,
                        int rec_status,
                        void *p_attribute_data);

    void PMPortExtendedSpeedsGetClbck(const clbck_data_t &clbck_data,
                        int rec_status,
                        void *p_attribute_data);

    void PMPortExtendedSpeedsClearClbck(const clbck_data_t &clbck_data,
                        int rec_status,
                        void *p_attribute_data);

    // VS
    void VSPortLLRStatisticsGetClbck(const clbck_data_t &clbck_data,
                        int rec_status,
                        void *p_attribute_data);

    void VSPortLLRStatisticsClearClbck(const clbck_data_t &clbck_data,
                        int rec_status,
                        void *p_attribute_data);

    void VSGeneralInfoGetClbck(const clbck_data_t &clbck_data,
                        int rec_status,
                        void *p_attribute_data);

    // SMP
    void SMPGUIDInfoTableGetClbck(const clbck_data_t &clbck_data,
                        int rec_status,
                        void *p_attribute_data);

    void SMPPkeyTableGetClbck(const clbck_data_t &clbck_data,
                        int rec_status,
                        void *p_attribute_data);

    void SMPSMInfoMadGetClbck(const clbck_data_t &clbck_data,
                        int rec_status,
                        void *p_attribute_data);

    void SMPLinearForwardingTableGetClbck(const clbck_data_t &clbck_data,
                        int rec_status,
                        void *p_attribute_data);

    void SMPMulticastForwardingTableGetClbck(const clbck_data_t &clbck_data,
                        int rec_status,
                        void *p_attribute_data);

    void SMPSLToVLMappingTableGetClbck(const clbck_data_t &clbck_data,
                        int rec_status,
                        void *p_attribute_data);

};

#endif          /* IBDIAG__CLBCK_H */
