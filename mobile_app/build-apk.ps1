param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug"
)

$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildDrive = "Z:"
$mappingCreated = $false

function Invoke-Checked {
    param(
        [Parameter(Mandatory = $true)]
        [string]$FilePath,

        [Parameter(ValueFromRemainingArguments = $true)]
        [string[]]$Arguments
    )

    & $FilePath @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code ${LASTEXITCODE}: $FilePath"
    }
}

try {
    $existingMapping = subst | Select-String -SimpleMatch "$buildDrive\:"
    if ($existingMapping) {
        if ($existingMapping.Line -notlike "*$projectRoot*") {
            throw "$buildDrive is already mapped to another location."
        }
    } else {
        Invoke-Checked subst $buildDrive $projectRoot
        $mappingCreated = $true
    }

    $mappedRoot = "$buildDrive\"
    $nodeRoot = Join-Path $mappedRoot ".tools\node-v22.23.1-win-x64"
    $jdkRoot = Get-ChildItem -LiteralPath (Join-Path $mappedRoot ".tools\jdk") -Directory |
        Select-Object -First 1 -ExpandProperty FullName
    $androidSdkRoot = Join-Path $mappedRoot ".android-sdk"

    if (-not (Test-Path -LiteralPath (Join-Path $nodeRoot "npm.cmd"))) {
        throw "Portable Node.js was not found under $nodeRoot."
    }
    if (-not $jdkRoot) {
        throw "Portable JDK was not found under .tools\jdk."
    }
    if (-not (Test-Path -LiteralPath (Join-Path $androidSdkRoot "platforms\android-36"))) {
        throw "Android API 36 SDK was not found under .android-sdk."
    }

    $env:JAVA_HOME = $jdkRoot
    $env:ANDROID_HOME = $androidSdkRoot
    $env:ANDROID_SDK_ROOT = $androidSdkRoot
    $env:GRADLE_USER_HOME = Join-Path $mappedRoot ".tools\gradle-home"
    $env:PATH = "$nodeRoot;$jdkRoot\bin;$androidSdkRoot\platform-tools;$env:PATH"

    $proxyValue = if ($env:HTTPS_PROXY) { $env:HTTPS_PROXY } else { $env:HTTP_PROXY }
    if ($proxyValue) {
        $proxyUri = [Uri]$proxyValue
        $env:GRADLE_OPTS = @(
            $env:GRADLE_OPTS
            "-Dhttp.proxyHost=$($proxyUri.Host)"
            "-Dhttp.proxyPort=$($proxyUri.Port)"
            "-Dhttps.proxyHost=$($proxyUri.Host)"
            "-Dhttps.proxyPort=$($proxyUri.Port)"
        ) -join " "
    }

    Push-Location $mappedRoot
    try {
        Invoke-Checked (Join-Path $nodeRoot "npm.cmd") run prepare:web
        Invoke-Checked (Join-Path $nodeRoot "npx.cmd") cap sync android

        Push-Location (Join-Path $mappedRoot "android")
        try {
            $gradleTask = if ($Configuration -eq "Release") {
                "assembleRelease"
            } else {
                "assembleDebug"
            }
            Invoke-Checked ".\gradlew.bat" $gradleTask "--no-daemon"
        } finally {
            Pop-Location
        }

        $variant = $Configuration.ToLowerInvariant()
        $apkOutputDir = Join-Path $mappedRoot "android\app\build\outputs\apk\$variant"
        $sourceApk = Get-ChildItem -LiteralPath $apkOutputDir -Filter "*.apk" -File |
            Sort-Object LastWriteTime -Descending |
            Select-Object -First 1 -ExpandProperty FullName
        if (-not $sourceApk) {
            throw "APK build completed, but no artifact was found under: $apkOutputDir"
        }

        $distDir = Join-Path $mappedRoot "dist"
        New-Item -ItemType Directory -Force -Path $distDir | Out-Null
        $targetApk = Join-Path $distDir "PixelClock-$variant.apk"
        Copy-Item -LiteralPath $sourceApk -Destination $targetApk -Force

        Write-Host ""
        Write-Host "APK ready: $targetApk"
    } finally {
        Pop-Location
    }
} finally {
    if ($mappingCreated) {
        subst $buildDrive /D
    }
}
