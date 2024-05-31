import struct
from enum import IntEnum, IntFlag, auto

TCP_PORT = 53467  # default port for TCP


class Status(IntFlag):
    """Status conditions for a hero."""

    HEALTHY = 0
    POISONED = auto()
    BURNED = auto()
    FROZEN = auto()
    PARALYZED = auto()
    CONFUSED = auto()
    ASLEEP = auto()
    BLINDED = auto()
    SILENCED = auto()


class ResponseCodes(IntEnum):
    """Response codes for server and client communication."""

    SVR_SUCCESS = 99  # server response flag: success
    SVR_FAILURE = auto()  # server response flag: internal error
    SVR_INVALID = auto()  # server response flag: invalid request
    SVR_NOT_FOUND = auto()  # server response flag: hero not found
    RQU_GET_HRO = auto()  # client request flag: get hero
    RQU_STR_HRO = auto()  # client request flag: store hero


class Hero:
    MAX_NAME_LEN = 55
    _packer = struct.Struct(f"@H H h B B {MAX_NAME_LEN + 1}s")

    def __init__(self, name: str, level: int) -> None:
        """Create a new instance of Hero.

        The hero's various attributes are calculated based on the level.

        Args:
            name (str): The name of the hero.
            level (int): The level of the hero.

        Raises:
            ValueError: Hero level must be between 1 and 100.
        """
        if level < 1 or level > 100:
            raise ValueError("Level must be between 1 and 100")
        self.name = name
        self._level = level
        self._health = level * 10
        self._attack = level * 2
        self._experience = 0
        self._status = Status.HEALTHY

    def set_status(self, status: Status) -> None:
        """Set the status of the hero.

        Args:
            status (Status): The status to set.
        """
        self._status = status

    def pack(self) -> bytes:
        """Pack the hero's attributes into a bytes object.

        Note that the name is truncated to the first 55 characters.

        Returns:
            bytes: The packed hero attributes.
        """
        return self._packer.pack(
            self._level,
            self._health,
            self._attack,
            self._experience,
            self._status,
            self.name[: self.MAX_NAME_LEN].encode(),
        )

    def __str__(self) -> str:
        return f"{self.name} of level {self._level} with status {self._status}"
