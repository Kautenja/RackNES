# RackNES Support

Find help with RackNES and CV Genie, report a problem, or suggest an improvement.
You do not need a source build or coding experience to ask for help.

## Manuals And Troubleshooting

-   [RackNES manual][racknes-manual]: ROM loading, controls, clocking, audio
    routing, snapshots, and troubleshooting.
-   [CV Genie manual][genie-manual]: expander placement, game maps, row
    assignments, voltage behavior, and troubleshooting.
-   [Changelog](CHANGELOG.md): fixes and behavior changes by version.

The PDF links above target the latest GitHub release. If you are using a
development build, see the [manual guide](manual/README.md) for building the
manuals from the same checkout.

## Questions And Bug Reports

Search the [existing issues][issues] for a matching question or problem.
If none matches, [open an issue][new-issue]. Use the bug report template for
unexpected behavior; for a usage question, explain what you are trying to
do and where you are stuck.

Include your Rack and plugin versions, operating system and CPU architecture,
sample rate, and enough steps to reproduce the problem. The bug report
template prompts for ROM details and module settings when relevant; unknown
details can be marked as unknown. A minimal patch, screenshot, or relevant
Rack log excerpt can help.

Do not upload commercial ROMs. Identify the game or homebrew and its revision
instead. Check patches and logs for private information, including local ROM
paths, before sharing them.

## Feature Requests

Use the feature request template in [new issues][new-issue]. Describe the
musical workflow or task you want to support, the behavior you would like,
and any workaround you currently use.

## Building And Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for environment setup, builds,
regression checks, and pull request guidance. For build or test failures,
include the commit, command and working directory, tool versions, and relevant
error output in the bug report.

[racknes-manual]: https://github.com/Kautenja/RackNES/releases/latest/download/RackNES.pdf
[genie-manual]: https://github.com/Kautenja/RackNES/releases/latest/download/CVGenie.pdf
[issues]: https://github.com/Kautenja/RackNES/issues
[new-issue]: https://github.com/Kautenja/RackNES/issues/new/choose
