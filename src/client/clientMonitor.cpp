/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

#include <epicsMutex.h>
#include <epicsGuard.h>

#include <pv/current_function.h>
#include <pv/pvData.h>
#include <pv/bitSet.h>

#define epicsExportSharedSymbols
#include "pv/logger.h"
#include "pva/client.h"
#include "pv/pvAccess.h"

namespace pvd = epics::pvData;
namespace pva = epics::pvAccess;
typedef epicsGuard<epicsMutex> Guard;
typedef epicsGuardRelease<epicsMutex> UnGuard;

namespace pvac {

struct Monitor::Impl : public pva::MonitorRequester
{
    mutable epicsMutex mutex;
    pva::Channel::shared_pointer chan;
    operation_type::shared_pointer op;
    bool started, done, seenEmpty;

    ClientChannel::MonitorCallback *cb;
    MonitorEvent event;

    pva::MonitorElement::Ref last;

    Impl(ClientChannel::MonitorCallback* cb)
        :started(false)
        ,done(false)
        ,seenEmpty(false)
        ,cb(cb)
    {}
    virtual ~Impl() {cancel();}

    void callEvent(Guard& G, MonitorEvent::event_t evt = MonitorEvent::Fail)
    {
        ClientChannel::MonitorCallback *cb=this->cb;
        if(!cb) return;

        event.event = evt;

        if(evt==MonitorEvent::Fail || evt==MonitorEvent::Cancel)
            this->cb = 0; // last event

        try {
            UnGuard U(G);
            cb->monitorEvent(event);
            return;
        }catch(std::exception& e){
            if(!this->cb || evt==MonitorEvent::Fail) {
                LOG(pva::logLevelError, "Unhandled exception in ClientChannel::MonitorCallback::monitorEvent(): %s", e.what());
            } else {
               event.event = MonitorEvent::Fail;
               event.message = e.what();
            }
        }
        // continues error handling
        try {
            UnGuard U(G);
            cb->monitorEvent(event);
            return;
        }catch(std::exception& e){
            LOG(pva::logLevelError, "Unhandled exception following exception in ClientChannel::MonitorCallback::monitorEvent(): %s", e.what());
        }
    }

    void cancel()
    {
        Guard G(mutex);

        last.reset();

        if(started) {
            op->stop();
            started = false;
        }
        op->destroy();
        callEvent(G, MonitorEvent::Cancel);
    }

    virtual std::string getRequesterName() OVERRIDE FINAL
    {
        Guard G(mutex);
        return chan ? chan->getRequesterName() : "<dead>";
    }


    virtual void monitorConnect(pvd::Status const & status,
                                pva::MonitorPtr const & operation,
                                pvd::StructureConstPtr const & structure) OVERRIDE FINAL
    {
        Guard G(mutex);
        if(!cb || started || done) return;

        if(!status.isOK()) {
            event.message = status.getMessage();
        } else {
            event.message.clear();
        }
        if(!status.isSuccess()) {
            callEvent(G);

        } else {
            pvd::Status sts(operation->start());
            if(sts.isSuccess()) {
                started = true;
                last.attach(operation);
            } else {
                event.message = sts.getMessage();
                callEvent(G);
            }
        }
    }


    virtual void channelDisconnect(bool destroy) OVERRIDE FINAL
    {
        Guard G(mutex);
        if(!cb || done) return;
        event.message = "Disconnect";
        started = false;
        callEvent(G, MonitorEvent::Disconnect);
    }

    virtual void monitorEvent(pva::MonitorPtr const & monitor) OVERRIDE FINAL
    {
        Guard G(mutex);
        if(!cb || done) return;
        event.message.clear();

        callEvent(G, MonitorEvent::Data);
    }

    virtual void unlisten(pva::MonitorPtr const & monitor) OVERRIDE FINAL
    {
        Guard G(mutex);
        if(!cb || done) return;
        done = true;

        if(seenEmpty)
            callEvent(G, MonitorEvent::Data);
        // else // wait until final poll()
    }
};

Monitor::Monitor(const std::tr1::shared_ptr<Impl>& impl)
    :impl(impl)
{}

Monitor::~Monitor() {}


std::string Monitor::name() const
{
    return impl ? impl->chan->getChannelName() : "<NULL>";
}

void Monitor::cancel()
{
    changed.clear();
    overrun.clear();
    root.reset();
    if(impl) impl->cancel();
}

bool Monitor::poll()
{
    if(!impl) return false;
    Guard G(impl->mutex);

    if(!impl->done && impl->last.next()) {
        root = impl->last->pvStructurePtr;
        changed = *impl->last->changedBitSet;
        overrun = *impl->last->overrunBitSet;

    } else {
        root.reset();
        changed.clear();
        overrun.clear();
    }
    return impl->seenEmpty = !!root;
}

bool Monitor::complete() const
{
    if(!impl) return true;
    Guard G(impl->mutex);
    return impl->done && impl->seenEmpty;
}

Monitor
ClientChannel::monitor(MonitorCallback *cb,
                           epics::pvData::PVStructure::const_shared_pointer pvRequest)
{
    if(!impl) throw std::logic_error("Dead Channel");
    if(!pvRequest)
        pvRequest = pvd::createRequest("field()");

    std::tr1::shared_ptr<Monitor::Impl> ret(new Monitor::Impl(cb));
    ret->chan = getChannel();

    {
        Guard G(ret->mutex);
        ret->op = ret->chan->createMonitor(ret, std::tr1::const_pointer_cast<pvd::PVStructure>(pvRequest));
    }

    return Monitor(ret);
}

}//namespace pvac
