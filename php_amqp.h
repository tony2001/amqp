/*
  +----------------------------------------------------------------------+
  | PHP Version 5														|
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2007 The PHP Group								|
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,	  |
  | that is bundled with this package in the file LICENSE, and is		|
  | available through the world-wide-web at the following url:		   |
  | http://www.php.net/license/3_01.txt								  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to		  |
  | license@php.net so we can mail you a copy immediately.			   |
  +----------------------------------------------------------------------+
  | Author: Alexandre Kalendarev akalend@mail.ru Copyright (c) 2009-2010 |
  | Contributor: 														 |
  | - Pieter de Zwart pdezwart@php.net									 |
  | - Brad Rodriguez brodrigu@gmail.com									 |
  +----------------------------------------------------------------------+
*/

/* $Id: php_amqp.h 316401 2011-09-08 05:45:36Z pdezwart $ */

#ifndef PHP_AMQP_H
#define PHP_AMQP_H

#include <amqp.h>

/* Add pseudo refcount macros for PHP version < 5.3 */
#ifndef Z_REFCOUNT_PP

#define Z_REFCOUNT_PP(ppz)				Z_REFCOUNT_P(*(ppz))
#define Z_SET_REFCOUNT_PP(ppz, rc)		Z_SET_REFCOUNT_P(*(ppz), rc)
#define Z_ADDREF_PP(ppz)				Z_ADDREF_P(*(ppz))
#define Z_DELREF_PP(ppz)				Z_DELREF_P(*(ppz))
#define Z_ISREF_PP(ppz)					Z_ISREF_P(*(ppz))
#define Z_SET_ISREF_PP(ppz)				Z_SET_ISREF_P(*(ppz))
#define Z_UNSET_ISREF_PP(ppz)			Z_UNSET_ISREF_P(*(ppz))
#define Z_SET_ISREF_TO_PP(ppz, isref)	Z_SET_ISREF_TO_P(*(ppz), isref)

#define Z_REFCOUNT_P(pz)				zval_refcount_p(pz)
#define Z_SET_REFCOUNT_P(pz, rc)		zval_set_refcount_p(pz, rc)
#define Z_ADDREF_P(pz)					zval_addref_p(pz)
#define Z_DELREF_P(pz)					zval_delref_p(pz)
#define Z_ISREF_P(pz)					zval_isref_p(pz)
#define Z_SET_ISREF_P(pz)				zval_set_isref_p(pz)
#define Z_UNSET_ISREF_P(pz)				zval_unset_isref_p(pz)
#define Z_SET_ISREF_TO_P(pz, isref)		zval_set_isref_to_p(pz, isref)

#define Z_REFCOUNT(z)					Z_REFCOUNT_P(&(z))
#define Z_SET_REFCOUNT(z, rc)			Z_SET_REFCOUNT_P(&(z), rc)
#define Z_ADDREF(z)						Z_ADDREF_P(&(z))
#define Z_DELREF(z)						Z_DELREF_P(&(z))
#define Z_ISREF(z)						Z_ISREF_P(&(z))
#define Z_SET_ISREF(z)					Z_SET_ISREF_P(&(z))
#define Z_UNSET_ISREF(z)				Z_UNSET_ISREF_P(&(z))
#define Z_SET_ISREF_TO(z, isref)		Z_SET_ISREF_TO_P(&(z), isref)

#if defined(__GNUC__)
#define zend_always_inline inline __attribute__((always_inline))
#elif defined(_MSC_VER)
#define zend_always_inline __forceinline
#else
#define zend_always_inline inline
#endif

static zend_always_inline zend_uint zval_refcount_p(zval* pz) {
	return pz->refcount;
}

static zend_always_inline zend_uint zval_set_refcount_p(zval* pz, zend_uint rc) {
	return pz->refcount = rc;
}

static zend_always_inline zend_uint zval_addref_p(zval* pz) {
	return ++pz->refcount;
}

static zend_always_inline zend_uint zval_delref_p(zval* pz) {
	return --pz->refcount;
}

static zend_always_inline zend_bool zval_isref_p(zval* pz) {
	return pz->is_ref;
}

static zend_always_inline zend_bool zval_set_isref_p(zval* pz) {
	return pz->is_ref = 1;
}

static zend_always_inline zend_bool zval_unset_isref_p(zval* pz) {
	return pz->is_ref = 0;
}

static zend_always_inline zend_bool zval_set_isref_to_p(zval* pz, zend_bool isref) {
	return pz->is_ref = isref;
}

#else

#define PHP_ATLEAST_5_3   true

#endif



extern zend_module_entry amqp_module_entry;
#define phpext_amqp_ptr &amqp_module_entry

#ifdef PHP_WIN32
#define PHP_AMQP_API __declspec(dllexport)
#else
#define PHP_AMQP_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#define AMQP_NOPARM			1

#define AMQP_DURABLE		2
#define AMQP_PASSIVE		4
#define AMQP_EXCLUSIVE		8
#define AMQP_AUTODELETE		16
#define AMQP_INTERNAL		32
#define AMQP_NOLOCAL		64
#define AMQP_NOACK			128
#define AMQP_IFEMPTY		256
#define AMQP_IFUNUSED		528
#define AMQP_MANDATORY		1024
#define AMQP_IMMEDIATE		2048
#define AMQP_MULTIPLE	   4096

#define AMQP_EX_TYPE_DIRECT	 "direct"
#define AMQP_EX_TYPE_FANOUT	 "fanout"
#define AMQP_EX_TYPE_TOPIC	  "topic"
#define AMQP_EX_TYPE_HEADER	 "header"

PHP_MINIT_FUNCTION(amqp);
PHP_MSHUTDOWN_FUNCTION(amqp);
PHP_MINFO_FUNCTION(amqp);

void amqp_error(amqp_rpc_reply_t x, char ** pstr);

/* True global resources - no need for thread safety here */
extern zend_class_entry *amqp_connection_class_entry;
extern zend_class_entry *amqp_queue_class_entry;
extern zend_class_entry *amqp_exchange_class_entry;
extern zend_class_entry *amqp_exception_class_entry,
				 *amqp_connection_exception_class_entry,
				 *amqp_exchange_exception_class_entry,
				 *amqp_queue_exception_class_entry;


#define FRAME_MAX				131072	/* max length (size) of frame */
#define HEADER_FOOTER_SIZE		8	   /*  7 bytes up front, then payload, then 1 byte footer */
#define DEFAULT_PORT			5672	/* default AMQP port */
#define DEFAULT_PORT_STR		"5672"
#define DEFAULT_HOST			"localhost"
#define DEFAULT_VHOST			"/"
#define DEFAULT_LOGIN			"guest"
#define DEFAULT_PASSWORD		"guest"
#define DEFAULT_ACK				"1"
#define DEFAULT_MIN_CONSUME		"0"
#define DEFAULT_MAX_CONSUME		"1"
#define AMQP_CHANNEL			1	   /* default channel number */
#define AMQP_HEARTBEAT			0	   /* heartbeat */

#define AMQP_NULLARGS			amqp_table_t arguments = {0, NULL};
#define AMQP_PASSIVE_D			short passive = (AMQP_PASSIVE & parms) ? 1 : 0;
#define AMQP_DURABLE_D			short durable = (AMQP_DURABLE & parms) ? 1 : 0;
#define AMQP_AUTODELETE_D		short auto_delete = (AMQP_AUTODELETE & parms) ? 1 : 0;
#define AMQP_EXCLUSIVE_D		short exclusive = (AMQP_EXCLUSIVE & parms) ? 1 : 0;

#define AMQP_SET_NAME(ctx, str) (ctx)->name_len = strlen(str) >= sizeof((ctx)->name) ? sizeof((ctx)->name) - 1 : strlen(str); \
			 strncpy((ctx)->name, name, (ctx)->name_len); \
				 (ctx)->name[(ctx)->name_len] = '\0';

/* If you declare any globals in php_amqp.h uncomment this:
 ZEND_DECLARE_MODULE_GLOBALS(amqp)
*/

typedef struct _amqp_connection_object {
	zend_object zo;
	char is_connected;
	char is_channel_connected;
	char *login;
	int char_len;
	char *password;
	int password_len;
	char *host;
	int host_len;
	char *vhost;
	int vhost_len;
	int port;
	int fd;
	amqp_connection_state_t conn;
} amqp_connection_object;

typedef struct _amqp_queue_object {
	zend_object zo;
	zval *cnn;
	char is_connected;
	char name[255];
	int name_len;
	char consumer_tag[255];
	int consumer_tag_len;
	int passive; /* @TODO: consider making these bit fields */
	int durable;
	int exclusive;
	int auto_delete; /* end @TODO */
} amqp_queue_object;


typedef struct _amqp_exchange_object {
	zend_object zo;
	zval *cnn;
	char is_connected;
	char name[255];
	int name_len;
} amqp_exchange_object;

#ifdef ZTS
#define AMQP_G(v) TSRMG(amqp_globals_id, zend_amqp_globals *, v)
#else
#define AMQP_G(v) (amqp_globals.v)
#endif

#endif	/* PHP_AMQP_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
