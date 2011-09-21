/*
  +---------------------------------------------------------------------+
  | PHP Version 5														|
  +---------------------------------------------------------------------+
  | Copyright (c) 1997-2007 The PHP Group								|
  +---------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,		|
  | that is bundled with this package in the file LICENSE, and is		|
  | available through the world-wide-web at the following url:			|
  | http://www.php.net/license/3_01.txt									|
  | If you did not receive a copy of the PHP license and are unable to	|
  | obtain it through the world-wide-web, please send a note to			|
  | license@php.net so we can mail you a copy immediately.				|
  |																		|
  | This source uses the librabbitmq under the MPL. For the MPL, please |
  | see LICENSE-MPL-RabbitMQ											|
  +---------------------------------------------------------------------+
  | Author: Alexandre Kalendarev akalend@mail.ru Copyright (c) 2009-2010|
  | Maintainer: Pieter de Zwart pdezwart@php.net						|
  | Contributers:														|
  | - Andrey Hristov													|
  | - Brad Rodriguez brodriguez@php.net									|
  +---------------------------------------------------------------------+
*/

/* $Id: amqp_connection.c 312161 2011-06-14 18:47:22Z jtansavatdi $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "zend_exceptions.h"

#include <stdint.h>
#include <signal.h>
#include <amqp.h>
#include <amqp_framing.h>

#include <unistd.h>

#include "php_amqp.h"

/**
 * 	php_amqp_connect
 *	handles connecting to amqp
 *	called by connect() and reconnect()
 */
void php_amqp_connect(amqp_connection_object *amqp_connection TSRMLS_DC)
{
	char str[256];
	char ** pstr = (char **) &str;
	void * old_handler;

	/* create the connection */
	amqp_connection->conn = amqp_new_connection();

	amqp_connection->fd = amqp_open_socket(amqp_connection->host, amqp_connection->port);

	if (amqp_connection->fd < 1) {
		/* Start ignoring SIGPIPE */
		old_handler = signal(SIGPIPE, SIG_IGN);

		amqp_destroy_connection(amqp_connection->conn);

		/* End ignoring of SIGPIPEs */
		signal(SIGPIPE, old_handler);

		zend_throw_exception(amqp_connection_exception_class_entry, "Socket error: could not connect to host.", 0 TSRMLS_CC);
		return;
	}
	amqp_connection->is_connected = '\1';

	amqp_set_sockfd(amqp_connection->conn, amqp_connection->fd);

	amqp_rpc_reply_t x = amqp_login(amqp_connection->conn, amqp_connection->vhost, 0, FRAME_MAX, AMQP_HEARTBEAT, AMQP_SASL_METHOD_PLAIN, amqp_connection->login, amqp_connection->password);

	if (x.reply_type != AMQP_RESPONSE_NORMAL) {
		amqp_error(x, pstr);
		zend_throw_exception(amqp_connection_exception_class_entry, *pstr, 0 TSRMLS_CC);
		return;
	}

	amqp_channel_open(amqp_connection->conn, AMQP_CHANNEL);

	x = amqp_get_rpc_reply(amqp_connection->conn);
	if (x.reply_type != AMQP_RESPONSE_NORMAL) {
		amqp_error(x, pstr);
		zend_throw_exception(amqp_connection_exception_class_entry, *pstr, 0 TSRMLS_CC);
		return;
	}

	amqp_connection->is_channel_connected = '\1';
}

/* 	php_amqp_disconnect
	handles disconnecting from amqp
	called by disconnect(), reconnect(), and d_tor
 */
void php_amqp_disconnect(amqp_connection_object *amqp_connection)
{
	void * old_handler;
	/*
	If we are trying to close the connection and the connection already closed, it will throw
	SIGPIPE, which is fine, so ignore all SIGPIPES
	*/

	/* Start ignoring SIGPIPE */
	old_handler = signal(SIGPIPE, SIG_IGN);

	if (amqp_connection->is_channel_connected == '\1') {
		amqp_channel_close(amqp_connection->conn, AMQP_CHANNEL, AMQP_REPLY_SUCCESS);
	}
	amqp_connection->is_channel_connected = '\0';

	if (amqp_connection->conn && amqp_connection->is_connected == '\1') {
		amqp_destroy_connection(amqp_connection->conn);
	}
	amqp_connection->is_connected = '\0';
	if (amqp_connection->fd) {
		close(amqp_connection->fd);
	}

	/* End ignoring of SIGPIPEs */
	signal(SIGPIPE, old_handler);
	
	return;

}

void amqp_dtor(void *object TSRMLS_DC)
{
	amqp_connection_object *ob = (amqp_connection_object*)object;

	php_amqp_disconnect(ob);

	zend_object_std_dtor(&ob->zo TSRMLS_CC);

	efree(object);

}

zend_object_value amqp_ctor(zend_class_entry *ce TSRMLS_DC)
{
	zend_object_value new_value;
	amqp_connection_object* obj = (amqp_connection_object*)emalloc(sizeof(amqp_connection_object));

	memset(obj, 0, sizeof(amqp_connection_object));

	zend_object_std_init(&obj->zo, ce TSRMLS_CC);

	new_value.handle = zend_objects_store_put(obj, (zend_objects_store_dtor_t)zend_objects_destroy_object,
		(zend_objects_free_object_storage_t)amqp_dtor, NULL TSRMLS_CC);
	new_value.handlers = zend_get_std_object_handlers();

	return new_value;
}


/* {{{ proto AMQPConnection::__construct([array optional])
 * The array can contain 'host', 'port', 'login', 'password', 'vhost' indexes
 */
PHP_METHOD(amqp_connection_class, __construct)
{
	zval *id;
	amqp_connection_object *amqp_connection;

	zval* iniArr = NULL;
	zval** zdata;

	/* Parse out the method parameters */
	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O|a", &id, amqp_connection_class_entry, &iniArr) == FAILURE) {
		zend_throw_exception(zend_exception_get_default(TSRMLS_C), "parse parameter error", 0 TSRMLS_CC);
		return;
	}

	amqp_connection = (amqp_connection_object *)zend_object_store_get_object(id TSRMLS_CC);

	/* Pull the login out of the $params array */
	zdata = NULL;
	if (iniArr && SUCCESS == zend_hash_find(HASH_OF (iniArr), "login", sizeof("login"), (void*)&zdata)) {
		convert_to_string(*zdata);
	}
	/* Validate the given login */
	if (zdata && Z_STRLEN_PP(zdata) > 0) {
		if (Z_STRLEN_PP(zdata) < 32) {
			amqp_connection->login = estrndup(Z_STRVAL_PP(zdata), Z_STRLEN_PP(zdata));
		} else {
			zend_throw_exception(amqp_connection_exception_class_entry, "Parameter 'login' exceeds 32 character limit.", 0 TSRMLS_CC);
			return;
		}
	} else {
		amqp_connection->login = estrndup(INI_STR("amqp.login"), strlen(INI_STR("amqp.login")) > 32 ? 32 : strlen(INI_STR("amqp.login")));
	}
	/* @TODO: write a macro to reduce code duplication */

	/* Pull the password out of the $params array */
	zdata = NULL;
	if (iniArr && SUCCESS == zend_hash_find(HASH_OF(iniArr), "password", sizeof("password"), (void*)&zdata)) {
		convert_to_string(*zdata);
	}
	/* Validate the given password */
	if (zdata && Z_STRLEN_PP(zdata) > 0) {
		if (Z_STRLEN_PP(zdata) < 32) {
			amqp_connection->password = estrndup(Z_STRVAL_PP(zdata), Z_STRLEN_PP(zdata));
		} else {
			zend_throw_exception(amqp_connection_exception_class_entry, "Parameter 'password' exceeds 32 character limit.", 0 TSRMLS_CC);
			return;
		}
	} else {
		amqp_connection->password = estrndup(INI_STR("amqp.password"), strlen(INI_STR("amqp.password")) > 32 ? 32 : strlen(INI_STR("amqp.password")));
	}

	/* Pull the host out of the $params array */
	zdata = NULL;
	if (iniArr && SUCCESS == zend_hash_find(HASH_OF(iniArr), "host", sizeof("host"), (void *)&zdata)) {
		convert_to_string(*zdata);
	}
	/* Validate the given host */
	if (zdata && Z_STRLEN_PP(zdata) > 0) {
		if (Z_STRLEN_PP(zdata) < 32) {
			amqp_connection->host = estrndup(Z_STRVAL_PP(zdata), Z_STRLEN_PP(zdata));
		} else {
			zend_throw_exception(amqp_connection_exception_class_entry, "Parameter 'host' exceeds 32 character limit.", 0 TSRMLS_CC);
			return;
		}
	} else {
		amqp_connection->host = estrndup(INI_STR("amqp.host"), strlen(INI_STR("amqp.host")) > 32 ? 32 : strlen(INI_STR("amqp.host")));
	}

	/* Pull the vhost out of the $params array */
	zdata = NULL;
	if (iniArr && SUCCESS == zend_hash_find(HASH_OF (iniArr), "vhost", sizeof("vhost"), (void*)&zdata)) {
		convert_to_string(*zdata);
	}
	/* Validate the given vhost */
	if (zdata && Z_STRLEN_PP(zdata) > 0) {
		if (Z_STRLEN_PP(zdata) < 32) {
			amqp_connection->vhost = estrndup(Z_STRVAL_PP(zdata), Z_STRLEN_PP(zdata));
		} else {
			zend_throw_exception(amqp_connection_exception_class_entry, "Parameter 'vhost' exceeds 32 character limit.", 0 TSRMLS_CC);
			return;
		}
	} else {
		amqp_connection->vhost = estrndup(INI_STR("amqp.vhost"), strlen(INI_STR("amqp.vhost")) > 32 ? 32 : strlen(INI_STR("amqp.vhost")));
	}

	amqp_connection->port = INI_INT("amqp.port");

	if (iniArr && SUCCESS == zend_hash_find(HASH_OF (iniArr), "port", sizeof("port"), (void*)&zdata)) {
		convert_to_long(*zdata);
		amqp_connection->port = (size_t)Z_LVAL_PP(zdata);
	}
}
/* }}} */


/* {{{ proto amqp::isConnected()
check amqp connection */
PHP_METHOD(amqp_connection_class, isConnected)
{
	zval *id;
	amqp_connection_object *amqp_connection;

	/* Try to pull amqp object out of method params */
	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O", &id, amqp_connection_class_entry) == FAILURE) {
		RETURN_FALSE;
	}

	/* Get the connection object out of the store */
	amqp_connection = (amqp_connection_object *)zend_object_store_get_object(id TSRMLS_CC);

	/* If the channel_connect is 1, we have a connection */
	if (amqp_connection->is_channel_connected == '\1') {
		RETURN_TRUE;
	}

	/* We have no connection */
	RETURN_FALSE;
}
/* }}} */


/* {{{ proto amqp::connect()
create amqp connection */
PHP_METHOD(amqp_connection_class, connect)
{
	zval *id;
	amqp_connection_object *amqp_connection;

	/* Try to pull amqp object out of method params */
	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O", &id, amqp_connection_class_entry) == FAILURE) {
		RETURN_FALSE;
	}

	/* Get the connection object out of the store */
	amqp_connection = (amqp_connection_object *)zend_object_store_get_object(id TSRMLS_CC);

	php_amqp_connect(amqp_connection TSRMLS_CC);
	/* @TODO: return connection success or failure */
	RETURN_TRUE;
}

/* }}} */

/* {{{ proto amqp::disconnect()
destroy amqp connection */
PHP_METHOD(amqp_connection_class, disconnect)
{
	zval *id;
	amqp_connection_object *amqp_connection;

	/* Try to pull amqp object out of method params */
	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O", &id, amqp_connection_class_entry) == FAILURE) {
		RETURN_FALSE;
	}

	/* Get the connection object out of the store */
	amqp_connection = (amqp_connection_object *)zend_object_store_get_object(id TSRMLS_CC);

	php_amqp_disconnect(amqp_connection);

	RETURN_TRUE;
}

/* }}} */

/* {{{ proto amqp::reconnect()
recreate amqp connection */
PHP_METHOD(amqp_connection_class, reconnect)
{
	zval *id;
	amqp_connection_object *amqp_connection;

	/* Try to pull amqp object out of method params */
	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O", &id, amqp_connection_class_entry) == FAILURE) {
		RETURN_FALSE;
	}

	/* Get the connection object out of the store */
	amqp_connection = (amqp_connection_object *)zend_object_store_get_object(id TSRMLS_CC);

	if (amqp_connection->is_connected) {
		php_amqp_disconnect(amqp_connection);
	}
	php_amqp_connect(amqp_connection TSRMLS_CC);
	/* @TODO: return the success or failure of connect */
	RETURN_TRUE;
}

/* }}} */

/* {{{ proto amqp::setLogin(string login)
set the login */
PHP_METHOD(amqp_connection_class, setLogin)
{
	zval *id;
	amqp_connection_object *amqp_connection;
	char *login;
	int login_len;

	/* @TODO: use macro when one is created for constructor */
	/* Get the login from the method params */
	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os", &id,
	amqp_connection_class_entry, &login, &login_len) == FAILURE) {
		RETURN_FALSE;
	}

	/* Validate login length */
	if (login_len > 32) {
		zend_throw_exception(amqp_connection_exception_class_entry, "Invalid 'login' given, exceeds 32 characters limit.", 0 TSRMLS_CC);
		return;
	}

	/* Get the connection object out of the store */
	amqp_connection = (amqp_connection_object *)zend_object_store_get_object(id TSRMLS_CC);

	/* Copy the login to the amqp object */
	amqp_connection->login = estrndup(login, login_len);

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto amqp::setPassword(string password)
set the password */
PHP_METHOD(amqp_connection_class, setPassword)
{
	zval *id;
	amqp_connection_object *amqp_connection;
	char *password;
	int password_len;

	/* Get the password from the method params */
	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os", &id,
	amqp_connection_class_entry, &password, &password_len) == FAILURE) {
		RETURN_FALSE;
	}

	/* Validate password length */
	if (password_len > 32) {
		zend_throw_exception(amqp_connection_exception_class_entry, "Invalid 'password' given, exceeds 32 characters limit.", 0 TSRMLS_CC);
		return;
	}

	/* Get the connection object out of the store */
	amqp_connection = (amqp_connection_object *)zend_object_store_get_object(id TSRMLS_CC);

	/* Copy the password to the amqp object */
	amqp_connection->password = estrndup(password, password_len);

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto amqp::setHost(string host)
set the host */
PHP_METHOD(amqp_connection_class, setHost)
{
	zval *id;
	amqp_connection_object *amqp_connection;
	char *host;
	int host_len;

	/* Get the host from the method params */
	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os", &id, amqp_connection_class_entry, &host, &host_len) == FAILURE) {
		RETURN_FALSE;
	}

	/* Validate host length */
	if (host_len > 1024) {
		zend_throw_exception(amqp_connection_exception_class_entry, "Invalid 'host' given, exceeds 1024 character limit.", 0 TSRMLS_CC);
		return;
	}

	/* Get the connection object out of the store */
	amqp_connection = (amqp_connection_object *)zend_object_store_get_object(id TSRMLS_CC);

	/* Copy the host to the amqp object */
	amqp_connection->host = estrndup(host, host_len);

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto amqp::setPort(mixed port)
set the port */
PHP_METHOD(amqp_connection_class, setPort)
{
	zval *id;
	amqp_connection_object *amqp_connection;
	zval *zvalPort;
	int port;

	/* Get the port from the method params */
	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oz", &id, amqp_connection_class_entry, &zvalPort) == FAILURE) {
		RETURN_FALSE;
	}

	/* Parse out the port*/
	switch (Z_TYPE_P(zvalPort)) {
		case IS_DOUBLE:
			port = (int)Z_DVAL_P(zvalPort);
			break;
		case IS_LONG:
			port = (int)Z_LVAL_P(zvalPort);
			break;
		case IS_STRING:
			convert_to_long(zvalPort);
			port = (int)Z_LVAL_P(zvalPort);
			break;
		default:
			port = 0;
	}

	/* Check the port value */
	if (port <= 0 || port > 65535) {
		zend_throw_exception(amqp_connection_exception_class_entry, "Invalid port given. Value must be between 1 and 65535.", 0 TSRMLS_CC);
	}

	/* Get the connection object out of the store */
	amqp_connection = (amqp_connection_object *)zend_object_store_get_object(id TSRMLS_CC);

	/* Copy the port to the amqp object */
	amqp_connection->port = port;

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto amqp::setVhost(string vhost)
set the vhost */
PHP_METHOD(amqp_connection_class, setVhost)
{
	zval *id;
	amqp_connection_object *amqp_connection;
	char *vhost;
	int vhost_len;

	/* Get the vhost from the method params */
	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os", &id,
	amqp_connection_class_entry, &vhost, &vhost_len) == FAILURE) {
		RETURN_FALSE;
	}

	/* Validate vhost length */
	if (vhost_len > 32) {
		zend_throw_exception(amqp_connection_exception_class_entry, "Parameter 'vhost' exceeds 32 characters limit.", 0 TSRMLS_CC);
		return;
	}

	/* Get the connection object out of the store */
	amqp_connection = (amqp_connection_object *)zend_object_store_get_object(id TSRMLS_CC);

	/* Copy the vhost to the amqp object */
	amqp_connection->vhost = estrndup(vhost, vhost_len);

	RETURN_TRUE;
}
/* }}} */


/*
*Local variables:
*tab-width: 4
*c-basic-offset: 4
*End:
*vim600: noet sw=4 ts=4 fdm=marker
*vim<600: noet sw=4 ts=4
*/
