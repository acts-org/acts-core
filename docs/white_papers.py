#!/usr/bin/env python3

# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

from datetime import datetime
from pathlib import Path
import shutil
import base64
import subprocess
from subprocess import run
from typing import IO, List
import re
import textwrap
import tempfile
import functools
import asyncio

import typer
import toml
import pydantic
import rich.console
import rich.panel
import rich.pretty
from jinja2 import Template
import aiohttp
from gidgethub.aiohttp import GitHubAPI
from fsspec.implementations.zip import ZipFileSystem


app = typer.Typer()


class MetaData(pydantic.BaseModel):
    authors: List[str]
    title: str
    description: str = ""


class WhitePaper(pydantic.BaseModel):
    repository: str
    slug: str
    pdf_url: str | None = None
    metadata: MetaData | None = None

    @property
    def repo(self) -> str:
        m = re.match(r"^https://github.com/(.*/.*)$", self.repository)
        assert m is not None
        return m.group(1)


class Config(pydantic.BaseModel):
    white_papers: List[WhitePaper]


def parse_metadata(content: str) -> MetaData:
    m = re.search(r"\\author{(.*)}", content)
    assert m is not None
    author_text = m.group(1)
    authors = [a.strip() for a in author_text.split(r"\and")]

    m = re.search(r"\\title{(.*)}", content)
    assert m is not None
    title = m.group(1).strip()

    return MetaData(authors=authors, title=title)


def which(cmd: str) -> Path | None:
    try:
        exe = (
            run(["which", cmd], check=True, capture_output=True).stdout.decode().strip()
        )
        return Path(exe)
    except subprocess.CalledProcessError:
        return None


def coro(fn):
    @functools.wraps(fn)
    def wrapper(*args, **kwargs):
        return asyncio.run(fn(*args, **kwargs))

    return wrapper


def make_titlepage_image(convert: Path, pdf: Path, output: Path):
    run(
        [
            convert,
            "-density",
            "300",
            f"{pdf}[0]",
            "-background",
            "white",
            "-alpha",
            "remove",
            "-alpha",
            "off",
            "-depth",
            "2",
            str(output),
        ],
        check=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )


async def get_file(gh: GitHubAPI, repo: str, file: str, ref: str | None = None) -> str:
    url = f"/repos/{repo}/contents/{file}"
    if ref is not None:
        url += f"?ref={ref}"
    r = await gh.getitem(url)
    return base64.b64decode(r["content"]).decode()


class Release(pydantic.BaseModel):
    class Asset(pydantic.BaseModel):
        name: str
        browser_download_url: str

    id: int
    created_at: datetime
    published_at: datetime | None
    tag_name: str | None
    assets: List[Asset]


class Artifact(pydantic.BaseModel):
    class WorkflowRun(pydantic.BaseModel):
        head_branch: str
        head_sha: str

    updated_at: datetime
    workflow_run: WorkflowRun
    archive_download_url: str


async def get_latest_release(gh: GitHubAPI, repo: str) -> Release | None:
    latest_release = None

    async for release in gh.getiter(f"/repos/{repo}/releases"):
        release = Release(**release)
        if release.published_at is None:
            continue
        if latest_release is None:
            latest_release = release
            continue

        assert latest_release.published_at is not None
        if release.published_at > latest_release.published_at:
            latest_release = release

    return latest_release


async def get_latest_artifact(
    gh: GitHubAPI, repo: str, branch: str = "main"
) -> Artifact | None:
    latest_artifact = None
    async for artifact in gh.getiter(
        f"/repos/{repo}/actions/artifacts", iterable_key="artifacts"
    ):
        artifact = Artifact(**artifact)
        if artifact.workflow_run.head_branch != branch:
            continue
        if latest_artifact is None:
            latest_artifact = artifact
            continue

        if artifact.updated_at > latest_artifact.updated_at:
            latest_artifact = artifact

    return latest_artifact


async def download_file(
    session: aiohttp.ClientSession, url: str, target: IO[bytes], *args, **kwargs
):
    r = await session.get(url, *args, **kwargs)

    async for data, _ in r.content.iter_chunks():
        target.write(data)
    target.seek(0)


def extract_pdf_from_artifact(fh: IO[bytes], target: IO[bytes]):
    zipfs = ZipFileSystem(fh)
    files = zipfs.ls("/", detail=False)
    if len(files) != 1:
        print("Unexpected number of files in artifact", files)
        raise typer.Exit(1)
    if not files[0].endswith(".pdf"):
        print("Unexpected file in artifact", files)
        raise typer.Exit(1)

    shutil.copyfileobj(zipfs.open(files[0]), target)


@app.command()
@coro
async def pull(
    config_file: Path = Path(__file__).parent / "white_papers.toml",
    github_token: str = typer.Option(..., envvar="GITHUB_TOKEN"),
):
    console = rich.console.Console()
    convert = which("convert")
    if convert is None:
        print("convert (imagemagick) not found, please install")
        raise typer.Exit(1)

    async with aiohttp.ClientSession() as session:
        gh = GitHubAPI(session, "requester", oauth_token=github_token)

        with console.status("[bold green]Loading config...") as status:
            config = Config(**toml.load(config_file))

            for whp in config.white_papers:
                status.update(f"Loading {whp.repository}...")

                repo = whp.repo
                slug = whp.slug

                image_path = Path(__file__).parent / "white_papers" / "figures"
                assert image_path.is_dir(), image_path

                title_page_file = image_path / f"{slug}.png"

                latest_release = await get_latest_release(gh, repo)

                metadata_ref = None

                if latest_release is not None:
                    for asset in latest_release.assets:
                        if asset.name.endswith(".pdf"):
                            whp.pdf_url = asset.browser_download_url
                            metadata_ref = latest_release.tag_name
                            break

                if whp.pdf_url is not None:
                    with tempfile.NamedTemporaryFile(suffix=".pdf") as pdf_fh:
                        status.update(f"\[{repo}] Downloading PDF...")
                        await download_file(
                            session,
                            whp.pdf_url,
                            pdf_fh,
                            headers={"Authorization": f"Token {github_token}"},
                        )
                        status.update(f"\[{repo}] Converting PDF to PNG...")
                        make_titlepage_image(
                            convert, Path(pdf_fh.name), title_page_file
                        )

                else:
                    latest_artifact = await get_latest_artifact(gh, repo)

                    if latest_artifact is None:
                        print("No artifacts found for", whp.repository)
                        raise typer.Exit(1)

                    metadata_ref = latest_artifact.workflow_run.head_sha

                    status.update(f"\[{repo}] Downloading artifact...")

                    with tempfile.TemporaryFile() as fh, tempfile.NamedTemporaryFile(
                        suffix=".pdf"
                    ) as pdf_fh:
                        await download_file(
                            session,
                            latest_artifact.archive_download_url,
                            fh,
                            headers={"Authorization": f"Token {github_token}"},
                        )

                        extract_pdf_from_artifact(fh, pdf_fh)

                        status.update(f"\[{repo}] Converting PDF to PNG...")
                        make_titlepage_image(
                            convert, Path(pdf_fh.name), title_page_file
                        )

                status.update(f"\[{repo}] Getting metadata for ref {metadata_ref}...")

                metadata = await get_file(gh, repo, "metadata.tex", ref=metadata_ref)
                abstract = await get_file(gh, repo, "abstract.tex", ref=metadata_ref)

                abstract = textwrap.dedent(abstract).strip()
                abstract = "\n".join(
                    [
                        line
                        for line in abstract.split("\n")
                        if not line.strip().startswith("%")
                    ]
                ).strip()
                metadata = parse_metadata(metadata)
                metadata.description = abstract

                console.print(
                    rich.panel.Panel(rich.pretty.Pretty(metadata), title=whp.repository)
                )

                whp.metadata = metadata

            status.update("Updating config...")

            with open(config_file, "w") as f:
                toml.dump(config.model_dump(), f)


@app.command()
def render(config_file: Path = Path(__file__).parent / "white_papers.toml"):
    config = Config(**toml.load(config_file))
    docs_path = Path(__file__).parent / "white_papers"
    assert docs_path.is_dir(), docs_path
    index_target_file = docs_path / "index.md"

    template_file = Path(__file__).parent / "white_paper_template.md.j2"
    index_template_file = Path(__file__).parent / "white_paper_index_template.md.j2"

    tpl = Template(template_file.read_text())
    index_tpl = Template(index_template_file.read_text())

    index_target_file.write_text(index_tpl.render(config=config))
    print("Index written to", index_target_file)

    for whp in config.white_papers:
        assert whp.metadata is not None, "White paper meta data is missing"
        target_file = docs_path / f"{whp.slug}.md"
        target_file.write_text(tpl.render(whp=whp, image_path="figures"))
        print("-", whp.metadata.title, "->", target_file)


if "__main__" == __name__:
    app()
