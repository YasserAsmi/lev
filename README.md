lev
===
Lightweight C++ wrapper for LibEvent 2 API

LibEvent is a great library.  It uses a C interface which is well designed but has a learning curve.
This library, lev, is a very simple API in C++ that encapsulates commonly used functionality.  It tries
to stay close to the C concept except when the concept wasn't very clear and it simplifies life-times of
objects.

lev is actually just a single header file, lev.h.  Just include it in your application and start using all
the classes:

class EvBaseLoop;
class EvEvent;
class EvSockAddr;
class EvKeyValues;
class EvBuffer;
class EvBufferEvent;
class EvConnListener;
class EvHttpUri;
class EvHttpRequest;
class EvHttpServer;

Code: An HTTP server using lev.  Look at the example section for more.
<code>
<br>  #include "lev.h"
<br>  using namespace lev;
<br>
<br>  static
<br>  void onHttpHello(struct evhttp_request* req, void* arg)
<br>  {
<br>      EvHttpRequest evreq(req);
<br>      evreq.output().printf("<..>Hello World!<..>");
<br>      evreq.sendReply(200, "OK");
<br>  }
<br>
<br>  int main(int argc, char** argv)
<br>  {
<br>      EvBaseLoop base;
<br>      EvHttpServer http(base);
<br>
<br>      http.addRoute("/hello", onHttpHello);
<br>      http.bind("127.0.0.1", 8080);
<br>
<br>      base.loop();
<br>
<br>      return 0;
<br>  }
</code>