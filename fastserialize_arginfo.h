/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: a554f81c8c70d724a570775265dac44711496cfd */

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_Rexpl_FastSerialize_serialize, 0, 2, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, rows_to_serialize, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, row_definitions, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_Rexpl_FastSerialize_unserialize, 0, 2, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, data, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, row_definitions, IS_ARRAY, 0)
ZEND_END_ARG_INFO()


ZEND_FUNCTION(Rexpl_FastSerialize_serialize);
ZEND_FUNCTION(Rexpl_FastSerialize_unserialize);


static const zend_function_entry ext_functions[] = {
	ZEND_NS_FALIAS("Rexpl\\FastSerialize", serialize, Rexpl_FastSerialize_serialize, arginfo_Rexpl_FastSerialize_serialize)
	ZEND_NS_FALIAS("Rexpl\\FastSerialize", unserialize, Rexpl_FastSerialize_unserialize, arginfo_Rexpl_FastSerialize_unserialize)
	ZEND_FE_END
};
