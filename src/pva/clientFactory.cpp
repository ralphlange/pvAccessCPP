/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * pvAccessCPP is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */

#include <string>

#include <epicsSignal.h>
#include <epicsExit.h>
#include <pv/lock.h>

#define epicsExportSharedSymbols
#include <pv/logger.h>
#include <pv/clientFactory.h>
#include <pv/clientContextImpl.h>

using namespace epics::pvData;
using namespace epics::pvAccess;

static
void pva_factory_cleanup(void*)
{
    try {
        ChannelProviderRegistry::clients()->remove("pva");
    } catch(std::exception& e) {
        LOG(logLevelWarn, "Error when unregister \"pva\" factory");
    }
}

void ClientFactory::start()
{
    epicsSignalInstallSigAlarmIgnore();
    epicsSignalInstallSigPipeIgnore();

    if(ChannelProviderRegistry::clients()->add("pva", createClientProvider, false))
        epicsAtExit(&pva_factory_cleanup, NULL);
}

void ClientFactory::stop()
{
    // unregister now done with exit hook
}

// automatically register on load
namespace {
struct pvaloader
{
    pvaloader() {
        ClientFactory::start();
    }
} pvaloaderinstance;
} // namespace

// perhaps useful during dynamic loading?
extern "C" {
void registerClientProvider_pva()
{
    try {
        ClientFactory::start();
    } catch(std::exception& e){
        std::cerr<<"Error loading pva: "<<e.what()<<"\n";
    }
}
} // extern "C"
