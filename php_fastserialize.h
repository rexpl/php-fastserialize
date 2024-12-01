/* fastserialize extension for PHP */

#ifndef PHP_FASTSERIALIZE_H
# define PHP_FASTSERIALIZE_H

extern zend_module_entry fastserialize_module_entry;
# define phpext_fastserialize_ptr &fastserialize_module_entry

# define PHP_FASTSERIALIZE_VERSION "0.1.0"

# if defined(ZTS) && defined(COMPILE_DL_FASTSERIALIZE)
ZEND_TSRMLS_CACHE_EXTERN()
# endif

#endif	/* PHP_FASTSERIALIZE_H */
