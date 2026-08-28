# UnrealAssignment

언리얼 리슨/전용 서버가 시작될 때 FastAPI 웹 서버에 게임 서버 주소를 등록하고, 클라이언트가 로그인하면 등록 주소를 받아 자동 접속하는 과제입니다.

## 구현 기능

- `POST /servers/register`: 실행 중인 언리얼 게임 서버 주소 등록
- `GET /servers/active`: 현재 등록 주소 확인
- `POST /login`: 로그인 정보와 함께 `server_address` 반환
- 리슨 서버와 전용 서버의 `Lobby` 시작 시 자동 등록
- 로그인 성공 후 등록 서버가 있으면 `OpenLevel`로 자동 접속
- 선택형 등록 토큰과 명령행 설정 지원
- Python 단위 테스트 및 UE 5.8 에디터 빌드 검증

상세 구조, 설정, 실행 및 검증 방법은 [docs/게임서버_등록_접속_가이드.md](docs/게임서버_등록_접속_가이드.md)에 있습니다.

## 빠른 실행

```powershell
cd Server
py -m venv .venv
.\.venv\Scripts\pip.exe install -r requirements.txt
$env:DB_PASSWORD = "MySQL 비밀번호"
.\run.bat
```

언리얼 타이틀 화면의 `ServerIP`에는 FastAPI 서버 IP만 입력합니다. 로그인 응답으로 받은 실제 게임 서버 주소는 별도로 저장되며, 등록된 서버가 있으면 자동으로 접속합니다.

## 검증

```powershell
.\Server\.venv\Scripts\python.exe -m unittest discover -s Server\tests -v
```

현재 등록 서버 확인:

```powershell
Invoke-RestMethod http://127.0.0.1:8080/servers/active
```
