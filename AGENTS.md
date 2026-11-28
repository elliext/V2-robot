# Agent Instructions

Domain facts taught by the repo owner live in [knowledge/](knowledge/). Treat them as authoritative. Read the relevant topic before changing autonomous, field-position, or match-related code.

## Branch policy

- Keep `main` production-only: no test files, test build targets, or hardware-test modes.
- Put test code and run tests on the remote `test` branch, preferably in a separate worktree.
- Production firmware builds may run on `main`. Do not merge test-only code back into `main`.
- Preserve production features when updating `test` from `main`.

## Knowledge topics

- [Field setup](knowledge/field-setup.md): V5RC quadrant names and layout, AprilTag IDs.
- [Game objects](knowledge/game-objects.md): Cup and pin dimensions.
