[private]
default:
    just --list --justfile {{ justfile() }}

format:
    indent src/*.c src/*.h -linux -nut -i4

release:
    flatpak-builder --user --install --force-clean build/release build-aux/com.konstantintutsch.Lock.yaml 

devel:
    flatpak-builder --user --install --force-clean build/devel build-aux/com.konstantintutsch.Lock.Devel.yaml 

offline:
    flatpak-builder --user --install --force-clean --disable-download build/devel build-aux/com.konstantintutsch.Lock.Devel.yaml 

setup:
    sudo dnf install -y indent
    sudo dnf install -y meson
    sudo dnf install -y flatpak-builder
    sudo dnf install -y libadwaita-devel
    sudo dnf install -y gpgme-devel
    flatpak install --user --assumeyes org.gnome.Sdk//47 org.gnome.Platform//47
