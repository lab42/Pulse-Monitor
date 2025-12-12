# Pulse Monitor Installer
# Double-click to install and configure auto-start

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
$exeName = "pulse-monitor.exe"
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$exePath = Join-Path $scriptDir $exeName

# Banner
Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "   Pulse Monitor Installer" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Check if executable exists
if (-not (Test-Path $exePath)) {
    Write-Host "ERROR: $exeName not found in current directory!" -ForegroundColor Red
    Write-Host "Please ensure $exeName is in the same folder as this installer." -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Press any key to exit..."
    $null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
    exit 1
}

# Check if already installed
$existingTask = Get-ScheduledTask -TaskName $serviceName -ErrorAction SilentlyContinue
if ($existingTask) {
    Write-Host "Pulse Monitor is already installed!" -ForegroundColor Yellow
    Write-Host ""
    $response = Read-Host "Do you want to reinstall? (y/n)"
    if ($response -ne 'y' -and $response -ne 'Y') {
        Write-Host "Installation cancelled." -ForegroundColor Yellow
        Start-Sleep -Seconds 2
        exit 0
    }
    Write-Host ""
    Write-Host "Removing existing installation..." -ForegroundColor Yellow
    Unregister-ScheduledTask -TaskName $serviceName -Confirm:$false
    Stop-Process -Name "pulse-monitor" -Force -ErrorAction SilentlyContinue
}

# Create installation directory
Write-Host "[1/4] Creating installation directory..." -ForegroundColor Green
try {
    if (Test-Path $installPath) {
        Remove-Item "$installPath\*" -Force -ErrorAction SilentlyContinue
    } else {
        New-Item -ItemType Directory -Force -Path $installPath | Out-Null
    }
    Write-Host "      ✓ Directory created: $installPath" -ForegroundColor Gray
} catch {
    Write-Host "      ✗ Failed to create directory: $_" -ForegroundColor Red
    exit 1
}

# Copy executable
Write-Host "[2/4] Installing executable..." -ForegroundColor Green
try {
    Copy-Item $exePath -Destination $installPath -Force
    Write-Host "      ✓ Executable installed" -ForegroundColor Gray
} catch {
    Write-Host "      ✗ Failed to copy executable: $_" -ForegroundColor Red
    exit 1
}

# Create scheduled task
Write-Host "[3/4] Configuring auto-start..." -ForegroundColor Green
try {
    $action = New-ScheduledTaskAction -Execute "$installPath\$exeName"
    $trigger = New-ScheduledTaskTrigger -AtStartup
    $principal = New-ScheduledTaskPrincipal -UserId "SYSTEM" -LogonType ServiceAccount -RunLevel Highest
    $settings = New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries -StartWhenAvailable -RestartCount 3 -RestartInterval (New-TimeSpan -Minutes 1)
    
    Register-ScheduledTask -TaskName $serviceName -Action $action -Trigger $trigger -Principal $principal -Settings $settings -Force | Out-Null
    Write-Host "      ✓ Auto-start configured (runs at system boot)" -ForegroundColor Gray
} catch {
    Write-Host "      ✗ Failed to create scheduled task: $_" -ForegroundColor Red
    exit 1
}

# Start the service
Write-Host "[4/4] Starting Pulse Monitor..." -ForegroundColor Green
try {
    Start-ScheduledTask -TaskName $serviceName
    Start-Sleep -Seconds 2
    
    # Verify it's running
    $process = Get-Process -Name "pulse-monitor" -ErrorAction SilentlyContinue
    if ($process) {
        Write-Host "      ✓ Pulse Monitor is running (PID: $($process.Id))" -ForegroundColor Gray
    } else {
        Write-Host "      ⚠ Started, but process not detected yet (may be connecting to device...)" -ForegroundColor Yellow
    }
} catch {
    Write-Host "      ✗ Failed to start: $_" -ForegroundColor Red
}

# Success message
Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "   Installation Complete!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""
Write-Host "Pulse Monitor is now installed and running." -ForegroundColor White
Write-Host "It will automatically start when Windows boots." -ForegroundColor White
Write-Host ""
Write-Host "Management Commands:" -ForegroundColor Cyan
Write-Host "  • Start:   " -NoNewline; Write-Host "Start-ScheduledTask -TaskName $serviceName" -ForegroundColor Yellow
Write-Host "  • Stop:    " -NoNewline; Write-Host "Stop-ScheduledTask -TaskName $serviceName" -ForegroundColor Yellow
Write-Host "  • Status:  " -NoNewline; Write-Host "Get-ScheduledTask -TaskName $serviceName" -ForegroundColor Yellow
Write-Host ""
Write-Host "To uninstall, run the uninstaller script." -ForegroundColor Gray
Write-Host ""
Write-Host "Press any key to exit..."
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
