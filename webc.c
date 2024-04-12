#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <mysql/mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// This structure defines a way to store the received data from the API call.
struct MemoryStruct {
  char *memory;
  size_t size;
};

// This function is a callback function used by CURL during the transfer. It's
// called repeatedly with chunks of data received from the server.
size_t write_data(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;
  char *ptr = realloc(mem->memory, mem->size + realsize + 1);
  if (!ptr) {
    printf("Not enough memory (realloc returen NULL)\n");
    return 0;
  }

  mem->memory = ptr;
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;
  return realsize;
};

int main(void) {
  /** API conecction **/
  CURL *curl;
  CURLcode res;

  struct MemoryStruct chunk;

  chunk.memory = malloc(1);
  chunk.size = 0;

  curl_global_init(CURL_GLOBAL_ALL);

  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_URL,
                   "https://jsonplaceholder.typicode.com/users");
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
  res = curl_easy_perform(curl);

  if (res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform() failed: %s\n",
            curl_easy_strerror(res));
    return 1;
  }
  curl_easy_cleanup(curl);

  /** JSON **/
  cJSON *json = cJSON_Parse(chunk.memory);

  if (json == NULL) {
    fprintf(stderr, "Error parsing JSON data: %s\n", cJSON_GetErrorPtr());
  } else {
    // Check if the parsed data is an array
    if (cJSON_IsArray(json)) {
      int i;
      cJSON *user;
      // Loop through each user object in the array
      for (i = 0; i < cJSON_GetArraySize(json); i++) {
        user = cJSON_GetArrayItem(json, i);
        // Check if the user object is valid
        if (cJSON_IsObject(user)) {
          // Get the "name" key-value pair
          cJSON *name = cJSON_GetObjectItem(user, "name");
          // Check if the "name" key exists and is a string
          if (cJSON_IsString(name)) {
            printf("User %d: %s\n", i + 1, name->valuestring);
          } else {
            fprintf(stderr, "Error: 'name' key not found or not a string\n");
          }
        } else {
          fprintf(stderr, "Error: Unexpected data type at index %d\n", i);
        }
      }
    } else {
      fprintf(stderr, "Error: Expected JSON array\n");
    }
    cJSON_Delete(json);
  }
  /** MYSQL **/

  free(chunk.memory);
  curl_global_cleanup();
  return 0;
}
