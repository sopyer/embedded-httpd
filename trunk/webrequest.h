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

#ifndef DBALSTER_WEBREQUEST_H
#define DBALSTER_WEBREQUEST_H

#include "config.h"

typedef struct _WebRequest WebRequest;

// simple wrapper for a http request
// just recv's the result into a buffer, that's it.

CAPI WebRequest* webrequest_create( const char* _hostname, unsigned short _port, const char* _location, const char* _method, size_t _maxBytes );
CAPI void webrequest_sprintf( WebRequest* _req, const char* _fmt, ... );
CAPI void webrequest_strcat( WebRequest* _req, const char* _orig );
CAPI bool webrequest_execute( WebRequest* _req );
CAPI int webrequest_get_result( WebRequest* _req );
CAPI const char* webrequest_get_header( WebRequest* _req, const char* _header );
CAPI const char* webrequest_get_content( WebRequest* _req );
CAPI size_t webrequest_get_content_length( WebRequest* _req );
CAPI void webrequest_destroy( WebRequest* _req );
CAPI void webrequest_reset( WebRequest* _req );

#endif
