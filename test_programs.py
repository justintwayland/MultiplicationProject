from os import listdir,getcwd,putenv,makedirs
from os.path import isfile, join, basename, isdir, exists
from re import compile, search
import argparse
import subprocess as sub
import bz2
from tempfile import TemporaryFile
parser = argparse.ArgumentParser(description="Time multiplication algorithms.")
parser.add_argument('-i', '--input', nargs="?", default=join(getcwd(), "data"),
                        help="Where to find data files. Defaults to data folder in current working directory")
parser.add_argument('programs', nargs=argparse.REMAINDER, help="Path to a valid program for integer multiplication")
args = parser.parse_args()
if not exists(args.input): raise ("input path %s does not exist" % input)
def get_tests(p):
    stream, test_list = bz2.BZ2File(p), []
    for line in stream: test_list.append(line.split())
    stream.close(); return test_list
def is_test_file(name): return search("n([0-9]+)\.bz2", name)
input_files = [join(getcwd(), "data", f) for f in listdir(join(getcwd(), "data"))
                   if isfile(join(getcwd(), "data", f)) and is_test_file(basename(f))]
test_runs = {}
for file_path in input_files: test_runs[search("n([0-9]+)", file_path).group(1)] = get_tests(file_path)
output, cur_output_pos,time_string,cur_timestring_pos = TemporaryFile('w+'), 0, TemporaryFile('w+'), 0
time_regex = compile("real\s*([0-9]+\.[0-9]+)\s*user\s*([0-9]+\.[0-9]+)\s*sys\s*([0-9]+\.[0-9]+)")
def dotest(program, inputs):
    # get the inputs
    x, y = inputs[0], inputs[1]; global output, time_string, cur_output_pos, cur_timestring_pos
    sub.call(["./ctime","./"+program,x,y], stdout=output, stderr=time_string)
    output.seek(cur_output_pos); time_string.seek(cur_timestring_pos)
    result = {'x':x, 'y':y, 'product':output.readline()[:-1], 'target':inputs[2]}
    times = time_regex.search(time_string.readline())
    if times is None:
        time_string.seek(cur_timestring_pos); raise Exception(time_string.readline())
    else: result["times"] = times.group(1,2,3)
    cur_output_pos, cur_timestring_pos = output.tell(), time_string.tell()
    return result
results = {}
# results organization: bits -> algorithm -> run results
for k, v in test_runs.items(): # for each test case...
    for program in args.programs:
        lines, run_result = ["x,y,result,target,realtime,usertime,systemtime"],[dotest(program, ins) for ins in v]
        for result in run_result:
            x,y,product,target = result["x"], result["y"], result["product"], result["target"]
            real,user,sys = result["times"]
            lines.append(",".join([x, y, product, target, real, user, sys]))
        with open(join(getcwd(), "data", program, "%s.csv" % k), "w") as f: f.write("\n".join(lines))
