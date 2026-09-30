# Markdown Style Guide

Use direct, practical prose for RackNES documentation. This guide is copied
from Fourier's Markdown style guide, with project names and documentation
locations adapted to this repository.

## Structure And Formatting

-   Start with one `#` title and a short paragraph explaining the document's
    purpose. Use ordered ATX headings (`##`, `###`) in Title Case.
-   Wrap prose near 80 columns. Keep commands, URLs, and table rows intact
    when wrapping would make them harder to use.
-   Use `-   ` for ordinary bullets with four-space continuation indentation.
    Use `1.  ` for ordered steps and `- [ ]` for task checkboxes.
-   Leave blank lines around paragraphs, lists, tables, and fenced blocks.
-   Tag code fences with their language. Use `shell` for simple commands,
    `cpp` for C++, and `text` for output or file trees.
-   Use inline code for paths, commands, identifiers, and literal values.
-   Prefer relative links within the repository. Use reference links for
    repeated destinations and inline links for one-off references.
-   Use tables for real comparisons and mappings, not as decoration.
-   Default to ASCII punctuation in new prose. Preserve product names such
    as RackNES, CV Genie, KautenjaDSP, Arhythmetic Units, and VCV Rack accurately.

## Content And Maintenance

Keep public product information in `README.md` and the user manuals.
Put contributor setup, architecture, and validation guidance in
[CONTRIBUTING.md](../../CONTRIBUTING.md), agent workflow in
[AGENTS.md](../../AGENTS.md), style guides in `docs/style-guides/`, and
feature-specific acceptance/completion evidence in its own [spec](../../specs/)
when one exists.
Follow `whitepaper/README.md` for the technical report and citation metadata.
Avoid duplicating the same rule across documents.

Commands must name their working directory and prerequisites. Distinguish
copyable commands from placeholders, current behavior from intended changes,
and automated results from manual observations. Include units, workloads,
and uncertainty for numerical or performance claims.

Ordinary documentation does not need source-file license headers. Link to
the repository's license document when relevant. Update affected links and
instructions when paths or commands change; avoid unrelated style rewrites.

Before finishing, check relative links and referenced files, inspect the
diff for stale product names or unsupported commands, and run
`git diff --check`. Generated manuals also require visual inspection.
