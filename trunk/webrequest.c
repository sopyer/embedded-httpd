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

#include "webrequest.h"
#include "network.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

struct _WebRequest {
  unsigned short  result;
  unsigned short  port;
  size_t          bytesUsed;
  size_t          maxBytes;
  // internal scratchpad
  char            bytes[1];  
};

void* webrequest_alloc( WebRequest* _req, size_t _size )
{
  char* p = _req->bytes + _req->bytesUsed;
  _req->bytesUsed += _size;
  return p;
}

CAPI void webrequest_sprintf( WebRequest* _req, const char* _fmt, ... )
{
  va_list ap;
  va_start(ap,_fmt);
  // TODO: vsnprintf
  int len = vsprintf(_req->bytes + _req->bytesUsed,_fmt,ap);
  _req->bytesUsed += len;
  va_end(ap);
}

CAPI void webrequest_strcat( WebRequest* _req, const char* _orig )
{
  char* p = _req->bytes + _req->bytesUsed;
  int i=0;
  while (*_orig && _req->bytesUsed<_req->maxBytes)
  {
    p[i++] = *_orig++;
    _req->bytesUsed++;
  }
}

static char* webrequest_strdup( WebRequest* _req, const char* _orig )
{
  char* p = _req->bytes + _req->bytesUsed;
  int i=0;
  while (*_orig && _req->bytesUsed<_req->maxBytes)
  {
    p[i++] = *_orig++;
    _req->bytesUsed++;
  }
  p[i] = 0;
  _req->bytesUsed++;
  return p;
}

CAPI WebRequest* webrequest_create( const char* _hostname, unsigned short _port, const char* _location, const char* _method, size_t _maxBytes )
{
  WebRequest* req = (WebRequest*) calloc(1,sizeof(WebRequest)+_maxBytes);
  if (req)
  {
    req->maxBytes = _maxBytes;
    req->bytesUsed = 0;    
    webrequest_strdup(req,_hostname);
    req->port = _port;    
    webrequest_sprintf(req,"%s %s HTTP/1.1\r\n",_method,_location);
    webrequest_strcat(req,"Host: dbalster\r\n");
    webrequest_strcat(req,"Connection: close\r\n");
    webrequest_strcat(req,"Content-Length: 00000000\r\n");
    
    webrequest_reset(req);
  }
  return req;
}

CAPI void webrequest_reset( WebRequest* _req )
{
  char* p = _req->bytes+1+strlen(_req->bytes);
  p = strstr(p,"Content-Length:");
  if (p)
  {
    _req->bytesUsed = p-_req->bytes;
    webrequest_strcat(_req,"Content-Length: 00000000\r\n");
  }
}

static bool webrequest_error(const char* _msg, ... )
{
  char msg[1000];
  va_list ap;
  va_start(ap,_msg);
  vsprintf(msg,_msg,ap);
  va_end(ap);
  fprintf(stderr,"ERROR: %s: %s\n",strerror(errno),msg);
  return false;
}

CAPI bool webrequest_execute( WebRequest* _req )
{
  bool result = false;
  char* hostname = _req->bytes;
  char* start = hostname+strlen(hostname)+1;
  char* p = strstr(start,"\r\n\r\n");
  if (p==0)
  {
    webrequest_strcat(_req,"\r\n");
    p = strstr(start,"\r\n\r\n");
  }
  if (p)
  {
    p += 4; // skip CRLFCRLF
    char* end = _req->bytes + _req->bytesUsed;
    int contentSize = (int)(end - p);
    char* contentLength = strstr(start,"Content-Length: ") + strlen("Content-Length: ");
    sprintf(contentLength,"%8d",contentSize);
    contentLength[8] = '\r';
  
    struct hostent* server = gethostbyname(hostname);
    if (0==server)
    {
      return webrequest_error(hostname);
    }
    int sock = (int) socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if (sock==-1)
    {
      return webrequest_error("socket()");
    }
    struct sockaddr_in serv_addr;
    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy((char *)&serv_addr.sin_addr.s_addr, (char *)server->h_addr, server->h_length);
    serv_addr.sin_port = htons(_req->port);
    if (connect(sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr)) < 0)
    {
      webrequest_error("connect()");
    }
    else
    {
      int len = strlen(start);
      int written = send(sock,start,len,0);
      if (written == len)
      {
        int read = recv(sock,_req->bytes,_req->maxBytes,0);
        if (read>=0)
        {
          _req->bytes[read]=0;

          if ( recv(sock,_req->bytes+read,_req->maxBytes-read,MSG_PEEK) || errno==EAGAIN)
          {
            int read2 = recv(sock,_req->bytes+read,_req->maxBytes-read,0);
            if (read2>0) _req->bytes[read+read2]=0; 
          }
          result = true;
        }
      }
    }
    closesocket(sock);
  }
  else
  {
    webrequest_error("invalid header and content");
  }
  
  return result;
}

CAPI size_t webrequest_get_content_length( WebRequest* _req )
{
  return strtoul(webrequest_get_header(_req,"Content-Length:"),0,0);
}

CAPI const char* webrequest_get_header( WebRequest* _req, const char* _header )
{
  char* p = strstr(_req->bytes,_header);
  if (p)
  {
    return p+strlen(_header);
  }
  return 0;
}

CAPI const char* webrequest_get_content( WebRequest* _req )
{
  return strstr(_req->bytes,"\r\n\r\n") + 4;
}

CAPI int webrequest_get_result( WebRequest* _req )
{
  int result = 0;
  result = strtoul( _req->bytes + strlen("HTTP/1.1"),0,0);
  return result;
}

CAPI void webrequest_destroy( WebRequest* _req )
{
  if (_req)
  {
    free(_req);
  }
}

