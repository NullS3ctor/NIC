#include "network/http_server.h"

const char* get_mime_type(const char* filename){
    const char* dot = strrchr(filename, '.');
    if (!dot) return MIME_TXT;

    if (strcmp(dot, ".html") == 0) return MIME_HTML;
    if (strcmp(dot, ".css")  == 0) return MIME_CSS;
    if (strcmp(dot, ".js")   == 0) return MIME_JS;
    if (strcmp(dot, ".jpg")  == 0) return MIME_JPG;
    if (strcmp(dot, ".png")  == 0) return MIME_PNG;
    if (strcmp(dot, ".txt")  == 0) return MIME_TXT;

    // Default if extension not found
    return MIME_TXT;
};


void parse_request(char* raw_request, HttpRequest* request){
    
    memset(request, 0, sizeof(HttpRequest));

    int items_read = sscanf(raw_request, "%9s %255s %9s",
                            request->method,
                            request->path,
                            request->version);
    if (items_read < 3) {
        printf("ERROR: La peticion HTTP esta incompleta o mal formada");
        return;
    }


    if (strcmp(request->path, "/") == 0) {
        strcpy(request->path, "/index.html");
    }


    // Search for content-length header
    char* len_pos = strstr(raw_request, "Content-Length:");

    if (len_pos) {
        sscanf(len_pos, "Content-Length: %d", &request->content_length);

    } else {
        request->content_length = 0;

    }
    // Search for the beginning of the body

    char* body_pos = strstr(raw_request, "\r\n\r\n");

    if(body_pos) {
        request->body = body_pos + 4;

    }
    else {
        request->body = NULL;
    }

    /* DEBUG */
    printf("DEBUB -> Metodo:%s | Ruta:%s | Version:%s",
        request->method,request->path,request->version);
}


void handle_request(const HttpRequest* request, char* response_buffer, size_t* response_len) {
    if (strcmp(request->method, "GET") == 0) {
        handle_get(request, response_buffer, response_len);
    } 
    else if (strcmp(request->method, "HEAD") == 0) {
        handle_head(request, response_buffer, response_len);
    } 
    else if (strcmp(request->method, "POST") == 0) {
        handle_post(request, response_buffer, response_len);
    }
    else if (strcmp(request->method, "PUT") == 0) {
        handle_put(request, response_buffer, response_len);
    }
    else if (strcmp(request->method, "DELETE") == 0) {
        handle_delete(request, response_buffer, response_len);
    }  
    else {
        // Método no soportado (501)
        sprintf(response_buffer, "HTTP/1.1 %s\r\n\r\nMethod not implemented.", HTTP_STATUS_501);
        *response_len = strlen(response_buffer);
    }
}


void handle_get(const HttpRequest* request, char* response_buffer, size_t* response_len){
    char filepath[MAX_PATH + 1];

    snprintf(filepath, sizeof(filepath), ".%s", request->path);
    

    FILE* file = fopen(filepath, "rb");

    if (!file) {
        sprintf(response_buffer, 
            "HTTP/1.1 %s\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: 13\r\n"
            "\r\n404 Not Found", 
            HTTP_STATUS_404, MIME_TXT);
        
        *response_len = strlen(response_buffer);
        return;
    }

    fseek(file, 0, SEEK_END);      
    long file_size = ftell(file);  
    rewind(file);                  

    
    const char* mime_type = get_mime_type(filepath);


    int header_len = sprintf(response_buffer,
            "HTTP/1.1 %s\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %ld\r\n"
            "Connection: close\r\n"
            "\r\n", 
            HTTP_STATUS_200, mime_type, file_size);


    size_t bytes_read = fread(response_buffer + header_len, 1, file_size, file);
    

    *response_len = header_len + bytes_read;
    fclose(file);
}


void handle_head(const HttpRequest* request, char* response_buffer, size_t* response_len){
    char filepath[MAX_PATH + 1];

    snprintf(filepath, sizeof(filepath), ".%s", request->path);
    

    FILE* file = fopen(filepath, "rb");

    if (!file) {
        sprintf(response_buffer, 
            "HTTP/1.1 %s\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: 13\r\n"
            "\r\n404 Not Found", 
            HTTP_STATUS_404, MIME_TXT);
        
        *response_len = strlen(response_buffer);
        return;
    }

    fseek(file, 0, SEEK_END);      
    long file_size = ftell(file);  
    rewind(file);                  

    
    const char* mime_type = get_mime_type(filepath);


    int header_len = sprintf(response_buffer,
            "HTTP/1.1 %s\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %ld\r\n"
            "Connection: close\r\n"
            "\r\n", 
            HTTP_STATUS_200, mime_type, file_size);
    

    *response_len = header_len;
    fclose(file);
}

void handle_delete(const HttpRequest* request, char* response_buffer, size_t* response_len){
    char filepath[MAX_PATH + 1];
    snprintf(filepath, sizeof(filepath), ".%s", request->path);
    int header_len;

    if(remove(filepath) == 0) {
        header_len = sprintf(response_buffer,
                                "HTTP/1.1 %s\r\n"
                                "Connection: close\r\n"
                                "\r\n", HTTP_STATUS_204);
    } else {
        const char* error_msg = "File not found or protected";

        header_len = sprintf(response_buffer,
                                "HTTP/1.1 %s\r\n"
                                "Content-Type: text/plain\r\n"
                                "Content-Length: %ld\r\n"
                                "Connection: close\r\n"
                                "\r\n"
                                "%s", HTTP_STATUS_404, strlen(error_msg), error_msg);

    }

    *response_len = header_len;
    
    
}


void handle_put(const HttpRequest* request, char* response_buffer, size_t* response_len){
    char filepath[MAX_PATH + 1];
    snprintf(filepath, sizeof(filepath), ".%s", request->path);

    int header_len;

    if (request->content_length <= 0 || request->body == NULL) {
        char* error_msg = "No content provided.";

        header_len = sprintf(response_buffer,
                                "HTTP/1.1 %s\r\n"
                                "Content-Type: text/plain\r\n"
                                "Content-Length: %ld\r\n"
                                "Connection: close\r\n"
                                "\r\n"
                                "%s", HTTP_STATUS_400, strlen(error_msg), error_msg);

        *response_len = header_len;
        return;
    }

    FILE* file = fopen(filepath, "wb");

    if (!file){
        char* error_msg = "Cannot write file.";

        header_len = sprintf(response_buffer,
                                "HTTP/1.1 %s\r\n"
                                "Content-Type: text/plain\r\n"
                                "Content-Length: %ld\r\n"
                                "Connection: close\r\n"
                                "\r\n"
                                "%s", HTTP_STATUS_500, strlen(error_msg), error_msg);


    } else {
        fwrite(request->body, 1, request->content_length, file);
        fclose(file);

        header_len = sprintf(response_buffer,
                                "HTTP/1.1 %s\r\n"
                                "Content-Type: text/plain\r\n"
                                "Connection: close\r\n"
                                "\r\n", HTTP_STATUS_201);
    }
    *response_len = header_len;  
}


void handle_post(const HttpRequest* request, char* response_buffer, size_t* response_len){

    int header_len;

    if (request->content_length <= 0) {
        char* error_msg = "Empty POST.";

        header_len = sprintf(response_buffer,
                                "HTTP/1.1 %s\r\n"
                                "Content-Type: text/plain\r\n"
                                "Content-Length: %ld\r\n"
                                "Connection: close\r\n"
                                "\r\n"
                                "%s", HTTP_STATUS_400, strlen(error_msg), error_msg);
        *response_len = header_len;
        return;
    }

    FILE* file = fopen("post_log.txt", "a");

    if (!file){
        char* error_msg = "An error has occurred.";

        header_len = sprintf(response_buffer,
                                "HTTP/1.1 %s\r\n"
                                "Content-Type: text/plain\r\n"
                                "Content-Length: %ld\r\n"
                                "Connection: close\r\n"
                                "\r\n"
                                "%s", HTTP_STATUS_500, strlen(error_msg), error_msg);


    } else {
        fprintf(file, "--- Nuevo POST (%d bytes) ---\n", request->content_length);
        fwrite(request->body, 1, request->content_length, file);
        fprintf(file, "\n");
        fclose(file);

        char* msg = "POST received and logged.";

        header_len = sprintf(response_buffer,
                                "HTTP/1.1 %s\r\n"
                                "Content-Type: text/plain\r\n"
                                "Content-Length: %ld\r\n"
                                "Connection: close\r\n"
                                "\r\n"
                                "%s", HTTP_STATUS_200, strlen(msg), msg);
    }
    *response_len = header_len;  
}