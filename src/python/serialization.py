"""Contain class for I/O operations."""
from __future__ import annotations

import socket
import sys
from struct import Struct, pack
from typing import TYPE_CHECKING, NamedTuple

if TYPE_CHECKING:
    from typing import Union


# header length, data length, data type as unsigned 32-bit int in network byte order
HEADER_PACKER = Struct("!3I")


class Packet(NamedTuple):
    """Contain the packet header and data.

    Attributes:
        data_len (int): The length of the data.
        data_type (int): The user-defined data type flag.
        data (bytes): The data.
    """

    data_len: int
    data_type: int
    data: bytes

    @classmethod
    def recv_pkt_data(cls, sock: socket.socket) -> Packet:
        """Receive a packet from the socket.

        Args:
            sock (socket.socket): The socket to receive the packet from.

        Returns:
            Packet: The packet received from the socket.
        """
        header = sock.recv(HEADER_PACKER.size)
        _, data_len, data_type = HEADER_PACKER.unpack(header)
        data = sock.recv(data_len)
        return Packet(data_len, data_type, data)

    def write_pkt_data(self, sock: socket.socket) -> None:
        """Write the packet to the socket.

        Args:
            sock (socket.socket): The socket to write the packet to.
        """
        sock.sendall(
            HEADER_PACKER.pack(
                HEADER_PACKER.size, self.data_len, self.data_type
            )
        )
        sock.sendall(pack(f"{self.data_len}s", self.data))


class IO_Info:
    """Contain class for I/O operations."""

    _header_packer = Struct("!3I")

    def __init__(
        self, host: Union[bytes, str, None], port: Union[bytes, str, int, None]
    ) -> None:
        """Create a new instance of IO_Info.

        Args:
            host (bytes | str | None): The host to connect to. Use None for localhost.
            port (bytes | str | int | None): The port to connect to.

        Raises:
            OSError: If the socket could not be opened and connected to.
        """
        self._connect_sock(host, port)

    def __del__(self) -> None:
        if self._sock is not None:
            self._sock.close()
            self._sock = None

    def _connect_sock(
        self, host: Union[bytes, str, None], port: Union[bytes, str, int, None]
    ) -> None:
        """Connect to the server.

        Args:
            host (bytes | str | None): The host to connect to. Use None for localhost.
            port (bytes | str | int | None): The port to connect to.

        Raises:
            err: If the socket could not be opened and connected to.
        """
        sock_ok = False
        for res in socket.getaddrinfo(
            host, port, socket.AF_UNSPEC, socket.SOCK_STREAM
        ):
            addr_fam, socktype, proto, _, sock_addr = res
            try:
                self._sock = socket.socket(addr_fam, socktype, proto)
            except OSError as exc:
                err = exc
                continue
            try:
                self._sock.connect(sock_addr)
            except OSError as exc:
                self._sock.close()
                err = exc
                continue
            sock_ok = True
            break
        if not sock_ok:
            print("Could not open or connect to socket", file=sys.stderr)
            if err:
                raise err

    def write_pkt_data(self, data: bytes, len: int, data_type: int) -> None:
        """Write packet data to the socket.

        Args:
            data (bytes): The data to write.
            len (int): The length of the data.
            data_type (int): The user-defined data type flag.

        Raises:
            OSError: If the socket is not open.
        """
        if self._sock is None:
            raise OSError("socket is not open")
        Packet(len, data_type, data).write_pkt_data(self._sock)

