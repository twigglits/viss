#!/bin/bash
# Script to push frontend files from the current viss repo to ../viss-frontend
# Usage: bash push_frontend.sh

FRONTEND_DIR="../viss-frontend"

# Remove destination files/directories if they exist
rm -f "$FRONTEND_DIR/app.py" "$FRONTEND_DIR/requirements.txt"
rm -rf "$FRONTEND_DIR/frontend"

# Copy app.py
cp -v ./app.py "$FRONTEND_DIR/"
# Copy requirements.txt
cp -v ./requirements.txt "$FRONTEND_DIR/"
# Copy frontend directory (templates and static)
cp -rv ./frontend "$FRONTEND_DIR/"

echo "Frontend files pushed to $FRONTEND_DIR."
