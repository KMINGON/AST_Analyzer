import json
from pycparser import c_generator
from generate_ast import from_json  # from_json 함수를 쓰기 위해 예제 파일 임포트

with open("ast.json", "r") as f:
    ast_json = f.read()

ast = from_json(ast_json)   # JSON 문자열을 AST 객체로 복원

generator = c_generator.CGenerator()    # AST 객체에서 C코드 복원
c_code = generator.visit(ast)

print("==================") # C코드 출력
print(c_code)

output_file = "ast.c"
with open(output_file, "w") as f:   # 복원된 C 파일 저장
    f.write(c_code)

print(f"\nFinished")
