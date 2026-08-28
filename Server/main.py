import os
import re
from threading import Lock

from fastapi import FastAPI, HTTPException
from pydantic import BaseModel, Field
import pymysql

from db import get_connection

app = FastAPI(title="L20260713_Day03 Auth Server")

_DEFAULT_GAME_PORT = 7777
_SERVER_ADDRESS_PATTERN = re.compile(
    r"^(?P<host>[A-Za-z0-9.-]+)(?::(?P<port>[0-9]{1,5}))?$"
)
_active_game_server = ""
_game_server_lock = Lock()


class AuthRequest(BaseModel):
    user_id: str = Field(min_length=1)
    passwd: str = Field(min_length=1)


class AuthResponse(BaseModel):
    result: bool
    message: str = ""
    idx: int = 0
    nickname: str = ""
    level: int = 0
    server_address: str = ""


class ServerRegisterRequest(BaseModel):
    server_address: str = Field(min_length=1, max_length=255)
    registration_token: str = ""


class ServerResponse(BaseModel):
    result: bool
    message: str = ""
    server_address: str = ""


def _normalize_server_address(value: str) -> str:
    address = value.strip()
    match = _SERVER_ADDRESS_PATTERN.fullmatch(address)
    if match is None:
        raise HTTPException(status_code=422, detail="올바른 게임 서버 주소가 아닙니다")

    port_text = match.group("port")
    port = int(port_text) if port_text else _DEFAULT_GAME_PORT
    if not 1 <= port <= 65535:
        raise HTTPException(status_code=422, detail="올바른 게임 서버 포트가 아닙니다")

    return f'{match.group("host")}:{port}'


def _get_active_game_server() -> str:
    with _game_server_lock:
        return _active_game_server


@app.post("/servers/register", response_model=ServerResponse)
def register_server(req: ServerRegisterRequest):
    expected_token = os.getenv("SERVER_REGISTRATION_TOKEN", "")
    if expected_token and req.registration_token != expected_token:
        raise HTTPException(status_code=401, detail="게임 서버 등록 토큰이 올바르지 않습니다")

    server_address = _normalize_server_address(req.server_address)
    global _active_game_server
    with _game_server_lock:
        _active_game_server = server_address

    return ServerResponse(result=True, server_address=server_address)


@app.get("/servers/active", response_model=ServerResponse)
def get_active_server():
    server_address = _get_active_game_server()
    if not server_address:
        return ServerResponse(result=False, message="등록된 게임 서버가 없습니다")
    return ServerResponse(result=True, server_address=server_address)


@app.post("/signup", response_model=AuthResponse)
def signup(req: AuthRequest):
    conn = get_connection()
    try:
        with conn.cursor() as cur:
            try:
                cur.execute(
                    "INSERT INTO member (user_id, passwd, nickname, level)"
                    " VALUES (%s, %s, %s, 1)",
                    (req.user_id, req.passwd, req.user_id),
                )
            except pymysql.err.IntegrityError:
                return AuthResponse(result=False, message="이미 존재하는 아이디입니다")

            new_idx = cur.lastrowid

        conn.commit()
    finally:
        conn.close()

    return AuthResponse(
        result=True, idx=new_idx, nickname=req.user_id, level=1
    )


@app.post("/login", response_model=AuthResponse)
def login(req: AuthRequest):
    conn = get_connection()
    try:
        with conn.cursor() as cur:
            cur.execute(
                "SELECT idx, nickname, level FROM member"
                " WHERE user_id = %s AND passwd = %s",
                (req.user_id, req.passwd),
            )
            row = cur.fetchone()
    finally:
        conn.close()

    if row is None:
        return AuthResponse(
            result=False, message="아이디 또는 비밀번호가 올바르지 않습니다"
        )

    return AuthResponse(
        result=True,
        idx=row["idx"],
        nickname=row["nickname"],
        level=row["level"],
        server_address=_get_active_game_server(),
    )
