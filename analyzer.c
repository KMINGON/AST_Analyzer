#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "json_c.c"

const char* json_get_key(json_value obj, int index) {
    if (obj.type != JSON_OBJECT) return NULL;
    json_object *jo = (json_object *)obj.value;
    if (index < 0 || index > jo->last_index) return NULL;
    return jo->keys[index];
}

json_value read_ast_json(const char *file){ //ast.json 파일 읽기
    FILE *fp = fopen(file, "r");
    if (!fp) {
        fprintf(stderr, "파일 열기 실패: %s\n", file);
        exit(1);
    }
    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    rewind(fp);

    char *buffer = (char *)malloc(len + 1);
    fread(buffer, 1, len, fp);
    buffer[len] = '\0';
    fclose(fp);

    json_value result = json_create(buffer);
    free(buffer);
    return result;
}

void extract_return_type(json_value func_def) { //함수의 리턴 값 추출 
    json_value decl = json_get(func_def, "decl");
    const char *func_name = json_get_string(decl, "name");
    json_value func_type = json_get(decl, "type");
    json_value return_type = json_get(func_type, "type");   //AST 파일의 리턴 값 위치의 기본 구조까지 접근

    while (json_get_string(return_type, "_nodetype") == NULL || strcmp(json_get_string(return_type, "_nodetype"), "IdentifierType") != 0) { //IdentifierType 노드가 나올 때 까지 반복
        return_type = json_get(return_type, "type");
    }
    json_value names = json_get(return_type, "names");
    const char *ret_type = json_get_string(names, 0);
    printf("name: %s\nReturn Type: %s\n", func_name, ret_type);
}



void extract_params(json_value func_def) {  //파라미터 값 추출
    json_value decl = json_get(func_def, "decl");
    json_value func_type = json_get(decl, "type");
    json_value args = json_get(func_type, "args"); 
    if (json_get_type(args) != JSON_OBJECT) {   //파라미터가 없는 함수이면 함수 리턴
        printf("No parameters\n");
        return;
    }
    json_value params = json_get(args, "params");   //AST 파일의 파라미터 값 위치의 기본 구조까지 접근
    if (json_get_type(params) != JSON_ARRAY) {  //파라미터가 없는 함수이면 함수 리턴
        printf("No parameters\n");
        return;
    }

    int len = json_len(params);
    printf("Params: ");
    for (int i = 0; i < len; i++) {
        json_value param = json_get(params, i);
        const char *name = json_get_string(param, "name");

        json_value type_node = json_get(param, "type");
        while (json_get_type(type_node) != JSON_UNDEFINED && strcmp(json_get_string(type_node, "_nodetype"), "IdentifierType") != 0) {  //IdentifierType 노드가 나올 때 까지 접근
            type_node = json_get(type_node, "type");
        }
        json_value names = json_get(type_node, "names");
        const char *param_type = json_get_string(names, 0);

        if (param_type && name) //값이 정상적으로 추출 된 경우에 출력
            printf("(%s %s)%s", param_type, name, i < len - 1 ? ", " : "\n");   //마지막 파마미터일 경우 개행문자 출력
    }
}

int count_ifs(json_value node) {
    if (json_get_type(node) != JSON_OBJECT && json_get_type(node) != JSON_ARRAY) return 0;  //잘못된 값일 경우 종료

    int is_if = 0;
    int total = 0;

    if (json_get_type(node) == JSON_ARRAY) {    //어레이 값일 경우 각 노드를 돌면서 함수 재귀 호출
        int size = json_len(node);
        for (int i = 0; i < size; i++) {    //어레이의 길이만큼 반복
            json_value child = json_get(node, i);   //어레이에 존재하는 노드 추출
            if (json_get_type(child) == JSON_OBJECT || json_get_type(child) == JSON_ARRAY) {
                total += count_ifs(child);  //추출한 노드에 대하여 다시 재귀 호출
            }
        }
    } else if (json_get_type(node) == JSON_OBJECT) {    //오브젝트 값일 경우 각 키, 값 탐색
        const char *nodetype = json_get_string(node, "_nodetype");
        is_if = (nodetype && strcmp(nodetype, "If") == 0) ? 1 : 0;  //노드 타입 값이 If일 경우 체크
        int size = json_len(node);
        for (int i = 0; i < size; i++) {
            const char *key = json_get_key(node, i);    //해당 노드의 키 값을 받아옴
            if (key != NULL) {
                json_value child = json_get(node, key); //받아온 키 값을 이용해 노드에 접근
                if (json_get_type(child) == JSON_OBJECT || json_get_type(child) == JSON_ARRAY) {
                    total += count_ifs(child);  //접근한 노드에 대하여 다시 재귀 호출
                }
            }
        }
    }
    total += is_if;
    return total;
}

int extract_func(json_value ext){ //함수 추출
    int cnt = 0;
    int size = json_len(ext);
    for(int i = 0; i < size; i++){
        json_value node = json_get(ext, i);
        const char *ntype = json_get_string(node, "_nodetype"); //_nodetype 키에 해당하는 값들 추출
        if (ntype && strcmp(ntype, "FuncDef") == 0) {   //추출한 타입이 함수인지 확인
            cnt++;  //함수 개수 카운팅
            printf("Funcion %d\n", cnt);
            extract_return_type(node);  //해당 함수의 리턴 값 추출
            extract_params(node);   //해당 함수의 파라미터 값 추출
            
            json_value body = json_get(node, "body");
            int ifs = count_ifs(body);
            printf("If count: %d\n", ifs);
            printf("===========\n");
        }
    }
    return cnt;
}

int main(){
    json_value ast = read_ast_json("ast.json");
    json_value ext = json_get(ast, "ext");
    printf("total function count : %d\n", extract_func(ext));
    return 0;
}