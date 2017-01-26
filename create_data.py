import bz2
import random
from os import getcwd
from os.path import join
from itertools import izip

n = 10000

random.seed()

cur = getcwd()
test_sizes = [64, 128, 256, 512, 1024, 2048, 4096]
test_files = [join(cur, "data", "n%i" % i) for i in test_sizes]

for bits, path in izip(test_sizes,test_files):
    data_file = bz2.BZ2File(path+".bz2", "wU")
    for _ in range(n):
        lhs = random.getrandbits(bits)
        rhs = random.getrandbits(bits)
        data_file.write("%i %i %i\n" % (lhs, rhs, lhs*rhs))
    data_file.close()
