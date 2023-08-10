function convertUTF8()
{
    param($Path);
    if (-not [System.IO.File]::Exists($Path))
    {
    throw [System.IO.FileNotFoundException]::new();
    }
    [System.IO.File]::WriteAllText($path,[System.IO.File]::ReadAllText($Path,[System.Text.Encoding]::Default),[System.Text.Encoding]::UTF8);
}
function ConvertAll-UTF8()
{
    param($Pattern)
    $files = [System.IO.Directory]::EnumerateFiles([System.Environment]::CurrentDirectory,$Pattern,[System.IO.SearchOption]::AllDirectories)
    foreach ($file in $files)
    {
    convertUTF8 -Path $file
    Write-Host "fucked " + $file
    }
}
ConvertAll-UTF8 -Pattern *.h
ConvertAll-UTF8 -Pattern *.hpp
ConvertAll-UTF8 -Pattern *.cpp
ConvertAll-UTF8 -Pattern *.vcxproj
ConvertAll-UTF8 -Pattern *.filters