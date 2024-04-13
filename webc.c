#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <mysql/field_types.h>
#include <mysql/mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

MYSQL *conn;
MYSQL_STMT *stmt;
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
  char *url = "https://jsonplaceholder.typicode.com/users";

  struct MemoryStruct chunk;

  chunk.memory = malloc(1);
  chunk.size = 0;

  curl_global_init(CURL_GLOBAL_ALL);

  curl = curl_easy_init();
  if (curl == NULL) {
    fprintf(stderr, "curl_easy_init() failed\n");
    return 1;
  }
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
  res = curl_easy_perform(curl);

  if (res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform() failed: %s\n",
            curl_easy_strerror(res));
    curl_easy_cleanup(curl);
    return 1;
  }
  curl_easy_cleanup(curl);

  /** MYSQL **/
  const char *server = "";
  const char *user = "";
  const char *password = "";
  const char *database = "";

  conn = mysql_init(NULL);
  // Handle connection error (e.g., free memory and return)
  if (conn == NULL) {
    fprintf(stderr, "Error initializing MySQL connection: %s\n",
            mysql_error(conn));
  }
  // Handle connection error (e.g., free memory and return)
  if (mysql_real_connect(conn, server, user, password, database, 0, NULL, 0) ==
      NULL) {
    fprintf(stderr, "Error connecting to MySQL: %s\n", mysql_error(conn));
    mysql_close(conn);
  }

  /** JSON **/
  cJSON *json = cJSON_Parse(chunk.memory);

  if (json == NULL) {
    fprintf(stderr, "Error parsing JSON data: %s\n", cJSON_GetErrorPtr());
  } else {
    // Check if the parsed data is an array
    if (cJSON_IsArray(json)) {
      cJSON *user;
      // Loop through each user object in the array
      cJSON_ArrayForEach(user, json) {
        // Check if the user object is valid
        if (cJSON_IsObject(user)) {
          // Get the object key-value pair
          cJSON *name = cJSON_GetObjectItem(user, "name");
          cJSON *email = cJSON_GetObjectItem(user, "email");
          cJSON *id = cJSON_GetObjectItem(user, "id");
          // Check if the object key exists and is the correct data type
          if (cJSON_IsString(name) && (cJSON_IsString(email)) &&
              (cJSON_IsNumber(id))) {
            // Escape data for SQL injection prevention
            char escaped_name[255];
            char escaped_emai[255];
            mysql_real_escape_string(conn, escaped_name, name->valuestring,
                                     strlen(name->valuestring));
            mysql_real_escape_string(conn, escaped_name, email->valuestring,
                                     strlen(email->valuestring));
            // Prepare SQL statement
            const char *sql = "INSERT INTO TESTE_USUARIO (name, username, id) "
                              "VALUES(?, ?, ?)";
            MYSQL_STMT *stmt = mysql_stmt_init(conn);
            if (!stmt) {
              fprintf(stderr, "Erro: mysql_stmt_init(), out of memory\n");
              exit(0);
            }
            // Prepare the statement with placeholders
            if (mysql_stmt_prepare(stmt, sql, strlen(sql))) {
              fprintf(stderr, "mysql_stmt_prepare(), INSERT failed\n");
              fprintf(stderr, "%s\n", mysql_stmt_error(stmt));
              exit(0);
            }
            fprintf(stdout, "prepare, INSERT succesful\n");
            // Get the parameter count from the statement
            int param_count = mysql_stmt_param_count(stmt);
            fprintf(stdout, "total parameters in INSERT: %d\n", param_count);
            if (param_count != 3) {
              fprintf(stderr, "invalid parameter count returned by MySQL\n");
              exit(0);
            }

            // Bind values to placeholders in the prepared statement
            MYSQL_BIND bind[3];
            memset(bind, 0, sizeof(bind));
            bind[0].buffer_type = MYSQL_TYPE_STRING;
            bind[0].buffer = name->valuestring;
            bind[0].buffer_length = strlen(name->valuestring);

            bind[1].buffer_type = MYSQL_TYPE_STRING;
            bind[1].buffer = email->valuestring;
            bind[1].buffer_length = strlen(email->valuestring);
            // No need to specify buffer_length for number type
            bind[2].buffer_type = MYSQL_TYPE_LONG;
            bind[2].buffer = (char *)&id->valueint;
            bind[2].buffer_length = 0;

            // Bind the buffers
            if (mysql_stmt_bind_param(stmt, bind)) {
              fprintf(stderr, "mysql_stmt_bind_param() failed\n");
              fprintf(stderr, "%s\n", mysql_stmt_error(stmt));
              exit(0);
            }
            // Execute the INSERT statement
            if (mysql_stmt_execute(stmt)) {
              fprintf(stderr, " mysql_stmt_execute() failed\n");
              fprintf(stderr, " %s\n", mysql_stmt_error(stmt));
              exit(0);
              if (mysql_stmt_close(stmt)) {
                // mysql_stmt_close() invalidates stmt, so call          //
                // mysql_error(mysql) rather than mysql_stmt_error(stmt) //
                fprintf(stderr, " failed while closing the statement\n");
                fprintf(stderr, " %s\n", mysql_error(conn));
                exit(0);
              }
            }
          } else {
            fprintf(stderr, "Error: key not found or not a correct type\n");
          }
        } else {
          fprintf(stderr,
                  "Error: cJSON_IsObject() unexpected data type in array\n");
        }
      }
    } else {
      fprintf(stderr, "Error: cJSON_IsArray() expected JSON array\n");
    }

    cJSON_Delete(json);
  }

  mysql_close(conn);
  free(chunk.memory);
  curl_global_cleanup();
  return 0;
}
