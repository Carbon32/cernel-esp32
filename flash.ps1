$ErrorActionPreference = "Stop"
idf.py -p COM3 flash
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }