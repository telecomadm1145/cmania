
mkdir ".\ReleasePackage"

mkdir ".\ReleasePackage\x86"
cp -Recurse .\Release\* .\ReleasePackage\x86
cp -Recurse .\cmania\Samples .\ReleasePackage\x86

mkdir ".\ReleasePackage\x64"
cp -Recurse .\x64\Release\* .\ReleasePackage\x64
cp -Recurse .\cmania\Samples .\ReleasePackage\x64

(".\ReleasePackage\x64",".\ReleasePackage\x86") | Compress-Archive -DestinationPath Release.zip -Force

del -Recurse ReleasePackage