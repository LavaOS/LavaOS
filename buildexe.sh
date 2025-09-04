APPS=("open.exe" "hello.exe" "freever.exe" "lsbin.exe" "bsod.exe")
for app in "${APPS[@]}"; do
  i686-w64-mingw32-gcc -nostdlib -o "$app" "applications/${app%.exe}.c" applications/appinclude/print.c -Iapplications/appinclude
done

i686-w64-mingw32-gcc -nostdlib -o stop.exe applications/shutdown.c
i686-w64-mingw32-gcc -nostdlib -o reboot.exe applications/reboot.c
