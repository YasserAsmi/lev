// Copyright (c) 2014 Yasser Asmi
// Released under the MIT License (http://opensource.org/licenses/MIT)

#ifndef _LEV_H
#define _LEV_H

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
#include <memory.h>
#include <netinet/tcp.h>
#include <signal.h>

#include <string>

#include <event2/event-config.h>
#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/util.h>
#include <event2/listener.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/http.h>

#ifndef dbgerr
    #define dbgerr(fmt, ...) \
        do { fprintf(stderr, "Error: %s(%d): " fmt, __FILE__, __LINE__, ## __VA_ARGS__); } while (0)

#endif

//HACK: access internal function to prevent base loop from exiting if there are no events
extern "C" void event_base_add_virtual(struct event_base *);


namespace lev
{

class IpAddr;
class EvBaseLoop;
class EvEvent;
class EvKeyValues;
class EvBuffer;
class EvBufferEvent;
class EvConnListener;
class EvHttpUri;


class IpAddr
{
public:
    IpAddr()
    {
        clear();
    }
    IpAddr(const char* addrandport)
    {
        clear();
        assign(addrandport);
    }
    IpAddr(const char* addr, uint16_t port)
    {
        clear();
        assign(addr, port);
    }

    inline bool assign(const char* addrandport)
    {
        // [IPv6Address]:port
        // [IPv6Address]
        // IPv6Address
        // IPv4Address:port
        // IPv4Address

        int ret = evutil_parse_sockaddr_port(addrandport, (struct sockaddr*)&mAddr, &mSize);
        return (ret == 0);
    }

    inline bool assign(const char* addrstr, uint16_t port)
    {
        // 'addrstr' is IPv4 address, port is in host order

        int ret = evutil_parse_sockaddr_port(addrstr, (struct sockaddr*)&mAddr, &mSize);
        ((struct sockaddr_in*)&mAddr)->sin_port = htons(port);

        return (ret == 0);
    }

    inline void assign(int address, uint16_t port)
    {
        // Parameters 'address' and 'port' are in host order
        mAddr.sa_family = AF_INET;
        ((struct sockaddr_in*)&mAddr)->sin_addr.s_addr = htonl(address);
        ((struct sockaddr_in*)&mAddr)->sin_port = htons(port);
        mSize = sizeof(struct sockaddr_in);
    }

    void setPort(uint16_t port)
    {
        ((struct sockaddr_in*)&mAddr)->sin_port = htons(port);
    }

    std::string toString() const
    {
        return makeStr(false);
    }
    std::string toStringFull() const
    {
        // Shows port
        return makeStr(true);
    }

    inline uint16_t port() const
    {
        // Only valid for AF_INET
        return ntohs(((struct sockaddr_in*)&mAddr)->sin_port);
    }

    inline const struct sockaddr* addr() const
    {
        return (struct sockaddr*)&mAddr;
    }

    inline int addrLen() const
    {
        return mSize;
    }

protected:
    struct sockaddr mAddr;
    int mSize;

    void clear()
    {
        memset(&mAddr, 0, sizeof(mAddr));
        mAddr.sa_family = AF_INET;
        mSize = sizeof(struct sockaddr_in);
    }

    std::string makeStr(bool showport) const
    {
        // Only valid for AF_INET
        char buf[64];
        char abuf[64];
        buf[0] = '\0';
        if (evutil_inet_ntop(AF_INET, &(((struct sockaddr_in*)&mAddr)->sin_addr), abuf, sizeof(abuf)))
        {
            if (!showport)
            {
                return std::string(abuf);
            }
            evutil_snprintf(buf, sizeof(buf), "%s:%d", abuf, port());
        }
        return std::string(buf);
    }
};


class EvEvent
{
public:
    EvEvent() :
        mPtr(NULL),
        mUserData(NULL)
    {
    }
    ~EvEvent()
    {
        free();
    }

    void free()
    {
        if (mPtr)
        {
            event_free(mPtr);
        }
        mPtr = NULL;
    }

    inline void* userData()
    {
        return mUserData;
    }
    inline void setUserData(void* userdata)
    {
        mUserData = userdata;
    }

    void newTimer(event_callback_fn callback, struct event_base* base)
    {
        int flags = EV_PERSIST;
        free();
        mPtr = event_new(base, -1, flags, callback, this);
    }

    void newSignal(event_callback_fn callback, int signum, struct event_base* base)
    {
        int flags = EV_PERSIST | EV_SIGNAL;
        free();
        mPtr = event_new(base, signum, flags, callback, this);
    }

    void newSignalCtx(event_callback_fn callback, int signum, struct event_base* base, void* ctx)
    {
        free();
        mPtr = evsignal_new(base, signum, callback, (void*)ctx);
    }

    void newUser(event_callback_fn callback, struct event_base* base)
    {
        free();
        mPtr = event_new(base, -1, 0, callback, this);
    }

    inline void start()
    {
        event_add(mPtr, NULL);
    }
    inline void start(int msecs)
    {
        timeval t = EvEvent::tvMsecs(msecs);
        event_add(mPtr, &t);
    }
    inline void end()
    {
        event_del(mPtr);
    }

    inline void activateUser(int res)
    {
        event_active(mPtr, res, 0);
    }

    inline struct event_base* base()
    {
        return event_get_base(mPtr);
    }

    inline void exitLoop()
    {
        event_base_loopexit(base(), NULL);
    }

    static
    struct timeval tvMsecs(int msecs)
    {
        timeval t;
        t.tv_sec = msecs / 1000;
        t.tv_usec = (msecs % 1000) * 1000;
        return t;
    }

protected:
    struct event* mPtr;
    void* mUserData;
};

class EvBuffer
{
public:
    EvBuffer() :
        mPtr(NULL),
        mOwner(false)
    {
    }
    EvBuffer(struct evbuffer* ptr) :
        mPtr(ptr),
        mOwner(false)
    {
    }
    ~EvBuffer()
    {
        free();
    }
    void newBuffer()
    {
        free();
        mPtr = evbuffer_new();
        mOwner = true;
    }
    void free()
    {
        if (mOwner && mPtr)
        {
            evbuffer_free(mPtr);
        }
        mOwner = false;
        mPtr = NULL;
    }

    inline void enableLocking()
    {
        evbuffer_enable_locking(mPtr, NULL);
    }
    inline void lock()
    {
        evbuffer_lock(mPtr);
    }
    inline void unlock()
    {
        evbuffer_unlock(mPtr);
    }
    inline size_t length()
    {
        return evbuffer_get_length(mPtr);
    }
    inline size_t space()
    {
        return evbuffer_get_contiguous_space(mPtr);
    }
    inline bool append(const void* data, size_t datalen)
    {
        return evbuffer_add(mPtr, data, datalen) == 0;
    }
    inline bool prepend(const void* data, size_t datalen)
    {
        return evbuffer_prepend(mPtr, data, datalen) == 0;
    }

    inline int vprintf(const char* fmt, va_list varg)
    {
        return evbuffer_add_vprintf(mPtr, fmt, varg);
    }
    int printf(const char* fmt, ...)
    {
        va_list va;
        va_start(va, fmt);
        int ret = vprintf(fmt, va);
        va_end(va);
        return ret;
    }

    bool append(const EvBuffer& src)
    {
        return evbuffer_add_buffer(mPtr, src.mPtr) == 0;
    }

    inline struct evbuffer* ptr()
    {
        return mPtr;
    }

protected:
    struct evbuffer* mPtr;
    bool mOwner;
};


class EvBufferEvent
{
public:
    EvBufferEvent() :
        mPtr(NULL),
        mOwner(false)
    {
    }
    EvBufferEvent(struct bufferevent* ptr) :
        mPtr(ptr),
        mOwner(false)
    {
    }
    ~EvBufferEvent()
    {
        free();
    }
    void free()
    {
        if (mOwner && mPtr)
        {
            bufferevent_free(mPtr);
        }
        mOwner = false;
        mPtr = NULL;
    }

    bool newForSocket(int fd, bufferevent_data_cb readcb, bufferevent_data_cb writecb,
        bufferevent_event_cb eventcb, void* cbarg, struct event_base* base)
    {
        // fd can be -1 if you connect later

        // typedef void (*bufferevent_data_cb)(struct bufferevent *bev, void *cbarg);
        // typedef void (*bufferevent_event_cb)(struct bufferevent *bev,short events, void *cbarg);

        free();

        struct bufferevent* be;
        int flags = /*BEV_OPT_THREADSAFE |*/ BEV_OPT_CLOSE_ON_FREE;

        // Create a new socket buffer event
        be = bufferevent_socket_new(base, fd, flags);
        if (be == NULL)
        {
            dbgerr("Failed to create libevent buffer event\n");
            return false;
        }

        bufferevent_setcb(be, readcb, writecb, eventcb, cbarg);

        // Assign the pointer to the object
        mPtr = be;
        mOwner = true;

        return true;
    }

    inline void own(bool objowns)
    {
        mOwner = objowns;
    }

    bool connect(const IpAddr& sa)
    {
        int ret = bufferevent_socket_connect(mPtr, (sockaddr*)sa.addr(), sa.addrLen());

        return (ret == 0);
    }

    inline EvBuffer input()
    {
        return EvBuffer(bufferevent_get_input(mPtr));
    }
    inline EvBuffer output()
    {
        return EvBuffer(bufferevent_get_output(mPtr));
    }

    inline void enable(short flags)
    {
        bufferevent_enable(mPtr, flags);
    }
    inline void disable(short flags)
    {
        bufferevent_disable(mPtr, flags);
    }

    void setTcpNoDelay()
    {
        int one = 1;
        setsockopt(bufferevent_getfd(mPtr), IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
    }

protected:
    struct bufferevent* mPtr;
    bool mOwner;
};

class EvConnListener
{
public:
    EvConnListener() :
        mPtr(NULL),
        mOwner(false)
    {
    }
    EvConnListener(struct evconnlistener* ptr) :
        mPtr(ptr),
        mOwner(false)
    {
    }
    ~EvConnListener()
    {
        free();
    }
    void free()
    {
        if (mOwner && mPtr)
        {
            evconnlistener_free(mPtr);
        }
        mOwner = false;
        mPtr = NULL;
    }
    void assign(struct evconnlistener* ptr)
    {
        free();
        mPtr = ptr;
    }
    void newListener(const IpAddr& sa, evconnlistener_cb callback, void* cbarg, struct event_base* base)
    {
        //dbglog("Listening on %s\n", sa.toString().c_str());

        free();

        int flags = LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE;
        int backlog = -1;  //TODO: expose

        mPtr = evconnlistener_new_bind(base, callback, cbarg, flags, backlog, sa.addr(), sa.addrLen());
        mOwner = true;
    }

    inline void enable()
    {
        evconnlistener_enable(mPtr);
    }
    inline void disable()
    {
        evconnlistener_disable(mPtr);
    }
    inline struct event_base* base()
    {
        return evconnlistener_get_base(mPtr);
    }
    void setTcpNoDelay(int fd)
    {
        int one = 1;
        setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
    }

protected:
    struct evconnlistener* mPtr;
    bool mOwner;
};


class EvBaseLoop
{
public:
    EvBaseLoop()
    {
        mBase = event_base_new();
        if (!mBase)
        {
            dbgerr("Failed to get libevent base\n");
        }
    }
    ~EvBaseLoop()
    {
        if (mBase)
        {
            event_base_free(mBase);
        }
    }

    inline operator struct event_base*()
    {
        return mBase;
    }

    inline struct event_base* base()
    {
        return mBase;
    }

    inline void incLoopRef()
    {
        event_base_add_virtual(mBase);
    }

    void loop(int flags = 0)
    {
        // EVLOOP_ONCE
        // EVLOOP_NONBLOCK
        // EVLOOP_NO_EXIT_ON_EMPTY

        assert(mBase);
        event_base_loop(mBase, flags);
    }

    static
    void enableDebug()
    {
        event_enable_debug_mode();
    }

protected:
    struct event_base* mBase;
};



class EvKeyValues
{
public:
    EvKeyValues() :
        mHdrHead(NULL),
        mHdrPtr(NULL),
        mOwner(false)
    {
    }
    EvKeyValues(struct evkeyvalq* headers) :
        mHdrHead(headers),
        mOwner(false)
    {
        moveFirst();
    }

    bool newFromUri(const char* uri)
    {
        // Parse out arguments from the query portion of an HTTP URI
        //    q=test&s=some+thing
        // Will yield
        //    key="q", value="test"
        //    key="s", value="some thing"
        // NOTE: Doesn't support keys only (ie key1&key2=value2)

        int ret;
        free();
        mOwner = true;
        mHdrHead = &mHdrEntry;
        ret = (evhttp_parse_query_str(uri, mHdrHead) == 0);
        moveFirst();
        return ret;
    }

    void free()
    {
        if (mOwner && mHdrHead)
        {
            evhttp_clear_headers(mHdrHead);
        }
        mOwner = false;
        mHdrHead = NULL;
    }
    const char* find(const char* key)
    {
        if (mHdrHead == NULL)
        {
            return NULL;
        }
        return evhttp_find_header(mHdrHead, key);
    }
    bool remove(const char* key)
    {
        if (mHdrHead == NULL)
        {
            return NULL;
        }
        return (evhttp_remove_header(mHdrHead, key) == 0);
    }
    bool add(const char* key, const char* value)
    {
        int ret = false;
        if (mHdrHead)
        {
            ret = (evhttp_add_header(mHdrHead, key, value) == 0);
        }
        return ret;
    }

    inline void moveFirst()
    {
        mHdrPtr = (mHdrHead != NULL) ? mHdrHead->tqh_first : NULL;
    }
    inline void moveNext()
    {
        mHdrPtr = (mHdrPtr != NULL) ? mHdrPtr->next.tqe_next : NULL;
    }
    inline bool eof()
    {
        return mHdrPtr == NULL;
    }
    inline const char* key()
    {
        return (mHdrPtr != NULL) ? mHdrPtr->key : NULL;
    }
    inline const char* value()
    {
        return (mHdrPtr != NULL) ? mHdrPtr->value : NULL;
    }

protected:
    struct evkeyvalq* mHdrHead;
    struct evkeyval* mHdrPtr;
    bool mOwner;
    struct evkeyvalq mHdrEntry;
};

class EvHttpUri
{
public:
    EvHttpUri() :
        mUri(NULL),
        mOwner(false)
    {
    }
    EvHttpUri(struct evhttp_uri* uri) :
        mOwner(false)
    {
        mUri = uri;
    }
    EvHttpUri(const struct evhttp_uri* uri) :
        mOwner(false)
    {
        mUri = (struct evhttp_uri*)uri;
    }
    EvHttpUri(struct evhttp_request* req) :
        mOwner(false)
    {
        mUri = (struct evhttp_uri*)evhttp_request_get_evhttp_uri(req);
    }
    ~EvHttpUri()
    {
        free();
    }

    inline void newEmpty()
    {
        free();
        mOwner = true;
        mUri = evhttp_uri_new();
    }

    void newParsed(const char* sourceuri, bool nonconformant)
    {
        // Parses RFC3986 and relative-refs:
        //      scheme://[[userinfo]@]foo.com[:port]]/[path][?query][#fragment]
        //      [path][?query][#fragment]

        free();
        mOwner = true;
        mUri = evhttp_uri_parse_with_flags(sourceuri, nonconformant ? EVHTTP_URI_NONCONFORMANT : 0);
    }
    void free()
    {
        if (mOwner && mUri)
        {
            evhttp_uri_free(mUri);
        }
        mOwner = false;
        mUri = NULL;
    }

    std::string join()
    {
        // Join together the uri parts from parsed data to form a URI-Reference.
        std::string s;
        char buf[1024];
        char* ret = evhttp_uri_join(mUri, buf, sizeof(buf));
        if (ret)
        {
            s.assign(ret);
        }
        return s;
    }

    // Get values

    inline const char* scheme()
    {
        return evhttp_uri_get_scheme(mUri);
    }
    inline const char* userInfo()
    {
        return evhttp_uri_get_userinfo(mUri);
    }
    inline const char* host()
    {
        return evhttp_uri_get_host(mUri);
    }
    inline int port()
    {
        return evhttp_uri_get_port(mUri);
    }
    inline const char* path()
    {
        return evhttp_uri_get_path(mUri);
    }
    inline const char* query()
    {
        return evhttp_uri_get_query(mUri);
    }
    inline const char* fragment()
    {
        return evhttp_uri_get_fragment(mUri);
    }

    // Set values

    bool setScheme(const char* scheme)
    {
        return evhttp_uri_set_scheme(mUri, scheme) == 0;
    }
    bool setUserInfo(const char* userinfo)
    {
        return evhttp_uri_set_userinfo(mUri, userinfo) == 0;
    }
    bool setHost(const char* host)
    {
        return evhttp_uri_set_host(mUri, host) == 0;
    }
    bool setPort(int port)
    {
        return evhttp_uri_set_port(mUri, port) == 0;
    }
    bool setPath(const char*path)
    {
        return evhttp_uri_set_path(mUri, path) == 0;
    }
    bool setQuery(const char* query)
    {
        return evhttp_uri_set_query(mUri, query) == 0;
    }
    bool setFragment(const char* fragment)
    {
        return evhttp_uri_set_fragment(mUri, fragment) == 0;
    }

protected:
    struct evhttp_uri* mUri;
    bool mOwner;
};


} // namespace lev

#endif // _LEV_H
