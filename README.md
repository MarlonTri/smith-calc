# View PDF

See "New Largest Smith Prime" paper (PDF) by Marlon Trifunovic at https://github.com/MarlonTri/smith-calc/blob/gh-pages/smith.pdf

# Code output
Assertions made in above PDF can be verified by running `smithCalc.cpp`. The output of the code is below:
```
Section 1:
c_{81525,k(81525)} =    2.815503...*10^49080
R49081 =                9.999999...*10^49080
c_{81526,k(81526)} =    1.126192...*10^49081

Section 2:
Let t0 = 1105923
c_{t0,k(t0)} =          8.596784...*10^665828
10^665829 =             1.000000...*10^665829
c_{t0+1,k(t0 + 1)} =    3.438713...*10^665829

Section 3:
Let t0 = 1105923
Recalculating Coefficients? = false
Loading Coefficients from ./3-665829-1105923.csv
S(M^{t0}) = 2508098743612
Sp(M^{t0}) = 4423692
S(M^{t0}) - Sp(M^{t0}) = 0 mod 7
(S(M^{t0}) - Sp(M^{t0}))/7 = 358299188560
```
Code runs near instantly when loading coefficients from file. Recalculating coefficients can be done by settings boolean `RECALCULATE` to `true`. It takes about 4 hours to calculate all t0=1105923 coefficients using all cores on an AMD Ryzen 7 5800X 8-Core Processor.