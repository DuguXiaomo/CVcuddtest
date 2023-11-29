# CUDD 库
CUDD_INCLUDE_PATH=/home/public/include
CUDD_LIB_PATH=/home/public/lib


# cJSON 库
CJSON_INCLUDE_PATH=/home/eda230906/cJSON.h
CJSON_LIB_PATH=/home/eda230906/cJSON.c

gcc -o bdd_processing test.c cudd.c cJSON.c \
-I$CUDD_INCLUDE_PATH -L$CUDD_LIB_PATH -lcudd \
-I$CJSON_INCLUDE_PATH -L$CJSON_LIB_PATH -lcjson -lm -std=c11