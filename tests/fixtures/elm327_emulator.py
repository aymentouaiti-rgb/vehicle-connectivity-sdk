#!/usr/bin/env python3
"""
ELM327 Emulator -- outil de TEST externe (section 24 et 27 de la spec).
Ce n'est pas un livrable du SDK : c'est un simulateur independant auquel
le VirtualConnector se connecte, exactement comme il se connecterait a un
vrai boitier ELM327 physique.
"""
import socket
import threading
import sys

FAKE_RESPONSES = {
    "010C": "410C1AF8",   # regime moteur
    "010D": "410D48",     # vitesse
    "0105": "410550",     # temperature liquide de refroidissement
}


def handle_client(conn):
    with conn:
        buffer = ""
        while True:
            data = conn.recv(1024)
            if not data:
                break
            buffer += data.decode(errors="ignore")
            while "\r" in buffer:
                line, _, buffer = buffer.partition("\r")
                cmd = line.strip().upper()
                if cmd == "DROPLINK":
                    return
                response = FAKE_RESPONSES.get(cmd, "NO DATA")
                conn.sendall((response + ">").encode())


def main(port):
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind(("127.0.0.1", port))
    server.listen(5)
    print(f"[elm327-emulator] listening on 127.0.0.1:{port}", flush=True)
    while True:
        conn, _ = server.accept()
        threading.Thread(target=handle_client, args=(conn,), daemon=True).start()


if __name__ == "__main__":
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 35000
    main(port)
