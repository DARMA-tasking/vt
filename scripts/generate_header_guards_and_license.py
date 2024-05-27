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


def generate_license(file_path, license_path):
    with open(file_path, 'r') as file:
        lines = file.readlines()

    with open(license_path, 'r') as license_file:
        license_file = license_file.readlines()

    file_name = os.path.basename(file_path)
    lenright = int((80 - 2 - len(file_name))/2)
    license_file[3] = f"//{' ' :<{lenright}}{file_name}\n"

    # Remove leading empty lines
    non_empty_lines_start = 0
    for i, line in enumerate(lines):
        if line.strip() != '':
            non_empty_lines_start = i
            break

    trimmed_lines = lines[non_empty_lines_start:]

    existing_license_start = -1
    existing_license_end = -1
    for i, line in enumerate(trimmed_lines):
        if line.startswith("//@HEADER"):
            if existing_license_start == -1:
                existing_license_start = i
            elif existing_license_end == -1:
                existing_license_end = i
                break

    if existing_license_start != -1:
        updated_file = trimmed_lines[:existing_license_start] + license_file + trimmed_lines[existing_license_end + 1:]
    else:
        updated_file = ['/*\n'] + license_file + ['*/\n'] + trimmed_lines

    with open(file_path, 'w') as file:
        file.writelines(updated_file)



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
    parser.add_argument("--license", "-l", dest='license_path', required=True)
    args = parser.parse_args()

    src_dir_abs = os.path.abspath(os.path.expanduser(args.src_dir))
    license_path_abs = os.path.abspath(os.path.expanduser(args.license_path))
    for root, _, files in os.walk(src_dir_abs):
        for file in files:
            if file.endswith('.h'):
                generate_license(os.path.join(root, file), license_path_abs)
                generate_header_guard(os.path.join(root, file), src_dir_abs)

if __name__ == '__main__':
    main()
