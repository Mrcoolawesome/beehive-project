Markdown

# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview
NASA F Prime flight software framework project for monitoring beehive weight using a Wii Balance Board and Raspberry Pi telemetry stack. The codebase is currently an early-stage skeleton; the full implementation will follow the three-tiered architecture explicitly stated in README.md: Driver -> Manager -> Application layers for every component, with the Wii board split into those three distinct roles (raw I/O / calibration & routing / business logic).

## Build and Run
This project strictly uses the `fprime-util` command-line tool to manage the CMake build system. Do not invoke `cmake` or `make` directly.

To build the project:
```bash
fprime-util generate  # Run once to set up the build cache
fprime-util build     # Compile the project
fprime-util check     # Run unit tests (when implemented)
```

## Coding Style and Conventions

Strictly enforce one layer per component: driver (raw I/O), manager (filtering & calibration), application (business logic). No cross-layer leaks; every state transition flows through the designated owner.

When creating new components, DO NOT edit CMakeLists.txt manually. Instead, use the built-in F' component generator from the directory where you want the component to live:

you also MUST be in the python virtual environment to use this tool

```bash
. venv/bin/activate
```
```bash
fprime-util new --component
```

## Architecture Decisions

  - Wii Board Driver: owns raw sensor I/O only — reads /dev/input/eventX via evdev. No units, no filtering, no business logic.

  - Wii Board Manager: owns calibration offsets and unit conversion (raw values -> kg), state management.

  - Wii Board Application: owns sampling rates, alert thresholds, telemetry dispatch to the server via F' comms layer.

TODO: implement each of these as genuine fprime components in /BeehiveProject/Components.


### What changed and why:
*   **Removed Rust:** Deleted the `cargo test` stub.
*   **Replaced manual CMake with `fprime-util`:** F' uses `fprime-util generate` to configure the build and `fprime-util build` to compile. Teaching Claude to use these commands ensures it won't break your build system.
*   **Fixed Component Generation:** Removed the hallucinated `fprime_setup` and the instruction to edit lines 14-15 of CMakeLists. F' has a built-in interactive generator (`fprime-util new --component`) that builds the folder structure, creates the `.fpp` templates, and automatically wires the component into your CMake files. 

<ElicitationsGroup message="To start building your F' components:">
{/* Reason: Offers logical next steps to begin generating the actual F' code using the corrected tooling. */}
  <Elicitation label="How to use fprime-util new --component" query="Walk me through how to use fprime-util new --component to generate the WiiBoardDriver component."/>
  <Elicitation label="Review the C++ evdev logic for the driver" query="Before we put it into F', can you write the raw C++ code using evdev to read the Balance Board?"/>
</ElicitationsGroup>