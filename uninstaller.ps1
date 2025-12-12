# Pulse Monitor Uninstaller
# Double-click to remove Pulse Monitor

# Check for admin privileges
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $isAdmin) {
    Write-Host "===============================================" -ForegroundColor Red
    Write-Host "  Administrator privileges required!" -ForegroundColor Red
    Write-Host "===============================================" -ForegroundColor Red
    Write-Host ""
    Write-Host "Please right-click this script and select 'Run as Administrator'" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Press any key to exit..."
    $null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
    exit 1
}

# Configuration
$serviceName = "PulseMonitor"
$installPath = "C:\Program Files\PulseMonitor"

# Banner
Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "   Pulse Monitor Uninstaller" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Check if installed
$existingTask = Get-ScheduledTask -TaskName $serviceName -ErrorAction SilentlyContinue
if (-not $existingTask -and -not (Test-Path $installPath)) {
    Write-Host "Pulse Monitor is not installed." -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Press any key to exit..."
    $null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
    exit 0
}

# Confirm uninstall
Write-Host "This will remove Pulse Monitor from your system." -ForegroundColor Yellow
$response = Read-Host "Are you sure you want to continue? (y/n)"
if ($response -ne 'y' -and $response -ne 'Y') {
    Write-Host "Uninstallation cancelled." -ForegroundColor Yellow
    Start-Sleep -Seconds 2
    exit 0
}

Write-Host ""

# Stop the service
Write-Host "[1/3] Stopping Pulse Monitor..." -ForegroundColor Green
try {
    $process = Get-Process -Name "pulse-monitor" -ErrorAction SilentlyContinue
    if ($process) {
        Stop-Process -Name "pulse-monitor" -Force
        Start-Sleep -Seconds 1
        Write-Host "      ✓ Process stopped" -ForegroundColor Gray
    } else {
        Write-Host "      ✓ No running process found" -ForegroundColor Gray
    }
} catch {
    Write-Host "      ⚠ Error stopping process: $_" -ForegroundColor Yellow
}

# Remove scheduled task
Write-Host "[2/3] Removing auto-start..." -ForegroundColor Green
try {
    if ($existingTask) {
        Unregister-ScheduledTask -TaskName $serviceName -Confirm:$false
        Write-Host "      ✓ Auto-start removed" -ForegroundColor Gray
    } else {
        Write-Host "      ✓ No scheduled task found" -ForegroundColor Gray
    }
} catch {
    Write-Host "      ⚠ Error removing scheduled task: $_" -ForegroundColor Yellow
}

# Remove installation directory
Write-Host "[3/3] Removing files..." -ForegroundColor Green
try {
    if (Test-Path $installPath) {
        Remove-Item $installPath -Recurse -Force
        Write-Host "      ✓ Installation directory removed" -ForegroundColor Gray
    } else {
        Write-Host "      ✓ No installation directory found" -ForegroundColor Gray
    }
} catch {
    Write-Host "      ⚠ Error removing files: $_" -ForegroundColor Yellow
}

# Success message
Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "   Uninstallation Complete!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""
Write-Host "Pulse Monitor has been removed from your system." -ForegroundColor White
Write-Host ""
Write-Host "Press any key to exit..."
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")