# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

from pathlib import Path
from typing import List

import pydantic


class Item(pydantic.BaseModel):
    path: Path
    line: int
    col: int
    message: str
    code: str
    severity: str

    def __hash__(self):
        return hash((self.path, self.line, self.col, self.code))

    def __eq__(self, other):
        return (self.path, self.line, self.col, self.code) == (
            other.path,
            other.line,
            other.col,
            other.code,
        )


class ItemCollection(pydantic.RootModel[List[Item]]):
    pass
