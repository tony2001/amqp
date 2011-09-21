dnl $Id: config.m4 307902 2011-02-01 10:37:53Z bjori $
dnl config.m4 for extension rabbit

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.
dnl amqp
dnl If your extension references something external, use with:


dnl Make sure that the comment is aligned:
 PHP_ARG_WITH(amqp, for amqp support,
 [  --with-amqp             Include amqp support])


if test "$PHP_AMQP" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-amqp -> check with-path
  
         SEARCH_PATH="/usr/local"     # you might want to change this

  	 #SEARCH_FOR="/usr/local/include/amqp.h"  
  	 #SEARCH_FOR="/usr/local/include/amqp_framing.h"  
         SEARCH_FOR="amqp_framing.h"  

     AC_MSG_CHECKING([for amqp files in default path])
      for i in $PHP_AMQP $SEARCH_PATH ; do
       if test -r $i/include/$SEARCH_FOR; 
       then
            AMQP_DIR=$i
            AC_MSG_RESULT(found in $i)
            break
         fi
       done


    if test -z "$AMQP_DIR"; then
       AC_MSG_RESULT([not found])
       AC_MSG_ERROR([Please reinstall the amqp distribution])
    fi

  dnl # --with-amqp -> add include path
  
  PHP_ADD_INCLUDE($AMQP_DIR/include)

  dnl # --with-amqp -> check for lib and symbol presence

      LIBNAME=rabbitmq # you may want to change this
      LIBSYMBOL=rabbitmq # you most likely want to change this 


   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $AMQP_DIR/lib, AMQP_SHARED_LIBADD)
   PHP_SUBST(AMQP_SHARED_LIBADD)

   AMQP_SOURCES="amqp.c amqp_exchange.c amqp_queue.c amqp_connection.c"
 
   PHP_NEW_EXTENSION(amqp, $AMQP_SOURCES, $ext_shared)
fi
