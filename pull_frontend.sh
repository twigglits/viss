#!/bin/bash
# Script to pull frontend files from ../viss-frontend into the current viss repo
# Usage: bash pull_frontend.sh

FRONTEND_DIR="../viss-frontend"
# Remove app.py and requirements.txt if they exist
rm -f app.py requirements.txt
rm -rf frontend
# Copy app.py
cp -v "$FRONTEND_DIR/app.py" ./
# Copy babel.config.js
cp -v "$FRONTEND_DIR/babel.config.js" ./
# Copy nativewind.d.ts
cp -v "$FRONTEND_DIR/nativewind.d.ts" ./
# Copy requirements.txt
cp -v "$FRONTEND_DIR/requirements.txt" ./
# Copy frontend directory (templates and static)
cp -rv "$FRONTEND_DIR/frontend" ./

echo "Frontend files pulled from $FRONTEND_DIR."
