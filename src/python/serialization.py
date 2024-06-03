"""Contain class for I/O operations."""
from __future__ import annotations

import socket
from struct import Struct, pack
from typing import TYPE_CHECKING, NamedTuple

if TYPE_CHECKING:
    from typing import Optional, Union


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

    def __str__(self) -> str:
        return f"Packet of {self.data_len} bytes with type {self.data_type}"


class IOInfo:
    """Contain class for I/O operations."""

    _header_packer = Struct("!3I")

    def __init__(self, host: Union[str, None], port: int) -> None:
        """Create a new instance of IOInfo.

        Args:
            host (bytes | str | None): The host to connect to. Use None for localhost.
            port (bytes | str | int | None): The port to connect to.

        Raises:
            OSError: If the socket could not be opened and connected to.
        """
        try:
            self._sock: Optional[socket.socket] = socket.create_connection(
                (host, port)
            )
        except OSError as exc:
            self._sock = None
            raise OSError("Could not open or connect to socket") from exc

    def __del__(self) -> None:
        if self._sock is not None:
            self._sock.close()
            self._sock = None

    def write_pkt_data(self, data: bytes, length: int, data_type: int) -> None:
        """Write packet data to the socket.

        Args:
            data (bytes): The data to write.
            length (int): The length of the data.
            data_type (int): The user-defined data type flag.

        Raises:
            OSError: If the socket is not open.
        """
        if self._sock is None:
            raise OSError("socket is not open")
        Packet(length, data_type, data).write_pkt_data(self._sock)

    def recv_pkt_data(self) -> Packet:
        """Receive packet data from the socket.

        Raises:
            OSError: If the socket is not open.

        Returns:
            Packet: The packet received from the socket.
        """
        if self._sock is None:
            raise OSError("socket is not open")
        return Packet.recv_pkt_data(self._sock)
