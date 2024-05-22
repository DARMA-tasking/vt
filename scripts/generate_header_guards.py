import os
import re
import argparse as ap

def has_license_header(lines):
    """
    Check if the first two lines contain the license header start.
    """
    if len(lines) >= 2 and lines[0] == "/*\n" and lines[1] == "//@HEADER\n":
        return True
    return False

def find_existing_guards(lines):
    """
    Find the range of lines containing existing header guards.
    """
    ifndef_pattern = re.compile(r'#(?:if !defined|ifndef)\s+([A-Z0-9_]+)')
    define_pattern = re.compile(r'#define\s+([A-Z0-9_]+)')
    endif_pattern = re.compile(r'#endif')

    ifndef_start = None
    define_line = None
    endif_line = None

    for i, line in enumerate(lines):
        if ifndef_start is None:
            match = ifndef_pattern.match(line)
            if match:
                ifndef_start = i
        if ifndef_start is not None and define_line is None:
            match = define_pattern.match(line)
            if match:
                define_line = i
        if ifndef_start is not None and define_line is not None:
            match = endif_pattern.match(line)
            if match:
                endif_line = i

    if ifndef_start is not None and define_line is not None and endif_line is not None:
        return ifndef_start, endif_line
    return None, None

def generate_header_guard(file_path, root):
    with open(file_path, 'r') as file:
        lines = file.readlines()

    has_license = has_license_header(lines)

    # Create a header guard macro name based on the file name
    base_name = os.path.relpath(file_path, root).upper().replace('/', '_').replace('.', '_').replace('-', '_')
    guard_name = f'INCLUDED_{base_name}'

    # Add header guards
    ifndef_guard = f'#if !defined {guard_name}\n#define {guard_name}\n'
    endif_guard = f'#endif /*{guard_name}*/\n'

    # Detect existing guards
    start_line, end_line = find_existing_guards(lines)

    if has_license:
        license_content = ''.join(lines[:42])
        file_content = ''.join(lines[42:])
    else:
        license_content = ''
        file_content = ''.join(lines)

    if start_line is not None and end_line is not None:
        # Replace existing guards
        new_content = license_content + "\n" + ifndef_guard + ''.join(lines[start_line+2:end_line]) + endif_guard
    else:
        # Insert new guards
        new_content = license_content + "\n" + ifndef_guard + file_content + endif_guard

    with open(file_path, 'w') as file:
        file.write(new_content)

def main():
    parser = ap.ArgumentParser()
    parser.add_argument("--src_dir", "-s", dest='src_dir', required=True)
    args = parser.parse_args()

    base_dir = os.path.expanduser(args.src_dir)
    for root, _, files in os.walk(base_dir):
        for file in files:
            if file.endswith('.h'):
                generate_header_guard(os.path.join(root, file), base_dir)

if __name__ == '__main__':
    main()
