param(
    [string]$ApkPath = ""
)

$ErrorActionPreference = "Stop"

$projectRoot = $PSScriptRoot
$adb = Join-Path $projectRoot ".android-sdk\platform-tools\adb.exe"

if ([string]::IsNullOrWhiteSpace($ApkPath)) {
    $ApkPath = Join-Path $projectRoot "dist\PixelClock-debug.apk"
}

if (-not (Test-Path -LiteralPath $adb)) {
    throw "Android installation tool is missing: $adb"
}

if (-not (Test-Path -LiteralPath $ApkPath)) {
    throw "APK is missing: $ApkPath"
}

& $adb start-server | Out-Host
$deviceLines = & $adb devices
$readyDevices = @(
    $deviceLines |
        Where-Object { $_ -match "^\S+\s+device$" } |
        ForEach-Object { ($_ -split "\s+")[0] }
)

if ($readyDevices.Count -eq 0) {
    $unauthorized = @($deviceLines | Where-Object { $_ -match "\s+unauthorized$" })

    if ($unauthorized.Count -gt 0) {
        Write-Host "Phone detected, but authorization is pending."
        Write-Host "Unlock the phone and tap 'Allow USB debugging', then run this script again."
    } else {
        Write-Host "No authorized Android phone was found."
        Write-Host "Connect the phone by USB, enable USB debugging, and approve the prompt on the phone."
    }

    exit 2
}

$serial = $readyDevices[0]
Write-Host "Installing Pixel Clock on $serial..."
& $adb -s $serial install -r $ApkPath

if ($LASTEXITCODE -ne 0) {
    throw "APK installation failed."
}

Write-Host "Pixel Clock was installed successfully."
