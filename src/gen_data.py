import random

total_size = 100000
with open("data.txt", 'w') as f:
    l = [i for i in range(1, 1000000)]
    random.shuffle(l)
    for num in l[:total_size]:
        f.write("%d\n" % num)
        # f.write("\n")

f.close()