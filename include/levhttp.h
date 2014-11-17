// Copyright (c) 2014 Yasser Asmi
// Released under the MIT License (http://opensource.org/licenses/MIT)

#ifndef _LEVHTTP_H
#define _LEVHTTP_H

namespace lev
{

class EvHttpRequest;
class EvHttpServer;


class EvHttpRequest
{
public:
    EvHttpRequest(struct evhttp_request* req)
    {
        mReq = req;
    }
    ~EvHttpRequest()
    {
    }

    inline void sendError(int error, const char* reason)
    {
        evhttp_send_error(mReq, error, reason);
    }
    inline void sendReply(int responsecode, const char* responsemsg)
    {
        evhttp_send_reply(mReq, responsecode, responsemsg, NULL);
    }
    inline void sendReply(int responsecode, const char* responsemsg, EvBuffer& body)
    {
        evhttp_send_reply(mReq, responsecode, responsemsg, body.ptr());
    }

    //TODO: add chunk API

    inline void cancel()
    {
        evhttp_cancel_request(mReq);
    }

    inline struct evhttp_connection* connection()
    {
        return evhttp_request_get_connection(mReq);
    }

    // uri

    inline const char* uriStr()
    {
        return evhttp_request_get_uri(mReq);
    }
    inline EvHttpUri uri()
    {
        EvHttpUri u(evhttp_request_get_evhttp_uri(mReq));
        return u;
    }

    inline const char* host()
    {
        return evhttp_request_get_host(mReq);
    }
    inline enum evhttp_cmd_type cmd()
    {
        return evhttp_request_get_command(mReq);
    }
    inline int responseCode()
    {
        return evhttp_request_get_response_code(mReq);
    }

    inline struct evkeyvalq* inputHdrs()
    {
        return evhttp_request_get_input_headers(mReq);
    }
    inline struct evkeyvalq* outputHdrs()
    {
        return evhttp_request_get_output_headers(mReq);
    }

    inline EvBuffer input()
    {
        return EvBuffer(evhttp_request_get_input_buffer(mReq));
    }
    inline EvBuffer output()
    {
        return EvBuffer(evhttp_request_get_output_buffer(mReq));
    }

protected:
    struct evhttp_request* mReq;

private:
    EvHttpRequest();
};


class EvHttpServer
{
public:
    typedef void (*RouteCallback)(struct evhttp_request*, void*);

    EvHttpServer(struct event_base* base)
    {
        mServer = evhttp_new(base);
        if (mServer == NULL)
        {
            dbgerr("Can't create http server");
        }
    }
    ~EvHttpServer()
    {
        if (mServer)
        {
            evhttp_free(mServer);
        }
    }

    void setDefaultRoute(RouteCallback callback, void* cbarg = NULL)
    {
        evhttp_set_gencb(mServer, callback, cbarg);
    }
    bool addRoute(const char* path, RouteCallback callback, void* cbarg = NULL)
    {
        int ret = evhttp_set_cb(mServer, path, callback, cbarg);
        if (ret == -2)
        {
            dbgerr("Path %s already exists\n", path);
        }
        return ret == 0;
    }
    bool deleteRoute(const char* path)
    {
        int ret = evhttp_del_cb(mServer, path);
        return ret == 0;
    }

    bool bind(const char* address, short port, EvConnListener* connout = NULL)
    {
        // can be called multiple times
        struct evhttp_bound_socket* ret;
        ret = evhttp_bind_socket_with_handle(mServer, address, port);
        if (ret)
        {
            if (connout)
            {
                connout->assign(evhttp_bound_socket_get_listener(ret));
            }
            return true;
        }
        return false;
    }
    inline bool bind(const IpAddr& sa)
    {
        return bind(sa.toString().c_str(), sa.port());
    }

    static
    std::string encodeUriString(const char* src)
    {
        std::string s;
        char* temp = evhttp_encode_uri(src);
        if (temp)
        {
            s.assign(temp);
            free(temp);
        }
        return s;
    }

    static
    std::string decodeUriString(const char* src)
    {
        std::string s;
        char* temp = evhttp_decode_uri(src);
        if (temp)
        {
            s.assign(temp);
            free(temp);
        }
        return s;
    }

    static
    std::string htmlEscape(const char* src)
    {
        // Replaces <, >, ", ' and & with &lt;, &gt;, &quot;, &#039; and &amp;
        std::string s;
        char* temp = evhttp_htmlescape(src);
        if (temp)
        {
            s.assign(temp);
            free(temp);
        }
        return s;
    }

protected:
    struct evhttp* mServer;

private:
    EvHttpServer();
};

} // namespace lev

#endif // _LEVHTTP_H
