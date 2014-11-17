// Copyright (c) 2014 Yasser Asmi
// Released under the MIT License (http://opensource.org/licenses/MIT)

#include <getopt.h>
#include "lev.h"

using namespace lev;

static
void onCtrlC(evutil_socket_t fd, short what, void* arg)
{
    EvEvent* ev = (EvEvent*)arg;
    printf("Ctrl-C --exiting loop\n");
    ev->exitLoop();
}

static
void onServEcho(struct bufferevent* bev, void* cbarg)
{
    EvBufferEvent evbuf(bev);

    // Copy all the data from the input buffer to the output buffer.
    evbuf.output().append(evbuf.input());
}

static
void onServEvent(struct bufferevent* bev, short events, void* cbarg)
{
    EvBufferEvent evbuf(bev);

    if (events & BEV_EVENT_ERROR)
    {
        printf("Error: Server connection error\n");
    }
    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR))
    {
        evbuf.own(true);
        evbuf.free();
    }
}

static
void onAccept(struct evconnlistener* listener, evutil_socket_t fd, struct sockaddr* address,
    int socklen, void* cbarg)
{
    EvConnListener evlis(listener);
    EvBufferEvent evbuf;

    if (evbuf.newForSocket(fd, onServEcho, NULL, onServEvent, NULL, evlis.base()))
    {
        evbuf.own(false);
        evbuf.enable(EV_READ | EV_WRITE);
    }

    evlis.setTcpNoDelay(fd);
}

void testServer(const char* arg)
{
    EvBaseLoop base;
    EvConnListener listener;
    IpAddr sin(arg ? arg : "127.0.0.1:60");

    printf("Server listening on %s\n", sin.toString().c_str());

    signal(SIGPIPE, SIG_IGN);

    EvEvent ctrlc;
    ctrlc.newSignal(onCtrlC, SIGINT, base);
    ctrlc.start();

    EvEvent evstop;
    evstop.newSignal(onCtrlC, SIGHUP, base);
    evstop.start();

    listener.newListener(sin, onAccept, NULL, base);

    base.loop();

}


static
void onClientTimeout(evutil_socket_t fd, short what, void* arg)
{
    EvEvent* ev = (EvEvent*)arg;
    printf("timeout\n");
    ev->exitLoop();
}

static
void onClientRead(struct bufferevent* bev, void* cbarg)
{
    EvBufferEvent evbuf(bev);
    int64_t* bytesread = (int64_t*)(cbarg);

    if (bytesread)
    {
        // Increment number of bytes read
        (*bytesread) += evbuf.input().length();
    }

    // Copy all the data from the input buffer to the output buffer.
    evbuf.output().append(evbuf.input());
}

static
void onClientEvent(struct bufferevent* bev, short events, void* ptr)
{
    EvBufferEvent evbuf(bev);

    if (events & BEV_EVENT_CONNECTED)
    {
        evbuf.setTcpNoDelay();
    }
    else if (events & BEV_EVENT_ERROR)
    {
        printf("Error: Client connection failed\n");
    }
}

void testClient(const char* arg)
{
    EvBaseLoop base;
    IpAddr sin;
    EvEvent evtimeout;
    EvBufferEvent evbuf;

    EvEvent ctrlc;
    ctrlc.newSignal(onCtrlC, SIGINT, base);
    ctrlc.start();

    evtimeout.newTimer(onClientTimeout, base);
    evtimeout.start(2000);

    // Connect the client
    sin.assign(arg ? arg : "127.0.0.1:60");
    printf("Client connecting on %s\n", sin.toString().c_str());

    int64_t bytesread = 0;
    if (evbuf.newForSocket(-1, onClientRead, NULL, onClientEvent, (void*)&bytesread, base))
    {
        evbuf.enable(EV_READ | EV_WRITE);
    }

    // Build a message
    for (int i = 0; i < 100; i++)
    {
        evbuf.output().printf("%d--libevent and lev are cool\n");
    }

    // Connect
    if (!evbuf.connect(sin))
    {
        printf("Error: Client failed to connect\n");
    }

    base.loop();

    printf("%ld Total bytes read\n", bytesread);
}


int main(int argc, char** argv)
{
    int opt = 0;
    bool showhelp = true;
    while ((opt = getopt(argc, argv, "cs")) != -1)
    {
        switch (opt)
        {
            case 'c':
                testClient(NULL);
                showhelp = false;
                break;
            case 's':
                testServer(NULL);
                showhelp = false;
                break;
        }
    }
    if (showhelp)
    {
        printf("sockcliserv OPTION\n");
        printf("   -s   start server\n");
        printf("   -c   start client\n");
    }

    return 0;
}


