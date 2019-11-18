import random

total_size = 1000000
with open("data.txt", 'w') as f:
    l = [i for i in range(1, 10000000)]
    random.shuffle(l)
    for num in l[:total_size]:
        f.write("%d\n" % num)
        # f.write("\n")

f.close()