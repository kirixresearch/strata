/*
 * Copyright (c) 2004-2009 Sergey Lyubka
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * $Id: mongoose.h 361 2009-05-19 12:49:30Z valenok $
 */

#ifndef MONGOOSE_HEADER_INCLUDED
#define	MONGOOSE_HEADER_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct mg_context;	/* Handle for the HTTP service itself	*/
struct mg_connection;	/* Handle for the individual connection	*/

/*
 * This structure contains full information about the HTTP request.
 * It is passed to the user-specified callback function as a parameter.
 */
struct mg_request_info {
	char	*request_method;	/* "GET", "POST", etc	*/
	char	*uri;			/* Normalized URI	*/
	char	*query_string;		/* \0 - terminated	*/
	char	*post_data;		/* POST data buffer	*/
	char	*remote_user;		/* Authenticated user	*/
	long	remote_ip;		/* Client's IP address	*/
	int	remote_port;		/* Client's port	*/
	int	post_data_len;		/* POST buffer length	*/
	int	http_version_major;
	int	http_version_minor;
	int	status_code;		/* HTTP status code	*/
	int	num_headers;		/* Number of headers	*/
	struct mg_header {
		char	*name;		/* HTTP header name	*/
		char	*value;		/* HTTP header value	*/
	} http_headers[64];		/* Maximum 64 headers	*/
};

/*
 * Functions dealing with initialization, starting and stopping Mongoose
 *
 * mg_start		Start serving thread. Return server context.
 * mg_stop		Stop server thread, and release the context.
 * mg_set_option	Set an option for the running context, return 1
 *			on success, 0 on error, -1 if option name is invalid.
 * mg_get_option	Get an option for the running context.
 * mg_get_option_list	Get a list of all known options, subject to mg_free()
 * mg_bind_to_uri	Associate user function with paticular URI.
 *			'*' in regex matches zero or more characters.
 * mg_bind_to_error_code	Associate user function with HTTP error code.
 *			Passing 0 as error code binds function to all codes.
 *			Error code is passed as status_code in request info.
 * mg_protect_uri	Bind authorization function to the URI regexp.
 * mg_set_log_callback	Set a function that will receive server log messages.
 * mg_modify_passwords_file	Add, edit or delete the entry in the
 *			passwords file. If password is not NULL, entry is added
 *			(or modified if already exists). If password is NULL,
 *			entry is deleted. Return 1 on success, 0 on error.
 */

struct mg_context *mg_start(void);
void mg_stop(struct mg_context *);
const char *mg_get_option(struct mg_context *, const char *);
int mg_set_option(struct mg_context *, const char *, const char *);
int mg_modify_passwords_file(struct mg_context *ctx, const char *file_name,
		const char *user_name, const char *password);

/*
 * User-defined callback function for URI handling, error handling,
 * or logging server messages.
 */
typedef void (*mg_callback_t)(struct mg_connection *,
		const struct mg_request_info *info, void *user_data);

void mg_bind_to_uri(struct mg_context *ctx, const char *uri_regex,
		mg_callback_t func, void *user_data);
void mg_bind_to_error_code(struct mg_context *ctx, int error_code,
		mg_callback_t func, void *user_data);
void mg_protect_uri(struct mg_context *ctx, const char *uri_regex,
		mg_callback_t func, void *user_data);
void mg_set_log_callback(struct mg_context *ctx, mg_callback_t func);

/*
 * Needed only if SSL certificate asks for a password.
 * Instead of prompting for a password, specified function will be called.
 */
typedef int (*mg_spcb_t)(char *buf, int num, int w, void *key);
void mg_set_ssl_password_callback(struct mg_context *ctx, mg_spcb_t func);

/*
 * Functions that can be used within the user URI callback
 *
 * mg_write	Send data to the remote end.
 * mg_printf	Send data, using printf() semantics.
 * mg_get_header Helper function to get HTTP header value
 * mg_get_var	Helper function to get form variable value.
 *		NOTE: Returned value must be mg_free()-ed by the caller.
 * mg_free	Free up memory returned by mg_get_var()
 * mg_authorize	Set authorization flag for the connection. Should be used
 *		only within callbacks registered with mg_protect_uri()
 */
int mg_write(struct mg_connection *, const void *buf, int len);
int mg_printf(struct mg_connection *, const char *fmt, ...);
const char *mg_get_header(const struct mg_connection *, const char *hdr_name);
void mg_authorize(struct mg_connection *);
char *mg_get_var(const struct mg_connection *, const char *var_name);
void mg_free(char *var);

/*
 * General helper functions
 * mg_version	Return current version.
 * mg_md5	Helper function. buf must be 33 bytes in size. Expects
 *		a NULL terminated list of asciz strings.
 *		Fills buf with stringified \0 terminated MD5 hash.
 */
const char *mg_version(void);
void mg_show_usage_string(FILE *fp);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MONGOOSE_HEADER_INCLUDED */
