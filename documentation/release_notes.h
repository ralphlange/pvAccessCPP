/** @page pvarelease_notes Release Notes

Release 6.0.0 (UNRELEASED)
==========================

- Incompatible changes
 - Requires pvDataCPP >=7.0.0 due to headers moved from pvDataCPP into this module: requester.h, destoryable.h, and monitor.h
 - Major changes to shared_ptr ownership rules for epics::pvAccess::ChannelProvider and
   associated classes.  See
  - @ref providers
  - @ref providers_changes
 - Add new library pvAccessIOC for use with PVAClientRegister.dbd and PVAServerRegister.dbd.
   Necessary to avoid having pvAccess library depend on all IOC core libraries.
 - Added new library pvAccessCA with "ca" provider.  Main pvAccess library no longer depends on libca.
   Applications needing the "ca" provider must link against pvAccessCA and pvAccess.
   See examples/Makefile in the source tree.
   The headers associated with this library are: caChannel.h, caProvider.h, and caStatus.h
 - A number of headers which were previously installed, but considered "private", are no longer installed.
 - epics::pvAccess::ChannelProviderRegistry may no longer be sub-classed.
 - Removed access to singleton registry via getChannelProviderRegistry() and registerChannelProviderFactory()
   in favor of epics::pvAccess::ChannelProviderRegistry::clients() and epics::pvAccess::ChannelProviderRegistry::servers().
   The "pva" and "ca" providers are registered with the clients() singleton.
   epics::pvAccess::ServerContext() looks up names with the servers() singleton.
 - Removed deprecated epics::pvAccess::Properties
 - The data members of epics::pvAccess::MonitorElement become const, preventing these pointers from being re-targeted.
- Simplifications
 - use of the epics::pvAccess::ChannelRequester interface is optional
   and may be omitted when calling createChannel().
   Channel state change notifications are deliviered
   to individual operations via epics::pvAccess::ChannelBaseRequester::channelDisconnect()
 - Default implementions added for the following methods
  - epics::pvAccess::Lockable::lock() and epics::pvAccess::Lockable::unlock() which do nothing.
  - epics::pvAccess::Channel::getConnectionState() returns CONNECTED
  - epics::pvAccess::Channel::isConnected() uses getConnectionState()
  - epics::pvAccess::Channel::getField() which errors
  - epics::pvAccess::Channel::getAccessRights() which returns rw
 - Added epics::pvAccess::SimpleChannelProviderFactory template and
   epics::pvAccess::ChannelProviderRegistry::add() avoids need for custom
   factory.
 - Added epics::pvAccess::MonitorElement::Ref iterator/smart-pointer
   to ensure proper handling of calls Monitor::poll() and Monitor::release().
 - epics::pvAccess::PipelineMonitor "internal" is collapsed into epics::pvAccess::Monitor.
   PipelineMonitor becomes a typedef for Monitor.
 - epics::pvAccess::RPCService is now a sub-class of epics::pvAccess::RPCServiceAsync
- Additions
 - pv/pvAccess.h now provides macros OVERRIDE and FINAL which conditionally expand to the c++11 keywords override and final.
 - Deliver disconnect notifications to individual Operations (get/put/rpc/monitor/...) via
   new epics::pvAccess::ChannelBaseRequester::channelDisconnect()
 - New API for server creation via epics::pvAccess::ServerContext::create() allows direct specification
   of configuration and ChannelProvider(s).
 - Add epics::pvAccess::ServerContext::getCurrentConfig() to get actual configuration, eg. for use with a client.
 - Classes from moved headers requester.h, destoryable.h, and monitor.h added to epics::pvAccess namespace.
   Typedefs provided in epics::pvData namespace.
 - Added Client API based on pvac::ClientProvider
 - pv/pvaVersion.h defines VERSION_INT and PVACCESS_VERSION_INT
 - epics::pvAccess::RPCClient may be directly constructed.
 - epics::pvAccess::RPCServer allows epics::pvAccess::Configuration to be specified and access to ServerContext.
 - Added epics::pvAccess::Configuration::keys() to iterate configuration parameters (excluding environment variables).
 - Added epics::pvAccess::Destoryable::cleaner
- Deprecations
 - epics::pvAccess::GUID in favor of epics::pvAccess::ServerGUID due to win32 name conflict.

Release 5.0.0 (July 2016)
=========================

- Remote channel destroy support
- Multiple network inteface support
- Local multicast (repetitor) reimplemented
- Monitor reconnect when channel type changes fix
- C++11 compilation fixes
- Added version to pvaTools
- Memory management improved
- pipeline: ackAny argument percentage support
- Monitor overrun memory issues fixed
- CA provider destruction fixed
- Replaced LGPL wildcard matcher with simplistic EPICS version

Release 4.1.2 (Oct 2015) 
========================

- Improved Jenkins build support
- Removed QtCreated IDE configuration files
- Use of getSubField<> instead of e.g. getDoubleField()
- CA support for pvget, pvput and pvinfo.
- vxWorks/RTEMS compiler warnings resolved.
- Transport shutdown improved.
- CA DBR status fix.
- Monitor queue handling improved.
- Fixed bad performance on 10Gbit or faster networks.
- Async RPC service.

Release 4.0.5  (Dec 2014)
=========================
(Starting point for release notes.)


*/


/** @page providers_changes Changes to ChannelProvider ownership rules

@tableofcontents

Major series 6.x includes changes to the rules for when user code may
store strong and/or weak references to epics::pvAccess::ChannelProvider
and related classes.  These rules exist to prevent strong reference loops from forming.

@section providers_changes_requester Operation <-> Requester

One change is the reversal of the allowed relationship between
an Operation and its associated Requester (see @ref provider_roles_requester for definitions).

Prior to 6.0.0 an Operation was required to hold a strong reference to its Requester.
This prevented the Requester from holding a strong ref to the Operation.

This was found to be inconvienent and frequently violated.
Beginning with 6.0.0 an Operation is prohibited from holding a strong reference to its Requester.

@subsection providers_changes_requester_port Porting

Legecy code does not store a strong reference to Requesters will see that they are immediately destory.

An example where the Operation is a ChannelGet and the Requester is ChannelGetRequester.

@code
// Bad example!
epics::pvAccess::Channel::shared_pointer chan = ...;
epics::pvAccess::ChannelGet::shared_pointer get;
{
    epics::pvAccess::ChannelGetRequester::shared_pointer req(new MyRequester(...));
    get = chan->createChannelGet(req, epics::pvData::createRequest("field()"));
    // 'req' is only strong ref.
    // MyRequester::~MyRequester() called here
    // MyRequester::getDone() never called!
}
@endcode

It is necessary to maintain a strong reference to the ChannelRequester as long as callbacks are desired.

@note Legacy code could be modified to strong each Requester alongside the associated Operation.

New code may utilize the new ownership rules and store the Operation in a custom Requester.

@code
struct MyRequester : public epics::pvAccess::ChannelGetRequester {
  epics::pvAccess::ChannelGet::shared_pointer get;
  ...
};
epics::pvAccess::ChannelGetRequester::shared_pointer req(new MyRequester(...));
epics::pvAccess::Channel::shared_pointer chan = ...;
req->get = chan->createChannelGet(req, epics::pvData::createRequest("field()"));
@endcode

@section providers_changes_store Must store Operation reference

Beginning with 6.0.0, all create methods of
epics::pvAccess::ChannelProvider and 
epics::pvAccess::Channel
are required to return shared_ptr which are uniquely ownered by the caller
and are not stored internally.

Thus the caller of a create method must keep a reference to each Operation
or the Operation will be destoryed.

Prior to 6.0.0 some providers, notibly the "pva" provider, did not do this.
It was thus (sometimes) necessary to explicitly call a  destory() method
to fully dispose of an Operation.
Failure to do this resulted in a slow resource leak.

Beginning with 6.0.0 an Operation must not rely on user code to call a destory() method.

@subsection providers_changes_store_port Porting

Legecy code may be relying on these internal references to keep an Operation alive.

Beginning with 6.0.0 the shared_ptr returned by any create method must be stored or the Operation will
be immediately destroy'd.

@note Beginning with 6.0.0 use of the epics::pvAccess::ChannelRequester interface is optional
      and may be omitted when calling createChannel().

*/
