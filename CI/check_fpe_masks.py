#!/usr/bin/env python3

# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

from pathlib import Path
import os
import re

import asyncio
import aiohttp
import gidgethub
from gidgethub.aiohttp import GitHubAPI
import typer
from rich import print
from rich.rule import Rule


def main(
    token: str = typer.Option(..., envvar="GITHUB_TOKEN"),
    repo: str = typer.Option(..., envvar="GITHUB_REPOSITORY"),
):
    asyncio.run(check(token, repo))


async def check(token: str, repo: str):
    ok = True

    async with aiohttp.ClientSession() as session:
        gh = GitHubAPI(session=session, requester="acts-project", oauth_token=token)
        srcdir = Path(__file__).parent.parent
        for root, _, files in os.walk(srcdir):
            root = Path(root)
            for f in files:
                if (
                    not f.endswith(".hpp")
                    and not f.endswith(".cpp")
                    and not f.endswith(".ipp")
                ):
                    continue
                f = root / f
                rel = f.relative_to(srcdir)
                first = True
                with f.open("r") as fh:
                    for i, line in enumerate(fh, start=1):
                        if m := re.match(r".*\/\/ ?MARK: ?(fpeMask.*)$", line):
                            if first:
                                print(Rule(str(rel)))
                            first = False
                            exp = m.group(1)
                            for m in re.findall(
                                r"fpeMask(?:Begin)?\( ?(\w+), ?(\d+) ?, ?#(\d+) ?\)",
                                exp,
                            ):
                                fpeType, count, number = m

                                loc = f"{rel}:{i}"
                                this_ok = True
                                try:
                                    issue = await gh.getitem(
                                        f"repos/{repo}/issues/{number}"
                                    )
                                except gidgethub.BadRequest as e:
                                    print(
                                        f":red_circle: [bold]FPE mask at {loc} has invalid issue number {number}[/bold]"
                                    )
                                    this_ok = False
                                    continue

                                if issue["state"] != "open":
                                    print(
                                        f":red_circle: [bold]FPE mask at {loc} has issue {number} but is not open[/bold]"
                                    )
                                    this_ok = False
                                if not "fpe" in issue["title"].lower():
                                    print(
                                        f":red_circle: [bold]FPE mask at {loc} has issue {number} but does not contain 'FPE' / 'fpe' in the title[/bold]"
                                    )
                                    this_ok = False
                                if not "fpe" in [l["name"] for l in issue["labels"]]:
                                    print(
                                        f":red_circle: [bold]FPE mask at {loc} has issue {number} but does not have the 'fpe' label[/bold]"
                                    )
                                    this_ok = False

                                if this_ok:
                                    print(
                                        f":green_circle: [bold]FPE mask at {loc}: {fpeType} <= {count}[/bold]"
                                    )

                                ok = ok and this_ok

    raise typer.Exit(code=0 if ok else 1)


typer.run(main)
