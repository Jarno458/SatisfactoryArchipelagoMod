#!/usr/bin/env bash
set -euo pipefail

############################################
# 0. Install basic build tools (Debian/Ubuntu)
############################################

if command -v apt >/dev/null 2>&1; then
    echo "Detected apt; installing build tools (requires sudo)..."
    sudo apt update
    sudo apt install -y build-essential git cmake curl wget
else
    echo "WARNING: 'apt' not found. Please ensure the following are installed:"
    echo "  - build-essential (or equivalent)"
    echo "  - git"
    echo "  - cmake"
    echo "  - curl or wget"
    echo "Continuing without automatic package installation..."
fi

############################################
# 1. Optional: download Satisfactory Mod Loader (SML)
############################################

echo
echo "Optional: download the Satisfactory Mod Loader (engine/modding assets)."
echo "This requires GitHub CLI ('gh') and access to the satisfactorymodding org."
echo "Repo: https://github.com/satisfactorymodding/SatisfactoryModLoader"
echo
echo "SML will be cloned into the top-level folder:"
echo "  ./SatisfactoryModLoader-master"
echo

read -r -p "Do you want to download/update the Satisfactory Mod Loader now? [y/N]: " INSTALL_SML

case "$INSTALL_SML" in
    [yY]|[yY][eE][sS])
        # Ensure GitHub CLI exists
        if ! command -v gh >/dev/null 2>&1; then
            echo "'gh' (GitHub CLI) not found. Attempting to install via apt..."
            if command -v apt >/dev/null 2>&1; then
                if ! (sudo apt update && sudo apt install -y gh); then
                    echo "ERROR: Failed to install GitHub CLI via apt."
                    exit 1
                fi
            else
                echo "ERROR: 'gh' not found and this script can only auto-install on apt systems."
                exit 1
            fi
        fi

        # Ensure GitHub CLI is authenticated
        if ! gh auth status >/dev/null 2>&1; then
            echo
            echo "GitHub CLI not authenticated. Running 'gh auth login'..."
            gh auth login
        fi

        echo
        if [ -d "SatisfactoryModLoader-master" ]; then
            echo "SatisfactoryModLoader-master already exists, skipping clone."
            echo "You can 'cd SatisfactoryModLoader-master && git pull' manually if desired."
        else
            echo "Cloning SatisfactoryModLoader into ./SatisfactoryModLoader-master ..."
            gh repo clone satisfactorymodding/SatisfactoryModLoader "SatisfactoryModLoader-master"
        fi
        ;;
    *)
        echo "Skipping Satisfactory Mod Loader download."
        ;;
esac

############################################
# 2. Clone SatisfactoryArchipelagoMod and APCpp
############################################

if [ ! -d "SatisfactoryArchipelagoMod" ]; then
    git clone https://github.com/Jarno458/SatisfactoryArchipelagoMod
else
    echo "SatisfactoryArchipelagoMod already exists, skipping clone."
fi

if [ ! -d "APCpp" ]; then
    git clone https://github.com/N00byKing/APCpp
else
    echo "APCpp already exists, skipping clone."
fi

cd APCpp
git submodule update --init --recursive

############################################
# 3. Patch CMakeLists.txt for static build
############################################

PATCH_FILE="../APCpp-static-build.patch"
PATCH_URL="https://github.com/adamboy7/APCpp/commit/19517a9.patch"

if [ -f CMakeLists.txt ]; then
    echo "Found CMakeLists.txt, preparing to patch for static build..."

    # Prefer local patch file; download if missing
    if [ -f "$PATCH_FILE" ]; then
        echo "Using local patch: $PATCH_FILE"
    else
        echo "Local patch '$PATCH_FILE' not found. Attempting to download from:"
        echo "  $PATCH_URL"

        if command -v curl >/dev/null 2>&1; then
            echo "Downloading with curl..."
            if ! curl -fL "$PATCH_URL" -o "$PATCH_FILE"; then
                echo "ERROR: Failed to download patch from:"
                echo "  $PATCH_URL"
                exit 1
            fi
        elif command -v wget >/dev/null 2>&1; then
            echo "Downloading with wget..."
            if ! wget -O "$PATCH_FILE" "$PATCH_URL"; then
                echo "ERROR: Failed to download patch from:"
                echo "  $PATCH_URL"
                exit 1
            fi
        else
            echo "ERROR: Neither 'curl' nor 'wget' is available to download the patch."
            echo "       Install one of them or place '$PATCH_FILE' manually."
            exit 1
        fi
    fi

    echo "Checking if patch applies cleanly..."
    if git apply --check "$PATCH_FILE"; then
        echo "Patch applies cleanly. Applying..."
        git apply "$PATCH_FILE"
        echo "Patch applied successfully."
    else
        echo "WARNING: Patch does not apply cleanly to current CMakeLists.txt."
        echo "Upstream may have changed. Skipping automatic modification."
        echo "You may need to inspect CMakeLists.txt and apply equivalent changes manually."
        # We intentionally do NOT exit here: "skip this step" behavior.
    fi
else
    echo "ERROR: CMakeLists.txt not found in APCpp."
    exit 1
fi

############################################
# 4. Configure and build (Release)
############################################

mkdir -p build
cd build

cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .

############################################
# 5. Copy built static libraries into SatisfactoryArchipelagoMod
############################################

# We are currently in APCpp/build
MOD_LIB_DIR="../../SatisfactoryArchipelagoMod/Source/APCpp/lib/Linux"
MBEDTLS_DIR="$MOD_LIB_DIR/mbedtls"

mkdir -p "$MOD_LIB_DIR"
mkdir -p "$MBEDTLS_DIR"

cp libAPCpp_static.a                        "$MOD_LIB_DIR/libAPCpp.a"
cp IXWebSocket/libixwebsocket.a             "$MOD_LIB_DIR/libixwebsocket.a"
cp jsoncpp/src/lib_json/libjsoncpp.a        "$MOD_LIB_DIR/libjsoncpp.a"
cp _deps/zlib-build/libz.a                  "$MOD_LIB_DIR/libz.a"

cp mbedtls_bin/library/libmbedtls.a         "$MBEDTLS_DIR/libmbedtls.a"
cp mbedtls_bin/library/libmbedcrypto.a      "$MBEDTLS_DIR/libmbedcrypto.a"
cp mbedtls_bin/library/libmbedx509.a        "$MBEDTLS_DIR/libmbedx509.a"

echo "Done. Static libraries copied into SatisfactoryArchipelagoMod."

# Return to top-level (where this script was run)
cd ../..

############################################
# 6. Optional: download Satisfactory mod dependencies (top-level)
############################################

echo
echo "Optional: clone recommended Satisfactory mod dependencies into the top-level directory:"
echo "  - ContentLib"
echo "  - MAMTips"
echo "  - FreeSamples"
echo "  - FixClientResourceSinkPoints"
echo "  - HoverpackFuseReminder"
echo
echo "These folders will be siblings of SatisfactoryArchipelagoMod."
echo

read -r -p "Do you want to install these mod dependencies now? [y/N]: " INSTALL_DEPS

if [ "$INSTALL_DEPS" = "y" ] || [ "$INSTALL_DEPS" = "Y" ] || [ "$INSTALL_DEPS" = "yes" ] || [ "$INSTALL_DEPS" = "Yes" ]; then
    echo "Cloning dependencies into top-level: $(pwd)"

    # ContentLib
    if [ -d "ContentLib" ]; then
        echo "  ContentLib already exists, skipping clone."
    else
        git clone https://github.com/Nogg-aholic/ContentLib "ContentLib"
    fi

    # MAMTips
    if [ -d "MAMTips" ]; then
        echo "  MAMTips already exists, skipping clone."
    else
        git clone https://github.com/Nogg-aholic/MAMTips "MAMTips"
    fi

    # FreeSamples
    if [ -d "FreeSamples" ]; then
        echo "  FreeSamples already exists, skipping clone."
    else
        git clone https://github.com/budak7273/FreeSamples "FreeSamples"
    fi

    # FixClientResourceSinkPoints
    if [ -d "FixClientResourceSinkPoints" ]; then
        echo "  FixClientResourceSinkPoints already exists, skipping clone."
    else
        git clone https://github.com/budak7273/FixClientResourceSinkPoints "FixClientResourceSinkPoints"
    fi

    # HoverpackFuseReminder
    if [ -d "HoverpackFuseReminder" ]; then
        echo "  HoverpackFuseReminder already exists, skipping clone."
    else
        git clone https://github.com/budak7273/HoverpackFuseReminder "HoverpackFuseReminder"
    fi

    echo "Dependency install step complete."
else
    echo "Skipping dependency installation."
fi

############################################
# 7. If SML exists, copy mods into SatisfactoryModLoader-master/Mods
############################################

echo
SML_DIR="SatisfactoryModLoader-master"
SML_MODS_DIR="$SML_DIR/Mods"

if [ -d "$SML_DIR" ]; then
    echo "Detected '$SML_DIR'. Preparing to copy mods into '$SML_MODS_DIR'..."
    mkdir -p "$SML_MODS_DIR"

    MOD_FOLDERS=(
        "SatisfactoryArchipelagoMod"
        "ContentLib"
        "MAMTips"
        "FreeSamples"
        "FixClientResourceSinkPoints"
        "HoverpackFuseReminder"
    )

    for mod in "${MOD_FOLDERS[@]}"; do
        if [ -d "$mod" ]; then
            if [ -d "$SML_MODS_DIR/$mod" ]; then
                echo "  $mod already exists in $SML_MODS_DIR, skipping copy."
            else
                echo "  Copying $mod -> $SML_MODS_DIR/$mod"
                cp -r "$mod" "$SML_MODS_DIR/$mod"
            fi
        else
            echo "  WARNING: '$mod' not found in top-level; skipping copy for this mod."
        fi
    done

    echo "Mod copy step into SML complete."
else
    echo "SatisfactoryModLoader-master not present; skipping copy of mods into SML/Mods."
fi

############################################
# 8. Optional: install Satisfactory Dedicated Server via steamcmd
############################################

echo
echo "Optional: download or update a Satisfactory Dedicated Server using steamcmd."
echo "It will be installed to: ~/SatisfactoryDedicatedServer"
echo

read -r -p "Do you want to install/update Satisfactory Dedicated Server now? [y/N]: " INSTALL_SDS

case "$INSTALL_SDS" in
    [yY]|[yY][eE][sS])
        # Ensure steamcmd exists
        if ! command -v steamcmd >/dev/null 2>&1; then
            echo "steamcmd not found. Attempting to install (apt-based distros only)..."
            if command -v apt >/dev/null 2>&1; then
                if ! (sudo add-apt-repository multiverse -y && sudo dpkg --add-architecture i386 && sudo apt update && sudo apt install -y steamcmd); then
                    echo "ERROR: Failed to install steamcmd via apt."
                    echo "Please install steamcmd manually and rerun this step."
                    exit 1
                fi
            else
                echo "ERROR: steamcmd not found and automatic install is only supported on apt-based distros."
                echo "Please install steamcmd manually and rerun this step."
                exit 1
            fi
        fi

        # Prime/update steamcmd, then install server
        steamcmd +quit
        steamcmd +force_install_dir ~/SatisfactoryDedicatedServer +login anonymous +app_update 1690800 validate +quit
        ;;
    *)
        echo "Skipping Satisfactory Dedicated Server install/update."
        ;;
esac

echo
echo "SatisfactoryArchipelagoMod set up. Please copy or share your files to a windows machine to set up Wwise and generate VS build files."
echo "After installing dependencies It's entirely possible run the modded satisfactory server inside of WSL for testing, just be advised that a windows client will be unable to see a WSL server socket."
echo "All done."
