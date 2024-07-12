import argparse
import json
import sys
from deepdiff import DeepDiff

def compare(file_to_validate, reference_file):
    """
    Compares file to validate wih reference
    """
    with open(file_to_validate) as val_file, open(reference_file) as ref_file:
        to_validate = json.load(val_file)
        reference = json.load(ref_file)
        diff = DeepDiff(to_validate, reference, report_repetition=True, math_epsilon=0.1)

        message = f"Comparing '{file_to_validate}' with reference file '{reference_file}'..."
        if diff:
            sys.stderr.write(f"{message} Failed!\n")
            sys.stderr.write("Detected differences:\n")
            json.dump(str(diff), sys.stderr, indent=4)
            sys.stderr.write("\n")
            sys.stderr.flush()
            sys.exit(1)
        else:
            print(f"{message} Status OK.")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--file-to-check", "-f", dest='file', required=True)
    parser.add_argument("--reference-file", "-r", dest='reference_file', required=True)
    args = parser.parse_args()

    compare(args.file, args.reference_file)


if __name__ == '__main__':
    main()
