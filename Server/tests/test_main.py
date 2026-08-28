import os
import sys
import unittest
from pathlib import Path
from unittest.mock import patch

SERVER_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SERVER_DIR))

import main


class _FakeCursor:
    def __init__(self, row):
        self.row = row

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        return False

    def execute(self, _query, _params):
        return None

    def fetchone(self):
        return self.row


class _FakeConnection:
    def __init__(self, row):
        self.row = row
        self.closed = False

    def cursor(self):
        return _FakeCursor(self.row)

    def close(self):
        self.closed = True


class GameServerApiTests(unittest.TestCase):
    def setUp(self):
        main._active_game_server = ""
        os.environ.pop("SERVER_REGISTRATION_TOKEN", None)

    def test_register_server_adds_default_port(self):
        response = main.register_server(
            main.ServerRegisterRequest(server_address="game.example.com")
        )

        self.assertTrue(response.result)
        self.assertEqual(response.server_address, "game.example.com:7777")
        self.assertEqual(main.get_active_server().server_address, response.server_address)

    def test_login_returns_registered_server_address(self):
        main.register_server(
            main.ServerRegisterRequest(server_address="127.0.0.1:7788")
        )
        connection = _FakeConnection(
            {"idx": 7, "nickname": "tester", "level": 3}
        )

        with patch.object(main, "get_connection", return_value=connection):
            response = main.login(
                main.AuthRequest(user_id="tester", passwd="password")
            )

        self.assertTrue(response.result)
        self.assertEqual(response.server_address, "127.0.0.1:7788")
        self.assertTrue(connection.closed)

    def test_registration_token_is_checked_when_configured(self):
        os.environ["SERVER_REGISTRATION_TOKEN"] = "secret"

        with self.assertRaises(main.HTTPException) as context:
            main.register_server(
                main.ServerRegisterRequest(
                    server_address="127.0.0.1", registration_token="wrong"
                )
            )

        self.assertEqual(context.exception.status_code, 401)


if __name__ == "__main__":
    unittest.main()
