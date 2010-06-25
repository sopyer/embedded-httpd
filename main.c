#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "httpd.h"

void* loadfile( const char* _filename, size_t* _size )
{
  void* mem = 0;
  FILE* file = fopen(_filename,"rb");
  if (file)
  {
    fseek(file, 0, SEEK_END);
    *_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    mem = malloc(*_size);
    fread(mem, 1, *_size, file);
    fclose(file);
  }
  return mem;
}

static void indexpage( HttpResponse* R )
{
  // simple response, in one piece.
  // it just answers the incoming request with one single output string
  // if content-size is zero, strlen is used to determine the content size
  httpresponse_response(R, 220, "Hello, world!", 0, 0);
}

static void svgpage( HttpResponse* R )
{
  // a chunked request. every "writef" directly writes to the outgoing socket.
  // this increases the overhead/timing of the whole response, but it doesn't require
  // a dynamic buffer management - which is perfect for embedded devices
  
  // in this case, it sends a mixed SVG/HTML document
  
  httpresponse_begin(R, 220, "Content-Type: text/xml\r\n");
  
  httpresponse_writef(R,
                     "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                     "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n"
                     "\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">"
                     "<html xml:lang=\"en\" xmlns=\"http://www.w3.org/1999/xhtml\" xmlns:svg=\"http://www.w3.org/2000/svg\">"
                     "<head>"
                     "<title>svg</title>"
                     "</head>"
                     "<body>"
                     );
  
  httpresponse_writef(R, "<h1>%s</h1>",httpresponse_location(R));

  httpresponse_writef(R,
   "<svg:svg version=\"1.1\">"
   "<svg:rect style=\"fill:#f73\" id=\"x\" width=\"300 px\" height=\"300 px\" x=\"0 px\" y=\"0 px\"/>"
   "</svg:svg> "
   );

  httpresponse_writef(R, "</body></html>");
  
  // don't forget to "end" the response
  httpresponse_end(R);
}

static void inputpage( HttpResponse* R )
{
  // it doesn't matter if it's a POST or GET argument; you can get it
  // by its name. if the argument is unknown, a null pointer is returned
  //
  // internally all parameters/headers have already been sorted, _get_arg()
  // is using a binary search
  const char* name = httpresponse_get_arg(R, "field1");
  if (name==0) name = "???";
  
  httpresponse_begin(R,220,0);
  
  // this shows how to enumerate over all arguments and headers
  
  httpresponse_writef(R, "<h2>http headers</h2>");
  for (size_t i=0; i<httpresponse_get_n_headers(R); ++i)
  {
    const HttpHeader* hdr = httpresponse_get_header_by_index(R, i);
    httpresponse_writef(R,"<li><b>%s</b> : %s</li>",hdr->name,hdr->value);
  }
  httpresponse_writef(R, "<h2>http parameters (POST+GET)</h2>");
  for (size_t i=0; i<httpresponse_get_n_args(R); ++i)
  {
    const HttpHeader* hdr = httpresponse_get_arg_by_index(R, i);
    httpresponse_writef(R,"<li><b>%s</b> : %s</li>",hdr->name,hdr->value);
  }
  
  httpresponse_writef(R,"<html><body><form method=\"POST\" action=\"%s\">",httpresponse_location(R));
  // yes, user input needs to be verified, but that's not the topic of this example
  httpresponse_writef(R,"<input type=\"text\" name=\"field1\" value=\"%s\"/>",name);
  httpresponse_writef(R,"<input type=\"submit\"/>");
  httpresponse_writef(R,"</form></body></form>");
  
  httpresponse_end(R);
}

static void http_handler( HttpResponse* R, void* _userdata )
{
  // normally you would use a hashtable, map or something similar here
  const char* loc = httpresponse_location(R);
  if (0==strcmp(loc,"/")) indexpage(R);
  else if (0==strcmp(loc,"/svg")) svgpage(R);
  else if (0==strcmp(loc,"/input")) inputpage(R);
  else
  {
    void* mem;
    size_t size;
    mem = loadfile(httpresponse_location(R)+1,&size);
    if (mem)
    {
      httpresponse_response(R, 220, (const char*) mem, size, "Content-Type: text/xml\r\n");
    }
    else
    {
      httpresponse_response(R, 404, "<h1>not really found</h1>", 0, 0);
    }
  }

}

static void download_something()
{
  // important: the buffer is re-used all over again and prevents allocations
  HttpRequest* req = httprequest_create("localhost", 80, "/foo/bar/filename.zip", "GET", 256*1024);
  if (req)
  {
    httprequest_execute(req);
    const char* content = httprequest_get_content(req);
    size_t size = strtoul(httprequest_get_header(req,"Content-Length:"),0,0);
    if (content && size)
    {
      // OK
    }
    // frees the buffer, closes the connection
    httprequest_destroy(req);
  }
}

#ifdef WIN32
#include <winsock.h>
#pragma comment(lib,"wsock32.lib")
#endif

int main (int argc, const char * argv[])
{
#ifdef WIN32
  WSADATA data;
  WSAStartup(MAKEWORD(2,2),&data);
#endif

  printf("server runs on http://localhost:8080/\n(default port 80 requires admin rights)\n");
  
  Httpd* srv = httpd_create(8080, http_handler, 0);
  if (srv)
  {
    while (1)
    {
      // httpd_process can be used in polling or waiting mode
      httpd_process(srv, true);
    }
    httpd_destroy(srv);
  }

  return 0;
}
