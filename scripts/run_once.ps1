# Usage:  .\scripts\run_once.ps1 [config\config.json]
param(
    [string]$ConfigPath = "config\config.json"
)
$ErrorActionPreference = "Stop"
Set-Location (Join-Path $PSScriptRoot "..")

if (Test-Path ".env") {
    Get-Content ".env" | ForEach-Object {
        if ($_ -match '^\s*([^#][^=]*)=(.*)$') {
            $k = $Matches[1].Trim()
            $v = $Matches[2].Trim().Trim('"')
            [Environment]::SetEnvironmentVariable($k, $v, "Process")
        }
    }
}

if (-not $env:TWELVEDATA_API_KEY) {
    Write-Error "TWELVEDATA_API_KEY not set (put it in .env)"
    exit 1
}

& ".\build\gold_smc.exe" $ConfigPath