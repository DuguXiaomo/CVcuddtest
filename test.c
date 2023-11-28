#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cJSON.h>
#include <cudd.h>

// 定义 BDDNode 为指向 CUDD DdNode 的指针
typedef DdNode* BDDNode;

// 变量的结构定义
typedef struct {
    int id;
    char name[50];
    int isSigned;
    int bitWidth;
} Variable;

// 操作符类型的枚举定义
typedef enum {
    OP_LOGICAL_AND,
    OP_LOGICAL_OR,
    OP_LOGICAL_NOT,
    OP_BITWISE_AND,
    OP_BITWISE_OR,
    OP_BITWISE_XOR,
    OP_BITWISE_NOT,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_MODULUS,
    OP_GREATER_THAN,
    OP_LESS_THAN,
    OP_GREATER_THAN_OR_EQUAL,
    OP_LESS_THAN_OR_EQUAL,
    OP_EQUAL,
    OP_NOT_EQUAL,
    OP_UNDEFINED
} OperatorType;

// 表达式的结构定义
typedef struct Expression {
    char op[10];
    int id;
    int value;
    struct Expression* lhsExpression;
    struct Expression* rhsExpression;
    struct Expression* ifExpression;
    int bitWidth;
    int thenPaths;
    int elsePaths;
    int complementPaths;
} Expression;

// CUDD 管理器
DdManager* ddManager;

// 函数原型
void initializeCuddManager();
void freeCuddManager();
int64_t extendToWidth(int64_t value, int targetWidth, int isSigned, int originalWidth);
void computePaths(Expression* expression);
void outputRandomAssignmentsToFile(Variable* variables, int numVariables, int* randomAssignments, int N, const char* filename);
Variable* readVariablesFromJSON(const char* jsonText, int* numVariables);
Expression* buildExpressionFromJSONObject(cJSON* jsonObject);
BDDNode buildBDDFromExpressions(DdManager* dd, cJSON* expressions, Variable* variables);
void sampleFromBDD(DdManager* dd, BDDNode bdd, Variable* variables, int numVariables, int N, Expression* expression);
int compareVariableIds(const void* a, const void* b);

// 初始化 CUDD 管理器
void initializeCuddManager() {
    ddManager = Cudd_Init(0, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);
}

// 释放 CUDD 管理器资源
void freeCuddManager() {
    if (ddManager != NULL) {
        Cudd_Quit(ddManager);
    }
}

// 将变量扩展到指定的位宽
int64_t extendToWidth(int64_t value, int targetWidth, int isSigned, int originalWidth) {
    if (isSigned) {
        int64_t signBit = value & (1ULL << (originalWidth - 1));
        if (signBit) {
            int64_t signExtension = (int64_t)-1 << originalWidth;
            return value | (signExtension & ~((1ULL << targetWidth) - 1));
        } else {
            return value & ((1ULL << targetWidth) - 1);
        }
    } else {
        return value & ((1ULL << targetWidth) - 1);
    }
}

// 计算表达式树中每个节点的路径数
void computePaths(Expression* expression) {
    if (expression == NULL) {
        return;
    }

    if (expression->lhsExpression != NULL) {
        computePaths(expression->lhsExpression);
    }

    if (expression->rhsExpression != NULL) {
        computePaths(expression->rhsExpression);
    }

    expression->thenPaths = expression->elsePaths = expression->complementPaths = 1;

    if (expression->lhsExpression != NULL) {
        expression->thenPaths += expression->lhsExpression->thenPaths;
        expression->elsePaths += expression->lhsExpression->elsePaths;
        expression->complementPaths += expression->lhsExpression->complementPaths;
    }

    if (expression->rhsExpression != NULL) {
        expression->thenPaths += expression->rhsExpression->thenPaths;
        expression->elsePaths += expression->rhsExpression->elsePaths;
        expression->complementPaths += expression->rhsExpression->complementPaths;
    }
}

// 输出随机赋值到文件，按指定格式
void outputRandomAssignmentsToFile(Variable* variables, int numVariables, int* randomAssignments, int N, const char* filename) {
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        fprintf(stderr, "Error opening file for writing\n");
        return;
    }

    fprintf(file, "\"assignment_list\": [\n");

    for (int i = 0; i < N; ++i) {
        fprintf(file, "[ // assignment %d\n", i + 1);

        qsort(randomAssignments + i * numVariables, numVariables, sizeof(int), compareVariableIds);

        for (int j = 0; j < numVariables; ++j) {
            int variableId = variables[j].id;
            int value = randomAssignments[i * numVariables + j];
            fprintf(file, "{\"value\": %d} // variable %d\n", value, variableId);
        }

        fprintf(file, "],\n");
    }

    fprintf(file, "]\n");

    fclose(file);
}

// 比较变量 ID 的辅助函数，用于 qsort
int compareVariableIds(const void* a, const void* b) {
    return (*((int*)a)) - (*((int*)b));
}

// 从 JSON 文本中读取变量
Variable* readVariablesFromJSON(const char* jsonText, int* numVariables) {
    cJSON* root = cJSON_Parse(jsonText);

    if (!root) {
        fprintf(stderr, "Error parsing JSON\n");
        return NULL;
    }

    cJSON* variableArray = cJSON_GetObjectItemCaseSensitive(root, "variable_list");

    if (!variableArray || !cJSON_IsArray(variableArray)) {
        fprintf(stderr, "JSON does not contain a 'variable_list' array\n");
        cJSON_Delete(root);
        return NULL;
    }

    *numVariables = cJSON_GetArraySize(variableArray);

    Variable* variables = (Variable*)malloc(*numVariables * sizeof(Variable));

    if (!variables) {
        fprintf(stderr, "Memory allocation error\n");
        cJSON_Delete(root);
        return NULL;
    }

    for (int i = 0; i < *numVariables; ++i) {
        cJSON* variable = cJSON_GetArrayItem(variableArray, i);

        if (!variable || !cJSON_IsObject(variable)) {
            fprintf(stderr, "Error reading variable at index %d\n", i);
            free(variables);
            cJSON_Delete(root);
            return NULL;
        }

        cJSON* id = cJSON_GetObjectItemCaseSensitive(variable, "id");
        cJSON* name = cJSON_GetObjectItemCaseSensitive(variable, "name");
        cJSON* isSigned = cJSON_GetObjectItemCaseSensitive(variable, "signed");
        cJSON* bitWidth = cJSON_GetObjectItemCaseSensitive(variable, "bit_width");

        if (!id || !cJSON_IsNumber(id) || !name || !cJSON_IsString(name) ||
            !isSigned || !cJSON_IsBool(isSigned) || !bitWidth || !cJSON_IsNumber(bitWidth)) {
            fprintf(stderr, "Error reading variable fields at index %d\n", i);
            free(variables);
            cJSON_Delete(root);
            return NULL;
        }

        variables[i].id = id->valueint;
        strncpy(variables[i].name, name->valuestring, sizeof(variables[i].name));
        variables[i].isSigned = cJSON_IsTrue(isSigned);
        variables[i].bitWidth = bitWidth->valueint;
    }

    cJSON_Delete(root);
    return variables;
}

// 从 JSON 对象构建表达式
Expression* buildExpressionFromJSONObject(cJSON* jsonObject) {
    if (!jsonObject || !cJSON_IsObject(jsonObject)) {
        fprintf(stderr, "Invalid JSON object\n");
        return NULL;
    }

    Expression* expression = (Expression*)malloc(sizeof(Expression));

    if (!expression) {
        fprintf(stderr, "Memory allocation error\n");
        return NULL;
    }

    cJSON* op = cJSON_GetObjectItemCaseSensitive(jsonObject, "op");
    cJSON* id = cJSON_GetObjectItemCaseSensitive(jsonObject, "id");
    cJSON* value = cJSON_GetObjectItemCaseSensitive(jsonObject, "value");
    cJSON* lhsExpression = cJSON_GetObjectItemCaseSensitive(jsonObject, "lhs_expression");
    cJSON* rhsExpression = cJSON_GetObjectItemCaseSensitive(jsonObject, "rhs_expression");
    cJSON* ifExpression = cJSON_GetObjectItemCaseSensitive(jsonObject, "if_expression");
    cJSON* bitWidth = cJSON_GetObjectItemCaseSensitive(jsonObject, "bit_width");

    if (!op || !cJSON_IsString(op) || !id || !cJSON_IsNumber(id) || !value || !cJSON_IsNumber(value) ||
        !bitWidth || !cJSON_IsNumber(bitWidth)) {
        fprintf(stderr, "Error reading expression fields\n");
        free(expression);
        return NULL;
    }

    strncpy(expression->op, op->valuestring, sizeof(expression->op));
    expression->id = id->valueint;
    expression->value = value->valueint;
    expression->bitWidth = bitWidth->valueint;

    expression->lhsExpression = buildExpressionFromJSONObject(lhsExpression);
    expression->rhsExpression = buildExpressionFromJSONObject(rhsExpression);
    expression->ifExpression = buildExpressionFromJSONObject(ifExpression);

    return expression;
}

// 从单个表达式构建 BDD
BDDNode buildBDDFromExpressions(DdManager* dd, cJSON* expressions, Variable* variables) {
    int numExpressions = cJSON_GetArraySize(expressions);

    if (numExpressions == 0) {
        return Cudd_ReadZero(dd);
    }

    // Assume expressions is an array of constraints
    BDDNode result = Cudd_ReadOne(dd);
    Cudd_Ref(result);

    for (int i = 0; i < numExpressions; ++i) {
        cJSON* constraint = cJSON_GetArrayItem(expressions, i);

        if (!constraint || !cJSON_IsObject(constraint)) {
            fprintf(stderr, "Error reading constraint at index %d\n", i);
            Cudd_RecursiveDeref(dd, result);
            return Cudd_ReadZero(dd);
        }

        cJSON* op = cJSON_GetObjectItemCaseSensitive(constraint, "op");

        if (!op || !cJSON_IsString(op)) {
            fprintf(stderr, "Error reading 'op' field in constraint at index %d\n", i);
            Cudd_RecursiveDeref(dd, result);
            return Cudd_ReadZero(dd);
        }

        Expression* expression = buildExpressionFromJSONObject(constraint);

        if (!expression) {
            fprintf(stderr, "Error building expression from constraint at index %d\n", i);
            Cudd_RecursiveDeref(dd, result);
            return Cudd_ReadZero(dd);
        }

        BDDNode expressionBDD = buildBDDFromExpression(dd, expression, variables);

        if (Cudd_IsConstant(expressionBDD)) {
            BDDNode temp = expressionBDD;
            expressionBDD = Cudd_bddIthVar(dd, expression->id);
            Cudd_Ref(expressionBDD);
            Cudd_RecursiveDeref(dd, temp);
        }

        BDDNode temp = Cudd_bddAnd(dd, result, expressionBDD);
        Cudd_Ref(temp);
        Cudd_RecursiveDeref(dd, result);
        Cudd_RecursiveDeref(dd, expressionBDD);
        result = temp;
    }

    return result;
}

// 主函数
int main() {
    const char* jsonText = "E:\\工程区\\test\\测试\\example_tests\\examples\\simple\\n5_m8_b8-16_t0.0311\\constraint.json"; // 你的 JSON 字符串

    int numVariables;
    Variable* variables = readVariablesFromJSON(jsonText, &numVariables);

    if (variables != NULL) {
        for (int i = 0; i < numVariables; ++i) {
            printf("Variable %d: id=%d, name=%s, signed=%d, bitWidth=%d\n",
                   i + 1, variables[i].id, variables[i].name, variables[i].isSigned, variables[i].bitWidth);
        }

        initializeCuddManager();

        cJSON* rootExpression = cJSON_Parse(jsonText);
        cJSON* constraints = cJSON_GetObjectItemCaseSensitive(rootExpression, "constraint_list");

        if (cJSON_IsArray(constraints)) {
            BDDNode bdd = buildBDDFromExpressions(ddManager, constraints, variables);

            int N = 5;
            // 定义并构建表达式
            cJSON* sampleExpressionJSON = cJSON_GetArrayItem(constraints, 0);  // 这里假设选择第一个约束作为例子
            Expression* expression = buildExpressionFromJSONObject(sampleExpressionJSON);

            // 调用 sampleFromBDD 函数
            sampleFromBDD(ddManager, bdd, variables, numVariables, N, expression);

            Cudd_RecursiveDeref(ddManager, bdd);
        }

        freeCuddManager();
        free(variables);
    }

    return 0;
}


