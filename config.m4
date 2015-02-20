dnl $Id$
dnl config.m4 for extension tcc

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

 PHP_ARG_WITH(tcc, for tcc support,
 dnl Make sure that the comment is aligned:
 [  --with-tcc             Include tcc support])

dnl Otherwise use enable:

dnl PHP_ARG_ENABLE(tcc, whether to enable tcc support,
dnl Make sure that the comment is aligned:
dnl [  --enable-tcc           Enable tcc support])

if test "$PHP_TCC" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-tcc -> check with-path
   SEARCH_PATH="/usr/local /usr /home/snow/playground/tcc"     # you might want to change this
   SEARCH_FOR="/include/libtcc.h"  # you most likely want to change this
   if test -r $PHP_TCC/$SEARCH_FOR; then # path given as parameter
     TCC_DIR=$PHP_TCC
   else # search default path list
     AC_MSG_CHECKING([for tcc files in default path])
     for i in $SEARCH_PATH ; do
       if test -r $i/$SEARCH_FOR; then
         TCC_DIR=$i
         AC_MSG_RESULT(found in $i)
       fi
     done
   fi
  dnl
   if test -z "$TCC_DIR"; then
     AC_MSG_RESULT([not found])
     AC_MSG_ERROR([Please reinstall the tcc distribution])
   fi

  dnl # --with-tcc -> add include path
   PHP_ADD_INCLUDE($TCC_DIR/include)

   # --with-tcc -> check for lib and symbol presence
   LIBNAME=tcc # you may want to change this
   LIBSYMBOL=tcc # you most likely want to change this 

  PHP_ADD_LIBRARY_WITH_PATH(tcc,$TCC_DIR/lib,TCC_SHARED_LIBADD)
  PHP_SUBST(TCC_SHARED_LIBADD)


   SEARCH_PATH="/usr/local /usr"     # you might want to change this
   SEARCH_FOR="/include/ffi.h"  # you most likely want to change this
   if test -r $PHP_TCC/$SEARCH_FOR; then # path given as parameter
     TCC_DIR=$PHP_TCC
   else # search default path list
     AC_MSG_CHECKING([for tcc files in default path])
     for i in $SEARCH_PATH ; do
       if test -r $i/$SEARCH_FOR; then
         TCC_DIR=$i
         AC_MSG_RESULT(found in $i)
       fi
     done
   fi
  dnl
   if test -z "$TCC_DIR"; then
     AC_MSG_RESULT([not found])
     AC_MSG_ERROR([Please reinstall the libffi distribution])
   fi

  dnl # --with-tcc -> add include path
   PHP_ADD_INCLUDE($TCC_DIR/include)
  PHP_ADD_LIBRARY_WITH_PATH(ffi,$TCC_DIR/lib,TCC_SHARED_LIBADD)
  PHP_SUBST(TCC_SHARED_LIBADD)
   

  PHP_NEW_EXTENSION(tcc, tcc.c, $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1)
fi
