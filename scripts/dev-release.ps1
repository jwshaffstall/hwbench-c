$ErrorActionPreference = 'Stop'

& (Join-Path $PSScriptRoot 'dev-common.ps1') -Config 'Release'
