import math
import timeit


def dig_sum2(n):
    return sum([int(x) for x in str(n)])

def dig_sum(n):
    return sum([ord(x)-48 for x in str(n)])

def dig_sum2_OBS(n):
    tot = 0
    while n > 0:
        tot += n % 10
        n = n // 10
    return tot

def dig_sum3_OBS(n, blocks = 10):
    cut = 10**blocks
    tot = 0
    while n > 0:
        rem = n % cut
        tot += dig_sum2(rem)
        n = n // cut
    return tot  
    
#(3*10**n + 1) ** e 
def digcalc(c,n,e):
    s = c*10**n + 1
    return dig_sum(s**e)

def digcalc2(c,n,e):
    cap = 10**n
    tot = 0

    for e2 in range(e + 1):
        s = (c ** e2) * math.comb(e,e2)
        if not s < cap:
            return
        tot += dig_sum(s)
    return tot

def digcalc3(c,n,e):
    cap = 10**n
    tot = 0
    run_exp = 1
    for e2 in range(e + 1):
        s = run_exp * math.comb(e,e2)
        if not s < cap:
            return
        tot += dig_sum(s)
        run_exp *= c
    return tot

def big_ass():
    for i in range(10):
        for j in range(10):
            for k in range(10):
                dg2 = digcalc2(i,j,k)
                #print(i,j,k,dg2)
                assert dg2 is None or digcalc3(i,j,k) == dg2


#print(timeit.timeit("dig_sum(BIG)", number=1000,globals=globals()))

goal = int(49080*math.log(10,3))
big_ass()
c1,c2 = 3,665829
f1 = lambda x: digcalc2(c1,c2,x)
f2 = lambda x: (f1(x) - 4*x)

i = goal
res = f2(i)
print(i, res, res % 7)
while res % 7 != 0:
    i -= 1
    res = f2(i)
    print(i, res, res % 7)

#for i in [2500,5000,10000,20000]:
    #print(f"f1-{i}", timeit.timeit(f"f1({i})", number=3, globals=globals()))
    #print(f"f2-{i}", timeit.timeit(f"f2({i})", number=3, globals=globals()))
    #print()
