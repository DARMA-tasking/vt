import subprocess
import argparse
import json
import sys
from deepdiff import DeepDiff

def generate(vt_build, out_file_name):
    """
    Runs vt lb_iter example to generate LBDatafile
    """
    exe_path = vt_build + "/examples/collection/lb_iter"
    out_dir = "--vt_lb_data_dir=" + vt_build
    out_file = "--vt_lb_data_file=" + out_file_name

    args = (exe_path, "4", "1.0", "1", "--vt_lb", "--vt_lb_interval=1", "--vt_lb_name=RotateLB", "--vt_lb_data", "--vt_lb_data_compress=false", out_dir, out_file)
    runner = subprocess.Popen(args, stdout=subprocess.PIPE)
    exit_code = runner.wait()
    if exit_code != 0:
        sys.exit(1)

def validate(vt_build, file_to_validate, reference_file):
    """
    Compares file to validate wih reference
    """
    print("Comparing '" + file_to_validate + "' with reference file '" + reference_file + "'.")

    with open(vt_build + "/" + file_to_validate) as val_file, open(reference_file) as ref_file:
        to_validate = json.load(val_file)
        reference = json.load(ref_file)
        diff = DeepDiff(to_validate, reference, report_repetition=True, math_epsilon=0.1)
        is_valid = not len(diff.affected_paths)

        if not is_valid:
            sys.stderr.write("Detected differences:\n")
            json.dump(str(diff), sys.stderr, indent=4)
            sys.stderr.write("\n")
            sys.stderr.flush()
            sys.exit(1)
        else:
            print("Comparison OK.")

def main():
    parser = argparse.ArgumentParser()
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--generate", "-g", dest='generate', required=False, action='store_true')
    group.add_argument("--validate", "-v", dest='validate', required=False, action='store_true')

    parser.add_argument("--vt-build-dir", "-b", dest='vt_build_dir', required=True)
    parser.add_argument("--file-name", "-f", dest='file_name', required=True)
    parser.add_argument("--reference-file", "-r", dest='reference_file', required=False)
    args = parser.parse_args()

    if args.generate:
        generate(args.vt_build_dir, args.file_name)
    if args.validate:
        validate(args.vt_build_dir, args.file_name, args.reference_file)


if __name__ == '__main__':
    main()
