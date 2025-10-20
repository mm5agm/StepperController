# Release notes

Release: docs-and-migration

Summary

- Add Quick Start to top-level README
- Document NVS/Preferences migration for runtime pulse delays in `MagLoop_Common_Files/README.md`
- Add a top-level `CHANGELOG.md`

Notes for reviewers

- The firmware expects new short Preference keys in namespace `stepper`: `slow_pd`, `med_pd`, `fast_pd`, `moveto_pd`. The firmware migrates legacy keys automatically if present.
- Files changed: `README.md`, `MagLoop_Common_Files/README.md`, `CHANGELOG.md`, `RELEASE_NOTES.md` (new)
