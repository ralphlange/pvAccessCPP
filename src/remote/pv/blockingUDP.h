/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * pvAccessCPP is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */

#ifndef BLOCKINGUDP_H_
#define BLOCKINGUDP_H_

#ifdef epicsExportSharedSymbols
#   define blockingUDPEpicsExportSharedSymbols
#   undef epicsExportSharedSymbols
#endif

#include <shareLib.h>
#include <osiSock.h>
#include <epicsThread.h>

#include <pv/noDefaultMethods.h>
#include <pv/byteBuffer.h>
#include <pv/lock.h>
#include <pv/event.h>
#include <pv/pvIntrospect.h>

#ifdef blockingUDPEpicsExportSharedSymbols
#   define epicsExportSharedSymbols
#	undef blockingUDPEpicsExportSharedSymbols
#endif

#include <shareLib.h>

#include <pv/remote.h>
#include <pv/pvaConstants.h>
#include <pv/inetAddressUtil.h>

namespace epics {
namespace pvAccess {

class BlockingUDPConnector;

enum InetAddressType { inetAddressType_all, inetAddressType_unicast, inetAddressType_broadcast_multicast };

class BlockingUDPTransport : public epics::pvData::NoDefaultMethods,
    public Transport,
    public TransportSendControl,
    public std::tr1::enable_shared_from_this<BlockingUDPTransport>,
    public epicsThreadRunable
{
public:
    POINTER_DEFINITIONS(BlockingUDPTransport);

private:
    friend class BlockingUDPConnector;
    BlockingUDPTransport(bool serverFlag,
                         ResponseHandler::shared_pointer const & responseHandler,
                         SOCKET channel, osiSockAddr &bindAddress,
                         short remoteTransportRevision);
public:

    static shared_pointer create(bool serverFlag,
                                 ResponseHandler::shared_pointer const & responseHandler,
                                 SOCKET channel, osiSockAddr& bindAddress,
                                 short remoteTransportRevision) EPICS_DEPRECATED
    {
        shared_pointer thisPointer(
            new BlockingUDPTransport(serverFlag, responseHandler,
        channel, bindAddress,
        remoteTransportRevision)
        );
        return thisPointer;
    }

    virtual ~BlockingUDPTransport();

    virtual bool isClosed() {
        return _closed.get();
    }

    /*
    void setReplyTransport(const Transport::shared_pointer& T)
    {
        _replyTransport = T;
    }
    */

    virtual const osiSockAddr* getRemoteAddress() const {
        return &_remoteAddress;
    }

    virtual const std::string& getRemoteName() const {
        return _remoteName;
    }

    virtual std::string getType() const {
        return std::string("udp");
    }

    virtual std::size_t getReceiveBufferSize() const {
        return _receiveBuffer.getSize();
    }

    virtual std::size_t getSocketReceiveBufferSize() const;

    virtual epics::pvData::int16 getPriority() const {
        return PVA_DEFAULT_PRIORITY;
    }

    virtual epics::pvData::int8 getRevision() const {
        return PVA_PROTOCOL_REVISION;
    }

    virtual void setRemoteRevision(epics::pvData::int8 /*revision*/) {
        // noop
    }

    virtual void setRemoteTransportReceiveBufferSize(
        std::size_t /*receiveBufferSize*/) {
        // noop for UDP (limited by 64k; MAX_UDP_SEND for PVA)
    }

    virtual void setRemoteTransportSocketReceiveBufferSize(
        std::size_t /*socketReceiveBufferSize*/) {
        // noop for UDP (limited by 64k; MAX_UDP_SEND for PVA)
    }

    virtual void aliveNotification() {
        // noop
    }

    virtual void changedTransport() {
        // noop
    }

    virtual bool verify(epics::pvData::int32 /*timeoutMs*/) {
        // noop
        return true;
    }

    virtual void verified(epics::pvData::Status const & /*status*/) {
        // noop
    }

    virtual void authNZInitialize(void*) {
        // noop
    }

    virtual void authNZMessage(epics::pvData::PVField::shared_pointer const & data) {
        // noop
    }

    virtual std::tr1::shared_ptr<SecuritySession> getSecuritySession() const {
        return std::tr1::shared_ptr<SecuritySession>();
    }

    // NOTE: this is not yet used for UDP
    virtual void setByteOrder(int byteOrder)  {
        // called from receive thread... or before processing
        _receiveBuffer.setEndianess(byteOrder);

        // sync?!
        _sendBuffer.setEndianess(byteOrder);
    }

    virtual void enqueueSendRequest(TransportSender::shared_pointer const & sender);

    virtual void flushSendQueue();

    void start();

    virtual void close();

    virtual void ensureData(std::size_t size) {
        if (_receiveBuffer.getRemaining() < size)
            throw std::underflow_error("no more data in UDP packet");
    }

    virtual void alignData(std::size_t alignment) {
        _receiveBuffer.align(alignment);
    }

    virtual bool directSerialize(epics::pvData::ByteBuffer* /*existingBuffer*/, const char* /*toSerialize*/,
                                 std::size_t /*elementCount*/, std::size_t /*elementSize*/)
    {
        return false;
    }

    virtual bool directDeserialize(epics::pvData::ByteBuffer* /*existingBuffer*/, char* /*deserializeTo*/,
                                   std::size_t /*elementCount*/, std::size_t /*elementSize*/)
    {
        return false;
    }

    virtual void startMessage(epics::pvData::int8 command, std::size_t ensureCapacity, epics::pvData::int32 payloadSize = 0);
    virtual void endMessage();

    virtual void flush(bool /*lastMessageCompleted*/) {
        // noop since all UDP requests are sent immediately
    }

    virtual void setRecipient(const osiSockAddr& sendTo) {
        _sendToEnabled = true;
        _sendTo = sendTo;
    }

    virtual void setLocalMulticastAddress(const osiSockAddr& sendTo) {
        _localMulticastAddressEnabled = true;
        _localMulticastAddress = sendTo;
    }

    virtual bool hasLocalMulticastAddress() const {
        return _localMulticastAddressEnabled;
    }

    virtual const osiSockAddr& getLocalMulticastAddress() const {
        return _localMulticastAddress;
    }

    virtual void flushSerializeBuffer() {
        // noop
    }

    virtual void ensureBuffer(std::size_t /*size*/) {
        // noop
    }

    virtual void alignBuffer(std::size_t alignment) {
        _sendBuffer.align(alignment);
    }

    virtual void cachedSerialize(
        const std::tr1::shared_ptr<const epics::pvData::Field>& field, epics::pvData::ByteBuffer* buffer)
    {
        // no cache
        field->serialize(buffer, this);
    }

    virtual std::tr1::shared_ptr<const epics::pvData::Field>
    cachedDeserialize(epics::pvData::ByteBuffer* buffer)
    {
        // no cache
        // TODO
        return epics::pvData::getFieldCreate()->deserialize(buffer, this);
    }

    virtual bool acquire(std::tr1::shared_ptr<TransportClient> const & /*client*/)
    {
        return false;
    }

    virtual void release(pvAccessID /*clientId*/) {}

    /**
     * Set ignore list.
     * @param address list of ignored addresses.
     */
    void setIgnoredAddresses(const InetAddrVector& addresses) {
        _ignoredAddresses = addresses;
    }

    /**
     * Get list of ignored addresses.
     * @return ignored addresses.
     */
    const InetAddrVector& getIgnoredAddresses() const {
        return _ignoredAddresses;
    }

    /**
     * Set tapped NIF list.
     * @param NIF address list to tap.
     */
    void setTappedNIF(const InetAddrVector& addresses) {
        _tappedNIF = addresses;
    }

    /**
     * Get list of tapped NIF addresses.
     * @return tapped NIF addresses.
     */
    const InetAddrVector& getTappedNIF() const {
        return _tappedNIF;
    }

    bool send(const char* buffer, size_t length, const osiSockAddr& address);

    bool send(epics::pvData::ByteBuffer* buffer, const osiSockAddr& address);

    bool send(epics::pvData::ByteBuffer* buffer, InetAddressType target = inetAddressType_all);

    /**
     * Get list of send addresses.
     * @return send addresses.
     */
    const InetAddrVector& getSendAddresses() {
        return _sendAddresses;
    }

    /**
     * Get bind address.
     * @return bind address.
     */
    const osiSockAddr* getBindAddress() const {
        return &_bindAddress;
    }

    bool isBroadcastAddress(const osiSockAddr* address, const InetAddrVector& broadcastAddresses)
    {
        for (size_t i = 0; i < broadcastAddresses.size(); i++)
            if (broadcastAddresses[i].ia.sin_addr.s_addr == address->ia.sin_addr.s_addr)
                return true;
        return false;
    }

    /**
     * Set list of send addresses.
     * @param addresses list of send addresses, non-<code>null</code>.
     */
    void setSendAddresses(const InetAddrVector& addresses) {
        std::vector<bool> isuni(addresses.size(), false);
        InetAddrVector broadcastAddresses;
        getBroadcastAddresses(broadcastAddresses, _channel, 0);
        for(size_t i=0, N=addresses.size(); i<N; i++)
            isuni[i] = !isBroadcastAddress(&addresses[i], broadcastAddresses)
                    && !isMulticastAddress(&addresses[i]);
        _sendAddresses = addresses;
        _isSendAddressUnicast.swap(isuni);
    }

    void join(const osiSockAddr & mcastAddr, const osiSockAddr & nifAddr);

    void setMutlicastNIF(const osiSockAddr & nifAddr, bool loopback);

protected:
    AtomicBoolean _closed;

    /**
     * Response handler.
     */
    ResponseHandler::shared_pointer _responseHandler;

    virtual void run();

private:
    bool processBuffer(Transport::shared_pointer const & transport, osiSockAddr& fromAddress, epics::pvData::ByteBuffer* receiveBuffer);

    void close(bool waitForThreadToComplete);

    // Context only used for logging in this class

    /**
     * Corresponding channel.
     */
    SOCKET _channel;

    /* When provided, this transport is used for replies (passed to handler)
     * instead of *this.  This feature is used in the situation where broadcast
     * traffic is received on one socket, but a different socket must be used
     * for unicast replies.
     *
    Transport::shared_pointer _replyTransport;
    */

    /**
     * Bind address.
     */
    osiSockAddr _bindAddress;

    /**
     * Remote address.
     */
    osiSockAddr _remoteAddress;
    std::string _remoteName;

    /**
     * Send addresses.
     */
    InetAddrVector _sendAddresses;

    std::vector<bool> _isSendAddressUnicast;

    /**
     * Ignore addresses.
     */
    InetAddrVector _ignoredAddresses;

    /**
     * Tapped NIF addresses.
     */
    InetAddrVector _tappedNIF;

    /**
     * Send address.
     */
    osiSockAddr _sendTo;
    bool _sendToEnabled;

    /**
     * Local multicast address.
     */
    osiSockAddr _localMulticastAddress;
    bool _localMulticastAddressEnabled;

    /**
     * Receive buffer.
     */
    epics::pvData::ByteBuffer _receiveBuffer;

    /**
     * Send buffer.
     */
    epics::pvData::ByteBuffer _sendBuffer;

    /**
     * Last message start position.
     */
    int _lastMessageStartPosition;

    /**
     * Used for process sync.
     */
    epics::pvData::Mutex _mutex;
    epics::pvData::Mutex _sendMutex;

    /**
     * Thread ID
     */
    std::auto_ptr<epicsThread> _thread;

    epics::pvData::int8 _clientServerWithEndianFlag;

};

class BlockingUDPConnector :
    public Connector,
    private epics::pvData::NoDefaultMethods {
public:
    POINTER_DEFINITIONS(BlockingUDPConnector);

    BlockingUDPConnector(
        bool serverFlag,
        bool reuseSocket,
        bool broadcast) :
        _serverFlag(serverFlag),
        _reuseSocket(reuseSocket),
        _broadcast(broadcast) {
    }

    virtual ~BlockingUDPConnector() {
    }

    /**
     * NOTE: transport client is ignored for broadcast (UDP).
     */
    virtual Transport::shared_pointer connect(TransportClient::shared_pointer const & client,
            ResponseHandler::shared_pointer const & responseHandler, osiSockAddr& bindAddress,
            epics::pvData::int8 transportRevision, epics::pvData::int16 priority);

private:

    /**
     * Client/server flag.
     */
    bool _serverFlag;

    /**
     * Reuse socket flag.
     */
    bool _reuseSocket;

    /**
     * Broadcast flag.
     */
    bool _broadcast;

};

typedef std::vector<BlockingUDPTransport::shared_pointer> BlockingUDPTransportVector;

void initializeUDPTransports(
    bool serverFlag,
    BlockingUDPTransportVector& udpTransports,
    const IfaceNodeVector& ifaceList,
    const ResponseHandler::shared_pointer& responseHandler,
    BlockingUDPTransport::shared_pointer& sendTransport,
    epics::pvData::int32& listenPort,
    bool autoAddressList,
    const std::string& addressList,
    const std::string& ignoreAddressList);


}
}

#endif /* BLOCKINGUDP_H_ */
