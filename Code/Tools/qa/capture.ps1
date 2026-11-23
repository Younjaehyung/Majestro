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
    [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr after, int x, int y, int cx, int cy, uint flags);
}
"@
[QaWin]::SetProcessDPIAware() | Out-Null
New-Item -ItemType Directory -Force $Dir | Out-Null

function Save-Shot([string]$name) {
    # 멀티 실행이면 창이 여러 개다. 먼저 뜬 순서로 p1(방장), p2, p3.
    $procs = @(Get-Process $Process -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowHandle -ne 0 } | Sort-Object StartTime)
    if ($procs.Count -eq 0) { return $false }
    for ($i = 0; $i -lt $procs.Count; $i++) {
        $h = $procs[$i].MainWindowHandle
        $r = New-Object QaWin+RECT
        [QaWin]::GetClientRect($h, [ref]$r) | Out-Null
        if ($r.R -le 0 -or $r.B -le 0) { continue }
        if ($procs.Count -gt 1) {
            # 다른 창에 가려진 창은 검게 찍힌다. 찍는 동안만 맨 위로 올린다 (-1 TOPMOST, -2 NOTOPMOST, 0x13 = 위치·크기 유지 + 활성화 안 함).
            [QaWin]::SetWindowPos($h, [IntPtr](-1), 0, 0, 0, 0, 0x13) | Out-Null
            Start-Sleep -Milliseconds 400
        }
        $bmp = New-Object System.Drawing.Bitmap($r.R, $r.B)
        $g = [System.Drawing.Graphics]::FromImage($bmp)
        $dc = $g.GetHdc()
        # 1 = 클라이언트 영역만, 2 = PW_RENDERFULLCONTENT (DX 스왑체인 내용까지)
        [QaWin]::PrintWindow($h, $dc, 3) | Out-Null
        $g.ReleaseHdc($dc); $g.Dispose()
        if ($procs.Count -gt 1) { [QaWin]::SetWindowPos($h, [IntPtr](-2), 0, 0, 0, 0, 0x13) | Out-Null }
        $file = if ($procs.Count -gt 1) { $name -replace '\.jpg$', ("_p{0}.jpg" -f ($i + 1)) } else { $name }
        # ponytail: 원본 해상도 JPEG 그대로. 용량이 문제면 여기서 축소한다.
        $bmp.Save((Join-Path $Dir $file), [System.Drawing.Imaging.ImageFormat]::Jpeg)
        $bmp.Dispose()
    }
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
