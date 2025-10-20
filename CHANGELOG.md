# Changelog

All notable changes to this project are documented here.

## [Unreleased]

- Persisted runtime pulse delays (PD) now use short Preference (NVS) keys in the `stepper` namespace: `slow_pd`, `med_pd`, `fast_pd`, `moveto_pd`.
- Firmware includes automatic migration from legacy keys (`slowPD`, `mediumPD`, `fastPD`, `moveToPD`) to the new keys on startup.
- Added Quick Start to top-level `README.md` and updated `MagLoop_Common_Files/README.md` with migration notes.


## Notes

- Migration is non-destructive: if old values exist they are copied into new keys.
- To reset values to firmware defaults, write new values into the new keys (via GUI or by issuing a reset command if implemented).
