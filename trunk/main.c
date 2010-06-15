#include <stdio.h>
#include <string.h>

#include "webserver.h"
#include "webresponse.h"
#include "webrequest.h"

static void indexpage( WebResponse* R )
{
  webresponse_response(R, 220, "Hello, world!", 0, 0);
}

static void svgpage( WebResponse* R )
{
  webresponse_begin(R, 220, "Content-Type: text/xml\r\n");
  
  webresponse_writef(R,
                     "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                     "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n"
                     "\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">"
                     "<html xml:lang=\"en\" xmlns=\"http://www.w3.org/1999/xhtml\" xmlns:svg=\"http://www.w3.org/2000/svg\">"
                     "<head>"
                     "<title>svg</title>"
                     "</head>"
                     "<body>"
                     );
  
  webresponse_writef(R, "<h1>%s</h1>",webresponse_location(R));
  
  webresponse_writef(R,
   "<svg:svg version=\"1.1\">"
   "<svg:rect style=\"fill:#f73\" id=\"x\" width=\"300 px\" height=\"300 px\" x=\"0 px\" y=\"0 px\"/>"
   "</svg:svg> "
   );
  
  
  webresponse_writef(R, "</body></html>");
  webresponse_end(R);
}

static CAPI void inputpage( WebResponse* R )
{
  const char* name = webresponse_get_arg(R, "field1");
  if (name==0) name = "???";
  
  webresponse_begin(R,220,0);
  
  webresponse_writef(R, "<h2>http headers</h2>");
  for (size_t i=0; i<webresponse_get_n_headers(R); ++i)
  {
    const HttpHeader* hdr = webresponse_get_header_by_index(R, i);
    webresponse_writef(R,"<li><b>%s</b> : %s</li>",hdr->name,hdr->value);
  }
  webresponse_writef(R, "<h2>http parameters (POST+GET)</h2>");
  for (size_t i=0; i<webresponse_get_n_args(R); ++i)
  {
    const HttpHeader* hdr = webresponse_get_arg_by_index(R, i);
    webresponse_writef(R,"<li><b>%s</b> : %s</li>",hdr->name,hdr->value);
  }
  
  webresponse_writef(R,"<html><body><form method=\"POST\" action=\"%s\">",webresponse_location(R));
  // yes, user input needs to be verified, but that's not the topic of this example
  webresponse_writef(R,"<input type=\"text\" name=\"field1\" value=\"%s\"/>",name);
  webresponse_writef(R,"<input type=\"submit\"/>");
  webresponse_writef(R,"</form></body></form>");
  
  webresponse_end(R);
}

static CAPI void http_handler( WebResponse* R, void* _userdata )
{
  // normally you would use a hashtable, map or something similar here
  const char* loc = webresponse_location(R);
  if (0==strcmp(loc,"/")) indexpage(R);
  else if (0==strcmp(loc,"/svg")) svgpage(R);
  else if (0==strcmp(loc,"/input")) inputpage(R);
  else
    webresponse_response(R, 404, "<h1>not really found</h1>", 0, 0);

}

void download_something()
{
  // important: the buffer is re-used all over again and prevents allocations
  WebRequest* req = webrequest_create("localhost", 80, "/foo/bar/filename.zip", "GET", 256*1024);
  if (req)
  {
    webrequest_execute(req);
    const char* content = webrequest_get_content(req);
    size_t size = strtoul(webrequest_get_header(req,"Content-Length:"),0,0);
    if (content && size)
    {
      // OK
    }
    webrequest_destroy(req);
  }
}

int main (int argc, const char * argv[])
{
  printf("server runs on http://localhost:8080/\n(default port 80 requires admin rights)\n");
  
  WebServer* srv = webserver_create(8080, http_handler, 0);
  if (srv)
  {
    while (1)
    {
      webserver_process(srv, true);
    }
    webserver_destroy(srv);
  }

  return 0;
}
