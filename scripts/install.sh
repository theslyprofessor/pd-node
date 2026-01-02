#!/bin/bash
# Install pd-node to Pure Data and/or Plugdata

set -e

REPO_DIR="$HOME/Code/github.com/theslyprofessor/pd-node"
PD_EXT_DIR="$HOME/Documents/Pd/externals"
PLUGDATA_EXT_DIR="$HOME/Library/Application Support/plugdata/externals"

install_to_pd() {
    echo "Installing pd-node to Pure Data..."
    mkdir -p "$PD_EXT_DIR"
    
    # Pure Data doesn't search subdirectories by default
    # So we install directly to externals/ directory
    cp "$REPO_DIR/binaries/arm64-macos/node.pd_darwin" "$PD_EXT_DIR/"
    cp "$REPO_DIR/binaries/arm64-macos/node-help.pd" "$PD_EXT_DIR/"
    
    # Also create pd-node subdirectory for pd-api and examples
    mkdir -p "$PD_EXT_DIR/pd-node"
    cp -r "$REPO_DIR/binaries/arm64-macos/pd-api" "$PD_EXT_DIR/pd-node/"
    cp -r "$REPO_DIR/examples" "$PD_EXT_DIR/pd-node/"
    
    echo "✅ Installed to Pure Data:"
    echo "   Binary: $PD_EXT_DIR/node.pd_darwin"
    echo "   Help: $PD_EXT_DIR/node-help.pd"
    echo "   Support files: $PD_EXT_DIR/pd-node/"
    echo ""
}

install_to_plugdata() {
    echo "Installing pd-node to Plugdata..."
    local dest="$PLUGDATA_EXT_DIR/pd-node"
    mkdir -p "$dest"
    
    # Plugdata handles subdirectories better
    cp "$REPO_DIR/binaries/arm64-macos/node.pd_darwin" "$dest/"
    cp "$REPO_DIR/binaries/arm64-macos/node-help.pd" "$dest/"
    cp -r "$REPO_DIR/binaries/arm64-macos/pd-api" "$dest/"
    cp -r "$REPO_DIR/examples" "$dest/"
    
    echo "✅ Installed to Plugdata:"
    echo "   $dest/"
    echo ""
}

# Check what's installed
HAS_PD=false
HAS_PLUGDATA=false

if [ -d "/Applications/Pd-0.54-1.app" ] || [ -d "~/Documents/Pd" ]; then
    HAS_PD=true
fi

if [ -d "/Applications/plugdata.app" ]; then
    HAS_PLUGDATA=true
fi

echo "pd-node Installer"
echo "================="
echo ""
echo "Detected installations:"
[ "$HAS_PD" = true ] && echo "  ✅ Pure Data"
[ "$HAS_PLUGDATA" = true ] && echo "  ✅ Plugdata"
echo ""

# Install to both if available
if [ "$HAS_PD" = true ]; then
    install_to_pd
fi

if [ "$HAS_PLUGDATA" = true ]; then
    install_to_plugdata
fi

if [ "$HAS_PD" = false ] && [ "$HAS_PLUGDATA" = false ]; then
    echo "⚠️  No Pure Data or Plugdata installation found"
    echo "Please install one of:"
    echo "  - Pure Data: https://puredata.info/downloads"
    echo "  - Plugdata: https://plugdata.org"
    exit 1
fi

echo "Installation complete!"
echo ""
echo "IMPORTANT: You MUST restart Pure Data/Plugdata for changes to take effect!"
echo ""
echo "Next steps:"
echo "1. Close Pure Data/Plugdata completely (Cmd+Q)"
echo "2. Reopen the application"
echo "3. Create a new patch"
echo "4. Create object: [node]"
echo "5. Should create without 'couldn't create' error"
echo ""
echo "⚠️  Note: Script execution not working yet (Phase 2 needed)"
echo "   Objects will create, but scripts won't execute until Phase 2 is implemented."
echo ""
echo "To verify runtime detection:"
echo "  cd $REPO_DIR"
echo "  ./test_detector"
