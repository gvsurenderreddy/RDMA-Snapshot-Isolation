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


/*
 * Fabric Utilities Project
 *
 * Data Model Header file
 *
 */

#ifndef IBDM_FABRIC_H
#define IBDM_FABRIC_H

#if HAVE_CONFIG_H
#  include "config.h"
#endif

#include <functional>
#include <map>
#include <set>
#include <list>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <inttypes.h>

#include <infiniband/ibutils/ibutils_common.h>

///////////////////////////////////////////////////////////////////////////////
const char * get_ibdm_version();

// string vectorization (Vectorize.cpp)
std::string compressNames (std::list<std::string> &words);

///////////////////////////////////////////////////////////////////////////////
#define IB_LID_UNASSIGNED 0
#define IB_LFT_UNASSIGNED 255
#define IB_SLT_UNASSIGNED 255
#define IB_HOP_UNASSIGNED 255
#define IB_NUM_SL 16
//maximum number of physical ports that can be returned in node info (not counting port 0)
#define IB_MAX_PHYS_NUM_PORTS  254
#define IB_MIN_PHYS_NUM_PORTS  1

#ifndef PRIx64
#if __WORDSIZE == 64
#define PRIx64 "lx"
#else
#define PRIx64 "llx"
#endif
#endif

using namespace std;

#define GET_LID_STEP(lmc) ((lid_t)(1 << lmc))
#define LID_INC(lid,inc) (lid = (lid_t)(lid + inc))

///////////////////////////////////////////////////////////////////////////////
//
// STL TYPEDEFS
//
// This section defines typedefs for all the STL containers used by the package

// for comparing strings
struct strless : public binary_function <string, string, bool> {
    bool operator()(const string& x, const string& y) const {
        return (strcmp(x.c_str(), y.c_str()) < 0);
    }
};

typedef vector<int > vec_int;
typedef vector<vec_int > vec_vec_int;
typedef vector<uint8_t > vec_byte;
typedef vector<uint64_t > vec_uint64;
typedef vector<vec_byte > vec_vec_byte;
typedef vector<vec_vec_byte > vec3_byte;

typedef vector<phys_port_t > vec_phys_ports;
typedef vector<lid_t > vec_lids;
typedef vector<class IBPort * > vec_pport;
typedef vector<class IBNode * > vec_pnode;
typedef vector<class VChannel * > vec_pvch;
typedef map< string, class IBSysPort *, strless > map_str_psysport;
typedef map< string, class IBNode *, strless > map_str_pnode;
typedef map< string, class IBSystem *, strless > map_str_psys;
typedef map< uint64_t, class IBPort *, less<uint64_t> > map_guid_pport;
typedef map< uint64_t, class IBNode *, less<uint64_t> > map_guid_pnode;
typedef map< uint64_t, class IBSystem *, less<uint64_t> > map_guid_psys;
typedef map< string, int, strless > map_str_int;
typedef map< string, string, strless > map_str_str;
typedef list<class IBPort * > list_pport;
typedef list<class IBNode * > list_pnode;
typedef list<class IBSystem * > list_psystem;
typedef list<int > list_int;
typedef list<unsigned > list_unsigned;
typedef list<char * > list_charp;
typedef list<string > list_str;
typedef list<phys_port_t > list_phys_ports;
typedef map< IBNode *, int, less< IBNode *> > map_pnode_int;
typedef map< IBNode *, vec_int, less< IBNode *> > map_pnode_vec_int;
typedef map< IBSystem *, int, less< IBSystem *> > map_psystem_int;
typedef map< string, list_pnode > map_str_list_pnode;
typedef set< uint16_t, less< uint16_t > > set_uint16;
typedef map< IBNode *, rank_t, less< IBNode *> > map_pnode_rank;

typedef map< IBNode *, IBNode * > map_template_pnode;
typedef set< string > set_boards;
typedef set< IBNode * > set_nodes_ptr;

///////////////////////////////////////////////////////////////////////////////
//
// CONSTANTS
//
#define FABU_LOG_NONE 0x0
#define FABU_LOG_ERROR 0x1
#define FABU_LOG_INFO 0x2
#define FABU_LOG_VERBOSE 0x4
#define IBNODE_UNASSIGNED_RANK 0xFF

// DFS constants type
typedef enum {Untouched,Open,Closed} dfs_t;

//
// GLOBALS
//
// Log level should be part of the "main" module
extern uint8_t FabricUtilsVerboseLevel;

void ibdmUseInternalLog();
void ibdmUseCoutLog();
void ibdmClearInternalLog();
char *ibdmGetAndClearInternalLog();


///////////////////////////////////////////////////////////////////////////////
// We only recognize CA or SW nodes
typedef enum {IB_UNKNOWN_NODE_TYPE, IB_CA_NODE, IB_SW_NODE} IBNodeType;

static inline IBNodeType char2nodetype(const char *w)
{
    if (!w || (*w == '\0')) return IB_UNKNOWN_NODE_TYPE;
    if (!strcmp(w,"SW"))    return IB_SW_NODE;
    if (!strcmp(w,"CA"))    return IB_CA_NODE;
    return IB_UNKNOWN_NODE_TYPE;
};

static inline const char * nodetype2char(const IBNodeType w)
{
    switch (w) {
    case IB_SW_NODE:    return("SW");
    case IB_CA_NODE:    return("CA");
    default:            return("UNKNOWN");
    }
};


typedef enum {IB_UNKNOWN_LINK_WIDTH = 0,
              IB_LINK_WIDTH_1X = 1,
              IB_LINK_WIDTH_4X = 2,
              IB_LINK_WIDTH_8X = 4,
              IB_LINK_WIDTH_12X =8,
} IBLinkWidth;

static inline IBLinkWidth char2width(const char *w)
{
    if (!w || (*w == '\0')) return IB_UNKNOWN_LINK_WIDTH;
    if (!strcmp(w,"1x"))    return IB_LINK_WIDTH_1X;
    if (!strcmp(w,"4x"))    return IB_LINK_WIDTH_4X;
    if (!strcmp(w,"8x"))    return IB_LINK_WIDTH_8X;
    if (!strcmp(w,"12x"))   return IB_LINK_WIDTH_12X;
    return IB_UNKNOWN_LINK_WIDTH;
};

static inline const char * width2char(const IBLinkWidth w)
{
    switch (w) {
    case IB_LINK_WIDTH_1X:  return("1x");
    case IB_LINK_WIDTH_4X:  return("4x");
    case IB_LINK_WIDTH_8X:  return("8x");
    case IB_LINK_WIDTH_12X: return("12x");
    default:                return("UNKNOWN");
    }
};


typedef enum {IB_UNKNOWN_LINK_SPEED = 0,
              IB_LINK_SPEED_2_5     = 1,
              IB_LINK_SPEED_5       = 2,
              IB_LINK_SPEED_10      = 4,
              IB_LINK_SPEED_14      = 1 << 8,   /* second byte is for extended ones */
              IB_LINK_SPEED_25      = 2 << 8,   /* second byte is for extended ones */
              IB_LINK_SPEED_FDR_10  = 1 << 16,  /* third byte is for vendor specific ones */
} IBLinkSpeed;

static inline const char * speed2char(const IBLinkSpeed s)
{
    switch (s) {
    case IB_LINK_SPEED_2_5:     return("2.5");
    case IB_LINK_SPEED_5:       return("5");
    case IB_LINK_SPEED_10:      return("10");
    case IB_LINK_SPEED_14:      return("14");
    case IB_LINK_SPEED_25:      return("25");
    case IB_LINK_SPEED_FDR_10:  return("FDR10");
    default:                    return("UNKNOWN");
    }
};

static inline IBLinkSpeed char2speed(const char *s)
{
    if (!s || (*s == '\0')) return IB_UNKNOWN_LINK_SPEED;
    if (!strcmp(s,"2.5"))   return IB_LINK_SPEED_2_5;
    if (!strcmp(s,"5"))     return IB_LINK_SPEED_5;
    if (!strcmp(s,"10"))    return IB_LINK_SPEED_10;
    if (!strcmp(s,"14"))    return IB_LINK_SPEED_14;
    if (!strcmp(s,"25"))    return IB_LINK_SPEED_25;
    if (!strcmp(s,"FDR10")) return IB_LINK_SPEED_FDR_10;
    return IB_UNKNOWN_LINK_SPEED;
};

static inline IBLinkSpeed extspeed2speed(uint8_t ext_speed)
{
    switch (ext_speed) {
    case 1 /* IB_LINK_SPEED_14 >> 8 */: return IB_LINK_SPEED_14;
    case 2 /* IB_LINK_SPEED_25 >> 8 */: return IB_LINK_SPEED_25;
    default:                            return IB_UNKNOWN_LINK_SPEED;
    }
};

static inline IBLinkSpeed mlnxspeed2speed(uint8_t mlnx_speed)
{
    switch (mlnx_speed) {
    case 1 /* IB_LINK_SPEED_FDR_10 >> 16 */:    return IB_LINK_SPEED_FDR_10;
    default:                                    return IB_UNKNOWN_LINK_SPEED;
    }
};

typedef enum {
    LLR_ACTIVE_CELL_RESERVED = 0,
    LLR_ACTIVE_CELL_64 = 1,
    LLR_ACTIVE_CELL_FIRST = LLR_ACTIVE_CELL_64,
    LLR_ACTIVE_CELL_128 = 2,
    LLR_ACTIVE_CELL_LAST = LLR_ACTIVE_CELL_128
} LLRActiveCellSize;

static inline unsigned int llr_active_cell2size(u_int8_t active_cell)
{
    switch (active_cell) {
    case LLR_ACTIVE_CELL_64:
        return 64;
    case LLR_ACTIVE_CELL_128:
        return 128;
    case LLR_ACTIVE_CELL_RESERVED: default:
        return 0;
    }
};

typedef enum {IB_UNKNOWN_PORT_STATE = 0,
              IB_PORT_STATE_DOWN = 1,
              IB_PORT_STATE_INIT = 2,
              IB_PORT_STATE_ARM = 3,
              IB_PORT_STATE_ACTIVE = 4
} IBPortState;

static inline IBPortState char2portstate(const char *w)
{
    if (!w || (*w == '\0')) return IB_UNKNOWN_PORT_STATE;
    if (!strcmp(w,"DOWN"))  return IB_PORT_STATE_DOWN;
    if (!strcmp(w,"INI"))   return IB_PORT_STATE_INIT;
    if (!strcmp(w,"ARM"))   return IB_PORT_STATE_ARM;
    if (!strcmp(w,"ACT"))   return IB_PORT_STATE_ACTIVE;
    return IB_UNKNOWN_PORT_STATE;
};

static inline const char * portstate2char(const IBPortState w)
{
    switch (w)
    {
    case IB_PORT_STATE_DOWN:    return("DOWN");
    case IB_PORT_STATE_INIT:    return("INI");
    case IB_PORT_STATE_ARM:     return("ARM");
    case IB_PORT_STATE_ACTIVE:  return("ACT");
    default:                    return("UNKNOWN");
    }
};


static inline string guid2str(uint64_t guid) {
    char buff[19];
    sprintf(buff, "0x%016" PRIx64 , guid);
    return buff;
};


///////////////////////////////////////////////////////////////////////////////
//
// Virtual Channel class
// Used for credit loops verification
//

class VChannel {
    vec_pvch depend;    // Vector of dependencies
    dfs_t    flag;      // DFS state
public:
    IBPort  *pPort;
    int vl;

    // Constructor
    VChannel(IBPort * p, int v) {
        flag = Untouched;
        pPort = p;
        vl = v;
    };

    //Getters/Setters
    inline void setDependSize(int numDepend) {
        if (depend.size() != (unsigned)numDepend) {
            depend.resize(numDepend);
            for (int i=0;i<numDepend;i++) {
                depend[i] = NULL;
            }
        }
    };
    inline int getDependSize() {
        return (int)depend.size();
    };

    inline dfs_t getFlag() {
        return flag;
    };
    inline void setFlag(dfs_t f) {
        flag = f;
    };

    inline void setDependency(int i,VChannel* p) {
        depend[i]=p;
    };
    inline VChannel* getDependency(int i) {
        return depend[i];
    };
};


///////////////////////////////////////////////////////////////////////////////
//
// IB Port class.
// This is not the "End Node" but the physical port of
// a node.
//
class IBPort {
    uint64_t        guid;           // The port GUID (on SW only on Port0)
    IBLinkWidth     width;          // The link width of the port
    IBLinkSpeed     speed;          // The link speed of the port
    IBPortState     port_state;     // The state of the port
public:
    class IBPort    *p_remotePort;  // Port connected on the other side of link
    class IBSysPort *p_sysPort;     // The system port (if any) connected to
    class IBNode    *p_node;        // The node the port is part of.
    vec_pvch        channels;       // Virtual channels associated with the port
    phys_port_t     num;            // Physical ports are identified by number.
    lid_t           base_lid;       // The base lid assigned to the port.
    uint8_t         lmc;            // The LMC assigned to the port (0 <= lmc < 8)
    unsigned int    counter1;       // a generic value to be used by various algorithms
    unsigned int    counter2;       // a generic value to be used by various algorithms
    u_int32_t       createIndex;    // Port index, we will use it to create vectors of extended
                                    // info regarding this node and access the info in O(1)

    // constructor
    IBPort(IBNode *p_nodePtr, phys_port_t number);

    // destructor:
    ~IBPort();

    // get the port name
    string getName();

    // connect the port to another node port
    void connect(IBPort *p_otherPort);

    // disconnect the port. Return 0 if successful
    int disconnect(int duringSysPortDisconnect = 0);

    inline uint64_t guid_get() {return guid;};
    void guid_set(uint64_t g);

    //get_internal and get_common wraper functions
    //It can occur in the abric that the ports on the link
    //report different speed/width/state. This is illegal situation in the fabric.
    //It means that one of the port's fw is lying, but we don't know which.
    //This is why everywhere that access to speed/width/state is needed,
    //use get_common_speed/width/state (that return the common data on the link)
    //Except when we want to check and report the inconsistency on the link in that data,
    //then use get_internal functions.

    //get internal port state
    IBPortState get_internal_state() { return port_state; }
    //set internal port_state
    void set_internal_state(IBPortState s) { port_state = s; }
    //get common port_state on the port and its peer
    IBPortState get_common_state();

    //get internal port width
    IBLinkWidth get_internal_width() { return width; }
    //set internal port width
    void set_internal_width(IBLinkWidth w) { width = w; }
    //get common port width on the port and its peer
    IBLinkWidth get_common_width();

    //get internal speed
    IBLinkSpeed get_internal_speed() { return speed; }
    //set internal speed
    void set_internal_speed(IBLinkSpeed sp) { speed = sp; }
    //get common port speed on the port and its peer
    IBLinkSpeed get_common_speed();

    void set_internal_data(IBLinkSpeed sp, IBLinkWidth w, IBPortState s) {
        speed = sp;
        width = w;
        port_state = s;
    }
};


///////////////////////////////////////////////////////////////////////////////
//
// Private App Data
//
typedef union _PrivateAppData {
    void        *ptr;
    uint64_t    val;
} PrivateAppData;


//
// IB Node class
//
class IBNode {
    uint64_t        guid;
    uint64_t        sys_guid;       // guid of the system that node is part of
    vec_pport       Ports;          // Vector of all the ports (in index 0 we will put port0 if exist)
    static bool     usePSL;         // true if should use data from PSL
    static bool     useSLVL;        // true if should use data from SLVL

 public:
    string          name;           // Name of the node (instance name of the chip)
    IBNodeType      type;           // Either a CA or SW
    device_id_t     devId;          // The device ID of the node
    uint32_t        revId;          // The device revision Id.
    uint32_t        vendId;         // The device Vendor ID.
    uint8_t         rank;           // The tree level the node is at 0 is root.
    class IBSystem  *p_system;      // What system we belong to
    class IBFabric  *p_fabric;      // What fabric we belong to.
    phys_port_t     numPorts;       // Number of physical ports
    string          attributes;     // Comma-sep string of arbitrary attributes k=v
    string          description;    // Description of the node
    vec_vec_byte    MinHopsTable;   // Table describing minimal hop count through
                                    // each port to each target lid
    vec_byte        LFT;            // The LFT of this node (for switches only)
    vec_byte        PSL;            // PSL table (CAxCA->SL mapping) of this node (for CAs only)
    vec3_byte       SLVL;           // SL2VL table of this node (for switches only)
    vec_uint64      MFT;            // The Multicast forwarding table
    PrivateAppData  appData1;       // Application Private Data #1
    PrivateAppData  appData2;       // Application Private Data #2
    u_int32_t       createIndex;    // Node index, we will use it to create vectors of extended
                                    // info regarding this node and access the info in O(1)
    bool              toIgnore;

    // Constructor
    IBNode(string n, class IBFabric *p_fab, class IBSystem *p_sys,
            IBNodeType t, phys_port_t np);

    // destructor:
    ~IBNode();

    // get the node name
    inline const string &getName() { return name; }

    // create a new port by name if required to
    IBPort *makePort(phys_port_t num);

    // get a port by number num = 1..N:
    inline IBPort *getPort(phys_port_t num) {
        if ((type == IB_SW_NODE) && (num == 0))
            return Ports[0];
        if ((num < 1) || (Ports.size() <= num))
            return NULL;
        return Ports[num];
    }

    // Set the min hop for the given port (* is all) lid pair
    void setHops(IBPort *p_port, lid_t lid, uint8_t hops);

    // Get the min number of hops defined for the given port or all
    uint8_t getHops(IBPort *p_port, lid_t lid);

    // Report Hop Table of the given node
    void repHopTable();

    // Scan the node ports and find the first port
    // with min hop to the lid
    IBPort *getFirstMinHopPort(lid_t lid);

    // Set the Linear Forwarding Table:
    void setLFTPortForLid(lid_t lid, phys_port_t portNum);

    // Get the LFT for a given lid
    phys_port_t getLFTPortForLid(lid_t lid);

    // Resize the Linear Forwarding Table (can be resize with linear top value):
    void resizeLFT(uint16_t newSize);

    // Set the PSL table
    void setPSLForLid(lid_t lid, lid_t maxLid, uint8_t sl);

    // Add entry to SL2VL table
    void setSLVL(phys_port_t iport,phys_port_t oport,uint8_t sl, uint8_t vl);

    // Get the PSL table for a given lid
    uint8_t getPSLForLid(lid_t lid);

    // Check if node contains PSL Info:
    bool isValidPSLInfo() { return !(PSL.empty() && usePSL); }

    // Get the SL2VL table entry
    uint8_t getSLVL(phys_port_t iport, phys_port_t oport, uint8_t sl);

    // Set the Multicast FDB table
    void setMFTPortForMLid(lid_t lid, phys_port_t portNum);
    void setMFTPortForMLid(lid_t lid, uint16_t portMask, uint8_t portGroup);

    // Get the list of ports for the givan MLID from the MFT
    list_phys_ports getMFTPortsForMLid(lid_t lid);

    int getLidAndLMC(phys_port_t portNum, lid_t &lid, uint8_t &lmc);

    inline uint64_t guid_get() {return guid;};
    void guid_set(uint64_t g);
    inline uint64_t system_guid_get() {return sys_guid;};
    void system_guid_set(uint64_t g);
}; // Class IBNode


///////////////////////////////////////////////////////////////////////////////
//
// System Port Class
// The System Port is a front pannel entity.
//
class IBSysPort {
public:
    string          name;               // The front pannel name of the port
    class IBSysPort *p_remoteSysPort;   // If connected the other side sys port
    class IBSystem  *p_system;          // System it benongs to
    class IBPort    *p_nodePort;        // The node port it connects to.

    // Constructor
    IBSysPort(string n, class IBSystem *p_sys);

    // destructor:
    ~IBSysPort();

    // connect two SysPorts
    void connectPorts(IBSysPort *p_otherSysPort);

    // connect two SysPorts and two node ports
    void connect(IBSysPort *p_otherSysPort,
            IBLinkWidth width = IB_LINK_WIDTH_4X,
            IBLinkSpeed speed = IB_LINK_SPEED_2_5,
            IBPortState state = IB_PORT_STATE_ACTIVE);

    // disconnect the SysPort (and ports). Return 0 if successful
    int disconnect(int duringPortDisconnect = 0);
};

///////////////////////////////////////////////////////////////////////////////

//typedef enum {IB_SYS_OK, IB_SYS_MOD, IB_SYS_NEW} IBSysTypeDesc;

///////////////////////////////////////////////////////////////////////////////
//
// IB System Class
// This is normally derived into a system specific class
//
class IBSystem {
public:
    string              name;       // the "host" name of the system
    string              type;       // what is the type i.e. Cougar, Buffalo etc
    string              cfg;        // configuration string modifier
    class IBFabric      *p_fabric;  // fabric belongs to
    map_str_psysport    PortByName; // A map provising pointer to the SysPort by name
    map_str_pnode       NodeByName; // All the nodes belonging to this system.
    bool                newDef;     // system without SysDef

    // Default Constractor - empty need to be overwritten
    IBSystem() {} ;

    // Destructor must be virtual
    virtual ~IBSystem();

    // Constractor
    IBSystem(const string &n, class IBFabric *p_fab, const string &t);

    // Get a string with all the System Port Names (even if not connected)
    virtual list_str getAllSysPortNames();

    // Get a Sys Port by name
    IBSysPort *getSysPort(string name);

    inline IBNode *getNode(string nName) {
        map_str_pnode::iterator nI = NodeByName.find(nName);
        if (nI != NodeByName.end()) {
            return (*nI).second;
        } else {
            return NULL;
        }
    };

    // make sure we got the port defined (so define it if not)
    virtual IBSysPort *makeSysPort(string pName);

    // get the node port for the given sys port by name
    virtual IBPort *getSysPortNodePortByName(string sysPortName);

    // Remove a sub module of the system
    int removeBoard(string boardName);

    // parse configuration string into a vector
    void cfg2Vector(const string& cfg,
            vector<string>& boardCfgs,
            int numBoards);

    // Write system IBNL into the given directory and return IBNL name
    int dumpIBNL(const char *ibnlDir, string &sysType);

    void generateSysPortName(char *buf, IBNode *p_node, unsigned int  pn);
};


///////////////////////////////////////////////////////////////////////////////
//
// IB Fabric Class
// The entire fabric
//
class IBFabric {
private:
    u_int32_t       numOfNodesCreated;
    u_int32_t       numOfPortsCreated;
public:
    map_str_pnode           NodeByName;     // Provide the node pointer by its name
    map_guid_pnode          NodeByGuid;     // Provides the node by guid
    map_str_psys            SystemByName;   // Provide the system pointer by its name
    map_guid_pnode          NodeBySystemGuid; // Provides the node by system guid
    map_guid_pport          PortByGuid;     // Provides the port by guid
    map_str_list_pnode      NodeByDesc;     // Provides nodes by node description - valid
                                            // only for discovery process, after that we will
                                            // not update this field if delete a node
    vec_pport               PortByLid;      // Pointer to the Port by its lid
    lid_t                   minLid;         // Track min lid used.
    lid_t                   maxLid;         // Track max lid used.
    uint8_t                 caLmc;          // Default LMC value used for all CAs and Routers
    uint8_t                 swLmc;          // Default LMC value used for all Switches
    uint8_t                 defAllPorts;    // If not zero all ports (unconn) are declared
    uint8_t                 subnCANames;    // CA nodes are marked by name
    uint8_t                 numSLs;         // Number of used SLs
    uint8_t                 numVLs;         // Number of used VLs
    set_uint16              mcGroups;       // A set of all active multicast groups

    // Constructor
    IBFabric() {
        maxLid              = 0;
        defAllPorts         = 1;
        subnCANames         = 1;
        numSLs              = 1;
        numVLs              = 1;
        caLmc               = 0;
        swLmc               = 0;
        minLid              = 0;
        PortByLid.push_back(NULL); // make sure we always have one for LID=0
        numOfNodesCreated   = 0;
        numOfPortsCreated   = 0;
    };

    // Destructor
    ~IBFabric();

    inline u_int32_t getNodeIndex() { return numOfNodesCreated++; }
    inline u_int32_t getPortIndex() { return numOfPortsCreated++; }

    // get the node by its name (create one of does not exist)
    IBNode *makeNode(string n,
                IBSystem *p_sys,
                IBNodeType type,
                phys_port_t numPorts);

    // get port by guid:
    IBPort *getPortByGuid(uint64_t guid);

    // get the node by its name
    IBNode *getNode(string name);
    IBNode *getNodeByGuid(uint64_t guid);

    // return the list of node pointers matching the required type
    list_pnode *getNodesByType(IBNodeType type);

    // crate a new generic system - basically an empty contaner for nodes...
    IBSystem *makeGenericSystem(const string &name, const string &sysType);

    // crate a new system - the type must have a pre-red SysDef
    IBSystem *makeSystem(string name, string type, string cfg = "");

    // get a system by name
    IBSystem *getSystem(string name);

    // get a system by guid
    IBSystem *getSystemByGuid(uint64_t guid);

    // Add a cable connection
    int addCable(string t1, string n1, string p1,
            string t2, string n2, string p2,
            IBLinkWidth width, IBLinkSpeed speed);

    // Parse the cables file and build the fabric
    int parseCables(const string &fn);

    // Parse Topology File
    int parseTopology(const string &fn);

    // Add a link into the fabric - this will create nodes / ports and link between them
    // by calling the forward methods makeNode + setNodePortsystem + makeLinkBetweenPorts
    int addLink(string type1, phys_port_t numPorts1,
            uint64_t sysGuid1, uint64_t nodeGuid1,  uint64_t portGuid1,
            int vend1, device_id_t devId1, int rev1, string desc1,
            lid_t lid1, uint8_t lmc1, phys_port_t portNum1,
            string type2, phys_port_t numPorts2,
            uint64_t sysGuid2, uint64_t nodeGuid2,  uint64_t portGuid2,
            int vend2, device_id_t devId2, int rev2, string desc2,
            lid_t lid2, uint8_t lmc2, phys_port_t portNum2,
            IBLinkWidth width, IBLinkSpeed speed, IBPortState portState);

    // create a new node in fabric (don't check if exists already), also create system if required
    IBNode * makeNode(IBNodeType type, phys_port_t numPorts,
                    uint64_t sysGuid, uint64_t nodeGuid,
                    int vend, device_id_t devId, int rev, string desc);

    // set the node's port given data (create one of does not exist).
    IBPort * setNodePort(IBNode * p_node, uint64_t portGuid,
			 lid_t lid, uint8_t limc, phys_port_t portNum,
			 IBLinkWidth width, IBLinkSpeed speed, IBPortState port_state);

    // Add a link between the given ports.
    // not creating sys ports for now.
    int makeLinkBetweenPorts(IBPort *p_port1, IBPort *p_port2);

    // Parse the OpenSM subnet.lst file and build the fabric from it.
    int parseSubnetLinks (string fn);

    // Parse ibnetdiscover file and build the fabric from it.
    int parseIBNetDiscover (string fn);

    // Parse OpenSM FDB dump file
    int parseFdbFile(string fn);

    // Parse PSL mapping
    int parsePSLFile(const string& fn);

    // Parse SLVL mapping
    int parseSLVLFile(const string& fn);

    // Parse an OpenSM MCFDBs file and set the MFT table accordingly
    int parseMCFdbFile(string fn);

    // set a lid port
    void setLidPort(lid_t lid, IBPort *p_port);

    // get a port by lid
    inline IBPort *getPortByLid(lid_t lid) {
        if ( PortByLid.empty() || (PortByLid.size() < (unsigned)lid+1))
            return NULL;
        return (PortByLid[lid]);
    };

    //set number of SLs
    inline void setNumSLs(uint8_t nSL) {
        numSLs=nSL;
    };

    //get number of SLs
    inline uint8_t getNumSLs() {
        return numSLs;
    };

    //set number of VLs
    inline void setNumVLs(uint8_t nVL) {
        numVLs=nVL;
    };

    //get number of VLs
    inline uint8_t getNumVLs() {
        return numVLs;
    };

    // dump out the contents of the entire fabric
    void dump(ostream &sout);

    // write out a topology file
    int dumpTopology(const char *fileName, const char *ibnlDir);

    // write out a LST file
    int dumpLSTFile(char *fileName);

    // Write out the name to guid and LID map
    int dumpNameMap(const char *fileName);

    //finish systems construction, create IBSystem ports and check sys type
    void constructSystems();

    //Returns: SUCCESS_CODE / ERR_CODE_IO_ERR
    static int OpenFile(const char *file_name,
                        ofstream& sout,
                        bool to_append,
                        string & err_message,
                        bool add_header = false,
                        ios_base::openmode mode = ios_base::out);

private:
    int parseSubnetLine(char *line);
    int constructGeneralSystem(IBSystem *p_system);
    int constructGeneralSystemNode(IBSystem *p_system, IBNode *p_node);
    int constructSystem(IBSystem *p_system,
                        IBSystem *p_systemTemplate, bool isHCA);
    int constructSystemNode(IBNode *p_node, IBNode *p_nodeT,
                            set_nodes_ptr &missingNodesPtr,
                            map_template_pnode &systemToTemplateMap,
                            bool &isModSys, bool &isNewSys);

};

enum SMP_AR_LID_STATE {
    AR_IB_LID_STATE_BOUNDED = 0x0,
    AR_IB_LID_STATE_FREE = 0x1,
    AR_IB_LID_STATE_STATIC = 0x2,
    AR_IB_LID_STATE_LAST
};



#endif /* IBDM_FABRIC_H */

