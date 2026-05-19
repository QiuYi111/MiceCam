#!/usr/bin/env python3
"""Validate a plugin.json against the MiceCam plugin manifest schema."""

import json
import sys

try:
    import jsonschema
except ImportError:
    print("ERROR: jsonschema package is required. Install with: pip install jsonschema")
    sys.exit(1)


def load_json(path):
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def validate_manifest(manifest_path, schema_path):
    manifest = load_json(manifest_path)
    schema = load_json(schema_path)

    validator = jsonschema.Draft7Validator(schema)
    errors = sorted(validator.iter_errors(manifest), key=lambda e: e.path)

    if errors:
        for err in errors:
            path = " -> ".join(str(p) for p in err.path) if err.path else "<root>"
            print(f"  FAIL: {path}: {err.message}")
        print(f"\n{len(errors)} validation error(s) found.")
        return False
    else:
        print(f"  PASS: {manifest_path}")
        return True


def main():
    if len(sys.argv) != 3:
        print("Usage: validate_plugin_manifest.py <manifest.json> <schema.json>")
        sys.exit(2)

    manifest_path = sys.argv[1]
    schema_path = sys.argv[2]

    print(f"Validating {manifest_path} against {schema_path}...")
    if validate_manifest(manifest_path, schema_path):
        print("\nValidation passed.")
        sys.exit(0)
    else:
        sys.exit(1)


if __name__ == "__main__":
    main()
