
# Count bugs

```
./count_bugs.sh "<directory>" |
awk -F ',' '{ sum1 += $2; sum2 += $3; sum3 += $4} END {print NR,sum1,sum2,sum3}'
```
