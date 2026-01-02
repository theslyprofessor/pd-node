#!/bin/bash
# Install pd-node to Plugdata

set -e

REPO_DIR="$HOME/Code/github.com/theslyprofessor/pd-node"
PLUGDATA_EXT="$HOME/Library/Application Support/plugdata/externals/pd-node"

echo "Installing pd-node to Plugdata..."
echo ""

# Create directory
mkdir -p "$PLUGDATA_EXT"

# Copy files
echo "Copying binary..."
cp "$REPO_DIR/binaries/arm64-macos/node.pd_darwin" "$PLUGDATA_EXT/"

echo "Copying help file..."
cp "$REPO_DIR/binaries/arm64-macos/node-help.pd" "$PLUGDATA_EXT/"

echo "Copying pd-api package..."
cp -r "$REPO_DIR/binaries/arm64-macos/pd-api" "$PLUGDATA_EXT/"

echo "Copying examples..."
cp -r "$REPO_DIR/examples" "$PLUGDATA_EXT/"

echo ""
echo "✅ Installed to: $PLUGDATA_EXT"
echo ""
echo "Files installed:"
ls -lh "$PLUGDATA_EXT/"
echo ""
echo "Next steps:"
echo "1. Open Plugdata"
echo "2. Create a new patch"
echo "3. Create object: [node]"
echo "4. Check console for initialization message"
echo ""
echo "⚠️  Note: Script execution not working yet (Phase 2 IPC bridge needed)"
echo ""
echo "To test runtime detection:"
echo "  cd ~/Code/github.com/theslyprofessor/pd-node"
echo "  ./test_detector"
