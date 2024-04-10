# webc

This is my first try making a C HTTP client that will: 

1) Receive some data from  a WEB API:

Using CURL_EASY functions from libcurl.h to make the request. It works realy well.

I used the prototype WRITEDATA function in the libcurl documentation to store the the data e return a pointer that will be used in the next step.

struct memory {
  char *response;
  size_t size;
};
 
static size_t cb(void *data, size_t size, size_t nmemb, void *clientp)
{
  size_t realsize = size * nmemb;
  struct memory *mem = (struct memory *)clientp;
 
  char *ptr = realloc(mem->response, mem->size + realsize + 1);
  if(!ptr)
    return 0;  /* out of memory! */
 
  mem->response = ptr;
  memcpy(&(mem->response[mem->size]), data, realsize);
  mem->size += realsize;
  mem->response[mem->size] = 0;
 
  return realsize;

Since I'm trying to learn C syntax, a lot of time was spend to understand how that function works.

2) Parse the Json content; 

3) Save the data in a MYSQL database.
