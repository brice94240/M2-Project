#include "stdafx.h"

#ifndef __WIN32__
#include <unistd.h>
#include <stdint.h>
#endif
#include <stdio.h>
#include <string.h>
#ifdef __FreeBSD__
#include <md5.h>
#else
#include "../../libthecore/include/xmd5.h"
#endif

#include "auth_brazil.h"

static const char* FN_md5(const char *src)
{
    static char s_buffer[512];

    memset(s_buffer, 0x00, sizeof(s_buffer));

    uint8_t digest[16] = {0};
    MD5_CTX md5;
    MD5Init(&md5);
    MD5Update(&md5, (const uint8_t*) src, strlen(src));
    MD5Final(digest, &md5);

    int32_t offset = 0;
    for (int32_t i=0; i<16; ++i) {
	offset += sprintf(s_buffer + offset, "%02x", digest[i]);
					        }
    return s_buffer;
}


static int32_t FN_make_request(const char *login, const char *password, char *dst, int32_t dst_size)
{
    int32_t len = snprintf(dst, dst_size,
	    "GET /metin2/?ID=%s&PW=%s HTTP/1.1\r\n"
	    "Host: auth.ongame.com.br\r\n"
	    "Connection: Close\r\n\r\n",
	    login, FN_md5(password));

    return len;
}


static int32_t FN_parse_reply(char *reply)
{
    char buffer[2048];
    strlcpy(buffer, reply, sizeof(buffer));

    const char *delim = "\r\n";
    char *last = 0;
    char *v = strtok_r(buffer, delim, &last);
    char *result = 0;

    while (v)
    {
	result = v;
	v = strtok_r(NULL, delim, &last);
    }

    if (result)
    {
	if (0 == strcasecmp("true", result))
	    return AUTH_BRAZIL_SUCC;
	else if (0 == strcasecmp("false", result))
	    return AUTH_BRAZIL_WRONGPWD;
	else if (0 == strcasecmp("unknown", result))
	    return AUTH_BRAZIL_NOID;
	else if (0 == strcasecmp("flash", result))
	    return AUTH_BRAZIL_FLASHUSER;
    }

    return AUTH_BRAZIL_SERVER_ERR;
}


extern void socket_timeout(socket_t s, int32_t sec, int32_t usec);

int32_t auth_brazil(const char *login, const char *pwd)
{
	const char *host = "127.0.0.1";
    int32_t		port	= 80;

    socket_t fd = socket_connect(host, port);
    if (fd < 0)
    {
	sys_err("[AUTH_BRAZIL] : could not connect to gsp server(%s)", host);
	return AUTH_BRAZIL_SERVER_ERR;
    }

    socket_block(fd);
    socket_timeout(fd, 3, 0);

    
    {
	char request[512];
	int32_t len = FN_make_request(login, pwd, request, sizeof(request));

#ifndef __WIN32__
	if (write(fd, request, len) < 0)
#else
	if (_write(fd, request, len) < 0)
#endif
	{
	    sys_err("[AUTH_BRAZIL] : could not send auth-request (%s)", login);
	    close(fd);
	    return AUTH_BRAZIL_SERVER_ERR;
	}
    }

    {
	char reply[1024] = {0};
	int32_t len = read(fd, reply, sizeof(reply));
	close(fd);

	if (len <= 0)
	{
	    sys_err("[AUTH_BRAZIL] : could not recv auth-reply (%s)", login);
	    return AUTH_BRAZIL_SERVER_ERR;
	}

	auth_brazil_inc_query_count();

	return FN_parse_reply(reply);
    }
}


static int32_t s_query_count = 0;

int32_t auth_brazil_inc_query_count()
{
    return ++s_query_count;
}

void auth_brazil_log()
{
    FILE *fp = 0;

    {
	fp = fopen("AUTH_COUNT.log", "a");

	if (0 == fp)
	    return;

	struct stat sb;
	fstat(fileno(fp), &sb);
	if (sb.st_size > 1024 * 1024)
	{
	    fclose(fp);
	    rename("AUTH_COUNT.log", "AUTH_COUNT.log.old");

	    fp = fopen("AUTH_COUNT.log", "a");
	}
    }

    {
	fprintf(fp, "%d\n", s_query_count);
	fclose(fp);
    }

    s_query_count = 0;
}
