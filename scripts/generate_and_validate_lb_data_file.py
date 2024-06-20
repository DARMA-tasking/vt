import subprocess
import argparse

def generate(vt_build, out_path):
    """
    Runs vt lb_iter example to generate LBDatafile
    """
    exe_path = vt_build + "/examples/collection/lb_iter"
    out_dir = "--vt_lb_data_dir=" + vt_build
    out_file = "--vt_lb_data_file=" + out_path
    
    args = (exe_path, "8", "1.0", "2", "--vt_lb", "--vt_lb_interval=1", "--vt_lb_name=RotateLB", "--vt_lb_data", "--vt_lb_data_compress=false", out_dir, out_file)
    runner = subprocess.Popen(args, stdout=subprocess.PIPE)
    runner.wait()

def validate(file_to_validate, reference_file):
    """
    Compares file to validate wih reference
    """


def main():
    parser = argparse.ArgumentParser()
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--generate", "-g", dest='generate', required=False, action='store_true')
    group.add_argument("--validate", "-v", dest='validate', required=False, action='store_true')

    parser.add_argument("--vt-source-dir", "-s", dest='vt_source_dir', required=True)
    parser.add_argument("--vt-build-dir", "-b", dest='vt_build_dir', required=True)

    parser.add_argument("--inout-file", "-i", dest='inout_file', required=True)
    parser.add_argument("--reference-file", "-r", dest='reference_file', required=False)
    args = parser.parse_args()

    if args.generate:
        generate(args.vt_build_dir, args.inout_file)
    if args.validate:
        validate(args.inout_file, args.reference_file)
    

if __name__ == '__main__':
    main()
