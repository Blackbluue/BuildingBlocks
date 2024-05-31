#!/usr/bin/env python3
import os.path
from subprocess import DEVNULL, Popen, TimeoutExpired
from time import sleep

from buildingblocks.src.python.serialization import IO_Info

from .hero import TCP_PORT, Hero, ResponseCodes, Status


def test_send_hero():
    server = Popen(
        [f"{os.path.abspath('test_net_adventure')}"],
        stdout=DEVNULL,
        stderr=DEVNULL,
    )
    assert server.poll() is None, "Server failed to start"
    sleep(1)

    allogenes = Hero("Yelan", 60)
    allogenes.set_status(
        Status.POISONED | Status.BURNED | Status.PARALYZED | Status.BLINDED
    )
    packed_hero = allogenes.pack()

    try:
        info = IO_Info(None, TCP_PORT)
    except Exception as e:
        close_server(server)
        assert False, f"Failed to create IO_Info instance with error: {e}"

    try:
        info.write_pkt_data(
            packed_hero, len(packed_hero), ResponseCodes.RQU_STR_HRO
        )
    except Exception as e:
        close_server(server)
        assert False, f"Failed to write packet with error: {e}"

    try:
        packet = info.recv_pkt_data()
    except Exception as e:
        close_server(server)
        assert False, f"Failed to receive packet with error: {e}"

    if packet.data_len != 0:
        close_server(server)
        assert packet.data_len == 0, "Received packet data length is not 0"

    if packet.data != b"":
        close_server(server)
        assert packet.data == b"", "Received packet data is not empty"

    if packet.data_type != ResponseCodes.SVR_SUCCESS:
        close_server(server)
        assert (
            packet.data_type == ResponseCodes.SVR_SUCCESS
        ), f"Received packet data type is not {ResponseCodes.SVR_SUCCESS}"

    assert close_server(server), "Server failed to close properly"


def close_server(server: Popen):
    server.terminate()
    try:
        server.wait(5)
        return True
    except TimeoutExpired:
        server.kill()
        return False


if __name__ == "__main__":
    test_send_hero()
