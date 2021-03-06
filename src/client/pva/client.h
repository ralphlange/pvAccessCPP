/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
#ifndef PVATESTCLIENT_H
#define PVATESTCLIENT_H

#include <stdexcept>

#include <epicsMutex.h>

#include <pv/pvData.h>
#include <pv/bitSet.h>

class epicsEvent;

namespace epics {namespace pvAccess {
class ChannelProvider;
class Channel;
class Monitor;
class Configuration;
}}//namespace epics::pvAccess

//! See @ref pvac API
namespace pvac {

/** @defgroup pvac Client API
 *
 * PVAccess network client (or other epics::pvAccess::ChannelProvider)
 *
 * Usage:
 *
 * 1. Construct a ClientProvider
 * 2. Use the ClientProvider to obtain a ClientChannel
 * 3. Use the ClientChannel to begin an get, put, rpc, or monitor operation
 *
 * Code examples
 *
 * - @ref examples_getme
 * - @ref examples_putme
 * - @ref examples_monitorme
 * @{
 */

class ClientProvider;

//! Handle for in-progress get/put/rpc operation
struct epicsShareClass Operation
{
    struct Impl
    {
        virtual ~Impl() {}
        virtual std::string name() const =0;
        virtual void cancel() =0;
    };

    Operation() {}
    Operation(const std::tr1::shared_ptr<Impl>&);
    ~Operation();
    //! Channel name
    std::string name() const;
    //! Immediate cancellation.
    //! Does not wait for remote confirmation.
    void cancel();

protected:
    std::tr1::shared_ptr<Impl> impl;
};

//! Information on put completion
struct PutEvent
{
    enum event_t {
        Fail,    //!< request ends in failure.  Check message
        Cancel,  //!< request cancelled before completion
        Success, //!< It worked!
    } event;
    std::string message;
    void *priv;
};

//! Information on get/rpc completion
struct epicsShareClass GetEvent : public PutEvent
{
    //! New data. NULL unless event==Success
    epics::pvData::PVStructure::const_shared_pointer value;
};

struct MonitorSync;

//! Handle for monitor subscription
struct epicsShareClass Monitor
{
    struct Impl;
    Monitor() {}
    Monitor(const std::tr1::shared_ptr<Impl>&);
    ~Monitor();

    //! Channel name
    std::string name() const;
    //! Immediate cancellation.
    //! Does not wait for remote confirmation.
    void cancel();
    /** updates root, changed, overrun
     *
     * @return true if root!=NULL
     * @note MonitorEvent::Data will not be repeated until poll()==false.
     */
    bool poll();
    //! true if all events received.
    //! Check after poll()==false
    bool complete() const;
    epics::pvData::PVStructure::const_shared_pointer root;
    epics::pvData::BitSet changed,
                          overrun;

private:
    std::tr1::shared_ptr<Impl> impl;
    friend struct MonitorSync;
};

//! Information on monitor subscription/queue change
struct MonitorEvent
{
    enum event_t {
        Fail=1,      //!< subscription ends in an error
        Cancel=2,    //!< subscription ends in cancellation
        Disconnect=4,//!< subscription interrupted to do lose of communication
        Data=8,      //!< Data queue not empty.  Call Monitor::poll()
    } event;
    std::string message; // set for event=Fail
};

/** Subscription usable w/o callbacks
 *
 * Basic usage is to call wait().
 * If true is returned, then the 'event', 'root', 'changed', and 'overrun'
 * members have been updated with a new event.
 * Test 'event.event' first to find out which kind of event has occured.
 */
struct epicsShareClass MonitorSync : public Monitor
{
    struct SImpl;
    MonitorSync() {}
    MonitorSync(const Monitor&, const std::tr1::shared_ptr<SImpl>&);
    ~MonitorSync();

    //! wait for new event
    bool wait();
    //! wait for new event
    //! @return false on timeout
    bool wait(double timeout);
    //! check if new event is available
    bool poll();

    //! Abort one call to wait()
    //! wait() will return with MonitorEvent::Fail
    void wake();

    //! most recent event
    //! updated only during wait() or poll()
    MonitorEvent event;
private:
    std::tr1::shared_ptr<SImpl> simpl;
};

//! information on connect/disconnect
struct ConnectEvent
{
    bool connected;
};

//! Thrown by blocking methods of ClientChannel on operation timeout
struct Timeout : public std::runtime_error
{
    Timeout();
};

/** Represents a single channel
 *
 * This class has two sets of methods, those which block for completion, and
 * those which use callbacks to signal completion.
 *
 * Those which block accept a 'timeout' argument (in seconds).
 *
 * Those which use callbacks accept a 'cb' argument and return an Operation or Monitor handle object.
 */
class epicsShareClass ClientChannel
{
    struct Impl;
    std::tr1::shared_ptr<Impl> impl;
    friend class ClientProvider;

    ClientChannel(const std::tr1::shared_ptr<Impl>& i) :impl(i) {}
public:
    //! Channel creation options
    struct epicsShareClass Options {
        short priority;
        std::string address;
        Options();
        bool operator<(const Options&) const;
    };

    //! Construct a null channel.  All methods throw.  May later be assigned from a valid ClientChannel
    ClientChannel() {}
    /** Construct a ClientChannel using epics::pvAccess::ChannelProvider::createChannel()
     *
     * Does not block.
     * @throw std::logic_error if the provider is NULL or name is an empty string
     * @throw std::runtime_error if the ChannelProvider can't provide
     */
    ClientChannel(const std::tr1::shared_ptr<epics::pvAccess::ChannelProvider>& provider,
                      const std::string& name,
                      const Options& opt = Options());
    ~ClientChannel();

    //! Channel name or an empty string
    std::string name() const;

    //! callback for get() and rpc()
    struct GetCallback {
        virtual ~GetCallback() {}
        //! get or rpc operation is complete
        virtual void getDone(const GetEvent& evt)=0;
    };

    //! Issue request to retrieve current PV value
    //! @param cb Completion notification callback.  Must outlive Operation (call Operation::cancel() to force release)
    //! @param pvRequest if NULL defaults to "field()".
    Operation get(GetCallback* cb,
                      epics::pvData::PVStructure::const_shared_pointer pvRequest = epics::pvData::PVStructure::const_shared_pointer());

    //! Block and retrieve current PV value
    //! @param timeout in seconds
    //! @param pvRequest if NULL defaults to "field()".
    //! @throws Timeout or std::runtime_error
    epics::pvData::PVStructure::const_shared_pointer
    get(double timeout = 3.0,
        epics::pvData::PVStructure::const_shared_pointer pvRequest = epics::pvData::PVStructure::const_shared_pointer());


    //! Start an RPC call
    //! @param cb Completion notification callback.  Must outlive Operation (call Operation::cancel() to force release)
    //! @param arguments encoded call arguments
    //! @param pvRequest if NULL defaults to "field()".
    Operation rpc(GetCallback* cb,
                      const epics::pvData::PVStructure::const_shared_pointer& arguments,
                      epics::pvData::PVStructure::const_shared_pointer pvRequest = epics::pvData::PVStructure::const_shared_pointer());

    //! Block and execute remote call
    //! @param timeout in seconds
    //! @param arguments encoded call arguments
    //! @param pvRequest if NULL defaults to "field()".
    epics::pvData::PVStructure::const_shared_pointer
    rpc(double timeout,
        const epics::pvData::PVStructure::const_shared_pointer& arguments,
        epics::pvData::PVStructure::const_shared_pointer pvRequest = epics::pvData::PVStructure::const_shared_pointer());

    //! callbacks for put()
    struct PutCallback {
        virtual ~PutCallback() {}
        struct Args {
            Args(epics::pvData::BitSet& tosend) :tosend(tosend) {}
            epics::pvData::PVStructure::const_shared_pointer root;
            epics::pvData::BitSet& tosend;
        };
        /** Server provides expected structure.
         *
         * Implementation must instanciate (or re-use) a PVStructure into args.root,
         * then initialize any necessary fields and set bits in args.tosend as approprate.
         *
         * If this method throws, then putDone() is called with PutEvent::Fail
         */
        virtual void putBuild(const epics::pvData::StructureConstPtr& build, Args& args) =0;
        //! Put operation is complete
        virtual void putDone(const PutEvent& evt)=0;
    };

    //! Initiate request to change PV
    //! @param cb Completion notification callback.  Must outlive Operation (call Operation::cancel() to force release)
    //! TODO: produce bitset to mask fields being set
    Operation put(PutCallback* cb,
                      epics::pvData::PVStructure::const_shared_pointer pvRequest = epics::pvData::PVStructure::const_shared_pointer());

    //! Put to the 'value' field and block until complete.
    //! Accepts a scalar value
    template<epics::pvData::ScalarType ID>
    inline void putValue(typename epics::pvData::meta::arg_type<typename epics::pvData::ScalarTypeTraits<ID>::type>::type value,
                  double timeout = 3.0,
                  epics::pvData::PVStructure::const_shared_pointer pvRequest = epics::pvData::PVStructure::const_shared_pointer())
    {
        putValue(static_cast<const void*>(&value), ID, timeout, pvRequest);
    }

    //! Put to the 'value' field and block until complete.
    //! Accepts untyped scalar value
    void putValue(const void* value, epics::pvData::ScalarType vtype,
                  double timeout = 3.0,
                  const epics::pvData::PVStructure::const_shared_pointer& pvRequest = epics::pvData::PVStructure::const_shared_pointer());

    //! Put to the 'value' field and block until complete.
    //! Accepts scalar array
    void putValue(const epics::pvData::shared_vector<const void>& value,
                  double timeout = 3.0,
                  const epics::pvData::PVStructure::const_shared_pointer& pvRequest = epics::pvData::PVStructure::const_shared_pointer());

    //! Monitor event notification
    struct MonitorCallback {
        virtual ~MonitorCallback() {}
        /** New monitor event
         *
         * - MonitorEvent::Fail - An Error occurred.  Check evt.message
         * - MonitorEvent::Cancel - Monitor::cancel() called
         * - MonitorEvent::Disconnect - Underlying ClientChannel becomes disconnected
         * - MonitorEvent::Data - FIFO becomes not empty.Call Monitor::poll()
         */
        virtual void monitorEvent(const MonitorEvent& evt)=0;
    };

    //! Begin subscription
    //! @param cb Completion notification callback.  Must outlive Operation (call Operation::cancel() to force release)
    Monitor monitor(MonitorCallback *cb,
                          epics::pvData::PVStructure::const_shared_pointer pvRequest = epics::pvData::PVStructure::const_shared_pointer());

    /** Begin subscription w/o callbacks
     *
     * @param event If not NULL, then subscription events are signaled to this epicsEvent.  Test with poll().
     *        Otherwise an internal epicsEvent is allocated for use with wait()
     *
     * @note For simple usage with a single MonitorSync, pass event=NULL and call wait().
     *       If more than one MonitorSync is being created, then pass a custom epicsEvent and use poll() to test
     *       which subscriptions have events pending.
     */
    MonitorSync monitor(const epics::pvData::PVStructure::const_shared_pointer& pvRequest = epics::pvData::PVStructure::const_shared_pointer(),
                        epicsEvent *event =0);

    //! Connection state change CB
    struct ConnectCallback {
        virtual ~ConnectCallback() {}
        virtual void connectEvent(const ConnectEvent& evt)=0;
    };
    //! Append to list of listeners
    //! @param cb Channel dis/connect notification callback.  Must outlive ClientChannel or call to removeConnectListener()
    void addConnectListener(ConnectCallback*);
    //! Remove from list of listeners
    void removeConnectListener(ConnectCallback*);

private:
    std::tr1::shared_ptr<epics::pvAccess::Channel> getChannel();
};

//! Central client context.
class epicsShareClass ClientProvider
{
    struct Impl;
    std::tr1::shared_ptr<Impl> impl;
public:

    /** Use named provider.
     *
     * @param providerName ChannelProvider name, may be prefixed with "clients:" or "servers:" to query
     *        epics::pvAccess::ChannelProviderRegistry::clients() or
     *        epics::pvAccess::ChannelProviderRegistry::servers().
     *        No prefix implies "clients:".
     */
    ClientProvider(const std::string& providerName,
                   const std::tr1::shared_ptr<epics::pvAccess::Configuration>& conf = std::tr1::shared_ptr<epics::pvAccess::Configuration>());
    ~ClientProvider();

    /** Get a new Channel
     *
     * Does not block.
     * Never returns NULL.
     * Uses internal Channel cache.
     */
    ClientChannel connect(const std::string& name,
                          const ClientChannel::Options& conf = ClientChannel::Options());

    //! Remove from channel cache
    bool disconnect(const std::string& name,
                    const ClientChannel::Options& conf = ClientChannel::Options());

    //! Clear channel cache
    void disconnect();
};

//! @}

}//namespace pvac

#endif // PVATESTCLIENT_H
