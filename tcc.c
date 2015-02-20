/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2014 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_tcc.h"

  #include <libtcc.h>
  #include <ffi.h>

/* If you declare any globals in php_tcc.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(tcc)
*/

/* True global resources - no need for thread safety here */
static int le_tcc;
static int le_tcc_descriptor;
zend_class_entry *Tcc_ce;

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("tcc.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_tcc_globals, tcc_globals)
    STD_PHP_INI_ENTRY("tcc.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_tcc_globals, tcc_globals)
PHP_INI_END()
*/
/* }}} */

/* Remove the following function when you have successfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto string confirm_tcc_compiled(string arg)
   Return a string to confirm that the module is compiled in */
PHP_FUNCTION(confirm_tcc_compiled)
{
	char *arg = NULL;
	size_t arg_len, len;
	char *strg;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &arg, &arg_len) == FAILURE) {
		return;
	}

	len = spprintf(&strg, 0, "Congratulations! You have successfully modified ext/%.78s/config.m4. Module %.78s is now compiled into PHP.", "tcc", arg);

	RETVAL_STRINGL(strg, len,1);
	efree(strg);
}
/* }}} */
/* The previous line is meant for vim and emacs, so it can correctly fold and
   unfold functions in source code. See the corresponding marks just before
   function definition, where the functions purpose is also documented. Please
   follow this convention for the convenience of others editing your code.
*/

ZEND_METHOD( Tcc , compile )
{
    TCCState *s;
    zval *func_pointer;
    zval *func_name;
    void *func_p = NULL;
    zend_class_entry *self;
    self = Z_OBJCE_P(getThis());

    s = tcc_new();
    if (!s) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING,"Could not create tcc state");
            RETURN_FALSE;
    }

    zval *func_s = zend_read_property(self, getThis(), "func_s", sizeof("func_s")-1, 0 TSRMLS_DC);
    /* MUST BE CALLED before any compilation */
    tcc_set_output_type(s, TCC_OUTPUT_MEMORY);

    if (tcc_compile_string(s, Z_STRVAL_P(func_s)) == -1){
        php_error_docref(NULL TSRMLS_CC, E_WARNING,"compile error");
            RETURN_FALSE;
    }


    /* relocate the code */
    if (tcc_relocate(s, TCC_RELOCATE_AUTO) < 0){
        php_error_docref(NULL TSRMLS_CC, E_WARNING,"Could not  relocate");
        RETURN_FALSE;
    }

    /* get entry symbol */
    func_name = zend_read_property(self, getThis(), "func_name", strlen("func_name"), 0 TSRMLS_DC);

    func_p = tcc_get_symbol(s, Z_STRVAL_P(func_name));

    if (!func_p){
        php_error_docref(NULL TSRMLS_CC, E_WARNING,"Could not  get symbol");
        RETURN_FALSE;
    }
    /* delete the state */
    //tcc_delete(s);
    MAKE_STD_ZVAL(func_pointer);
    ZEND_REGISTER_RESOURCE(func_pointer,func_p,le_tcc_descriptor);
    //printf("func_p->%p\n",func_p);
    zend_update_property(self, getThis(), "func_p", strlen("func_p"), func_pointer TSRMLS_DC);
}

ZEND_METHOD( Tcc , func_define )
{
     zval *func_name;  
     zval *func_args;  
     zval *func_ret;  
     int i = 0;

     zend_class_entry *self;
    self = Z_OBJCE_P(getThis());
    MAKE_STD_ZVAL(func_args);
    array_init(func_args);
    MAKE_STD_ZVAL(func_name);
    MAKE_STD_ZVAL(func_ret);
    /* 取得参数个数 */  
    int argument_count = ZEND_NUM_ARGS();  
    zval ***args = (zval ***)safe_emalloc( sizeof(zval**),argument_count, 0);


    if(argument_count < 2)  
    {  
        WRONG_PARAM_COUNT;  
    }  
      
    /* 参数个数正确，开始接收。 */  
    if(zend_get_parameters_array_ex(argument_count, args) != SUCCESS)  
    {  
        WRONG_PARAM_COUNT;  
    } 
    for (i=2;i<argument_count;++i){
        add_next_index_string(func_args,Z_STRVAL_PP(args[i]),1);
    }
    ZVAL_STRING(func_name,Z_STRVAL_PP(args[1]),1);
    ZVAL_STRING(func_ret,Z_STRVAL_PP(args[0]),1);

zend_update_property(self, getThis(), "func_name", strlen("func_name"), func_name TSRMLS_DC);
zend_update_property(self, getThis(), "func_ret", strlen("func_ret"), func_ret TSRMLS_DC);
zend_update_property(self, getThis(), "func_args", strlen("func_args"), func_args TSRMLS_DC);
}
 
int addd(int a ,int b){
        return a+b;
      }
ZEND_METHOD( Tcc , call )
{
    zval *func_name;  
     zval *func_args;  
     zval *func_ret;  
     int i = 0;
     char *type;
     zval **z_item;

     zend_class_entry *self;
     ffi_cif cif;
    ffi_type **cargs;
    void **values;
    self = Z_OBJCE_P(getThis());
    zval *zfunc_p  = zend_read_property(self, getThis(), "func_p", strlen("func_p"), 0 TSRMLS_DC);
    MAKE_STD_ZVAL(func_args);
    array_init(func_args);
    /* 取得参数个数 */  
    int argument_count = ZEND_NUM_ARGS();  
    zval ***args = safe_emalloc( sizeof(zval**), argument_count,0);
    func_name = zend_read_property(self, getThis(), "func_name", strlen("func_name"), 0 TSRMLS_DC);
    func_ret = zend_read_property(self, getThis(), "func_ret", strlen("func_ret"), 0 TSRMLS_DC);
    func_args = zend_read_property(self, getThis(), "func_args", strlen("func_args"), 0 TSRMLS_DC);
    //php_var_dump(&func_name, 1 TSRMLS_CC); 
    //php_var_dump(&func_ret, 1 TSRMLS_CC); 
    //php_var_dump(&func_args, 1 TSRMLS_CC); 
    int args_count = zend_hash_num_elements(Z_ARRVAL_P(func_args));
    cargs = (ffi_type **)malloc(sizeof(ffi_type *) *(args_count+1));
    values = (void **)malloc(sizeof(void *) *(args_count+1));
    float *fvalues = (float *)malloc(sizeof(float) *(args_count+1));
    long *lvalues = (long *)malloc(sizeof(long) *(args_count+1));
    char **svalues = (char **)malloc(sizeof(char *) *(args_count+1));
    ffi_type *ret_type;
    char *sret;
    long lret;
    float fret;
    void *func_p;
    int (*func )(int,int);
    ZEND_FETCH_RESOURCE(func_p,void*,&zfunc_p,-1,PHP_TCC_DESCRIPTOR_RES_NAME,le_tcc_descriptor);
    if(argument_count < args_count)  
    {  
        WRONG_PARAM_COUNT;  
    }  
      
      
    /* 参数个数正确，开始接收。 */  
    if(zend_get_parameters_array_ex(argument_count, args) != SUCCESS)  
    {  
        WRONG_PARAM_COUNT;  
    } 
    zend_hash_internal_pointer_reset(Z_ARRVAL_P(func_args)); 
    for (i=0;i<args_count;++i){
        zend_hash_get_current_data(Z_ARRVAL_P(func_args), (void**) &z_item);
        type = Z_STRVAL_PP(z_item);

        if(strcmp(type,"char *") == 0){
            *(cargs + i) = &ffi_type_pointer;
            svalues[i]= Z_STRVAL_PP(args[i]);
            *(values + i) = &svalues[i];
        }else if(strcmp(type,"int") == 0){
            *(cargs + i) = &ffi_type_slong;
            lvalues[i]= Z_LVAL_PP(args[i]);
            //printf("%d==>%d\n",i,lvalues[i]);
            *(values + i) = &lvalues[i];
        }else if(strcmp(type,"float") == 0){
            *(cargs + i) = &ffi_type_float;
            fvalues[i]= Z_DVAL_PP(args[i]);
            *(values + i) = &fvalues[i];
        }
        zend_hash_move_forward(Z_ARRVAL_P(func_args));
    }
*(cargs + i) = NULL;
            *(values + i) = NULL;
type = Z_STRVAL_P(func_ret);
        if(strcmp(type,"char *") == 0){
            if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, args_count,
                   &ffi_type_pointer, cargs) == FFI_OK)
                     {
                       ffi_call(&cif, func_p, &sret, values);
                     }
                     RETURN_STRING(sret,1);
        }else if(strcmp(type,"int") == 0){
            if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, args_count,
                   &ffi_type_slong, cargs) == FFI_OK)
                     {
                       ffi_call(&cif, func_p, &lret, values);
                     }
                     //printf("==>%d\n",lret);
                     RETURN_LONG(lret);
        }else if(strcmp(type,"float") == 0){
            if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, args_count,
                   &ffi_type_float, cargs) == FFI_OK)
                     {
                       ffi_call(&cif, func_p, &fret, values);
                     }
                     RETURN_DOUBLE(fret);
        }

RETURN_FALSE;
}
 
ZEND_METHOD( Tcc , __construct )
{
    char *func_s;
    int length = 0;
    zend_class_entry *self;
    self = Z_OBJCE_P(getThis());
    if( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &func_s,&length) == FAILURE )
    {
        printf("Error\n");
        RETURN_NULL();
    }
    zend_update_property_stringl(self, getThis(), "func_s", strlen("func_s"),func_s, length TSRMLS_DC);
}
/* {{{ php_tcc_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_tcc_init_globals(zend_tcc_globals *tcc_globals)
{
	tcc_globals->global_value = 0;
	tcc_globals->global_string = NULL;
}
*/
/* }}} */
static void php_tcc_descriptor_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
}

/* {{{ tcc_functions[]
 *
 * Every user visible function must have an entry in tcc_functions[].
 */
const zend_function_entry tcc_functions[] = {
    ZEND_ME(Tcc,    call,  NULL,   ZEND_ACC_PUBLIC)
              ZEND_ME(Tcc,    func_define,  NULL,   ZEND_ACC_PUBLIC)
              ZEND_ME(Tcc,    compile,  NULL,   ZEND_ACC_PUBLIC)
             ZEND_ME(Tcc,    __construct,    NULL,   ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
    PHP_FE_END  /* Must be the last line in tcc_functions[] */
};

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(tcc)
{
	/* If you have INI entries, uncomment these lines
	REGISTER_INI_ENTRIES();
	*/
	le_tcc_descriptor = zend_register_list_destructors_ex(php_tcc_descriptor_dtor, NULL, PHP_TCC_DESCRIPTOR_RES_NAME,module_number);
	zend_class_entry  ce;
	    INIT_CLASS_ENTRY( ce,"Tcc",tcc_functions);
                     Tcc_ce = zend_register_internal_class_ex(
                            &ce, zend_exception_get_default(TSRMLS_C), NULL TSRMLS_CC
                        );
	    zend_declare_property_null(Tcc_ce, "func_s", strlen("func_s"), ZEND_ACC_PROTECTED TSRMLS_CC);
	    zend_declare_property_null(Tcc_ce, "func_name", strlen("func_name"), ZEND_ACC_PROTECTED TSRMLS_CC);
	    zend_declare_property_null(Tcc_ce, "func_args", strlen("func_args"), ZEND_ACC_PROTECTED TSRMLS_CC);
	    zend_declare_property_null(Tcc_ce, "func_p", strlen("func_p"), ZEND_ACC_PROTECTED TSRMLS_CC);
	return SUCCESS;
}



/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(tcc)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(tcc)
{
#if defined(COMPILE_DL_TCC) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(tcc)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(tcc)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "tcc support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */


/* }}} */

/* {{{ tcc_module_entry
 */
zend_module_entry tcc_module_entry = {
	STANDARD_MODULE_HEADER,
	"tcc",
	tcc_functions,
	PHP_MINIT(tcc),
	PHP_MSHUTDOWN(tcc),
	PHP_RINIT(tcc),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(tcc),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(tcc),
	PHP_TCC_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_TCC
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE();
#endif
ZEND_GET_MODULE(tcc)
#endif


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
