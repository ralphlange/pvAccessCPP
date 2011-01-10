
/* testRemoteClientImpl.cpp */
/* Author:  Matej Sekoranja Date: 2011.1.1 */


#include <pvAccess.h>
#include <iostream>
#include <sstream>
#include <showConstructDestruct.h>
#include <lock.h>
#include <standardPVField.h>
#include <memory>

#include <caConstants.h>
#include <timer.h>
#include <blockingUDP.h>
#include <blockingTCP.h>
#include <namedLockPattern.h>
#include <inetAddressUtil.h>
#include <hexDump.h>
#include <remote.h>
#include <channelSearchManager.h>
#include <clientContextImpl.h>

using namespace epics::pvData;
using namespace epics::pvAccess;



PVDATA_REFCOUNT_MONITOR_DEFINE(mockChannelProcess);

class ChannelImplProcess : public ChannelProcess
{
    private:
		ChannelProcessRequester* m_channelProcessRequester;
		PVStructure* m_pvStructure;
		PVScalar* m_valueField;
    
    private:
    ~ChannelImplProcess()
    {
        PVDATA_REFCOUNT_MONITOR_DESTRUCT(mockChannelProcess);
    }

    public:
    ChannelImplProcess(ChannelProcessRequester* channelProcessRequester, PVStructure *pvStructure, PVStructure *pvRequest) :
        m_channelProcessRequester(channelProcessRequester), m_pvStructure(pvStructure)
    {
        PVDATA_REFCOUNT_MONITOR_CONSTRUCT(mockChannelProcess);

        PVField* field = pvStructure->getSubField(String("value"));
        if (field == 0)
        {
            Status* noValueFieldStatus = getStatusCreate()->createStatus(STATUSTYPE_ERROR, "no 'value' field");
        	m_channelProcessRequester->channelProcessConnect(noValueFieldStatus, this);
        	delete noValueFieldStatus;
        	
        	// NOTE client must destroy this instance...
        	// do not access any fields and return ASAP
        	return;
        }
        
        if (field->getField()->getType() != scalar)
        {
            Status* notAScalarStatus = getStatusCreate()->createStatus(STATUSTYPE_ERROR, "'value' field not scalar type");
        	m_channelProcessRequester->channelProcessConnect(notAScalarStatus, this);
        	delete notAScalarStatus;
        	
        	// NOTE client must destroy this instance….
        	// do not access any fields and return ASAP
        	return;
        }
        
        m_valueField = static_cast<PVScalar*>(field);
        	
        // TODO pvRequest 
    	m_channelProcessRequester->channelProcessConnect(getStatusCreate()->getStatusOK(), this);
    }
    
    virtual void process(bool lastRequest)
    {
        switch (m_valueField->getScalar()->getScalarType())
        {
            case pvBoolean:
            {
                // negate
                PVBoolean *pvBoolean = static_cast<PVBoolean*>(m_valueField);
                pvBoolean->put(!pvBoolean->get());
                break;
            }
            case pvByte:
            {
                // increment by one
                PVByte *pvByte = static_cast<PVByte*>(m_valueField);
                pvByte->put(pvByte->get() + 1);
                break;
            }
            case pvShort:
            {
                // increment by one
                PVShort *pvShort = static_cast<PVShort*>(m_valueField);
                pvShort->put(pvShort->get() + 1);
                break;
            }
            case pvInt:
            {
                // increment by one
                PVInt *pvInt = static_cast<PVInt*>(m_valueField);
                pvInt->put(pvInt->get() + 1);
                break;
            }
            case pvLong:
            {
                // increment by one
                PVLong *pvLong = static_cast<PVLong*>(m_valueField);
                pvLong->put(pvLong->get() + 1);
                break;
            }
            case pvFloat:
            {
                // increment by one
                PVFloat *pvFloat = static_cast<PVFloat*>(m_valueField);
                pvFloat->put(pvFloat->get() + 1.0f);
                break;
            }
            case pvDouble:
            {
                // increment by one
                PVDouble *pvDouble = static_cast<PVDouble*>(m_valueField);
                pvDouble->put(pvDouble->get() + 1.0);
                break;
            }
            case pvString:
            {
                // increment by one
                PVString *pvString = static_cast<PVString*>(m_valueField);
                String val = pvString->get();
                if (val.empty())
                    pvString->put("gen0");
                else
                {
                    char c = val[0];
                    c++;
                    pvString->put("gen" + c);
                }
                break;
            }
            default:
                // noop
                break;
            
        } 
    	m_channelProcessRequester->processDone(getStatusCreate()->getStatusOK());
    	
    	if (lastRequest)
    	   destroy();
    }
    
    virtual void destroy()
    {
        delete this;
    }
    
};






PVDATA_REFCOUNT_MONITOR_DEFINE(mockChannelGet);

class ChannelImplGet : public ChannelGet
{
    private:
		ChannelGetRequester* m_channelGetRequester;
		PVStructure* m_pvStructure;
		BitSet* m_bitSet;
		volatile bool m_first;
    
    private:
    ~ChannelImplGet()
    {
        PVDATA_REFCOUNT_MONITOR_DESTRUCT(mockChannelGet);
    }

    public:
    ChannelImplGet(ChannelGetRequester* channelGetRequester, PVStructure *pvStructure, PVStructure *pvRequest) :
        m_channelGetRequester(channelGetRequester), m_pvStructure(pvStructure),
        m_bitSet(new BitSet(pvStructure->getNumberFields())), m_first(true)
    {
        PVDATA_REFCOUNT_MONITOR_CONSTRUCT(mockChannelGet);

        // TODO pvRequest 
    	m_channelGetRequester->channelGetConnect(getStatusCreate()->getStatusOK(), this, m_pvStructure, m_bitSet);
    }
    
    virtual void get(bool lastRequest)
    {
    	m_channelGetRequester->getDone(getStatusCreate()->getStatusOK());
    	if (m_first)
    	{
    		m_first = false;
    		m_bitSet->set(0);  // TODO
    	}
    	
    	if (lastRequest)
    	   destroy();
    }
    
    virtual void destroy()
    {
        delete m_bitSet;
        delete this;
    }
    
};








PVDATA_REFCOUNT_MONITOR_DEFINE(mockChannelPut);

class ChannelImplPut : public ChannelPut
{
    private:
		ChannelPutRequester* m_channelPutRequester;
		PVStructure* m_pvStructure;
		BitSet* m_bitSet;
		volatile bool m_first;
    
    private:
    ~ChannelImplPut()
    {
        PVDATA_REFCOUNT_MONITOR_DESTRUCT(mockChannelPut);
    }

    public:
    ChannelImplPut(ChannelPutRequester* channelPutRequester, PVStructure *pvStructure, PVStructure *pvRequest) :
        m_channelPutRequester(channelPutRequester), m_pvStructure(pvStructure),
        m_bitSet(new BitSet(pvStructure->getNumberFields())), m_first(true)
    {
        PVDATA_REFCOUNT_MONITOR_CONSTRUCT(mockChannelPut);

        // TODO pvRequest 
    	m_channelPutRequester->channelPutConnect(getStatusCreate()->getStatusOK(), this, m_pvStructure, m_bitSet);
    }
    
    virtual void put(bool lastRequest)
    {
    	m_channelPutRequester->putDone(getStatusCreate()->getStatusOK());
    	if (lastRequest)
    	   destroy();
    }
    
    virtual void get()
    {
    	m_channelPutRequester->getDone(getStatusCreate()->getStatusOK());
    }

    virtual void destroy()
    {
        delete m_bitSet;
        delete this;
    }
    
};







PVDATA_REFCOUNT_MONITOR_DEFINE(mockMonitor);

class MockMonitor : public Monitor, public MonitorElement
{
    private:
		MonitorRequester* m_monitorRequester;
		PVStructure* m_pvStructure;
		BitSet* m_changedBitSet;
		BitSet* m_overrunBitSet;
		volatile bool m_first;
		Mutex* m_lock;
		volatile int m_count;
    
    private:
    ~MockMonitor()
    {
        PVDATA_REFCOUNT_MONITOR_DESTRUCT(mockMonitor);
    }

    public:
    MockMonitor(MonitorRequester* monitorRequester, PVStructure *pvStructure, PVStructure *pvRequest) :
        m_monitorRequester(monitorRequester), m_pvStructure(pvStructure),
        m_changedBitSet(new BitSet(pvStructure->getNumberFields())),
        m_overrunBitSet(new BitSet(pvStructure->getNumberFields())),
        m_first(true),
        m_lock(new Mutex()),
        m_count(0)
    {
        PVDATA_REFCOUNT_MONITOR_CONSTRUCT(mockMonitor);

        m_changedBitSet->set(0);
        
        // TODO pvRequest 
    	m_monitorRequester->monitorConnect(getStatusCreate()->getStatusOK(), this, const_cast<Structure*>(m_pvStructure->getStructure()));
    }
    
    virtual Status* start()
    {
        // fist monitor
        m_monitorRequester->monitorEvent(this);
        
        // client needs to delete status, so passing shared OK instance is not right thing to do
        return getStatusCreate()->createStatus(STATUSTYPE_OK, "Monitor started.");
    }

    virtual Status* stop()
    {
        // client needs to delete status, so passing shared OK instance is not right thing to do
        return getStatusCreate()->createStatus(STATUSTYPE_OK, "Monitor stopped.");
    }

    virtual MonitorElement* poll()
    {
        Lock xx(m_lock);
        if (m_count)
        {
            return 0;
        }
        else
        {
            m_count++;
            return this;
        }
    }

    virtual void release(MonitorElement* monitorElement)
    {
        Lock xx(m_lock);
        if (m_count)
            m_count--;
    }
    
    virtual void destroy()
    {
        delete stop();
        
        delete m_lock;
        delete m_overrunBitSet;
        delete m_changedBitSet;
        delete this;
    }
    
    // ============ MonitorElement ============
    
    virtual PVStructure* getPVStructure()
    {
        return m_pvStructure;
    }
    
    virtual BitSet* getChangedBitSet()
    {
        return m_changedBitSet;
    }

    virtual BitSet* getOverrunBitSet()
    {
        return m_overrunBitSet;
    }
    
    
};



 // TODO consider std::unordered_map
typedef std::map<pvAccessID, ResponseRequest*> IOIDResponseRequestMap;



// TODO log
#define CALLBACK_GUARD(code) try { code } catch(...) { }



        class DebugResponse :  public ResponseHandler, private epics::pvData::NoDefaultMethods {
        public:
            /**
             * @param context
             */
            DebugResponse()
            {
            }

            virtual ~DebugResponse() {
            }

            virtual void handleResponse(osiSockAddr* responseFrom,
                    Transport* transport, int8 version, int8 command,
                    int payloadSize, epics::pvData::ByteBuffer* payloadBuffer)
                    {
                        
                 char ipAddrStr[48];
               ipAddrToDottedIP(&responseFrom->ia, ipAddrStr, sizeof(ipAddrStr));
 
                ostringstream prologue;
                prologue<<"Message [0x"<<hex<<(int)command<<", v0x"<<hex;
                prologue<<(int)version<<"] received from "<< ipAddrStr;
                       std::cout << "ole / " << prologue.str() << std::endl;

                hexDump(prologue.str(), "received",
                        (const int8*)payloadBuffer->getArray(),
                        payloadBuffer->getPosition(), payloadSize);
                       
                    }
        };

        class SearchResponseHandler :  public ResponseHandler, private epics::pvData::NoDefaultMethods {
            private:
            ClientContextImpl* m_context;
        public:
            SearchResponseHandler(ClientContextImpl* context) : m_context(context)
            {
            }

            virtual ~SearchResponseHandler() {
            }

            virtual void handleResponse(osiSockAddr* responseFrom,
                    Transport* transport, int8 version, int8 command,
                    int payloadSize, epics::pvData::ByteBuffer* payloadBuffer)
                    {
		// TODO super.handleResponse(responseFrom, transport, version, command, payloadSize, payloadBuffer);

		transport->ensureData(5);
		int32 searchSequenceId = payloadBuffer->getInt();
		bool found = payloadBuffer->getByte() != 0;
		if (!found)
			return;

		transport->ensureData((128+2*16)/8);

        osiSockAddr serverAddress;
        serverAddress.ia.sin_family = AF_INET;

		// 128-bit IPv6 address
		/*
		int8* byteAddress = new int8[16];
		for (int i = 0; i < 16; i++) 
		  byteAddress[i] = payloadBuffer->getByte(); };
		*/
		
        // IPv4 compatible IPv6 address expected
        // first 80-bit are 0
        if (payloadBuffer->getLong() != 0) return;
        if (payloadBuffer->getShort() != 0) return;
        if (payloadBuffer->getShort() != (int16)0xFFFF) return;
   
		// accept given address if explicitly specified by sender
        serverAddress.ia.sin_addr.s_addr = htonl(payloadBuffer->getInt());
        if (serverAddress.ia.sin_addr.s_addr == INADDR_ANY)
            serverAddress.ia.sin_addr = responseFrom->ia.sin_addr; 

        serverAddress.ia.sin_port = htons(payloadBuffer->getShort());

		// reads CIDs
		ChannelSearchManager* csm = m_context->getChannelSearchManager();
		int16 count = payloadBuffer->getShort();
		for (int i = 0; i < count; i++)
		{
			transport->ensureData(4);
			pvAccessID cid = payloadBuffer->getInt();
			csm->searchResponse(cid, searchSequenceId, version & 0x0F, &serverAddress);
		}

                        
       }
        };


/**
 * CA response handler - main handler which dispatches responses to appripriate handlers.
 * @author <a href="mailto:matej.sekoranjaATcosylab.com">Matej Sekoranja</a>
 * @version $Id: ClientResponseHandler.java,v 1.1 2010/05/03 14:45:40 mrkraimer Exp $
 */
class ClientResponseHandler : public ResponseHandler, private epics::pvData::NoDefaultMethods {
    private:
    
	/**
	 * Table of response handlers for each command ID.
	 */
	ResponseHandler** m_handlerTable;

	/**
	 * Context instance.
	 */
	ClientContextImpl* m_context;
	
	public:
	
	~ClientResponseHandler() {
	   delete[] m_handlerTable;
	}
	
	/**
	 * @param context
	 */
	ClientResponseHandler(ClientContextImpl* context) : m_context(context) {
		static ResponseHandler* badResponse = new DebugResponse();
		static ResponseHandler* dataResponse = 0; //new DataResponseHandler(context);
		
		#define HANDLER_COUNT 28
		m_handlerTable = new ResponseHandler*[HANDLER_COUNT];
		m_handlerTable[ 0] = badResponse; // TODO new BeaconHandler(context), /*  0 */
		m_handlerTable[ 1] = badResponse; // TODO new ConnectionValidationHandler(context), /*  1 */
		m_handlerTable[ 2] = badResponse; // TODO new NoopResponse(context, "Echo"), /*  2 */
		m_handlerTable[ 3] = badResponse; // TODO new NoopResponse(context, "Search"), /*  3 */
		m_handlerTable[ 4] = new SearchResponseHandler(context), /*  4 */
		m_handlerTable[ 5] = badResponse; // TODO new NoopResponse(context, "Introspection search"), /*  5 */
		m_handlerTable[ 6] = dataResponse; /*  6 - introspection search */
		m_handlerTable[ 7] = badResponse; // TODO new CreateChannelHandler(context), /*  7 */
		m_handlerTable[ 8] = badResponse; // TODO new NoopResponse(context, "Destroy channel"), /*  8 */ // TODO it might be useful to implement this...
		m_handlerTable[ 9] = badResponse; /*  9 */
		m_handlerTable[10] = dataResponse; /* 10 - get response */
		m_handlerTable[11] = dataResponse; /* 11 - put response */
		m_handlerTable[12] = dataResponse; /* 12 - put-get response */
		m_handlerTable[13] = dataResponse; /* 13 - monitor response */
		m_handlerTable[14] = dataResponse; /* 14 - array response */
		m_handlerTable[15] = badResponse; /* 15 - cancel request */
		m_handlerTable[16] = dataResponse; /* 16 - process response */
		m_handlerTable[17] = dataResponse; /* 17 - get field response */
		m_handlerTable[18] = badResponse; // TODO new MessageHandler(context), /* 18 - message to Requester */
		m_handlerTable[19] = badResponse; // TODO new MultipleDataResponseHandler(context), /* 19 - grouped monitors */
		m_handlerTable[20] = dataResponse; /* 20 - RPC response */
		m_handlerTable[21] = badResponse; /* 21 */
		m_handlerTable[22] = badResponse; /* 22 */
		m_handlerTable[23] = badResponse; /* 23 */
		m_handlerTable[24] = badResponse; /* 24 */
		m_handlerTable[25] = badResponse; /* 25 */
		m_handlerTable[26] = badResponse; /* 26 */
		m_handlerTable[27] = badResponse; /* 27 */
	}

    virtual void handleResponse(osiSockAddr* responseFrom,
                        Transport* transport, int8 version, int8 command,
                        int payloadSize, ByteBuffer* payloadBuffer)
    {
        int c = command+0;
			std::cout << "received  " << c << std::endl;
		if (command < 0 || command >= HANDLER_COUNT)
		{
			// TODO context.getLogger().fine("Invalid (or unsupported) command: " + command + ".");
			std::cout << "Invalid (or unsupported) command: " << command << "." << std::endl;
			// TODO remove debug output
			char buf[100];
			sprintf(buf, "Invalid CA header %d its payload buffer", command);
			hexDump(buf, (const int8*)(payloadBuffer->getArray()), payloadBuffer->getPosition(), payloadSize);
			return;
		}
		
		// delegate
		m_handlerTable[c]->handleResponse(responseFrom, transport, version, command, payloadSize, payloadBuffer);
	}
};

        class TCI : public TransportSendControl {
        public:
            virtual void flushSerializeBuffer() {
            }

            virtual void ensureBuffer(int size) {
            }

            virtual void startMessage(int8 command, int ensureCapacity){}
            virtual void endMessage() {}

            virtual void flush(bool lastMessageCompleted) {}

            virtual void setRecipient(const osiSockAddr& sendTo) {}
        };


        







PVDATA_REFCOUNT_MONITOR_DEFINE(channel);


/**
 * Context state enum.
 */
enum ContextState {
	/**
	 * State value of non-initialized context.
	 */
	CONTEXT_NOT_INITIALIZED,

	/**
	 * State value of initialized context.
	 */
	CONTEXT_INITIALIZED,

	/**
	 * State value of destroyed context.
	 */
	CONTEXT_DESTROYED
};
    

class TestClientContextImpl : public ClientContextImpl
{






/**
 * Implementation of CAJ JCA <code>Channel</code>.
 * @author <a href="mailto:matej.sekoranjaATcosylab.com">Matej Sekoranja</a>
 */
class TestChannelImpl : public ChannelImpl {
    private:
        
    	/**
    	 * Context.
    	 */
    	ClientContextImpl* m_context;

    	/**
    	 * Client channel ID.
    	 */
    	pvAccessID m_channelID;
    
    	/**
    	 * Channel name.
    	 */
        String m_name;
    
    	/**
    	 * Channel requester.
    	 */
         ChannelRequester* m_requester;

    	/**
    	 * Process priority.
    	 */
    	short m_priority;
    
    	/**
    	 * List of fixed addresses, if <code<0</code> name resolution will be used.
    	 */
    	InetAddrVector* m_addresses;
    	
    	/**
    	 * Connection status.
    	 */
    	ConnectionState m_connectionState;
    
    	/**
    	 * List of all channel's pending requests (keys are subscription IDs). 
    	 */
    	IOIDResponseRequestMap m_responseRequests;
    	
    	/**
    	 * Allow reconnection flag. 
    	 */
    	bool m_allowCreation;
    
    	/**
    	 * Reference counting.
    	 * NOTE: synced on <code>m_channelMutex</code>. 
    	 */
    	int m_references;
    
    	/* ****************** */
    	/* CA protocol fields */ 
    	/* ****************** */
    
    	/**
    	 * Server transport.
    	 */
    	Transport* m_transport;
    
    	/**
    	 * Server channel ID.
    	 */
    	pvAccessID m_serverChannelID;

    
        /**
         * Context sync. mutex.
         */
        Mutex m_channelMutex;

        /**
         * Flag indicting what message to send.
         */    
	   bool m_issueCreateMessage;
        
        // TODO mock
        PVStructure* m_pvStructure;
        
    private:
    ~TestChannelImpl() 
    {
        PVDATA_REFCOUNT_MONITOR_DESTRUCT(channel);
    }
    
    public:
    
	/**
	 * Constructor.
	 * @param context
	 * @param name
	 * @param listener
	 * @throws CAException
	 */
    TestChannelImpl(
            ClientContextImpl* context,
            pvAccessID channelID,
            String name,
			ChannelRequester* requester,
			short priority,
			InetAddrVector* addresses) :
        m_context(context),
        m_channelID(channelID),
        m_name(name),
        m_requester(requester),
        m_priority(priority),
        m_addresses(addresses),
        m_connectionState(NEVER_CONNECTED),
        m_allowCreation(true),
        m_references(1),
        m_transport(0),
        m_serverChannelID(0xFFFFFFFF),
        m_issueCreateMessage(true)
    {
        PVDATA_REFCOUNT_MONITOR_CONSTRUCT(channel);
     
		// register before issuing search request
		m_context->registerChannel(this);
		
		// connect
		connect();

    }
    
    virtual void destroy()
    {
        if (m_addresses) delete m_addresses;
        delete m_pvStructure;
        delete this;
    };

    virtual String getRequesterName()
    {
        return getChannelName();
    };
    
    virtual void message(String message,MessageType messageType) 
    {
        std::cout << "[" << getRequesterName() << "] message(" << message << ", " << messageTypeName[messageType] << ")" << std::endl; 
    }

    virtual ChannelProvider* getProvider() 
    {
        return m_context->getProvider();
    }

	// NOTE: synchronization guarantees that <code>transport</code> is non-<code>null</code> and <code>state == CONNECTED</code>.
    virtual epics::pvData::String getRemoteAddress()
    {
	   Lock guard(&m_channelMutex);
		if (m_connectionState != CONNECTED) {
		  static String emptyString;
			return emptyString;
		}
		else
		{
		  return inetAddressToString(m_transport->getRemoteAddress());
		}
	}

    virtual epics::pvData::String getChannelName()
    {
        return m_name;
    }

    virtual ChannelRequester* getChannelRequester()
    {
        return m_requester;
    }

    virtual ConnectionState getConnectionState()
    {
	   Lock guard(&m_channelMutex);
       return m_connectionState;
    }

    virtual bool isConnected()
    {
        return getConnectionState() == CONNECTED;
    }

    virtual AccessRights getAccessRights(epics::pvData::PVField *pvField)
    {
        return readWrite;
    }

	/**
	 * Get client channel ID.
	 * @return client channel ID.
	 */
	pvAccessID getChannelID() {
		return m_channelID;
	}
	
	virtual pvAccessID getSearchInstanceID() {
		return m_channelID;
	}

	virtual String getSearchInstanceName() {
	   return m_name;
	}

	void connect() {
	   Lock guard(&m_channelMutex);
		// if not destroyed...
		if (m_connectionState == DESTROYED)
			throw std::runtime_error("Channel destroyed.");
		else if (m_connectionState != CONNECTED)
			initiateSearch();
	}

	void disconnect() {
	   Lock guard(&m_channelMutex);
		// if not destroyed...
		if (m_connectionState == DESTROYED)
			throw std::runtime_error("Channel destroyed.");
		else if (m_connectionState == CONNECTED)
			disconnect(false, true);
	}	
	
	/**
	 * Create a channel, i.e. submit create channel request to the server.
	 * This method is called after search is complete.
	 * @param transport
	 */
	void createChannel(Transport* transport) 
	{
	   Lock guard(&m_channelMutex);

		// do not allow duplicate creation to the same transport
		if (!m_allowCreation)
			return;
		m_allowCreation = false;
		
		// check existing transport
		if (m_transport && m_transport != transport)
		{
			disconnectPendingIO(false);
			
			ReferenceCountingTransport* rct = dynamic_cast<ReferenceCountingTransport*>(m_transport);
			if (rct) rct->release(this);
		}
		else if (m_transport == transport)
		{
			// request to sent create request to same transport, ignore
			// this happens when server is slower (processing search requests) than client generating it
			return;
		}
		
		m_transport = transport;
		m_transport->enqueueSendRequest(this);
	}
	
	virtual void cancel() {
		// noop
	}

	virtual void timeout() {
		createChannelFailed();
	}

	/**
	 * Create channel failed.
	 */
	virtual void createChannelFailed()
	{
	   Lock guard(&m_channelMutex);

		cancel();
		// ... and search again
		initiateSearch();
	}

	/**
	 * Called when channel created succeeded on the server.
	 * <code>sid</code> might not be valid, this depends on protocol revision.
	 * @param sid
	 */
	void connectionCompleted(pvAccessID sid/*,  rights*/) 
	{
	   Lock guard(&m_channelMutex);

        bool allOK = false;
		try
		{
			// do this silently
			if (m_connectionState == DESTROYED)
				return;

			// store data
			m_serverChannelID = sid;
			//setAccessRights(rights);

			// user might create monitors in listeners, so this has to be done before this can happen
			// however, it would not be nice if events would come before connection event is fired
			// but this cannot happen since transport (TCP) is serving in this thread 
			resubscribeSubscriptions();
			setConnectionState(CONNECTED);
			allOK = true;
		}
		catch (...) {
		  // noop
            // TODO at least log something??		 
		}
		
		if (!allOK)
		{
			// end connection request
			cancel();
		}
	}

	/**
	 * @param force force destruction regardless of reference count
	 */
	void destroy(bool force) {
	   Lock guard(&m_channelMutex);
		if (m_connectionState == DESTROYED)
			throw std::runtime_error("Channel already destroyed.");
			
		// do destruction via context
		m_context->destroyChannel(this, force);
		
	}

	/**
	 * Increment reference.
	 */
	void acquire() {
	   Lock guard(&m_channelMutex);
		m_references++;
	}

	/**
	 * Actual destroy method, to be called <code>CAJContext</code>.
	 * @param force force destruction regardless of reference count
	 * @throws CAException
	 * @throws IllegalStateException
	 * @throws IOException
	 */
	void destroyChannel(bool force) {
	   Lock guard(&m_channelMutex);

		if (m_connectionState == DESTROYED)
			throw std::runtime_error("Channel already destroyed.");

		m_references--;
		if (m_references > 0 && !force)
			return;
		
		// stop searching...
		m_context->getChannelSearchManager()->unregisterChannel(this);
		cancel();

		disconnectPendingIO(true);

		if (m_connectionState == CONNECTED)
		{
			disconnect(false, true);
		}
		else if (m_transport)
		{
			// unresponsive state, do not forget to release transport
			ReferenceCountingTransport* rct = dynamic_cast<ReferenceCountingTransport*>(m_transport);
			if (rct) rct->release(this);
			m_transport = 0;
		}

		setConnectionState(DESTROYED);
		
		// unregister
		m_context->unregisterChannel(this);
	}

	/**
	 * Disconnected notification.
	 * @param initiateSearch	flag to indicate if searching (connect) procedure should be initiated
	 * @param remoteDestroy		issue channel destroy request.
	 */
	void disconnect(bool initiateSearch, bool remoteDestroy) {
	   Lock guard(&m_channelMutex);
		
		if (m_connectionState != CONNECTED && !m_transport)
			return;
			
		if (!initiateSearch) {
			// stop searching...
			m_context->getChannelSearchManager()->unregisterChannel(this);
			cancel();
		}
		setConnectionState(DISCONNECTED);

		disconnectPendingIO(false);

		// release transport
		if (m_transport)
		{
			if (remoteDestroy) {
				m_issueCreateMessage = false;
				m_transport->enqueueSendRequest(this);
			}
			
			ReferenceCountingTransport* rct = dynamic_cast<ReferenceCountingTransport*>(m_transport);
			if (rct) rct->release(this);
			m_transport = 0;
		}
		
		if (initiateSearch)
			this->initiateSearch();

	}

	/**
	 * Initiate search (connect) procedure.
	 */
	void initiateSearch()
	{
	   Lock guard(&m_channelMutex);

	   m_allowCreation = true;
		
		if (!m_addresses)
			m_context->getChannelSearchManager()->registerChannel(this);
			/* TODO
		else
			// TODO not only first
			// TODO minor version
			// TODO what to do if there is no channel, do not search in a loop!!! do this in other thread...!
			searchResponse(CAConstants.CA_MINOR_PROTOCOL_REVISION, addresses[0]);
			*/
	}

	virtual void searchResponse(int8 minorRevision, osiSockAddr* serverAddress) {
	   Lock guard(&m_channelMutex);
		Transport* transport = m_transport;
		if (transport)
		{
			// multiple defined PV or reconnect request (same server address)
			if (sockAddrAreIdentical(transport->getRemoteAddress(), serverAddress))
			{
				m_requester->message("More than one channel with name '" + m_name +
				  			   "' detected, additional response from: " + inetAddressToString(serverAddress), warningMessage);
				return;
			}
		}
		
		transport = m_context->getTransport(this, serverAddress, minorRevision, m_priority);
		if (!transport)
		{
			createChannelFailed();
			return;
		}

		// create channel
		createChannel(transport);
	}

	virtual void transportClosed() {
		disconnect(true, false);
	}

	virtual void transportChanged() {
		initiateSearch();
	}

	virtual void transportResponsive(Transport* transport) {
	   Lock guard(&m_channelMutex);
		if (m_connectionState == DISCONNECTED)
		{
			updateSubscriptions();
			
			// reconnect using existing IDs, data
			connectionCompleted(m_serverChannelID/*, accessRights*/);
		}
	}

	void transportUnresponsive() {
	   Lock guard(&m_channelMutex);
		if (m_connectionState == CONNECTED)
		{
			// NOTE: 2 types of disconnected state - distinguish them
			setConnectionState(DISCONNECTED);

			// ... CA notifies also w/ no access rights callback, although access right are not changed 
		}
	}

	/**
	 * Set connection state and if changed, notifies listeners.
	 * @param newState	state to set.
	 */
	void setConnectionState(ConnectionState connectionState)
	{
	   Lock guard(&m_channelMutex);
		if (m_connectionState != connectionState)
		{
			m_connectionState = connectionState;
			
			//bool connectionStatusToReport = (connectionState == CONNECTED);
			//if (connectionStatusToReport != lastReportedConnectionState)
			{
				//lastReportedConnectionState = connectionStatusToReport;
				// TODO via dispatcher ?!!!
				m_requester->channelStateChange(this, connectionState);
			}
		}
	}

	virtual void lock() {
		// noop
	}

	
	virtual void send(ByteBuffer* buffer, TransportSendControl* control) {
	   m_channelMutex.lock();
	   bool issueCreateMessage = m_issueCreateMessage;
	   m_channelMutex.unlock();
	   
		if (issueCreateMessage)
		{
			control->startMessage((int8)7, 2+4);
			
			// count
			buffer->putShort((int16)1);
			// array of CIDs and names
			buffer->putInt(m_channelID);
			SerializeHelper::serializeString(m_name, buffer, control);
			// send immediately
			// TODO
			control->flush(true);
		}
		else
		{
			control->startMessage((int8)8, 4+4);
			// SID
	   m_channelMutex.lock();
	   pvAccessID sid = m_serverChannelID;
	   m_channelMutex.unlock();
			buffer->putInt(sid);
			// CID
			buffer->putInt(m_channelID);
			// send immediately
			// TODO
			control->flush(true);
		}
	}

	virtual void unlock() {
		// noop
	}


	/**
	 * Disconnects (destroys) all channels pending IO.
	 * @param destroy	<code>true</code> if channel is being destroyed.
	 */
	void disconnectPendingIO(bool destroy)
	{
// TODO
	}
	
	/**
	 * Resubscribe subscriptions. 
	 */
	// TODO to be called from non-transport thread !!!!!!
	void resubscribeSubscriptions()
	{
// TODO
	}

	/**
	 * Update subscriptions. 
	 */
	// TODO to be called from non-transport thread !!!!!!
	void updateSubscriptions()
	{
// TODO
	}

	virtual void getField(GetFieldRequester *requester,epics::pvData::String subField)
    {
        requester->getDone(getStatusCreate()->getStatusOK(),m_pvStructure->getSubField(subField)->getField());
    }

    virtual ChannelProcess* createChannelProcess(
            ChannelProcessRequester *channelProcessRequester,
            epics::pvData::PVStructure *pvRequest)
    {
    	return new ChannelImplProcess(channelProcessRequester, m_pvStructure, pvRequest);
    }

    virtual ChannelGet* createChannelGet(
            ChannelGetRequester *channelGetRequester,
            epics::pvData::PVStructure *pvRequest)
    {
    	return new ChannelImplGet(channelGetRequester, m_pvStructure, pvRequest);
    }

    virtual ChannelPut* createChannelPut(
            ChannelPutRequester *channelPutRequester,
            epics::pvData::PVStructure *pvRequest)
    {
    	return new ChannelImplPut(channelPutRequester, m_pvStructure, pvRequest);
    }

    virtual ChannelPutGet* createChannelPutGet(
            ChannelPutGetRequester *channelPutGetRequester,
            epics::pvData::PVStructure *pvRequest)
    {
        // TODO
        return 0;
    }
    
    virtual ChannelRPC* createChannelRPC(ChannelRPCRequester *channelRPCRequester,
            epics::pvData::PVStructure *pvRequest)
    {
        // TODO
        return 0;
    }

    virtual epics::pvData::Monitor* createMonitor(
            epics::pvData::MonitorRequester *monitorRequester,
            epics::pvData::PVStructure *pvRequest)
    {
    	return new MockMonitor(monitorRequester, m_pvStructure, pvRequest);
    }

    virtual ChannelArray* createChannelArray(
            ChannelArrayRequester *channelArrayRequester,
            epics::pvData::PVStructure *pvRequest)
    {
        // TODO
        return 0;
    }
    
    
    
    virtual void printInfo() {
        String info;
        printInfo(&info);
        std::cout << info.c_str() << std::endl;
    }
    
    virtual void printInfo(epics::pvData::StringBuilder out) {
        //Lock lock(&m_channelMutex);
        //std::ostringstream ostr;
        //static String emptyString;
        
		out->append(  "CHANNEL  : "); out->append(m_name);
		out->append("\nSTATE    : "); out->append(ConnectionStateNames[m_connectionState]);
		if (m_connectionState == CONNECTED)
		{
       		out->append("\nADDRESS  : "); out->append(getRemoteAddress());
			//out->append("\nRIGHTS   : "); out->append(getAccessRights());
		}
		out->append("\n");
    }
};
    

    class ChannelProviderImpl;
    
    class ChannelImplFind : public ChannelFind
    {
        public:
        ChannelImplFind(ChannelProvider* provider) : m_provider(provider)
        {
        }
    
        virtual void destroy()
        {
            // one instance for all, do not delete at all
        }
       
        virtual ChannelProvider* getChannelProvider()
        {
            return m_provider;
        };
        
        virtual void cancelChannelFind()
        {
            throw std::runtime_error("not supported");
        }
        
        private:
        
        // only to be destroyed by it
        friend class ChannelProviderImpl;
        virtual ~ChannelImplFind() {}
        
        ChannelProvider* m_provider;  
    };
        
    class ChannelProviderImpl : public ChannelProvider {
        public:
        
        ChannelProviderImpl(ClientContextImpl* context) :
            m_context(context) {
        }
    
        virtual epics::pvData::String getProviderName()
        {
            return "ChannelProviderImpl";
        }
        
        virtual void destroy()
        {
            delete this;
        }
        
        virtual ChannelFind* channelFind(
            epics::pvData::String channelName,
            ChannelFindRequester *channelFindRequester)
        {
            m_context->checkChannelName(channelName);
    
    		if (!channelFindRequester)
    			throw std::runtime_error("null requester");
            
            std::auto_ptr<Status> errorStatus(getStatusCreate()->createStatus(STATUSTYPE_ERROR, "not implemented", 0));
            channelFindRequester->channelFindResult(errorStatus.get(), 0, false);
            return 0;
        }
    
        virtual Channel* createChannel(
            epics::pvData::String channelName,
            ChannelRequester *channelRequester,
            short priority)
        {
            return createChannel(channelName, channelRequester, priority, emptyString);
        }
    
        virtual Channel* createChannel(
            epics::pvData::String channelName,
            ChannelRequester *channelRequester,
            short priority,
            epics::pvData::String address)
        {
            // TODO support addressList
    		Channel* channel = m_context->createChannelInternal(channelName, channelRequester, priority, 0);
    		if (channel)
          	     channelRequester->channelCreated(getStatusCreate()->getStatusOK(), channel);
    	    return channel;
    		
    		// NOTE it's up to internal code to respond w/ error to requester and return 0 in case of errors
        }
        
        private:
        ~ChannelProviderImpl() {};
        
        /* TODO static*/ String emptyString;
        ClientContextImpl* m_context;
    };

    public:
    
    TestClientContextImpl() : 
    	m_addressList(""), m_autoAddressList(true), m_connectionTimeout(30.0f), m_beaconPeriod(15.0f),
    	m_broadcastPort(CA_BROADCAST_PORT), m_receiveBufferSize(MAX_TCP_RECV), m_timer(0),
    	m_broadcastTransport(0), m_searchTransport(0), m_connector(0), m_transportRegistry(0),
    	m_namedLocker(0), m_lastCID(0), m_lastIOID(0), m_channelSearchManager(0),
        m_version(new Version("CA Client", "cpp", 0, 0, 0, 1)),
        m_provider(new ChannelProviderImpl(this)),
        m_contextState(CONTEXT_NOT_INITIALIZED)
    {
        loadConfiguration();
    }
    
    virtual Version* getVersion() {
        return m_version;
    }

    virtual ChannelProvider* getProvider() {
        Lock lock(&m_contextMutex);
        return m_provider;
    }

    virtual Timer* getTimer()
    {
        Lock lock(&m_contextMutex);
        return m_timer;
    }

    virtual TransportRegistry* getTransportRegistry()
    {
        Lock lock(&m_contextMutex);
        return m_transportRegistry;
    }

    virtual BlockingUDPTransport* getSearchTransport()
    {
        Lock lock(&m_contextMutex);
        return m_searchTransport;
    }

    virtual void initialize() {
        Lock lock(&m_contextMutex);
        
		if (m_contextState == CONTEXT_DESTROYED)
			throw std::runtime_error("Context destroyed.");
		else if (m_contextState == CONTEXT_INITIALIZED)
			throw std::runtime_error("Context already initialized.");
		
		internalInitialize();

		m_contextState = CONTEXT_INITIALIZED;
    }
        
    virtual void printInfo() {
        String info;
        printInfo(&info);
        std::cout << info.c_str() << std::endl;
    }
    
    virtual void printInfo(epics::pvData::StringBuilder out) {
        Lock lock(&m_contextMutex);
        std::ostringstream ostr;
        static String emptyString;
        
		out->append(  "CLASS : ::epics::pvAccess::ClientContextImpl");
		out->append("\nVERSION : "); out->append(m_version->getVersionString());
		out->append("\nADDR_LIST : "); ostr << m_addressList; out->append(ostr.str()); ostr.str(emptyString);
		out->append("\nAUTO_ADDR_LIST : ");  out->append(m_autoAddressList ? "true" : "false");
		out->append("\nCONNECTION_TIMEOUT : "); ostr << m_connectionTimeout; out->append(ostr.str()); ostr.str(emptyString);
		out->append("\nBEACON_PERIOD : "); ostr << m_beaconPeriod; out->append(ostr.str()); ostr.str(emptyString);
		out->append("\nBROADCAST_PORT : "); ostr << m_broadcastPort; out->append(ostr.str()); ostr.str(emptyString);
		out->append("\nRCV_BUFFER_SIZE : "); ostr << m_receiveBufferSize; out->append(ostr.str()); ostr.str(emptyString);
		out->append("\nSTATE : ");
		switch (m_contextState)
		{
			case CONTEXT_NOT_INITIALIZED:
				out->append("CONTEXT_NOT_INITIALIZED");
				break;
			case CONTEXT_INITIALIZED:
				out->append("CONTEXT_INITIALIZED");
				break;
			case CONTEXT_DESTROYED:
				out->append("CONTEXT_DESTROYED");
				break;
			default:
				out->append("UNKNOWN");
		}
		out->append("\n");
    }
    
    virtual void destroy()
    {
       m_contextMutex.lock();

		if (m_contextState == CONTEXT_DESTROYED)
		{
		    m_contextMutex.unlock();
			throw std::runtime_error("Context already destroyed.");
		}
		
		// go into destroyed state ASAP			
		m_contextState =  CONTEXT_DESTROYED;

		internalDestroy();
    }
    
    virtual void dispose()
    {
        destroy();
    }    
           
    private:
    ~TestClientContextImpl() {};
    
    void loadConfiguration() {
        // TODO
        /*
		m_addressList = config->getPropertyAsString("EPICS4_CA_ADDR_LIST", m_addressList);
		m_autoAddressList = config->getPropertyAsBoolean("EPICS4_CA_AUTO_ADDR_LIST", m_autoAddressList);
		m_connectionTimeout = config->getPropertyAsFloat("EPICS4_CA_CONN_TMO", m_connectionTimeout);
		m_beaconPeriod = config->getPropertyAsFloat("EPICS4_CA_BEACON_PERIOD", m_beaconPeriod);
		m_broadcastPort = config->getPropertyAsInteger("EPICS4_CA_BROADCAST_PORT", m_broadcastPort);
		m_receiveBufferSize = config->getPropertyAsInteger("EPICS4_CA_MAX_ARRAY_BYTES", m_receiveBufferSize);
        */
    }

    void internalInitialize() {
		
		m_timer = new Timer("pvAccess-client timer", lowPriority);
		m_connector = new BlockingTCPConnector(this, m_receiveBufferSize, m_beaconPeriod);
		m_transportRegistry = new TransportRegistry();
		m_namedLocker = new NamedLockPattern<String>();
        
		// setup UDP transport
		initializeUDPTransport();

		// setup search manager
		m_channelSearchManager = new ChannelSearchManager(this);
    }
    
	/**
	 * Initialized UDP transport (broadcast socket and repeater connection).
	 */
	void initializeUDPTransport() {
		// setup UDP transport
		try
		{
			// where to bind (listen) address
            osiSockAddr listenLocalAddress;
            listenLocalAddress.ia.sin_family = AF_INET;
            listenLocalAddress.ia.sin_port = htons(m_broadcastPort);
            listenLocalAddress.ia.sin_addr.s_addr = htonl(INADDR_ANY);
        
			// where to send address
            SOCKET socket = epicsSocketCreate(AF_INET, SOCK_DGRAM, 0);
            InetAddrVector* broadcastAddresses = getBroadcastAddresses(socket);
    cout<<"Broadcast addresses: "<<broadcastAddresses->size()<<endl;
    for(size_t i = 0; i<broadcastAddresses->size(); i++) {
        broadcastAddresses->at(i)->ia.sin_port = htons(m_broadcastPort);
        cout<<"Broadcast address: ";
        cout<<inetAddressToString(broadcastAddresses->at(i))<<endl;
    }
           //InetAddrVector* broadcastAddresses = getSocketAddressList("255.255.255.255", m_broadcastPort);

/// TOD !!!! addresses !!!!! by pointer and not copied

			BlockingUDPConnector* broadcastConnector = new BlockingUDPConnector(true, broadcastAddresses, true);
			
			m_broadcastTransport = (BlockingUDPTransport*)broadcastConnector->connect(
						0, new ClientResponseHandler(this),
						listenLocalAddress, CA_MINOR_PROTOCOL_REVISION,
						CA_DEFAULT_PRIORITY);

			BlockingUDPConnector* searchConnector = new BlockingUDPConnector(false, broadcastAddresses, true);
			
			// undefined address
            osiSockAddr undefinedAddress;
            undefinedAddress.ia.sin_family = AF_INET;
            undefinedAddress.ia.sin_port = htons(0);
            undefinedAddress.ia.sin_addr.s_addr = htonl(INADDR_ANY);
            
			m_searchTransport = (BlockingUDPTransport*)searchConnector->connect(
										0, new ClientResponseHandler(this),
										undefinedAddress, CA_MINOR_PROTOCOL_REVISION,
										CA_DEFAULT_PRIORITY);

			// set broadcast address list
			if (!m_addressList.empty())
			{
				// if auto is true, add it to specified list
				InetAddrVector* appendList = 0;
				if (m_autoAddressList)
					appendList = m_broadcastTransport->getSendAddresses();
				
				InetAddrVector* list = getSocketAddressList(m_addressList, m_broadcastPort, appendList);
				// TODO delete !!!!
				if (list && list->size()) {
					m_broadcastTransport->setBroadcastAddresses(list);
					m_searchTransport->setBroadcastAddresses(list);
				}
			}

			m_broadcastTransport->start();
			m_searchTransport->start();

		}
		catch (...)
		{
		  // TODO
		}
	}
    
    void internalDestroy() {
        
		// stop searching
		if (m_channelSearchManager)
			delete m_channelSearchManager; //->destroy();
        
		// stop timer
		if (m_timer) 
			delete m_timer;

		//
		// cleanup
		//
		
		// this will also close all CA transports
		destroyAllChannels();
		
		// TODO destroy !!!
		if (m_broadcastTransport)
			delete m_broadcastTransport; //->destroy(true);
		if (m_searchTransport)
			delete m_searchTransport; //->destroy(true);
			
		if (m_namedLocker) delete m_namedLocker;
		if (m_transportRegistry) delete m_transportRegistry;
		if (m_connector) delete m_connector;

        m_provider->destroy();
        delete m_version;
		m_contextMutex.unlock();
        delete this;
    }
    
    void destroyAllChannels() {
        // TODO
    }

	/**
	 * Check channel name.
	 */
	void checkChannelName(String& name) {
		if (name.empty())
			throw std::runtime_error("null or empty channel name");
		else if (name.length() > UNREASONABLE_CHANNEL_NAME_LENGTH)
			throw std::runtime_error("name too long");
	}

	/**
	 * Check context state and tries to establish necessary state.
	 */
	void checkState() {
        Lock lock(&m_contextMutex); // TODO check double-lock?!!!

		if (m_contextState == CONTEXT_DESTROYED)
			throw std::runtime_error("Context destroyed.");
		else if (m_contextState == CONTEXT_NOT_INITIALIZED)
			initialize();
	}

	/**
	 * Register channel.
	 * @param channel
	 */
	void registerChannel(ChannelImpl* channel)
	{
	   Lock guard(&m_cidMapMutex);
	   m_channelsByCID[channel->getChannelID()] = channel;
	}

	/**
	 * Unregister channel.
	 * @param channel
	 */
	void unregisterChannel(ChannelImpl* channel)
	{
	   Lock guard(&m_cidMapMutex);
       m_channelsByCID.erase(channel->getChannelID());
	}

	/**
	 * Searches for a channel with given channel ID.
	 * @param channelID CID.
	 * @return channel with given CID, <code>0</code> if non-existent.
	 */
	ChannelImpl* getChannel(pvAccessID channelID)
	{
	   Lock guard(&m_cidMapMutex);
	   CIDChannelMap::iterator it = m_channelsByCID.find(channelID); 
	   return (it == m_channelsByCID.end() ? 0 : it->second);
	}

	/**
	 * Generate Client channel ID (CID).
	 * @return Client channel ID (CID). 
	 */
	pvAccessID generateCID()
	{
        Lock guard(&m_cidMapMutex);
        
        // search first free (theoretically possible loop of death)
        while (m_channelsByCID.find(++m_lastCID) != m_channelsByCID.end());
        // reserve CID
        m_channelsByCID[m_lastCID] = 0;
        return m_lastCID;
	}

	/**
	 * Free generated channel ID (CID).
	 */
	void freeCID(int cid)
	{
        Lock guard(&m_cidMapMutex);
        m_channelsByCID.erase(cid);
	}

	
	/**
	 * Get, or create if necessary, transport of given server address.
	 * @param serverAddress	required transport address
	 * @param priority process priority.
	 * @return transport for given address
	 */
	Transport* getTransport(TransportClient* client, osiSockAddr* serverAddress, int16 minorRevision, int16 priority)
	{
		try
		{
			return m_connector->connect(client, new ClientResponseHandler(this), *serverAddress, minorRevision, priority);
		}
		catch (...)
		{
		  // TODO log
		  printf("failed to get transport\n");
		  return 0;
		}
	}
	
		/**
	 * Internal create channel.
	 */
	// TODO no minor version with the addresses
	// TODO what if there is an channel with the same name, but on different host!
	ChannelImpl* createChannelInternal(String name, ChannelRequester* requester, short priority,
			InetAddrVector* addresses) { // TODO addresses

		checkState();
		checkChannelName(name);
		
		if (requester == 0)
			throw std::runtime_error("null requester");
		
		if (priority < ChannelProvider::PRIORITY_MIN || priority > ChannelProvider::PRIORITY_MAX)
			throw std::range_error("priority out of bounds");

		bool lockAcquired = true; // TODO namedLocker->acquireSynchronizationObject(name, LOCK_TIMEOUT);
		if (lockAcquired)
		{ 
			try
			{
		  	    pvAccessID cid = generateCID();
				return new TestChannelImpl(this, cid, name, requester, priority, addresses);
			}
			catch(...) {
			 // TODO
			 return 0;
			}
			// TODO namedLocker.releaseSynchronizationObject(name);	
		}
		else
		{     
		  // TODO is this OK?
			throw std::runtime_error("Failed to obtain synchronization lock for '" + name + "', possible deadlock.");
		}
	}

	/**
	 * Destroy channel.
	 * @param channel
	 * @param force
	 * @throws CAException
	 * @throws IllegalStateException
	 */
	void destroyChannel(ChannelImpl* channel, bool force) {
			
		String name = channel->getChannelName();
		bool lockAcquired = true; //namedLocker->acquireSynchronizationObject(name, LOCK_TIMEOUT);
		if (lockAcquired)
		{ 
			try
			{   
				channel->destroyChannel(force);
			}
			catch(...) {
			 // TODO
			}
			// TODO	namedLocker->releaseSynchronizationObject(channel.getChannelName());	
		}
		else
		{ 
		  // TODO is this OK?    
		  throw std::runtime_error("Failed to obtain synchronization lock for '" + name + "', possible deadlock.");
		}
	}

	/**
	 * Get channel search manager.
	 * @return channel search manager.
	 */
	ChannelSearchManager* getChannelSearchManager() {
		return m_channelSearchManager;
	}
	
	/**
	 * A space-separated list of broadcast address for process variable name resolution.
	 * Each address must be of the form: ip.number:port or host.name:port
	 */
	String m_addressList;
	
	/**
	 * Define whether or not the network interfaces should be discovered at runtime. 
	 */
	bool m_autoAddressList;

	/**
	 * If the context doesn't see a beacon from a server that it is connected to for
	 * connectionTimeout seconds then a state-of-health message is sent to the server over TCP/IP.
	 * If this state-of-health message isn't promptly replied to then the context will assume that
	 * the server is no longer present on the network and disconnect.
	 */
	float m_connectionTimeout;
	
	/**
	 * Period in second between two beacon signals.
	 */
	float m_beaconPeriod;
	
	/**
	 * Broadcast (beacon, search) port number to listen to.
	 */
	int m_broadcastPort;
	
	/**
	 * Receive buffer size (max size of payload).
	 */
	int m_receiveBufferSize;
	
	/**
	 * Timer.
	 */
	Timer* m_timer;

	/**
	 * Broadcast transport needed to listen for broadcasts.
	 */
	BlockingUDPTransport* m_broadcastTransport;
	
	/**
	 * UDP transport needed for channel searches.
	 */
	BlockingUDPTransport* m_searchTransport;

	/**
	 * CA connector (creates CA virtual circuit).
	 */
	BlockingTCPConnector* m_connector;

	/**
	 * CA transport (virtual circuit) registry.
	 * This registry contains all active transports - connections to CA servers. 
	 */
	TransportRegistry* m_transportRegistry;

	/**
	 * Context instance.
	 */
	NamedLockPattern<String>* m_namedLocker;

	/**
	 * Context instance.
	 */
	static const int LOCK_TIMEOUT = 20 * 1000;	// 20s

	/**
	 * Map of channels (keys are CIDs).
	 */
	 // TODO consider std::unordered_map
	typedef std::map<pvAccessID, ChannelImpl*> CIDChannelMap;
	CIDChannelMap m_channelsByCID;

    /**
     *  CIDChannelMap mutex.
     */
    Mutex m_cidMapMutex;

	/**
	 * Last CID cache. 
	 */
	pvAccessID m_lastCID;

	/**
	 * Map of pending response requests (keys are IOID).
	 */
	 // TODO consider std::unordered_map
	typedef std::map<pvAccessID, ResponseRequest*> IOIDResponseRequestMap;
	IOIDResponseRequestMap m_pendingResponseRequests;

	/**
	 * Last IOID cache. 
	 */
	pvAccessID m_lastIOID;

	/**
	 * Channel search manager.
	 * Manages UDP search requests.
	 */
	ChannelSearchManager* m_channelSearchManager;

	/**
	 * Beacon handler map.
	 */
	 // TODO consider std::unordered_map
//	typedef std::map<osiSockAddr, BeaconHandlerImpl*> AddressBeaconHandlerMap;
//	AddressBeaconHandlerMap m_beaconHandlers;
	
	/**
	 * Version.
	 */
    Version* m_version;

	/**
	 * Provider implementation.
	 */
    ChannelProviderImpl* m_provider;
    
    /**
     * Context state.
     */
    ContextState m_contextState;
    
    /**
     * Context sync. mutex.
     */
    Mutex m_contextMutex;

    friend class ChannelProviderImpl;
};


class ChannelFindRequesterImpl : public ChannelFindRequester
{
    virtual void channelFindResult(epics::pvData::Status *status,ChannelFind *channelFind,bool wasFound)
    {
        std::cout << "[ChannelFindRequesterImpl] channelFindResult("
                  << status->toString() << ", ..., " << wasFound << ")" << std::endl; 
    }
};

class ChannelRequesterImpl : public ChannelRequester
{
    virtual String getRequesterName()
    {
        return "ChannelRequesterImpl";
    };
    
    virtual void message(String message,MessageType messageType) 
    {
        std::cout << "[" << getRequesterName() << "] message(" << message << ", " << messageTypeName[messageType] << ")" << std::endl; 
    }

    virtual void channelCreated(epics::pvData::Status* status, Channel *channel)
    {
        std::cout << "channelCreated(" << status->toString() << ", "
                  << (channel ? channel->getChannelName() : "(null)") << ")" << std::endl;
    }
    
    virtual void channelStateChange(Channel *c, ConnectionState connectionState)
    {
        std::cout << "channelStateChange(" << c->getChannelName() << ", " << ConnectionStateNames[connectionState] << ")" << std::endl;
    }
};

class GetFieldRequesterImpl : public GetFieldRequester
{
    virtual String getRequesterName()
    {
        return "GetFieldRequesterImpl";
    };
    
    virtual void message(String message,MessageType messageType) 
    {
        std::cout << "[" << getRequesterName() << "] message(" << message << ", " << messageTypeName[messageType] << ")" << std::endl; 
    }

    virtual void getDone(epics::pvData::Status *status,epics::pvData::FieldConstPtr field)
    {
        std::cout << "getDone(" << status->toString() << ", ";
        if (field)
        {
            String str;
            field->toString(&str);
            std::cout << str;
        }
        else
            std::cout << "(null)";
        std::cout << ")" << std::endl;
    }
};

class ChannelGetRequesterImpl : public ChannelGetRequester
{
    ChannelGet *m_channelGet;
    epics::pvData::PVStructure *m_pvStructure;
    epics::pvData::BitSet *m_bitSet;
                
    virtual String getRequesterName()
    {
        return "ChannelGetRequesterImpl";
    };
    
    virtual void message(String message,MessageType messageType) 
    {
        std::cout << "[" << getRequesterName() << "] message(" << message << ", " << messageTypeName[messageType] << ")" << std::endl; 
    }

    virtual void channelGetConnect(epics::pvData::Status *status,ChannelGet *channelGet,
            epics::pvData::PVStructure *pvStructure,epics::pvData::BitSet *bitSet)
    {
        std::cout << "channelGetConnect(" << status->toString() << ")" << std::endl;
        
        // TODO sync
        m_channelGet = channelGet;
        m_pvStructure = pvStructure;
        m_bitSet = bitSet;
    }

    virtual void getDone(epics::pvData::Status *status)
    {
        std::cout << "getDone(" << status->toString() << ")" << std::endl;
        String str;
        m_pvStructure->toString(&str);
        std::cout << str;
        std::cout << std::endl;
    }
};

class ChannelPutRequesterImpl : public ChannelPutRequester
{
    ChannelPut *m_channelPut;
    epics::pvData::PVStructure *m_pvStructure;
    epics::pvData::BitSet *m_bitSet;
                
    virtual String getRequesterName()
    {
        return "ChannelPutRequesterImpl";
    };
    
    virtual void message(String message,MessageType messageType) 
    {
        std::cout << "[" << getRequesterName() << "] message(" << message << ", " << messageTypeName[messageType] << ")" << std::endl; 
    }

    virtual void channelPutConnect(epics::pvData::Status *status,ChannelPut *channelPut,
            epics::pvData::PVStructure *pvStructure,epics::pvData::BitSet *bitSet)
    {
        std::cout << "channelPutConnect(" << status->toString() << ")" << std::endl;
        
        // TODO sync
        m_channelPut = channelPut;
        m_pvStructure = pvStructure;
        m_bitSet = bitSet;
    }

    virtual void getDone(epics::pvData::Status *status)
    {
        std::cout << "getDone(" << status->toString() << ")" << std::endl;
        String str;
        m_pvStructure->toString(&str);
        std::cout << str;
        std::cout << std::endl;
    }

    virtual void putDone(epics::pvData::Status *status)
    {
        std::cout << "putDone(" << status->toString() << ")" << std::endl;
        String str;
        m_pvStructure->toString(&str);
        std::cout << str;
        std::cout << std::endl;
    }

};
 
 
class MonitorRequesterImpl : public MonitorRequester
{
    virtual String getRequesterName()
    {
        return "MonitorRequesterImpl";
    };
    
    virtual void message(String message,MessageType messageType) 
    {
        std::cout << "[" << getRequesterName() << "] message(" << message << ", " << messageTypeName[messageType] << ")" << std::endl; 
    }
    
    virtual void monitorConnect(Status* status, Monitor* monitor, Structure* structure)
    {
        std::cout << "monitorConnect(" << status->toString() << ")" << std::endl;
        if (structure)
        {
            String str;
            structure->toString(&str);
            std::cout << str << std::endl;
        }
    }
    
    virtual void monitorEvent(Monitor* monitor)
    {
        std::cout << "monitorEvent" << std::endl;

        MonitorElement* element = monitor->poll();
        
        String str("changed/overrun ");
        element->getChangedBitSet()->toString(&str);
        str += '/';
        element->getOverrunBitSet()->toString(&str);
        str += '\n';
        element->getPVStructure()->toString(&str);
        std::cout << str << std::endl;
        
        monitor->release(element);
    }
    
    virtual void unlisten(Monitor* monitor)
    {
        std::cout << "unlisten" << std::endl;
    }
}; 


class ChannelProcessRequesterImpl : public ChannelProcessRequester
{
    ChannelProcess *m_channelProcess;
                
    virtual String getRequesterName()
    {
        return "ProcessRequesterImpl";
    };
    
    virtual void message(String message,MessageType messageType) 
    {
        std::cout << "[" << getRequesterName() << "] message(" << message << ", " << messageTypeName[messageType] << ")" << std::endl; 
    }

    virtual void channelProcessConnect(epics::pvData::Status *status,ChannelProcess *channelProcess)
    {
        std::cout << "channelProcessConnect(" << status->toString() << ")" << std::endl;
        
        // TODO sync
        m_channelProcess = channelProcess;
    }

    virtual void processDone(epics::pvData::Status *status)
    {
        std::cout << "processDone(" << status->toString() << ")" << std::endl;
    }

};

int main(int argc,char *argv[])
{
    TestClientContextImpl* context = new TestClientContextImpl();
    context->printInfo();

    context->initialize();    
    context->printInfo();
    
    epicsThreadSleep ( 1.0 );
    
    //ChannelFindRequesterImpl findRequester;
    //context->getProvider()->channelFind("something", &findRequester);
    
    ChannelRequesterImpl channelRequester;
    Channel* channel = context->getProvider()->createChannel("structureArrayTest", &channelRequester);
    channel->printInfo();
    /*
    GetFieldRequesterImpl getFieldRequesterImpl;
    channel->getField(&getFieldRequesterImpl, "timeStamp.secondsPastEpoch");
    
    ChannelGetRequesterImpl channelGetRequesterImpl;
    ChannelGet* channelGet = channel->createChannelGet(&channelGetRequesterImpl, 0);
    channelGet->get(false);
    channelGet->destroy();
    
    ChannelPutRequesterImpl channelPutRequesterImpl;
    ChannelPut* channelPut = channel->createChannelPut(&channelPutRequesterImpl, 0);
    channelPut->get();
    channelPut->put(false);
    channelPut->destroy();
    
    
    MonitorRequesterImpl monitorRequesterImpl;
    Monitor* monitor = channel->createMonitor(&monitorRequesterImpl, 0);

    Status* status = monitor->start();
    std::cout << "monitor->start() = " << status->toString() << std::endl;
    delete status;


    ChannelProcessRequesterImpl channelProcessRequester;
    ChannelProcess* channelProcess = channel->createChannelProcess(&channelProcessRequester, 0);
    channelProcess->process(false);
    channelProcess->destroy();
    

    status = monitor->stop();
    std::cout << "monitor->stop() = " << status->toString() << std::endl;
    delete status;
    
    
    monitor->destroy();
    */
    epicsThreadSleep ( 100.0 );
    channel->destroy();
    
    context->destroy();
    
    std::cout << "-----------------------------------------------------------------------" << std::endl;
    getShowConstructDestruct()->constuctDestructTotals(stdout);
    return(0);
}