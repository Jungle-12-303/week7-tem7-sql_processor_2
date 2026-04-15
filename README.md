# Simple SQL Processor with B+ Tree Index

메모리 기반 `users` 테이블에 대해 아주 작은 SQL 처리기와 B+ 트리 인덱스를 구현한 MVP 프로젝트입니다. 목표는 복잡한 DBMS 기능보다 `동작하는 인덱스`, `설명 가능한 split`, `ID 검색 성능 비교`에 집중하는 것입니다.

## 1. B+ 트리 구조 설명

- 내부 노드: 검색 경로를 안내하는 key와 child pointer만 저장
- 리프 노드: 실제 `(key, value)`를 저장
- value는 `Record *`
- 리프 노드끼리는 `next` 포인터로 연결

```text
                    [30 | 70]
                  /     |      \
                 /      |       \
        [10 | 20]  [30 | 50]  [70 | 90]
            |           |          |
         record*     record*    record*

리프 연결:
[10 | 20] -> [30 | 50] -> [70 | 90] -> NULL
```

이 프로젝트에서는 `order = 4`를 사용합니다.

- 한 노드의 최대 key 수: `3`
- key가 `4개`가 되면 split 발생

## 2. 구현 범위

### 포함
- 메모리 기반 B+ 트리
- `insert(key=id, value=record pointer)`
- `search(key)`
- leaf node 연결 리스트
- leaf split / internal split
- 고정 스키마 `users(id, name, age)`
- `INSERT INTO users VALUES ('Alice', 20);`
- `SELECT * FROM users WHERE id = 1;`
- `SELECT * FROM users WHERE name = 'Alice';`
- `SELECT * FROM users WHERE age = 20;`
- `id` 자동 증가
- `id` 검색은 B+ 트리 사용
- `name`, `age` 검색은 선형 탐색 사용

### 제외
- DELETE, UPDATE
- range scan
- 디스크 저장
- JOIN, GROUP BY
- 범용 SQL 파서

## 3. 파일 구성

- `bptree.h`, `bptree.c`: B+ 트리 핵심 구조와 insert/search/split
- `table.h`, `table.c`: 레코드 저장과 인덱스 연동
- `sql.h`, `sql.c`: 고정 패턴 SQL 파서/실행기
- `main.c`: 데모용 REPL
- `perf_test.c`: 100만 건 insert 후 검색 속도 비교
- `unit_test.c`: assert 기반 단위 테스트
- `Makefile`: 빌드 스크립트

## 4. 빌드 및 실행 방법

```bash
make
./main
./unit_test
./perf_test
```

## 5. SQL 예시

```sql
INSERT INTO users VALUES ('Alice', 20);
INSERT INTO users VALUES ('Bob', 30);

SELECT * FROM users WHERE id = 1;
SELECT * FROM users WHERE name = 'Bob';
SELECT * FROM users WHERE age = 20;
```

예상 출력:

```text
Inserted row with id = 1
Inserted row with id = 2
id=1, name='Alice', age=20
id=2, name='Bob', age=30
id=1, name='Alice', age=20
```

## 6. 핵심 로직 설명: split 과정

### leaf split

리프 노드가 가득 찬 상태에서 새 key가 들어오면, 기존 key와 새 key를 정렬된 임시 배열에 모은 뒤 `2 | 2`로 나눕니다.

```text
기존 leaf: [10 | 20 | 30]
40 삽입:   [10 | 20 | 30 | 40]

split 후:
왼쪽 leaf  = [10 | 20]
오른쪽 leaf = [30 | 40]

부모로 올리는 key = 30
```

### internal split

내부 노드가 overflow 되면 가운데 separator key를 부모로 올리고, 왼쪽/오른쪽 child 집합을 분리합니다.

```text
임시 key: [10 | 20 | 30 | 40]

왼쪽 internal  = [10 | 20]
부모로 승격    = 30
오른쪽 internal = [40]
```

핵심 아이디어는 다음과 같습니다.

- 실제 데이터 포인터는 리프에만 저장
- 부모는 오른쪽 서브트리의 시작 key를 관리
- split이 루트까지 전파되면 새 루트가 생성됨

## 7. 성능 테스트 방법

- `1,000,000`건 순차 insert
- `10,000`건 ID 검색 샘플
- 비교 대상
  - B+ 트리 기반 `table_find_by_id`
  - 선형 탐색 기반 `table_scan_by_id`
- 시간 측정은 `gettimeofday()` 사용

| Inserted Rows | B+ Tree Search Time (ms) | Linear Search Time (ms) | Speedup |
| --- | ---: | ---: | ---: |
| 1,000,000 | 10.104 | 13119.946 | 1298.49x |

## 8. 발표용 핵심 포인트

- 내부 노드는 "길 안내판", 리프 노드는 "실제 데이터 보관소"입니다.
- `id`는 B+ 트리 인덱스를 쓰므로 빠르게 내려가서 찾습니다.
- `name`, `age`는 인덱스가 없어서 끝까지 선형 탐색합니다.
- 핵심 구현 포인트는 `leaf split`, `internal split`, `새 루트 생성`입니다.
