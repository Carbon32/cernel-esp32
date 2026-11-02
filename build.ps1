$ErrorActionPreference = "Stop"
idf.py build
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }