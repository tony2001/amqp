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

/* $Id: amqp.c 316401 2011-09-08 05:45:36Z pdezwart $ */

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

#include "php_amqp.h"
#include "amqp_connection.h"
#include "amqp_queue.h"
#include "amqp_exchange.h"

#include <unistd.h>

/* True global resources - no need for thread safety here */
zend_class_entry *amqp_connection_class_entry;
zend_class_entry *amqp_queue_class_entry;
zend_class_entry *amqp_exchange_class_entry;
zend_class_entry *amqp_exception_class_entry,
				 *amqp_connection_exception_class_entry,
				 *amqp_exchange_exception_class_entry,
				 *amqp_queue_exception_class_entry;

/* The last parameter of ZEND_BEGIN_ARG_INFO_EX indicates how many of the method parameters are required. */
/* amqp_connection_class ARG_INFO definition */
ZEND_BEGIN_ARG_INFO_EX(arginfo_amqp_connection_class__construct, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_ARRAY_INFO(0, credentials, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_amqp_connection_class_isConnected, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_amqp_connection_class_connect, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_amqp_connection_class_disconnect, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_amqp_connection_class_reconnect, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_amqp_connection_class_setLogin, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 1) 
	ZEND_ARG_INFO(0, login)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_amqp_connection_class_setPassword, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, password)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_amqp_connection_class_setHost, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, host)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_amqp_connection_class_setPort, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, port)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_amqp_connection_class_setVhost, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, vhost)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_amqp_connection_class_setReadTimeout, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, timeout_msec)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_amqp_connection_class_setWriteTimeout, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, timeout_msec)
ZEND_END_ARG_INFO()

/* amqp_queue_class ARG_INFO definition */
ZEND_BEGIN_ARG_INFO_EX(arginfo_amqp_queue_class__construct, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, amqp_connection)
	ZEND_ARG_INFO(0, queue_name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_amqp_queue_class_declare, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, queue_name)
	ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_amqp_queue_class_delete, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_INFO(0, queue_name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_amqp_queue_class_purge, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, queue_name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_amqp_queue_class_bind, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_INFO(0, exchange_name)
	ZEND_ARG_INFO(0, routing_key)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_amqp_queue_class_unbind, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, exchange_name)
	ZEND_ARG_INFO(0, routing_key)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_amqp_queue_class_consume, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, options)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_amqp_queue_class_get, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_amqp_queue_class_cancel, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, consumer_tag)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_amqp_queue_class_ack, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, delivery_tag)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_amqp_queue_class_getName, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

/* amqp_exchange ARG_INFO definition */
ZEND_BEGIN_ARG_INFO_EX(arginfo_amqp_exchange_class__construct, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, amqp_connection)
	ZEND_ARG_INFO(0, exchange_name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_amqp_exchange_class_declare, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_INFO(0, exchange_name)
	ZEND_ARG_INFO(0, exchange_type)
	ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_amqp_exchange_class_bind, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 1)
	ZEND_ARG_INFO(0, exchange_name)
	ZEND_ARG_INFO(0, routing_key)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_amqp_exchange_class_delete, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 0)
	ZEND_ARG_INFO(0, exchange_name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_amqp_exchange_class_publish, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 2)
	ZEND_ARG_INFO(0, message)
	ZEND_ARG_INFO(0, routing_key)
ZEND_END_ARG_INFO()


/* {{{ amqp_functions[]
*
*Every user visible function must have an entry in amqp_functions[].
*/
zend_function_entry amqp_connection_class_functions[] = {
	PHP_ME(amqp_connection_class, __construct, 	arginfo_amqp_connection_class__construct,	ZEND_ACC_PUBLIC)
	PHP_ME(amqp_connection_class, isConnected, 	arginfo_amqp_connection_class_isConnected,	ZEND_ACC_PUBLIC)
	PHP_ME(amqp_connection_class, connect, 		arginfo_amqp_connection_class_connect, 		ZEND_ACC_PUBLIC)
	PHP_ME(amqp_connection_class, disconnect, 	arginfo_amqp_connection_class_disconnect,	ZEND_ACC_PUBLIC)
	PHP_ME(amqp_connection_class, reconnect, 	arginfo_amqp_connection_class_reconnect,	ZEND_ACC_PUBLIC)
	PHP_ME(amqp_connection_class, setLogin, 	arginfo_amqp_connection_class_setLogin,		ZEND_ACC_PUBLIC)
	PHP_ME(amqp_connection_class, setPassword, 	arginfo_amqp_connection_class_setPassword,	ZEND_ACC_PUBLIC)
	PHP_ME(amqp_connection_class, setHost, 		arginfo_amqp_connection_class_setHost,		ZEND_ACC_PUBLIC)
	PHP_ME(amqp_connection_class, setPort, 		arginfo_amqp_connection_class_setPort,		ZEND_ACC_PUBLIC)
	PHP_ME(amqp_connection_class, setVhost, 	arginfo_amqp_connection_class_setVhost,		ZEND_ACC_PUBLIC)
	PHP_ME(amqp_connection_class, setReadTimeout, 	arginfo_amqp_connection_class_setReadTimeout,		ZEND_ACC_PUBLIC)
	PHP_ME(amqp_connection_class, setWriteTimeout, 	arginfo_amqp_connection_class_setWriteTimeout,		ZEND_ACC_PUBLIC)

	{NULL, NULL, NULL}	/* Must be the last line in amqp_functions[] */
};

zend_function_entry amqp_queue_class_functions[] = {
	PHP_ME(amqp_queue_class, __construct,	arginfo_amqp_queue_class__construct,	ZEND_ACC_PUBLIC)
	PHP_ME(amqp_queue_class, declare,		arginfo_amqp_queue_class_declare,		ZEND_ACC_PUBLIC)
	PHP_ME(amqp_queue_class, delete,		arginfo_amqp_queue_class_delete,		ZEND_ACC_PUBLIC)
	PHP_ME(amqp_queue_class, purge,			arginfo_amqp_queue_class_purge,			ZEND_ACC_PUBLIC)
	PHP_ME(amqp_queue_class, bind,			arginfo_amqp_queue_class_bind,			ZEND_ACC_PUBLIC)
	PHP_ME(amqp_queue_class, unbind,		arginfo_amqp_queue_class_unbind,		ZEND_ACC_PUBLIC)
	PHP_ME(amqp_queue_class, consume,		arginfo_amqp_queue_class_consume,		ZEND_ACC_PUBLIC)
	PHP_ME(amqp_queue_class, get,			arginfo_amqp_queue_class_get,			ZEND_ACC_PUBLIC)
	PHP_ME(amqp_queue_class, cancel,		arginfo_amqp_queue_class_cancel,		ZEND_ACC_PUBLIC)
	PHP_ME(amqp_queue_class, ack,			arginfo_amqp_queue_class_ack,			ZEND_ACC_PUBLIC)
	PHP_ME(amqp_queue_class, getName,		arginfo_amqp_queue_class_getName,		ZEND_ACC_PUBLIC)

	{NULL, NULL, NULL}	/* Must be the last line in amqp_functions[] */
};

zend_function_entry amqp_exchange_class_functions[] = {
	PHP_ME(amqp_exchange_class, __construct,	arginfo_amqp_exchange_class__construct, ZEND_ACC_PUBLIC)
	PHP_ME(amqp_exchange_class, declare,		arginfo_amqp_exchange_class_declare,	ZEND_ACC_PUBLIC)
	PHP_ME(amqp_exchange_class, bind,			arginfo_amqp_exchange_class_bind,		ZEND_ACC_PUBLIC)
	PHP_ME(amqp_exchange_class, delete,			arginfo_amqp_exchange_class_delete,		ZEND_ACC_PUBLIC)
	PHP_ME(amqp_exchange_class, publish,		arginfo_amqp_exchange_class_publish,	ZEND_ACC_PUBLIC)

	/* PHP_ME(amqp_queue_class, unbind,		 NULL, ZEND_ACC_PUBLIC) */

	{NULL, NULL, NULL}	/* Must be the last line in amqp_functions[] */
};

zend_function_entry amqp_functions[] = {
	{NULL, NULL, NULL}	/* Must be the last line in amqp_functions[] */
};
/* }}} */

/* {{{ amqp_module_entry
*/
zend_module_entry amqp_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"amqp",
	amqp_functions,
	PHP_MINIT(amqp),
	PHP_MSHUTDOWN(amqp),
	NULL,
	NULL,
	PHP_MINFO(amqp),
#if ZEND_MODULE_API_NO >= 20010901
	"0.1",
#endif
	STANDARD_MODULE_PROPERTIES
};
	/* }}} */

#ifdef COMPILE_DL_AMQP
	ZEND_GET_MODULE(amqp)
#endif


void amqp_error(amqp_rpc_reply_t x, char ** pstr)
{
	switch (x.reply_type) {
		case AMQP_RESPONSE_NORMAL:
			return;

		case AMQP_RESPONSE_NONE:	
			spprintf(pstr, 0, "Missing RPC reply type.");
			break;

		case AMQP_RESPONSE_LIBRARY_EXCEPTION:
			spprintf(pstr, 0, "Library error: %s\n", amqp_error_string(x.library_error));
			break;
			
		case AMQP_RESPONSE_SERVER_EXCEPTION:
			switch (x.reply.id) {
				case AMQP_CONNECTION_CLOSE_METHOD: {
					amqp_connection_close_t *m = (amqp_connection_close_t *)x.reply.decoded;
					spprintf(pstr, 0, "Server connection error: %d, message: %.*s",
						m->reply_code,
						(int) m->reply_text.len,
						(char *)m->reply_text.bytes);
					/* No more error handling necessary, returning. */
					return;
				}
				case AMQP_CHANNEL_CLOSE_METHOD: {
					amqp_channel_close_t *m = (amqp_channel_close_t *) x.reply.decoded;
					spprintf(pstr, 0, "Server channel error: %d, message: %.*s",
						m->reply_code,
						(int)m->reply_text.len,
						(char *)m->reply_text.bytes);
					/* No more error handling necessary, returning. */
					return;
				}
			}
		/* Default for the above switch should be handled by the below default. */
		default:
			spprintf(pstr, 0, "Unknown server error, method id 0x%08X",	x.reply.id);
			break;
	}
}

PHP_INI_BEGIN()
	PHP_INI_ENTRY("amqp.host",			DEFAULT_HOST,			PHP_INI_ALL, NULL)
	PHP_INI_ENTRY("amqp.vhost",			DEFAULT_VHOST,			PHP_INI_ALL, NULL)
	PHP_INI_ENTRY("amqp.port",			DEFAULT_PORT_STR,		PHP_INI_ALL, NULL)
	PHP_INI_ENTRY("amqp.login",			DEFAULT_LOGIN,			PHP_INI_ALL, NULL)
	PHP_INI_ENTRY("amqp.password",		DEFAULT_PASSWORD,		PHP_INI_ALL, NULL)
	PHP_INI_ENTRY("amqp.ack",			DEFAULT_ACK,			PHP_INI_ALL, NULL)
	PHP_INI_ENTRY("amqp.min_consume",	DEFAULT_MIN_CONSUME,	PHP_INI_ALL, NULL)
	PHP_INI_ENTRY("amqp.max_consume",	DEFAULT_MAX_CONSUME,	PHP_INI_ALL, NULL)
PHP_INI_END()

/* {{{ PHP_MINIT_FUNCTION
*/
PHP_MINIT_FUNCTION(amqp)
{
	zend_class_entry ce;

	INIT_CLASS_ENTRY(ce, "AMQPConnection", amqp_connection_class_functions);
	ce.create_object = amqp_ctor;
	amqp_connection_class_entry = zend_register_internal_class(&ce TSRMLS_CC);

	INIT_CLASS_ENTRY(ce, "AMQPQueue", amqp_queue_class_functions);
	ce.create_object = amqp_queue_ctor;
	amqp_queue_class_entry = zend_register_internal_class(&ce TSRMLS_CC);

	INIT_CLASS_ENTRY(ce, "AMQPExchange", amqp_exchange_class_functions);
	ce.create_object = amqp_exchange_ctor;
	amqp_exchange_class_entry = zend_register_internal_class(&ce TSRMLS_CC);

	INIT_CLASS_ENTRY(ce, "AMQPException", NULL);
	amqp_exception_class_entry = zend_register_internal_class_ex(&ce, (zend_class_entry*)zend_exception_get_default(TSRMLS_C), NULL TSRMLS_CC);

	INIT_CLASS_ENTRY(ce, "AMQPConnectionException", NULL);
	amqp_connection_exception_class_entry = zend_register_internal_class_ex(&ce, amqp_exception_class_entry, NULL TSRMLS_CC);

	INIT_CLASS_ENTRY(ce, "AMQPExchangeException", NULL);
	amqp_exchange_exception_class_entry = zend_register_internal_class_ex(&ce, amqp_exception_class_entry, NULL TSRMLS_CC);

	INIT_CLASS_ENTRY(ce, "AMQPQueueException", NULL);
	amqp_queue_exception_class_entry = zend_register_internal_class_ex(&ce, amqp_exception_class_entry, NULL TSRMLS_CC);

	REGISTER_INI_ENTRIES();

	REGISTER_LONG_CONSTANT("AMQP_DURABLE",			AMQP_DURABLE,		CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("AMQP_PASSIVE",			AMQP_PASSIVE,		CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("AMQP_EXCLUSIVE",		AMQP_EXCLUSIVE,		CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("AMQP_AUTODELETE",		AMQP_AUTODELETE,	CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("AMQP_INTERNAL",			AMQP_INTERNAL,		CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("AMQP_NOLOCAL",			AMQP_NOLOCAL,		CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("AMQP_NOACK",			AMQP_NOACK,			CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("AMQP_IFEMPTY",			AMQP_IFEMPTY,		CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("AMQP_IFUNUSED",			AMQP_IFUNUSED,		CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("AMQP_MANDATORY",		AMQP_MANDATORY,		CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("AMQP_IMMEDIATE",		AMQP_IMMEDIATE,		CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("AMQP_MULTIPLE",			AMQP_MULTIPLE,		CONST_CS | CONST_PERSISTENT);

	REGISTER_STRING_CONSTANT("AMQP_EX_TYPE_DIRECT",	AMQP_EX_TYPE_DIRECT,	CONST_CS | CONST_PERSISTENT);
	REGISTER_STRING_CONSTANT("AMQP_EX_TYPE_FANOUT",	AMQP_EX_TYPE_FANOUT,	CONST_CS | CONST_PERSISTENT);
	REGISTER_STRING_CONSTANT("AMQP_EX_TYPE_TOPIC",	AMQP_EX_TYPE_TOPIC,		CONST_CS | CONST_PERSISTENT);
	REGISTER_STRING_CONSTANT("AMQP_EX_TYPE_HEADER",	AMQP_EX_TYPE_HEADER,	CONST_CS | CONST_PERSISTENT);

	return SUCCESS;

}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
*/
PHP_MSHUTDOWN_FUNCTION(amqp)
{
	UNREGISTER_INI_ENTRIES();

	return SUCCESS;
}
/* }}} */


/* {{{ PHP_MINFO_FUNCTION
*/
PHP_MINFO_FUNCTION(amqp)
{
	/* Build date time from compiler macros */
	char datetime[32];
	char **pstr = (char **)&datetime;
	spprintf(pstr, 0, "%s @ %s", __DATE__, __TIME__);

	php_info_print_table_start();
	php_info_print_table_header(2, "Version",					"$Revision: 316401 $");
	php_info_print_table_header(2, "Compiled",					*pstr);
	php_info_print_table_header(2, "AMQP protocol version", 	"8.0");
	DISPLAY_INI_ENTRIES();

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
