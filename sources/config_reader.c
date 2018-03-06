
/*
 * Odissey.
 *
 * Advanced PostgreSQL connection pooler.
*/

#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <ctype.h>
#include <assert.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <machinarium.h>

#include "sources/macro.h"
#include "sources/version.h"
#include "sources/atomic.h"
#include "sources/util.h"
#include "sources/error.h"
#include "sources/list.h"
#include "sources/pid.h"
#include "sources/id.h"
#include "sources/logger.h"
#include "sources/daemon.h"
#include "sources/config.h"
#include "sources/config_mgr.h"
#include "sources/parser.h"
#include "sources/config_reader.h"

enum
{
	OD_LYES,
	OD_LNO,
	OD_LINCLUDE,
	OD_LDAEMONIZE,
	OD_LLOG_TO_STDOUT,
	OD_LLOG_DEBUG,
	OD_LLOG_CONFIG,
	OD_LLOG_SESSION,
	OD_LLOG_QUERY,
	OD_LLOG_FILE,
	OD_LLOG_FORMAT,
	OD_LLOG_STATS,
	OD_LPID_FILE,
	OD_LLOG_SYSLOG,
	OD_LLOG_SYSLOG_IDENT,
	OD_LLOG_SYSLOG_FACILITY,
	OD_LSTATS_INTERVAL,
	OD_LLISTEN,
	OD_LHOST,
	OD_LPORT,
	OD_LBACKLOG,
	OD_LNODELAY,
	OD_LKEEPALIVE,
	OD_LREADAHEAD,
	OD_LWORKERS,
	OD_LRESOLVERS,
	OD_LPIPELINE,
	OD_LCACHE,
	OD_LCACHE_CHUNK,
	OD_LCACHE_COROUTINE,
	OD_LCLIENT_MAX,
	OD_LCLIENT_FWD_ERROR,
	OD_LTLS,
	OD_LTLS_CA_FILE,
	OD_LTLS_KEY_FILE,
	OD_LTLS_CERT_FILE,
	OD_LTLS_PROTOCOLS,
	OD_LSTORAGE,
	OD_LTYPE,
	OD_LDEFAULT,
	OD_LDATABASE,
	OD_LUSER,
	OD_LPASSWORD,
	OD_LPOOL,
	OD_LPOOL_SIZE,
	OD_LPOOL_TIMEOUT,
	OD_LPOOL_TTL,
	OD_LPOOL_CANCEL,
	OD_LPOOL_ROLLBACK,
	OD_LSTORAGE_DB,
	OD_LSTORAGE_USER,
	OD_LSTORAGE_PASSWORD,
	OD_LAUTHENTICATION,
	OD_LAUTH_QUERY,
	OD_LAUTH_QUERY_DB,
	OD_LAUTH_QUERY_USER
};

typedef struct
{
	od_parser_t  parser;
	od_config_t *config;
	od_error_t  *error;
	char        *config_file;
	char        *data;
	int          data_size;
	uint64_t     version;
} od_configreader_t;

static od_keyword_t od_config_keywords[] =
{
	/* main */
	od_keyword("yes",                 OD_LYES),
	od_keyword("no",                  OD_LNO),
	od_keyword("include",             OD_LINCLUDE),
	od_keyword("daemonize",           OD_LDAEMONIZE),
	od_keyword("pid_file",            OD_LPID_FILE),
	od_keyword("log_debug",           OD_LLOG_DEBUG),
	od_keyword("log_to_stdout",       OD_LLOG_TO_STDOUT),
	od_keyword("log_config",          OD_LLOG_CONFIG),
	od_keyword("log_session",         OD_LLOG_SESSION),
	od_keyword("log_query",           OD_LLOG_QUERY),
	od_keyword("log_file",            OD_LLOG_FILE),
	od_keyword("log_format",          OD_LLOG_FORMAT),
	od_keyword("log_stats",           OD_LLOG_STATS),
	od_keyword("log_syslog",          OD_LLOG_SYSLOG),
	od_keyword("log_syslog_ident",    OD_LLOG_SYSLOG_IDENT),
	od_keyword("log_syslog_facility", OD_LLOG_SYSLOG_FACILITY),
	od_keyword("stats_interval",      OD_LSTATS_INTERVAL),
	/* listen */
	od_keyword("listen",              OD_LLISTEN),
	od_keyword("host",                OD_LHOST),
	od_keyword("port",                OD_LPORT),
	od_keyword("backlog",             OD_LBACKLOG),
	od_keyword("nodelay",             OD_LNODELAY),
	od_keyword("keepalive",           OD_LKEEPALIVE),
	od_keyword("readahead",           OD_LREADAHEAD),
	od_keyword("workers",             OD_LWORKERS),
	od_keyword("resolvers",           OD_LRESOLVERS),
	od_keyword("pipeline",            OD_LPIPELINE),
	od_keyword("cache",               OD_LCACHE),
	od_keyword("cache_chunk",         OD_LCACHE_CHUNK),
	od_keyword("cache_coroutine",     OD_LCACHE_COROUTINE),
	od_keyword("client_max",          OD_LCLIENT_MAX),
	od_keyword("client_fwd_error",    OD_LCLIENT_FWD_ERROR),
	od_keyword("tls",                 OD_LTLS),
	od_keyword("tls_ca_file",         OD_LTLS_CA_FILE),
	od_keyword("tls_key_file",        OD_LTLS_KEY_FILE),
	od_keyword("tls_cert_file",       OD_LTLS_CERT_FILE),
	od_keyword("tls_protocols",       OD_LTLS_PROTOCOLS),
	/* storage */
	od_keyword("storage",             OD_LSTORAGE),
	od_keyword("type",                OD_LTYPE),
	od_keyword("default",             OD_LDEFAULT),
	/* database */
	od_keyword("database",            OD_LDATABASE),
	od_keyword("user",                OD_LUSER),
	od_keyword("password",            OD_LPASSWORD),
	od_keyword("pool",                OD_LPOOL),
	od_keyword("pool_size",           OD_LPOOL_SIZE),
	od_keyword("pool_timeout",        OD_LPOOL_TIMEOUT),
	od_keyword("pool_ttl",            OD_LPOOL_TTL),
	od_keyword("pool_cancel",         OD_LPOOL_CANCEL),
	od_keyword("pool_rollback",       OD_LPOOL_ROLLBACK),
	od_keyword("storage_db",          OD_LSTORAGE_DB),
	od_keyword("storage_user",        OD_LSTORAGE_USER),
	od_keyword("storage_password",    OD_LSTORAGE_PASSWORD),
	od_keyword("authentication",      OD_LAUTHENTICATION),
	od_keyword("auth_query",          OD_LAUTH_QUERY),
	od_keyword("auth_query_db",       OD_LAUTH_QUERY_DB),
	od_keyword("auth_query_user",     OD_LAUTH_QUERY_USER),
	{ 0, 0, 0 }
};

static int
od_configreader_open(od_configreader_t *reader, char *config_file)
{
	reader->config_file = config_file;
	/* read file */
	struct stat st;
	int rc = lstat(config_file, &st);
	if (rc == -1)
		goto error;
	char *config_buf = malloc(st.st_size);
	if (config_buf == NULL)
		goto error;
	FILE *file = fopen(config_file, "r");
	if (file == NULL) {
		free(config_buf);
		goto error;
	}
	rc = fread(config_buf, st.st_size, 1, file);
	fclose(file);
	if (rc != 1) {
		free(config_buf);
		goto error;
	}
	reader->data = config_buf;
	reader->data_size = st.st_size;
	od_parser_init(&reader->parser, reader->data, reader->data_size);
	return 0;
error:
	od_errorf(reader->error, "failed to open config file '%s'",
	          config_file);
	return -1;
}

static void
od_configreader_close(od_configreader_t *reader)
{
	free(reader->data);
}

static void
od_configreader_error(od_configreader_t *reader, od_token_t *token, char *fmt, ...)
{
	char msg[256];
	va_list args;
	va_start(args, fmt);
	od_vsnprintf(msg, sizeof(msg), fmt, args);
	va_end(args);
	int line = reader->parser.line;
	if (token)
		line = token->line;
	od_errorf(reader->error, "%s:%d %s", reader->config_file,
	          line, msg);
}

static bool
od_configreader_is(od_configreader_t *reader, int id)
{
	od_token_t token;
	int rc;
	rc = od_parser_next(&reader->parser, &token);
	od_parser_push(&reader->parser, &token);
	if (rc != id)
		return false;
	return true;
}

static bool
od_configreader_keyword(od_configreader_t *reader, od_keyword_t *keyword)
{
	od_token_t token;
	int rc;
	rc = od_parser_next(&reader->parser, &token);
	if (rc != OD_PARSER_KEYWORD)
		goto error;
	od_keyword_t *match;
	match = od_keyword_match(od_config_keywords, &token);
	if (keyword == NULL)
		goto error;
	if (keyword != match)
		goto error;
	return true;
error:
	od_parser_push(&reader->parser, &token);
	od_configreader_error(reader, &token, "expected '%s'", keyword->name);
	return false;
}

static bool
od_configreader_symbol(od_configreader_t *reader, char symbol)
{
	od_token_t token;
	int rc;
	rc = od_parser_next(&reader->parser, &token);
	if (rc != OD_PARSER_SYMBOL)
		goto error;
	if (token.value.num != (uint64_t)symbol)
		goto error;
	return true;
error:
	od_parser_push(&reader->parser, &token);
	od_configreader_error(reader, &token, "expected '%c'", symbol);
	return false;
}

static bool
od_configreader_string(od_configreader_t *reader, char **value)
{
	od_token_t token;
	int rc;
	rc = od_parser_next(&reader->parser, &token);
	if (rc != OD_PARSER_STRING) {
		od_parser_push(&reader->parser, &token);
		od_configreader_error(reader, &token, "expected 'string'");
		return false;
	}
	char *copy = malloc(token.value.string.size + 1);
	if (copy == NULL) {
		od_parser_push(&reader->parser, &token);
		od_configreader_error(reader, &token, "memory allocation error");
		return false;
	}
	memcpy(copy, token.value.string.pointer, token.value.string.size);
	copy[token.value.string.size] = 0;
	if (*value)
		free(*value);
	*value = copy;
	return true;
}

static bool
od_configreader_number(od_configreader_t *reader, int *number)
{
	od_token_t token;
	int rc;
	rc = od_parser_next(&reader->parser, &token);
	if (rc != OD_PARSER_NUM) {
		od_parser_push(&reader->parser, &token);
		od_configreader_error(reader, &token, "expected 'number'");
		return false;
	}
	/* uint64 to int conversion */
	*number = token.value.num;
	return true;
}

static bool
od_configreader_yes_no(od_configreader_t *reader, int *value)
{
	od_token_t token;
	int rc;
	rc = od_parser_next(&reader->parser, &token);
	if (rc != OD_PARSER_KEYWORD)
		goto error;
	od_keyword_t *keyword;
	keyword = od_keyword_match(od_config_keywords, &token);
	if (keyword == NULL)
		goto error;
	switch (keyword->id) {
	case OD_LYES:
		*value = 1;
		break;
	case OD_LNO:
		*value = 0;
		break;
	default:
		goto error;
	}
	return true;
error:
	od_parser_push(&reader->parser, &token);
	od_configreader_error(reader, &token, "expected 'yes/no'");
	return false;
}

static int
od_configreader_listen(od_configreader_t *reader)
{
	od_config_t *config = reader->config;

	od_configlisten_t *listen;
	listen = od_configlisten_add(config);
	if (listen == NULL) {
		return -1;
	}

	/* { */
	if (! od_configreader_symbol(reader, '{'))
		return -1;

	for (;;)
	{
		od_token_t token;
		int rc;
		rc = od_parser_next(&reader->parser, &token);
		switch (rc) {
		case OD_PARSER_KEYWORD:
			break;
		case OD_PARSER_EOF:
			od_configreader_error(reader, &token, "unexpected end of config file");
			return -1;
		case OD_PARSER_SYMBOL:
			/* } */
			if (token.value.num == '}')
				return 0;
			/* fall through */
		default:
			od_configreader_error(reader, &token, "incorrect or unexpected parameter");
			return -1;
		}
		od_keyword_t *keyword;
		keyword = od_keyword_match(od_config_keywords, &token);
		if (keyword == NULL) {
			od_configreader_error(reader, &token, "unknown parameter");
			return -1;
		}
		switch (keyword->id) {
		/* host */
		case OD_LHOST:
			if (! od_configreader_string(reader, &listen->host))
				return -1;
			continue;
		/* port */
		case OD_LPORT:
			if (! od_configreader_number(reader, &listen->port))
				return -1;
			continue;
		/* backlog */
		case OD_LBACKLOG:
			if (! od_configreader_number(reader, &listen->backlog))
				return -1;
			continue;
		/* tls */
		case OD_LTLS:
			if (! od_configreader_string(reader, &listen->tls))
				return -1;
			continue;
		/* tls_ca_file */
		case OD_LTLS_CA_FILE:
			if (! od_configreader_string(reader, &listen->tls_ca_file))
				return -1;
			continue;
		/* tls_key_file */
		case OD_LTLS_KEY_FILE:
			if (! od_configreader_string(reader, &listen->tls_key_file))
				return -1;
			continue;
		/* tls_cert_file */
		case OD_LTLS_CERT_FILE:
			if (! od_configreader_string(reader, &listen->tls_cert_file))
				return -1;
			continue;
		/* tls_protocols */
		case OD_LTLS_PROTOCOLS:
			if (! od_configreader_string(reader, &listen->tls_protocols))
				return -1;
			continue;
		default:
			od_configreader_error(reader, &token, "unexpected parameter");
			return -1;
		}
	}
	/* unreach */
	return -1;
}

static int
od_configreader_storage(od_configreader_t *reader)
{
	od_configstorage_t *storage;
	storage = od_configstorage_add(reader->config);
	if (storage == NULL)
		return -1;
	/* name */
	if (! od_configreader_string(reader, &storage->name))
		return -1;
	/* { */
	if (! od_configreader_symbol(reader, '{'))
		return -1;

	for (;;)
	{
		od_token_t token;
		int rc;
		rc = od_parser_next(&reader->parser, &token);
		switch (rc) {
		case OD_PARSER_KEYWORD:
			break;
		case OD_PARSER_EOF:
			od_configreader_error(reader, &token, "unexpected end of config file");
			return -1;
		case OD_PARSER_SYMBOL:
			/* } */
			if (token.value.num == '}')
				return 0;
			/* fall through */
		default:
			od_configreader_error(reader, &token, "incorrect or unexpected parameter");
			return -1;
		}
		od_keyword_t *keyword;
		keyword = od_keyword_match(od_config_keywords, &token);
		if (keyword == NULL) {
			od_configreader_error(reader, &token, "unknown parameter");
			return -1;
		}
		switch (keyword->id) {
		/* type */
		case OD_LTYPE:
			if (! od_configreader_string(reader, &storage->type))
				return -1;
			continue;
		/* host */
		case OD_LHOST:
			if (! od_configreader_string(reader, &storage->host))
				return -1;
			continue;
		/* port */
		case OD_LPORT:
			if (! od_configreader_number(reader, &storage->port))
				return -1;
			continue;
		/* tls */
		case OD_LTLS:
			if (! od_configreader_string(reader, &storage->tls))
				return -1;
			continue;
		/* tls_ca_file */
		case OD_LTLS_CA_FILE:
			if (! od_configreader_string(reader, &storage->tls_ca_file))
				return -1;
			continue;
		/* tls_key_file */
		case OD_LTLS_KEY_FILE:
			if (! od_configreader_string(reader, &storage->tls_key_file))
				return -1;
			continue;
		/* tls_cert_file */
		case OD_LTLS_CERT_FILE:
			if (! od_configreader_string(reader, &storage->tls_cert_file))
				return -1;
			continue;
		/* tls_protocols */
		case OD_LTLS_PROTOCOLS:
			if (! od_configreader_string(reader, &storage->tls_protocols))
				return -1;
			continue;
		default:
			od_configreader_error(reader, &token, "unexpected parameter");
			return -1;
		}
	}
	/* unreach */
	return -1;
}

static int
od_configreader_route(od_configreader_t *reader, char *db_name, int db_name_len,
                      int db_is_default)
{
	char *user_name = NULL;
	int   user_name_len = 0;
	int   user_is_default = 0;

	/* user name or default */
	if (od_configreader_is(reader, OD_PARSER_STRING)) {
		if (! od_configreader_string(reader, &user_name))
			return -1;
	} else {
		if (! od_configreader_keyword(reader, &od_config_keywords[OD_LDEFAULT]))
			return -1;
		user_is_default = 1;
		user_name = strdup("default");
		if (user_name == NULL)
			return -1;
	}
	user_name_len = strlen(user_name);

	/* ensure route does not exists and add new route */
	od_configroute_t *route;
	route = od_configroute_match(reader->config, db_name, user_name);
	if (route) {
		od_errorf(reader->error, "route '%s.%s': is redefined",
		          db_name, user_name);
		free(user_name);
		return -1;
	}
	route = od_configroute_add(reader->config, reader->version);
	if (route == NULL) {
		free(user_name);
		return -1;
	}
	route->user_is_default = user_is_default;
	route->user_name_len = user_name_len;
	route->user_name = strdup(user_name);
	free(user_name);
	if (route->user_name == NULL)
		return -1;
	route->db_is_default = db_is_default;
	route->db_name_len = db_name_len;
	route->db_name = strdup(db_name);
	if (route->db_name == NULL)
		return -1;

	/* { */
	if (! od_configreader_symbol(reader, '{'))
		return -1;

	for (;;)
	{
		od_token_t token;
		int rc;
		rc = od_parser_next(&reader->parser, &token);
		switch (rc) {
		case OD_PARSER_KEYWORD:
			break;
		case OD_PARSER_EOF:
			od_configreader_error(reader, &token, "unexpected end of config file");
			return -1;
		case OD_PARSER_SYMBOL:
			/* } */
			if (token.value.num == '}')
				return 0;
			/* fall through */
		default:
			od_configreader_error(reader, &token, "incorrect or unexpected parameter");
			return -1;
		}
		od_keyword_t *keyword;
		keyword = od_keyword_match(od_config_keywords, &token);
		if (keyword == NULL) {
			od_configreader_error(reader, &token, "unknown parameter");
			return -1;
		}
		switch (keyword->id) {
		/* authentication */
		case OD_LAUTHENTICATION:
			if (! od_configreader_string(reader, &route->auth))
				return -1;
			break;
		/* auth_query */
		case OD_LAUTH_QUERY:
			if (! od_configreader_string(reader, &route->auth_query))
				return -1;
			break;
		/* auth_query_db */
		case OD_LAUTH_QUERY_DB:
			if (! od_configreader_string(reader, &route->auth_query_db))
				return -1;
			break;
		/* auth_query_user */
		case OD_LAUTH_QUERY_USER:
			if (! od_configreader_string(reader, &route->auth_query_user))
				return -1;
			break;
		/* password */
		case OD_LPASSWORD:
			if (! od_configreader_string(reader, &route->password))
				return -1;
			route->password_len = strlen(route->password);
			continue;
		/* storage */
		case OD_LSTORAGE:
			if (! od_configreader_string(reader, &route->storage_name))
				return -1;
			continue;
		/* client_max */
		case OD_LCLIENT_MAX:
			if (! od_configreader_number(reader, &route->client_max))
				return -1;
			route->client_max_set = 1;
			continue;
		/* client_fwd_error */
		case OD_LCLIENT_FWD_ERROR:
			if (! od_configreader_yes_no(reader, &route->client_fwd_error))
				return -1;
			continue;
		/* pool */
		case OD_LPOOL:
			if (! od_configreader_string(reader, &route->pool_sz))
				return -1;
			continue;
		/* pool_size */
		case OD_LPOOL_SIZE:
			if (! od_configreader_number(reader, &route->pool_size))
				return -1;
			continue;
		/* pool_timeout */
		case OD_LPOOL_TIMEOUT:
			if (! od_configreader_number(reader, &route->pool_timeout))
				return -1;
			continue;
		/* pool_ttl */
		case OD_LPOOL_TTL:
			if (! od_configreader_number(reader, &route->pool_ttl))
				return -1;
			continue;
		/* storage_database */
		case OD_LSTORAGE_DB:
			if (! od_configreader_string(reader, &route->storage_db))
				return -1;
			continue;
		/* storage_user */
		case OD_LSTORAGE_USER:
			if (! od_configreader_string(reader, &route->storage_user))
				return -1;
			route->storage_user_len = strlen(route->storage_user);
			continue;
		/* storage_password */
		case OD_LSTORAGE_PASSWORD:
			if (! od_configreader_string(reader, &route->storage_password))
				return -1;
			route->storage_password_len = strlen(route->storage_password);
			continue;
		/* pool_cancel */
		case OD_LPOOL_CANCEL:
			if (! od_configreader_yes_no(reader, &route->pool_cancel))
				return -1;
			continue;
		/* pool_rollback */
		case OD_LPOOL_ROLLBACK:
			if (! od_configreader_yes_no(reader, &route->pool_rollback))
				return -1;
			continue;
		/* log_debug */
		case OD_LLOG_DEBUG:
			if (! od_configreader_yes_no(reader, &route->log_debug))
				return -1;
			continue;
		default:
			od_configreader_error(reader, &token, "unexpected parameter");
			return -1;
		}
	}

	/* unreach */
	return -1;
}

static int
od_configreader_database(od_configreader_t *reader)
{
	char *db_name = NULL;
	int   db_name_len = 0;
	int   db_is_default = 0;

	/* name or default */
	if (od_configreader_is(reader, OD_PARSER_STRING)) {
		if (! od_configreader_string(reader, &db_name))
			return -1;
	} else {
		if (! od_configreader_keyword(reader, &od_config_keywords[OD_LDEFAULT]))
			return -1;
		db_is_default = 1;
		db_name = strdup("default");
		if (db_name == NULL)
			return -1;
	}
	db_name_len = strlen(db_name);

	/* { */
	if (! od_configreader_symbol(reader, '{'))
		goto error;

	for (;;)
	{
		od_token_t token;
		int rc;
		rc = od_parser_next(&reader->parser, &token);
		switch (rc) {
		case OD_PARSER_KEYWORD:
			break;
		case OD_PARSER_EOF:
			od_configreader_error(reader, &token, "unexpected end of config file");
			goto error;
		case OD_PARSER_SYMBOL:
			/* } */
			if (token.value.num == '}') {
				/* make sure that db.default is defined */
				od_configroute_t *route;
				route = od_configroute_match(reader->config, db_name, "default");
				if (! route) {
					od_errorf(reader->error, "route '%s.default': is not defined",
					          db_name);
					free(db_name);
					return -1;
				}
				free(db_name);
				return 0;
			}
			/* fall through */
		default:
			od_configreader_error(reader, &token, "incorrect or unexpected parameter");
			goto error;
		}
		od_keyword_t *keyword;
		keyword = od_keyword_match(od_config_keywords, &token);
		if (keyword == NULL) {
			od_configreader_error(reader, &token, "unknown parameter");
			goto error;
		}
		switch (keyword->id) {
		/* user */
		case OD_LUSER:
			rc = od_configreader_route(reader, db_name, db_name_len, db_is_default);
			if (rc == -1)
				goto error;
			continue;
		default:
			od_configreader_error(reader, &token, "unexpected parameter");
			goto error;
		}
	}
	/* unreach */
	return -1;
error:
	free(db_name);
	return -1;
}

static int
od_configreader_parse(od_configreader_t *reader)
{
	od_config_t *config = reader->config;
	for (;;)
	{
		od_token_t token;
		int rc;
		rc = od_parser_next(&reader->parser, &token);
		switch (rc) {
		case OD_PARSER_EOF:
			return 0;
		case OD_PARSER_KEYWORD:
			break;
		default:
			od_configreader_error(reader, &token, "incorrect or unexpected parameter");
			return -1;
		}
		od_keyword_t *keyword;
		keyword = od_keyword_match(od_config_keywords, &token);
		if (keyword == NULL) {
			od_configreader_error(reader, &token, "unknown parameter");
			return -1;
		}
		switch (keyword->id) {
		/* include */
		case OD_LINCLUDE:
		{
			char *config_file;
			if (! od_configreader_string(reader, &config_file))
				return -1;
			rc = od_configreader_import(reader->config, reader->error, config_file,
			                            reader->version);
			free(config_file);
			if (rc == -1)
				return -1;
			continue;
		}
		/* daemonize */
		case OD_LDAEMONIZE:
			if (! od_configreader_yes_no(reader, &config->daemonize))
				return -1;
			continue;
		/* pid_file */
		case OD_LPID_FILE:
			if (! od_configreader_string(reader, &config->pid_file))
				return -1;
			continue;
		/* log_debug */
		case OD_LLOG_DEBUG:
			if (! od_configreader_yes_no(reader, &config->log_debug))
				return -1;
			continue;
		/* log_stdout */
		case OD_LLOG_TO_STDOUT:
			if (! od_configreader_yes_no(reader, &config->log_to_stdout))
				return -1;
			continue;
		/* log_config */
		case OD_LLOG_CONFIG:
			if (! od_configreader_yes_no(reader, &config->log_config))
				return -1;
			continue;
		/* log_session */
		case OD_LLOG_SESSION:
			if (! od_configreader_yes_no(reader, &config->log_session))
				return -1;
			continue;
		/* log_query */
		case OD_LLOG_QUERY:
			if (! od_configreader_yes_no(reader, &config->log_query))
				return -1;
			continue;
		/* log_stats */
		case OD_LLOG_STATS:
			if (! od_configreader_yes_no(reader, &config->log_stats))
				return -1;
			continue;
		/* log_format */
		case OD_LLOG_FORMAT:
			if (! od_configreader_string(reader, &config->log_format))
				return -1;
			continue;
		/* log_file */
		case OD_LLOG_FILE:
			if (! od_configreader_string(reader, &config->log_file))
				return -1;
			continue;
		/* log_syslog */
		case OD_LLOG_SYSLOG:
			if (! od_configreader_yes_no(reader, &config->log_syslog))
				return -1;
			continue;
		/* log_syslog_ident */
		case OD_LLOG_SYSLOG_IDENT:
			if (! od_configreader_string(reader, &config->log_syslog_ident))
				return -1;
			continue;
		/* log_syslog_facility */
		case OD_LLOG_SYSLOG_FACILITY:
			if (! od_configreader_string(reader, &config->log_syslog_facility))
				return -1;
			continue;
		/* stats_interval */
		case OD_LSTATS_INTERVAL:
			if (! od_configreader_number(reader, &config->stats_interval))
				return -1;
			continue;
		/* client_max */
		case OD_LCLIENT_MAX:
			if (! od_configreader_number(reader, &config->client_max))
				return -1;
			config->client_max_set = 1;
			continue;
		/* readahead */
		case OD_LREADAHEAD:
			if (! od_configreader_number(reader, &config->readahead))
				return -1;
			continue;
		/* nodelay */
		case OD_LNODELAY:
			if (! od_configreader_yes_no(reader, &config->nodelay))
				return -1;
			continue;
		/* keepalive */
		case OD_LKEEPALIVE:
			if (! od_configreader_number(reader, &config->keepalive))
				return -1;
			continue;
		/* workers */
		case OD_LWORKERS:
			if (! od_configreader_number(reader, &config->workers))
				return -1;
			continue;
		/* resolvers */
		case OD_LRESOLVERS:
			if (! od_configreader_number(reader, &config->resolvers))
				return -1;
			continue;
		/* pipeline */
		case OD_LPIPELINE:
			if (! od_configreader_number(reader, &config->pipeline))
				return -1;
			continue;
		/* cache */
		case OD_LCACHE:
			if (! od_configreader_number(reader, &config->cache))
				return -1;
			continue;
		/* cache_chunk */
		case OD_LCACHE_CHUNK:
			if (! od_configreader_number(reader, &config->cache_chunk))
				return -1;
			continue;
		/* cache_coroutine */
		case OD_LCACHE_COROUTINE:
			if (! od_configreader_number(reader, &config->cache_coroutine))
				return -1;
			continue;
		/* listen */
		case OD_LLISTEN:
			rc = od_configreader_listen(reader);
			if (rc == -1)
				return -1;
			continue;
		/* storage */
		case OD_LSTORAGE:
			rc = od_configreader_storage(reader);
			if (rc == -1)
				return -1;
			continue;
		/* database */
		case OD_LDATABASE:
			rc = od_configreader_database(reader);
			if (rc == -1)
				return -1;
			continue;
		default:
			od_configreader_error(reader, &token, "unexpected parameter");
			return -1;
		}
	}
	/* unreach */
	return -1;
}

int od_configreader_import(od_config_t *config, od_error_t *error,
                           char *config_file,
                           uint64_t version)
{
	od_configreader_t reader;
	memset(&reader, 0, sizeof(reader));
	reader.error   = error;
	reader.config  = config;
	reader.version = version;
	int rc;
	rc = od_configreader_open(&reader, config_file);
	if (rc == -1)
		return -1;
	rc = od_configreader_parse(&reader);
	od_configreader_close(&reader);
	return rc;
}
