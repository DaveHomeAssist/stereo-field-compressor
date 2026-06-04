#!/usr/bin/env bash
#
# sign-and-notarize.sh — Developer ID sign + notarize + staple the
# Stereo Field Compressor VST3 + Standalone for macOS distribution.
#
# WHAT IT DOES
#   1. Code-signs each bundle inside-out (NOT --deep) with Hardened Runtime
#      (--options runtime) and a secure timestamp (--timestamp).
#   2. Notarizes each via `xcrun notarytool` (zip with ditto → submit → wait).
#   3. Staples the ticket to each BUNDLE, then packages a stapled, offline-
#      verifiable distribution zip.
#
# ONE-TIME PREREQS
#   - Paid Apple Developer Program ($99/yr) + a "Developer ID Application"
#     certificate installed in your login keychain. Verify:
#         security find-identity -v -p codesigning
#   - Store notary credentials once (keychain profile reused below):
#         xcrun notarytool store-credentials "SFC" \
#           --apple-id "you@example.com" --team-id "YOURTEAMID"
#     (or pass an App Store Connect API key: --key/--key-id/--issuer)
#   - In CMakeLists juce_add_plugin(...), for the Standalone mic prompt:
#         HARDENED_RUNTIME_ENABLED       TRUE
#         MICROPHONE_PERMISSION_ENABLED  TRUE
#         MICROPHONE_PERMISSION_TEXT     "Stereo Field Compressor uses the microphone to process audio input."
#
# USAGE
#   TEAM_ID=ABCDE12345 NOTARY_PROFILE=SFC ./scripts/sign-and-notarize.sh
#   # optional 1st arg = path to the Release artefacts dir
#
set -euo pipefail

# ─── CONFIG — set TEAM_ID + NOTARY_PROFILE (env or here) ─────────────────────
TEAM_ID="${TEAM_ID:-REPLACE_TEAMID}"        # 10-char Apple Developer Team ID
NOTARY_PROFILE="${NOTARY_PROFILE:-SFC}"     # profile name from `notarytool store-credentials`
# Full signing identity (auto-built from TEAM_ID; override with SIGN_ID=... if needed):
SIGN_ID="${SIGN_ID:-Developer ID Application: David Robertson (${TEAM_ID})}"
# ─────────────────────────────────────────────────────────────────────────────

PRODUCT="Stereo Field Compressor"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
REL="${1:-$REPO_DIR/build/StereoFieldCompressor_artefacts/Release}"
ENTITLEMENTS="$SCRIPT_DIR/standalone.entitlements"
DIST="$REPO_DIR/dist"

VST3="$REL/VST3/$PRODUCT.vst3"
APP="$REL/Standalone/$PRODUCT.app"

bold() { printf '\n\033[1m▸ %s\033[0m\n' "$*"; }
die()  { printf '\033[31m✗ ERROR:\033[0m %s\n' "$*" >&2; exit 1; }

# ─── Preflight ───────────────────────────────────────────────────────────────
[[ "$TEAM_ID" == REPLACE_TEAMID ]] && die "Set TEAM_ID (and store a notarytool profile named '$NOTARY_PROFILE')."
[[ -f "$ENTITLEMENTS" ]] || die "Missing entitlements: $ENTITLEMENTS"
[[ -d "$VST3" ]] || die "VST3 not found: $VST3  (build Release first)"
[[ -d "$APP"  ]] || die "Standalone not found: $APP  (build Release first)"
command -v xcrun >/dev/null || die "xcrun not found (install Xcode Command Line Tools)."
security find-identity -v -p codesigning | grep -q "$TEAM_ID" \
  || die "No Developer ID codesigning identity for team $TEAM_ID in keychain."

# ─── Sign (inside-out; never --deep) ────────────────────────────────────────
sign_bundle() {                       # $1 = bundle, $2 = optional entitlements file
  local bundle="$1" ents="${2:-}"
  codesign --force --timestamp --options runtime -s "$SIGN_ID" "$bundle/Contents/MacOS/$PRODUCT"
  if [[ -n "$ents" ]]; then
    codesign --force --timestamp --options runtime --entitlements "$ents" -s "$SIGN_ID" "$bundle"
  else
    codesign --force --timestamp --options runtime -s "$SIGN_ID" "$bundle"
  fi
  codesign --verify --strict --verbose=2 "$bundle"
}

bold "Signing VST3 (plugin needs no entitlements)…"
sign_bundle "$VST3"
bold "Signing Standalone .app (audio-input entitlement)…"
sign_bundle "$APP" "$ENTITLEMENTS"

# ─── Notarize each bundle: zip → submit --wait → staple the bundle ──────────
notarize() {                          # $1 = bundle
  local bundle="$1" name tmp zip
  name="$(basename "$bundle")"
  tmp="$(mktemp -d)"; zip="$tmp/${name}.zip"
  ditto -c -k --keepParent "$bundle" "$zip"        # bundle-safe zip (NOT plain zip)
  bold "Notarizing $name …"
  xcrun notarytool submit "$zip" --keychain-profile "$NOTARY_PROFILE" --wait
  xcrun stapler staple "$bundle"                   # staple the BUNDLE (you cannot staple a zip)
  xcrun stapler validate -v "$bundle"
  rm -rf "$tmp"
}

notarize "$VST3"
notarize "$APP"

# ─── Package stapled, offline-verifiable distribution zip ───────────────────
bold "Packaging distributable…"
ARCH="$(lipo -archs "$VST3/Contents/MacOS/$PRODUCT" | tr ' ' '-')"
PKG_ROOT="$DIST/StereoFieldCompressor-macOS-$ARCH"
rm -rf "$DIST"; mkdir -p "$PKG_ROOT"
cp -R "$VST3" "$APP" "$PKG_ROOT/"
ditto -c -k --keepParent "$PKG_ROOT" "$DIST/StereoFieldCompressor-macOS-$ARCH.zip"

# ─── Final Gatekeeper assessment ────────────────────────────────────────────
bold "Gatekeeper assessment (expect: accepted / source=Notarized Developer ID)…"
spctl -a -vvv "$APP" || true   # the .app is the meaningful check; plugins are assessed by their host

bold "Done → $DIST/StereoFieldCompressor-macOS-$ARCH.zip"
echo "   (verify a plugin anywhere with:  xcrun stapler validate -v <path>.vst3 )"
