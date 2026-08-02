# TODO

## Decompiler

- **Truncated `vars` list when DATA contains a struct overlay.** Observed
  2026-08-01 on ENPHSTAC.COD (from the STAGE.DAT 6.03.17 corpus,
  applications-trial/reference-corpora): the program uses RDA0..RDA115 as
  plain vars plus a struct (`st_116 = RDA116..RDA124`). tboldc emitted a
  `vars =` list that stopped well short of RDA115, so the best-effort
  source failed to compile (`undefined variable 'RDA40'`) and the
  round-trip refinement loop aborted ("verify: final compilation failed";
  `-f` needed to get output at all). Hand-widening the vars list to
  RDA0..RDA115 while keeping the emitted struct produced bytecode
  identical to the original. Fix: when emitting DATA, size the plain vars
  list from the highest register referenced (or the data-area size in the
  COD header), not just from the registers seen before the first struct.
