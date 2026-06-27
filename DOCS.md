# DOCS.md  -  srb2dbot Documentation Index

Documentation is written in pure HTML under the `doc/` directory and updated to reflect the current state of the project.

## Table of Contents

| Page | Description |
|---|---|---|
| [index.html](doc/index.html) | Project overview, quick start, deployment (NixOS → Debian 12), features at a glance, security model, dependencies |
| [commands.html](doc/commands.html) | Complete slash command reference with parameters and behavior |
| [building.html](doc/building.html) | Build instructions for NixOS and Linux, dependency installation, CMake presets, Debian 12 native build |
| [architecture.html](doc/architecture.html) | Directory structure, runtime pipeline, key files, anonymous namespace dependencies, security architecture, deployment, CI pipeline, gametype resolution |
| [tools.html](doc/tools.html) | Reference for companion bash scripts (thumbnail grabber, intermission overlay generator, build.sh) |

## Updating Documentation

Documentation files should be updated after any significant change to:

1. **Command set**  -  additions, removals, or parameter changes → update `commands.html`
2. **Build system**  -  new presets, dependency changes → update `building.html`
3. **Architecture**  -  file restructuring, new modules, security changes → update `architecture.html`
4. **Overview**  -  new features, dependency changes → update `index.html`

The table of contents in this file should always reflect the current set of documentation pages.
