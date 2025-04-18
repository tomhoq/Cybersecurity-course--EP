/**
 *  Jiazi Yi
 * LIX, Ecole Polytechnique
 * jiazi.yi@polytechnique.edu
 *
 * Updated by Pierre Pfister
 * Cisco Systems
 * ppfister@cisco.com
 *
 * Updated by Kevin Jiokeng
 * LIX, Ecole Polytechnique
 * kevin.jiokeng@polytechnique.edu
 *
 */

 /*
 * Refs:
 * For the getaddrinfo part of my computer Networks project was used
 * https://github.com/tomhoq/Networks-Project/blob/main/user/tcp.c
 *  inside function communicate_tcp
 *
 * For the reading while loop I also grabbed some old code from same project and adapted it
 * https://github.com/tomhoq/Networks-Project/blob/main/AS/server.c 
 * line 284
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "url.h"
#include "wgetX.h"

int main(int argc, char* argv[]) {
    url_info info;
    const char * file_name = "received_page";
    if (argc < 2) {
        fprintf(stderr, "Missing argument. Please enter URL.\n");
        return 1;
    }

    char *url = argv[1];

    // Get optional file name
    if (argc > 2) {
	    file_name = argv[2];
    }

    // First parse the URL
    int ret = parse_url(url, &info);
    if (ret) {
        fprintf(stderr, "Could not parse URL '%s': %s\n", url, parse_url_errstr[ret]);
        return 2;
    }

    //If needed for debug
    //print_url_info(&info);

    // Download the page
    struct http_reply reply;

    ret = download_page(&info, &reply);
    if (ret) {
        printf("Error downloading page\n");
	    return 3;
    }
    // Now parse the responses
    char *response = read_http_reply(&reply);
    if (response == NULL) {
        fprintf(stderr, "Could not parse http reply\n");
        return 4;
    } 
    int redirs = 0;
    if (has_location(response)) {
        ret = parse_url(response + 10, &info);
        if (ret) {
            fprintf(stderr, "Could not parse URL '%s': %s\n", url, parse_url_errstr[ret]);
            return 2;
        }
        ret = download_page(&info, &reply);
        if (ret) {
            return 3;
        }
        response = read_http_reply(&reply);
        if (response == NULL) {
            fprintf(stderr, "Could not parse http reply\n");
            return 4;
        } else if (has_location(response)) {
            redirs += 1;
            if (redirs > 5) {
                fprintf(stderr, "Too many redirections\n");
                return 5;
            }
            //continue;
        }
        write_data(file_name, response, reply.reply_buffer + reply.reply_buffer_length - response);
    }

    // Write response to a file
    write_data(file_name, response, reply.reply_buffer + reply.reply_buffer_length - response);

    // Free allocated memory
    free(reply.reply_buffer);

    // Just tell the user where is the file
    fprintf(stderr, "the file is saved in %s.", file_name);
    return 0;
}

int download_page(url_info *info, http_reply *reply) {

    /*
     * To be completed:
     *   You will first need to resolve the hostname into an IP address.
     *
     *   Option 1: Simplistic
     *     Use gethostbyname function.
     *
     *   Option 2: Challenge
     *     Use getaddrinfo and implement a function that works for both IPv4 and IPv6.
     *
     */
    struct addrinfo hints, *res, *p;
    int fd, t;
 
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    char port_str[7];
    snprintf(port_str, sizeof(port_str), "%d", info->port); 
    t = getaddrinfo(info->host, port_str, &hints, &res);
    if (t != 0) {
        printf("Error getting address info: %s\n", gai_strerror(t));
        return -1;
    }   
    for (p = res; p != NULL; p = p->ai_next) {
        fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd == -1) continue;  // Try next address if socket creation fails

        if (connect(fd, p->ai_addr, p->ai_addrlen) == 0) {
            break;  // Successfully connected
        }

        close(fd);  // Close the failed socket
    }
    freeaddrinfo(res);

    /*
     * To be completed:
     *   Next, you will need to send the HTTP request.
     *   Use the http_get_request function given to you below.
     *   It uses malloc to allocate memory, and snprintf to format the request as a string.
     *
     *   Use 'write' function to send the request into the socket.
     *
     *   Note: You do not need to send the end-of-string \0 character.
     *   Note2: It is good practice to test if the function returned an error or not.
     *   Note3: Call the shutdown function with SHUT_WR flag after sending the request
     *          to inform the server you have nothing left to send.
     *   Note4: Free the request buffer returned by http_get_request by calling the 'free' function.
     *
     */


    char *request = http_get_request(info);
     
   printf("%s\n", request);    
    if (write(fd, request, strlen(request)) == -1) {
         close(fd);
         free(request);
         return -1;
     }
     if (shutdown(fd, SHUT_WR) == -1 ) {
         close(fd);
         free(request);
         return -1;
     }
     free(request);

    /*
     * To be completed:
     *   Now you will need to read the response from the server.
     *   The response must be stored in a buffer allocated with malloc, and its address must be save in reply->reply_buffer.
     *   The length of the reply (not the length of the buffer), must be saved in reply->reply_buffer_length.
     *
     *   Important: calling recv only once might only give you a fragment of the response.
     *              in order to support large file transfers, you have to keep calling 'recv' until it returns 0.
     *
     *   Option 1: Simplistic
     *     Only call recv once and give up on receiving large files.
     *     BUT: Your program must still be able to store the beginning of the file and
     *          display an error message stating the response was truncated, if it was.
     *
     *   Option 2: Challenge
     *     Do it the proper way by calling recv multiple times.
     *     Whenever the allocated reply->reply_buffer is not large enough, use realloc to increase its size:
     *        reply->reply_buffer = realloc(reply->reply_buffer, new_size);
     *
     */


    size_t buffer_size = 4096;
    reply->reply_buffer = malloc(buffer_size);

    size_t total_received = 0;
    ssize_t received;
    while ((received = recv(fd, reply->reply_buffer + total_received, buffer_size - total_received, 0)) > 0) { //iterate until 0 bytes read
        total_received += received;
        if (total_received >= buffer_size) {
            buffer_size *= 2;
            reply->reply_buffer = realloc(reply->reply_buffer, buffer_size);
            if (!reply->reply_buffer) {
                printf("Error reallocating memory.\n");
                close(fd);
                return -1;
            }
        }
    }
    if (received == -1) {
        printf("Error reading from socket.\n");
        free(reply->reply_buffer);
        close(fd);
        return -1;
    }
    reply->reply_buffer_length = total_received;
    close(fd);

    return 0;
}

void write_data(const char *path, const char * data, int len) {
    /*
     * To be completed:
     *   Use fopen, fwrite and fclose functions.
     */

     FILE *file = fopen(path, "w");
     if (file == NULL) {
        printf("Failed opening file. Check permissions\n");
        return;
     }
     if (fwrite(data, 1, len, file) != len) {
        printf("Failed writing to file.\n");
        fclose(file);
        return;
     }
     
    fclose(file);
}

char* http_get_request(url_info *info) {
    
    char port_str[10] = "";
    if (info->port != 80) {
        snprintf(port_str, sizeof(port_str), ":%d", info->port);
    }

    char * request_buffer = (char *) malloc(100 + strlen(info->path) + strlen(info->host) + strlen(port_str));
    snprintf(request_buffer, 1024, "GET /%s HTTP/1.1\r\nHost: %s%s\r\nConnection: close\r\n\r\n",
	    info->path, info->host, port_str);
    return request_buffer;
}

char *next_line(char *buff, int len) {
    if (len == 0) {
	return NULL;
    }

    char *last = buff + len - 1;
    while (buff != last) {
	if (*buff == '\r' && *(buff+1) == '\n') {
	    return buff;
	}
	buff++;
    }
    return NULL;
}

int empty_line(char *buff) {
    return *buff == '\r' && *(buff+1) == '\n';
}

int has_location(char * buff) {
    return strncmp(buff, "Location: ", 10) == 0;
}

char *read_http_reply(struct http_reply *reply) {

    // Let's first isolate the first line of the reply
    char *status_line = next_line(reply->reply_buffer, reply->reply_buffer_length);
    if (status_line == NULL) {
	fprintf(stderr, "Could not find status\n");
	return NULL;
    }
    *status_line = '\0'; // Make the first line is a null-terminated string

    // Now let's read the status (parsing the first line)
    int status;
    double http_version;
    int rv = sscanf(reply->reply_buffer, "HTTP/%lf %d", &http_version, &status);
    if (rv != 2) {
	fprintf(stderr, "Could not parse http response first line (rv=%d, %s)\n", rv, reply->reply_buffer);
	return NULL;
    }
    
    char *buf = status_line + 2;

    printf("%d\n", status);

    if (status != 200 && status != 301) {
	fprintf(stderr, "Server returned status %d (should be 200)\n", status);
	return NULL;
    } else if (status == 301 || status == 302) {
        //find the line with the new location
         while ((status_line = next_line(buf, reply->reply_buffer_length - (buf - reply->reply_buffer))) != NULL)
         {
            if (has_location(buf)) {
                *status_line = '\0';
                return buf;
            }
            buf = status_line + 2;        
        }
        return 0;
    }

    /*
     * To be completed:
     *   The previous code only detects and parses the first line of the reply.
     *   But servers typically send additional header lines:
     *     Date: Mon, 05 Aug 2019 12:54:36 GMT<CR><LF>
     *     Content-type: text/css<CR><LF>
     *     Content-Length: 684<CR><LF>
     *     Last-Modified: Mon, 03 Jun 2019 22:46:31 GMT<CR><LF>
     *     <CR><LF>
     *
     *   Keep calling next_line until you read an empty line, and return only what remains (without the empty line).
     *   Hint: Take a look at how end of lines are tested in next_line function declaration, to get inspiration
     *
     *   Difficul challenge:
     *     If you feel like having a real challenge, go on and implement HTTP redirect support for your client.
     *
     */
    
    while ((status_line = next_line(buf, reply->reply_buffer_length - (buf - reply->reply_buffer))) != NULL)
    {
        if (empty_line(buf)) {
            buf = status_line + 2;
            break;
        }
        buf = status_line + 2;        
    }
    return buf;
}
