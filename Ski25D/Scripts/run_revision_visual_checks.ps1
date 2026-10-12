param([string]$Mode='scenery')
$ErrorActionPreference='Stop'
$projectRoot=Split-Path $PSScriptRoot -Parent
$editor='D:/UE_5.5/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$projectFile=Join-Path $projectRoot 'Ski25D.uproject'
if($Mode -eq 'scenery') {
    foreach($map in @('SkiRimeMap','SkiBeidahuMap','SkiChangbaiMap')) {
        & $editor $projectFile "/Game/Ski25D/Maps/$map" -game -RenderOffscreen -SkiPreviewTest -SkiArtPreview -unattended -ResX=1280 -ResY=720 -windowed -NoVSync -ForceRes "-abslog=$projectRoot/Saved/Logs/Revision_$map.log"
        if($LASTEXITCODE -ne 0){throw "Preview failed: $map"}
        Write-Output "SCENERY_CAPTURED $map"
    }
} elseif($Mode -eq 'beidahu') {
    & $editor $projectFile /Game/Ski25D/Maps/SkiBeidahuMap -game -RenderOffscreen -SkiPreviewTest -SkiArtPreview -unattended -ResX=1280 -ResY=720 -windowed -NoVSync -ForceRes "-abslog=$projectRoot/Saved/Logs/Revision_SkiBeidahuMap.log"
    if($LASTEXITCODE -ne 0){throw 'Preview failed'}
} elseif($Mode -eq 'signs') {
    foreach($map in @('SkiRimeMap','SkiBeidahuMap','SkiChangbaiMap')) {
        & $editor $projectFile "/Game/Ski25D/Maps/$map" -game -RenderOffscreen -SkiPreviewTest -SkiSignReview -unattended -ResX=1280 -ResY=720 -windowed -NoVSync -ForceRes "-abslog=$projectRoot/Saved/Logs/Signs_$map.log"
        if($LASTEXITCODE -ne 0){throw "Sign preview failed: $map"}
        Write-Output "SIGNS_CAPTURED $map"
    }
} elseif($Mode -eq 'menu') {
    & $editor $projectFile /Game/Ski25D/Maps/SkiChangbaiMap -game -RenderOffscreen -SkiMenuReview -unattended -ResX=1280 -ResY=720 -windowed -NoVSync -ForceRes "-abslog=$projectRoot/Saved/Logs/MenuReview.log"
    if($LASTEXITCODE -ne 0){throw 'Menu check failed'}
} elseif($Mode -eq 'physics') {
    & $editor $projectFile /Game/Ski25D/Maps/SkiChangbaiMap -game -nullrhi -SkiLandingCheck -unattended "-abslog=$projectRoot/Saved/Logs/RevisionLanding.log"
    if($LASTEXITCODE -ne 0){throw 'Landing check failed'}
    & $editor $projectFile /Game/Ski25D/Maps/SkiRimeMap -game -nullrhi -benchmark -fps=60 -SkiSmokeTest -SkiCampaignTest -unattended "-abslog=$projectRoot/Saved/Logs/RevisionCampaign.log"
    if($LASTEXITCODE -ne 0){throw 'Campaign check failed'}
} else {throw 'Unknown test mode'}
