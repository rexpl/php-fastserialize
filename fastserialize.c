/* fastserialize extension for PHP */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "php.h"
#include "ext/standard/info.h"
#include "php_fastserialize.h"
#include "fastserialize_arginfo.h"

enum Type {
	type_bigint,
	type_int,
	type_varchar255,
};

struct ColumnDefinition {
	enum Type type;
	size_t key_size;
	char* key;
};

static void throw_value_exception(const char* error_message, const char* extra) {
	zend_string* class_name = zend_string_init("ValueError", strlen("ValueError"), 0);
	zend_throw_error(zend_lookup_class(class_name), error_message, extra);
	zend_string_release(class_name);
}

static struct ColumnDefinition* parse_row_definitions(zval* row_definitions, size_t* num_columns) {
    // Get the hash table of row_definitions
    HashTable* columns_array = Z_ARRVAL_P(row_definitions);

    *num_columns = zend_hash_num_elements(columns_array);

    struct ColumnDefinition* column_definitions = (struct ColumnDefinition*) malloc(*num_columns * sizeof(struct ColumnDefinition));

    if (column_definitions == NULL) {
		zend_error(E_ERROR, "Failed to allocate memory.");
        return NULL;
    }

    int index = 0;

	#define CLEAR_MEMORY_BECAUSE_FAILED \
		for (size_t i = 0; i < index; i++) free(column_definitions[i].key); \
		free(column_definitions);

	zend_ulong numeric_key;
	zend_string* string_key;
    zval* value;

    ZEND_HASH_FOREACH_KEY_VAL(columns_array, numeric_key, string_key, value) {
		if (Z_TYPE_P(value) != IS_STRING && string_key != NULL) {
			CLEAR_MEMORY_BECAUSE_FAILED;
			throw_value_exception("Each column definition key should be a string.", "");
			return NULL;
		}

		enum Type type;
		const char* column_type_string = Z_STRVAL_P(value);

		if (strcmp(column_type_string, "bigint") == 0) {
			type = type_bigint;
		} else if (strcmp(column_type_string, "int") == 0) {
			type = type_int;
		} else if (strcmp(column_type_string, "varchar255") == 0) {
			type = type_varchar255;
		} else  {
			CLEAR_MEMORY_BECAUSE_FAILED;
			throw_value_exception("Invalid column type \"%s\".", column_type_string);
			return NULL;
		}

		column_definitions[index].type = type;
		column_definitions[index].key_size = string_key->len;
		column_definitions[index].key = strdup(ZSTR_VAL(string_key)); // Copy the key as string

		// Increment the index for the next struct
		index++;
    } ZEND_HASH_FOREACH_END();

    return column_definitions;
}

static void clear_column_definition(struct ColumnDefinition* column_definitions, size_t num_columns) {
	for (size_t i = 0; i < num_columns; i++) {
		free(column_definitions[i].key);
	}
	free(column_definitions);
}

static void exit_with_value_error(struct ColumnDefinition* columns, size_t num_columns, zval* return_value, const char* error_message, const char* column_key) {
	clear_column_definition(columns, num_columns);
	throw_value_exception(error_message, column_key);

	RETURN_NULL();
}

PHP_FUNCTION(Rexpl_FastSerialize_serialize)
{
	zval* rows_to_serialize;
	zval* row_definition;

	ZEND_PARSE_PARAMETERS_START(2, 2);
		Z_PARAM_ARRAY(rows_to_serialize);
		Z_PARAM_ARRAY(row_definition);
	ZEND_PARSE_PARAMETERS_END();

	size_t num_columns;
    struct ColumnDefinition* column_definitions = parse_row_definitions(row_definition, &num_columns);

	// Failed probably to user error, and will have freed its memory and reported the error.
	if (column_definitions == NULL) {
		return;
	}

	HashTable* rows_array = Z_ARRVAL_P(rows_to_serialize);

	if (zend_hash_num_elements(rows_array) == 0) {
		clear_column_definition(column_definitions, num_columns);
        RETURN_EMPTY_STRING();
    }

	size_t totalSize = 0;
    zval* entry;

	ZEND_HASH_FOREACH_VAL(rows_array, entry)
	{
		if (Z_TYPE_P(entry) != IS_ARRAY) {
			exit_with_value_error(column_definitions, num_columns, return_value, "Each row must be an array.", "");
			return;
        }

        HashTable* row = Z_ARRVAL_P(entry);

		for (size_t i = 0; i < num_columns; i++) {
			struct ColumnDefinition column = column_definitions[i];
			zval* value = zend_hash_str_find(row, column.key, column.key_size);

			if (value == NULL) {
				exit_with_value_error(column_definitions, num_columns, return_value, "Row with missing column \"%s\".", column.key);
				return;
			}

			if (column.type == type_varchar255) {
				if (Z_TYPE_P(value) != IS_STRING) {
					exit_with_value_error(column_definitions, num_columns, return_value, "Column \"%s\" encountered a non string value.", column.key);
					return;
				}
				size_t strLength = Z_STRLEN_P(value);
				if (strLength > 255) {
					exit_with_value_error(column_definitions, num_columns, return_value, "Column \"%s\" encountered a string longer than 255 bytes.", column.key);
					return;
				}
				// legnth header + string + null termination byte
				totalSize += sizeof(uint8_t) + strLength + 1;
			}
			// We group the different types of ints together so we don't have to validate all the time.
			else {
				if (Z_TYPE_P(value) != IS_LONG) {
					exit_with_value_error(column_definitions, num_columns, return_value, "Column \"%s\" encountered a non int value.", column.key);
					return;
				}
				totalSize += column.type == type_bigint ? sizeof(long) : sizeof(int);
			}
		}
	}
	ZEND_HASH_FOREACH_END();

	char* result = malloc(totalSize);
    if (result == NULL) {
		clear_column_definition(column_definitions, num_columns);
        zend_error(E_ERROR, "Failed to allocate memory for serialization.");
        RETURN_NULL();
    }

	char* cursor = result;

	ZEND_HASH_FOREACH_VAL(rows_array, entry)
    {
        HashTable* row = Z_ARRVAL_P(entry);

        for (size_t i = 0; i < num_columns; i++) {
			struct ColumnDefinition column = column_definitions[i];
			zval* value = zend_hash_str_find(row, column.key, column.key_size);

			switch (column.type) {
				case type_bigint: {
					long raw_value = Z_LVAL_P(value);
					memcpy(cursor, &raw_value, sizeof(long));
        			cursor += sizeof(long);
					break;
				}
				case type_int: {
					int raw_value = (int) Z_LVAL_P(value);
					memcpy(cursor, &raw_value, sizeof(int));
        			cursor += sizeof(int);
					break;
				}
				case type_varchar255: {
					char* str = Z_STRVAL_P(value);
        			size_t str_length = Z_STRLEN_P(value);

					uint8_t length_header = (uint8_t) str_length;
					memcpy(cursor, &length_header, sizeof(uint8_t));
					cursor += sizeof(uint8_t);

					memcpy(cursor, str, str_length + 1); // Include null terminator
					cursor += str_length + 1;
					break;
				}
			}
		}
    }
    ZEND_HASH_FOREACH_END();
	
	clear_column_definition(column_definitions, num_columns);
	RETVAL_STRINGL(result, totalSize);
	free(result);
}

PHP_FUNCTION(Rexpl_FastSerialize_unserialize)
{
	char *data;
	size_t data_length;
	zval* row_definition;

	ZEND_PARSE_PARAMETERS_START(2, 2);
		Z_PARAM_STRING(data, data_length);
		Z_PARAM_ARRAY(row_definition);
	ZEND_PARSE_PARAMETERS_END();

	size_t num_columns;
    struct ColumnDefinition* column_definitions = parse_row_definitions(row_definition, &num_columns);

	// Failed probably to user error, and will have freed its memory and reported the error.
	if (column_definitions == NULL) {
		return;
	}

	char* cursor = data;
    size_t total_length = data_length;

	array_init_size(return_value, 100000);

	while (cursor < data + total_length) {
        zval entry;
    	array_init(&entry);

		for (size_t i = 0; i < num_columns; i++) {
			struct ColumnDefinition column = column_definitions[i];

			switch (column.type) {
				case type_int: {
					int int_value;
					memcpy(&int_value, cursor, sizeof(int));
					cursor += sizeof(int);
					add_assoc_long_ex(&entry, column.key, column.key_size, int_value);
					break;
				}
				case type_bigint: {
					long long_value;
					memcpy(&long_value, cursor, sizeof(long));
					cursor += sizeof(long);
					add_assoc_long_ex(&entry, column.key, column.key_size, long_value);
					break;
				}
				case type_varchar255: {}
			}
		}

		add_next_index_zval(return_value, &entry);
	}

	clear_column_definition(column_definitions, num_columns);
}

/* {{{ PHP_RINIT_FUNCTION */
PHP_RINIT_FUNCTION(fastserialize)
{
#if defined(ZTS) && defined(COMPILE_DL_FASTSERIALIZE)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif

	return SUCCESS;
}

PHP_MINFO_FUNCTION(fastserialize)
{
	php_info_print_table_start();
	php_info_print_table_row(2, "fastserialize support", "enabled");
	php_info_print_table_end();
}

zend_module_entry fastserialize_module_entry = {
	STANDARD_MODULE_HEADER,
	"fastserialize",
	ext_functions,
	NULL,
	NULL,
	PHP_RINIT(fastserialize),
	NULL,
	PHP_MINFO(fastserialize),
	PHP_FASTSERIALIZE_VERSION,
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_FASTSERIALIZE
# ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
# endif
ZEND_GET_MODULE(fastserialize)
#endif
