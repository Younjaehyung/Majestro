# 게임 창을 프로세스 밖에서 찍는다. 게임 코드는 이 스크립트를 모른다.
#   -Dir   : 저장 폴더
#   -Log   : qa_log.txt 경로. FAIL/WARN 줄이 새로 생기면 그때도 찍는다.
#   -Every : 주기 캡처 간격(초)
# Majestro 프로세스가 사라지면 끝난다.
param([string]$Dir, [string]$Log, [int]$Every = 20, [string]$Process = "Majestro")

Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class QaWin {
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr dc, uint flags);
    [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
}
"@
[QaWin]::SetProcessDPIAware() | Out-Null
New-Item -ItemType Directory -Force $Dir | Out-Null

function Save-Shot([string]$name) {
    $p = Get-Process $Process -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowHandle -ne 0 } | Select-Object -First 1
    if (-not $p) { return $false }
    $r = New-Object QaWin+RECT
    [QaWin]::GetClientRect($p.MainWindowHandle, [ref]$r) | Out-Null
    if ($r.R -le 0 -or $r.B -le 0) { return $true }
    $bmp = New-Object System.Drawing.Bitmap($r.R, $r.B)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $dc = $g.GetHdc()
    # 1 = 클라이언트 영역만, 2 = PW_RENDERFULLCONTENT (DX 스왑체인 내용까지)
    [QaWin]::PrintWindow($p.MainWindowHandle, $dc, 3) | Out-Null
    $g.ReleaseHdc($dc); $g.Dispose()
    # ponytail: 원본 해상도 JPEG 그대로. 용량이 문제면 여기서 축소한다.
    $bmp.Save((Join-Path $Dir $name), [System.Drawing.Imaging.ImageFormat]::Jpeg)
    $bmp.Dispose()
    return $true
}

$start = Get-Date
$nextPeriodic = 0
$seen = 0
while ($true) {
    $t = [int]((Get-Date) - $start).TotalSeconds
    $name = $null
    if ($Log -and (Test-Path $Log)) {
        $hits = @(Select-String -Path $Log -Pattern " (FAIL|WARN) " -Encoding UTF8 | Where-Object { $_.Line -notmatch "SUMMARY" })
        if ($hits.Count -gt $seen) {
            $key = ($hits[-1].Line -replace '^.*? (FAIL|WARN) (\S+).*$', '$1_$2')
            $seen = $hits.Count
            $name = "{0:D4}_{1}.jpg" -f $t, $key
        }
    }
    if (-not $name -and $t -ge $nextPeriodic) { $name = "{0:D4}.jpg" -f $t }
    if ($name) {
        if (-not (Save-Shot $name)) { if ($t -gt 30) { break } }
        if ($t -ge $nextPeriodic) { $nextPeriodic = $t + $Every }
    }
    Start-Sleep -Milliseconds 500
}
