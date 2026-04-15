# B+Tree 함수 학습 워크북

이 문서는 이 프로젝트의 시스템 다이어그램 흐름대로 함수를 따라가며 공부하기 위한 작업표입니다.
목표는 `main -> sql -> table -> bptree` 호출 흐름을 실제 코드 기준으로 끝까지 이해하는 것입니다.

이 문서는 혼자 읽는 요약본이 아니라, 저와 같이 읽기 위한 진행표로 설계했습니다.
각 단계마다 아래 4가지를 확인하세요.

1. 지금 단계의 목표
2. 실제 호출되는 함수 순서
3. 코드에서 꼭 봐야 하는 포인트
4. 저에게 바로 보낼 다음 메시지

## 사용 방법

아래 단계는 순서대로 보는 것을 기준으로 작성했습니다.
한 단계를 읽고 나면 문서에 적힌 "다음에 나에게 보낼 메시지"를 그대로 보내면 됩니다.
그러면 제가 그 단계의 함수들을 코드 기준으로 같이 읽어드리겠습니다.

권장 공부 방식은 다음과 같습니다.

1. 문서에서 해당 단계의 목적을 먼저 읽는다.
2. 적힌 함수 순서를 눈으로 따라간다.
3. 실제 소스 파일을 연다.
4. 함수마다 "누가 호출했고, 누구를 호출하는지"를 적는다.
5. 단계 끝의 체크 질문에 스스로 답한다.
6. 다음 메시지를 보내고 저와 이어서 읽는다.

## 전체 지도

이 프로젝트의 큰 흐름은 아래 순서입니다.

1. `main.c`
2. `sql.c`
3. `table.c`
4. `bptree.c`
5. `unit_test.c`
6. `perf_test.c`, `condition_perf_test.c`

핵심 공부 축은 3개입니다.

1. `INSERT`가 어떻게 레코드 저장과 B+Tree 삽입으로 이어지는가
2. `SELECT WHERE id ...`가 어떻게 인덱스를 타는가
3. `leaf split`, `internal split`, `leaf next`가 왜 필요한가

---

## 0단계. 구조체부터 잡기

### 목표

함수 읽기 전에 이 프로젝트의 데이터가 메모리에서 어떤 모양으로 연결되는지 이해합니다.

### 먼저 볼 구조체

- `Record`
- `Table`
- `BPTreeNode`
- `BPTree`
- `SQLResult`
- `TableComparison`
- `SQLStatus`, `SQLAction`

### 볼 파일

- `table.h`
- `bptree.h`
- `sql.h`

### 꼭 볼 포인트

- `Record`는 실제 한 행 데이터다
- `Table`은 전체 행 배열과 PK 인덱스를 같이 가진다
- `BPTreeNode`는 leaf와 internal을 공통 구조로 표현한다
- leaf에서는 `values[]`가 실제 `Record *`를 들고, internal에서는 `children[]`가 중요하다
- `next`는 leaf끼리 옆으로 연결하는 포인터다
- `SQLResult`는 실행 결과를 `main()`으로 돌려주기 위한 포장 구조체다

### 구조 관계 한 줄 요약

`Table`이 `rows`와 `pk_index`를 갖고, `pk_index`는 `BPTree`이며, 그 안의 leaf node `values[]`가 결국 `Record *`를 가리킵니다.

### 스스로 답할 질문

1. 실제 사용자 데이터는 어느 구조체에 들어있는가
2. `Table`은 왜 배열과 B+Tree를 둘 다 갖는가
3. `BPTreeNode`에서 leaf와 internal은 어떤 필드를 중심으로 다르게 쓰는가
4. `SQLResult`는 왜 `record`와 `records`를 둘 다 가지는가

### 다음에 나에게 보낼 메시지

`0단계 같이 읽자. 구조체 관계부터 설명해줘`

---

## 1단계. 프로그램 진입점 잡기

### 목표

사용자 입력 한 줄이 어디로 들어가고, 어디서 `INSERT`와 `SELECT`로 갈라지는지 이해합니다.

### 호출 순서

`main()` -> `sql_execute()`

### 볼 파일과 함수

- `main.c`
- `sql.c`
- `main()`
- `sql_execute()`

### 꼭 볼 포인트

- `Table *table = table_create();` 로 테이블이 언제 만들어지는가
- `fgets()` 로 받은 문자열이 어디로 전달되는가
- `sql_execute()`가 `EXIT/QUIT`, `INSERT`, `SELECT`를 어떤 순서로 판별하는가
- `SQLResult`가 왜 필요한가

### 스스로 답할 질문

1. 사용자 입력은 어디에서 처음 처리되는가
2. `INSERT`와 `SELECT`는 어느 함수에서 갈라지는가
3. `sql_execute()`가 바로 `bptree.c`로 가지 않는 이유는 무엇인가

### 다음에 나에게 보낼 메시지

`1단계 같이 읽자. main()과 sql_execute()부터 설명해줘`

---

## 2단계. INSERT 파서 읽기

### 목표

`INSERT INTO users VALUES ('Alice', 20);` 가 어떻게 파싱되는지 이해합니다.

### 호출 순서

`sql_execute()` -> `sql_execute_insert()` -> 파싱 보조 함수들

### 볼 파일과 함수

- `sql.c`
- `sql_execute_insert()`
- `sql_match_keyword()`
- `sql_parse_identifier()`
- `sql_parse_string()`
- `sql_parse_int()`
- `sql_match_statement_end()`

### 꼭 볼 포인트

- 이 프로젝트는 범용 SQL 파서가 아니라 고정 문법 파서라는 점
- `users` 테이블만 허용되는 위치
- `name`, `age`를 문자열과 정수로 분리해서 읽는 방식
- 파싱이 끝난 뒤 실제 데이터 저장은 `table_insert()`로 넘긴다는 점

### 스스로 답할 질문

1. `INSERT` 문법이 틀리면 어디서 실패하는가
2. `id`는 왜 입력받지 않는가
3. 파싱 성공 후 실제 저장 책임은 누구에게 넘어가는가

### 다음에 나에게 보낼 메시지

`2단계 같이 읽자. sql_execute_insert()와 파싱 함수들을 코드 흐름대로 설명해줘`

---

## 3단계. 테이블 삽입 경로 읽기

### 목표

레코드가 메모리에 저장되고 자동 증가 ID가 붙는 위치를 이해합니다.

### 호출 순서

`sql_execute_insert()` -> `table_insert()` -> `table_ensure_capacity()` -> `bptree_insert()`

### 볼 파일과 함수

- `table.c`
- `table_insert()`
- `table_ensure_capacity()`

### 꼭 볼 포인트

- `rows` 동적 배열이 커지는 방식
- `record->id = table->next_id++;` 의 의미
- `rows[size] = record` 와 `size++` 의 순서
- 테이블 저장과 인덱스 저장이 둘 다 성공해야 삽입 성공으로 본다는 점

### 스스로 답할 질문

1. 자동 증가 ID는 어디서 부여되는가
2. 레코드 포인터는 어디에 저장되는가
3. 인덱스 삽입이 실패하면 왜 `NULL`을 반환하는가

### 다음에 나에게 보낼 메시지

`3단계 같이 읽자. table_insert() 내부 흐름과 데이터 상태 변화를 설명해줘`

---

## 4단계. B+Tree 삽입 엔트리 읽기

### 목표

트리 삽입의 시작점과 "일반 삽입 vs split" 분기를 이해합니다.

### 호출 순서

`table_insert()` -> `bptree_insert()` -> `bptree_find_leaf()` -> 일반 삽입 또는 split

### 볼 파일과 함수

- `bptree.h`
- `bptree.c`
- `bptree_insert()`
- `bptree_find_leaf()`
- `bptree_insert_into_leaf()`

### 꼭 볼 포인트

- `BPTREE_ORDER = 4`
- 최대 key 수가 `3`이라는 뜻
- 루트가 비어 있을 때 첫 리프를 만드는 코드
- 중복 key를 막는 코드
- 리프가 안 찼을 때는 정렬 상태를 유지하며 바로 삽입한다는 점

### 스스로 답할 질문

1. 빈 트리의 첫 삽입은 어떤 모양인가
2. 어떤 조건에서 split이 일어나는가
3. `bptree_find_leaf()`는 내부 노드에서 child를 어떻게 고르는가

### 다음에 나에게 보낼 메시지

`4단계 같이 읽자. bptree_insert()에서 일반 삽입까지 설명해줘`

---

## 5단계. Leaf Split 읽기

### 목표

리프 노드가 꽉 찼을 때 어떤 식으로 둘로 나뉘는지 이해합니다.

### 호출 순서

`bptree_insert()` -> `bptree_split_leaf_and_insert()` -> `bptree_insert_into_parent()`

### 볼 파일과 함수

- `bptree.c`
- `bptree_split_leaf_and_insert()`
- `bptree_insert_into_parent()`

### 꼭 볼 포인트

- `temp_keys`, `temp_values`에 기존 값과 새 값을 합치는 방식
- `left_count = BPTREE_ORDER / 2`
- 왼쪽 리프와 오른쪽 리프에 값이 어떻게 나뉘는가
- 부모로 올리는 값이 왜 `right_leaf->keys[0]` 인가
- `right_leaf->next = leaf->next; leaf->next = right_leaf;` 가 왜 중요한가

### 스스로 답할 질문

1. split 전과 후의 리프 key 분포는 어떻게 달라지는가
2. 왜 실제 데이터 포인터는 리프에만 남는가
3. `next` 포인터를 연결하지 않으면 이후 어떤 기능이 불편해지는가

### 다음에 나에게 보낼 메시지

`5단계 같이 읽자. leaf split을 예시 key 1,2,3,4 기준으로 설명해줘`

---

## 6단계. Parent 반영과 Internal Split 읽기

### 목표

리프에서 발생한 split이 부모로 전파되고, 필요하면 새 루트가 생기는 과정을 이해합니다.

### 호출 순서

`bptree_split_leaf_and_insert()` -> `bptree_insert_into_parent()` -> `bptree_insert_into_internal()` 또는 `bptree_split_internal_and_insert()` -> `bptree_create_new_root()`

### 볼 파일과 함수

- `bptree.c`
- `bptree_insert_into_parent()`
- `bptree_insert_into_internal()`
- `bptree_split_internal_and_insert()`
- `bptree_create_new_root()`

### 꼭 볼 포인트

- 부모가 없으면 새 루트를 만든다는 점
- 부모에 여유가 있으면 단순 삽입만 한다는 점
- 부모도 꽉 찼으면 내부 노드 split이 재귀적으로 이어진다는 점
- `child->parent` 재설정 코드

### 스스로 답할 질문

1. 새 루트는 어떤 경우에만 만들어지는가
2. internal split에서는 어떤 key가 승격되는가
3. 왜 split은 한 번으로 끝나지 않고 위로 전파될 수 있는가

### 다음에 나에게 보낼 메시지

`6단계 같이 읽자. internal split과 새 루트 생성까지 이어서 설명해줘`

---

## 7단계. SELECT 진입점 읽기

### 목표

`SELECT * FROM users WHERE ...` 가 어떤 기준으로 `id`, `name`, `age` 경로로 갈라지는지 이해합니다.

### 호출 순서

`main()` -> `sql_execute()` -> `sql_execute_select()`

### 볼 파일과 함수

- `sql.c`
- `sql_execute_select()`
- `sql_parse_comparison()`

### 꼭 볼 포인트

- `WHERE`가 없으면 전체 조회로 간다는 점
- `column == id`, `column == name`, `column == age` 분기
- 비교 연산자 `=`, `<`, `<=`, `>`, `>=` 해석 방식
- 여기서 검색 전략이 결정된다는 점

### 스스로 답할 질문

1. 인덱스를 쓸지 말지는 어디서 결정되는가
2. 왜 `name`은 `=`만 허용되는가
3. `SELECT * FROM users;` 는 어떤 함수로 내려가는가

### 다음에 나에게 보낼 메시지

`7단계 같이 읽자. sql_execute_select()가 조회 경로를 어떻게 고르는지 설명해줘`

---

## 8단계. ID 단건 조회 읽기

### 목표

`WHERE id = 3` 같은 단건 조회가 실제로 B+Tree 검색을 타는 경로를 이해합니다.

### 호출 순서

`sql_execute_select()` -> `table_find_by_id_condition()` -> `table_find_by_id()` -> `bptree_search()` -> `bptree_find_leaf()`

### 볼 파일과 함수

- `table.c`
- `bptree.c`
- `table_find_by_id_condition()`
- `table_find_by_id()`
- `bptree_search()`
- `bptree_find_leaf()`

### 꼭 볼 포인트

- `comparison == TABLE_COMPARISON_EQ` 분기
- leaf를 찾은 뒤 leaf 내부 key 배열을 다시 확인하는 이유
- 검색 결과가 `Record *` 하나로 돌아오는 구조

### 스스로 답할 질문

1. `WHERE id = ?` 에서는 왜 range용 로직이 필요 없는가
2. 내부 노드는 실제 value를 가지는가
3. 최종 비교는 leaf 내부에서 왜 한 번 더 하는가

### 다음에 나에게 보낼 메시지

`8단계 같이 읽자. WHERE id = 값 이 B+Tree를 타는 흐름을 설명해줘`

---

## 9단계. ID 범위 조회와 Leaf Link 읽기

### 목표

`WHERE id >= 10` 같은 조건에서 `next` 포인터가 왜 필요한지 이해합니다.

### 호출 순서

`sql_execute_select()` -> `table_find_by_id_condition()` -> `table_find_id_leaf()` 또는 `table_find_leftmost_leaf()` -> `table_collect_leaf_chain()`

### 볼 파일과 함수

- `table.c`
- `table_find_by_id_condition()`
- `table_find_id_leaf()`
- `table_find_leftmost_leaf()`
- `table_collect_leaf_chain()`

### 꼭 볼 포인트

- `GT`, `GE`, `LT`, `LE` 가 서로 다른 방식으로 시작 위치를 잡는 점
- `id >= x` 는 시작 리프를 찾고 `next`로 끝까지 따라간다는 점
- `id < x` 는 맨 왼쪽 리프부터 순서대로 읽다가 멈춘다는 점

### 스스로 답할 질문

1. `next` 포인터는 어떤 쿼리에서 진짜 효과를 발휘하는가
2. 왜 `>=` 는 시작 leaf를 찾고, `<` 는 맨 왼쪽부터 읽는가
3. 범위 조회에서 B+Tree가 선형 탐색보다 유리한 이유는 무엇인가

### 다음에 나에게 보낼 메시지

`9단계 같이 읽자. WHERE id >= 값 과 leaf next 연결을 설명해줘`

---

## 10단계. name/age 선형 탐색과 비교하기

### 목표

인덱스를 쓰는 `id` 경로와 인덱스를 안 쓰는 `name`, `age` 경로를 비교합니다.

### 호출 순서

`sql_execute_select()` -> `table_find_by_name_matches()` 또는 `table_find_by_age_condition()`

### 볼 파일과 함수

- `sql.c`
- `table.c`
- `table_find_by_name_matches()`
- `table_find_by_age_condition()`

### 꼭 볼 포인트

- `rows[0]`부터 끝까지 순회하는 구조
- `id` 경로와 달리 `bptree.c`를 전혀 호출하지 않는다는 점
- 왜 성능 차이가 나는지 코드 수준에서 설명할 수 있어야 한다는 점

### 스스로 답할 질문

1. `name`, `age`는 왜 선형 탐색인가
2. `id`와 비교했을 때 호출 함수 수는 어떻게 다른가
3. 발표에서 성능 차이를 어떤 코드 근거로 설명할 수 있는가

### 다음에 나에게 보낼 메시지

`10단계 같이 읽자. name/age 선형 탐색과 id 인덱스 경로를 비교해줘`

---

## 11단계. 단위 테스트로 이해 고정하기

### 목표

핵심 동작이 어떤 테스트로 검증되는지 보고, 공부한 내용을 코드로 다시 확인합니다.

### 호출 순서

테스트 함수 -> 대상 함수 호출 -> `assert`

### 볼 파일과 함수

- `unit_test.c`
- `test_leaf_split_search()`
- `test_internal_split_new_root()`
- `test_leaf_next_link()`
- `test_table_find_by_id()`

### 꼭 볼 포인트

- leaf split 후에도 검색이 유지되는지
- internal split 후 트리 높이가 올라가는지
- 리프끼리 `next` 연결이 유지되는지
- 테이블과 인덱스 연동이 실제로 검증되는지

### 스스로 답할 질문

1. 내 공부가 맞는지 가장 먼저 확인할 테스트는 무엇인가
2. 어떤 테스트가 leaf split을 증명하는가
3. 어떤 테스트가 새 루트 생성을 간접적으로 보여주는가

### 다음에 나에게 보낼 메시지

`11단계 같이 읽자. unit_test.c로 핵심 동작 검증을 같이 보자`

---

## 12단계. 성능 테스트와 발표 연결

### 목표

코드 구조 이해를 성능 설명과 발표 포인트로 연결합니다.

### 호출 순서

성능 테스트 코드 -> 테이블 조회 함수 -> 인덱스 경로 또는 선형 탐색 경로

### 볼 파일과 함수

- `perf_test.c`
- `condition_perf_test.c`
- `table_find_by_id()`
- `table_scan_by_id()`
- `table_find_by_id_condition()`
- `table_find_by_age_condition()`

### 꼭 볼 포인트

- `id`는 인덱스 기반
- `age`는 선형 탐색 기반
- 반환 건수와 검색 전략이 어떻게 다른지
- 왜 이 결과가 B+Tree 학습의 마무리로 적절한지

### 스스로 답할 질문

1. 성능 테스트는 어떤 함수끼리 비교하는가
2. `WHERE id >= ...` 와 `WHERE age >= ...` 의 차이는 코드 어디에서 생기는가
3. 발표에서 "왜 빠른가"를 어떤 함수 이름으로 설명할 수 있는가

### 다음에 나에게 보낼 메시지

`12단계 같이 읽자. 성능 테스트 코드와 발표 포인트를 연결해서 설명해줘`

---

## 빠른 복습용 최소 루트

시간이 없으면 아래 함수들만 순서대로 보세요.

1. `main()`
2. `sql_execute()`
3. `sql_execute_insert()`
4. `table_insert()`
5. `bptree_insert()`
6. `bptree_split_leaf_and_insert()`
7. `bptree_split_internal_and_insert()`
8. `sql_execute_select()`
9. `table_find_by_id_condition()`
10. `bptree_search()`
11. `table_collect_leaf_chain()`
12. `unit_test.c`의 split 관련 테스트 3개

---

## 단계 진행 기록

- [ ] 1단계 완료
- [ ] 2단계 완료
- [ ] 3단계 완료
- [ ] 4단계 완료
- [ ] 5단계 완료
- [ ] 6단계 완료
- [ ] 7단계 완료
- [ ] 8단계 완료
- [ ] 9단계 완료
- [ ] 10단계 완료
- [ ] 11단계 완료
- [ ] 12단계 완료

---

## 지금 바로 시작할 추천 메시지

`0단계 같이 읽자. 구조체 관계부터 설명해줘`
