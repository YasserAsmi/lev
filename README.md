lev
===

Lightweight C++ wrapper for LibEvent 2 API

[![Build Status](https://webapi.biicode.com/v1/badges/lasote/lasote/lev/master)](https://www.biicode.com/lasote/lev) 

LibEvent is a great library.  It uses a C interface which is well designed but has a learning curve.
This library, lev, is a very simple API in C++ that encapsulates commonly used functionality.  It tries
to stay close to the libevent API concepts except when there is confusion. It simplifies life-times of
objects.  The callback function signatures remain identical--but lev objects can be constructed within the
callback functions.
```
      EvHttpRequest evreq(req);
```
lev is actually just a couple of header file, lev.h, and levhttp.h (if you need a httpserver).  Just include
it in your application and start using all the classes--no need to build or install:

```
class IpAddr;
class EvBaseLoop;
class EvEvent;
class EvKeyValues;
class EvBuffer;
class EvBufferEvent;
class EvConnListener;
class EvHttpUri;
class EvHttpRequest;
class EvHttpServer;
```

Code: An HTTP server using lev.  Look at the example section for more.

```
  #include "lev.h"
  #include "levhttp.h"
  using namespace lev;

  static
  void onHttpHello(struct evhttp_request* req, void* arg)
  {
      EvHttpRequest evreq(req);
      evreq.output().printf("<..>Hello World!<..>");
      evreq.sendReply(200, "OK");
  }

  int main(int argc, char** argv)
  {
      EvBaseLoop base;
      EvHttpServer http(base);

      http.addRoute("/hello", onHttpHello);
      http.bind("127.0.0.1", 8080);

      base.loop();

      return 0;
  }
```
