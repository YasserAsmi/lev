// Copyright (c) 2014 Yasser Asmi
// Released under the MIT License (http://opensource.org/licenses/MIT)

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
void onHttpHello(struct evhttp_request* req, void* arg)
{
    EvHttpRequest evreq(req);

    evreq.output().printf("<html><body><center><h1>Hello World!</h1></center></body></html>");

    evreq.sendReply(200, "OK");
}

static
void onHttpDefault(struct evhttp_request* req, void* arg)
{
    EvHttpRequest evreq(req);
    EvHttpUri uri = evreq.uri();

    evreq.output().printf("<html><body>");
    evreq.output().printf("<center><h1>%s</h1></center>", evreq.uriStr());
    evreq.output().printf("host=%s<br>", uri.host());
    evreq.output().printf("path=%s<br>", uri.path());
    evreq.output().printf("query=%s<br>", uri.query());
    evreq.output().printf("</body></html>");

    evreq.sendReply(200, "OK");
}

int main(int argc, char** argv)
{
    //EvBaseLoop::enableDebug();

    EvBaseLoop base;

    EvEvent ctrlc;
    ctrlc.newSignal(onCtrlC, SIGINT, base);
    ctrlc.start();

    EvHttpServer http(base);
    http.setDefaultRoute(onHttpDefault);
    http.addRoute("/hello", onHttpHello);

    http.bind("127.0.0.1", 8080);

    base.loop();

    return 0;
}
