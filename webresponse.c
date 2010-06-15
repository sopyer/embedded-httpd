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

#include "webresponse.h"
#include "network.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static int hexnibble( const char c )
{
  int result = 0;
  if (c>='a' && c<='f') result += c-'a'+10;
  if (c>='A' && c<='F') result += c-'A'+10;
  if (c>='0' && c<='9') result += c-'0';
  return result;
}

// decode '+' and '%xx' back into bytes
char* uri_decode_inplace( char* _text )
{
  int j=0;
  for (int i=0; _text[i]; ++i)
  {
    char c=_text[i];
    switch( c )
    {
      case '+': c = ' '; break;
      case '%':
      {
        int lo,hi;
        lo = hexnibble(_text[++i]);
        hi = hexnibble(_text[++i]);
        c = lo <<4 | hi;
      }
        break;
      default:
        break;
    }
    _text[j++]=c;
  }
  _text[j]=0;
  return _text;
}

void quoting_strncpy_append( const char* _text, char* _output, size_t _size, size_t* _index )
{
  if (_output)
  {
    // TODO range check, prevent buffer overflow (_output+_size < _output+_index+strlen(_text))
    strcat(_output,_text);
  }
  *_index += strlen(_text);
}

size_t quoting_strncpy( char* _output, const char* _input, size_t _size )
{
  size_t i=0;
  size_t o=0;
  char c[2];
  c[1] = 0;
  
  if (_output) *_output = 0; // clear output buffer
  
  while ( (*c = _input[i++]) )
  {
    switch (*c)
    {
      case '\'': quoting_strncpy_append("&apos;",_output,_size,&o); break;
      case '"': quoting_strncpy_append("&quot;",_output,_size,&o); break;
      case '<': quoting_strncpy_append("&lt;",_output,_size,&o); break;
      case '>': quoting_strncpy_append("&gt;",_output,_size,&o); break;
      case '&': quoting_strncpy_append("&amp;",_output,_size,&o); break;
      case ' ': quoting_strncpy_append("&nbsp;",_output,_size,&o); break;
      case '\t': quoting_strncpy_append("&nbsp;&nbsp;",_output,_size,&o); break;
      default: quoting_strncpy_append(c,_output,_size,&o); break;
    }
  }
  
  return o;
}

struct _WebResponse
{
  int netsocket;
  char* memory;
  char* method;		// GET, POST, PUT, FINDPROP, ...
  char* location;	// /path/to/page
  int n_args;
  HttpHeader* args;
  int n_headers;
  HttpHeader* headers;
  bool chunked;
};

static const struct
{
	unsigned short code;
	const char* message;
} 
HttpErrorMessages[] =  
{
  { 100, "Continue" }, 
  { 200, "OK" }, 
  { 220, "OK" }, 
  { 302, "Found" }, 
  { 303, "See Other" }, 
  { 400, "Bad Request" }, 
  { 403, "Forbidden" }, 
  { 404, "Not Found" }, 
  { 405, "Method Not Allowed" }, 
  { 408, "Request Timeout" }, 
  { 500, "Internal Server Error" }, 
  { 505, "HTTP Version Not Supported" }, 
};

CAPI WebResponse*	webresponse_create (unsigned int _socket)
{
	WebResponse* wr = (WebResponse*) calloc(1,sizeof(WebResponse));
  if (wr)
  {
    wr->netsocket = _socket;
    wr->chunked = false;
    wr->memory = 0;
    wr->method = 0;
    wr->location = 0;
    wr->n_args = 0;
    wr->args = 0;
    wr->n_headers = 0;
    wr->headers = 0;
  }
	return wr;
}

CAPI void webresponse_destroy (WebResponse* _context)
{
	free (_context->memory);
	closesocket(_context->netsocket); 
	free (_context);
}

CAPI int webresponse_write(WebResponse* _context, const void* _memory, const int _size)
{
  return send(_context->netsocket, (const char*)_memory, (int)_size, 0);
}

CAPI int webresponse_read(WebResponse* _context, void* _memory, const int _size)
{
  return recv(_context->netsocket, (char*)_memory, (int)_size, 0);
}

static int comparePairs(const void* l, const void* r)
{
  const HttpHeader* pl = (const HttpHeader*)l;
  const HttpHeader* pr = (const HttpHeader*)r;
  return strcmp(pl->name, pr->name);
}

CAPI bool webresponse_parse(WebResponse* _context)
{
  const static int RECEIVE_BUFFER_SIZE = 8 * 1024;
  char buffer[RECEIVE_BUFFER_SIZE];
  int bytesRead = webresponse_read(_context, buffer, RECEIVE_BUFFER_SIZE - 1);
  if (bytesRead <= 0)
  {
    return webresponse_response(_context, 500, 0, 0, 0);
  }
  buffer[bytesRead] = 0;

  // extract method
  char* method = buffer;
  char* eom = strchr(method, ' ');
  if (0 == eom)
  {
    return webresponse_response(_context, 500, 0, 0, 0);
  }
  *eom++ = 0; // terminate method

  // extract location and (optional) arguments
  char* location = eom;
  char* eol = strstr(location, " HTTP/1.1\r\n");
  if (0 == eol)
  {
    return webresponse_response(_context, 500, 0, 0, 0);
  }
  *eol = 0;	// terminate location

  char* args = strchr(location, '?');
  if (args)
  {
    *args++ = 0; // args is now pointing to the argument list
  }
  else
  {
    args = eol;	// empty arguments
  }
  eol += strlen(" HTTP/1.1\r\n");

  char* eoh = 0;			// end of header

  if (0 == strcmp(method, "POST") || 0 == strcmp(method, "GET"))
  {
    // mozilla sends the header in two chunks, yeah.
    eoh = strstr(eol, "\r\n\r\n");
    if (0 == eoh)
    {
      bytesRead += webresponse_read(_context, buffer + bytesRead, RECEIVE_BUFFER_SIZE - bytesRead - 1);
      buffer[bytesRead] = 0;
    }
    
    if (0 == eoh)
      return webresponse_response(_context, 500, 0, 0, 0);
    
    eoh += 4; // end-of-header now points directly to the first byte of content
  }
  else
  {
    return webresponse_response(_context, 500, "unsupported method", 0, 0);
  }

  // method .. eom : size of method
  // args .. eol : size of GET arguments
  // eol .. eoh-2 : size of headers
  // eoh .. buffer+bytesRead ; size of POST arguments

  _context->n_headers = -1;
  int hchars = 0;
  char* headers = eol;
  for (; headers[hchars]; ++hchars)
  {
    if (headers[hchars] == '\r')
    {
      _context->n_headers++;
    }
  }

  // how many name=value pairs should be allocated:
  _context->n_args = 0;	// count pairs
  int chars = 0;	// count characters
  int bytesLeft = bytesRead - (int)(eoh - buffer);
  for (; args[chars]; ++chars)
  {
    if (args[chars] == '=') _context->n_args++;
  }
  if (0 == strcmp("POST", method))
  {
    for (const char* p = eoh; p < eoh + bytesLeft; ++p)
      if (*p == '=') _context->n_args++;
  }

  // count 

  location = uri_decode_inplace(location);

  int locLen = (int)strlen(location) + 1;
  int metLen = (int)strlen(method) + 1;

  _context->memory = (char*) calloc (1,_context->n_args * sizeof(HttpHeader) + _context->n_headers * sizeof(HttpHeader) + hchars + 1 + chars + bytesLeft + 1 + locLen + metLen);
  _context->location = _context->memory + _context->n_headers * sizeof(HttpHeader) + _context->n_args * sizeof(HttpHeader) + hchars + 1 + chars + bytesLeft + 1;
  _context->method = _context->location + locLen;
  _context->args = (HttpHeader*)(_context->memory);
  _context->headers = (HttpHeader*)(_context->memory + _context->n_args * sizeof(HttpHeader));

  strcpy(_context->location, location);
  strcpy(_context->method, method);

  if (_context->n_headers && hchars)
  {
    char* strings = _context->memory + _context->n_headers * sizeof(HttpHeader) + _context->n_args * sizeof(HttpHeader);
    if (headers)
      strcpy(strings, headers);
    strings[hchars] = 0;
    
    int pair = 0;
    char* last = strings;
    int i = 0;
    bool needeol = false;
    for (; i < hchars; ++i)
    {
      if (strings[i] == ':' && !needeol)
      {
        needeol = true;
        _context->headers[pair].name = last;
        last = strings + i + 1;
        strings[i] = 0;
      }
      if (strings[i] == '\r' && needeol)
      {
        needeol = false;
        _context->headers[pair].value = last;
        ++pair;
        last = strings + i + 1;
        strings[i] = 0;
      }
    }
    if (pair < _context->n_headers)
    {
      _context->headers[pair].value = last;
      strings[i] = 0;
    }
    
    for (int i=0; i<_context->n_headers; ++i)
    {
    	uri_decode_inplace(_context->headers[i].name);
    	uri_decode_inplace(_context->headers[i].value);
    }
    
    if (_context->n_headers)
      qsort(_context->headers, _context->n_headers, sizeof(HttpHeader), &comparePairs);
  }


  if (_context->n_args && (chars || bytesLeft))
  {
    char* strings = _context->memory + _context->n_headers * sizeof(HttpHeader) + _context->n_args * sizeof(HttpHeader) + hchars + 1;
    if (args)
      strcpy(strings, args);
    if (bytesLeft)
      strncpy(strings + chars, eoh, bytesLeft);
    strings[chars + bytesLeft] = 0;
    
    int pair = 0;
    char* last = strings;
    int i = 0;
    for (; i < chars + bytesLeft; ++i)
    {
      if (strings[i] == '=')
      {
        _context->args[pair].name = last;
        last = strings + i + 1;
        strings[i] = 0;
      }
      if (strings[i] == '&')		// GET
      {
        _context->args[pair].value = last;
        ++pair;
        last = strings + i + 1;
        strings[i] = 0;
      }
    }
    if (pair < _context->n_args)
    {
      _context->args[pair].value = last;
      strings[i] = 0;
    }
    
    for (int i = 0; i < _context->n_args; ++i)
    {
      uri_decode_inplace(_context->args[i].name);
      uri_decode_inplace(_context->args[i].value);
    }
    
    if (_context->n_args)
      qsort(_context->args, _context->n_args, sizeof(HttpHeader), &comparePairs);
  }

  return true;
}

CAPI int webresponse_writef(WebResponse* _context, const char* _fmt, ...)
{
  char buf[4096];
  va_list ap;

  va_start(ap, _fmt);
  int len = vsprintf(buf, _fmt, ap);
  va_end(ap);

  if (0 == len)
  return 0;

  if (_context->chunked && len)
  {
    char num[10];
    sprintf(num, "%x\r\n", len);
    webresponse_write(_context, num, (int)strlen(num));
    len = webresponse_write(_context, buf, len);
    webresponse_write(_context, "\r\n", 2);
    return len;
  }

  return webresponse_write(_context, buf, len);
}

CAPI bool webresponse_response (WebResponse* _context, unsigned int _code, const char* _content, const size_t _contentLength, const char* _userHeader)
{
  _context->chunked = false;

  const char* message = "???";
  // for this few messages we don't need binsearch
  for (unsigned int i = 0; i < sizeof(HttpErrorMessages) / sizeof(HttpErrorMessages[0]); ++i)
  {
    if (HttpErrorMessages[i].code == _code)
    {
      message = HttpErrorMessages[i].message;
      break;
    }
  }

  // setup automatic parameters
  size_t contentLength = (0 == _contentLength && _content) ? strlen(_content) : _contentLength;
  const char* userHeader = _userHeader ? _userHeader : "Content-Type: text/html\r\n";

  // send HTTP header
  webresponse_writef(_context, "HTTP/1.1 %03d %s\r\n"
           "Server: dbalster/httpd\r\n"
           "Cache-Control: no-cache\r\n"
           "Content-Length: %d\r\n"
           "%s"
           "\r\n", _code, message, contentLength, userHeader);

  // write the actual content (the "page")
  if (_content)
  {
    webresponse_write(_context, _content, (int)contentLength);
  }

  return false;
}

CAPI const char* webresponse_location (WebResponse* _context)
{
	return _context->location;
}

CAPI const char*	webresponse_method(WebResponse* _context)
{
	return _context->method;
}

CAPI int webresponse_get_n_args(WebResponse* _context)
{
	return _context->n_args;
}

CAPI int webresponse_get_n_headers(WebResponse* _context)
{
	return _context->n_headers;
}

CAPI void webresponse_begin(WebResponse* _context, unsigned int _code, const char* _userHeader)
{
  const char* message = "???";
  // for this few messages we don't need binsearch
  for (unsigned int i = 0; i < sizeof(HttpErrorMessages) / sizeof(HttpErrorMessages[0]); ++i)
  {
    if (HttpErrorMessages[i].code == _code)
    {
      message = HttpErrorMessages[i].message;
      break;
    }
  }

  const char* userHeader = _userHeader ? _userHeader : "Content-Type: text/html\r\n";

  // send HTTP header
  webresponse_writef(_context, "HTTP/1.1 %03d %s\r\n"
           "Server: dbalster/http\r\n"
           "Cache-Control: no-cache\r\n"
           "Transfer-Encoding: chunked\r\n"
           "%s"
           "\r\n", _code, message, userHeader);

  _context->chunked = true;
}

CAPI void webresponse_end(WebResponse* _context )
{
    webresponse_write(_context, "0\r\n\r\n", 5);
    _context->chunked = false;
}

CAPI const char* webresponse_get_arg(WebResponse* _context, const char* _key) 
{
	HttpHeader p;
	p.name = ((char*)_key);
  const HttpHeader* result = (const HttpHeader*)bsearch(&p, _context->args, _context->n_args, sizeof(HttpHeader), &comparePairs);
  if (result) return result->value;
  return 0;
}

CAPI const HttpHeader* webresponse_get_arg_by_index(WebResponse* _context, int _index) 
{
  if (_index < 0 || _index >= _context->n_args) return 0;
  return &(_context->args[_index]);
}

CAPI const char* webresponse_get_header(WebResponse* _context, const char* _key) 
{
	HttpHeader p;
	p.name = ((char*)_key);
	const HttpHeader* result = (const HttpHeader*)bsearch(&p, _context->headers, _context->n_headers, sizeof(HttpHeader), &comparePairs);
  if (result) return result->value;
  return 0;
}

CAPI const HttpHeader* webresponse_get_header_by_index(WebResponse* _context, int _index) 
{
  if (_index < 0 || _index >= _context->n_headers) return 0;
  return &(_context->headers[_index]);
}
