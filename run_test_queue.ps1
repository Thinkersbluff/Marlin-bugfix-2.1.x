<#
Run the focused native `test_queue` from Windows by invoking the WSL runner script.

Usage:
- Double-click this file in Explorer (will open PowerShell and run).
- Or run from a PowerShell prompt:
    .\run_test_queue.ps1

Notes:
- Requires WSL and the repo located at `C:\Users\BadBa\Marlin-bugfix-2.1.x`.
- The script calls the WSL runner `./scripts/run_test_queue.sh` which activates the repo `.venv` and runs the test.
- If Windows PowerShell blocks script execution you can either unblock this file or allow local scripts:
    Unblock-File .\run_test_queue.ps1
    Set-ExecutionPolicy -Scope CurrentUser -ExecutionPolicy RemoteSigned
#>

$RepoWinPath = 'C:\Users\BadBa\Marlin-bugfix-2.1.x'
# Build WSL command arguments safely and invoke wsl.exe
$wslArgs = @('bash', '-lc', 'cd /mnt/c/Users/BadBa/Marlin-bugfix-2.1.x && ./scripts/run_test_queue.sh')

Write-Host "Running run_test_queue.sh inside WSL..."

# Invoke WSL with argument array to avoid quoting issues
& wsl.exe @wslArgs
$rc = $LASTEXITCODE
if ($rc -ne 0) {
    Write-Error "WSL runner exited with code $rc"
    exit $rc
}

Write-Host "Done. See 'test/pio_test_queue.log' in the repository for results."
exit 0
