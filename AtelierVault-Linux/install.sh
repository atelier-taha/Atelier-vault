#!/bin/bash
set -e

# Target directories
INSTALL_DIR="$HOME/.local/bin"
DESKTOP_DIR="$HOME/.local/share/applications"
ICON_DIR="$HOME/.local/share/icons"

# Create directories if they don't exist
mkdir -p "$INSTALL_DIR"
mkdir -p "$DESKTOP_DIR"
mkdir -p "$ICON_DIR"

# Copy binary & icon
cp AtelierVault "$INSTALL_DIR/AtelierVault"
chmod +x "$INSTALL_DIR/AtelierVault"

if [ -f "icon.png" ]; then
    cp icon.png "$ICON_DIR/ateliervault.png"
fi

# Create .desktop entry for system menu integration
cat <<EOF > "$DESKTOP_DIR/ateliervault.desktop"
[Desktop Entry]
Name=Atelier's Vault
Comment=Secure Password Manager
Exec=$INSTALL_DIR/AtelierVault
Icon=ateliervault
Terminal=false
Type=Application
Categories=Utility;Security;
EOF

chmod +x "$DESKTOP_DIR/ateliervault.desktop"

echo "✅ Atelier's Vault installed successfully!"
echo "You can now run it from your application menu or terminal using 'AtelierVault'."