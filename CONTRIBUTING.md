# Contributing

How changes get into this repository. Short version: small changes, explained in comments, tested, reviewed before they are merged.

For building, flashing and testing see [OPERATIONS.md](OPERATIONS.md).

## A teaching project

Code here is written to be read by someone who is learning. That shapes how comments are written; see [Comments teach](#comments-teach).

## Workflow

### Before v0.1

Work is committed directly to `main`. The goal of this phase is a skeleton that runs on the hardware.

### From v0.1 on

Every change follows the same path:

```
issue  ──▶  branch  ──▶  commits  ──▶  pull request  ──▶  review  ──▶  merge to main
```

1. **Open an issue** that describes the bug or the feature. Templates are provided. Discussing the "what" before writing the "how" saves rework.
2. **Create a branch** from an up-to-date `main`, named after the issue:

   | Kind | Branch name | Example |
   | --- | --- | --- |
   | Feature | `feature/<issue>-<short-name>` | `feature/12-improv-provisioning` |
   | Bug fix | `fix/<issue>-<short-name>` | `fix/17-button-false-wake` |
   | Docs, tooling | `chore/<issue>-<short-name>` | `chore/21-ci-cache` |

3. **Commit** in small steps that each build and pass the tests.
4. **Open a pull request** into `main`. Fill in the template and link the issue with `Closes #12` so it closes automatically on merge.
5. **Review.** CI must be green and the owner approves.
6. **Merge**, then delete the branch.

Why this much process for a small project: each step leaves a record. The issue says why, the pull request says what and how it was tested, and `main` only ever contains reviewed, working code.

## Commit messages

[Conventional Commits](https://www.conventionalcommits.org) format:

```
<type>: <what changed, in the imperative, no full stop>

<optional body: why, and anything a reader of the history should know>
```

| Type | For |
| --- | --- |
| `feat` | A new capability |
| `fix` | A bug fix |
| `docs` | Documentation only |
| `test` | Tests only |
| `refactor` | Restructuring without behaviour change |
| `build` | Build system, dependencies, CI |
| `chore` | Everything else |

Examples: `feat: wake on button press`, `fix: keep old screen when download fails`.

## Code rules

### Layers

| Folder | May include | Must not include |
| --- | --- | --- |
| `firmware/src/pure/` | C++ standard library, other `pure/` headers | anything else |
| `firmware/src/app/` | `pure/`, other `app/` headers | Arduino, ESP-IDF, `hal/`, `storage/` |
| `firmware/src/hal/`, `storage/` | Arduino, ESP-IDF, `app/`, `pure/` | each other, unless there is a good reason |
| `firmware/src/main.cpp` | everything | |

Rule of thumb: **if it is a decision, it belongs in `pure/` or `app/`; if it touches a pin, it belongs in `hal/`.** New hardware gets a new interface in `app/ports.h`, a real adapter in `hal/`, and a fake in `tests/host/fakes.h`.

The host test build enforces the first two rows: it compiles only `pure/` and `app/`, without any Arduino headers available.

### Style

- C++17. Formatting is done by `clang-format` with the rules in `.clang-format`. Do not format by hand:

  ```sh
  find firmware/src firmware/tests firmware/include \( -name '*.h' -o -name '*.cpp' \) | xargs clang-format -i
  ```

- Names: `camelCase` functions and variables, `PascalCase` types, `kPascalCase` constants, `g_` prefix for the rare global, `I` prefix for interfaces.
- No exceptions, no RTTI, no heap allocation in the wake cycle. Functions report failure through their return value.
- Every wait on the outside world has a timeout.
- No bare numbers in logic. Give them a name in `config.h` or next to their use.
- No GPIO numbers outside `hal/board.h`.
- No secrets in the repository. `secrets.h` is ignored by git; keep it that way.

### Comments teach

Comments explain **why**, and name the idea being used, so a learner can look it up. When you apply a pattern or an algorithm:

1. Mark it at the place of use with a `PATTERN:` or `ALGORITHM:` comment: what it is, and why it is the right tool here.
2. Add a row to [docs/PATTERNS.md](docs/PATTERNS.md) if it is not listed yet.

Do not narrate what the code plainly says (`i++  // increase i`). Do write down anything that was surprising, non-obvious, or learned the hard way on the hardware.

## Tests

- Every change to `pure/` or `app/` comes with tests in `firmware/tests/host/`.
- A bug fix starts with a test that fails because of the bug.
- Hardware behaviour that cannot be unit tested is checked on the board; say what you checked in the pull request.

Before asking for review:

```sh
# unit tests
cmake -S firmware/tests/host -B build/host-tests && cmake --build build/host-tests
ctest --test-dir build/host-tests --output-on-failure

# firmware still builds
(cd firmware && pio run)
```

## Definition of done

- [ ] Builds without warnings in project code
- [ ] Unit tests pass; new logic has new tests
- [ ] Formatted with `clang-format`
- [ ] Patterns and algorithms are commented and listed in `docs/PATTERNS.md`
- [ ] Docs updated where behaviour changed (`README`, `OPERATIONS`, `docs/`)
- [ ] `CHANGELOG.md` has a line under "Unreleased"
- [ ] Tested on the board if it touches `hal/`, `storage/` or `main.cpp`

## Third-party code and licenses

This project is released under the [MIT license](LICENSE). By contributing you agree that your contribution is released under the same license.

Do not copy code from other projects into this repository without checking its license first. Some of the projects this one learned from are GPL-licensed or have no license at all. Ideas and hardware facts are free to use; code is not. When in doubt, write it yourself and credit the inspiration in the README.
