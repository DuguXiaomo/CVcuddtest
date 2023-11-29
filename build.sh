# CUDD 库
CUDD_INCLUDE_PATH=/path/to/cudd/include
CUDD_LIB_PATH=/path/to/cudd/lib

# cJSON 库
CJSON_INCLUDE_PATH=/path/to/cJSON
CJSON_LIB_PATH=/path/to/cJSON

gcc -o bdd_processing src/main.c src/cudd_functions.c src/json_functions.c \
-I$CUDD_INCLUDE_PATH -L$CUDD_LIB_PATH -lcudd \
-I$CJSON_INCLUDE_PATH -L$CJSON_LIB_PATH -lcjson -lm -std=c11