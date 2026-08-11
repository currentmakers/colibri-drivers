#!/usr/bin/env python3
"""Build a Colibri IO EEPROM image."""

import argparse
import struct
import sys

# EEPROM layout offsets (see colibri-sdk/colibri-io-eeprom.h)
OFF_FINGERPRINT      = 0x0000
OFF_SERIAL_NUMBER    = 0x0004
OFF_VENDOR_ID        = 0x0010
OFF_VENDOR_MODEL_ID  = 0x0014
OFF_VENDOR_REVISION  = 0x0018
OFF_VENDOR_NAME_PTR  = 0x0020
OFF_VENDOR_NAME_LEN  = 0x0024
OFF_VENDOR_MODEL_PTR = 0x0028
OFF_VENDOR_MODEL_LEN = 0x002C
OFF_PRODUCT_LINK_PTR = 0x0030
OFF_PRODUCT_LINK_LEN = 0x0034
OFF_DOC_LINK_PTR     = 0x0038
OFF_DOC_LINK_LEN     = 0x003C
OFF_CODE_PIC_LEN     = 0x0040
OFF_TEXT_AREA        = 0x0200
OFF_PIC_ARM          = 0x4000

TEXT_AREA_SIZE = 2048
PIC_ARM_SIZE   = 8192
EEPROM_SIZE    = 0x6000  # 24576 bytes, full layout

TEXT_AREA_START = OFF_TEXT_AREA  # 0x200, absolute pointer base into text_area

FINGERPRINT = 0xDEADFACE
VENDOR_ID   = 1
FIRST_SERIAL = 100


def cstr(s: str) -> bytes:
    """Encode a string with a single NUL terminator (matches sizeof of a C literal)."""
    return s.encode("utf-8") + b"\x00"


def slug(model: str) -> str:
    return model.lower().replace(" ", "-")


def next_serial(model: str) -> int:
    """Read (and increment) the per-model serial counter file, starting at FIRST_SERIAL."""
    path = f"{slug(model)}.serial"
    try:
        with open(path) as f:
            serial = int(f.read().strip())
    except FileNotFoundError:
        serial = FIRST_SERIAL
    with open(path, "w") as f:
        f.write(str(serial + 1))
    return serial


def build_eeprom(model, vendor_model_id, vendor_revision, vendor, serial_number, pic_code):
    model_url = f"https://currentmakers.com/products/colibri/{slug(model)}"
    doc_url   = f"https://currentmakers.com/docs/colibri/{slug(model)}"

    vendor_b = cstr(vendor)
    model_b  = cstr(model)
    murl_b   = cstr(model_url)
    durl_b   = cstr(doc_url)

    if len(pic_code) > PIC_ARM_SIZE:
        raise ValueError(f"pic code {len(pic_code)} bytes exceeds pic_arm ({PIC_ARM_SIZE})")

    # Consecutive pointers into the text area (absolute EEPROM offsets).
    vendor_name_ptr  = TEXT_AREA_START
    vendor_model_ptr = vendor_name_ptr + len(vendor_b)
    product_link_ptr = vendor_model_ptr + len(model_b)
    doc_link_ptr     = product_link_ptr + len(murl_b)

    text_blob = vendor_b + model_b + murl_b + durl_b
    if len(text_blob) > TEXT_AREA_SIZE:
        raise ValueError(f"text area overflow: {len(text_blob)} > {TEXT_AREA_SIZE}")

    eeprom = bytearray(EEPROM_SIZE)

    def put_u32(off, val):
        struct.pack_into("<I", eeprom, off, val & 0xFFFFFFFF)

    put_u32(OFF_FINGERPRINT,      FINGERPRINT)
    put_u32(OFF_SERIAL_NUMBER,    serial_number)
    put_u32(OFF_VENDOR_ID,        VENDOR_ID)
    put_u32(OFF_VENDOR_MODEL_ID,  vendor_model_id)
    put_u32(OFF_VENDOR_REVISION,  vendor_revision)

    put_u32(OFF_VENDOR_NAME_PTR,  vendor_name_ptr)
    put_u32(OFF_VENDOR_NAME_LEN,  len(vendor_b))
    put_u32(OFF_VENDOR_MODEL_PTR, vendor_model_ptr)
    put_u32(OFF_VENDOR_MODEL_LEN, len(model_b))

    put_u32(OFF_PRODUCT_LINK_PTR, product_link_ptr)
    put_u32(OFF_PRODUCT_LINK_LEN, len(murl_b))
    put_u32(OFF_DOC_LINK_PTR,     doc_link_ptr)
    put_u32(OFF_DOC_LINK_LEN,     len(durl_b))

    put_u32(OFF_CODE_PIC_LEN,     len(pic_code))

    eeprom[OFF_TEXT_AREA:OFF_TEXT_AREA + len(text_blob)] = text_blob
    eeprom[OFF_PIC_ARM:OFF_PIC_ARM + len(pic_code)]      = pic_code

    return eeprom


def auto_int(x: str) -> int:
    return int(x, 0)  # supports 0x.., 0.., decimal


def revision(x: str) -> int:
    """Pack 1-4 characters into a u32 (first char in the low byte, e.g. 'A' -> 0x41)."""
    b = x.encode("ascii")
    if not 1 <= len(b) <= 4:
        raise argparse.ArgumentTypeError("revision must be 1-4 ASCII characters")
    return int.from_bytes(b, "little")


def main():
    p = argparse.ArgumentParser(description="Build a Colibri IO EEPROM image.")
    p.add_argument("--model", required=True,
                   help="Model name, e.g. 'Colibri AIC'")
    p.add_argument("--vendor-model-id", required=True, type=auto_int,
                   help="Vendor model id (e.g. 0x0001)")
    p.add_argument("--vendor-revision", required=True, type=revision,
                   help="Vendor revision, 1-4 characters (e.g. 'A' or '1b')")
    p.add_argument("--serial", type=auto_int, default=None,
                   help="Serial number; if omitted, use/increment the per-model .serial file")
    p.add_argument("--vendor", default="CurrentMakers",
                   help="Vendor name (default: CurrentMakers)")
    p.add_argument("--pic", required=True, metavar="FILE",
                   help="Path to the PIC ARM code .bin file")
    p.add_argument("-o", "--output", default="",
                   help="Output .bin file (default: <pic-filename>.eeprom, '-' for stdout)")
    args = p.parse_args()

    with open(args.pic, "rb") as f:
        pic_code = f.read()

    serial_number = args.serial if args.serial is not None else next_serial(args.model)

    eeprom = build_eeprom(
        model=args.model,
        vendor_model_id=args.vendor_model_id,
        vendor_revision=args.vendor_revision,
        vendor=args.vendor,
        serial_number=serial_number,
        pic_code=pic_code,
    )

    if args.output == "-":
        sys.stdout.buffer.write(eeprom)
    else:
        if args.output == "":
            outfile = args.pic + ".eeprom"
        else:
            outfile = args.output
        with open(outfile, "wb") as f:
            f.write(eeprom)


if __name__ == "__main__":
    main()
