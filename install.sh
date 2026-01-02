#!/bin/bash
# Install pd-node to Pure Data and/or Plugdata

set -e

REPO_DIR="$HOME/Code/github.com/theslyprofessor/pd-node"
PD_EXT="$HOME/Documents/Pd/externals/pd-node"
PLUGDATA_EXT="$HOME/Library/Application Support/plugdata/externals/pd-node"

install_to() {
    local dest="$1"
    local name="$2"
    
    echo "Installing pd-node to $name..."
    mkdir -p "$dest"
    
    cp "$REPO_DIR/binaries/arm64-macos/node.pd_darwin" "$dest/"
    cp "$REPO_DIR/binaries/arm64-macos/node-help.pd" "$dest/"
    cp -r "$REPO_DIR/binaries/arm64-macos/pd-api" "$dest/"
    cp -r "$REPO_DIR/examples" "$dest/"
    
    echo "✅ Installed to: $dest"
    ls -lh "$dest/" | grep -v total
    echo ""
}

# Check what's installed
HAS_PD=false
HAS_PLUGDATA=false

if [ -d "/Applications/Pd-0.54-1.app" ]; then
    HAS_PD=true
fi

if [ -d "/Applications/plugdata.app" ]; then
    HAS_PLUGDATA=true
fi

echo "Detected installations:"
[ "$HAS_PD" = true ] && echo "  ✅ Pure Data (Pd-0.54-1)"
[ "$HAS_PLUGDATA" = true ] && echo "  ✅ Plugdata"
echo ""

# Install to both if available
if [ "$HAS_PD" = true ]; then
    install_to "$PD_EXT" "Pure Data"
fi

if [ "$HAS_PLUGDATA" = true ]; then
    install_to "$PLUGDATA_EXT" "Plugdata"
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
echo "Next steps:"
echo "1. Open Pure Data or Plugdata"
echo "2. Create a new patch"
echo "3. Create object: [node]"
echo "4. Check console for initialization"
echo ""
echo "⚠️  Note: Script execution not working yet (Phase 2 needed)"
echo ""
echo "To test runtime detection:"
echo "  cd $REPO_DIR"
echo "  ./test_detector"
