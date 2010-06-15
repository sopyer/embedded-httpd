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

#ifndef DBALSTER_WEBRESPONSE_H
#define DBALSTER_WEBRESPONSE_H

#include "config.h"

typedef struct _WebResponse WebResponse;
typedef struct _HttpHeader HttpHeader;

struct _HttpHeader
{
  char* name;
  char* value;
};

CAPI WebResponse* webresponse_create (unsigned int _socket);
CAPI void webresponse_destroy (WebResponse* _context);
CAPI bool webresponse_parse(WebResponse* _context);
CAPI bool webresponse_response(WebResponse* _context, unsigned int _code, const char* _content, size_t _contentLength, const char* _userHeader);
CAPI int webresponse_write(WebResponse* _context, const void* _memory, const int _size);
CAPI void	webresponse_begin(WebResponse* _context, unsigned int _code, const char* _userHeader);
CAPI int webresponse_writef(WebResponse* _context, const char* _fmt, ...);   
CAPI void	webresponse_end(WebResponse* _context);
CAPI const char* webresponse_location (WebResponse* _context);
CAPI const char* webresponse_method(WebResponse* _context);
CAPI int webresponse_get_n_args(WebResponse* _context);
CAPI int webresponse_get_n_headers(WebResponse* _context);
CAPI const char* webresponse_get_arg(WebResponse* _context, const char* _key);
CAPI const HttpHeader* webresponse_get_arg_by_index(WebResponse* _context, int _index);
CAPI const char* webresponse_get_header(WebResponse* _context, const char* _key);
CAPI const HttpHeader*	webresponse_get_header_by_index(WebResponse* _context, int _index);

#endif

