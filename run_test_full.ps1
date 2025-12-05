<#
PowerShell wrapper to run the full native test suite inside WSL.

Usage:
- Run from PowerShell prompt:
    .\run_test_full.ps1
- Requires WSL and the repo at C:\Users\BadBa\Marlin-bugfix-2.1.x
#>

$WslArgs = @('bash','-lc','cd /mnt/c/Users/BadBa/Marlin-bugfix-2.1.x && ./scripts/run_test_full.sh')

Write-Host "Running run_test_full.sh inside WSL..."
& wsl.exe @WslArgs
$rc = $LASTEXITCODE
if ($rc -ne 0) {
    Write-Error "WSL runner exited with code $rc"
    exit $rc
}

Write-Host "Done. See 'test/pio_test_full.log' in the repository for results."
exit 0
