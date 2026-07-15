SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

if [[ ! -d "$SCRIPT_DIR/dsdl_generated" ]]; then
    python3 "$SCRIPT_DIR/dronecan_dsdlc/dronecan_dsdlc.py" -O "$SCRIPT_DIR/dsdl_generated" "$SCRIPT_DIR/DSDL/dronecan" "$SCRIPT_DIR/DSDL/uavcan" "$SCRIPT_DIR/DSDL/com" "$SCRIPT_DIR/DSDL/ardupilot"
fi
