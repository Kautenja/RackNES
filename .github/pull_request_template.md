# Pull Request

Describe the problem, the resulting behavior, and the evidence for this change.
Use the same review and validation standards for human and agent-assisted work.

<!-- Follow ../CONTRIBUTING.md and the relevant guide in ../docs/style-guides/.
Coding agents must also follow ../AGENTS.md. Describe the final change so a
reviewer can assess it without access to a chat or local session history.
Fill in the relevant sections and remove unused prompts. Mark checks as not
applicable with a reason when appropriate. Only claim checks you ran. -->

## Change

<!-- Explain the problem and what changes for users or contributors. Include
a before/after example when useful. Link an issue with "Fixes #123" only if
this PR resolves it; otherwise use "Related to #123". -->

## Compatibility And Processing

<!-- For module/emulator changes, explain effects on saved patches, parameter/
port/light IDs, slugs, custom JSON, serialized game/location indices, expander
messages, and ROM or snapshot handling. Describe a compatibility plan for
intentional changes. Address affected emulation scheduling, clock/sample-rate
handling, controller inputs, and individual-channel/MIX routing.
For processing/shared-buffer changes, explain allocation, bounded per-sample
work, and engine/display ownership or synchronization implications. -->

## Validation

<!-- Choose checks using CONTRIBUTING.md's "Choosing Validation" section.
Record exact commands and working directories, results, OS/architecture,
compiler, and Rack/SDK versions where relevant. Distinguish:
- Rack plugin build and executable headless regression checks.
- Manual Rack checks: ROM/homebrew revision and mapper, sample rate, settings,
  observations, and screenshots for visual changes. Include existing-patch
  loading when persistence changes. Do not upload commercial ROMs.
- Documentation-only checks: links, paths, commands, and git diff --check;
  no C++ build is required. Inspect rendered pages for manual/report changes.
State skipped checks, failures, and limitations with reasons.
Separate observed results from assumptions or proposed checks. If reporting
CI or another contributor's results, link the run or evidence and identify it.
For performance claims, include comparable baseline/candidate workloads,
compiler flags, repeated measurements, and uncertainty. -->

## Checklist

- [ ] I reviewed the diff and followed the relevant contributor/style guidance.
- [ ] I recorded validation results and any skipped or failing checks.
- [ ] I preserved compatibility or documented intentional changes and evidence.
- [ ] I updated affected documentation, manifests, patches, and resources,
      or explained why no updates are needed.
