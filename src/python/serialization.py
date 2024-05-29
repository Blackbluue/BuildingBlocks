"""Contain class for I/O operations."""
import socket
import sys
from typing import Union


class IO_Info:
    """Contain class for I/O operations."""

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
        self._header = Struct("!3I")

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
