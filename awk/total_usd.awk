#!/usr/bin/awk -f
NR > 1 { total += $5 }
END {
    printf "Total USD Value: %.2f\n", total
}
