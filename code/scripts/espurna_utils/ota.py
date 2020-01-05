import os


def app_inject_ota_board(projenv):
    board = os.environ.get("ESPURNA_BOARD")
    if board:
        projenv.Append(CPPDEFINES=[board])
