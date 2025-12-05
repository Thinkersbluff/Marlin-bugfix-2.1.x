Generate Version README
=======================

What this documents
- The `buildroot/bin/generate_version` script now preserves intentionally commented-out `#define` lines from your existing `Marlin/Version.h` when regenerating the file. This prevents the generator from silently "uncommenting" fields you intentionally disabled (for example `// #define DEFAULT_MACHINE_UUID ...`).

Which keys are preserved (by default)
- `PROTOCOL_VERSION`
- `SOURCE_CODE_URL`
- `DEFAULT_MACHINE_UUID`
- `WEBSITE_URL`

Behavior summary
- If the script finds a commented-out `#define` for one of the keys above in the existing `Marlin/Version.h`, it will copy that commented line verbatim into the newly generated `Marlin/Version.h` instead of writing an active `#define`.
- Other keys (for example `SHORT_BUILD_VERSION`, `DETAILED_BUILD_VERSION`, `STRING_DISTRIBUTION_DATE`, `MACHINE_NAME`) are always written actively by the generator.
- The script still overwrites `Marlin/Version.h` (it does not merge changes). If you want to keep a hand-edited file, commit or back it up before running the generator.

How to override values
- Environment variables (highest priority): pass values into the script environment prior to running it. Example:

  ```bash
  ALLOW_GENERATE_VERSION=1 \
  SHORT_BUILD_VERSION="v2.1.0" \
  DETAILED_BUILD_VERSION="Thinkersbluff-CR6-v2.1.0" \
  STRING_DISTRIBUTION_DATE="2025-12-02" \
  MACHINE_NAME="Creality CR6-SE" \
  ./buildroot/bin/generate_version Marlin
  ```

- Manual edit: edit `Marlin/Version.h` and commit that file in the repository (useful for reproducible release builds). If you want a key to remain commented in the generated output, leave it commented in the source `Marlin/Version.h` (the generator will preserve that comment for the keys listed above).

Recommended usage patterns
- Release builds: commit a static `Marlin/Version.h` with `SHORT_BUILD_VERSION` and `DETAILED_BUILD_VERSION` set to stable values.
- CI / daily dev builds: run `generate_version` with `ALLOW_GENERATE_VERSION=1` so `DETAILED_BUILD_VERSION` embeds branch+describe+timestamp, but pass any fields you want fixed as environment variables.

Notes about Windows/WSL
- The generator is a bash script. On Windows use WSL (or Git Bash) to run it. Example from PowerShell:

  ```powershell
  wsl bash -lc 'cd /mnt/c/Users/BadBa/Marlin-bugfix-2.1.x && ALLOW_GENERATE_VERSION=1 ./buildroot/bin/generate_version Marlin'
  ```

- The script requires `git` to produce branch/describe info; run it from a Git-aware environment (WSL or CI).

If you want different keys preserved or a different policy (for example: preserve *all* commented defines), I can update `buildroot/bin/generate_version` accordingly.
