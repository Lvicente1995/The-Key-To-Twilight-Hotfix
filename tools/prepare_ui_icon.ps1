# Optional Windows authoring helper. Platform builds consume the prepared PNG.
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$project = Split-Path $PSScriptRoot -Parent
$sourcePath = Join-Path (Split-Path $project -Parent) 'model\kingdom-key-icon-source.png'
$destination = Join-Path $project 'res\ui\tex1_48x48_7a16cbeebf26f7d6_3b746898124fa62c_9.png'
$source = [Drawing.Image]::FromFile($sourcePath)
$target = [Drawing.Bitmap]::new(192, 192, [Drawing.Imaging.PixelFormat]::Format32bppArgb)
$graphics = [Drawing.Graphics]::FromImage($target)
try {
    if ($source.Width -ne $source.Height) { throw 'Expected square icon artwork.' }
    $graphics.CompositingMode = [Drawing.Drawing2D.CompositingMode]::SourceCopy
    $graphics.CompositingQuality = [Drawing.Drawing2D.CompositingQuality]::HighQuality
    $graphics.InterpolationMode = [Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $graphics.PixelOffsetMode = [Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $graphics.Clear([Drawing.Color]::Transparent)
    $graphics.DrawImage($source, [Drawing.Rectangle]::new(0, 0, 192, 192), 0, 0, $source.Width, $source.Height, [Drawing.GraphicsUnit]::Pixel)
    [IO.Directory]::CreateDirectory((Split-Path $destination -Parent)) | Out-Null
    $target.Save($destination, [Drawing.Imaging.ImageFormat]::Png)
    Write-Output $destination
} finally {
    $graphics.Dispose()
    $target.Dispose()
    $source.Dispose()
}
