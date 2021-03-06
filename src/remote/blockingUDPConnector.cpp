/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * pvAccessCPP is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */

#include <sys/types.h>

#include <osiSock.h>

#define epicsExportSharedSymbols
#include <pv/blockingUDP.h>
#include <pv/remote.h>
#include <pv/logger.h>

using namespace std;
using namespace epics::pvData;

namespace {
struct closer {
    epics::pvAccess::Transport::shared_pointer P;
    closer(const epics::pvAccess::Transport::shared_pointer& P) :P(P) {}
    void operator()(epics::pvAccess::Transport*) {
        try{
            P->close();
        }catch(...){
            P.reset();
            throw;
        }
        P.reset();
    }
};
}

namespace epics {
namespace pvAccess {

Transport::shared_pointer BlockingUDPConnector::connect(TransportClient::shared_pointer const & /*client*/,
        ResponseHandler::shared_pointer const & responseHandler, osiSockAddr& bindAddress,
        int8 transportRevision, int16 /*priority*/) {

    LOG(logLevelDebug, "Creating datagram socket to: %s.",
        inetAddressToString(bindAddress).c_str());

    SOCKET socket = epicsSocketCreate(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(socket==INVALID_SOCKET) {
        char errStr[64];
        epicsSocketConvertErrnoToString(errStr, sizeof(errStr));
        LOG(logLevelError, "Error creating socket: %s.", errStr);
        return Transport::shared_pointer();
    }

    int optval = _broadcast ? 1 : 0;
    int retval = ::setsockopt(socket, SOL_SOCKET, SO_BROADCAST, (char *)&optval, sizeof(optval));
    if(retval<0)
    {
        char errStr[64];
        epicsSocketConvertErrnoToString(errStr, sizeof(errStr));
        LOG(logLevelError, "Error setting SO_BROADCAST: %s.", errStr);
        epicsSocketDestroy (socket);
        return Transport::shared_pointer();
    }

    /*
    IPv4 multicast addresses are defined by the leading address bits of 1110,
    originating from the classful network design of the early Internet when this
    group of addresses was designated as Class D. The Classless Inter-Domain Routing (CIDR)
    prefix of this group is 224.0.0.0/4.
    The group includes the addresses from 224.0.0.0 to 239.255.255.255.
    Address assignments from within this range are specified in RFC 5771,
    an Internet Engineering Task Force (IETF) Best Current Practice document (BCP 51).*/


    // set SO_REUSEADDR or SO_REUSEPORT, OS dependant
    if (_reuseSocket)
        epicsSocketEnableAddressUseForDatagramFanout(socket);

    retval = ::bind(socket, (sockaddr*)&(bindAddress.sa), sizeof(sockaddr));
    if(retval<0) {
        char ip[20];
        sockAddrToDottedIP((sockaddr*)&(bindAddress.sa), ip, sizeof(ip));
        char errStr[64];
        epicsSocketConvertErrnoToString(errStr, sizeof(errStr));
        LOG(logLevelError, "Error binding socket %s: %s.", ip, errStr);
        epicsSocketDestroy (socket);
        return Transport::shared_pointer();
    }

    // sockets are blocking by default
    BlockingUDPTransport::shared_pointer transport(new BlockingUDPTransport(_serverFlag, responseHandler,
            socket, bindAddress, transportRevision));

    // the worker thread holds a strong ref, which is released by transport->close()
    // note: casting to Transport* to prevent iOS version of shared_ptr from trying (and failing)
    //       to setup shared_from_this() using the wrapped pointer
    Transport::shared_pointer ret(static_cast<Transport*>(transport.get()), closer(transport));

    return ret;
}

}
}
