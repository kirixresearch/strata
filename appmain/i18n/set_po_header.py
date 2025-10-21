#!/usr/bin/env python3
"""
Script to upsert header fields in .po files.
Usage: python set_po_header.py messages.po --header "Language: en" --header "MIME-Version: 1.1"
"""

import argparse
import re
import sys
from typing import Dict, List, Tuple


def parse_po_header(content_bytes: bytes) -> Tuple[bytes, Dict[str, str], bytes, bytes]:
    """
    Parse a .po file content and extract the header section.
    Returns: (header_start_bytes, header_fields, rest_of_file_bytes, line_ending_bytes)
    """
    # Detect line ending
    if b'\r\n' in content_bytes:
        line_ending_bytes = b'\r\n'
    elif b'\n' in content_bytes:
        line_ending_bytes = b'\n'
    else:
        line_ending_bytes = b'\n'  # default
    
    lines = content_bytes.split(line_ending_bytes)
    
    # Find the start of the header (msgid "" followed by msgstr "")
    header_start_idx = -1
    for i, line in enumerate(lines):
        if line.strip() == b'msgid ""':
            if i + 1 < len(lines) and lines[i + 1].strip() == b'msgstr ""':
                header_start_idx = i
                break
    
    if header_start_idx == -1:
        raise ValueError("Could not find header section in .po file")
    
    # Find the end of the header (empty line or non-quoted line)
    header_end_idx = header_start_idx + 2
    header_fields = {}
    
    # Parse header fields
    for i in range(header_start_idx + 2, len(lines)):
        line = lines[i].strip()
        
        # Empty line or non-quoted line ends the header
        if not line or not line.startswith(b'"'):
            header_end_idx = i
            break
            
        # Parse header field: "Field-Name: value\n"
        if line.endswith(b'\\n"'):
            field_line = line[1:-3]  # Remove leading " and trailing \n"
            if b':' in field_line:
                field_name, field_value = field_line.split(b':', 1)
                header_fields[field_name.strip().decode('utf-8')] = field_value.strip().decode('utf-8')
    
    # Reconstruct the parts
    header_start_bytes = line_ending_bytes.join(lines[:header_start_idx + 2])
    rest_of_file_bytes = line_ending_bytes.join(lines[header_end_idx:])
    
    return header_start_bytes, header_fields, rest_of_file_bytes, line_ending_bytes


def format_header_fields(fields: Dict[str, str], line_ending_bytes: bytes) -> bytes:
    """Format header fields as .po header lines."""
    lines = []
    for field_name, field_value in fields.items():
        lines.append(f'"{field_name}: {field_value}\\n"'.encode('utf-8'))
    return line_ending_bytes.join(lines)


def upsert_header_fields(existing_fields: Dict[str, str], new_fields: Dict[str, str]) -> Dict[str, str]:
    """Upsert new fields into existing fields."""
    result = existing_fields.copy()
    result.update(new_fields)
    return result


def main():
    parser = argparse.ArgumentParser(description='Upsert header fields in .po files')
    parser.add_argument('po_file', help='Path to the .po file')
    parser.add_argument('--header', action='append', dest='headers', 
                       help='Header field to upsert (format: "Field-Name: value")')
    
    args = parser.parse_args()
    
    if not args.headers:
        print("No header fields specified. Use --header 'Field-Name: value' to add/update fields.")
        return 1
    
    try:
        # Read the .po file in binary mode to preserve line endings
        with open(args.po_file, 'rb') as f:
            content_bytes = f.read()
        
        # Parse the header
        header_start_bytes, existing_fields, rest_of_file_bytes, line_ending_bytes = parse_po_header(content_bytes)
        
        # Parse new header fields from command line
        new_fields = {}
        for header_arg in args.headers:
            if ':' not in header_arg:
                print(f"Invalid header format: {header_arg}. Expected 'Field-Name: value'")
                return 1
            field_name, field_value = header_arg.split(':', 1)
            new_fields[field_name.strip()] = field_value.strip()
        
        # Upsert the fields
        updated_fields = upsert_header_fields(existing_fields, new_fields)
        
        # Format the updated content
        header_fields_bytes = format_header_fields(updated_fields, line_ending_bytes)
        updated_content_bytes = header_start_bytes + line_ending_bytes + header_fields_bytes + line_ending_bytes + rest_of_file_bytes
        
        # Write back to file in binary mode
        with open(args.po_file, 'wb') as f:
            f.write(updated_content_bytes)
        
        print(f"Successfully updated {args.po_file}")
        print("Updated fields:")
        for field_name, field_value in new_fields.items():
            print(f"  {field_name}: {field_value}")
        
        return 0
        
    except FileNotFoundError:
        print(f"Error: File '{args.po_file}' not found")
        return 1
    except Exception as e:
        print(f"Error: {e}")
        return 1


if __name__ == '__main__':
    sys.exit(main())
