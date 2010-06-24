/*
 * Copyright (c) Daniel Balster
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Daniel Balster nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY DANIEL BALSTER ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL DANIEL BALSTER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "webserver.h"

#include "webrequest.h"
#include "webresponse.h"
#include "network.h"

#include <stdio.h>

struct _WebServer
{
  int                 socket;
  void*               userdata;
	HttpRequestHandler  handler;
};

WebServer* webserver_create ( unsigned short _port, HttpRequestHandler _handler, void* _userdata )
{
  bool result = false;
	WebServer* server = (WebServer*) calloc(1,sizeof(WebServer));
		
	server->handler = _handler;
  server->userdata = _userdata;

  server->socket = (int) socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (server->socket == -1)
  {
    printf ("socket");
  }

  int opt = 1;
  if (setsockopt(server->socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)))
  {
    printf ("setsocketopt");
  }

  struct sockaddr_in sa;
  sa.sin_port = htons(_port);
  sa.sin_family = AF_INET;
  sa.sin_addr.s_addr = INADDR_ANY;
  if (bind(server->socket, (struct sockaddr*)&sa, sizeof(sa)))
  {
    printf ("bind");
  }

  if (-1 == listen(server->socket, 5))
  {
    printf ("listen");
  }
  else
  {
    result = true;
  }
  
  if (result==false)
  {
    webserver_destroy(server);
    return 0;
  }

  return server;
}

void webserver_destroy (WebServer* _server)
{
  if (_server)
  {
    closesocket(_server->socket);
    free (_server);
  }
}

void webserver_process (WebServer* _server, bool _blocking)
{
  if (-1 == _server->socket) return;

  struct sockaddr_in sa;
  int sin_size = sizeof(sa);

  if (false == _blocking)
  {
    fd_set fds;
    struct timeval tv;
    int rc;

    tv.tv_sec = 0;
    tv.tv_usec = 0;

    FD_ZERO(&fds);
    FD_SET(_server->socket, &fds);
    rc = select(_server->socket + 1, &fds, NULL, NULL, &tv);

    if (rc <= 0) return;
  }

  int client = (int)accept(_server->socket, (struct sockaddr*)&sa, (socklen_t*)&sin_size);
  if (client < 0) return;

  WebResponse* req = webresponse_create (client); 
  if (req)
  {
    if (webresponse_parse (req))
    {
      _server->handler(req,_server->userdata);
    }
    webresponse_destroy(req);
  }
  else
  {
    closesocket(client);
  }
}
