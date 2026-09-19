#!/usr/bin/env python3
"""
ELM327 Emulator -- variante PORT SERIE (outil de test, section 24/27).
Lit/ecrit directement sur un chemin de peripherique (ex: un des deux
bouts d'une paire de pseudo-terminaux crees par socat), au lieu d'un
socket TCP. Meme table de reponses que l'emulateur TCP.
"""
import os
import sys

FAKE_RESPONSES = {
    "010C": "410C1AF8",
    "010D": "410D48",
    "0105": "410550",
}


def main(device_path):
    # Le pty est deja configure en mode brut par socat (option "pty,raw"
    # cote lien) -- pas besoin de le reconfigurer ici.
    fd = os.open(device_path, os.O_RDWR)
    print(f"[elm327-emulator-serial] listening on {device_path}", flush=True)

    buffer = ""
    while True:
        chunk = os.read(fd, 256)
        if not chunk:
            continue
        buffer += chunk.decode(errors="ignore")
        while "\r" in buffer:
            line, _, buffer = buffer.partition("\r")
            cmd = line.strip().upper()
            if not cmd:
                continue
            response = FAKE_RESPONSES.get(cmd, "NO DATA")
            os.write(fd, (response + ">").encode())


if __name__ == "__main__":
    main(sys.argv[1])
